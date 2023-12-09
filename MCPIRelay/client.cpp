#include <client.hpp>
#include <entity_mapper.hpp>
#include <json.hpp>

#include <BitStream.h>
#include <MessageIdentifiers.h>
#include <RakPeerInterface.h>
#include <RakSleep.h>

#include <network/packets/add_entity_packet.hpp>
#include <network/packets/add_item_entity_packet.hpp>
#include <network/packets/add_mob_packet.hpp>
#include <network/packets/add_painting_packet.hpp>
#include <network/packets/chat_packet.hpp>
#include <network/packets/hurt_armor_packet.hpp>
#include <network/packets/login_packet.hpp>
#include <network/packets/login_status_packet.hpp>
#include <network/packets/message_packet.hpp>
#include <network/packets/move_player_packet.hpp>
#include <network/packets/place_block_packet.hpp>
#include <network/packets/player_equipment_packet.hpp>
#include <network/packets/ready_packet.hpp>
#include <network/packets/remove_block_packet.hpp>
#include <network/packets/remove_entity_packet.hpp>
#include <network/packets/remove_player_packet.hpp>
#include <network/packets/request_chunk_packet.hpp>
#include <network/packets/set_health_packet.hpp>
#include <network/packets/start_game_packet.hpp>
#include <network/packets/take_item_entity_packet.hpp>
#include <network/packets/tile_event_packet.hpp>
#include <network/packets/update_block_packet.hpp>
#include <network/packets/use_item_packet.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using MCPIRelay::Client;
using nlohmann::json;
using RoadRunner::network::packets::AddEntityPacket;
using RoadRunner::network::packets::AddItemEntityPacket;
using RoadRunner::network::packets::AddMobPacket;
using RoadRunner::network::packets::AddPaintingPacket;
using RoadRunner::network::packets::ChatPacket;
using RoadRunner::network::packets::HurtArmorPacket;
using RoadRunner::network::packets::LoginPacket;
using RoadRunner::network::packets::LoginStatusPacket;
using RoadRunner::network::packets::MessagePacket;
using RoadRunner::network::packets::MovePlayerPacket;
using RoadRunner::network::packets::PlaceBlockPacket;
using RoadRunner::network::packets::PlayerEquipmentPacket;
using RoadRunner::network::packets::ReadyPacket;
using RoadRunner::network::packets::RemoveBlockPacket;
using RoadRunner::network::packets::RemoveEntityPacket;
using RoadRunner::network::packets::RequestChunkPacket;
using RoadRunner::network::packets::SetHealthPacket;
using RoadRunner::network::packets::StartGamePacket;
using RoadRunner::network::packets::TakeItemEntityPacket;
using RoadRunner::network::packets::TileEventPacket;
using RoadRunner::network::packets::UpdateBlockPacket;
using RoadRunner::network::packets::UseItemPacket;

RakNet::Packet *packet;

std::unordered_set<uint8_t> add_entity_packets = {
    AddMobPacket::packet_id,
    AddEntityPacket::packet_id,
    AddItemEntityPacket::packet_id,
    AddPaintingPacket::packet_id};

Client::Client(Relay *relay, Server *server) {
    this->relay = relay;
    this->server = server;
    set_connection(server->create_connection());
    this->entity_id = 0;
    this->mapper = new MCPIRelay::Mapper();
}

void Client::set_connection(std::pair<RakNet::RakPeerInterface *, RakNet::RakNetGUID> connection) {
    this->targetPeer = connection.first;
    this->targetGuid = connection.second;
}

Client::~Client() {
    // std::cout << "Destroying relay > server peer.\n";
    this->relay->peer->CloseConnection(this->guid, true, 0, IMMEDIATE_PRIORITY);
    this->targetPeer->CloseConnection(this->targetGuid, true, 0, IMMEDIATE_PRIORITY);
    this->targetPeer->Shutdown(100, 0, IMMEDIATE_PRIORITY);
    RakNet::RakPeerInterface::DestroyInstance(this->targetPeer);
    delete this->mapper;
}

