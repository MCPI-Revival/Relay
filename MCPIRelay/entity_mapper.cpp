#include <entity_mapper.hpp>

#include <network/packets/start_game_packet.hpp>
#include <network/packets/add_mob_packet.hpp>
#include <network/packets/add_player_packet.hpp>
#include <network/packets/remove_player_packet.hpp>
#include <network/packets/add_entity_packet.hpp>
#include <network/packets/remove_entity_packet.hpp>
#include <network/packets/add_item_entity_packet.hpp>
#include <network/packets/take_item_entity_packet.hpp>
#include <network/packets/move_entity_packet.hpp>
#include <network/packets/move_entity_pos_rot_packet.hpp>
#include <network/packets/move_player_packet.hpp>
#include <network/packets/place_block_packet.hpp>
#include <network/packets/remove_block_packet.hpp>
#include <network/packets/add_painting_packet.hpp>
#include <network/packets/entity_event_packet.hpp>
#include <network/packets/player_equipment_packet.hpp>
#include <network/packets/player_armor_equipment_packet.hpp>
#include <network/packets/interact_packet.hpp>
#include <network/packets/use_item_packet.hpp>
#include <network/packets/player_action_packet.hpp>
#include <network/packets/set_entity_data_packet.hpp>
#include <network/packets/set_entity_motion_packet.hpp>
#include <network/packets/animate_packet.hpp>
#include <network/packets/respawn_packet.hpp>
#include <network/packets/send_inventory_packet.hpp>
#include <network/packets/drop_item_packet.hpp>

#include <network/packets/login_packet.hpp>
#include <network/packets/login_status_packet.hpp>
#include <network/packets/ready_packet.hpp>
#include <network/packets/message_packet.hpp>

using RoadRunner::network::packets::StartGamePacket;
using RoadRunner::network::packets::AddMobPacket;
using RoadRunner::network::packets::AddPlayerPacket;
using RoadRunner::network::packets::RemovePlayerPacket;
using RoadRunner::network::packets::AddEntityPacket;
using RoadRunner::network::packets::RemoveEntityPacket;
using RoadRunner::network::packets::AddItemEntityPacket;
using RoadRunner::network::packets::TakeItemEntityPacket;
using RoadRunner::network::packets::MoveEntityPacket;
using RoadRunner::network::packets::MoveEntityPacket_PosRot;
using RoadRunner::network::packets::MovePlayerPacket;
using RoadRunner::network::packets::PlaceBlockPacket;
using RoadRunner::network::packets::RemoveBlockPacket;
using RoadRunner::network::packets::AddPaintingPacket;
using RoadRunner::network::packets::EntityEventPacket;
using RoadRunner::network::packets::PlayerEquipmentPacket;
using RoadRunner::network::packets::PlayerArmorEquipmentPacket;
using RoadRunner::network::packets::InteractPacket;
using RoadRunner::network::packets::UseItemPacket;
using RoadRunner::network::packets::PlayerActionPacket;
using RoadRunner::network::packets::SetEntityDataPacket;
using RoadRunner::network::packets::SetEntityMotionPacket;
using RoadRunner::network::packets::AnimatePacket;
using RoadRunner::network::packets::RespawnPacket;
using RoadRunner::network::packets::SendInventoryPacket;
using RoadRunner::network::packets::DropItemPacket;

using RoadRunner::network::packets::LoginPacket;
using RoadRunner::network::packets::LoginStatusPacket;
using RoadRunner::network::packets::ReadyPacket;
using RoadRunner::network::packets::MessagePacket;


using MCPIRelay::Mapper;

Mapper::Mapper() {
	//std::cout << "[MAPPER]: Created\n";
	this->is_downstream = false;

}

Mapper::~Mapper() {
	//std::cout << "[MAPPER]: Destroyed\n";
}

int32_t Mapper::allocate_entity_id() {
	if (this->free_entity_ids.size() == 0) {
		this->size++;
		return this->size - 1;
	}
	int32_t last = this->free_entity_ids.back();
	this->free_entity_ids.pop_back();
	return last;
}


