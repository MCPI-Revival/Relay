#pragma once

#include <area.hpp>

#include <BitStream.h>
#include <RakPeerInterface.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

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

        bool require_authorized_clients = false;

        std::unordered_map<std::string, MCPIRelay::Area *> protected_areas;
        std::unordered_map<std::string, std::string> server_gateways_by_area;

        std::unordered_set<MCPIRelay::Client *> clients;

        bool is_position_reserved(int32_t x, int32_t y, int32_t z);
        std::string check_server_gateways(int32_t x, int32_t y, int32_t z);
        std::pair<RakNet::RakPeerInterface *, RakNet::RakNetGUID> create_connection();
        void connect_client(MCPIRelay::Client *client);
        void disconnect_client(MCPIRelay::Client *client);
        void post_to_chat(std::string msg);

        Server(Relay *relay, std::string name, std::string host, uint16_t port);
        ~Server(){};
    };
}
