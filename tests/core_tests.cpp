// SPDX-License-Identifier: MIT
#include "core.hpp"
#include "reference.hpp"
#include "avatar.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <cstring>
using namespace lab;
int main(int argc,char**argv){
    if(argc>1){World w;std::string error;bool ok=loadWorld(argv[1],w,error);std::cout<<(ok?"OK":"INVALID")<<" "<<error<<"\n";return ok?0:2;}
    std::mt19937 random(71425);int comparisons=0;
    for(int test=0;test<20000;++test){
        std::vector<Candidate> list;int z=int(random()%65)-32,minZ=z-int(random()%8),maxZ=z+int(random()%8);
        for(unsigned i=0,n=1+random()%12;i<n;++i){int base=int(random()%100)-50,h=random()%35;unsigned flags=1+2*(random()%4);list.push_back({flags,base,base+((flags&4)?h/2:h),h});}
        int a=900,b=900;bool native=solveCandidates(list,z,minZ,maxZ,a),original=referenceSolve(list,z,minZ,maxZ,b);
        assert(native==original);if(native)assert(a==b);++comparisons;
    }
    for(int test=0;test<3000;++test){
        std::vector<Draw> input;std::vector<int> size,avg;std::vector<uint64_t>flags;std::vector<bool>slope;
        // One terrain plus random statics and one actor; insertion order is stable.
        for(int i=0,n=2+random()%18;i<n;++i){
            Kind kind=i==0?Kind::Land:i==n-1?Kind::Actor:Kind::Static;
            int z=int(random()%16)-8,h=random()%2?0:20,average=int(random()%16)-8;bool sloped=random()%2;
            uint64_t f=random()%2?Background:0;
            input.push_back({kind,0,0,z,priority(kind,z,h,f,average,sloped),(size_t)i,(uint32_t)i});
            size.push_back(h);avg.push_back(average);flags.push_back(f);slope.push_back(sloped);
        }
        auto expected=referenceOrder(input,size,flags,avg,slope);auto sorted=input;
        std::stable_sort(sorted.begin(),sorted.end(),[](const Draw&a,const Draw&b){if(a.priority!=b.priority)return a.priority<b.priority;if((a.kind==Kind::Land)!=(b.kind==Kind::Land))return a.kind==Kind::Land;return a.order<b.order;});
        for(size_t i=0;i<sorted.size();++i){
            assert((int)sorted[i].index==expected[i]);
        }
        ++comparisons;
    }
    int next=99;
    assert(solveCandidates({{7,0,0,0},{3,24,24,0}},0,0,2,next)&&next==0);
    assert(!solveCandidates({{7,0,0,0},{1,10,30,20}},0,0,2,next)); // no headroom
    assert(solveCandidates({{7,0,0,0},{7,0,2,4}},0,0,2,next)&&next==2); // half-height bridge
    assert(!solveCandidates({{3,4,4,0}},0,0,2,next)); // cannot climb a solid 4-unit floor
    World w=demoWorld();Actor a={8,10,0},b;int d=0;
    assert(step(w,a,6,b,d));assert(b.x==7&&b.y==10);
    assert(!step(w,{4,5,4},0,b,d)); // solid room wall
    auto list=renderList(w,{6,6,4},true);
    assert(std::none_of(list.begin(),list.end(),[](const Draw&v){return v.kind==Kind::Static&&v.z>=24;}));
    auto all=renderList(w,{6,6,4},false);
    assert(all.size()>list.size()); // cutaway never deletes stored objects
    assert(!step(w,{-1,0,0},2,b,d));
    auto h=heights(w,11,10);assert(h.slope&&h.top==0&&h.right==2&&h.bottom==2&&h.left==0);
    w.vertices[0]=-3;w.vertices[1]=-3;w.vertices[17]=-2;w.vertices[18]=-2;
    h=heights(w,0,0);assert(h.average==-3); // arithmetic-shift-compatible negative average
    RenderCache cache;
    auto checkCache=[&](const World&world,Actor actor,bool cut){
        auto expected=renderList(world,actor,cut);
        const auto&actual=cache.get(world,actor,cut);
        assert(actual.size()==expected.size());
        for(size_t i=0;i<actual.size();++i){
            assert(actual[i].kind==expected[i].kind&&actual[i].index==expected[i].index&&
                   actual[i].priority==expected[i].priority&&actual[i].z==expected[i].z&&
                   actual[i].x==expected[i].x&&actual[i].y==expected[i].y);
        }
    };
    for(int i=0;i<120;++i)checkCache(w,{6,6,4},true);
    assert(cache.rebuilds==1);
    checkCache(w,{7,6,4},true);assert(cache.rebuilds==2);
    checkCache(w,{7,6,20},true);assert(cache.rebuilds==3);
    checkCache(w,{7,6,20},false);assert(cache.rebuilds==4);
    World other=demoWorld();checkCache(other,{7,6,20},false);assert(cache.rebuilds==5);
    cache.invalidate();checkCache(other,{7,6,20},false);assert(cache.rebuilds==6);
    for(int direction=0;direction<8;++direction)for(unsigned frame=0;frame<4;++frame){
        unsigned uo=(direction+7)&7;
        assert(avatarImage(direction,true,true,frame,72)==40+uo*4+frame);
        assert(avatarImage(direction,true,false,frame,72)==uo*5+1+frame);
        assert(avatarImage(direction,false,true,frame,72)==uo*5);
        assert(avatarImage(direction,true,true,frame,40)==uo*5+1+frame);
    }
    std::cout<<comparisons<<" upstream cases + cache equivalence + run/idle/walk atlas checks passed\n";
}
