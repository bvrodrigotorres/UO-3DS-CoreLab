// SPDX-License-Identifier: MIT
#include "doors.hpp"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace lab {
static bool read(const std::string&path,std::vector<unsigned char>&b){
    std::ifstream f(path,std::ios::binary);if(!f)return false;
    f.seekg(0,std::ios::end);auto n=f.tellg();if(n<0||n>512*1024)return false;f.seekg(0);
    b.resize((size_t)n);f.read((char*)b.data(),b.size());return bool(f);
}
bool Doors::load(const std::string&path,const std::string&scene,const World&w,std::string&error){
    std::vector<unsigned char>b,raw;
    if(!read(path,b)||b.size()<16||std::memcmp(b.data(),"UOD1",4)){error="No valid doors.bin";return false;}
    auto n=[&](size_t p,int count){uint32_t v=0;for(int i=0;i<count;++i)v|=uint32_t(b[p+i])<<(8*i);return v;};
    unsigned count=n(6,2),base=n(8,4);
    if(n(4,2)!=1||count>256||base>w.objects.size()||b.size()!=16+12*count||!read(scene,raw)){error="Invalid door data";return false;}
    uint32_t hash=2166136261u;for(auto v:raw)hash=(hash^v)*16777619u;
    if(hash!=n(12,4)){error="Doors belong to another scene";return false;}
    std::vector<DoorState>next;std::vector<int>indices(w.objects.size(),-1);
    for(unsigned i=0;i<count;++i){
        size_t p=16+12*i,index=n(p,4);DoorState d;
        d.index=index;d.openArt=n(p+4,2);d.openImage=n(p+6,2);d.dx=(int8_t)b[p+8];d.dy=(int8_t)b[p+9];d.pair=n(p+10,2);
        if(index>=w.objects.size()||index<base||indices[index]!=-1||d.openImage>=w.images||std::abs(d.dx)>1||std::abs(d.dy)>1||(d.pair!=65535&&(d.pair>=count||d.pair==i))){error="Invalid door record";return false;}
        d.closed=w.objects[index];
        if(!(d.closed.flags&Door)||!w.contains(d.closed.x+d.dx,d.closed.y+d.dy)){error="Invalid door placement";return false;}
        indices[index]=i;next.push_back(d);
    }
    for(size_t i=0;i<next.size();++i)if(next[i].pair!=65535&&next[next[i].pair].pair!=i){error="Invalid paired doors";return false;}
    entries=std::move(next);lookup=std::move(indices);baseObjects=base;error.clear();return true;
}
bool Doors::tryOpen(const Actor&a,int dx,int dy){
    bool pending=false;
    for(auto&d:entries){
        if(d.open||std::abs(d.closed.z-a.z)>=16)continue;
        int x=d.closed.x-a.x,y=d.closed.y-a.y;
        bool target=x==dx&&y==dy;
        if(dx&&dy)target=target||(x==dx&&y==0)||(x==0&&y==dy);
        if(!target)continue;
        d.opening=true;pending=true;
        if(d.pair!=65535&&!entries[d.pair].open)entries[d.pair].opening=true;
    }
    return pending;
}
bool Doors::update(World&w,float dt){
    bool changed=false;
    for(auto&d:entries)if(d.opening&&!d.open){
        d.progress=std::min(1.f,d.progress+std::max(0.f,dt)/.18f);
        if(d.progress>=1){
            auto&o=w.objects[d.index];o.x=d.closed.x+d.dx;o.y=d.closed.y+d.dy;o.art=d.openArt;o.image=d.openImage;
            // The original impassable leaf moves to its open position, leaving
            // its old tile free. It is not deleted or made globally passable.
            d.open=true;d.opening=false;changed=true;
        }
    }
    if(changed)w.index();
    return changed;
}
bool Doors::visual(size_t index,DoorVisual&v)const{
    if(index>=lookup.size()||lookup[index]<0)return false;
    const auto&d=entries[lookup[index]];
    v={d.closed.x+d.dx*d.progress,d.closed.y+d.dy*d.progress,d.progress<.5f?d.closed.image:d.openImage};return true;
}
}
