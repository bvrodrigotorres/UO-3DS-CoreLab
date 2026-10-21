// SPDX-License-Identifier: MIT
#include "inventory.hpp"
#include "equipment.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
using namespace lab;
namespace {
int fail(const char*,const char*){return -1;}
unsigned total(const Inventory&i,unsigned type){unsigned count=0;for(auto s:i.backpack)if(s.type==type)count+=s.count;for(auto&c:i.chests)for(auto s:c.items)if(s.type==type)count+=s.count;for(int s:i.equipment)if(s==int(type))++count;return count;}
uint32_t hash(const std::vector<uint8_t>&b){uint32_t h=2166136261u;for(auto c:b)h=(h^c)*16777619u;return h;}
void put(std::vector<uint8_t>&b,uint32_t n){for(int j=0;j<4;++j)b.push_back(uint8_t(n>>(j*8)));}
std::vector<uint8_t>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void write(const std::string&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());}
}
int main(int argc,char**argv){
    assert(argc==2);std::string path=std::string(argv[1])+"/equipment.sav";
    World w=demoWorld();Object chest;chest.art=0xE42;chest.x=5;chest.y=5;w.objects={chest};w.index();Actor a{4,5,0};
    Inventory inv;inv.init(w,path);assert(inv.chests[0].items.size()==7);inv.open(w,a);
    for(int t:{5,6,7}){size_t pos=0;while(inv.chests[0].items[pos].type!=t)++pos;assert(inv.transfer(w,a,true,pos));}
    for(int t:{5,6,7})assert(total(inv,t)==1);
    unsigned seen=99;inv.prepareEquipment=[&](unsigned mask){seen=mask;return false;};
    assert(!inv.equip(0)&&inv.equipmentMask()==0&&inv.backpack.size()==3&&seen==1);
    inv.prepareEquipment=[&](unsigned mask){seen=mask;return true;};
    for(int t:{5,6,7}){assert(inv.equip(0));assert(total(inv,t)==1);}assert(inv.equipmentMask()==7&&seen==7&&inv.backpack.empty());
    Inventory load;load.init(w,path);assert(load.equipmentMask()==7&&load.backpack.empty());
    for(int t:{5,6,7})assert(total(load,t)==1);
    assert(!inv.unequip(1,fail)&&inv.equipmentMask()==7&&inv.backpack.empty());
    assert(inv.unequip(1)&&inv.equipmentMask()==5&&inv.backpack[0].type==6);
    assert(!inv.equip(0,fail)&&inv.equipmentMask()==5&&inv.backpack[0].type==6);
    assert(inv.equip(0));inv.backpack.assign(BagSlots,{5,1});assert(!inv.unequip(0)&&inv.equipmentMask()==7);
    inv.backpack={{6,1}};unsigned before=total(inv,6);assert(inv.equip(0)&&total(inv,6)==before&&inv.backpack.size()==1);
    inv.backpack={{0,10}};assert(!inv.equip(0)&&inv.backpack[0].count==10);
    inv.backpack.clear();for(int slot=0;slot<3;++slot)assert(inv.unequip(slot));assert(inv.equipmentMask()==0);
    // Convert a valid save to the exact previous UIN1 layout, then migrate.
    auto bytes=read(path);assert(bytes[3]=='2');bytes.resize(bytes.size()-16);bytes[3]='1';put(bytes,hash(bytes));
    std::string legacy=std::string(argv[1])+"/legacy.sav";write(legacy,bytes);
    Inventory old;old.init(w,legacy);assert(old.equipmentMask()==0&&old.backpack.size()==3);
    auto robeTotal=total(old,6);assert(old.equip(1));Inventory migrated;migrated.init(w,legacy);assert(migrated.equipmentMask()==2&&total(migrated,6)==robeTotal);
    // A valid checksum cannot make an item occupy an incompatible layer.
    bytes=read(legacy);bytes[bytes.size()-12]=5;bytes.resize(bytes.size()-4);put(bytes,hash(bytes));write(legacy,bytes);
    Inventory bad;bad.init(w,legacy);assert(bad.backpack.empty()&&bad.equipmentMask()==0&&!bad.equip(0));
    EquipmentArt art;assert(!art.load(legacy));
    std::vector<uint8_t>b={'U','E','Q','1',76,0,104,0};put(b,8*76*104*4);std::vector<uint8_t>pixels(8*76*104*4,0);put(b,hash(pixels));for(int i=0;i<8;++i)put(b,hash(pixels));b.insert(b.end(),pixels.begin(),pixels.end());
    std::string artPath=std::string(argv[1])+"/equipment.bin";write(artPath,b);assert(art.load(artPath)&&art.dolls.size()==pixels.size());
    std::vector<uint8_t>fb(320*240*3+32,0xA5);
    for(unsigned mask=0;mask<8;++mask){for(int slot=0;slot<3;++slot)inv.equipment[slot]=(mask&(1u<<slot))?slot+5:-1;drawJournal(fb.data()+16,w,a,nullptr,2,"",&inv,&art);}
    for(int n=0;n<16;++n)assert(fb[n]==0xA5&&fb[fb.size()-1-n]==0xA5);
    b.back()^=1;write(artPath,b);assert(!art.load(artPath)&&art.dolls.empty());
    assert(!equipmentChecksum(artPath,0));
    std::cout<<"Equipment: conservation, swap, bag capacity, SD rollback, migration, invalid slots, eight paperdolls and asset integrity passed\n";
}
