// SPDX-License-Identifier: MIT
#include "core.hpp"
#include "reference.hpp"
#include "avatar.hpp"
#include "exploration.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <cstring>
#include <fstream>
#include <cerrno>
#include <sys/stat.h>
using namespace lab;
static int fatRename(const char*source,const char*target){
    struct stat info;
    if(stat(target,&info)==0){errno=EEXIST;return -1;}
    return std::rename(source,target);
}
static int failInstall(const char*source,const char*target){
    std::string path=source;
    if(path.size()>=4&&path.substr(path.size()-4)==".tmp"){errno=EIO;return -1;}
    return fatRename(source,target);
}
int main(int argc,char**argv){
    if(argc==3&&std::string(argv[1])=="--exploration"){
        std::string root=argv[2],error;World w=demoWorld();Actor a={7,7,4},b={0,0,0};
        std::string file=root+"/position.sav";
        assert(savePosition(file,w,a,error));assert(loadPosition(file,w,b,error));
        assert(a.x==b.x&&a.y==b.y&&a.z==b.z);
        assert(savePosition(file,w,{8,10,0},error));
        assert(loadPosition(file,w,b,error)&&b.x==8&&b.y==10);
        // Emulate SD/FAT rename: an existing target cannot be overwritten.
        for(int i=0;i<4;++i){
            assert(savePosition(file,w,{7+i,10,0},error,fatRename));
            assert(loadPosition(file,w,b,error)&&b.x==7+i&&b.y==10);
        }
        assert(!savePosition(file,w,{5,10,0},error,failInstall));
        assert(loadPosition(file,w,b,error)&&b.x==10); // rollback kept last save
        // Simulated interruption after the old file moved to .bak.
        std::remove((file+".bak").c_str());
        assert(fatRename(file.c_str(),(file+".bak").c_str())==0);
        assert(loadPosition(file,w,b,error)&&b.x==10);
        assert(savePosition(file,w,{8,10,0},error,fatRename));
        assert(loadPosition(file,w,b,error)&&b.x==8);
        World changed=w;changed.vertices[0]++;
        assert(!loadPosition(file,changed,b,error));
        {std::fstream f(file,std::ios::binary|std::ios::in|std::ios::out);f.seekp(9);f.put('x');}
        Actor before=b;assert(!loadPosition(file,w,b,error));assert(b.x==before.x&&b.y==before.y&&b.z==before.z);
        assert(!savePosition(file,w,{-1,0,0},error));
        assert(mapCell(w,a,7,7)=='@'&&mapCell(w,a,-1,0)==' '&&mapCell(w,a,4,5)=='#');
        {std::ofstream f(root+"/avatar.t3x",std::ios::binary);f<<"hello";}
        {std::ofstream f(root+"/avatar.meta");f<<"UOA1 400 0 4 2 72 4f9f2cab\n";}
        assert(verifyRunAtlas(root+"/avatar.t3x",root+"/avatar.meta"));
        {std::ofstream f(root+"/avatar.meta");f<<"UOA1 400 0 4 1 72 4f9f2cab\n";}
        assert(!verifyRunAtlas(root+"/avatar.t3x",root+"/avatar.meta"));
        {std::ofstream f(root+"/avatar.meta");f<<"UOA1 400 0 4 2 72 4f9f2cab\n";}
        {std::ofstream f(root+"/avatar.t3x",std::ios::binary);f<<"wrong";}
        assert(!verifyRunAtlas(root+"/avatar.t3x",root+"/avatar.meta"));
        std::cout<<"Save integrity, scene identity, map and run provenance passed\n";return 0;
    }
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
