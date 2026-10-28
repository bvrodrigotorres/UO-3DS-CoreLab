// SPDX-License-Identifier: MIT
#pragma once
#include <fstream>
#include <array>
#include <cstdint>
namespace lab {
inline bool fileHash(const std::string&path,size_t maximum,uint32_t&result){
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||f.tellg()>std::streamoff(maximum))return false;
    size_t remaining=size_t(f.tellg());f.seekg(0);uint32_t h=2166136261u;std::array<unsigned char,4096>b;
    while(remaining){size_t n=std::min(remaining,b.size());if(!f.read(reinterpret_cast<char*>(b.data()),n))return false;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;remaining-=n;}
    result=h;return true;
}
}
