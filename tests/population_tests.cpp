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
    Actor player{w.startX,w.startY,w.startZ}; npcs.reset(w,player);assert(npcs.all().size()==161);assert(npcs.styles().size()==16);
    int animals=0;
    for(size_t i=0;i<npcs.all().size();++i){const auto&n=npcs.all()[i];animals+=n.animal;assert(canStand(w,n.position));
        for(size_t j=0;j<i;++j){const auto&p=npcs.all()[j];assert(p.position.x!=n.position.x||p.position.y!=n.position.y||std::abs(p.position.z-n.position.z)>=16);}}
    assert(animals==62);Doors doors;assert(doors.load(root+"/doors.bin",root+"/scene.bin",w,error));
    for(int tick=0;tick<12000;++tick){if(tick%100==0)player=npcs.all()[(tick/100)%161].home;
        npcs.update(w,player,player,false,1.f/60,doors);doors.update(w,1.f/60);
        for(const auto&n:npcs.all()){assert(w.contains(n.position.x,n.position.y));assert(std::abs(n.position.x-n.home.x)<=n.roam);assert(std::abs(n.position.y-n.home.y)<=n.roam);}}
    Npcs invalid;assert(!invalid.loadPopulation(root+"/scene.bin",root+"/scene.bin",w,error));
    std::cout<<"161 population placements, 62 animals, 16 styles and 12000 simulation ticks passed\n";
}
