// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <algorithm>
namespace lab {
// Only supporting ground belongs in the first pass. Raised obstacles, stairs,
// walls and upper floors retain the normal isometric occlusion order.
inline bool groundPass(const World&w,const Draw&d,float footZ){
    if(d.kind==Kind::Land){auto h=heights(w,d.x,d.y);return std::max({h.top,h.right,h.bottom,h.left})<=footZ+.01f;}
    if(d.kind!=Kind::Static||d.index>=w.objects.size())return false;
    const auto&o=w.objects[d.index];
    return o.height==0&&(o.flags&Surface)&&!(o.flags&(Wall|Door|Roof|Bridge|Impassable))&&o.z<=footZ+.01f;
}
inline bool objectCoveredByGround(const World&w,const Object&object,float footZ){
    if(!w.contains(object.x,object.y))return false;
    for(size_t i:w.cells[object.y*w.width+object.x]){
        const auto&o=w.objects[i];Draw d{Kind::Static,o.x,o.y,o.z,0,i,0};
        if(o.z>object.z&&o.z>=object.z+std::max(0,object.height)&&groundPass(w,d,footZ))return true;
    }
    return false;
}
// A floor moved into the ground pass still hides inhabitants of a lower storey.
inline bool coveredByGround(const World&w,const Actor&a,float footZ){
    if(!w.contains(a.x,a.y))return false;
    for(size_t i:w.cells[a.y*w.width+a.x]){
        const auto&o=w.objects[i];
        Draw d{Kind::Static,o.x,o.y,o.z,0,i,0};
        if(o.z>=a.z+14&&groundPass(w,d,footZ))return true;
    }
    return false;
}
}
