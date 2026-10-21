// SPDX-License-Identifier: MIT
#include "inventory.hpp"
#include "character.hpp"
#include "performance.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
namespace {int fail(const char*,const char*){return -1;}unsigned total(const Inventory&i,int t){unsigned n=0;for(auto s:i.backpack)if(s.type==t)n+=s.count;for(int x:i.equipment)if(x==t)++n;return n;}}
int main(int argc,char**argv){
    assert(argc==2);std::string path=std::string(argv[1])+"/wardrobe.sav";
    World w=demoWorld();Object chest;chest.art=0xE42;chest.x=5;chest.y=5;w.objects={chest};w.index();
    Inventory i;i.init(w,path);i.initialDress(2,2,true);i.addAccessories();auto before=i.equipment;auto loot=i.chests[0].items.size();
    assert(i.equipment[5]==9&&i.equipment[7]==11&&i.equipment[2]==12&&i.equipmentMask()==1956);
    i.initialDress(1,1,false);i.addAccessories();assert(i.equipment==before&&i.chests[0].items.size()==loot&&loot==10);
    assert(i.checkpoint());Inventory loaded;loaded.init(w,path);assert(loaded.healthy()&&loaded.restored&&loaded.dressed&&loaded.accessories&&loaded.equipment==before);
    unsigned prepared=0;i.prepareEquipment=[&](unsigned key){prepared=key;return true;};
    i.backpack={{13,1},{14,1},{15,1},{8,1},{10,1},{7,1},{5,1},{6,1}};
    for(int t:{13,14,15,8,10,7,5,6}){size_t index=0;while(i.backpack[index].type!=t)++index;assert(i.equip(index));assert(prepared==i.equipmentMask());}
    for(int slot=0;slot<8;++slot)assert(i.equipment[slot]>=0);
    assert(i.equipmentMask()==255&&total(i,9)==1&&total(i,11)==1&&total(i,12)==1);
    auto savedEquipment=i.equipment;auto bagSize=i.backpack.size();assert(!i.unequip(3,fail)&&i.equipment==savedEquipment&&i.backpack.size()==bagSize);
    Inventory resumed;resumed.init(w,path);assert(resumed.healthy()&&resumed.equipment==savedEquipment);
    for(int slot=0;slot<8;++slot){assert(i.unequip(slot));assert(prepared==i.equipmentMask());}
    assert(i.equipmentMask()==0);for(int t=5;t<16;++t)assert(total(i,t)==1);
    Appearance a;a.enabled=true;a.realClothes=true;AppearanceEditor pause(a,false);int shirt=a.shirt,pants=a.pants,shoes=a.shoes;
    for(int row=0;row<4;++row){pause.page=2;pause.row=row;pause.change(1);}
    assert(pause.draft.shirt==shirt&&pause.draft.pants==pants&&pause.draft.shoes==shoes);
    pause.page=3;pause.row=0;pause.change(1);assert(pause.draft.shirtColor!=a.shirtColor);
    for(int seed=0;seed<100;++seed){pause.randomize(seed);assert(pause.draft.shirt==shirt&&pause.draft.pants==pants&&pause.draft.shoes==shoes);}
    pause.page=3;pause.row=2;pause.move(1);assert(pause.row==3);pause.change(1);assert(pause.draft.valid());
    EquipmentCache<int>cache;cache.insert(1956,1,20);cache.insert(255,2,20);assert(cache.get(1956)==1&&cache.get(255)==2&&cache.victim(1956)==255);assert(cache.remove(1956)==1);
    const int xy[8][2]={{290,35},{290,59},{290,107},{204,35},{204,59},{290,83},{204,83},{204,107}};
    for(int slot=0;slot<8;++slot)assert(equipmentHit(xy[slot][0],xy[slot][1])==slot);
    std::cout<<"Wardrobe: real starter items, one-time migration, 8 slots, variant cache keys, persistence, SD rollback, locked pause garments and touch targets passed\n";
}
