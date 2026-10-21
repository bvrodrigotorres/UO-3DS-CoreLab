// SPDX-License-Identifier: MIT
#pragma once
#include "visibility.hpp"
#include <algorithm>
#include <cmath>
namespace lab {
// Roofs are immutable even when doors move. Build connectivity once per scene.
class RoofCutaway {
    const World*world=nullptr;const Object*storage=nullptr;
    std::vector<int>group;std::vector<unsigned char>active;
public:
    void build(const World&w){
        world=&w;storage=w.objects.data();group.assign(w.objects.size(),-1);active.clear();
        std::vector<size_t>queue;
        for(size_t seed=0;seed<w.objects.size();++seed){
            if(!(w.objects[seed].flags&Roof)||group[seed]>=0)continue;
            int id=int(active.size());active.push_back(0);queue.clear();queue.push_back(seed);group[seed]=id;
            for(size_t at=0;at<queue.size();++at){const auto&o=w.objects[queue[at]];
                for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){int x=o.x+dx,y=o.y+dy;if(!w.contains(x,y))continue;
                    for(size_t next:w.cells[y*w.width+x]){const auto&n=w.objects[next];
                        if((n.flags&Roof)&&group[next]<0&&std::abs(n.z-o.z)<=16){group[next]=id;queue.push_back(next);}
                    }
                }
            }
        }
    }
    void update(const World&w,const Actor&a,RoofMode mode){
        if(world!=&w||storage!=w.objects.data()||group.size()!=w.objects.size())build(w);
        std::fill(active.begin(),active.end(),0);
        if(mode!=RoofMode::Auto||!w.contains(a.x,a.y))return;
        const auto&cell=w.cells[a.y*w.width+a.x];
        for(size_t i:cell){const auto&o=w.objects[i];if(group[i]<0||o.z<=a.z+14)continue;
            bool standingOnRoof=false;
            for(size_t j:cell)if(group[j]==group[i]&&std::abs(w.objects[j].z+w.objects[j].height-a.z)<=2)standingOnRoof=true;
            if(!standingOnRoof)active[group[i]]=1;
        }
    }
    bool hidden(const World&w,size_t i,const Actor&a,RoofMode mode)const{
        if(mode==RoofMode::Full||i>=group.size()||!(w.objects[i].flags&Roof))return false;
        if(mode==RoofMode::Hidden)return w.objects[i].z>a.z+2;
        return group[i]>=0&&active[group[i]];
    }
};
}
