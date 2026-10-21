// SPDX-License-Identifier: MIT
#include "performance.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
int main(){
    Actor a{5,6,40};PositionCheckpoint save;save.baseline(a,0);
    for(uint64_t t=0;t<600000;t+=16)assert(!save.due(a,t)); // no unchanged writes
    a.x=6;save.active(60000);assert(!save.due(a,61999));assert(save.due(a,62000));
    save.result(a,62000,true);assert(!save.dirty(a));
    a.x=7;assert(!save.due(a,121999));assert(save.due(a,122000));
    save.result(a,122000,false);assert(save.dirty(a));assert(!save.due(a,123000));
    save.active(182000);assert(!save.due(a,183999));assert(save.due(a,184000));
    save.result(a,184000,true);a.z=41;assert(save.dirty(a));a.z=40;assert(!save.dirty(a));
    PositionCheckpoint other;other.baseline({1,2,3},200000);assert(!other.dirty({1,2,3}));assert(!save.dirty(a));
    // Every cached combination can be selected without asking the loader or
    // discarding resources. With low memory, protect the currently worn atlas.
    EquipmentCache<int>cache;assert(!cache.get(0));assert(cache.victim(0)==-1);
    for(unsigned mask=0;mask<8;++mask)cache.insert(mask,int(mask+1),2*1024*1024);
    assert(cache.bytes()==16*1024*1024);
    for(unsigned repeat=0;repeat<1000;++repeat)for(unsigned mask=0;mask<8;++mask)assert(cache.get(mask)==int(mask+1));
    cache.get(0);assert(cache.victim(1)==2);assert(cache.remove(2)==3);assert(!cache.get(2));
    assert(cache.bytes()==14*1024*1024);assert(cache.get(1)==2);
    cache.insert(2,23,2*1024*1024);assert(cache.get(2)==23);
    assert(cache.victim(0)!=0);assert(!cache.get(8));
    std::cout<<"Performance: unchanged saves, idle debounce, 60s interval, failed-write retry, independent scenes and 8000 cached equipment selections passed\n";
}
