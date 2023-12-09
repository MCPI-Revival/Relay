#pragma once

#include <algorithm>
#include <string>

namespace MCPIRelay {
    class Area {
    public:
        int32_t x1, y1, z1, x2, y2, z2;

        bool is_inside(int32_t x, int32_t y, int32_t z) {
            return (
                (x >= x1) && (x <= x2) &&
                (y >= y1) && (y <= y2) &&
                (z >= z1) && (z <= z2));
        }

        Area(int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2) {
            this->x1 = std::min(x1, x2);
            this->y1 = std::min(y1, y2);
            this->z1 = std::min(z1, z2);
            this->x2 = std::max(x1, x2);
            this->y2 = std::max(y1, y2);
            this->z2 = std::max(z1, z2);
        }

        ~Area(){};
    };
}
