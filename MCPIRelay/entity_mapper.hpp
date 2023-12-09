#pragma once

#include <BitStream.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace MCPIRelay {

    class Mapper {
    public:
        std::vector<int32_t> free_entity_ids{};
        int32_t size = 0;
        bool has_game_started = false;

        std::unordered_map<int32_t, int32_t> downstream_entity_id_map{};
        std::unordered_map<int32_t, int32_t> upstream_entity_id_map{};

        int32_t allocate_entity_id();
        void free_entity_id(int32_t entity_id);

        bool is_downstream = false;
        void map_bitstream(RakNet::BitStream &stream, RakNet::BitStream &out_stream);
        int32_t map_entity_id(int32_t entity_id);

        int32_t map_upstream(int32_t entity_id);
        int32_t map_downstream(int32_t entity_id);

        void map_upstream_bitstream(RakNet::BitStream &stream, RakNet::BitStream &out_stream);
        void map_downstream_bitstream(RakNet::BitStream &stream, RakNet::BitStream &out_stream);
        void set_client_entity_id(int32_t entity_id);
        void free_mapping(int32_t entity_id);
        void remove_upstream_mapping(int32_t entity_id);
        void remove_downstream_mapping(int32_t entity_id);
        void dump_map();

        Mapper();
        ~Mapper();
    };
}
