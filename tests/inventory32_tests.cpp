// SPDX-License-Identifier: MIT
#include "inventory_drag.hpp"
#include "combat.hpp"
#include "hud.hpp"
#include "commerce.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int fail(const char*,const char*){return -1;}
bool same(const Bag&a,const Bag&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].type!=b[i].type||a[i].count!=b[i].count)return false;return true;}
int main(int argc,char**argv){
 assert(argc>=2);std::string root=std::string(argv[1])+"/";World w;w.width=w.height=16;w.land.resize(256);w.vertices.resize(289);Object chest;chest.art=0xe42;chest.x=6;chest.y=5;w.objects.push_back(chest);w.index();Actor a{5,5,0};Inventory inv;inv.init(w,root+"items.sav");inv.backpack={{0,100},{5,1},{4,1}};inv.chests[0].items.clear();inv.followChest(0);assert(inv.checkpoint());
 InventoryDrag drag;assert(drag.begin(inv,20,40));drag.move(180,50);assert(drag.finish(w,a,inv)&&inv.chests[0].items[0].count==100&&inv.backpack.size()==2);
 assert(drag.begin(inv,180,40));drag.move(20,50);assert(drag.finish(w,a,inv)&&inv.chests[0].items.empty()&&inv.backpack.back().count==100);
 Bag body;assert(inv.openExternal(w,a,body,a,"a rat"));assert(inv.transfer(w,a,false,0)&&body[0].type==5);assert(inv.transfer(w,a,true,0)&&body.empty()); // ordinary two-way corpse transfer
 assert(drag.begin(inv,20,40));auto before=inv.backpack;drag.move(120,215);assert(drag.finish(w,a,inv,fail)&&same(inv.backpack,before)&&inv.nearestGround(w,a)<0);
 assert(drag.begin(inv,20,40));drag.move(125,220);assert(drag.finish(w,a,inv));int index=inv.nearestGround(w,a);assert(index>=0&&inv.ground[index].items.size()==1&&inv.ground[index].remaining[0]==180);assert(inv.followGround(index)&&inv.containerName()=="SUELO");
 auto oldGround=inv.ground[index].items;before=inv.backpack;assert(!inv.takeAll(w,a,fail)&&same(inv.ground[index].items,oldGround)&&same(inv.backpack,before));
 Inventory resumed;resumed.init(w,root+"items.sav");assert(resumed.healthy()&&resumed.restored&&resumed.nearestGround(w,a)>=0);int floor=resumed.nearestGround(w,a);resumed.tickGround(120);assert(resumed.ground[floor].remaining[0]==60);
 resumed.backpack.push_back({3,2});assert(resumed.drop(w,a,false,resumed.backpack.size()-1));assert(resumed.ground[floor].remaining.size()==2&&resumed.ground[floor].remaining[0]==60&&resumed.ground[floor].remaining[1]==180);resumed.tickGround(0);assert(resumed.ground[floor].remaining[0]==60);resumed.tickGround(60.1f);assert(resumed.ground[floor].items.size()==1&&resumed.ground[floor].items[0].type==3);
 Inventory continued;continued.init(w,root+"items.sav");assert(continued.restored&&continued.ground[0].items.size()==1&&continued.ground[0].remaining[0]>119&&continued.ground[0].remaining[0]<120);
 continued.followGround(0);assert(continued.takeAll(w,a)&&continued.ground[0].items.empty());Inventory picked;picked.init(w,root+"items.sav");assert(picked.restored&&picked.nearestGround(w,a)<0);
 // Failed expiry preserves recoverable objects, without retrying disk every frame.
 inv.backpack={{7,1}};assert(inv.drop(w,a,false,0));index=inv.nearestGround(w,a);inv.tickGround(181,fail);assert(!inv.ground[index].items.empty());inv.tickGround(.1f,fail);assert(!inv.ground[index].items.empty());inv.tickGround(1);assert(inv.ground[index].items.empty());
 // Moving or swapping the source container cancels an in-progress drag.
 inv.backpack={{5,1}};inv.followChest(0);assert(drag.begin(inv,20,40));inv.openExternal(w,a,body,a,"BODY");drag.move(180,40);assert(drag.finish(w,a,inv)&&inv.backpack.size()==1&&body.empty());
 assert(drag.begin(inv,20,40));drag.move(25,43);assert(!drag.finish(w,a,inv)&&inv.backpack.size()==1); // tap remains a tap
 assert(drag.begin(inv,20,40));inv.backpack.clear();drag.move(125,220);assert(drag.finish(w,a,inv)&&inv.nearestGround(w,a)<0);
 inv.backpack={{5,1}};inv.followChest(0);assert(drag.begin(inv,20,40));drag.move(310,200);assert(drag.finish(w,a,inv)&&inv.backpack.size()==1);
 // Reach must be rechecked at release; no drops/deposits through distant containers.
 inv.chests[0].items={{5,1}};inv.followChest(0);assert(drag.begin(inv,180,40));drag.move(20,40);assert(drag.finish(w,{1,1,0},inv)&&inv.chests[0].items.size()==1);
 Inventory full;full.init(w,root+"full.sav");full.backpack={{5,1}};for(size_t i=0;i<full.ground.size();++i){auto&p=full.ground[i];p.position={int(i%16),int(i/16),0};p.items.assign(ChestSlots,{4,1});p.remaining.assign(ChestSlots,180);}assert(!full.drop(w,a,false,0)&&full.backpack.size()==1); // no silent eviction
 assert(characterPaperdollHit(250,60)&&!characterPaperdollHit(170,60));
 // Ground shares normal nearest-container selection, with a different floor excluded.
 inv.backpack={{4,1}};assert(inv.drop(w,a,false,0));Combat combat;assert(combat.inspectNearest(w,a,inv)&&inv.containerName()=="SUELO");assert(inv.nearestGround(w,{5,5,40})<0);
 inv.followGround(inv.nearestGround(w,a));assert(drag.begin(inv,180,40));drag.move(125,220);assert(drag.finish(w,a,inv)&&!inv.ground[inv.groundIndex()].items.empty()); // cannot reset its lifetime by dropping it again
 std::vector<uint8_t>fb(320*240*3+32,0xa5);drawInventory(fb.data()+16,w,a,inv,nullptr,&drag);Commerce shop;drawCommerce(fb.data()+16,inv,shop);for(int i=0;i<16;++i)assert(fb[i]==0xa5&&fb[fb.size()-1-i]==0xa5);
 std::cout<<"Inventory 32: bidirectional corpses, reserved item gestures, drag/cancel/stale/reach checks, atomic floor drop/pickup/UIN7 reload, per-item 180s expiry and SD rollback passed\n";
}
