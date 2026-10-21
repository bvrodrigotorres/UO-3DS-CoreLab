// SPDX-License-Identifier: MIT
#include "npc_push.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    Actor from{4,4,0},to{5,4,0};NpcPush push;
    for(int i=0;i<26;++i)assert(!push.allow(true,from,to,2,1.f/60));
    bool passed=false;for(int i=0;i<3;++i)passed|=push.allow(true,from,to,2,1.f/60);assert(passed);
    assert(!push.allow(true,from,{4,5,0},4,1.f/60)); // different intent/target restarts contact
    push.reset();assert(!push.allow(true,from,to,2,0));
    assert(push.allow(false,from,to,2,0));assert(!push.allow(true,from,to,2,1.f/60));
    // This policy is consulted only after geometry succeeds: walls stay solid.
    World w;w.width=w.height=10;w.vertices.assign(121,0);w.land.resize(100);
    Object wall;wall.x=5;wall.y=4;wall.z=0;wall.height=40;wall.flags=Wall|Impassable;w.objects.push_back(wall);w.index();
    Actor next;int actual;assert(!step(w,from,2,next,actual));
    assert(PlayerStatus::barWidth(25,50,148)==74);assert(PlayerStatus::barWidth(-1,50,148)==0);
    assert(PlayerStatus::barWidth(90,50,148)==148);assert(PlayerStatus::barWidth(10,0,148)==0);
    Inventory inv;inv.backpack={{0,25},{1,10},{0,75}};inv.chests.push_back({0,{{0,500}}});assert(PlayerStatus::gold(&inv)==100&&PlayerStatus::gold(nullptr)==0);
    TouchPages pages;pages.page=1;pages.begin(30,JournalLayout::ZoomTop+4);assert(pages.end()&&pages.zoom==1);pages.begin(160,JournalLayout::ZoomTop+4);assert(pages.end()&&pages.zoom==2);
    pages.begin(160,175);assert(!pages.end()&&pages.zoom==2); // resource panel cannot change map zoom
    pages.begin(250,216);assert(!pages.end()&&pages.page==1);
    pages.begin(250,225);assert(!pages.end()&&pages.page==1);
    std::vector<uint8_t>fb(320*240*3+32,0xCD);PlayerStatus stats;stats.hits=29;stats.mana=17;stats.stamina=43;
    drawJournal(fb.data()+16,w,from,nullptr,2,"",&inv,nullptr,&stats);
    for(int i=0;i<16;++i)assert(fb[i]==0xCD&&fb[fb.size()-1-i]==0xCD);
    if(argc==2){std::ofstream f(argv[1],std::ios::binary);f<<"P6\n320 240\n255\n";for(int y=0;y<240;++y)for(int x=0;x<320;++x){auto p=fb.data()+16+3*(x*240+239-y);char rgb[]={char(p[2]),char(p[1]),char(p[0])};f.write(rgb,3);}}
    std::cout<<"NPC contact/retry/release, solid geometry, status bars, live gold, moved touch targets and framebuffer bounds passed\n";
}
