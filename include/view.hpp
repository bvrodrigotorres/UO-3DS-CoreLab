// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <algorithm>
#include <array>
namespace lab {
inline bool worldVisible(const World&w,const Draw&d,float cameraX,float cameraY,float footZ,const std::vector<std::array<unsigned,2>>&sizes){
    auto project=[&](float x,float y,float z){return std::array<float,2>{200+(x-cameraX-y+cameraY)*22,126+(x-cameraX+y-cameraY)*22-(z-footZ)*4};};
    if(d.kind==Kind::Land){
        auto h=heights(w,d.x,d.y);auto p=std::array<std::array<float,2>,4>{project(d.x,d.y,h.top),project(d.x+1,d.y,h.right),project(d.x+1,d.y+1,h.bottom),project(d.x,d.y+1,h.left)};
        float l=p[0][0],r=l,t=p[0][1]-22,b=t;
        for(auto v:p){l=std::min(l,v[0]);r=std::max(r,v[0]);t=std::min(t,v[1]-22);b=std::max(b,v[1]-22);}return l<400&&r>0&&t<240&&b>0;
    }
    if(d.kind==Kind::Static){
        const auto&o=w.objects[d.index];if(o.image==NoImage||o.image>=sizes.size())return false;
        auto c=project(d.x,d.y,d.z);auto size=sizes[o.image];
        return c[0]-float(size[0])/2<400&&c[0]+float(size[0])/2>0&&c[1]+22-size[1]<240&&c[1]+22>0;
    }
    return true;
}
}
