// SPDX-License-Identifier: MIT
#include "world_expedition.hpp"
#include "npcs.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){assert(argc==4);std::string regional=std::string(argv[1])+"/",graphicsRoot=std::string(argv[3])+"/",error;
 World source;assert(loadWorld(regional+"scene.bin",source,error));Npcs population;assert(population.loadPopulation(regional+"population.bin",regional+"scene.bin",source,error));assert(population.loadNames(regional+"npc_names.bin",regional+"scene.bin",regional+"population.bin",error));population.reset(source,{source.startX,source.startY,source.startZ});
 Npcs pilot;pilot.importWindowSpawns(population,source.originX,source.originY);size_t roster=pilot.all().size();WorldArchive archive;WorldGraphics graphics;assert(archive.open(argv[2],error)&&graphics.load(graphicsRoot,error));
 int previousX=0,previousY=0;unsigned seenHumans=0,seenAnimals=0,seenAggressive=0;
 for(auto global:{Actor{1496,1628,20},Actor{1430,1723,20},Actor{1378,1488,10},Actor{1240,1463,0},Actor{1377,1381,0},Actor{4467,1283,0},Actor{1377,1381,0},Actor{1496,1628,20}}){
  World w;int ox=std::max(0,(global.x-32)/16*16),oy=std::max(0,(global.y-32)/16*16);assert(archive.window(ox,oy,64,w,error)&&graphics.bind(w,error));Actor local{global.x-ox,global.y-oy,global.z};pilot.shiftWindow(previousX-ox,previousY-oy);previousX=ox;previousY=oy;
  for(int pass=0;pass<40;++pass)pilot.prepareWindow(w,local);
  assert(pilot.all().size()==roster&&pilot.activeCount()<=Npcs::WindowActiveLimit);unsigned humans=0,animals=0,hostile=0;
  for(size_t i=0;i<roster;++i)if(pilot.simulates(i)){const auto&n=pilot.all()[i];assert(canStand(w,n.position)&&n.style>=0&&size_t(n.style)<pilot.styles().size()&&!n.name.empty());humans+=!n.animal;animals+=n.animal;hostile+=n.aggressive;}
  seenHumans+=humans;seenAnimals+=animals;seenAggressive+=hostile;std::cout<<global.x<<","<<global.y<<" humans="<<humans<<" animals="<<animals<<" aggressive="<<hostile<<" roster="<<roster<<"\n";
 }
 assert(seenHumans&&seenAnimals&&seenAggressive);std::cout<<"Actual regional spawns bind safely across town, cemetery, forests, distant travel and return without roster growth\n";
}
