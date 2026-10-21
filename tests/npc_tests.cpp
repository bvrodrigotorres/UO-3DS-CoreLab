// SPDX-License-Identifier: MIT
#include "npcs.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    World w;std::string error;
    if(argc==3){assert(loadWorld(argv[1],w,error));}
    else {w.width=w.height=32;w.originX=1488;w.originY=1596;w.vertices.assign(33*33,20);w.land.resize(32*32);w.index();}
    Actor player{13,16,20};Npcs npcs;npcs.reset(w,player);assert(npcs.all().size()==3);
    Doors doors;if(argc==3)assert(doors.load(argv[2],argv[1],w,error));
    auto canonical=w.objects;bool walked=false;int updates=0;
    std::vector<Actor>occupied;
    for(int tick=0;tick<12000;++tick){
        occupied={player};npcs.addOccupants(occupied);doors.update(w,.016f,occupied);
        npcs.update(w,player,player,false,.016f,doors);
        for(size_t i=0;i<npcs.all().size();++i){const auto&n=npcs.all()[i];
            assert(canStand(w,n.position));assert(n.position.z==20);
            assert(std::abs(n.position.x-n.home.x)<=3&&std::abs(n.position.y-n.home.y)<=3);
            assert(n.position.x!=player.x||n.position.y!=player.y);
            assert(!npcs.occupied(n.position,(int)i));
            if(n.moving)walked=true;
        }
        ++updates;
    }
    assert(walked);assert(!npcs.greeting({12,16,20}).empty());
    auto draws=npcs.drawList(w,player,true);assert(!draws.empty());
    for(const auto&d:draws){assert(d.kind==Kind::Npc&&d.index<npcs.all().size());}
    for(size_t i=1;i<draws.size();++i)assert(!drawBefore(draws[i],draws[i-1]));
    Draw floor{Kind::Land,5,5,20,20,0,0},person{Kind::Npc,5,5,20,21,0,0xffffff00};assert(drawBefore(floor,person));
    assert(!canStand(w,{-1,0,20}));
    std::cout<<"NPC wandering, supports, occupancy, greetings and depth order: "<<updates<<" ticks passed\n";
}
