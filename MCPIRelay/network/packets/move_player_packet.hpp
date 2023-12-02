#pragma once

#include <BitStream.h>
#include <iostream>
#include <cstdint>

namespace RoadRunner {
    namespace network {
        namespace packets {
            class MovePlayerPacket {
            public:
                static const uint8_t packet_id;

                int32_t entity_id;
                float x;
                float y;
                float z;
                float pitch;
                float yaw;

                bool deserialize_body(RakNet::BitStream *stream);

                void serialize_body(RakNet::BitStream *stream);
            };
        }
    }
}
