// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "cell_index.hpp"

namespace lab {
constexpr uint64_t Background=1, Wall=0x10, Impassable=0x40, Wet=0x80;
constexpr uint64_t Surface=0x200, Bridge=0x400, Roof=0x10000000, Door=0x20000000;
constexpr uint16_t NoImage=65535;
struct Land {
    uint64_t flags=0;
    uint16_t art=3, image=NoImage;
    bool texture=false;
};
struct Object {
    uint64_t flags=0;
    uint32_t order=0;
    int16_t x=0,y=0;
    uint16_t art=0,hue=0,image=NoImage;
    int8_t z=0;uint8_t height=0;
};

static_assert(sizeof(Land)==16&&sizeof(Object)==24,"World records must retain their bounded RAM layout");
struct World {
    int width=0,height=0,originX=0,originY=0;
    int startX=0,startY=0,startZ=0,images=0;
    uint32_t compatiblePosition=0,compatibleInventory=0;
    std::vector<int8_t> vertices;
    std::vector<Land> land;
    std::vector<Object> objects;
    CellIndex cells;
    void index();
    void moved(size_t index,int oldX,int oldY);
    bool contains(int x,int y) const;
    int vertex(int x,int y) const;
};
struct Heights { int top,right,bottom,left,minimum,average; bool slope; };
Heights heights(const World&,int x,int y);
int directionalHeight(const Heights&,int direction);
enum class Kind { Land,Static,Actor,Npc,Corpse,Blood,Ground };
struct Draw { Kind kind; int x,y,z,priority; size_t index; uint32_t order; };
struct Actor {int x=0,y=0,z=0;};
int priority(Kind kind,int z,int height,uint64_t flags,int average,bool slope);
std::vector<Draw> renderList(const World&,const Actor&,bool cutaway);
// Worlds are immutable after index(). Explicitly invalidate after editing one.
class RenderCache {
    const World* world=nullptr;
    Actor actor;
    bool cutaway=false;
    std::vector<Draw> entries;
public:
    unsigned rebuilds=0;
    const std::vector<Draw>&get(const World&w,const Actor&a,bool cut){
        if(world!=&w||a.x!=actor.x||a.y!=actor.y||a.z!=actor.z||cut!=cutaway){
            entries=renderList(w,a,cut);world=&w;actor=a;cutaway=cut;++rebuilds;
        }
        return entries;
    }
    void invalidate(){world=nullptr;}
    void release(){world=nullptr;std::vector<Draw>().swap(entries);}
};
struct Candidate {unsigned flags;int z,average,height;bool terrain=false;};
// Normal walking subset of CrossUO CalculateNewZ, isolated from player/network/UI.
bool solveCandidates(std::vector<Candidate> list,int currentZ,int minZ,int maxZ,int& nextZ);
bool step(const World&,const Actor&,int direction,Actor& result,int& actualDirection);
bool loadWorld(const std::string&,World&,std::string& error);
World demoWorld();
}
