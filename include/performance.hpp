// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include "inventory.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace lab {
inline bool equipmentTouchWhileMoving(bool equipMode,int x,int y){
    return InventoryLayout::mode(x,y)||InventoryLayout::page(x,y)||(equipMode&&InventoryLayout::slot(x,y,InventoryLayout::Left)>=0);
}
class FramePacer {
    uint64_t next=0;
public:
    void reset(){next=0;}
    uint64_t delay(uint64_t microseconds){if(!next||microseconds>next+100000)next=microseconds;uint64_t wait=next>microseconds?next-microseconds:0;next+=33333;return wait;}
};
class FrameStats {
    std::array<float,120>samples{};unsigned at=0,count=0;
public:
    void observe(float ms){if(ms<=0)return;samples[at++%samples.size()]=ms;count=std::min<unsigned>(count+1,samples.size());}
    float p95()const{if(!count)return 0;auto sorted=samples;std::sort(sorted.begin(),sorted.begin()+count);return sorted[(count-1)*95/100];}
};
class PositionCheckpoint {
    Actor saved{};bool known=false;
    uint64_t attempted=0,activity=0;
public:
    void baseline(const Actor&a,uint64_t now){saved=a;known=true;attempted=activity=now;}
    void suspend(uint64_t elapsed){attempted+=elapsed;activity+=elapsed;}
    void active(uint64_t now){activity=now;}
    bool dirty(const Actor&a)const{return !known||a.x!=saved.x||a.y!=saved.y||a.z!=saved.z;}
    bool due(const Actor&a,uint64_t now)const{return dirty(a)&&now-attempted>=60000&&now-activity>=2000;}
    void result(const Actor&a,uint64_t now,bool ok){attempted=now;if(ok){saved=a;known=true;}}
};
// GPU-independent residency policy; texture ownership stays in the renderer.
template<class T>class EquipmentCache {
    struct Entry {T value{};size_t bytes=0;uint64_t used=0;unsigned key=0;};
    std::array<Entry,8>entries{};uint64_t clock=0;
public:
    T get(unsigned key){for(auto&e:entries)if(e.value&&e.key==key){e.used=++clock;return e.value;}return T{};}
    void insert(unsigned key,T value,size_t bytes){for(auto&e:entries)if(!e.value||e.key==key){e={value,bytes,++clock,key};return;}}
    int victim(unsigned protectedKey,unsigned secondProtected=~0u)const{const Entry*best=nullptr;for(const auto&e:entries)if(e.key!=protectedKey&&e.key!=secondProtected&&e.value&&(!best||e.used<best->used))best=&e;return best?int(best->key):-1;}
    T remove(unsigned key){for(auto&e:entries)if(e.value&&e.key==key){auto value=e.value;e={};return value;}return T{};}
    bool full()const{for(const auto&e:entries)if(!e.value)return false;return true;}
    size_t bytes()const{size_t n=0;for(const auto&e:entries)n+=e.bytes;return n;}
};
}
