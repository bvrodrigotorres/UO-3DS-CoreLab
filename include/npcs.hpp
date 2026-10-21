// SPDX-License-Identifier: MIT
#pragma once
#include "doors.hpp"
namespace lab {
struct Npc {
    Actor position,from,home;int direction=4,style=0;float progress=1,wait=1,animation=0;bool moving=false;
    std::string name,greeting;
};
class Npcs {
    std::vector<Npc> people;uint32_t random=12345;
public:
    void reset(const World&,const Actor&player);
    void update(const World&,const Actor&player,const Actor&from,bool playerMoving,float dt,Doors&);
    bool occupied(const Actor&,int except=-1)const;
    void addOccupants(std::vector<Actor>&)const;
    std::string greeting(const Actor&)const;
    const std::vector<Npc>&all()const{return people;}
    std::vector<Draw> drawList(const World&,const Actor&,bool cutaway)const;
};
bool canStand(const World&,const Actor&);
bool drawBefore(const Draw&,const Draw&);
}
