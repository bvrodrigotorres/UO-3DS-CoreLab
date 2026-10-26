// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <limits>
namespace lab {
struct TextureSlice {uint64_t offset=0;uint32_t size=0,checksum=0;};
// Separate small index: validate it before admitting any reads or allocations.
inline bool readTextureIndex(const std::string&path,uint64_t archiveSize,const std::vector<size_t>&costs,std::vector<TextureSlice>&out){
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f||f.tellg()!=std::streamoff(8+costs.size()*16))return false;
    std::vector<uint8_t>b(size_t(f.tellg()));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),b.size()))return false;
    auto n=[&](size_t p,unsigned count){uint64_t v=0;for(unsigned i=0;i<count;++i)v|=uint64_t(b[p+i])<<(i*8);return v;};
    if(b[0]!='U'||b[1]!='T'||b[2]!='P'||b[3]!='1'||n(4,4)!=costs.size())return false;
    std::vector<TextureSlice>next;next.reserve(costs.size());uint64_t end=0;
    for(size_t i=0;i<costs.size();++i){size_t p=8+i*16;TextureSlice s{n(p,8),uint32_t(n(p+8,4)),uint32_t(n(p+12,4))};
        if(!s.size||s.size!=costs[i]||s.offset<end||s.offset>archiveSize||s.size>archiveSize-s.offset||s.offset>uint64_t(std::numeric_limits<int32_t>::max()))return false;
        end=s.offset+s.size;next.push_back(s);
    }
    if(end!=archiveSize)return false;
    out=std::move(next);return true;
}
}
