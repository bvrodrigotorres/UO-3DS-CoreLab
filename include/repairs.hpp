// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
namespace lab {
// Explicit visual repair requested for the exposed wall cell in Britain.
// It reuses a neighboring wooden floor without changing world/support data.
inline const Object* floorUnderWall(const World&w,const Object&wall){
    if(wall.x+w.originX!=1501||wall.y+w.originY!=1611||wall.z!=40||wall.art!=308||!(wall.flags&Wall))return nullptr;
    for(size_t i:w.cells[wall.y*w.width+wall.x]){
        const auto&o=w.objects[i];if(o.z==40&&(o.flags&Surface)&&!(o.flags&Wall))return nullptr;
    }
    const int dx[]={-1,0,1,0},dy[]={0,1,0,-1};
    for(int direction=0;direction<4;++direction){
        int x=wall.x+dx[direction],y=wall.y+dy[direction];if(!w.contains(x,y))continue;
        for(size_t i:w.cells[y*w.width+x]){
            const auto&o=w.objects[i];
            if(o.z==40&&o.height==0&&(o.flags&Surface)&&o.art>=1205&&o.art<=1212&&o.image!=NoImage)return &o;
        }
    }
    return nullptr;
}
}
