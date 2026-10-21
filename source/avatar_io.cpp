// SPDX-License-Identifier: MIT
#include "avatar.hpp"
#include <fstream>
#include <cstdint>
namespace lab {
bool verifyRunAtlas(const std::string&path,const std::string&metadata){
    std::ifstream meta(metadata);std::string magic;unsigned body=0,walk=0,idle=0,run=0,count=0;uint32_t expected=0;
    if(!(meta>>magic>>body>>walk>>idle>>run>>count>>std::hex>>expected)||magic!="UOA1"||(body!=400&&body!=401)||walk!=0||idle!=4||run!=2||count!=72)return false;
    meta>>std::ws;if(meta.peek()!=EOF)return false;
    std::ifstream file(path,std::ios::binary);if(!file)return false;
    uint32_t hash=2166136261u;char buf[4096];
    while(file){file.read(buf,sizeof(buf));for(std::streamsize i=0;i<file.gcount();++i)hash=(hash^(unsigned char)buf[i])*16777619u;}
    return file.eof()&&hash==expected;
}
}
