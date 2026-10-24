// SPDX-License-Identifier: MIT
#pragma once
#include "npcs.hpp"
#include "journal.hpp"
#include <cmath>
namespace lab {
struct RadarMarker {int x=0,y=0,priority=0;std::array<uint8_t,3>color{};};
inline bool radarMarker(const Npc&n,const Actor&player,int zoom,RadarMarker&out){
    if(!n.alive)return false;
    float t=n.moving?std::max(0.f,std::min(1.f,n.progress)):1.f;
    float dx=n.from.x+(n.position.x-n.from.x)*t-player.x,dy=n.from.y+(n.position.y-n.from.y)*t-player.y;
    int scale=std::max(1,std::min(4,zoom));
    float x=103+(dx-dy)*scale,y=JournalLayout::MapCenter+(dx+dy)*scale;
    if(x<20||x>=186||y<JournalLayout::MapTop||y>=JournalLayout::MapBottom)return false;
    out.x=int(std::floor(x+.5f));out.y=int(std::floor(y+.5f));
    if(out.x>=186||out.y>=JournalLayout::MapBottom)return false;
    out.priority=n.hostile||n.aggressive?2:n.animal?0:1;
    out.color=out.priority==2?std::array<uint8_t,3>{{240,45,35}}:out.priority==0?std::array<uint8_t,3>{{190,190,190}}:std::array<uint8_t,3>{{250,220,45}};
    return true;
}
}
