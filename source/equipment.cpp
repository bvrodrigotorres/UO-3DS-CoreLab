// SPDX-License-Identifier: MIT
#include "equipment.hpp"
#include <fstream>
#include <cstring>
namespace lab {
namespace {
uint32_t get(const std::vector<uint8_t>&b,size_t p){return uint32_t(b[p])|uint32_t(b[p+1])<<8|uint32_t(b[p+2])<<16|uint32_t(b[p+3])<<24;}
}
bool EquipmentArt::load(const std::string&p){
    std::ifstream f(p,std::ios::binary);std::vector<uint8_t>b(48+8*76*104*4);dolls.clear();currentDoll.clear();currentKey=~0u;playerName.clear();checksums.fill(0);
    if(!f.read(reinterpret_cast<char*>(b.data()),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UEQ1",4)||b[4]!=76||b[5]||b[6]!=104||b[7]||get(b,8)!=8*76*104*4)return false;
    uint32_t hash=2166136261u;for(size_t i=48;i<b.size();++i)hash=(hash^b[i])*16777619u;
    if(hash!=get(b,12))return false;
    for(size_t n=0;n<8;++n)checksums[n]=get(b,16+n*4);
    dolls.assign(b.begin()+48,b.end());return true;
}
bool equipmentChecksum(const std::string&p,uint32_t expected){
    std::ifstream f(p,std::ios::binary);if(!f)return false;
    uint32_t h=2166136261u;char b[4096];while(f){f.read(b,sizeof b);for(std::streamsize i=0;i<f.gcount();++i)h=(h^uint8_t(b[i]))*16777619u;}
    return f.eof()&&h==expected;
}
}
