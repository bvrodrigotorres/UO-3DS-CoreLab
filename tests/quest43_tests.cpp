// SPDX-License-Identifier: MIT
#include "quests.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int fail(const char*,const char*){return -1;}
int main(int argc,char**argv){
 assert(argc>=2);std::string root=std::string(argv[1])+"/",error;
 World w;w.width=w.height=32;w.originX=1360;w.originY=1470;w.startX=w.startY=5;w.vertices.assign(33*33,0);w.land.resize(32*32);w.index();
 Npcs npcs;Npc healer;healer.name="Ansel";healer.title="the healer";healer.position=healer.from=healer.home={6,5,0};Npc vendor=healer;vendor.name="Mira";vendor.title="the provisioner";vendor.position=vendor.from=vendor.home={9,5,0};Npc citizen=healer;citizen.name="Doran";citizen.title="the bard";citizen.position=citizen.from=citizen.home={12,5,0};npcs.combatants()={healer,vendor,citizen};
 Inventory bag;bag.init(w,root+"quests.sav");bag.backpack={{0,10}};assert(bag.checkpoint());
 Quests quests;quests.setup(w,npcs);Actor player{5,5,0};assert(quests.giver(0,npcs,bag)==0&&quests.giver(1,npcs,bag)==1);
 assert(!quests.marker(npcs,bag).active);assert(!quests.interact(w,npcs,{20,20,0},bag));
 Object wall;wall.x=6;wall.y=5;wall.z=0;wall.height=20;wall.flags=Impassable;w.objects={wall};w.index();assert(!quests.interact(w,npcs,player,bag));w.objects.clear();w.index();
 npcs.combatants()[0].position.z=40;assert(!quests.interact(w,npcs,player,bag));npcs.combatants()[0].position.z=0;
 assert(!quests.killed(w,healer,bag));assert(quests.interact(w,npcs,player,bag));assert(bag.quests[0].state==1);
 Npc skeleton;skeleton.name="a skeleton";skeleton.animal=true;skeleton.position={18,18,0};
 assert(Quests::cemetery(w,skeleton));Npc outside=skeleton;outside.position={1000,1000,0};assert(!quests.killed(w,outside,bag));Npc rat=skeleton;rat.name="a rat";assert(!quests.killed(w,rat,bag));
 assert(!quests.killed(w,skeleton,bag,fail)&&bag.quests[0].count==0);
 for(int i=0;i<3;++i)assert(quests.killed(w,skeleton,bag));
 Inventory restored;restored.init(w,root+"quests.sav");assert(restored.healthy()&&restored.restored&&restored.quests[0].count==3&&restored.backpack[0].count==10);
 bag=restored;assert(quests.killed(w,skeleton,bag));assert(quests.killed(w,skeleton,bag));assert(!quests.killed(w,skeleton,bag));assert(quests.ready(0,bag));
 auto marker=quests.marker(npcs,bag);assert(marker.active&&marker.x==6&&marker.y==5);
 npcs.combatants()[0].position.x=7;marker=quests.marker(npcs,bag);assert(marker.x==7);npcs.combatants()[0].position.x=6;
 auto count=Quests::owned(bag,0);assert(!quests.interact(w,npcs,player,bag,fail));assert(bag.quests[0].state==1&&Quests::owned(bag,0)==count);
 // A full bag may not partially grant gold then lose the second reward.
 auto previous=bag.backpack;bag.backpack={{0,10}};while(bag.backpack.size()<BagSlots)bag.backpack.push_back({5,1});assert(!quests.interact(w,npcs,player,bag));assert(Quests::owned(bag,0)==10&&bag.quests[0].state==1);bag.backpack=previous;
 assert(quests.interact(w,npcs,player,bag));assert(bag.quests[0].state==2&&Quests::owned(bag,0)==160&&Quests::owned(bag,3)==5);assert(!quests.marker(npcs,bag).active);
 Inventory rewarded;rewarded.init(w,root+"quests.sav");assert(rewarded.restored&&rewarded.quests[0].state==2);assert(!quests.interact(w,npcs,player,rewarded)&&Quests::owned(rewarded,0)==160);
 std::swap(npcs.combatants()[0],npcs.combatants()[2]);assert(quests.giver(0,npcs,rewarded)==2);std::swap(npcs.combatants()[0],npcs.combatants()[2]);
 quests.select(1);assert(quests.interact(w,npcs,{8,5,0},bag));assert(!quests.ready(1,bag));bag.backpack.push_back({1,3});assert(quests.ready(1,bag));assert(quests.marker(npcs,bag).active);
 assert(!quests.interact(w,npcs,{5,5,0},bag));assert(bag.quests[1].state==1); // wrong NPC cannot claim
 assert(!quests.interact(w,npcs,{8,5,0},bag,fail));assert(Quests::owned(bag,1)==3&&Quests::owned(bag,3)==5);
 bag.saveSink=[](const std::string&,std::vector<uint8_t>){return false;};assert(!quests.interact(w,npcs,{8,5,0},bag));assert(bag.quests[1].state==1&&Quests::owned(bag,1)==3&&Quests::owned(bag,0)==160);bag.saveSink={};
 assert(quests.interact(w,npcs,{8,5,0},bag));assert(bag.quests[1].state==2&&Quests::owned(bag,0)==240&&!Quests::owned(bag,1)&&!Quests::owned(bag,3));
 assert(!quests.interact(w,npcs,{8,5,0},bag)&&Quests::owned(bag,0)==240);
 // Combat emits the mission event only for a real death, not an attack/MISS.
 Npcs enemy;Npc weak=skeleton;weak.position=weak.from=weak.home={6,5,0};weak.maximumHits=1;weak.name="a skeleton";weak.meleeMax=0;enemy.combatants()={weak};Combat fight;fight.reset(enemy);int deaths=0;fight.onKill=[&](const Npc&n){assert(!n.alive);++deaths;};PlayerStatus status;fight.toggle(enemy,player);fight.update(w,enemy,player,false,.01f,status,bag);assert(deaths==1);fight.update(w,enemy,player,false,.01f,status,bag);assert(deaths==1);
 auto pane=quests.panel(w,npcs,{11,5,0},bag);assert(pane.speaker=="Doran");auto line=pane.dialogue;quests.interact(w,npcs,{11,5,0},bag);assert(quests.panel(w,npcs,{11,5,0},bag).dialogue!=line);
 std::vector<uint8_t>pixels(320*240*3+32,0xAA);for(unsigned id=0;id<2;++id){quests.select(id);drawQuests(pixels.data()+16,quests.panel(w,npcs,player,bag));}for(int i=0;i<16;++i)assert(pixels[i]==0xAA&&pixels[pixels.size()-1-i]==0xAA);
 // A valid-checksum but invalid quest record must not become a fresh writable save.
 std::ifstream input(root+"quests.sav",std::ios::binary);std::vector<uint8_t>bytes{std::istreambuf_iterator<char>(input),{}};assert(bytes[3]=='8');bytes[bytes.size()-36]=9;uint32_t hash=2166136261;for(size_t i=0;i<bytes.size()-4;++i)hash=(hash^bytes[i])*16777619;for(int i=0;i<4;++i)bytes[bytes.size()-4+i]=hash>>(8*i);{std::ofstream out(root+"corrupt.sav",std::ios::binary);out.write((char*)bytes.data(),bytes.size());}Inventory bad;bad.init(w,root+"corrupt.sav");assert(!bad.healthy()&&!bad.restored);
 if(argc==3){std::string region=std::string(argv[2])+"/";World real;assert(loadWorld(region+"scene.bin",real,error));Npcs population;assert(population.loadPopulation(region+"population.bin",region+"scene.bin",real,error));assert(population.loadNames(region+"npc_names.bin",region+"scene.bin",region+"population.bin",error));population.reset(real,{real.startX,real.startY,real.startZ});Quests actual;actual.setup(real,population);Inventory empty;unsigned undead=0;for(const auto&n:population.all())if(Quests::cemetery(real,n))++undead;assert(undead>0);for(unsigned q=0;q<2;++q){int id=actual.giver(q,population,empty);assert(id>=0);const auto&n=population.all()[id];std::cout<<"Quest giver "<<q<<": "<<n.name<<" "<<n.title<<" at "<<n.position.x+real.originX<<","<<n.position.y+real.originY<<"\n";}std::cout<<"Matching cemetery creatures: "<<undead<<"\n";}
 std::cout<<"Quests: accept, selective kills, exact turn-in, live marker, conversations, durable one-time rewards, full bag/SD rollback, migration and UI bounds passed\n";
}
