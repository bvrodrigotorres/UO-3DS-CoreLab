// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include "avatar.hpp"
namespace lab {
constexpr unsigned MountedBank=1u<<21;
constexpr unsigned MountedFrames=128,MountedSize=128;
// ClassicUO MovementSpeed: 400/200 ms on foot, 200/100 ms mounted.
constexpr float movementSeconds(bool running,bool mounted){return mounted?(running?.10f:.20f):(running?.20f:.40f);}
inline unsigned mountedFrame(int direction,bool moving,bool running,unsigned phase,bool striking=false){
    unsigned d=avatarDirection(direction);
    if(striking&&!moving)return 88+d*5+std::min(4u,phase);
    return d*11+(moving?(running?6u:1u)+phase%5:0u);
}
inline bool horseIconHit(int x,int y){return x>=198&&x<258&&y>=203&&y<223;}
}
