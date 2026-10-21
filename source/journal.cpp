// SPDX-License-Identifier: MIT
#include "journal.hpp"
#include <fstream>
#include <iterator>
#include <cstring>
namespace lab {
static uint32_t hash(const std::vector<uint8_t>&b,size_t offset=0){uint32_t h=2166136261u;for(size_t i=offset;i<b.size();++i)h=(h^b[i])*16777619u;return h;}
static bool read(const std::string&p,std::vector<uint8_t>&out,size_t limit){
    std::ifstream f(p,std::ios::binary);if(!f)return false;
    f.seekg(0,std::ios::end);auto n=f.tellg();if(n<0||n>(std::streamoff)limit)return false;f.seekg(0);
    out.resize((size_t)n);f.read((char*)out.data(),out.size());return bool(f);
}
bool loadJournal(const std::string&path,const std::string&scene,Journal&out,std::string&error){
    std::vector<uint8_t>b,world;
    if(!read(path,b,300000)||b.size()<28||std::memcmp(b.data(),"UOJ1",4)){error="No valid journal.bin; basic map";return false;}
    auto n=[&](int p,int size){uint32_t v=0;for(int i=0;i<size;++i)v|=uint32_t(b[p+i])<<(8*i);return v;};
    Journal j;j.width=n(4,2);j.height=n(6,2);j.originX=(int32_t)n(8,4);j.originY=(int32_t)n(12,4);
    j.dollWidth=n(24,2);j.dollHeight=n(26,2);
    if(j.originX<0||j.originY<0||j.originX>7168-j.width||j.originY>4096-j.height||j.width<1||j.width>256||j.height<1||j.height>256||j.dollWidth<1||j.dollWidth>76||j.dollHeight<1||j.dollHeight>104||
       b.size()!=28+size_t(j.width*j.height*3+j.dollWidth*j.dollHeight*4)||hash(b,28)!=n(20,4)){
        error="Invalid journal dimensions/checksum";return false;
    }
    if(!read(scene,world,512*1024)||hash(world)!=n(16,4)){error="Journal belongs to another scene";return false;}
    size_t end=28+j.width*j.height*3;j.map.assign(b.begin()+28,b.begin()+end);j.doll.assign(b.begin()+end,b.end());
    out=std::move(j);error.clear();return true;
}
}
