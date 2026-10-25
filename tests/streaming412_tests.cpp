// SPDX-License-Identifier: MIT
#include "npc_pose_cache.hpp"
#include "save_queue.hpp"
#include <cassert>
#include <thread>
#include <chrono>
#include <fstream>
#include <iostream>
using namespace lab;
static void wait(const std::function<bool()>&f){auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(8);while(!f()){assert(std::chrono::steady_clock::now()<limit);std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
int main(int argc,char**argv){
    assert(argc==2);const std::string root=std::string(argv[1])+"/";
    // Many more pages than queue slots; a slow request survives each frame's
    // full visibility scan, and all needed pages finish while standing still.
    WorldPageRead world;std::vector<unsigned char>needed(64,1),resident(64);std::vector<uint64_t>retry(64);
    int remaining=0;unsigned uploads=0;
    for(uint64_t tick=0;tick<600;++tick){int page=world.choose(needed,resident,retry,tick);if(page<0)continue;if(world.pending<0){world.pending=page;remaining=5;}
        assert(world.choose(needed,resident,retry,tick)==page);
        if(--remaining==0){resident[page]=1;world.pending=-1;++uploads;}
    }
    assert(uploads==64&&world.choose(needed,resident,retry,600)==-1);
    resident.assign(64,0);retry[0]=1000;assert(world.choose(needed,resident,retry,700)==1);world.pending=5;needed[5]=0;assert(world.choose(needed,resident,retry,700)==5);world.pending=-1;
    // Movement frame changes every tick, reads complete seven ticks later.
    // Completed old phases are still admitted, and the four walking poses stay
    // available even after a slow storage cycle has finished.
    NpcPoseCache cache;std::vector<int>pending;int elapsed=0;unsigned shown[4]={};
    for(unsigned tick=0;tick<200;++tick){unsigned frame=1+tick%4;std::vector<NpcPoseNeed>demands;addNpcPoseCycle(demands,3,frame,48,1);orderNpcPoseNeeds(demands);
        std::vector<int>pinned;for(const auto&d:demands)pinned.push_back(d.key);assert(pinned.size()==5);
        if(++elapsed==7){for(int key:pending){assert(std::find(pinned.begin(),pinned.end(),key)!=pinned.end());int slot=cache.victim(pinned);assert(slot>=0);cache.assign(slot,key);}pending.clear();elapsed=0;}
        for(const auto&d:demands)if(pending.size()<4&&cache.find(d.key)<0&&std::find(pending.begin(),pending.end(),d.key)==pending.end())pending.push_back(d.key);
        if(tick>30){assert(cache.find(3*48+frame)>=0);++shown[frame-1];}
    }
    for(unsigned count:shown)assert(count>30);
    std::vector<NpcPoseNeed>death;addNpcPoseCycle(death,2,47,48,5);assert(death.size()==1&&death[0].key==2*48+47);
    std::vector<NpcPoseNeed>directions;for(unsigned dir=0;dir<8;++dir)addNpcPoseCycle(directions,1,dir*5+2,48,4);assert(directions.size()==40);addNpcPoseCycle(directions,1,2,48,0);orderNpcPoseNeeds(directions);assert(directions.front().key==50&&directions.front().distance==0);
    // Independent actual async queues: even four protected unread NPC files
    // cannot block a terrain request, nor can terrain traffic evict animation.
    for(int i=0;i<5;++i){std::ofstream f(root+std::to_string(i));f<<"pose";}
    AsyncResources scenery,poses;assert(scenery.start()&&poses.start());
    for(int i=0;i<4;++i)assert(poses.request(root+std::to_string(i),4,0,true));
    assert(scenery.request(root+"4",4,0,true));ResourceRead file;wait([&]{return scenery.take(root+"4",file);});assert(file.valid);
    for(int i=0;i<4;++i){wait([&]{return poses.take(root+std::to_string(i),file);});assert(file.valid);}
    std::cout<<"Streaming 41.2: all 64 terrain pages converge under delayed I/O; full walking cycles survive changing frames; independent bounded queues make progress together\n";
}
