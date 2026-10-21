// SPDX-License-Identifier: MIT
#include "core.hpp"
#include <fstream>
#include <iterator>
#include <cstring>
namespace lab {
bool loadWorld(const std::string&path,World&out,std::string&error){
    std::ifstream f(path,std::ios::binary);
    if(!f){error="No scene.bin; using built-in demo";return false;}
    f.seekg(0,std::ios::end);auto length=f.tellg();f.seekg(0);
    if(length<32||length>8*1024*1024){error="Invalid scene size";return false;}
    std::vector<unsigned char>b((size_t)length);f.read((char*)b.data(),b.size());
    if(!f||std::memcmp(b.data(),"UCL1",4)){error="Invalid scene header";return false;}
    size_t p=4;bool ok=true;
    auto read=[&](int n)->uint64_t {if(p+n>b.size()){ok=false;return 0;}uint64_t v=0;for(int i=0;i<n;++i)v|=uint64_t(b[p++])<<(i*8);return v;};
    if(read(2)!=1){error="Unsupported scene version";return false;}
    World w;w.width=read(2);w.height=read(2);w.images=read(2);unsigned count=read(4);
    w.originX=(int32_t)read(4);w.originY=(int32_t)read(4);
    w.startX=read(2);w.startY=read(2);w.startZ=(int16_t)read(2);unsigned reserved=read(2);
    if(!ok||w.width<1||w.width>256||w.height<1||w.height>256||w.images>8192||count>131072||reserved||!w.contains(w.startX,w.startY)){
        error="Scene limits or spawn invalid";return false;
    }
    size_t expected=32+(w.width+1)*(w.height+1)+w.width*w.height*13+count*26;
    if(expected!=b.size()){error="Scene length mismatch";return false;}
    for(int i=0;i<(w.width+1)*(w.height+1);++i)w.vertices.push_back((int8_t)read(1));
    auto imageOK=[&](unsigned n){return n==NoImage||n<(unsigned)w.images;};
    for(int i=0;i<w.width*w.height;++i){
        Land l;l.art=read(2);l.image=read(2);l.flags=read(8);unsigned mode=read(1);l.texture=mode==1;
        if(!imageOK(l.image)||mode>1){error="Invalid land image";return false;}w.land.push_back(l);
    }
    for(unsigned i=0;i<count;++i){
        Object o;o.x=read(2);o.y=read(2);o.z=(int8_t)read(1);o.height=read(1);o.art=read(2);o.hue=read(2);o.image=read(2);o.flags=read(8);o.order=read(4);unsigned pad=read(2);
        if(!w.contains(o.x,o.y)||!imageOK(o.image)||pad){error="Invalid object";return false;}w.objects.push_back(o);
    }
    if(!ok){error="Truncated scene";return false;}
    std::ifstream migration(path+".saves",std::ios::binary);
    if(migration){
        unsigned char a[16];migration.read((char*)a,16);
        auto u=[&](int at){return uint32_t(a[at])|uint32_t(a[at+1])<<8|uint32_t(a[at+2])<<16|uint32_t(a[at+3])<<24;};
        uint32_t hash=2166136261u;for(auto v:b)hash=(hash^v)*16777619u;
        if(!migration||migration.peek()!=EOF||std::memcmp(a,"UWC1",4)||u(4)!=hash||!u(8)||!u(12)){error="Invalid save migration";return false;}
        w.compatiblePosition=u(8);w.compatibleInventory=u(12);
    }
    w.index();out=std::move(w);error.clear();return true;
}
}