void Client::handle_upstream_packet(RakNet::Packet *packet) {
    if (this->token_name == "") {
        std::cout << "Empty token name, using guid instead." << std::endl;
        this->token_name = this->guid.ToString();
    }
    // Client > relay
    if (!packet) return;
    if (packet->bitSize != 0) {
        RakNet::BitStream input_stream(packet->data, BITS_TO_BYTES((int)packet->bitSize), true);
        RakNet::BitStream stream;

        this->mapper->map_upstream_bitstream(input_stream, stream);

        uint8_t packet_id;
        stream.Read<uint8_t>(packet_id);
        if (packet_id == LoginPacket::packet_id) {
            LoginPacket login;
            login.deserialize_body(&stream);
            this->username = login.username.C_String();

            log() << "join " << this->username << std::endl;

            // std::cout << "Username:" << this->username << "\n";
            // this->username += "#0000";
            login.username = RakNet::RakString(this->username.c_str());
            send_upstream_packet(login);
            return;
        } else if (packet_id == ChatPacket::packet_id) {
            ChatPacket msg;
            msg.deserialize_body(&stream);

            std::string text = msg.message.C_String();
            std::istringstream iss(text);
            std::vector<std::string> arguments;
            std::string argument;

            while (iss >> argument) {
                arguments.push_back(argument);
            }

            if (arguments[0][0] == '/') {
                log() << "command " << text << std::endl;
                this->relay->command_parser->parse_command(this, arguments);
                return;
            }

            log() << "chat " << text << std::endl;
        } else if (packet_id == RemoveBlockPacket::packet_id) {
            RemoveBlockPacket remove_block;
            remove_block.deserialize_body(&stream);
            int32_t x, y, z;
            x = remove_block.x;
            y = remove_block.y;
            z = remove_block.z;

            log() << "break " << x << "," << y << "," << z << std::endl;

            /*
            post_to_chat("Cannot interact here!");
            UpdateBlockPacket dont;
            dont.x = x;
            dont.y = y;
            dont.z = z;
            dont.block = 7; //Bedrock
            dont.meta = 0;
            send_downstream_packet(dont);
            return;
            */
        } else if (packet_id == UseItemPacket::packet_id) {
            UseItemPacket use_item;
            use_item.deserialize_body(&stream);
            // If compass is used, show debug
            if (use_item.block == 345) {
                post_to_chat("- XYZ:" + std::to_string(use_item.x) + "," + std::to_string(use_item.y) + "," + std::to_string(use_item.z));
                post_to_chat("- fXYZ:" + std::to_string(use_item.fx) + "," + std::to_string(use_item.fy) + "," + std::to_string(use_item.fz));
                post_to_chat("- Face:" + std::to_string(use_item.face));
            } else if (use_item.face != 255) {
                int32_t x, y, z;
                x = use_item.x;
                y = use_item.y;
                z = use_item.z;

                std::string target = this->server->check_server_gateways(x, y, z);
                if (target != "") {
                    this->relay->servers[target]->connect_client(this);
                    return;
                }

                switch (use_item.face) {
                case 0:
                    y--;
                    break;
                case 1:
                    y++;
                    break;
                case 2:
                    z--;
                    break;
                case 3:
                    z++;
                    break;
                case 4:
                    x--;
                    break;
                case 5:
                    x++;
                    break;
                }

                log() << "place " << use_item.block << ":" << (int)use_item.meta << " " << x << "," << y << "," << z << std::endl;

                if (this->server->is_position_reserved(x, y, z)) {
                    post_to_chat("Cannot interact here!");
                    UpdateBlockPacket dont;
                    dont.x = x;
                    dont.y = y;
                    dont.z = z;
                    dont.block = 0;
                    dont.meta = 0;
                    send_downstream_packet(dont);
                    return;
                }
            }
        } else if (packet_id == MovePlayerPacket::packet_id) {
            MovePlayerPacket move_player;
            move_player.deserialize_body(&stream);
            bool valid = true;

            // Ensure entity id is that of the player
            if (move_player.entity_id != this->mapper->map_upstream(0)) {
                // Attempt to move a different entity
                if (this->debug) post_to_chat("EID did not match" + std::to_string(move_player.entity_id) + " " + std::to_string(this->mapper->map_upstream(0)));
                return;
            }

            /*
             * This is the code for making the player unable to escape the world border by wrapping their coordinates around.
             * I am disabling it for now as it should be a configurable thing.
             * TODO: Make this configurable
            if (move_player.x < 0.f) {
                move_player.x += 256.f;
                valid = false;
            } else if (move_player.x > 256.f) {
                move_player.x -= 256.f;
                valid = false;
            }
            if (move_player.z < 0.f) {
                move_player.z += 256.f;
                valid = false;
            } else if (move_player.z > 256.f) {
                move_player.z -= 256.f;
                valid = false;
            }
            */
            if (this->debug) post_to_chat("!" + std::to_string(move_player.x) + ":" + std::to_string(move_player.y) + ":" + std::to_string(move_player.z) + " " + std::to_string(valid));
            if (!valid) {
                int32_t temporary_eid = move_player.entity_id;
                move_player.entity_id = 0;
                send_downstream_packet(move_player);
                move_player.entity_id = temporary_eid;
            }
        }
        /*else if (packet_id == PlayerEquipmentPacket::packet_id) {
        PlayerEquipmentPacket equip_item;
        equip_item.deserialize_body(&stream);
        std::cout << "EI{B{" << equip_item.block << "};M{" << equip_item.meta << "}}\n";
        post_to_chat("Equipped item:");
        post_to_chat("- Block:" + std::to_string(equip_item.block));
        post_to_chat("- Meta:" + std::to_string(equip_item.meta));
    }*/
        // Send to server
        stream.ResetReadPointer();
        this->targetPeer->Send(&stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, this->targetGuid, false);
    }
}

