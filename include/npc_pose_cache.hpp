// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>
#include "npcs.hpp"
#include <cstring>
namespace lab {
// Two 1024x1024 RGBA5551 textures: 112 exact original poses, fixed 4 MiB.
inline bool readNpcPoseManifest(const std::vector<uint8_t>&b,const std::vector<NpcStyle>&styles,unsigned frames,std::vector<uint32_t>&checksums){
    if((frames!=40&&frames!=48)||styles.empty()||styles.size()>256||b.size()!=8+styles.size()*(16+4*frames))return false;
    auto u16=[&](size_t i){return unsigned(b[i])|unsigned(b[i+1])<<8;};
    auto u32=[&](size_t i){return uint32_t(b[i])|uint32_t(b[i+1])<<8|uint32_t(b[i+2])<<16|uint32_t(b[i+3])<<24;};
    if(std::memcmp(b.data(),"UPS1",4)||u16(4)!=styles.size()||u16(6)!=frames)return false;
    std::vector<uint32_t>next;next.reserve(styles.size()*frames);size_t cursor=8;
    for(const auto&m:styles){
        if(int16_t(u16(cursor))!=m.pivotX||int16_t(u16(cursor+2))!=m.pivotY||u16(cursor+4)!=unsigned(m.width)||u16(cursor+6)!=unsigned(m.height)||u32(cursor+8)!=m.bytes||u32(cursor+12)!=m.checksum||m.width<=0||m.width>128||m.height<=0||m.height>144)return false;
        cursor+=16;for(unsigned n=0;n<frames;++n,cursor+=4)next.push_back(u32(cursor));
    }
    checksums=std::move(next);return true;
}
struct NpcPoseNeed {int key;float distance;unsigned priority;};
inline void addNpcPoseCycle(std::vector<NpcPoseNeed>&out,int style,unsigned frame,unsigned frames,float distance){
    auto add=[&](unsigned pose,unsigned priority){const int key=int(unsigned(style)*frames+pose);auto found=std::find_if(out.begin(),out.end(),[&](const NpcPoseNeed&n){return n.key==key;});if(found==out.end())out.push_back({key,distance,priority});else if(priority<found->priority||(priority==found->priority&&distance<found->distance))*found={key,distance,priority};};
    add(frame,0);
    if(frame<40){unsigned base=frame/5*5,phase=frame%5;for(unsigned n=1;n<=4;++n)add(base+(phase?1+(phase-1+n)%4:n),n);add(base,5);}
}
inline void orderNpcPoseNeeds(std::vector<NpcPoseNeed>&out){std::stable_sort(out.begin(),out.end(),[](const NpcPoseNeed&a,const NpcPoseNeed&b){return a.priority<b.priority||(a.priority==b.priority&&a.distance<b.distance);});}
// One protected read stays in flight until consumed; enumerating more pages cannot
// replace it. The same selector is exercised by delayed-I/O native regressions.
struct WorldPageRead {
    int pending=-1;
    int choose(const std::vector<unsigned char>&needed,const std::vector<unsigned char>&resident,const std::vector<uint64_t>&retry,uint64_t now,const std::vector<uint32_t>&priority={})const{
        if(pending>=0)return pending;
        int best=-1;
        for(size_t i=0;i<needed.size();++i)if(needed[i]&&!resident[i]&&now>=retry[i]){
            if(priority.size()!=needed.size())return int(i);
            if(best<0||priority[i]<priority[unsigned(best)])best=int(i);
        }
        return best;
    }
    // Keep the protected in-flight page, then read the closest visible misses.
    // Small scenery pages can overlap disk I/O with the next GPU upload without
    // increasing the existing four-request / four-MiB AsyncResources limit.
    std::vector<int> batch(const std::vector<unsigned char>&needed,const std::vector<unsigned char>&resident,const std::vector<uint64_t>&retry,uint64_t now,const std::vector<uint32_t>&priority,const std::vector<size_t>&costs,int first)const{
        if(first<0)return {};
        std::vector<int>out{first};size_t total=costs[unsigned(first)];
        // A speculative read may finish, but never starts more speculation.
        if(!needed[unsigned(first)])return out;
        auto admitted=resident;admitted[unsigned(first)]=1;WorldPageRead next;
        while(out.size()<4){int page=next.choose(needed,admitted,retry,now,priority);if(page<0)break;
            admitted[unsigned(page)]=1;
            if(costs[unsigned(page)]>512*1024||total>512*1024-costs[unsigned(page)])break;
            out.push_back(page);total+=costs[unsigned(page)];
        }
        return out;
    }
};
struct NpcPoseCache {
    static constexpr unsigned Slots=112, Across=8, Rows=7, CellWidth=128, CellHeight=144;
    struct Slot {int key=-1;uint64_t used=0;};
    std::array<Slot,Slots> slots{};uint64_t clock=0;
    int find(int key)const{for(unsigned i=0;i<Slots;++i)if(slots[i].key==key)return int(i);return -1;}
    int fallback(int style,unsigned frames,bool dead=false)const{int best=-1;for(unsigned i=0;i<Slots;++i)if(slots[i].key>=0&&unsigned(slots[i].key)/frames==unsigned(style)&&((unsigned(slots[i].key)%frames>=40)==dead)&&(best<0||slots[i].used>slots[best].used))best=int(i);return best;}
    int victim(const std::vector<int>&pinned)const{int best=-1;for(unsigned i=0;i<Slots;++i){if(slots[i].key<0)return int(i);if(std::find(pinned.begin(),pinned.end(),slots[i].key)==pinned.end()&&(best<0||slots[i].used<slots[best].used))best=int(i);}return best;}
    void touch(int slot){if(slot>=0)slots[unsigned(slot)].used=++clock;}
    void assign(int slot,int key){slots[unsigned(slot)]={key,++clock};}
};
}
