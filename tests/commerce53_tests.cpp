// SPDX-License-Identifier: MIT
#include "commerce.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int failTraining(const char*,const char*){return -1;}
void image(const std::string&path,const std::vector<uint8_t>&fb,int width){std::ofstream out(path,std::ios::binary);out<<"P6\n"<<width<<" 240\n255\n";for(int y=0;y<240;++y)for(int x=0;x<width;++x){auto p=fb.data()+16+3*(x*240+239-y);char rgb[]={char(p[2]),char(p[1]),char(p[0])};out.write(rgb,3);}}
int main(int argc,char**argv){
 assert(argc==2);std::string root=std::string(argv[1])+"/";World w;w.width=w.height=16;w.vertices.assign(289,0);w.land.resize(256);w.index();Actor actor{5,5,0};
 Inventory bag;bag.init(w,root+"merchant.sav");bag.backpack={{0,1000}};
 assert(bag.tradeQuantity(true,3,8,50)&&PlayerStatus::gold(&bag)==600&&bag.backpack[1].count==50);
 assert(bag.tradeQuantity(false,3,4,20)&&PlayerStatus::gold(&bag)==680&&bag.backpack[1].count==30);
 assert(!bag.tradeQuantity(true,3,8,10,failTraining)&&PlayerStatus::gold(&bag)==680&&bag.backpack[1].count==30);
 assert(!bag.tradeQuantity(true,3,8,65535)&&!bag.tradeQuantity(true,3,8,0)&&!bag.tradeQuantity(true,5,35,2));
 bag.backpack={{0,65535},{3,65530}};assert(!bag.tradeQuantity(true,3,1,6)&&bag.backpack[1].count==65530);
 bag.backpack.assign(24,{4,1});bag.backpack[0]={0,80};assert(bag.tradeQuantity(true,3,8,10)&&bag.backpack.size()==24&&PlayerStatus::gold(&bag)==0);
 assert(bag.tradeQuantity(false,3,4,10)&&PlayerStatus::gold(&bag)==40&&bag.backpack.size()==24);
 bag.backpack.assign(24,{4,1});bag.backpack[0]={0,81};assert(!bag.tradeQuantity(true,3,8,10)&&PlayerStatus::gold(&bag)==81);
 bag.backpack={{0,1000}};Npcs npcs;Npc healer;healer.name="Anastasia";healer.title="the healer";healer.position=healer.from=healer.home={6,5,0};npcs.combatants().push_back(healer);
 Commerce shop;assert(shop.update(w,npcs,actor)&&shop.available()&&!shop.lessons.empty());
 PlayerStatus status;status.skills.start(5);TradeMenu menu;menu.refresh(shop,bag);menu.row=0;assert(menu.quote(shop,bag,status)&&menu.confirming&&menu.quantity==1&&menu.maximum==125);menu.changeQuantity(9);assert(menu.quantity==10);
 assert(menu.execute(shop,bag,status)&&PlayerStatus::gold(&bag)==920&&bag.backpack[1].count==10);assert(menu.quote(shop,bag,status));assert(!menu.back()&&!menu.confirming&&menu.back());
 menu.switchTab(2,shop,bag);assert(menu.tab==2&&menu.rows[0]==unsigned(Skill::Healing));assert(menu.quote(shop,bag,status)&&menu.quantity==300&&menu.maximum==300);menu.quantity=150;
 assert(!menu.execute(shop,bag,status,failTraining)&&status.skills.value(Skill::Healing)==0&&PlayerStatus::gold(&bag)==920&&bag.trainingSerial==0);
 assert(menu.execute(shop,bag,status)&&status.skills.value(Skill::Healing)==150&&PlayerStatus::gold(&bag)==770&&bag.trainingSerial==1&&status.trainingSerial==1);
 // A crash before the separate vitals write recovers every paid lesson from the gold receipt.
 Inventory recovered;recovered.init(w,root+"merchant.sav");assert(recovered.healthy()&&recovered.restored&&recovered.trainingSerial==1&&PlayerStatus::gold(&recovered)==770);
 PlayerStatus crashed;crashed.skills.start(5);assert(crashed.recoverTraining(recovered)&&crashed.skills.value(Skill::Healing)==150);assert(!crashed.recoverTraining(recovered));
 assert(menu.quote(shop,bag,status)&&menu.quantity==150);assert(menu.execute(shop,bag,status)&&status.skills.value(Skill::Healing)==300&&bag.trainingSerial==2&&PlayerStatus::gold(&bag)==620);assert(!menu.quote(shop,bag,status));
 menu.row=1;assert(menu.quote(shop,bag,status)&&menu.quantity==50);status.skills.entries[unsigned(Skill::Anatomy)].lock=SkillLock::Locked;assert(!menu.quote(shop,bag,status));status.skills.entries[unsigned(Skill::Anatomy)].lock=SkillLock::Up;
 assert(menu.quote(shop,bag,status)&&menu.execute(shop,bag,status)&&status.skills.value(Skill::Anatomy)==300&&bag.trainingSerial==3);
 recovered.init(w,root+"merchant.sav");crashed=PlayerStatus{};assert(crashed.recoverTraining(recovered)&&crashed.skills.value(Skill::Healing)==300&&crashed.skills.value(Skill::Anatomy)==300);
 assert(saveVitals(root+"merchant-vitals.sav",status));PlayerStatus loaded;assert(loadVitals(root+"merchant-vitals.sav",loaded)&&loaded.trainingSerial==3&&loaded.skills.value(Skill::Healing)==300);
 loaded.skills.entries[unsigned(Skill::Healing)].value=299;assert(!loaded.recoverTraining(recovered)&&loaded.skills.value(Skill::Healing)==299); // receipt never overwrites later practice/decay
 Inventory destination;destination.init(w,root+"merchant-other.sav");destination.copyPlayerFrom(recovered);assert(destination.checkpoint());Inventory arrived;arrived.init(w,root+"merchant-other.sav");assert(arrived.healthy()&&arrived.trainingSerial==3);
 menu.row=2;bag.backpack={{0,9}};assert(menu.quote(shop,bag,status)&&menu.maximum==9);menu.quantity=9;assert(menu.execute(shop,bag,status)&&status.skills.value(Skill::Forensics)==9&&PlayerStatus::gold(&bag)==0);
 bag.backpack={{0,1000}};auto backup=status.skills;for(auto&s:status.skills.entries)s.value=0;for(unsigned i=0;i<7;++i)status.skills.entries[i].value=1000;menu.row=0;assert(!menu.quote(shop,bag,status));status.skills=backup;
 npcs.combatants()[0].title="the blacksmith";assert(shop.update(w,npcs,actor));assert(shop.lessons.size()==2&&shop.lessons[0]==Skill::Blacksmith);
 npcs.combatants()[0].title="the armourer";assert(shop.update(w,npcs,actor)&&shop.accepts(21)&&shop.lessons[0]==Skill::Blacksmith);
 npcs.combatants()[0].title="the vet";assert(shop.update(w,npcs,actor)&&shop.lessons.size()==2&&shop.lessons[1]==Skill::Veterinary);
 npcs.combatants()[0].title="the warrior";assert(shop.update(w,npcs,actor)&&shop.stock.empty()&&!shop.lessons.empty());
 npcs.combatants()[0].animal=true;assert(shop.update(w,npcs,actor)&&!shop.available());npcs.combatants()[0].animal=false;npcs.combatants()[0].title="the healer";shop.update(w,npcs,actor);
 // Guard the different framebuffer widths, then render both screens for visual review.
 std::vector<uint8_t>top(400*240*3+32,0xa5),bottom(320*240*3+32,0xa5);menu=TradeMenu{};menu.switchTab(2,shop,bag);menu.row=2;assert(menu.quote(shop,bag,status));
 drawTradeMenu(top.data()+16,bottom.data()+16,bag,shop,menu,status);
 for(int i=0;i<16;++i)assert(top[i]==0xa5&&top[top.size()-1-i]==0xa5&&bottom[i]==0xa5&&bottom[bottom.size()-1-i]==0xa5);
 image(root+"trade-top.ppm",top,400);image(root+"trade-bottom.ppm",bottom,320);
 drawCommerce(bottom.data()+16,bag,shop);image(root+"trade-preview.ppm",bottom,320);
 // A cancelled confirmation never changes balances; stale/distant sellers cannot be used.
 menu.back();auto gold=PlayerStatus::gold(&bag);assert(!menu.execute(shop,bag,status)&&gold==PlayerStatus::gold(&bag));npcs.combatants()[0].position={12,12,0};shop.update(w,npcs,actor);assert(!menu.quote(shop,bag,status));
 std::cout<<"Commerce53: bulk/caps/full bags/rollback; trainers, cost, locks, cap30, total700; durable receipt recovery and zone copies; UVT5/UINB; UI bounds passed\n";
}
