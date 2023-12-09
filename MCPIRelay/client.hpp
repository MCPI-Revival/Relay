#pragma once

#include <entity_mapper.hpp>
#include <server.hpp>

#include <BitStream.h>
#include <relay.hpp>

#include <map>
#include <string>
#include <unordered_set>

namespace MCPIRelay {
    class Relay;

    class Client {
    public:
        MCPIRelay::Relay *relay;
        RakNet::RakNetGUID guid;
        RakNet::RakPeerInterface *targetPeer;
        RakNet::RakNetGUID targetGuid;
        std::string username = "???";
        bool authorized = false;
        int rank = 0;
        std::string token_name = "";
        bool debug = false;
        bool god_mode = false;
        // 0 - unwanted
        // 1 - untrusted
        // 2 - trusted
        // 3 - admin

        int32_t entity_id;
        std::unordered_set<int32_t> known_entity_ids;
        std::unordered_set<int32_t> known_player_entity_ids;
        MCPIRelay::Mapper *mapper;
        MCPIRelay::Server *server;

        void connect_to_instance(MCPIRelay::Server &server);
        void handle_upstream_packet(RakNet::Packet *packet);
        template <typename T>
        void send_downstream_packet(T &packet);
        template <typename T>
        void send_upstream_packet(T &packet);

        void handle_downstream_packets();

        void post_to_chat(std::string message);
        void reload_chunks();
        void remove_entities();

        void login(RakNet::RakPeerInterface *targetPeer, RakNet::RakNetGUID targetGuid);
        void switch_connection(RakNet::RakPeerInterface *targetPeer, RakNet::RakNetGUID targetGuid);
        void set_connection(std::pair<RakNet::RakPeerInterface *, RakNet::RakNetGUID> connection);
        void setup_perms_by_ip(std::string ip);

        std::ostream &log();

        Client(Relay *relay, Server *server);
        ~Client();
    };
}
