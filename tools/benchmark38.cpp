// SPDX-License-Identifier: MIT
// Desktop comparative harness; timings are not Old3DS FPS measurements.
#include "npcs.hpp"
#include "inventory.hpp"
#ifdef CORELAB38
#include "save_queue.hpp"
#endif
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <new>
#include <iostream>
using namespace lab;
static std::atomic<size_t>allocations{0};
void*operator new(size_t n){++allocations;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void*p)noexcept{std::free(p);}
void operator delete(void*p,size_t)noexcept{std::free(p);}
using Clock=std::chrono::steady_clock;
static double elapsed(Clock::time_point start){return std::chrono::duration<double,std::milli>(Clock::now()-start).count();}
template<class F>void measure(const char*name,int runs,F f){size_t before=allocations;auto start=Clock::now();for(int n=0;n<runs;++n)f(n);double ms=elapsed(start);std::cout<<name<<",runs="<<runs<<",ms="<<ms<<",allocations="<<allocations-before<<"\n";}
#ifndef CORELAB38
static int slowRename(const char*a,const char*b){std::this_thread::sleep_for(std::chrono::milliseconds(20));return std::rename(a,b);}
#endif
int main(int argc,char**argv){
 World w;w.width=w.height=128;w.land.resize(16384);w.vertices.resize(16641);for(int y=45;y<83;++y)for(int x=45;x<83;++x){Object o;o.x=x;o.y=y;o.z=0;o.flags=Surface;o.order=w.objects.size();w.objects.push_back(o);o.z=20;o.flags=Wall|Impassable;o.height=20;o.order=w.objects.size();if((x+y)%7==0)w.objects.push_back(o);}w.index();Actor a{64,64,0};size_t checksum=0;
 for(int i=0;i<32;++i){Actor next;int d;step(w,a,i%8,next,d);}
 measure("geometry",20000,[&](int n){Actor next;int d;if(step(w,a,n%8,next,d))checksum+=next.x;});
 measure("render_lists",500,[&](int n){auto list=renderList(w,{62+n%5,62+(n/5)%5,0},true);checksum+=list.size();});
 Npcs npcs;auto&people=npcs.combatants();for(int y=52;y<75&&people.size()<525;++y)for(int x=52;x<75&&people.size()<525;++x)if(x!=64||y!=64){Npc n;n.position=n.from=n.home={x,y,0};n.animal=true;n.roam=4;n.wait=0;if(people.size()%11==0){n.aggressive=true;n.maximumHits=50;n.meleeMax=4;}people.push_back(n);}Doors doors;
 measure("crowd_525",1500,[&](int){npcs.update(w,a,a,false,.05f,doors);});
 auto state=[](const Npcs&population){uint32_t h=2166136261u;for(const auto&n:population.all())for(int v:{n.position.x,n.position.y,n.position.z,n.from.x,n.from.y,int(n.moving),int(n.hostile),n.direction,int(n.progress*10000),int(n.wait*1000)})h=(h^uint32_t(v))*16777619u;return h;};std::cout<<"crowd_state="<<state(npcs)<<"\n";
 if(argc>2){World real;std::string error;if(!loadWorld(std::string(argv[2])+"/scene.bin",real,error)){std::cerr<<error;return 1;}Npcs original;if(!original.loadPopulation(std::string(argv[2])+"/population.bin",std::string(argv[2])+"/scene.bin",real,error)){std::cerr<<error;return 1;}Actor at{real.startX,real.startY,real.startZ};original.reset(real,at);for(auto&n:original.all())if(n.aggressive){at=n.home;break;}
 measure("britain_512_npc",1000,[&](int){original.update(real,at,at,false,.05f,doors);});measure("britain_512_draw",200,[&](int){checksum+=renderList(real,at,true).size();});std::cout<<"region_state="<<state(original)<<"\n";std::cout<<"region_objects="<<real.objects.size()<<",npcs="<<original.all().size()<<"\n";}
 if(argc>1){Inventory inv;inv.init(w,std::string(argv[1])+"/inventory.sav");inv.backpack.clear();Object chest;chest.x=64;chest.y=63;chest.art=0xE40;w.objects.push_back(chest);w.index();inv.init(w,std::string(argv[1])+"/inventory.sav");inv.backpack.clear();inv.chests[0].items={{1,10}};inv.checkpoint();inv.opened=0;
#ifdef CORELAB38
 AsyncSaves queue([](const std::string&p,const std::vector<uint8_t>&bytes,std::string&e){std::this_thread::sleep_for(std::chrono::milliseconds(40));return atomicSaveBytes(p,bytes,e);});queue.start();inv.saveSink=[&](const std::string&p,std::vector<uint8_t>b){return queue.enqueue(p,std::move(b));};
 measure("loot_10_with_40ms_storage",10,[&](int n){if(!inv.transfer(w,a,n%2==0,0))std::abort();});if(!queue.flush())return 2;std::cout<<"physical_writes="<<queue.status().writes<<"\n";
#else
 measure("loot_10_with_40ms_storage",10,[&](int n){if(!inv.transfer(w,a,n%2==0,0,slowRename))std::abort();});std::cout<<"physical_writes=10\n";
#endif
 }
 std::cout<<"checksum="<<checksum<<"\n";
}
