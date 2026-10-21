// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
namespace lab {
struct DoorState {
    size_t index=0;Object closed;uint16_t openArt=0,openImage=NoImage,pair=65535;
    int dx=0,dy=0;float progress=0;bool opening=false,open=false;
};
struct DoorVisual {float x,y;uint16_t image;};
class Doors {
    std::vector<DoorState> entries;
    std::vector<int> lookup;
public:
    unsigned baseObjects=0;
    bool load(const std::string&,const std::string&scene,const World&,std::string&);
    bool tryOpen(const Actor&,int dx,int dy);
    bool update(World&,float dt);
    bool visual(size_t index,DoorVisual&)const;
    size_t count()const{return entries.size();}
};
}
