// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
namespace lab {
// Only softens mobile occupancy, after the ordinary world step has succeeded.
class NpcPush {
    Actor origin{},target{};int direction=-1;float elapsed=0;
    static bool same(const Actor&a,const Actor&b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
public:
    void reset(){direction=-1;elapsed=0;}
    bool allow(bool occupied,const Actor&from,const Actor&to,int intent,float dt){
        if(!occupied){reset();return true;}
        if(direction!=intent||!same(origin,from)||!same(target,to)){direction=intent;origin=from;target=to;elapsed=0;}
        if(dt>0)elapsed+=dt;
        return elapsed>=.45f;
    }
};
}
