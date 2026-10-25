// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <array>
namespace lab {
// Full-client geometry archive. No global World, GPU textures or save mutations.
class WorldArchive {
    struct Entry {uint64_t offset=0;uint32_t bytes=0,checksum=0,count=0,first=0;};
    struct Slot {int key=-1;uint64_t used=0;std::vector<uint8_t>data;};
    std::string path;
    int mapWidth=0,mapHeight=0,columns=0,rows=0;
    uint64_t fileBytes=0,tick=0;unsigned reads=0;
    std::vector<Entry>entries;
    std::vector<uint64_t>landFlags,staticFlags;
    std::vector<uint16_t>textures;
    std::vector<uint8_t>heights;
    std::array<Slot,4>cache;
    const std::vector<uint8_t>*sector(unsigned,std::string&);
public:
    static constexpr unsigned SectorSide=64,WindowSide=128,MaxObjects=65536;
    static constexpr size_t CacheBudget=512*1024;
    bool open(const std::string&,std::string&);
    bool window(int x,int y,unsigned side,World&,std::string&);
    bool verify(std::string&);
    int width()const{return mapWidth;}
    int height()const{return mapHeight;}
    size_t sectorCount()const{return entries.size();}
    size_t cacheBytes()const;
    unsigned diskReads()const{return reads;}
    uint32_t objectCount()const{return entries.empty()?0:entries.back().first+entries.back().count;}
};
}
