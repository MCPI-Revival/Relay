#include <server.hpp>
#include <client.hpp>
#include <json.hpp>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>
#include <RakSleep.h>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <stdexcept>
#include <iostream>
#include <random>

using nlohmann::json;
using MCPIRelay::Server;


Server::Server(Relay *relay, RakNet::RakPeerInterface* peer, RakNet::RakNetGUID guid) {
	this->relay = relay;
	this->temporary_peer = peer;
	this->temporary_guid = guid;
	this->direct = false;
}

Server::Server(Relay *relay, std::string name, std::string host, uint16_t port) {
	this->relay = relay;
	this->name = name;
	this->host = host;
	this->port = port;
	this->direct = true;
	this->ready = true;
}

void Server::connect_client(MCPIRelay::Client *client) {
	std::cout << client->username << " is connecting to " << this->name << "\n";
	if(!this->ready) {
		client->post_to_chat("Cannot connect: server not ready.");
		throw std::runtime_error("Server not ready.");
		return;
	}
	if(this->require_authorized_clients && !client->authorized) {
		client->post_to_chat("Cannot connect: client not authorized.");
		throw std::runtime_error("Client not authorized.");
		return;
	}

	std::pair<RakNet::RakPeerInterface*, RakNet::RakNetGUID> connection = create_connection();

	client->server->disconnect_client(client);
	client->server = this;
	client->switch_connection(connection.first, connection.second);
	this->clients.insert(client);
}

std::pair<RakNet::RakPeerInterface*, RakNet::RakNetGUID> Server::create_connection() {
	if(this->direct) {
		RakNet::RakPeerInterface *targetPeer = RakNet::RakPeerInterface::GetInstance();
		RakNet::SocketDescriptor sd(0, 0);
		targetPeer->Startup(1, &sd, 1);
		targetPeer->SetMaximumIncomingConnections(1);
		targetPeer->Connect(this->host.c_str(), this->port, nullptr, 0);

		bool awaiting_answer = true;
		RakNet::RakNetGUID targetGuid;
		RakNet::Packet *temp_packet;
		while (awaiting_answer) {
			temp_packet = targetPeer->Receive();
			if (!temp_packet) continue;
			uint8_t temp_packet_id;
			targetGuid = temp_packet->guid;
			RakNet::BitStream temp_receive_stream(temp_packet->data, (int)temp_packet->bitSize, false);
			temp_receive_stream.Read<uint8_t>(temp_packet_id);
			if (temp_packet_id == ID_CONNECTION_REQUEST_ACCEPTED) {
				awaiting_answer = false;
			} else {
				if ((int)temp_packet_id == ID_CONNECTION_ATTEMPT_FAILED) {
					awaiting_answer = false;
					throw std::runtime_error("Connection attempt failed.");
				}
			}
			RakSleep(1);
		}
		return std::make_pair(targetPeer, targetGuid);
	} else {
		// TODO: Remove this
		return std::make_pair(this->temporary_peer, this->temporary_guid);
	}
}

bool Server::is_position_reserved(int32_t x, int32_t y, int32_t z) {
	if(protected_areas.size() == 0) return false;
	for(const auto& pair : protected_areas) {
		if(pair.second->is_inside(x, y, z))
			return true;
	}
	return false;
}

std::string Server::check_server_gateways(int32_t x, int32_t y, int32_t z) {
	if(protected_areas.size() == 0) return "";
	for(const auto& pair : protected_areas) {
		if((this->server_gateways_by_area.count(pair.first) > 0) && pair.second->is_inside(x, y, z))
			return server_gateways_by_area[pair.first];
	}
	return "";
}

void Server::disconnect_client(MCPIRelay::Client *client) {
	this->clients.erase(client);
}

template <typename T>
void Server::send_downstream_packet(T &packet) {
	// Relay > server
	RakNet::BitStream send_stream;
	send_stream.Write<uint8_t>(packet.packet_id);
	packet.serialize_body(&send_stream);
	this->temporary_peer->Send(&send_stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, this->temporary_guid, false);
}
