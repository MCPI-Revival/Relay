#pragma once

#include <BitStream.h>
#include <iostream>
#include <cstdint>

namespace RoadRunner {
    namespace network {
        namespace packets {
            class ContainerAckPacket {
            public:
                static const uint8_t packet_id;

                uint8_t window_id;
                int16_t unknown_1;
                bool unknown_2;

                bool deserialize_body(RakNet::BitStream *stream);

                void serialize_body(RakNet::BitStream *stream);
            };
        }
    }
}
