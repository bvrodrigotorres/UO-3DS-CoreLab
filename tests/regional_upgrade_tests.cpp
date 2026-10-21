// SPDX-License-Identifier: MIT
#include "exploration.hpp"
#include "inventory.hpp"
#include "doors.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
using namespace lab;
int main(int argc,char**argv){if(argc!=4)return 2;
std::string previous=argv[1],current=argv[2],folder=argv[3];World old,w;std::string e;assert(loadWorld(previous+"/scene.bin",old,e));assert(loadWorld(current+"/scene.bin",w,e));Actor a{237,116,17},b;const std::string pos=folder+"/compat16-pos.sav",inv=folder+"/compat16-inv.sav";std::remove(pos.c_str());std::remove(inv.c_str());assert(savePosition(pos,old,a,e));assert(loadPosition(pos,w,b,e));assert(a.x==b.x&&a.y==b.y&&a.z==b.z);Inventory original;original.init(old,inv);original.backpack={{0,123},{5,1}};original.equipment={{-1,6,-1}};original.chests[0].items={{0,432}};assert(original.checkpoint());Inventory restored;restored.init(w,inv);assert(restored.restored&&restored.healthy());assert(restored.backpack[0].count==123&&restored.equipment[1]==6&&restored.chests[0].items[0].count==432);assert(restored.chests.size()==original.chests.size());Doors doors;assert(doors.load(current+"/doors.bin",current+"/scene.bin",w,e));assert(doors.count()==158);const auto closed=w.objects;unsigned originals=0;
std::ifstream file(current+"/doors.bin",std::ios::binary);std::vector<unsigned char>raw((std::istreambuf_iterator<char>(file)),{});
for(size_t offset=16;offset<raw.size();offset+=12){size_t i=unsigned(raw[offset])|unsigned(raw[offset+1])<<8|unsigned(raw[offset+2])<<16|unsigned(raw[offset+3])<<24;auto d=closed[i];
 World test=w;Doors active;assert(active.load(current+"/doors.bin",current+"/scene.bin",test,e));if(!active.tryOpen({d.x,d.y-1,d.z},0,1)){std::cerr<<"not interactive index "<<i<<" art "<<d.art<<" xyz "<<d.x<<","<<d.y<<","<<d.z<<"\n";continue;}assert(active.update(test,.1f));assert(test.objects[i].art==d.art+1);for(auto index:test.cells[d.y*test.width+d.x]){const auto&o=test.objects[index];assert(!(o.flags&Door)||o.z!=d.z||o.art!=d.art);}assert(active.update(test,3));assert(test.objects[i].art==d.art);++originals;
}std::remove(pos.c_str());std::remove(inv.c_str());assert(originals==158);std::cout<<"v0.15 position, inventory, equipment and chest loot restored; "<<originals<<" door leaves open and close without a duplicate closed leaf\n";}
