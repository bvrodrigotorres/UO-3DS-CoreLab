// SPDX-License-Identifier: MIT
#include "world_expedition.hpp"
#include <fstream>
#include <cstring>
namespace lab {
namespace {
uint32_t u32(const uint8_t*p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
uint16_t u16(const uint8_t*p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
}
bool WorldGraphics::load(const std::string&root,std::string&error){
    std::ifstream f(root+"graphics.bin",std::ios::binary|std::ios::ate);
    auto bad=[&](){error="CATALOGO MUNDIAL INVALIDO";return false;};
    if(!f||f.tellg()<16||f.tellg()>1024*1024)return bad();
    std::vector<uint8_t>b(size_t(f.tellg()));f.seekg(0);if(!f.read((char*)b.data(),b.size()))return bad();
    unsigned images=u32(b.data()+4),records=u32(b.data()+8);uint32_t hash=2166136261u;for(size_t i=16;i<b.size();++i)hash=(hash^b[i])*16777619u;
    if(std::memcmp(b.data(),"UWG1",4)||!images||images>=NoImage||b.size()!=16+uint64_t(records)*6||hash!=u32(b.data()+12))return bad();
    WorldGraphics next;next.count=images;next.land.assign(16384,NoImage);next.statics.assign(65536,NoImage);next.texture.assign(65536,NoImage);
    for(unsigned i=0;i<records;++i){auto*p=b.data()+16+i*6;unsigned kind=u16(p),art=u16(p+2),id=u16(p+4);if(kind>2||id>=images)return bad();auto&v=kind==0?next.land:kind==1?next.statics:next.texture;if(art>=v.size()||v[art]!=NoImage)return bad();v[art]=id;}
    std::ifstream rf(root+"radar.bin",std::ios::binary|std::ios::ate);
    if(!rf||rf.tellg()<32768||rf.tellg()>512*1024||rf.tellg()%2)return bad();
    std::vector<uint8_t>r(size_t(rf.tellg()));rf.seekg(0);if(!rf.read((char*)r.data(),r.size()))return bad();
    next.radar.reserve(r.size()/2);for(size_t i=0;i<r.size();i+=2)next.radar.push_back(u16(r.data()+i));
    *this=std::move(next);error.clear();return true;
}
bool WorldGraphics::bind(World&w,std::string&error)const{
    for(const auto&l:w.land)if(l.art!=2&&(l.art>=land.size()||land[l.art]==NoImage)){error="FALTA TERRENO MUNDIAL";return false;}
    for(const auto&o:w.objects)if(o.art>=statics.size()||statics[o.art]==NoImage){error="FALTA OBJETO MUNDIAL";return false;}
    for(auto&l:w.land){l.image=l.art==2?NoImage:land[l.art];l.texture=false;}
    // Sloped texture IDs are not present in Land. Keep the original diamond
    // fallback here until the archive exposes its texid to this resolver.
    for(auto&o:w.objects)o.image=statics[o.art];
    w.images=count;error.clear();return true;
}
Journal WorldGraphics::map(const World&w)const{
    Journal j;j.width=w.width;j.height=w.height;j.originX=w.originX;j.originY=w.originY;j.map.resize(size_t(w.width*w.height)*3);
    std::vector<int>top(w.land.size());
    auto color=[&](size_t at,unsigned art){uint16_t c=art<radar.size()?radar[art]:0;for(unsigned n=0;n<3;++n)j.map[at*3+n]=uint8_t(((c>>(10-n*5))&31)*255/31);};
    for(size_t i=0;i<w.land.size();++i){color(i,w.land[i].art);top[i]=w.vertices[(i/w.width)*(w.width+1)+i%w.width];}
    for(const auto&o:w.objects){size_t i=size_t(o.y*w.width+o.x);if(o.art>1&&o.z+o.height>=top[i]){color(i,0x4000u+o.art);top[i]=o.z+o.height;}}
    return j;
}
}