void Mapper::free_entity_id(int32_t entity_id) {
	if (entity_id == this->size - 1) {
		this->size--;
	} else {
		this->free_entity_ids.push_back(entity_id);
	}
}

int32_t Mapper::map_entity_id(int32_t entity_id) {
	if(is_downstream) {
		// Server > Client
		//if(this->downstream_entity_id_map.count(entity_id) > 0) std::cout << "Mapped " << entity_id << " to " << downstream_entity_id_map "\n";
		if(this->downstream_entity_id_map.count(entity_id) > 0) return this->downstream_entity_id_map[entity_id];
		int32_t mapped = allocate_entity_id();
		this->downstream_entity_id_map[entity_id] = mapped;
		this->upstream_entity_id_map[mapped] = entity_id;
		//std::cout << "[MAPPER_" << upstream_entity_id_map[0] << "]: Alloc " << entity_id << ">" << mapped << "\n";
		return mapped;
	} else {
		// Client > Server
		if(this->upstream_entity_id_map.count(entity_id) > 0) return this->upstream_entity_id_map[entity_id];
		std::cout << "[MAPPER]: It appears that a client has somehow created an entity, which should not be possible.\nThe program will now gracefully break itself, prepare for the worst.\n";
		return entity_id;
	}
}

int32_t Mapper::map_upstream(int32_t entity_id) {
	is_downstream = false;
	return map_entity_id(entity_id);
}

int32_t Mapper::map_downstream(int32_t entity_id) {
	is_downstream = true;
	return map_entity_id(entity_id);
}

void Mapper::map_upstream_bitstream(RakNet::BitStream& stream, RakNet::BitStream& out_stream) {
	is_downstream = false;
	map_bitstream(stream, out_stream);
}

void Mapper::map_downstream_bitstream(RakNet::BitStream& stream, RakNet::BitStream& out_stream) {
	is_downstream = true;
	map_bitstream(stream, out_stream);
}

void Mapper::free_mapping(int32_t entity_id) {
	// entity_id is the one the server uses, not the client!
	int32_t mapped_id = this->downstream_entity_id_map[entity_id];
	free_entity_id(mapped_id);
}

