// SPDX-License-Identifier: MIT
#include "region_migration.hpp"
#include "combat.hpp"
#include "journal.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
using namespace lab;
namespace {
void put(std::vector<uint8_t>&b,uint64_t n,int bytes){while(bytes--){b.push_back(n&255);n>>=8;}}
uint32_t hash(const std::vector<uint8_t>&b,size_t start=0){uint32_t h=2166136261;for(size_t i=start;i<b.size();++i)h=(h^b[i])*16777619;return h;}
void write(const std::string&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write((const char*)b.data(),b.size());}
void scene(const std::string&p,const World&w){std::vector<uint8_t>b{'U','C','L','1'};put(b,1,2);put(b,w.width,2);put(b,w.height,2);put(b,0,2);put(b,w.objects.size(),4);put(b,w.originX,4);put(b,w.originY,4);put(b,w.startX,2);put(b,w.startY,2);put(b,w.startZ,2);put(b,0,2);
    for(int z:w.vertices)put(b,z,1);
    for(auto l:w.land){put(b,l.art,2);put(b,65535,2);put(b,l.flags,8);put(b,0,1);}
    for(auto o:w.objects){put(b,o.x,2);put(b,o.y,2);put(b,o.z,1);put(b,o.height,1);put(b,o.art,2);put(b,o.hue,2);put(b,65535,2);put(b,o.flags,8);put(b,o.order,4);put(b,0,2);}write(p,b);
}
std::vector<uint8_t>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(int argc,char**argv){
    assert(argc>=2);std::string root=std::string(argv[1])+"/",oldRoot=root+"old/",newRoot=root+"new/",error;mkdir(oldRoot.c_str(),0700);mkdir(newRoot.c_str(),0700);
    World old;old.width=old.height=8;old.originX=1408;old.originY=1536;old.startX=old.startY=2;old.vertices.assign(81,0);old.land.resize(64);Object chest;chest.art=0xe42;chest.x=chest.y=2;old.objects={chest};old.vertices[0]=-7;old.land[0].flags=0xFEDCBA9876543210ull;Object marker;marker.x=marker.y=0;marker.art=321;marker.z=-7;marker.height=255;marker.flags=0xFEDCBA9876543210ull;old.objects.insert(old.objects.begin(),marker);old.index();scene(oldRoot+"scene.bin",old);
    World summary;assert(loadWorld(oldRoot+"scene.bin",summary,error,false));
    assert(summary.land.empty()&&summary.vertices.empty()&&summary.cells.empty()&&summary.objects.size()==1);
    assert(summary.compatiblePosition==positionSnapshot(old).compatiblePosition&&summary.compatibleInventory==inventoryWorldId(old));
    World large;large.width=large.height=512;large.originX=1216;large.originY=1344;large.startX=large.startY=194;large.vertices.assign(513*513,0);large.land.resize(512*512);chest.x=chest.y=194;large.objects={chest};chest.x=chest.y=400;large.objects.push_back(chest);large.index();scene(newRoot+"scene.bin",large);
    World loaded;assert(loadWorld(newRoot+"scene.bin",loaded,error)&&loaded.width==512);
    auto snapshot=positionSnapshot(large);assert(snapshot.vertices.empty()&&snapshot.objects.empty()&&snapshot.cells.empty()&&snapshot.land.empty());
    assert(savePosition(root+"snapshot.sav",snapshot,{195,195,0},error));Actor position;assert(loadPosition(root+"snapshot.sav",large,position,error)&&position.x==195);
    Inventory previous;previous.init(old,oldRoot+"game-2-inventory-private.sav");previous.backpack={{0,87},{7,1}};previous.equipment[5]=8;previous.lastEquipment[5]=8;previous.chests[0].items={{0,3},{5,1}};previous.dressed=true;previous.ground[0]={{3,3,0},{{3,4}},{170.f}};assert(previous.checkpoint());
    Appearance appearance;appearance.enabled=appearance.realClothes=true;appearance.name="Explorer";appearance.hairColor=14;assert(appearance.save(oldRoot+"game-2-appearance.sav",error));PlayerStatus status;status.hits=23;assert(saveVitals(oldRoot+"game-2-vitals-private.sav",status));assert(savePosition(oldRoot+"game-2-position-private.sav",old,{3,3,0},error));Campaign oldCampaign(oldRoot);assert(oldCampaign.commit(2,error));auto original=read(oldRoot+"game-2-inventory-private.sav");
    assert(migrateRegion(oldRoot,newRoot,large,error));Campaign imported(newRoot);assert(imported.load()&&imported.generation==1);Inventory next;next.init(large,newRoot+imported.prefix()+"inventory-private.sav");assert(next.restored&&next.healthy()&&next.backpack.size()==2&&next.backpack[0].count==87&&next.equipment[5]==8&&next.lastEquipment[5]==8&&next.chests[0].items.size()==2&&next.chests[0].items[0].count==3&&next.chests[1].items.size()>2);
    assert(next.ground[0].position.x==195&&next.ground[0].position.y==195&&next.ground[0].items[0].count==4);
    Appearance resumed;assert(resumed.load(newRoot+imported.prefix()+"appearance.sav")&&resumed.name==appearance.name&&resumed.hairColor==14);PlayerStatus resumedStats;assert(loadVitals(newRoot+imported.prefix()+"vitals-private.sav",resumedStats)&&resumedStats.hits==23);assert(loadPosition(newRoot+imported.prefix()+"position-private.sav",large,position,error)&&position.x==195&&position.y==195);
    next.backpack[0].count=12;assert(next.checkpoint());assert(migrateRegion(oldRoot,newRoot,large,error));Inventory again;again.init(large,newRoot+imported.prefix()+"inventory-private.sav");assert(again.backpack[0].count==12&&read(oldRoot+"game-2-inventory-private.sav")==original);
    assert(!migrateRegion(oldRoot,root+"missing/",large,error)&&read(oldRoot+"game-2-inventory-private.sav")==original);
    // Extended population preserves source combat data and autonomous aggression.
    std::vector<uint8_t>payload;put(payload,30,2);put(payload,70,2);put(payload,60,2);put(payload,80,2);put(payload,100,4);put(payload,1,4);
    put(payload,390,2);put(payload,390,2);put(payload,0,2);put(payload,0,2);put(payload,5,2);put(payload,3,2);std::string name="ORC";for(int i=0;i<32;++i)payload.push_back(i<3?name[i]:0);for(int n:{65,5,7,60})put(payload,n,2);
    std::vector<uint8_t>pop{'U','N','P','2'};put(pop,1,2);put(pop,1,2);put(pop,hash(read(newRoot+"scene.bin")),4);put(pop,hash(payload),4);pop.insert(pop.end(),payload.begin(),payload.end());write(root+"population.bin",pop);
    Npcs npcs;assert(npcs.loadPopulation(root+"population.bin",newRoot+"scene.bin",large,error)&&npcs.hasDeathFrames());npcs.reset(large,{384,390,0});Doors doors;npcs.update(large,{384,390,0},{384,390,0},false,.05f,doors);assert(npcs.all()[0].hostile&&npcs.all()[0].moving&&npcs.all()[0].maximumHits==65&&npcs.all()[0].meleeMax==7);auto&plant=npcs.combatants()[0];plant.stationary=true;plant.moving=false;plant.hostile=false;plant.wait=0;plant.position=plant.from=plant.home={390,390,0};npcs.update(large,{380,390,0},{380,390,0},false,.1f,doors);assert(!plant.moving);
    Combat combat;combat.reset(npcs);assert(combat.vitals()[0].hits==65);
    // Dense hostile pack behind an impassable wall: route work remains bounded
    // after prolonged play, including failed searches and cooldown retries.
    World blockedWorld;blockedWorld.width=blockedWorld.height=32;blockedWorld.vertices.assign(33*33,0);blockedWorld.land.resize(32*32);
    for(int y=0;y<32;++y){Object wall;wall.x=17;wall.y=y;wall.z=0;wall.height=80;wall.flags=Impassable;blockedWorld.objects.push_back(wall);}blockedWorld.index();
    auto&pack=npcs.combatants();pack.clear();for(int i=0;i<103;++i){Npc enemy;enemy.position=enemy.from=enemy.home={19,16,0};enemy.animal=enemy.hostile=true;enemy.wait=0;pack.push_back(enemy);}
    unsigned searches=0;for(int frame=0;frame<1200;++frame){npcs.update(blockedWorld,{15,16,0},{15,16,0},false,1.f/60,doors);assert(npcs.lastRouteSearches()<=1&&npcs.lastRouteNodes()<=64);searches+=npcs.lastRouteSearches();}
    assert(searches>100);for(const auto&enemy:npcs.all())assert(enemy.position.x>17);

    if(argc==3){World actual;assert(loadWorld(std::string(argv[2])+"/scene.bin",actual,error)&&actual.width==512&&actual.objects.size()>110000);Npcs population;assert(population.loadPopulation(std::string(argv[2])+"/population.bin",std::string(argv[2])+"/scene.bin",actual,error)&&population.hasDeathFrames());population.reset(actual,{actual.startX,actual.startY,actual.startZ});assert(population.all().size()>500);assert(population.loadNames(std::string(argv[2])+"/npc_names.bin",std::string(argv[2])+"/scene.bin",std::string(argv[2])+"/population.bin",error));Journal journal;assert(loadJournal(std::string(argv[2])+"/journal.bin",std::string(argv[2])+"/scene.bin",journal,error)&&journal.width==512);std::cout<<"Private map objects "<<actual.objects.size()<<", NPCs "<<population.all().size()<<"\n";}
    std::cout<<"Region 29: 512 map, lightweight identity, safe campaign import, original untouched, retained chests/gear/colors, no reimport and aggressive metadata passed\n";
}
