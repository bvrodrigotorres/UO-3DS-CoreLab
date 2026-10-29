// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <array>
#include <cstdio>
#include <fstream>
#include <cerrno>
#include <sys/stat.h>
namespace lab {
// This small atomic record is the authority for the player's current zone.
// Destination inventory/position/vitals must be durable BEFORE switching it.
struct Journey {
    unsigned zone=0;
    Actor returnPoint{1492,1640,24};
    static uint32_t hash(const unsigned char*b){uint32_t h=2166136261u;for(unsigned i=0;i<24;++i)h=(h^b[i])*16777619u;return h;}
    bool load(const std::string&path){
        struct stat st;std::string input=path;
        if(::stat(path.c_str(),&st)!=0){
            if(errno!=ENOENT)return false;
            input=path+".bak";
            if(::stat(input.c_str(),&st)!=0){if(errno!=ENOENT)return false;*this=Journey{};return true;}
        }
        // Only recover an absent primary. Corrupt records must fail closed;
        // selecting an older zone silently could discard recent possessions.
        std::ifstream f(input,std::ios::binary);if(!f)return false;
        std::array<unsigned char,28>b{};if(!f.read((char*)b.data(),b.size())||f.peek()!=EOF)return false;
        auto get=[&](unsigned p){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(b[p+i])<<(8*i);return v;};
        if(get(0)!=0x31524a55||get(4)>1||get(20)!=0||get(24)!=hash(b.data()))return false;
        Actor point{int(get(8)),int(get(12)),int(int32_t(get(16)))};
        if(point.x<1088||point.x>1855||point.y<1216||point.y>1983||point.z< -128||point.z>127)return false;
        zone=get(4);returnPoint=point;return true;
    }
    bool save(const std::string&path,int(*renameFile)(const char*,const char*)=std::rename)const{
        if(zone>1||returnPoint.x<1088||returnPoint.x>1855||returnPoint.y<1216||returnPoint.y>1983||returnPoint.z< -128||returnPoint.z>127)return false;
        std::array<unsigned char,28>b{};auto put=[&](unsigned p,uint32_t v){for(unsigned i=0;i<4;++i)b[p+i]=v>>(8*i);};
        put(0,0x31524a55);put(4,zone);put(8,returnPoint.x);put(12,returnPoint.y);put(16,returnPoint.z);put(24,hash(b.data()));
        std::string tmp=path+".tmp";FILE*f=std::fopen(tmp.c_str(),"wb");if(!f)return false;
        bool ok=std::fwrite(b.data(),1,b.size(),f)==b.size();if(std::fflush(f))ok=false;if(std::fclose(f))ok=false;
        if(!ok){std::remove(tmp.c_str());return false;}
        // FSUSER_RenameFile on the 3DS SD never overwrites an existing path.
        // Rotate the old authority first, exactly as position/inventory saves do.
        // Destination possessions are already durable before this commit.
        const std::string backup=path+".bak";struct stat st;
        bool exists=::stat(path.c_str(),&st)==0;
        if(!exists&&errno!=ENOENT){std::remove(tmp.c_str());return false;}
        if(exists){
            if((std::remove(backup.c_str())!=0&&errno!=ENOENT)||renameFile(path.c_str(),backup.c_str())!=0){std::remove(tmp.c_str());return false;}
        }
        if(renameFile(tmp.c_str(),path.c_str())==0)return true;
        // A failed restore leaves the backup readable by load(), including
        // recovery after an interruption between the two successful renames.
        if(exists)renameFile(backup.c_str(),path.c_str());
        std::remove(tmp.c_str());return false;
    }
};
inline bool sewerPortal(const World&w,const Actor&a,unsigned zone){int x=a.x+w.originX,y=a.y+w.originY;return zone?std::abs(x-6031)<=2&&y>=1498&&y<=1502&&a.z>=25:std::abs(x-1491)<=2&&y>=1639&&y<=1643&&std::abs(a.z-24)<=8;}
}
