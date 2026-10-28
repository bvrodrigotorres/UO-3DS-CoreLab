// SPDX-License-Identifier: MIT
#include "core.hpp"
#include <fstream>
#include <array>
#include <algorithm>
#include <cstring>
namespace lab {
namespace {
// Keep only one SD block alongside the decoded world, including during import
// when the previous region is still alive. A whole-file buffer doubled RAM.
class SceneReader {
    std::ifstream&file;
    std::array<unsigned char,4096>buffer{};
    size_t remaining,at=0,used=0;
public:
    bool ok=true;
    uint32_t hash=2166136261u;
    SceneReader(std::ifstream&f,size_t bytes):file(f),remaining(bytes){}
    uint64_t read(unsigned n){
        uint64_t result=0;
        for(unsigned i=0;i<n;++i){
            if(at==used){
                used=std::min(remaining,buffer.size());at=0;
                if(!used||!file.read(reinterpret_cast<char*>(buffer.data()),used)){ok=false;return 0;}
                remaining-=used;
            }
            unsigned char byte=buffer[at++];hash=(hash^byte)*16777619u;
            result|=uint64_t(byte)<<(8*i);
        }
        return result;
    }
};
}

bool loadWorld(const std::string&path,World&out,std::string&error,bool buildIndex){
    std::ifstream f(path,std::ios::binary);
    if(!f){error="No scene.bin; using built-in demo";return false;}
    f.seekg(0,std::ios::end);auto length=f.tellg();f.seekg(0);
    if(length<32||length>24*1024*1024){error="Invalid scene size";return false;}
    SceneReader reader(f,size_t(length));
    auto read=[&](unsigned n){return reader.read(n);};
    if(read(4)!=0x314c4355u){error="Invalid scene header";return false;}
    if(read(2)!=1){error="Unsupported scene version";return false;}
    World w;w.width=read(2);w.height=read(2);w.images=read(2);unsigned count=read(4);
    w.originX=(int32_t)read(4);w.originY=(int32_t)read(4);
    w.startX=read(2);w.startY=read(2);w.startZ=(int16_t)read(2);unsigned reserved=read(2);
    if(!reader.ok||w.width<1||w.width>768||w.height<1||w.height>768||w.images>32768||count>262144||reserved||!w.contains(w.startX,w.startY)){
        error="Scene limits or spawn invalid";return false;
    }
    size_t expected=32+(w.width+1)*(w.height+1)+w.width*w.height*13+count*26;
    if(expected!=size_t(length)){error="Scene length mismatch";return false;}
    w.vertices.reserve((w.width+1)*(w.height+1));w.land.reserve(w.width*w.height);w.objects.reserve(count);
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
    if(!reader.ok){error="Truncated scene";return false;}
    std::ifstream migration(path+".saves",std::ios::binary);
    if(migration){
        unsigned char a[16];migration.read((char*)a,16);
        auto u=[&](int at){return uint32_t(a[at])|uint32_t(a[at+1])<<8|uint32_t(a[at+2])<<16|uint32_t(a[at+3])<<24;};
        uint32_t hash=reader.hash;
        if(!migration||migration.peek()!=EOF||std::memcmp(a,"UWC1",4)||u(4)!=hash||!u(8)||!u(12)){error="Invalid save migration";return false;}
        w.compatiblePosition=u(8);w.compatibleInventory=u(12);
    }
    if(buildIndex)w.index();
    out=std::move(w);error.clear();return true;
}
}
