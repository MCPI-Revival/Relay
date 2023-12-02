#include <network/packets/login_status_packet.hpp>

const uint8_t RoadRunner::network::packets::LoginStatusPacket::packet_id = 131;

bool RoadRunner::network::packets::LoginStatusPacket::deserialize_body(RakNet::BitStream *stream) {
	return stream->Read<int32_t>(this->status);
}

void RoadRunner::network::packets::LoginStatusPacket::serialize_body(RakNet::BitStream *stream) {
    stream->Write<int32_t>(this->status);
}
