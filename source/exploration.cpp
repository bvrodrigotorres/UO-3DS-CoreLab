// SPDX-License-Identifier: MIT
#include "exploration.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <array>
namespace lab {
static uint32_t hashBytes(const unsigned char*p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static uint32_t worldId(const World&w){
    uint32_t h=2166136261u;
    auto add=[&](uint64_t v){for(int i=0;i<8;++i){h=(h^(v&255))*16777619u;v>>=8;}};
    add(w.width);add(w.height);add(w.originX);add(w.originY);
    for(int z:w.vertices)add(z);
    for(const auto&l:w.land){add(l.art);add(l.flags);}
    for(const auto&o:w.objects){add(o.art);add(o.x);add(o.y);add(o.z);add(o.height);add(o.flags);}
    return h;
}
static void put(unsigned char*p,uint32_t n){for(int i=0;i<4;++i)p[i]=(n>>(i*8))&255;}
static uint32_t get(const unsigned char*p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
bool savePosition(const std::string&path,const World&w,const Actor&a,std::string&error){
    if(!w.contains(a.x,a.y)||a.z< -127||a.z>127){error="Invalid position";return false;}
    std::array<unsigned char,28>b{};b[0]='U';b[1]='L';b[2]='S';b[3]='1';
    put(b.data()+4,worldId(w));put(b.data()+8,a.x);put(b.data()+12,a.y);put(b.data()+16,a.z);put(b.data()+20,1);
    put(b.data()+24,hashBytes(b.data(),24));
    std::string tmp=path+".tmp";
    FILE*f=std::fopen(tmp.c_str(),"wb");if(!f){error="Cannot open save";return false;}
    bool ok=std::fwrite(b.data(),1,b.size(),f)==b.size();
    if(std::fclose(f)!=0)ok=false;
    if(ok)ok=std::rename(tmp.c_str(),path.c_str())==0;
    if(!ok){std::remove(tmp.c_str());error="Save failed; previous save kept";return false;}
    error="Position saved";return true;
}
bool loadPosition(const std::string&path,const World&w,Actor&a,std::string&error){
    std::ifstream f(path,std::ios::binary);std::array<unsigned char,28>b{};
    if(!f){error="No saved position";return false;}
    f.read((char*)b.data(),b.size());
    if(!f||f.peek()!=EOF||b[0]!='U'||b[1]!='L'||b[2]!='S'||b[3]!='1'||get(b.data()+20)!=1||get(b.data()+24)!=hashBytes(b.data(),24)){
        error="Invalid save";return false;
    }
    if(get(b.data()+4)!=worldId(w)){error="Save belongs to another scene";return false;}
    Actor next={(int32_t)get(b.data()+8),(int32_t)get(b.data()+12),(int32_t)get(b.data()+16)};
    if(!w.contains(next.x,next.y)||next.z< -127||next.z>127){error="Invalid saved position";return false;}
    a=next;error="Position restored";return true;
}
char mapCell(const World&w,const Actor&a,int x,int y){
    if(!w.contains(x,y))return ' ';
    if(x==a.x&&y==a.y)return '@';
    const auto&l=w.land[y*w.width+x];if(l.flags&Wet)return '~';
    if(l.flags&Impassable)return '#';
    bool floor=false;
    for(size_t i:w.cells[y*w.width+x]){
        const auto&o=w.objects[i];
        if((o.flags&Impassable)&&o.z<a.z+16&&o.z+std::max(1,o.height)>a.z)return '#';
        if((o.flags&Surface)&&o.z<=a.z+2&&o.z>=a.z-2)floor=true;
    }
    return floor?'+':'.';
}
}
