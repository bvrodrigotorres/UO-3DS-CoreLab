// SPDX-License-Identifier: MIT
#include "character_depth.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    World w=demoWorld();Actor actor{8,8,37};
    auto support=[&](int z,int height,uint64_t flags,float visible){
        Object o;o.z=z;o.height=height;o.flags=flags;w.objects={o};
        return supportingFragment(w,{Kind::Static,8,8,z,z,0,0},actor,visible);
    };
    assert(support(35,5,Surface|Bridge,34.5)); // walking height 37, not art height 40
    assert(support(30,10,Surface|Bridge,34.5));
    assert(!support(40,0,Surface,34.5)); // upper floor must cover us
    assert(!support(35,5,Surface|Wall,37));
    assert(!support(35,0,Surface|Door,37));
    assert(!support(20,5,Surface|Impassable,37));
    assert(!support(20,5,Surface|Roof,37));
    actor.z=27;assert(support(35,0,Surface,37)); // visual descent height
    assert(!support(35,0,Surface,27));
    assert(!supportingFragment(w,{Kind::Npc,8,8,20,21,0,0},actor,37));
    assert(fragmentDepth(CharacterDepthZ)<fragmentDepth(WorldDepthZ));
    // Emulate the explicit GPU depth test: supports use <=, all other draws
    // use ALWAYS. Normal opaque scenery releases the actor marker; rejected
    // transparent texels write neither color nor depth.
    float depth=1;int color=0;
    auto draw=[&](int value,bool lower,bool player,bool opaque=true,bool translucent=false){
        if(!opaque)return;
        float next=fragmentDepth(player?CharacterDepthZ:WorldDepthZ);
        if(lower&&next>depth)return;
        color=value;if(!translucent)depth=next;
    };
    draw(1,true,false);assert(color==1);draw(2,false,true);
    draw(3,true,false);assert(color==2); // step cannot erase robe
    draw(4,false,false,false);draw(5,true,false);assert(color==2);
    draw(6,false,false);assert(color==6); // railing occludes robe
    draw(7,true,false);assert(color==7); // painter order resumes behind railing
    draw(8,false,true);draw(9,false,false,true,true);draw(10,true,false);assert(color==9);
    draw(11,false,false);assert(color==11); // upper floor remains visible
    // No actor fragment => every world fragment paints in its original order.
    unsigned seed=19;
    for(int view=0;view<1000;++view){depth=1;color=0;
        for(int i=1;i<100;++i){seed=seed*1664525u+1013904223u;draw(i,seed&1,false);assert(color==i);}
    }
    if(argc==2){std::string error;assert(loadWorld(argv[1],w,error));size_t cases=0;
        for(int z:{20,22,27,32,37,40})for(int y=6;y<=10;++y)for(int x=5;x<=10;++x){
            actor={x,y,z};const auto list=renderList(w,actor,true);
            for(const auto&d:list){(void)supportingFragment(w,d,actor,z-.5f);++cases;}
        }
        std::cout<<"Private staircase classifications: "<<cases<<" unchanged draw entries\n";
    }
    std::cout<<"Character pixel depth: bridge heights, foreground occlusion, upper floors, alpha rejection and 1000 unchanged background sequences passed\n";
}