void Client::handle_downstream_packets() {
    // Server > relay
    packet = this->targetPeer->Receive();
    if (!packet) return;
    if (packet->bitSize != 0) {
        RakNet::BitStream input_stream(packet->data, BITS_TO_BYTES((int)packet->bitSize), false);
        RakNet::BitStream stream;
        this->mapper->map_downstream_bitstream(input_stream, stream);

        uint8_t packet_id;
        stream.Read<uint8_t>(packet_id);
        // if (this->debug) post_to_chat("!" + std::to_string((int)packet_id));
        if (add_entity_packets.count(packet_id) > 0) {
            int32_t entity_id;
            stream.Read<int32_t>(entity_id);
            this->known_entity_ids.insert(entity_id);
            // post_to_chat("+" + std::to_string(entity_id));
        } else if (packet_id == RemoveEntityPacket::packet_id) {
            RemoveEntityPacket remove_entity;
            remove_entity.deserialize_body(&stream);
            // std::cout << "Remove entity: " << remove_entity.entity_id << "\n";
            this->known_entity_ids.erase(remove_entity.entity_id);
            // post_to_chat("-" + std::to_string(remove_entity.entity_id));
        } else if (packet_id == HurtArmorPacket::packet_id && this->god_mode) {
            post_to_chat("..HurtArmor..");
            this->targetPeer->DeallocatePacket(packet);
            return;
        } else if (packet_id == SetHealthPacket::packet_id && this->god_mode) {
            post_to_chat("..SetHealth..");
            this->targetPeer->DeallocatePacket(packet);
            return;
        }

        // Send to client
        stream.ResetReadPointer();
        this->relay->peer->Send(&stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, this->guid, false);
    }
    this->targetPeer->DeallocatePacket(packet);
}

template <typename T>
void Client::send_downstream_packet(T &packet) {
    // Relay > client
    RakNet::BitStream send_stream;
    send_stream.Write<uint8_t>(packet.packet_id);
    packet.serialize_body(&send_stream);
    this->relay->peer->Send(&send_stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, this->guid, false);
}

template <typename T>
void Client::send_upstream_packet(T &packet) {
    // Relay > server
    RakNet::BitStream send_stream;
    send_stream.Write<uint8_t>(packet.packet_id);
    packet.serialize_body(&send_stream);
    this->targetPeer->Send(&send_stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, this->targetGuid, false);
}

void Client::post_to_chat(std::string message) {
    MessagePacket msg;
    msg.message = message.c_str();
    send_downstream_packet(msg);
}

void Client::reload_chunks() {
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            RequestChunkPacket req;
            req.x = x;
            req.z = z;
            send_upstream_packet(req);
        }
    }
}

