#pragma once

#include <area.hpp>

#include <RakPeerInterface.h>
#include <BitStream.h>

#include <string>
#include <unordered_set>
#include <unordered_map>

namespace MCPIRelay {
	class Area;
	class Client;
	class Relay;

	class Server {
	public:
		MCPIRelay::Relay *relay;
		std::string host = "localhost";
		uint16_t port = 19132;
		std::string name = "???";

		RakNet::RakPeerInterface* temporary_peer;
		RakNet::RakNetGUID temporary_guid;

		bool ready = false;
		int64_t awaiting_token;
		bool require_authorized_clients = false;
		bool direct = true;

		std::unordered_map<std::string, MCPIRelay::Area	*> protected_areas;
		std::unordered_map<std::string, std::string> server_gateways_by_area;

		std::unordered_set<MCPIRelay::Client *> clients;

		bool is_position_reserved(int32_t x, int32_t y, int32_t z);
		std::string check_server_gateways(int32_t x, int32_t y, int32_t z);
		std::pair<RakNet::RakPeerInterface*, RakNet::RakNetGUID> create_connection();
		void connect_client(MCPIRelay::Client *client);
		void disconnect_client(MCPIRelay::Client *client);
		void post_to_chat(std::string msg);

		template <typename T>
		void send_downstream_packet(T &packet);
		
		Server(Relay *relay, RakNet::RakPeerInterface* peer, RakNet::RakNetGUID guid);
		Server(Relay *relay, std::string name, std::string host, uint16_t port);
		~Server() {};
	};
}
