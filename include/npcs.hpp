// SPDX-License-Identifier: MIT
#pragma once
#include "doors.hpp"
#include <array>
#include "overhead_font.hpp"
namespace lab {
struct Npc {
    Actor position,from,home;int direction=4,style=0;float progress=1,wait=1,animation=0;bool moving=false;
    bool alive=true,hostile=false,aggressive=false,stationary=false,swims=false,waterOnly=false;int maximumHits=0,meleeMin=0,meleeMax=0,respawnSeconds=20;
    uint64_t aggressionOrder=0;NamePresence namePresence;std::string name,greeting,title;std::array<uint8_t,3>nameColor{{238,238,0}};uint16_t nameHue=0x35;int roam=3;bool animal=false;
};
struct NpcStyle {int pivotX=0,pivotY=0,width=0,height=0;unsigned bytes=0,checksum=0;};
class Npcs {
    bool windowed=false;
    std::vector<unsigned char>simulation,placement;
    bool occupancyActive=false;mutable bool occupancyDirty=true;mutable std::vector<int>occupancyHead=std::vector<int>(4096,-1);mutable std::vector<int>occupancyNext;
    mutable unsigned occupancyBuilds=0;
    void rebuildOccupancy()const;
    void occupancyMoved(size_t,const Actor&,const Actor&,bool);
    void beginMove(size_t,const Actor&,int);
    bool deathFrames=false;
    uint64_t aggressionClock=0;size_t routeCursor=0;unsigned routeNodes=0,routeSearches=0;
    std::vector<Npc> people,original;std::vector<NpcStyle>art;uint32_t random=12345;
public:
    static constexpr unsigned WindowActiveLimit=64;
    // Stable regional roster: moving the geometry window never recreates NPCs.
    void importWindowSpawns(const Npcs&,int dx,int dy);
    void shiftWindow(int dx,int dy);
    void prepareWindow(const World&,const Actor&);
    bool simulates(size_t i)const{return !windowed||(i<simulation.size()&&simulation[i]);}
    unsigned activeCount()const;
    bool loadPopulation(const std::string&,const std::string&scene,const World&,std::string&);
    bool loadNames(const std::string&,const std::string&scene,const std::string&population,std::string&);
    unsigned lastOccupancyBuilds()const{return occupancyBuilds;}
    unsigned lastRouteNodes()const{return routeNodes;}
    unsigned lastRouteSearches()const{return routeSearches;}
    unsigned frameCount()const{return deathFrames?48:40;}
    bool hasDeathFrames()const{return deathFrames;}
    bool hasPopulation()const{return !art.empty();}
    const std::vector<NpcStyle>&styles()const{return art;}
    void reset(const World&,const Actor&player);
    void engage(size_t index){if(index>=people.size())return;auto&n=people[index];if(!n.hostile||!n.aggressionOrder)n.aggressionOrder=++aggressionClock;n.hostile=true;}
    void update(const World&,const Actor&player,const Actor&from,bool playerMoving,float dt,Doors&);
    bool occupied(const Actor&,int except=-1)const;
    void addOccupants(std::vector<Actor>&)const;
    std::string greeting(const Actor&)const;
    std::vector<Npc>&combatants(){occupancyDirty=true;return people;}
    const std::vector<Npc>&all()const{return people;}
    std::vector<Draw> drawList(const World&,const Actor&,bool cutaway)const;
};
bool canStand(const World&,const Actor&);
bool canStandMobile(const World&,const Actor&,bool swims,bool waterOnly);
bool stepMobile(const World&,const Actor&,int direction,Actor&,int&actual,bool swims,bool waterOnly);
bool drawBefore(const Draw&,const Draw&);
}
