// SPDX-License-Identifier: MIT
#include "journey.hpp"
#include "quests.hpp"
#include "developer_menu.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <tuple>
using namespace lab;
int refuse(const char*,const char*){return -1;}
int main(int argc,char**argv){
 assert(argc>=2);std::string root=std::string(argv[1])+"/",error;Journey state;
 assert(state.load(root+"journey.sav")&&state.zone==0);assert(state.save(root+"journey.sav"));
 Journey next=state;next.zone=1;assert(!next.save(root+"journey.sav",refuse));Journey after;assert(after.load(root+"journey.sav")&&after.zone==0);
 assert(next.save(root+"journey.sav"));assert(after.load(root+"journey.sav")&&after.zone==1&&after.returnPoint.x==1492);
 {std::ofstream corrupt(root+"journey.sav",std::ios::binary);corrupt<<"UJR1bad";}assert(!after.load(root+"journey.sav"));
 World town;town.originX=1088;town.originY=1216;town.width=town.height=32;town.vertices.assign(33*33,0);town.land.resize(1024);Object chest;chest.art=0xe40;chest.x=2;chest.y=2;town.objects={chest};town.index();
 Inventory bag;bag.init(town,root+"town.sav");bag.backpack={{0,10},{5,1}};bag.equipment[0]=16;bag.lastEquipment[0]=5;bag.dressed=bag.accessories=true;
 bag.ground[0]={{3,3,0},{{1,1}},{180}};bag.chests[0].items={{0,77}};
 Npcs humans;Npc healer;healer.name="Ansel";healer.title="the healer";healer.position=healer.from=healer.home={6,5,0};humans.combatants()={healer};Quests quest;quest.setup(town,humans);quest.select(2);
 assert(quest.interact(town,humans,{5,5,0},bag));assert(bag.quests[2].state==1);
 World sewer=town;sewer.originX=6016;sewer.originY=1408;
 Inventory inside;inside.init(sewer,root+"sewer.sav");inside.chests[0].items={{0,55}};inside.copyPlayerFrom(bag);
 assert(inside.backpack.size()==2&&inside.equipment[0]==16&&inside.lastEquipment[0]==5&&inside.dressed&&inside.accessories);assert(inside.ground[0].items.empty()&&inside.chests[0].items[0].count==55);
 Npc rat;rat.animal=true;rat.aggressive=true;rat.name="a sewer rat";Npc boss=rat;boss.name="El Devorador";
 assert(!quest.killed(sewer,rat,inside,refuse)&&inside.quests[2].count==0);
 for(int i=0;i<5;++i)assert(quest.killed(sewer,rat,inside));assert(!quest.ready(2,inside));assert(!quest.killed(sewer,rat,inside));
 assert(quest.killed(sewer,boss,inside)&&quest.ready(2,inside));assert(!quest.killed(sewer,boss,inside));
 // Reload the new wire format with the new mission and keep old zone containers.
 assert(inside.checkpoint());Inventory reload;reload.init(sewer,root+"sewer.sav");
 assert(reload.healthy()&&reload.restored&&reload.quests[2].count==0x10005u);
 // A crash after writing the destination but before committing the zone
 // must still restore the source; committing then selects the complete target.
 Journey authority;assert(authority.save(root+"authority.sav"));assert(bag.checkpoint());
 Journey resumed;assert(resumed.load(root+"authority.sav")&&resumed.zone==0);
 Inventory source;source.init(town,root+"town.sav");assert(source.restored&&source.quests[2].count==0);
 authority.zone=1;assert(authority.save(root+"authority.sav"));assert(resumed.load(root+"authority.sav")&&resumed.zone==1);
 assert(reload.quests[2].count==0x10005u&&reload.chests[0].items[0].count==55);
 bag.copyPlayerFrom(inside);assert(bag.chests[0].items[0].count==77&&!bag.ground[0].items.empty());
 bag.backpack.push_back({28,1});quest.setup(town,humans);quest.select(2);assert(!quest.interact(town,humans,{5,5,0},bag,refuse)&&bag.quests[2].state==1);
 assert(quest.interact(town,humans,{5,5,0},bag));assert(Quests::owned(bag,0)==260&&Quests::owned(bag,28)==2);assert(!quest.interact(town,humans,{5,5,0},bag));
 assert(!validQuest(2,{2,5,1,1})&&!validQuest(2,{1,0x20005,1,1})&&validQuest(2,{2,0x10005,1,1}));
 World actual;if(argc==3){std::string folder=argv[2];assert(loadWorld(folder+"/scene.bin",actual,error));Doors doors;assert(doors.load(folder+"/doors.bin",folder+"/scene.bin",actual,error));Npcs npcs;assert(npcs.loadPopulation(folder+"/population.bin",folder+"/scene.bin",actual,error));assert(npcs.loadNames(folder+"/npc_names.bin",folder+"/scene.bin",folder+"/population.bin",error));npcs.reset(actual,{16,91,31});assert(npcs.all().size()==70&&npcs.all().back().name=="El Devorador");assert(canStand(actual,{16,91,31})&&canStand(actual,{15,91,42}));assert(sewerPortal(actual,{15,91,42},1));Inventory chests;chests.init(actual,root+"actual.sav");assert(chests.chests.size()==2);chests.copyPlayerFrom(bag);chests.chests[0].items.clear();assert(chests.checkpoint());Inventory loaded;loaded.init(actual,root+"actual.sav");assert(loaded.healthy()&&loaded.restored&&loaded.quests[2].state==2&&loaded.chests.size()==2&&Quests::owned(loaded,28)==2&&loaded.chests[0].items.empty());// With doors opened through normal interactions, the boss, both chests,
 // sufficient mission enemies and the exit must share a walking component.
 for(auto&o:actual.objects)if(o.flags&Door){o.flags=0;o.height=0;}actual.index();
 std::queue<Actor>pending;pending.push({16,91,31});std::set<std::tuple<int,int,int>>seen;
 while(!pending.empty()){auto a=pending.front();pending.pop();if(!seen.emplace(a.x,a.y,a.z).second)continue;for(int d=0;d<8;++d){Actor n;int direction;if(step(actual,a,d,n,direction)&&!seen.count({n.x,n.y,n.z}))pending.push(n);}}
 const auto&leader=npcs.all().back();assert(seen.count({leader.position.x,leader.position.y,leader.position.z})&&seen.count({15,91,42}));
 unsigned hostile=0;for(const auto&n:npcs.all())if(n.aggressive&&seen.count({n.position.x,n.position.y,n.position.z}))++hostile;assert(hostile>=5);
 for(size_t i=0;i<chests.chests.size();++i){bool accessible=false;for(auto [x,y,z]:seen)if(chests.reachable(actual,{x,y,z},i)){accessible=true;break;}assert(accessible);}
 std::cout<<"Private dungeon: 70 creatures, 2 lootable chests, boss/exit paths and "<<hostile<<" reachable hostiles validated\n";}
 std::cout<<"Journey: atomic zone authority, failed commit, corrupt save, personal transfer without ground/chests, sewer mission and one-time reward passed\n";
}
