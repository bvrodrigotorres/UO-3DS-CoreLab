// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace lab {
struct EquipmentArt {
    std::vector<uint8_t>dolls;
    std::string playerName;
    std::array<uint32_t,8>checksums{};
    bool load(const std::string&);
};
bool equipmentChecksum(const std::string&,uint32_t);
}
