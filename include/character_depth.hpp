// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <algorithm>
namespace lab {
// Preserve the painter list. Only lower walkable fragments test the player's
// opaque-pixel depth marker; ordinary scenery always keeps painter semantics.
inline bool supportingFragment(const World&w,const Draw&d,const Actor&a,float visualZ){
    const float ceiling=std::max(float(a.z),visualZ)+1.f;
    if(d.kind==Kind::Land){
        auto h=heights(w,d.x,d.y);
        return std::max({h.top,h.right,h.bottom,h.left})<=ceiling;
    }
    if(d.kind!=Kind::Static||d.index>=w.objects.size())return false;
    const auto&o=w.objects[d.index];
    if(!(o.flags&(Surface|Bridge))||(o.flags&(Wall|Door|Roof|Impassable)))return false;
    // Bridge tiles are walked at half their tiledata height (same as movement).
    return o.z+((o.flags&Bridge)?o.height/2:o.height)<=ceiling;
}
constexpr float WorldDepthZ=0.f,CharacterDepthZ=.5f;
// Mtx_OrthoTilt(-1,1,leftHanded=true) and DepthMap(true,-1,0).
constexpr float fragmentDepth(float vertexZ){return .5f-.5f*vertexZ;}
}
