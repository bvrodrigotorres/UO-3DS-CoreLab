// SPDX-License-Identifier: MIT
#include "developer_menu.hpp"
#include "equipment_menu.hpp"
#include "hud.hpp"
#include "character.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
static_assert(sizeof(Npcs)<1024,"Spatial buckets must not be embedded in stack NPC instances");
static_assert(sizeof(EquipmentMenu)<2048,"Equipment editor must remain lightweight");
int main(int argc,char**argv){
 World w;w.width=w.height=24;w.originX=1360;w.originY=1470;w.startX=w.startY=8;w.vertices.assign(625,0);w.land.resize(576);w.index();Npcs npcs;Npc smith;smith.name="Smith";smith.title="the blacksmith";smith.position=smith.from=smith.home={10,10,0};npcs.combatants().push_back(smith);Actor tile;
 assert(safeTeleportTile(w,npcs,smith.position,tile)&&canStand(w,tile)&&!npcs.occupied(tile)&&std::max(std::abs(tile.x-10),std::abs(tile.y-10))==1);
 Object floor;floor.x=floor.y=10;floor.z=20;floor.flags=Surface;w.objects.push_back(floor);w.index();npcs.combatants()[0].position.z=20;assert(!safeTeleportTile(w,npcs,{10,10,20},tile)); // cannot use a different floor beneath the vendor
 assert(!safeTeleportTile(w,npcs,{-1,0,0},tile));w.objects.clear();for(auto&l:w.land)l.flags=Wet;w.index();assert(!safeTeleportTile(w,npcs,{8,8,0},tile));for(auto&l:w.land)l.flags=0;w.index();npcs.combatants()[0].position.z=0;
 DeveloperMenu menu(w,npcs);assert(menu.destinations.size()==10&&menu.destinations[0].available&&menu.destinations[1].available&&!menu.destinations[2].available);menu.selected=1;assert(menu.resolve(w,npcs,tile)&&canStand(w,tile)&&!npcs.occupied(tile));menu.pageMove();assert(menu.page()==1);assert(menu.tap(30,220)==-1&&menu.tap(200,220)==1);
 std::vector<uint8_t>fb(320*240*3+32,0xa5);for(int i=0;i<10;++i){menu.selected=i;drawDeveloperMenu(fb.data()+16,menu,w);}for(int i=0;i<16;++i)assert(fb[i]==0xa5&&fb[fb.size()-1-i]==0xa5);
 PauseMenu pause;pause.selected=3;assert(pause.activate()==PauseAction::Options);pause.selected=4;assert(pause.activate()==PauseAction::Explore);pause.selected=5;assert(pause.activate()==PauseAction::MainMenu);pause.selected=6;assert(pause.activate()==PauseAction::Exit);
 // Small menu-side keys must never require an Inventory on the call stack.
 Inventory bag;bag.equipment[0]=16;bag.equipment[8]=27;bag.equipment[9]=28;assert(Inventory::appearanceKey(bag.equipment)==bag.equipmentMask());
 if(argc==2){World actual;std::string e;assert(loadWorld(std::string(argv[1])+"/scene.bin",actual,e));Npcs population;assert(population.loadPopulation(std::string(argv[1])+"/population.bin",std::string(argv[1])+"/scene.bin",actual,e));assert(population.loadNames(std::string(argv[1])+"/npc_names.bin",std::string(argv[1])+"/scene.bin",std::string(argv[1])+"/population.bin",e));population.reset(actual,{actual.startX,actual.startY,actual.startZ});DeveloperMenu real(actual,population);for(size_t i=0;i<real.destinations.size();++i){real.selected=int(i);assert(real.resolve(actual,population,tile)&&canStand(actual,tile)&&!population.occupied(tile));std::cout<<real.destinations[i].name<<": "<<tile.x+actual.originX<<","<<tile.y+actual.originY<<","<<tile.z<<"\n";}}
 std::cout<<"Developer 33: safe POIs, vendor occupancy, no lower-floor/water/outside teleport, menu touch/pause/framebuffer bounds and lightweight equipment keys passed\n";
}
