// SPDX-License-Identifier: MIT
#include "world_archive.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>
namespace lab {
namespace {
uint32_t u32(const uint8_t*p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
uint64_t u64(const uint8_t*p){return u32(p)|uint64_t(u32(p+4))<<32;}
uint16_t u16(const uint8_t*p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
uint32_t fnv(const uint8_t*p,size_t count,uint32_t h=2166136261u){for(size_t i=0;i<count;++i)h=(h^p[i])*16777619u;return h;}
constexpr size_t TerrainBytes=65*65*3;
}
size_t WorldArchive::cacheBytes()const{size_t bytes=0;for(const auto&s:cache)bytes+=s.data.size();return bytes;}
bool WorldArchive::open(const std::string&name,std::string&error){
    std::ifstream f(name,std::ios::binary|std::ios::ate);auto bad=[&](){error="ARCHIVO DE BRITANNIA INVALIDO";return false;};
    if(!f||f.tellg()<64||f.tellg()>std::streamoff(512*1024*1024))return bad();
    WorldArchive next;next.fileBytes=uint64_t(f.tellg());next.path=name;f.seekg(0);uint8_t header[64];if(!f.read((char*)header,64))return bad();
    if(std::memcmp(header,"UOW1",4)||u16(header+4)!=1||u16(header+6)!=SectorSide)return bad();
    unsigned width=u32(header+8),height=u32(header+12),cols=u32(header+16),rows=u32(header+20),lands=u32(header+24),statics=u32(header+28),count=u32(header+32);
    uint64_t tables=u64(header+40),index=u64(header+48),data=u64(header+56);
    if(!width||!height||width>7168||height>4096||width%64||height%64||cols!=width/64||rows!=height/64||count!=cols*rows||!lands||lands>16384||!statics||statics>65536||tables!=64||index!=64+uint64_t(lands+statics)*12||data!=index+uint64_t(count)*24||data>next.fileBytes)return bad();
    next.catalogueIdentity=u32(header+36);next.mapWidth=width;next.mapHeight=height;next.columns=cols;next.rows=rows;
    uint32_t hash=2166136261u;uint8_t record[24];
    next.landFlags.reserve(lands);next.textures.reserve(lands);next.staticFlags.reserve(statics);next.heights.reserve(statics);next.entries.reserve(count);
    for(unsigned i=0;i<lands;++i){if(!f.read((char*)record,12)||u16(record+10))return bad();hash=fnv(record,12,hash);next.landFlags.push_back(u64(record));next.textures.push_back(u16(record+8));}
    for(unsigned i=0;i<statics;++i){if(!f.read((char*)record,12)||record[9]||u16(record+10))return bad();hash=fnv(record,12,hash);next.staticFlags.push_back(u64(record));next.heights.push_back(record[8]);}
    uint64_t expected=data;uint32_t first=0;
    for(unsigned i=0;i<count;++i){if(!f.read((char*)record,24))return bad();hash=fnv(record,24,hash);Entry e{u64(record),u32(record+8),u32(record+12),u32(record+16),u32(record+20)};
        if(e.offset!=expected||e.count>65536||e.bytes!=TerrainBytes+uint64_t(e.count)*7||e.bytes>CacheBudget||e.offset>next.fileBytes||e.bytes>next.fileBytes-e.offset||e.first!=first||uint64_t(first)+e.count>10000000)return bad();
        expected+=e.bytes;first+=e.count;next.entries.push_back(e);
    }
    if(expected!=next.fileBytes||hash!=u32(header+36))return bad();
    *this=std::move(next);error.clear();return true;
}
const std::vector<uint8_t>*WorldArchive::sector(unsigned key,std::string&error){
    if(key>=entries.size()){error="SECTOR FUERA DEL MAPA";return nullptr;}
    for(auto&s:cache)if(s.key==int(key)){s.used=++tick;return &s.data;}
    const auto&e=entries[key];
    // Evict before allocating: resident sector bytes never exceed 512 KiB.
    while(cacheBytes()+e.bytes>CacheBudget){auto victim=cache.end();for(auto i=cache.begin();i!=cache.end();++i)if(i->key>=0&&(victim==cache.end()||i->used<victim->used))victim=i;if(victim==cache.end())break;victim->key=-1;std::vector<uint8_t>().swap(victim->data);}
    auto slot=std::min_element(cache.begin(),cache.end(),[](const Slot&a,const Slot&b){return a.key<0?b.key>=0:b.key<0?false:a.used<b.used;});
    slot->key=-1;std::vector<uint8_t>().swap(slot->data);
    std::ifstream f(path,std::ios::binary);std::vector<uint8_t>b(e.bytes);f.seekg(std::streamoff(e.offset));++reads;
    if(!f.read((char*)b.data(),b.size())||fnv(b.data(),b.size())!=e.checksum){error="SECTOR INCOMPLETO O CORRUPTO";return nullptr;}
    slot->data=std::move(b);slot->key=int(key);slot->used=++tick;return &slot->data;
}
bool WorldArchive::window(int x,int y,unsigned side,World&out,std::string&error,unsigned limit){
    if(path.empty()||!side||side>WindowSide||x<0||y<0||x>mapWidth-int(side)||y>mapHeight-int(side)){error="VENTANA FUERA DE BRITANNIA";return false;}
    World next;next.width=next.height=int(side);next.originX=x;next.originY=y;next.startX=next.startY=int(side)/2;
    // Count only included records: a 64-square sliding view may touch four
    // sectors, but must not reserve the objects of all four entire sectors.
    size_t maximum=0;
    for(int sx=x/64;sx<=(x+int(side)-1)/64;++sx)for(int sy=y/64;sy<=(y+int(side)-1)/64;++sy){unsigned key=unsigned(sx*rows+sy);auto*b=sector(key,error);if(!b)return false;
        for(unsigned i=0;i<entries[key].count;++i){const auto*r=b->data()+TerrainBytes+size_t(i)*7;int xx=sx*64+r[2],yy=sy*64+r[3];if(xx>=x&&yy>=y&&xx<x+int(side)&&yy<y+int(side))++maximum;}}
    if(maximum>std::min(limit,MaxObjects)){error="SECTOR DEMASIADO DENSO PARA PRECARGA";return false;}
    next.vertices.assign((side+1)*(side+1),0);next.land.resize(side*side);next.objects.reserve(maximum);
    for(int sx=x/64;sx<=(x+int(side)-1)/64;++sx)for(int sy=y/64;sy<=(y+int(side)-1)/64;++sy){unsigned key=unsigned(sx*rows+sy);auto*b=sector(key,error);if(!b)return false;
        const auto&e=entries[key];int ox=sx*64,oy=sy*64;
        for(int yy=std::max(y,oy);yy<=std::min(y+int(side),oy+64);++yy)for(int xx=std::max(x,ox);xx<=std::min(x+int(side),ox+64);++xx){size_t at=size_t((yy-oy)*65+xx-ox)*3;unsigned art=u16(b->data()+at);if(art>=landFlags.size()){error="TERRENO INVALIDO";return false;}
            int tx=xx-x,ty=yy-y;next.vertices[ty*(side+1)+tx]=int8_t((*b)[at+2]);
            if(tx<int(side)&&ty<int(side)){auto&l=next.land[ty*side+tx];l.art=art;l.flags=landFlags[art];}
        }
        for(unsigned i=0;i<e.count;++i){const uint8_t*r=b->data()+TerrainBytes+size_t(i)*7;unsigned art=u16(r);int xx=ox+r[2],yy=oy+r[3];
            if(r[2]>=64||r[3]>=64||art>=staticFlags.size()){error="OBJETO DE SECTOR INVALIDO";return false;}
            if(xx<x||yy<y||xx>=x+int(side)||yy>=y+int(side))continue;
            if(next.objects.size()>=MaxObjects){error="DEMASIADOS OBJETOS EN LA VENTANA";return false;}
            Object o;o.art=art;o.x=xx-x;o.y=yy-y;o.z=int8_t(r[4]);o.hue=u16(r+5);o.flags=staticFlags[art];o.height=heights[art];o.order=e.first+i;next.objects.push_back(o);
        }
    }
    for(unsigned yy=0;yy<side;++yy)for(unsigned xx=0;xx<side;++xx){auto&l=next.land[yy*side+xx];l.texture=textures[l.art]>0&&lab::heights(next,xx,yy).slope;}
    next.startZ=lab::heights(next,next.startX,next.startY).average;
    next.index();
    out=std::move(next);error.clear();return true;
}
bool WorldArchive::radar(int x,int y,unsigned side,const std::vector<uint16_t>&palette,Journal&out,std::string&error){
    if(!side||side>128||x<0||y<0||x+int(side)>mapWidth||y+int(side)>mapHeight){error="RADAR FUERA DEL MAPA";return false;}
    Journal j;j.width=j.height=side;j.originX=x;j.originY=y;j.map.resize(side*side*3);std::vector<int16_t>top(side*side,-32768);
    auto color=[&](size_t at,unsigned art){uint16_t c=art<palette.size()?palette[art]:0;for(unsigned n=0;n<3;++n)j.map[at*3+n]=uint8_t(((c>>(10-n*5))&31)*255/31);};
    for(int sx=x/64;sx<=(x+int(side)-1)/64;++sx)for(int sy=y/64;sy<=(y+int(side)-1)/64;++sy){unsigned key=unsigned(sx*rows+sy);auto*b=sector(key,error);if(!b)return false;
        for(int yy=std::max(y,sy*64);yy<std::min(y+int(side),sy*64+64);++yy)for(int xx=std::max(x,sx*64);xx<std::min(x+int(side),sx*64+64);++xx){auto*r=b->data()+((yy-sy*64)*65+xx-sx*64)*3;size_t at=size_t(yy-y)*side+xx-x;top[at]=int8_t(r[2]);color(at,u16(r));}
        for(unsigned i=0;i<entries[key].count;++i){auto*r=b->data()+TerrainBytes+size_t(i)*7;unsigned art=u16(r);int xx=sx*64+r[2],yy=sy*64+r[3];if(art>=heights.size()||r[2]>=64||r[3]>=64){error="RADAR INVALIDO";return false;}if(xx<x||yy<y||xx>=x+int(side)||yy>=y+int(side))continue;size_t at=size_t(yy-y)*side+xx-x;int z=int8_t(r[4])+heights[art];if(art>1&&z>=top[at]){top[at]=z;color(at,0x4000u+art);}}
    }
    out=std::move(j);error.clear();return true;
}
bool WorldArchive::verify(std::string&error){for(unsigned i=0;i<entries.size();++i)if(!sector(i,error))return false;error.clear();return true;}
}
