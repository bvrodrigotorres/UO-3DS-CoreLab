// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace lab {
inline bool equipmentTouchWhileMoving(bool equipMode,int x,int y){
    return (y>=163&&y<181&&x>=20&&x<150)||(equipMode&&x>=20&&x<134&&y>=65&&y<161);
}
class PositionCheckpoint {
    Actor saved{};bool known=false;
    uint64_t attempted=0,activity=0;
public:
    void baseline(const Actor&a,uint64_t now){saved=a;known=true;attempted=activity=now;}
    void active(uint64_t now){activity=now;}
    bool dirty(const Actor&a)const{return !known||a.x!=saved.x||a.y!=saved.y||a.z!=saved.z;}
    bool due(const Actor&a,uint64_t now)const{return dirty(a)&&now-attempted>=60000&&now-activity>=2000;}
    void result(const Actor&a,uint64_t now,bool ok){attempted=now;if(ok){saved=a;known=true;}}
};
// GPU-independent residency policy; texture ownership stays in the renderer.
template<class T>class EquipmentCache {
    struct Entry {T value{};size_t bytes=0;uint64_t used=0;};
    std::array<Entry,8>entries{};uint64_t clock=0;
public:
    T get(unsigned mask){if(mask>=8)return T{};auto&e=entries[mask];if(e.value)e.used=++clock;return e.value;}
    void insert(unsigned mask,T value,size_t bytes){entries[mask]={value,bytes,++clock};}
    int victim(unsigned protectedMask)const{
        int best=-1;for(unsigned i=0;i<8;++i)if(i!=protectedMask&&entries[i].value&&(best<0||entries[i].used<entries[best].used))best=int(i);return best;
    }
    T remove(unsigned mask){auto value=entries[mask].value;entries[mask]={};return value;}
    size_t bytes()const{size_t n=0;for(const auto&e:entries)n+=e.bytes;return n;}
};
}
