// SPDX-License-Identifier: MIT
#include "save_queue.hpp"
#include "performance.hpp"
#include "combat.hpp"
#include "hud.hpp"
#include "character.hpp"
#include <atomic>
#include <thread>
#include <chrono>
#include <fstream>
#include <cassert>
#include <iostream>
using namespace lab;
static void wait(const std::function<bool()>&f){auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(!f()){assert(std::chrono::steady_clock::now()<until);std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
static std::vector<uint8_t>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
int main(int argc,char**argv){
 assert(argc==2);std::string root=std::string(argv[1])+"/";
 std::atomic<bool>entered{false},release{false};AsyncSaves queue([&](const std::string&p,const std::vector<uint8_t>&b,std::string&e){entered=true;while(!release)std::this_thread::sleep_for(std::chrono::milliseconds(1));return atomicSaveBytes(p,b,e);});assert(queue.start());assert(queue.enqueue(root+"coalesced.sav",{0}));wait([&]{return entered.load();});
 for(unsigned n=1;n<=100;++n)assert(queue.enqueue(root+"coalesced.sav",{uint8_t(n)}));assert(queue.status().pending==2&&read(root+"coalesced.sav").empty());release=true;assert(queue.flush());assert(read(root+"coalesced.sav")==std::vector<uint8_t>{100}&&queue.status().writes==2&&!queue.status().error);
 // Failure retains the newest queued snapshot, never an older failed snapshot.
 std::atomic<bool>recover{false};AsyncSaves failing([&](const std::string&p,const std::vector<uint8_t>&b,std::string&e){return recover?atomicSaveBytes(p,b,e):false;});assert(failing.start());assert(failing.enqueue(root+"retry.sav",{1}));wait([&]{return failing.status().failures>0;});assert(failing.enqueue(root+"retry.sav",{9}));assert(!failing.flush()&&failing.status().error&&read(root+"retry.sav").empty());recover=true;assert(failing.flush()&&read(root+"retry.sav")==std::vector<uint8_t>{9}&&!failing.status().error);assert(!failing.enqueue(root+"huge.sav",std::vector<uint8_t>(524289)));
 World w;w.width=w.height=32;w.land.resize(1024);w.vertices.resize(1089);Object chest;chest.x=10;chest.y=9;chest.art=0xE40;w.objects.push_back(chest);w.index();Actor player{10,10,0};Inventory inv;inv.init(w,root+"inventory.sav");inv.backpack.clear();inv.chests[0].items={{1,3},{3,5},{4,1}};assert(inv.checkpoint());auto original=read(root+"inventory.sav");
 // Touch operations return while physical storage is held blocked, and conserve items.
 entered=false;release=false;AsyncSaves saves([&](const std::string&p,const std::vector<uint8_t>&b,std::string&e){entered=true;while(!release)std::this_thread::sleep_for(std::chrono::milliseconds(1));return atomicSaveBytes(p,b,e);});assert(saves.start());inv.saveSink=[&](const std::string&p,std::vector<uint8_t>b){return saves.enqueue(p,std::move(b));};inv.opened=0;assert(inv.takeAll(w,player));wait([&]{return entered.load();});assert(inv.backpack.size()==3&&inv.chests[0].items.empty()&&read(root+"inventory.sav")==original);assert(inv.transfer(w,player,false,0));assert(inv.transfer(w,player,true,0));assert(inv.backpack.size()==3&&inv.chests[0].items.empty());release=true;assert(saves.flush());Inventory loaded;loaded.init(w,root+"inventory.sav");assert(loaded.restored&&loaded.backpack.size()==3&&loaded.chests[0].items.empty());
 PlayerStatus status;status.skills.start(5);status.hits=17;assert(saveVitals(root+"vitals.sav",status,std::rename,inv.saveSink)&&saves.flush());PlayerStatus resumed;assert(loadVitals(root+"vitals.sav",resumed)&&resumed.hits==17&&resumed.skills.total()==1000);
 AsyncResources resources;assert(resources.start());assert(resources.request(root+"vitals.sav",512));ResourceRead result;wait([&]{return resources.take(root+"vitals.sav",result);});assert(result.valid&&result.bytes==read(root+"vitals.sav"));assert(resources.request(root+"vitals.sav",512,1234));wait([&]{return resources.take(root+"vitals.sav",result);});assert(!result.valid&&result.bytes.empty());assert(resources.request(root+"missing.bin",128));wait([&]{return resources.take(root+"missing.bin",result);});assert(!result.valid);assert(!resources.request(root+"oversize.bin",5*1024*1024));
 // Walking away from unconsumed reads must not pin all four slots forever.
 for(int i=0;i<20;++i){std::string path=root+"walk-away-"+std::to_string(i)+".bin";wait([&]{return resources.request(path,2*1024*1024);});std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 wait([&]{return resources.request(root+"vitals.sav",512);});wait([&]{return resources.take(root+"vitals.sav",result);});assert(result.valid);
 FramePacer pacer;assert(pacer.delay(1000000)==0&&pacer.delay(1001000)==32333);pacer.reset();assert(pacer.delay(10000000)==0);FrameStats frames;for(int i=0;i<120;++i)frames.observe(i<110?33:100);assert(frames.p95()==100);
 EquipmentCache<int>cache;cache.insert(5,1,20);cache.insert(4101,2,20);cache.insert(8,3,20);assert(cache.victim(5,4101)==8);
 Npcs people;for(int y=2;y<30;y+=2)for(int x=2;x<30;x+=2){Npc n;n.animal=true;n.position=n.from=n.home={x,y,0};n.roam=3;n.wait=0;people.combatants().push_back(n);}Doors doors;for(int n=0;n<200;++n){people.update(w,player,player,false,.05f,doors);assert(people.lastOccupancyBuilds()<=1);for(size_t i=0;i<people.all().size();++i)assert(people.occupied(people.all()[i].position));}
 // Native swizzle is bit-identical to the reference for every pixel of supported atlases.
 for(unsigned width:{128u,256u,1024u})for(unsigned y=0;y<256;++y)for(unsigned x=0;x<width;++x){unsigned n=0;for(unsigned bit=0;bit<3;++bit){n|=((x>>bit)&1)<<(2*bit);n|=((y>>bit)&1)<<(2*bit+1);}assert(characterTexel(x,y,width)==((y/8)*(width/8)+x/8)*64+n);}
 std::vector<uint8_t>fb(320*240*3+32,0xa5);for(int i=0;i<4;++i){inv.savePending=i&1;inv.saveFailed=i&2;drawInventory(fb.data()+16,w,player,inv);}for(int i=0;i<16;++i)assert(fb[i]==0xa5&&fb[fb.size()-1-i]==0xa5);
 std::cout<<"Performance 38: nonblocking/coalesced/durable loot, newest-write retry, atomic resources and skills save, bounded async reads/checksums, pacing, bank protection, incremental collision and exact swizzle passed\n";
}
