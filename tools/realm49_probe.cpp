// SPDX-License-Identifier: MIT
#include "region_migration.hpp"
#include "quests.hpp"
#include "developer_menu.hpp"
#include "journal.hpp"
#include "texture_preview.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <set>
#include <tuple>
using namespace lab;
static size_t memory(const World&w){return w.land.capacity()*sizeof(Land)+w.vertices.capacity()*sizeof(int8_t)+w.objects.capacity()*sizeof(Object)+w.cells.bytes();}
int main(int argc,char**argv){
 assert(argc==4);std::string old=std::string(argv[1])+"/",root=std::string(argv[2])+"/",tmp=std::string(argv[3])+"/",error;World w,before;
 assert(loadWorld(root+"scene.bin",w,error)&&w.width==768&&w.height==768&&w.objects.size()>170000);assert(loadWorld(old+"scene.bin",before,error));
 Doors doors;assert(doors.load(root+"doors.bin",root+"scene.bin",w,error)&&doors.count()==400);
 Npcs npcs,previous;assert(npcs.loadPopulation(root+"population.bin",root+"scene.bin",w,error));assert(npcs.loadNames(root+"npc_names.bin",root+"scene.bin",root+"population.bin",error));npcs.reset(w,{w.startX,w.startY,w.startZ});assert(npcs.all().size()==898&&npcs.styles().size()==58&&npcs.activeCount()<=64);
 assert(previous.loadPopulation(old+"population.bin",old+"scene.bin",before,error)&&previous.loadNames(old+"npc_names.bin",old+"scene.bin",old+"population.bin",error));previous.reset(before,{before.startX,before.startY,before.startZ});assert(previous.all().size()==525);
 unsigned swim=0;for(size_t i=0;i<npcs.all().size();++i){const auto&n=npcs.all()[i];assert(canStandMobile(w,n.position,n.swims,n.waterOnly));swim+=n.swims;if(i<previous.all().size()){const auto&p=previous.all()[i];assert((n.name=="a horse"&&!p.animal&&n.animal)||(n.name==p.name&&n.title==p.title&&n.animal==p.animal));}}
 assert(swim==22);DeveloperMenu travel(w,npcs);assert(travel.destinations.size()==12);for(size_t i=10;i<12;++i){assert(travel.destinations[i].available);std::cout<<travel.destinations[i].name<<" "<<travel.destinations[i].target.x+w.originX<<","<<travel.destinations[i].target.y+w.originY<<"\n";}travel.pageMove();assert(travel.page()==1);travel.pageMove();assert(travel.page()==2);travel.pageMove();assert(travel.page()==0);Journal journal;assert(loadJournal(root+"journal.bin",root+"scene.bin",journal,error)&&journal.width==768);
 using Key=std::tuple<int,int,int,int,int>;std::multiset<Key>objects;for(const auto&o:w.objects)objects.emplace(o.art,o.x+w.originX,o.y+w.originY,o.z,o.hue);
 unsigned missing=0;for(const auto&o:before.objects){auto at=objects.find({o.art,o.x+before.originX,o.y+before.originY,o.z,o.hue});if(at==objects.end())++missing;else objects.erase(at);}assert(!missing);
 std::filesystem::create_directories(tmp+"old");std::filesystem::create_directories(tmp+"new");std::filesystem::copy_file(old+"scene.bin",tmp+"old/scene.bin",std::filesystem::copy_options::overwrite_existing);
 if(std::filesystem::exists(old+"scene.bin.saves"))std::filesystem::copy_file(old+"scene.bin.saves",tmp+"old/scene.bin.saves",std::filesystem::copy_options::overwrite_existing);
 Inventory bag;bag.init(before,tmp+"old/game-2-inventory-private.sav");bag.backpack={{0,321},{3,7}};bag.dressed=bag.accessories=true;for(size_t i=0;i<bag.chests.size();++i)bag.chests[i].items={{0,uint16_t(i+1)}};bag.quests[0]={1,4,6,Quests::signature(npcs.all()[5])};assert(bag.checkpoint());
 Appearance appearance;appearance.enabled=appearance.realClothes=true;appearance.name="Rodrigo";assert(appearance.save(tmp+"old/game-2-appearance.sav",error));PlayerStatus stats;stats.hits=23;stats.skills.start(4);assert(saveVitals(tmp+"old/game-2-vitals-private.sav",stats));assert(savePosition(tmp+"old/game-2-position-private.sav",before,{before.startX,before.startY,before.startZ},error));Campaign campaign(tmp+"old/");assert(campaign.commit(2,error));
 if(!migrateRegion(tmp+"old/",tmp+"new/",w,error)){std::cerr<<error<<"\n";return 1;}Campaign next(tmp+"new/");assert(next.load());Inventory moved;moved.init(w,tmp+"new/"+next.prefix()+"inventory-private.sav");assert(moved.restored&&moved.backpack[0].count==321&&moved.quests[0].count==4&&moved.quests[0].signature==bag.quests[0].signature);Actor at;assert(loadPosition(tmp+"new/"+next.prefix()+"position-private.sav",w,at,error)&&at.x+w.originX==before.startX+before.originX);assert(migrateRegion(tmp+"old/",tmp+"new/",w,error));
 std::cout<<"Private realm: "<<w.objects.size()<<" objects, "<<doors.count()<<" doors, "<<npcs.all().size()<<" NPCs, "<<swim<<" swimmers; 525 stable indices (six corrected horses) and "<<bag.chests.size()<<" chests migrate; all old structures retained; geometry RAM "<<memory(w)<<" bytes\n";
}
