// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
namespace lab {
enum class RoofMode { Auto, Hidden, Full };
inline RoofMode nextRoofMode(RoofMode m){return m==RoofMode::Auto?RoofMode::Hidden:m==RoofMode::Hidden?RoofMode::Full:RoofMode::Auto;}
inline const char*roofModeName(RoofMode m){return m==RoofMode::Auto?"ROOFS AUTO":m==RoofMode::Hidden?"ROOFS HIDDEN":"ROOFS FULL";}
struct ScreenRect {float left,top,right,bottom;};
// Evaluate the rendered sprite, including its overhang, rather than its map cell.
// Only objects painted after the actor can obscure it. Floors below the feet
// stay opaque; this changes presentation only, never movement or world data.
inline float objectOpacity(const Object&o,float footZ,RoofMode mode,bool afterActor,ScreenRect r){
    if(mode==RoofMode::Full)return 1;
    bool upper=(o.flags&(Roof|Surface))&&o.z>=footZ+14;
    if(mode==RoofMode::Hidden&&upper)return 0;
    bool wall=(o.flags&Wall)&&o.z+o.height>footZ+5;
    bool overlaps=r.left<213&&r.right>187&&r.top<131&&r.bottom>81;
    return afterActor&&(upper||wall)&&overlaps?.18f:1.f;
}
}
