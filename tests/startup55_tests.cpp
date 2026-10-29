// SPDX-License-Identifier: MIT
#include "startup_probe.hpp"
#include "file_hash.hpp"
#include "doors.hpp"
#include "journal.hpp"
#include "npcs.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
using namespace lab;
using Bytes=std::vector<uint8_t>;
static void put(Bytes&b,size_t at,uint64_t n,unsigned size=4){for(unsigned i=0;i<size;++i)b[at+i]=uint8_t(n>>(8*i));}
static uint32_t hash(const Bytes&b,size_t start=0){uint32_t h=2166136261u;for(size_t i=start;i<b.size();++i)h=(h^b[i])*16777619u;return h;}
static void write(const std::string&p,const Bytes&b){std::ofstream f(p,std::ios::binary);f.write((const char*)b.data(),b.size());assert(f.good());}
static Bytes read(const std::string&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static void dir(const std::string&p){assert(::mkdir(p.c_str(),0700)==0);}
int main(int argc,char**argv){
 assert(argc==2);std::string root=std::string(argv[1])+"/startup55/",error;dir(root);dir(root+"britain-complete");dir(root+"britain-expanded");dir(root+"britain-sewers");auto town=root+"britain-complete/",old=root+"britain-expanded/",sewers=root+"britain-sewers/";
 write(town+"ready.bin",{1});write(town+"scene.bin",{1,2,3});write(old+"scene.bin",{1,2,3});write(sewers+"ready.bin",{1});
 StartupProbe empty(root);assert(empty.worldRoot==town&&empty.saveRoot==town&&!empty.canContinue&&!empty.directSewers(MenuAction::Continue));
 Campaign prior(old);assert(prior.commit(7,error));write(old+prior.prefix()+"inventory-private.sav",{1,2,3});GameOptions options;options.zoom=4;assert(options.save(old+"options.sav",error));
 StartupProbe migration(root);assert(migration.worldRoot==town&&migration.saveRoot==old&&migration.canContinue&&!migration.authoritative&&migration.options.zoom==4);auto oldBytes=read(old+"campaign.sav");
 StartMenu menu;menu.selected=0;assert(menu.activate(true)==MenuAction::None&&menu.page==MenuPage::Confirm);menu.back();assert(read(old+"campaign.sav")==oldBytes); // cancelling never starts import or replaces a campaign
 options.zoom=3;assert(options.save(town+"options.sav",error));StartupProbe preferences(root);assert(preferences.saveRoot==old&&preferences.optionsRoot==town&&preferences.options.zoom==3);
 Campaign active(town);assert(active.commit(12,error));auto prefix=active.prefix();write(town+prefix+"inventory-sewers.sav",{1,2,3});Journey trip;trip.zone=1;assert(trip.save(town+prefix+"journey.sav"));
 StartupProbe direct(root);assert(direct.saveRoot==town&&direct.canContinue&&direct.authoritative&&direct.directSewers(MenuAction::Continue)&&!direct.directSewers(MenuAction::NewGame));
 assert(std::rename((town+"campaign.sav").c_str(),(town+"campaign.sav.bak").c_str())==0);StartupProbe backup(root);assert(backup.canContinue&&backup.directSewers(MenuAction::Continue)&&backup.prefix==prefix);
 write(town+prefix+"journey.sav",{0});StartupProbe corrupt(root);assert(!corrupt.canContinue&&!corrupt.journeyValid&&!corrupt.directSewers(MenuAction::Continue));assert(trip.save(town+prefix+"journey.sav"));
 std::remove((sewers+"ready.bin").c_str());StartupProbe missing(root);assert(!missing.canContinue&&!missing.directSewers(MenuAction::Continue));write(sewers+"ready.bin",{1});
 // Minimal decoded map + companion assets, with real byte checksums.
 std::string scene=town+"scene.bin";Bytes world(32+9+4*13);std::memcpy(world.data(),"UCL1",4);put(world,4,1,2);put(world,6,2,2);put(world,8,2,2);put(world,10,1,2);put(world,16,1488);put(world,20,1596);
 for(unsigned i=0;i<4;++i){put(world,41+i*13,3,2);put(world,43+i*13,NoImage,2);}write(scene,world);World w;assert(loadWorld(scene,w,error)&&w.sceneChecksum==hash(world)&&w.sceneBytes==world.size()&&w.sceneSource==scene);
 uint32_t check=0;assert(fileHash(scene,1000,check)&&check==w.sceneChecksum);assert(!sceneFileHash(w,scene,1,check)&&check==w.sceneChecksum);
 Bytes doors(16);std::memcpy(doors.data(),"UOD1",4);put(doors,4,1,2);put(doors,12,check);write(town+"doors.bin",doors);
 Bytes journal(44);std::memcpy(journal.data(),"UOJ1",4);put(journal,4,2,2);put(journal,6,2,2);put(journal,8,1488);put(journal,12,1596);put(journal,16,check);put(journal,20,hash(journal,28));put(journal,24,1,2);put(journal,26,1,2);write(town+"journal.bin",journal);
 Bytes pop(16+16+44);std::memcpy(pop.data(),"UNP1",4);put(pop,4,1,2);put(pop,6,1,2);put(pop,8,check);put(pop,20,32,2);put(pop,22,32,2);put(pop,24,16);std::memcpy(pop.data()+44,"healer",6);put(pop,12,hash(pop,16));write(town+"population.bin",pop);
 Bytes names(20+85);std::memcpy(names.data(),"UNL1",4);put(names,4,1,2);put(names,8,check);put(names,12,hash(pop));std::memcpy(names.data()+20,"Anastasia",9);std::memcpy(names.data()+52,"the healer",10);put(names,100,0x35,2);names[102]=names[103]=255;put(names,16,hash(names,20));write(town+"npc_names.bin",names);
 // Once decoded, companion validation needs zero additional map payload reads.
 assert(std::rename(scene.c_str(),(scene+".hidden").c_str())==0);Doors d;Journal j;Npcs n;
 assert(d.load(town+"doors.bin",scene,w,error));assert(loadJournal(town+"journal.bin",scene,j,error,&w));assert(n.loadPopulation(town+"population.bin",scene,w,error));assert(n.loadNames(town+"npc_names.bin",scene,town+"population.bin",error));n.reset(w,{1,1,0});assert(n.all().size()==1&&n.all()[0].name=="Anastasia");
 World unknown;assert(!sceneFileHash(unknown,scene,1000,check));assert(!sceneFileHash(w,scene+"other",1000,check));
 auto invalid=doors;invalid[12]^=1;write(town+"doors.bin",invalid);assert(!d.load(town+"doors.bin",scene,w,error));write(town+"doors.bin",doors);
 invalid=journal;invalid[16]^=1;write(town+"journal.bin",invalid);assert(!loadJournal(town+"journal.bin",scene,j,error,&w));write(town+"journal.bin",journal);
 invalid=pop;invalid[8]^=1;write(town+"population.bin",invalid);Npcs bad;assert(!bad.loadPopulation(town+"population.bin",scene,w,error));write(town+"population.bin",pop);
 invalid=names;invalid[20]^=1;write(town+"npc_names.bin",invalid);assert(!n.loadNames(town+"npc_names.bin",scene,town+"population.bin",error));
 // A later scene load always computes a fresh identity; no cross-zone/disk cache.
 world[41]=4;write(scene,world);World changed;assert(loadWorld(scene,changed,error)&&changed.sceneChecksum!=w.sceneChecksum);assert(!d.load(town+"doors.bin",scene,changed,error)&&!bad.loadPopulation(town+"population.bin",scene,changed,error));
 std::cout<<"Startup55: metadata-only menu probe, migration hints, cancellation, direct sewer plan, backup/corrupt journey; one-pass scene identity, zero rereads, all companion corruption and fresh-load rejection passed\n";
}
