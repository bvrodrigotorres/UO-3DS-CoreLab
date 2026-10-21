// SPDX-License-Identifier: MIT
#include "core.hpp"
#include "view.hpp"
#include "doors.hpp"
#include "inventory.hpp"
#include "journal.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <chrono>
using namespace lab;
int main(int argc,char**argv){
    World w;w.width=w.height=256;w.vertices.assign(257*257,0);w.land.resize(256*256);
    for(int y=0;y<256;y+=2)for(int x=0;x<256;x+=2){Object o;o.x=x;o.y=y;o.flags=Surface;w.objects.push_back(o);}
    w.index();
    for(Actor a:std::vector<Actor>{{0,0,0},{128,128,0},{255,255,0}}){
        auto list=renderList(w,a,false);unsigned land=0;int last=-1;
        for(const auto&d:list){assert(std::abs(d.x-a.x)<=12&&std::abs(d.y-a.y)<=12);assert(d.x+d.y>=last);last=d.x+d.y;land+=d.kind==Kind::Land;}
        assert(land<=625&&land>=169);
    }
    unsigned seed=17;
    for(int n=0;n<1000;++n){seed=seed*1664525+1013904223;size_t i=seed%w.objects.size();auto&o=w.objects[i];int x=o.x,y=o.y;o.x=(x+1)%256;o.y=(y+3)%256;w.moved(i,x,y);}
    World rebuilt=w;rebuilt.index();assert(w.cells==rebuilt.cells);
    if(argc==2){
        const std::string root=argv[1],scene=root+"/scene.bin";std::string error;
        assert(loadWorld(scene,w,error));assert(w.width==256&&w.height==256&&w.objects.size()>40000);
        Doors doors;assert(doors.load(root+"/doors.bin",scene,w,error));assert(doors.count()>=147&&doors.count()<=158);
        Inventory inventory;inventory.init(w,"/private/tmp/corelab-large-test-never-created.sav");assert(inventory.healthy());
        Journal journal;assert(loadJournal(root+"/journal.bin",scene,journal,error));assert(journal.width==256);
        std::ifstream f(root+"/image_sizes.bin",std::ios::binary);std::vector<unsigned char>b(8+w.images*4+(w.images+31)/32*4);assert(f.read((char*)b.data(),b.size()));
        auto n=[&](size_t at,int count){unsigned value=0;for(int i=0;i<count;++i)value|=unsigned(b[at+i])<<(8*i);return value;};
        std::vector<std::array<unsigned,2>>sizes;for(int i=0;i<w.images;++i)sizes.push_back({n(8+4*i,2),n(10+4*i,2)});
        std::vector<size_t>costs;for(int i=0;i<(w.images+31)/32;++i)costs.push_back(n(8+4*w.images+4*i,4));
        size_t peak=0,views=0;auto start=std::chrono::steady_clock::now();
        for(int y=2;y<256;y+=4)for(int x=2;x<256;x+=4)for(int z:{0,20,40,60}){
            Actor a{x,y,z};auto list=renderList(w,a,true);std::vector<bool>needed(costs.size());
            for(const auto&d:list)if(worldVisible(w,d,x-.5f,y+.25f,z+.5f,sizes)){
                unsigned id=d.kind==Kind::Land?w.land[d.index].image:d.kind==Kind::Static?w.objects[d.index].image:NoImage;
                if(id!=NoImage)needed[id/32]=true;
            }
            size_t used=0;for(size_t i=0;i<needed.size();++i)if(needed[i])used+=costs[i];peak=std::max(peak,used);++views;
        }
        std::cout<<"Large private region: "<<w.objects.size()<<" objects, "<<inventory.chests.size()<<" chests, "<<doors.count()<<" doors, "<<views<<" views; peak needed atlas MiB "<<peak/1048576.f<<"; host ms "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()<<"\n";
        assert(peak<=14*1024*1024);
        const auto closed=w.objects;
        for(size_t i=0;i<closed.size();++i)if((closed[i].flags&Door)&&closed[i].order>=doors.baseObjects){
            Actor a{closed[i].x,closed[i].y-1,closed[i].z};if(!w.contains(a.x,a.y))continue;
            if(doors.tryOpen(a,0,1)){assert(doors.update(w,.1f));rebuilt=w;rebuilt.index();assert(w.cells==rebuilt.cells);doors.update(w,3);}
        }
    }
    std::cout<<"Large-region bounded view and incremental cell index passed\n";
}
