// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <algorithm>
namespace lab {
// Local neighbor-column prefix scheduling, independently implemented from
// CrossUO GameScreen::AddOffsetCharacterTileToRenderList. Never a floor pass.
class CharacterOrder {
    std::vector<Draw>ordered;
    std::vector<uint8_t>used;
public:
    const std::vector<Draw>&build(const World&w,const std::vector<Draw>&input,int visualZ){
        ordered.clear();ordered.reserve(input.size());used.assign(input.size(),0);
        auto emit=[&](size_t index){ordered.push_back(input[index]);used[index]=1;};
        for(size_t i=0;i<input.size();++i){
            if(used[i])continue;
            const auto&actor=input[i];
            if(actor.kind==Kind::Actor){
                // During descent follow the visible feet rather than the old
                // landing height. At the end of a step this is exactly the
                // same order as standing still; upper floors stay in front.
                int ceiling=std::max(actor.z,visualZ)+1;
                const int dx[]={1,1,2,-1,0,1,2,1};
                const int dy[]={-1,-2,-2,2,1,0,-1,1};
                for(int cell=0;cell<8;++cell){
                    int x=actor.x+dx[cell],y=actor.y+dy[cell];
                    if(!w.contains(x,y))continue;
                    for(size_t j=i+1;j<input.size();++j){
                        const auto&d=input[j];
                        if(used[j]||d.x!=x||d.y!=y)continue;
                        int top=d.priority;
                        if(d.kind==Kind::Static){
                            if(d.index>=w.objects.size())break;
                            const auto&o=w.objects[d.index];
                            // At foot level, only a walkable surface may be
                            // pulled forward. Even zero-height art can be a
                            // visible wall, door or decoration in front of us.
                            if(o.z>=std::min(actor.z,visualZ)-2&&
                               ((o.flags&(Wall|Door|Impassable))||!(o.flags&(Surface|Bridge))))break;
                            top+=o.height;
                        }else if(d.kind==Kind::Npc)top+=14;
                        else if(d.kind!=Kind::Land)break;
                        // Stop the entire column at its first high object.
                        // Pulling just the floor can expose items/NPC below it.
                        if(top>ceiling)break;
                        emit(j);
                    }
                }
            }
            emit(i);
        }
        return ordered;
    }
};
}
