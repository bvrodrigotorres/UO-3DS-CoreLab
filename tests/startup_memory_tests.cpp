// SPDX-License-Identifier: MIT
#include "core.hpp"
#include <cassert>
#include <cstdlib>
#include <new>
#include <fstream>
#include <iostream>
#include <array>
#include <cstring>
namespace {
size_t live=0,peak=0,limit=~size_t(0);
struct alignas(std::max_align_t) Allocation {size_t bytes;};
void* allocate(size_t n){
    n=std::max(n,size_t(1));
    if(n>limit-live)throw std::bad_alloc();
    auto*p=static_cast<Allocation*>(std::malloc(sizeof(Allocation)+n));
    if(!p)throw std::bad_alloc();
    p->bytes=n;live+=n;peak=std::max(peak,live);return p+1;
}
void release(void*p){if(p){auto*a=static_cast<Allocation*>(p)-1;live-=a->bytes;std::free(a);}}
void fixture(const std::string&path,unsigned width,unsigned objects){
    std::ofstream f(path,std::ios::binary);uint32_t hash=2166136261u;
    auto put=[&](uint64_t v,unsigned n){for(unsigned i=0;i<n;++i){unsigned char c=v>>(i*8);f.put(char(c));hash=(hash^c)*16777619u;}};
    put(0x314c4355,4);put(1,2);put(width,2);put(width,2);put(1,2);put(objects,4);
    put(1088,4);put(1216,4);put(10,2);put(10,2);put(0,2);put(0,2);
    for(unsigned i=0;i<(width+1)*(width+1);++i)put(0,1);
    for(unsigned i=0;i<width*width;++i){put(3,2);put(lab::NoImage,2);put(0,8);put(0,1);}
    for(unsigned i=0;i<objects;++i){
        put(i%width,2);put(i/width%width,2);put(uint8_t(-7),1);put(255,1);
        put(321,2);put(0xABC,2);put(lab::NoImage,2);put(0xFEDCBA9876543210ull,8);put(i,4);put(0,2);
    }
    f.close();std::ofstream side(path+".saves",std::ios::binary);
    auto word=[&](uint32_t v){for(unsigned i=0;i<4;++i)side.put(char(v>>(i*8)));};
    word(0x31435755);word(hash);word(123);word(456);
}
}
void* operator new(size_t n){return allocate(n);}void* operator new[](size_t n){return allocate(n);}
void operator delete(void*p)noexcept{release(p);}void operator delete[](void*p)noexcept{release(p);}
void operator delete(void*p,size_t)noexcept{release(p);}void operator delete[](void*p,size_t)noexcept{release(p);}
int main(int argc,char**argv){
    assert(argc==2);std::string root=std::string(argv[1])+"/",error;
    fixture(root+"realm.bin",768,171867);fixture(root+"previous.bin",768,171867);
    limit=30*1024*1024;peak=live;
    // Reserve 5 MiB for SDK, HUD, renderer metadata and other startup residents.
    std::vector<unsigned char>sdk(5*1024*1024);
    try{
        lab::World realm,previous;
        assert(lab::loadWorld(root+"realm.bin",realm,error));
        assert(realm.compatiblePosition==123&&realm.compatibleInventory==456);
        assert(realm.objects.back().order==171866&&realm.objects.back().z==-7&&realm.objects.back().height==255&&realm.objects.back().flags==0xFEDCBA9876543210ull);
        assert(lab::loadWorld(root+"previous.bin",previous,error,false));
        assert(previous.width==768&&realm.width==768&&previous.cells.empty()&&previous.land.empty()&&previous.vertices.empty());
        // Corrupt a sidecar hash; failed loading must preserve the caller's world.
        previous={};std::fstream side(root+"previous.bin.saves",std::ios::binary|std::ios::in|std::ios::out);side.seekp(4);side.put(0);side.close();
        assert(!lab::loadWorld(root+"previous.bin",previous,error,false)&&previous.width==0);
        std::cout<<"Startup memory: 768 region + previous 768 region + 5 MiB SDK reserve; peak "<<peak<<" / "<<limit<<" bytes; streamed sidecar hash and record boundaries OK\n";
    }catch(const std::bad_alloc&){std::cerr<<"Startup exceeds 30 MiB budget; peak before failure "<<peak<<" bytes\n";return 1;}
    return 0;
}
