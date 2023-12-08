#include <relay.hpp>
#include <server.hpp>
#include <json.hpp>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>
#include <RakSleep.h>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <csignal>
#include <atomic>
#include <fstream>

using nlohmann::json;
using MCPIRelay::Relay;
std::atomic<bool> isRunning(true);

void exit_program(int signal) { isRunning = false; }

Relay::Relay(uint16_t port, uint16_t server_port, uint32_t max_clients) {
	std::signal(SIGINT, exit_program);

	std::cout << "Starting relay" << std::endl;
	load_servers();
	this->command_parser = new MCPIRelay::CommandParser();
	this->peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::SocketDescriptor sd(port, 0);
	peer->Startup(max_clients, &sd, 1);
    peer->SetMaximumIncomingConnections(max_clients);
	// Set the MOTD
	RakNet::RakString data = "MCCPP;MINECON;RELAY";
        RakNet::BitStream stream;
        data.Serialize(&stream);
        peer->SetOfflinePingResponse((const char *)stream.GetData(), stream.GetNumberOfBytesUsed());

    this->server_peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::SocketDescriptor ssd(server_port, 0);
	server_peer->Startup(max_clients, &ssd, 1);
	server_peer->SetMaximumIncomingConnections(max_clients);

    while (isRunning) {
    	for (int i = 0; i < 100; i++) {
			handle_client_networking();
    	}
    	handle_chat_file();
    	if (empty_packets >= 100) {
    		RakSleep(1);
    	}
    }
}

Relay::~Relay() {
	std::cout << "Stopping relay" << std::endl;
	for (const auto& pair : clients) {
		//RakNet::RakNetGUID clientGUID = pair.first;
		MCPIRelay::Client* client = pair.second;
		std::cout << "Disconnecting client " << client->guid.ToString() << std::endl;
		client->post_to_chat("Relay is turning off, disconnecting.");
		delete client;
	}
	RakNet::RakPeerInterface::DestroyInstance(peer);
}

void Relay::load_servers() {
	std::ifstream f("servers.json");
	json servers = json::parse(f);
	//std::cout << servers.dump(4) << std::endl;
	for(auto& object : servers.items()) {
		MCPIRelay::Server *server = new MCPIRelay::Server(this, object.key(), object.value()["host"], object.value()["port"]);
		server->direct = object.value()["direct"];
		server->require_authorized_clients = object.value()["auth_only"];
		for(auto& area : object.value()["protected"].items()) {
			int32_t x1 = area.value()["pos1"][0];
			int32_t y1 = area.value()["pos1"][1];
			int32_t z1 = area.value()["pos1"][2];
			int32_t x2 = area.value()["pos2"][0];
			int32_t y2 = area.value()["pos2"][1];
			int32_t z2 = area.value()["pos2"][2];
			server->protected_areas[area.key()] = new MCPIRelay::Area(x1, y1, z1, x2, y2, z2);

			if(area.value().contains("destination")) {
				server->server_gateways_by_area[area.key()] = area.value()["destination"];
			}
		}
		this->servers[object.key()] = server;
	}
}


void Relay::handle_client_networking() {
	// Handle server > relay communications
	for(const auto& pair : clients) {
		//RakNet::RakNetGUID clientGUID = pair.first;
		MCPIRelay::Client* client = pair.second;
		client->handle_downstream_packets();
	}
	// Handle client > relay communications
	RakNet::Packet *packet;
	packet = peer->Receive();
	if(!packet) {
		this->empty_packets++;
		return;
	}
	this->empty_packets = 0;

	if(packet->bitSize != 0) {
		RakNet::BitStream receive_stream(packet->data, (int)packet->bitSize, false);

		uint8_t packet_id;
		receive_stream.Read<uint8_t>(packet_id);
		switch (packet_id) {
		case ID_NEW_INCOMING_CONNECTION:
			std::cout << "Client " << packet->guid.ToString() << " connected" << std::endl;

			if (this->clients.count(packet->guid) == 0) {
				MCPIRelay::Client *client;
				try {
					client = new MCPIRelay::Client(this, this->servers["main"]);
				} catch(const std::exception& e) {
					std::cerr << "Failed to create client: " << e.what() << std::endl;
					break;
				}
				/*
				 * Auth system integration
				 * Currently disabled due to not being configurable.
				 * For now everyone is granted the rank of admin
				 * surely a perfectly balanced thing with no exploits.
				 * TODO: Make this configurable / depend on the server

				client->setup_perms_by_ip(packet->systemAddress.ToString(false));

				if (!client->authorized) {
					std::cout << "Client not authorized!" << std::endl;
					delete client;
					break;
				}
				*/
				// Give admin rights
				client->authorized = true;
				client->rank = 3;

				client->log() << "connect " << packet->systemAddress.ToString() << std::endl;

				this->servers["main"]->clients.insert(client);

				client->guid = packet->guid;
				this->clients[packet->guid] = client;
			} else {
				std::cout << "Client already exists!" << std::endl;
			}
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::cout << packet->guid.ToString() << " attempted to connect to full relay." << std::endl;
			break;
		case ID_DISCONNECTION_NOTIFICATION:
		case ID_CONNECTION_LOST:
			std::cout << "Client " << packet->guid.ToString() << " disconnected" << std::endl;
			if (this->clients.count(packet->guid) != 0) {
				this->clients[packet->guid]->log() << "leave" << std::endl;
				delete this->clients[packet->guid];
				this->clients.erase(packet->guid);
			}
			break;
		default:
			if (this->clients.count(packet->guid) != 0) {
				this->clients[packet->guid]->handle_upstream_packet(packet);
			}
			break;
		}
	}
	peer->DeallocatePacket(packet);
}

void Relay::handle_chat_file() {
	std::ifstream chat_file("chat.txt");

	std::string line;
	while (std::getline(chat_file, line)) {
		// Send to everyone
		if (line != "") {
			post_to_chat(line);
		}
	}
	chat_file.close();

	std::ofstream file("chat.txt", std::ios::trunc); // Open the file in truncation mode to clear it
	file.close();
}

void Relay::post_to_chat(std::string message) {
	for(const auto& pair : this->clients) {
		pair.second->post_to_chat(message);
	}
}