void Mapper::map_bitstream(RakNet::BitStream& stream, RakNet::BitStream& out_stream) {
	uint8_t packet_id;
	stream.Read<uint8_t>(packet_id);
	out_stream.Write(packet_id);

	if(packet_id == StartGamePacket::packet_id) {
		StartGamePacket packet;
		this->has_game_started = true;
		packet.deserialize_body(&stream);
		packet.entity_id = map_entity_id(packet.entity_id);
		packet.serialize_body(&out_stream);
	} else if(packet_id == LoginPacket::packet_id ||
			  packet_id == LoginStatusPacket::packet_id ||
			  packet_id == ReadyPacket::packet_id) {
		out_stream.Write(stream);
	} else if(has_game_started) {
		if(packet_id == AddMobPacket::packet_id) {
			AddMobPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == AddPlayerPacket::packet_id) {
			AddPlayerPacket packet;
			packet.deserialize_body(&stream);
			int32_t entity_id = packet.entity_id;
			packet.entity_id = map_entity_id(packet.entity_id);
			if(this->downstream_entity_id_map[entity_id] != 0) {
				packet.serialize_body(&out_stream);
			} else {
				std::cout << "[MAPPER_" << upstream_entity_id_map[0] << "]: Got AddPlayerPacket with same entity id! (initial=" << packet.entity_id << ")\n";
				dump_map();
				out_stream.SetWriteOffset(0);
				out_stream.Write<uint8_t>(MessagePacket::packet_id);
				MessagePacket msg;
				msg.message = "BAD PLAYER EID!";
				msg.serialize_body(&out_stream);
				return;

			}
		} else if(packet_id == RemovePlayerPacket::packet_id) {
			RemovePlayerPacket packet;
			packet.deserialize_body(&stream);
			int32_t entity_id = packet.entity_id;
			packet.entity_id = map_entity_id(packet.entity_id);
			if(this->downstream_entity_id_map[entity_id] != 0) {
				packet.serialize_body(&out_stream);
			} else {
				std::cout << "[MAPPER_" << upstream_entity_id_map[0] << "]: Got RemovePlayerPacket with same entity id!\n";
				dump_map();
				out_stream.SetWriteOffset(0);
				out_stream.Write<uint8_t>(MessagePacket::packet_id);
				MessagePacket msg;
				msg.message = "BAD PLAYER EID, BUT MORE CONCERNING!";
				msg.serialize_body(&out_stream);
				return;
			}
		} else if(packet_id == AddEntityPacket::packet_id) {
			AddEntityPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == RemoveEntityPacket::packet_id) {
			RemoveEntityPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == AddItemEntityPacket::packet_id) {
			AddItemEntityPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == TakeItemEntityPacket::packet_id) {
			TakeItemEntityPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.target = map_entity_id(packet.target);
			packet.serialize_body(&out_stream);
		} else if(packet_id == MoveEntityPacket::packet_id) {
			MoveEntityPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == MoveEntityPacket_PosRot::packet_id) {
			MoveEntityPacket_PosRot packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == MovePlayerPacket::packet_id) {
			MovePlayerPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == PlaceBlockPacket::packet_id) {
			PlaceBlockPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == RemoveBlockPacket::packet_id) {
			RemoveBlockPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == AddPaintingPacket::packet_id) {
			AddPaintingPacket packet;
			packet.deserialize_body(&stream);
			//packet.title = "Wind";
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == EntityEventPacket::packet_id) {
			EntityEventPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == PlayerEquipmentPacket::packet_id) {
			PlayerEquipmentPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == PlayerArmorEquipmentPacket::packet_id) {
			PlayerArmorEquipmentPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == InteractPacket::packet_id) {
			InteractPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.target = map_entity_id(packet.target);
			packet.serialize_body(&out_stream);
		} else if(packet_id == UseItemPacket::packet_id) {
			UseItemPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == PlayerActionPacket::packet_id) {
			PlayerActionPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == SetEntityDataPacket::packet_id) {
			SetEntityDataPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == SetEntityMotionPacket::packet_id) {
			SetEntityMotionPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == AnimatePacket::packet_id) {
			AnimatePacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == RespawnPacket::packet_id) {
			RespawnPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == SendInventoryPacket::packet_id) {
			SendInventoryPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else if(packet_id == DropItemPacket::packet_id) {
			DropItemPacket packet;
			packet.deserialize_body(&stream);
			packet.entity_id = map_entity_id(packet.entity_id);
			packet.serialize_body(&out_stream);
		} else {
			out_stream.Write(&stream);
		}
	}
}

void Mapper::set_client_entity_id(int32_t entity_id) {
	std::cout << "[MAPPER_" << upstream_entity_id_map[0] << "]: CEID:" << entity_id << "\n";
	downstream_entity_id_map.erase(upstream_entity_id_map[0]);
	this->downstream_entity_id_map[entity_id] = 0;
	this->upstream_entity_id_map[0] = entity_id;
}

void Mapper::remove_upstream_mapping(int32_t entity_id) {
	// This is a permament operation! After removing a mapping the entity id will be unavailable
	this->upstream_entity_id_map.erase(downstream_entity_id_map[entity_id]);
	this->downstream_entity_id_map.erase(entity_id);

}

void Mapper::remove_downstream_mapping(int32_t entity_id) {
	this->downstream_entity_id_map.erase(upstream_entity_id_map[entity_id]);
	this->upstream_entity_id_map.erase(entity_id);
}

void Mapper::dump_map() {
	std::cout << "[MAPPER_" << upstream_entity_id_map[0] << "]: Mapper dump (downstream):\n";
	for (const auto& pair : downstream_entity_id_map) {
		std::cout << pair.first << ": " << pair.second << "\n";
	}
	std::cout << "-----------------\n";
}
