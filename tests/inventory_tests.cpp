// SPDX-License-Identifier: MIT
#include "inventory.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstdio>
using namespace lab;
static int failRename(const char*,const char*){return -1;}
static int calls=0;
static int failInstall(const char*a,const char*b){if(++calls==2)return -1;return std::rename(a,b);}
static unsigned total(const Inventory&i,unsigned type){unsigned n=0;for(auto s:i.backpack)if(s.type==type)n+=s.count;for(const auto&c:i.chests)for(auto s:c.items)if(s.type==type)n+=s.count;return n;}
int main(int argc,char**argv){
    assert(argc>=2);std::string path=std::string(argv[1])+"/inventory.sav";
    World w=demoWorld();Object c;c.x=5;c.y=5;c.z=0;c.art=0xE42;c.flags=Impassable;c.height=10;
    w.objects={c};w.index();Actor a{4,5,0};Inventory inv;inv.init(w,path);
    assert(inv.chests.size()==1&&inv.backpack.empty()&&inv.nearest(w,a)==0);
    assert(inv.nearest(w,{4,4,0})==-1&&inv.nearest(w,{4,5,20})==-1);
    inv.open(w,a);assert(inv.opened==0);
    auto gold=total(inv,0);assert(inv.transfer(w,a,true,0));assert(total(inv,0)==gold&&inv.backpack[0].count==gold);
    Inventory restored;restored.init(w,path);assert(total(restored,0)==gold&&restored.backpack[0].count==gold);
    restored.open(w,a);assert(restored.transfer(w,a,false,0));assert(restored.backpack.empty()&&total(restored,0)==gold);
    Inventory again;again.init(w,path);assert(again.backpack.empty()&&total(again,0)==gold);
    again.open(w,a);auto chestSize=again.chests[0].items.size();
    assert(!again.transfer(w,a,true,0,failRename));assert(again.backpack.empty()&&again.chests[0].items.size()==chestSize);
    calls=0;assert(!again.transfer(w,a,true,0,failInstall));assert(again.backpack.empty());
    Inventory rollback;rollback.init(w,path);assert(rollback.backpack.empty()&&total(rollback,0)==gold);
    assert(!again.transfer(w,{0,0,0},true,0)&&again.opened==-1);
    Object wall=c;wall.art=100;wall.flags=Wall|Impassable;wall.height=20;w.objects.push_back(wall);w.index();
    assert(inv.nearest(w,a)==-1);w.objects.pop_back();w.index();
    inv.opened=0;inv.backpack={{0,65535}};inv.chests[0].items={{0,1}};
    assert(!inv.transfer(w,a,true,0)&&inv.backpack[0].count==65535&&inv.chests[0].items[0].count==1);
    inv.backpack={{1,2}};inv.chests[0].items={{1,3}};assert(inv.transfer(w,a,true,0));assert(inv.backpack[0].count==5&&inv.chests[0].items.empty());
    inv.backpack.assign(BagSlots,{5,1});inv.chests[0].items={{0,3}};
    assert(!inv.transfer(w,a,true,0)&&inv.backpack.size()==BagSlots);
    inv.backpack={{5,1}};inv.chests[0].items={{5,1}};assert(inv.transfer(w,a,true,0));assert(inv.backpack.size()==2);
    // One atomic save holds both sides. A missing primary recovers its backup.
    assert(std::remove(path.c_str())==0);Inventory recovery;recovery.init(w,path);assert(recovery.backpack.size()==1&&recovery.backpack[0].type==1);
    // Corruption is never silently replaced with a fresh supply of loot.
    {std::ofstream f(path,std::ios::binary);f<<"bad";}Inventory bad;bad.init(w,path);bad.open(w,a);assert(bad.opened<0&&bad.backpack.empty()&&bad.chests[0].items.empty());
    assert(!bad.transfer(w,a,true,0));
    ItemIcons icons;assert(!icons.load(path));
    std::vector<uint8_t>pixels(320*240*3+32,0xA5);drawInventory(pixels.data()+16,w,a,inv);
    for(int n=0;n<16;++n)assert(pixels[n]==0xA5&&pixels[pixels.size()-1-n]==0xA5);
    TouchPages pages;pages.page=1;pages.begin(260,100);pages.move(20,101);assert(pages.end()&&pages.page==2);
    pages.begin(50,100);pages.move(250,102);assert(pages.end()&&pages.page==1);
    pages.page=2;pages.begin(250,220);assert(!pages.end()&&pages.page==2);
    inv.init(w,std::string(argv[1])+"/touch.sav");inv.open(w,a);assert(inv.opened==0);
    inv.tap(w,a,175,42);assert(!inv.backpack.empty());inv.tap(w,a,23,42);assert(inv.backpack.empty());
    std::cout<<"Inventory conservation, reach, transfer, merge, persistence, failed SD rollback, corruption, touch and HUD passed\n";
    if(argc==3){
        World actual;std::string error;assert(loadWorld(argv[2],actual,error));Inventory map;map.init(actual,std::string(argv[1])+"/map.sav");assert(map.chests.size()==8);
        for(size_t n=0;n<map.chests.size();++n){const auto&o=actual.objects[map.chests[n].object];bool reachable=false;
            for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(std::abs(dx)+std::abs(dy)==1)
                for(int z=o.z-8;z<=o.z+8;++z){Actor stand{o.x+dx,o.y+dy,z};if(map.reachable(actual,stand,n))reachable=true;}
            std::cout<<"Chest "<<n+1<<" at "<<o.x+actual.originX<<","<<o.y+actual.originY<<","<<o.z<<" reach="<<reachable<<"\n";assert(reachable);
        }
    }
}
