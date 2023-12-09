#pragma once

#include <client.hpp>
#include <commands.hpp>
#include <server.hpp>

#include <RakPeerInterface.h>

#include <cstdint>
#include <map>
#include <vector>

namespace MCPIRelay {
    class Client;

    class Relay {
    public:
        RakNet::RakPeerInterface *peer;
        RakNet::RakPeerInterface *server_peer;
        CommandParser *command_parser;
        std::map<RakNet::RakNetGUID, Client *> clients;
        std::map<RakNet::RakNetGUID, Server *> server_peers;
        std::map<std::string, Server *> servers;
        int empty_packets = 0;

        void load_servers();
        void load_tokens();
        void handle_client_networking();
        void handle_chat_file();

        void post_to_chat(std::string message);

        Relay(uint16_t port, uint16_t server_port, uint32_t max_clients);
        ~Relay();
    };
}
