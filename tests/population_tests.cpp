// SPDX-License-Identifier: MIT
#include "npcs.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    if(argc!=2)return 2;
    std::string root=argv[1],error;World w;assert(loadWorld(root+"/scene.bin",w,error));
    Npcs npcs;assert(npcs.loadPopulation(root+"/population.bin",root+"/scene.bin",w,error));
    bool names=std::ifstream(root+"/npc_names.bin").good();
    if(names)assert(npcs.loadNames(root+"/npc_names.bin",root+"/scene.bin",root+"/population.bin",error));
    Actor player{w.startX,w.startY,w.startZ}; npcs.reset(w,player);assert(npcs.all().size()==196);assert(npcs.styles().size()==17);
    int animals=0,protectedNames=0;
    for(size_t i=0;i<npcs.all().size();++i){const auto&n=npcs.all()[i];animals+=n.animal;if(names){protectedNames+=n.nameHue==0x35;assert(!n.name.empty());assert(n.name!="a ship key");if(n.animal)assert(n.nameHue==0x3B2&&n.title.empty());else assert(!n.title.empty());}assert(canStand(w,n.position));
        for(size_t j=0;j<i;++j){const auto&p=npcs.all()[j];assert(p.position.x!=n.position.x||p.position.y!=n.position.y||std::abs(p.position.z-n.position.z)>=16);}}
    assert(animals==87);if(names)assert(protectedNames==101);Doors doors;assert(doors.load(root+"/doors.bin",root+"/scene.bin",w,error));
    for(int tick=0;tick<12000;++tick){if(tick%100==0)player=npcs.all()[(tick/100)%196].home;
        npcs.update(w,player,player,false,1.f/60,doors);doors.update(w,1.f/60);
        for(const auto&n:npcs.all()){assert(w.contains(n.position.x,n.position.y));assert(std::abs(n.position.x-n.home.x)<=n.roam);assert(std::abs(n.position.y-n.home.y)<=n.roam);}}
    Npcs invalid;assert(!invalid.loadPopulation(root+"/scene.bin",root+"/scene.bin",w,error));
    std::cout<<"196 population placements, 87 animals, 17 styles and 12000 simulation ticks passed\n";
}
