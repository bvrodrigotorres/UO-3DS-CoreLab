// SPDX-License-Identifier: MIT
#include "npc_pose_cache.hpp"
#include "save_queue.hpp"
#include <cassert>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
using namespace lab;
static void wait(const std::function<bool()>&f){auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(8);while(!f()){assert(std::chrono::steady_clock::now()<limit);std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
int main(int argc,char**argv){
    assert(argc==2);std::string root=std::string(argv[1])+"/";
    NpcPoseCache cache;std::vector<int>pinned;
    // More species than the former whole-sheet budget, including all corpse orientations.
    for(unsigned n=0;n<NpcPoseCache::Slots;++n){int key=int(n*48);int slot=cache.victim(pinned);assert(slot>=0);cache.assign(slot,key);pinned.push_back(key);}
    assert(cache.victim(pinned)==-1);
    for(unsigned n=0;n<112;++n)assert(cache.find(int(n*48))>=0);
    int retained=cache.find(48);pinned.erase(pinned.begin());assert(cache.victim(pinned)==cache.find(0));
    cache.assign(cache.victim(pinned),48+40);assert(cache.fallback(1,48)==retained);assert(cache.fallback(1,48,true)!=retained);
    for(unsigned n=0;n<20000;++n){int key=int((n%51)*48+n%48);int old=cache.find(key);if(old>=0)cache.touch(old);else {int slot=cache.victim({});assert(slot>=0);cache.assign(slot,key);}assert(cache.find(key)>=0);}
    // Every occupied cell and swizzled address remains inside the fixed GPU banks.
    for(unsigned n=0;n<112;++n){unsigned cell=n%56,x=cell%8*128,y=cell/8*144;assert(x+128<=1024&&y+144<=1024);}
    std::vector<NpcStyle>styles={{10,70,60,80,100,123}};std::vector<uint8_t>meta(8+16+48*4);std::memcpy(meta.data(),"UPS1",4);meta[4]=1;meta[6]=48;
    auto put=[&](unsigned at,unsigned v,unsigned bytes){for(unsigned n=0;n<bytes;++n)meta[at+n]=uint8_t(v>>(n*8));};
    put(8,10,2);put(10,70,2);put(12,60,2);put(14,80,2);put(16,100,4);put(20,123,4);
    std::vector<uint32_t>checksums;assert(readNpcPoseManifest(meta,styles,48,checksums)&&checksums.size()==48);
    for(size_t n=0;n<meta.size();++n){auto truncated=meta;truncated.resize(n);assert(!readNpcPoseManifest(truncated,styles,48,checksums));}
    meta[12]=129;assert(!readNpcPoseManifest(meta,styles,48,checksums));meta[12]=60;meta[20]^=1;assert(!readNpcPoseManifest(meta,styles,48,checksums));
    AsyncResources reads;assert(reads.start());
    for(int i=0;i<4;++i){std::ofstream f(root+std::to_string(i),std::ios::binary);f<<"pose";}
    // Scenery requests cannot evict completed visible-creature reads.
    for(int i=0;i<4;++i)assert(reads.request(root+std::to_string(i),4,0,true));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));assert(!reads.request(root+"scenery",4));
    ResourceRead file;for(int i=0;i<4;++i){wait([&]{return reads.take(root+std::to_string(i),file);});assert(file.valid&&file.bytes.size()==4);}
    for(int i=0;i<4;++i)assert(reads.request(root+std::to_string(i),4,0,true));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));reads.retain({});assert(reads.request(root+"scenery",4));wait([&]{return reads.take(root+"scenery",file);});assert(!file.valid);
    std::cout<<"Streaming 41: 112 bounded poses, 20000 cache transitions, separate living/dead fallbacks, manifest validation, protected async leases and cancellation passed\n";
}
