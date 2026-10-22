// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
namespace lab {
struct DoorState {
    size_t index=0;Object closed;uint16_t openArt=0,openImage=NoImage,pair=65535;
    int dx=0,dy=0;float elapsed=0;bool opening=false,open=false;
};
class Doors {
    std::vector<DoorState> entries;
public:
    unsigned baseObjects=0;
    bool load(const std::string&,const std::string&scene,const World&,std::string&);
    void reset(World&);
    bool tryOpen(const Actor&,int dx,int dy);
    bool update(World&,float dt,const std::vector<Actor>&occupants={});
    size_t count()const{return entries.size();}
};
}