void Client::switch_connection(RakNet::RakPeerInterface *newTargetPeer, RakNet::RakNetGUID newTargetGuid) {
    this->targetPeer->CloseConnection(this->targetGuid, true, 0, IMMEDIATE_PRIORITY);
    this->targetPeer->Shutdown(100, 0, IMMEDIATE_PRIORITY);
    RakNet::RakPeerInterface::DestroyInstance(this->targetPeer);
    this->targetPeer = newTargetPeer;
    this->targetGuid = newTargetGuid;
    remove_entities();
    post_to_chat("Entities removed");
    // delete this->mapper;
    // this->mapper = new MCPIRelay::Mapper;
    // post_to_chat("Mapper reset");
    try {
        login(newTargetPeer, newTargetGuid);
    } catch (const std::exception &e) {
        post_to_chat("Failed to connect: " + *e.what());
        post_to_chat("Unrecoverable state, terminating connection.");
        this->relay->clients.erase(this->guid);
        delete this;
        return;
    }
    post_to_chat("Servers changed");
    reload_chunks();
    post_to_chat("Chunks reloaded");
}

void Client::login(RakNet::RakPeerInterface *targetPeer, RakNet::RakNetGUID targetGuid) {
    LoginPacket login;
    login.username = RakNet::RakString(this->username.c_str());
    login.protocol_1 = 9;
    login.protocol_2 = 9;
    send_upstream_packet(login);
    // Await StartGamePacket and set up player id mapping

    bool awaiting_answer = true;
    RakNet::Packet *packet;
    while (awaiting_answer) {
        packet = targetPeer->Receive();
        if (!packet) continue;
        uint8_t packet_id;
        RakNet::BitStream receive_stream(packet->data, (int)packet->bitSize, false);
        receive_stream.Read<uint8_t>(packet_id);
        if (packet_id == StartGamePacket::packet_id) {
            awaiting_answer = false;
            StartGamePacket start_game;
            start_game.deserialize_body(&receive_stream);
            this->mapper->set_client_entity_id(start_game.entity_id);
            // this->mapper->map_downstream_bitstream(receive_stream, stream);
        } else if (packet_id == LoginStatusPacket::packet_id) {
            LoginStatusPacket packet;
            packet.deserialize_body(&receive_stream);
            int32_t status_id = packet.status;
            if (status_id != 0) {
                std::cout << status_id << std::endl;
                throw std::runtime_error("Nonzero status received.");
            }
        }
        RakSleep(1);
    }
    ReadyPacket ready;
    ready.status = true;
    send_upstream_packet(ready);
}

void Client::remove_entities() {
    for (const auto &entity_id : known_entity_ids) {
        std::cout << "Removing entity " << entity_id << std::endl;
        RemoveEntityPacket remove_entity;
        remove_entity.entity_id = entity_id;
        send_downstream_packet(remove_entity);
        this->mapper->remove_downstream_mapping(entity_id);
        // this->mapper->free_entity_id(entity_id);
    }
    known_entity_ids.clear();

    for (const auto &player_entity_id : known_player_entity_ids) {
        // std::cout << "Removing player " << player_entity_id << "\n";
        RemoveEntityPacket remove_entity;
        remove_entity.entity_id = player_entity_id;
        send_downstream_packet(remove_entity);
        this->mapper->remove_downstream_mapping(player_entity_id);
        // this->mapper->free_entity_id(player_entity_id);
    }
    known_player_entity_ids.clear();
}

void Client::setup_perms_by_ip(std::string ip) {
    std::ifstream f("authorized.json");
    try {
        json authorized = json::parse(f);
        if (authorized.contains(ip)) {
            this->authorized = true;
            this->rank = authorized[ip]["level"];
            this->token_name = authorized[ip]["name"];
            authorized.erase(ip);
        } else {
            this->authorized = false;
            this->rank = 1;
        }
        std::ofstream o("authorized.json");
        o << std::setw(4) << authorized << std::endl;
    } catch (const std::exception &e) {
        std::cout << "Failed to get auth details, check that an authorized.json file is present and valid." << std::endl;
        std::cout << "Reason: " << e.what() << std::endl;
        std::cout << "Rank set to 1 (DEFAULT)" << std::endl;
        this->authorized = false;
        this->rank = 1;
    }
}

std::ostream &Client::log() {
    std::cout << "#" << this->token_name << " ";
    return std::cout;
}
