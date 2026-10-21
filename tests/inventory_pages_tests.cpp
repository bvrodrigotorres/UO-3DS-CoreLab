// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
namespace {
int calls=0;
int counted(const char*a,const char*b){++calls;return std::rename(a,b);}
int failInstall(const char*a,const char*b){if(++calls==2)return -1;return std::rename(a,b);}
bool equal(const Bag&a,const Bag&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].type!=b[i].type||a[i].count!=b[i].count)return false;return true;}
std::vector<char>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(int argc,char**argv){
    assert(argc==2);std::string path=std::string(argv[1])+"/pages.sav";
    World w;w.width=w.height=16;w.vertices.assign(289,0);w.land.resize(256);Object chest;chest.art=0xE42;chest.x=5;chest.y=5;w.objects={chest};w.index();Actor player{4,5,0};Inventory inv;inv.init(w,path);
    Combat combat;Npcs npcs;combat.reset(npcs);
    assert(combat.inspectNearest(w,player,inv)&&inv.containerName()=="COFRE");assert(!combat.inspectNearest(w,player,inv));
    auto&body=combat.corpses[0];body.active=true;body.position={3,6,0};body.name="a rat";body.decay=90;body.loot={{0,3}};
    assert(!combat.inspectNearest(w,player,inv)&&inv.opened==0); // adjacent chest beats diagonal corpse
    body.position=player;assert(combat.inspectNearest(w,player,inv)&&inv.containerName()=="a rat");
    inv.equipMode=true;assert(!combat.inspectNearest(w,player,inv)&&inv.equipMode);
    body.active=false;assert(combat.inspectNearest(w,player,inv)&&inv.containerName()=="COFRE");
    assert(combat.inspectNearest(w,{0,0,0},inv)&&!inv.hasContainer());combat.inspectNearest(w,player,inv);
    // Bulk transfer is one atomic write, not one write for every item.
    inv.backpack={{0,3}};inv.chests[0].items={{0,8},{3,4},{4,1}};assert(inv.checkpoint());calls=0;
    assert(inv.takeAll(w,player,counted)&&calls==2&&inv.chests[0].items.empty()&&PlayerStatus::gold(&inv)==11);
    Inventory restored;restored.init(w,path);assert(restored.restored&&equal(restored.backpack,inv.backpack));
    inv.chests[0].items={{0,2},{3,4}};assert(inv.checkpoint());auto oldFile=read(path);Bag oldBag=inv.backpack,oldChest=inv.chests[0].items;calls=0;
    assert(!inv.takeAll(w,player,failInstall)&&equal(oldBag,inv.backpack)&&equal(oldChest,inv.chests[0].items)&&read(path)==oldFile);
    inv.backpack.assign(23,{4,1});inv.chests[0].items={{0,5},{3,2}};
    assert(inv.takeAll(w,player)&&inv.backpack.size()==24&&inv.chests[0].items.size()==1&&inv.chests[0].items[0].type==3);
    assert(!inv.takeAll(w,player)&&inv.chests[0].items[0].count==2);
    inv.backpack.assign(23,{4,1});inv.backpack.push_back({0,65535});inv.chests[0].items={{0,1}};
    assert(!inv.takeAll(w,player)&&inv.chests[0].items[0].count==1&&PlayerStatus::gold(&inv)==65535);
    // All 24 slots persist; the second page maps its last tile to item 23.
    inv.backpack.assign(24,{5,1});inv.backpack[23]={7,1};inv.equipment.fill(-1);inv.equipMode=true;assert(inv.checkpoint());
    inv.tap(w,player,25,200);assert(inv.bagPage==1);
    inv.tap(w,player,InventoryLayout::Left+2*44+3,InventoryLayout::Top+3*38+3);assert(inv.equipment[2]==7&&inv.backpack.size()==23);
    assert(inv.unequip(2)&&inv.backpack.size()==24);Inventory pages;pages.init(w,path);assert(pages.restored&&pages.backpack.size()==24&&pages.backpack[23].type==7);
    auto bytes=read(path);assert(bytes[3]=='3');
    // UIN2 saves with the original nine slots migrate without resupply/loss.
    inv.backpack={{0,17},{5,1}};inv.chests[0].items={{1,2}};assert(inv.checkpoint());bytes=read(path);bytes[3]='2';uint32_t hash=2166136261;
    for(size_t i=0;i<bytes.size()-4;++i)hash=(hash^uint8_t(bytes[i]))*16777619u;
    for(int i=0;i<4;++i)bytes[bytes.size()-4+i]=char(hash>>(8*i));
    {std::ofstream f(path,std::ios::binary);f.write(bytes.data(),bytes.size());}
    Inventory legacy;legacy.init(w,path);assert(legacy.restored&&legacy.backpack.size()==2&&legacy.chests[0].items.size()==1&&PlayerStatus::gold(&legacy)==17);
    // Corpse bulk transfer persists collected loot and leaves no borrowed items.
    body.active=true;body.position=player;body.loot={{0,4},{3,1}};legacy.backpack.clear();assert(combat.inspectNearest(w,player,legacy));assert(legacy.takeAll(w,player)&&body.loot.empty());
    assert(!legacy.takeAll(w,player));Inventory loot;loot.init(w,path);assert(PlayerStatus::gold(&loot)==4);
    for(int page=0;page<2;++page){pages.bagPage=page;std::vector<uint8_t>fb(320*240*3+32,0xCD);drawInventory(fb.data()+16,w,player,pages);for(int i=0;i<16;++i)assert(fb[i]==0xCD&&fb[fb.size()-1-i]==0xCD);}
    TouchPages touch;touch.page=2;touch.begin(200,220);assert(!touch.end()&&touch.page==2);
    std::cout<<"Nearest chest/corpse preview, atomic/partial take-all, SD rollback, 24-slot saves, UIN2 migration, page-2 equipment and enlarged touch/HUD bounds passed\n";
}
