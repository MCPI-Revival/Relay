#include <network/packets/player_equipment_packet.hpp>

const uint8_t RoadRunner::network::packets::PlayerEquipmentPacket::packet_id = 159;

bool RoadRunner::network::packets::PlayerEquipmentPacket::deserialize_body(RakNet::BitStream *stream) {
    if (!stream->Read<int32_t>(this->entity_id)) {
        return false;
    }
    if (!stream->Read<uint16_t>(this->block)) {
        return false;
    }
    if (!stream->Read<uint16_t>(this->meta)) {
        return false;
    }
    return true;
}

void RoadRunner::network::packets::PlayerEquipmentPacket::serialize_body(RakNet::BitStream *stream) {
    stream->Write<int32_t>(this->entity_id);
    stream->Write<uint16_t>(this->block);
    stream->Write<uint16_t>(this->meta);
}
