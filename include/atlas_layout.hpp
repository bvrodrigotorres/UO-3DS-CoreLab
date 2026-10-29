// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <vector>
namespace lab {
// Legacy regional packs use 32 slots. Compact world packs declare fewer sprites
// per page without wasting 16-bit image handles on transparent padding.
inline bool readAtlasLayout(const std::vector<uint8_t>&b,unsigned&slots){
    if(b.size()!=8||b[0]!='U'||b[1]!='W'||b[2]!='P'||b[3]!='1'||b[5]||b[6]||b[7])return false;
    unsigned n=b[4];if(n!=1&&n!=2&&n!=4&&n!=8&&n!=32)return false;slots=n;return true;
}
}
