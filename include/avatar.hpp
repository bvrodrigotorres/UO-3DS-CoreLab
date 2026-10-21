// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <string>
namespace lab {
bool verifyRunAtlas(const std::string&atlas,const std::string&metadata);
inline unsigned avatarImage(int direction,bool moving,bool running,unsigned frame,size_t count){
    unsigned uo=((unsigned(direction)&7)+7)&7;
    if(!moving)return uo*5;
    frame%=4;
    if(running&&count>=72)return 40+uo*4+frame;
    return uo*5+1+frame;
}
}
