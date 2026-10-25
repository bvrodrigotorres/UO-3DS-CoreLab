// SPDX-License-Identifier: MIT
#pragma once
#include "world_archive.hpp"
#include "journal.hpp"
namespace lab {
// Global catalog -> bounded local scene. Never touches campaign inventory/state.
class WorldGraphics {
    std::vector<uint16_t>land,statics,texture,radar;
    unsigned count=0;
public:
    bool load(const std::string&,std::string&);
    bool bind(World&,std::string&)const;
    Journal map(const World&)const;
    unsigned images()const{return count;}
};
struct ExpeditionWindow {
    static int origin(int global,int extent){return std::max(0,std::min(extent-128,((global-32)/64)*64));}
    static bool needs(const World&w,const Actor&a,int width,int height){
        return (a.x<24&&w.originX>0)||(a.y<24&&w.originY>0)||
            (a.x>=104&&w.originX+128<width)||(a.y>=104&&w.originY+128<height);
    }
    static Actor global(const World&w,const Actor&a){return {a.x+w.originX,a.y+w.originY,a.z};}
    static Actor local(const World&w,const Actor&a){return {a.x-w.originX,a.y-w.originY,a.z};}
};
}
