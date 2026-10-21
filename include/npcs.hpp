// SPDX-License-Identifier: MIT
#pragma once
#include "doors.hpp"
#include <array>
namespace lab {
struct Npc {
    Actor position,from,home;int direction=4,style=0;float progress=1,wait=1,animation=0;bool moving=false;
    std::string name,greeting,title;std::array<uint8_t,3>nameColor{{238,238,0}};uint16_t nameHue=0x35;int roam=3;bool animal=false;
};
struct NpcStyle {int pivotX=0,pivotY=0,width=0,height=0;unsigned bytes=0,checksum=0;};
class Npcs {
    std::vector<Npc> people,original;std::vector<NpcStyle>art;uint32_t random=12345;
public:
    bool loadPopulation(const std::string&,const std::string&scene,const World&,std::string&);
    bool loadNames(const std::string&,const std::string&scene,const std::string&population,std::string&);
    bool hasPopulation()const{return !art.empty();}
    const std::vector<NpcStyle>&styles()const{return art;}
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
