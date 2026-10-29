// SPDX-License-Identifier: MIT
#include "commerce.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
static int fail551(const char*,const char*){return -1;}
static std::vector<uint8_t>wire(const Skills&s){std::vector<uint8_t>b;s.encode(b);return b;}
static Skills full(){Skills s;s.ready=true;for(unsigned i=0;i<7;++i)s.entries[i].value=1000;return s;}
int main(int argc,char**argv){
 assert(argc==2);std::string path=std::string(argv[1])+"/training551.sav";World w;w.width=w.height=16;w.land.resize(256);w.vertices.resize(289);w.index();Actor a{5,5,0};Npcs n;Npc healer;healer.name="A healer";healer.title="the healer";healer.position=healer.from=healer.home={6,5,0};n.combatants().push_back(healer);Commerce shop;assert(shop.update(w,n,a));Inventory inv;inv.init(w,path);inv.backpack={{0,1000}};PlayerStatus status;status.skills=full();TradeMenu menu;menu.switchTab(2,shop,inv);assert(menu.rows[0]==unsigned(Skill::Healing));
 auto original=wire(status.skills);assert(!menu.quote(shop,inv,status)&&menu.message.find("BAJAR")!=std::string::npos&&wire(status.skills)==original);
 status.skills.entries[0].lock=SkillLock::Down;status.skills.entries[2].lock=SkillLock::Down;status.skills.entries[1].lock=SkillLock::Locked;status.skills.entries[unsigned(Skill::Healing)].checks=5;status.skills.entries[unsigned(Skill::Healing)].wait=4;
 original=wire(status.skills);assert(status.skills.trainingRoom(Skill::Healing)==300);assert(menu.quote(shop,inv,status)&&menu.maximum==300&&menu.quantity==300);assert(wire(status.skills)==original);assert(!menu.back()&&wire(status.skills)==original&&PlayerStatus::gold(&inv)==1000);
 assert(menu.quote(shop,inv,status));assert(!menu.execute(shop,inv,status,fail551)&&PlayerStatus::gold(&inv)==1000&&wire(status.skills)==original&&status.skills.entries[unsigned(Skill::Healing)].wait==4&&inv.trainingSerial==0);
 assert(menu.execute(shop,inv,status)&&status.skills.total()==7000&&status.skills.value(Skill::Healing)==300&&PlayerStatus::gold(&inv)==700);
 assert(status.skills.entries[0].value==850&&status.skills.entries[2].value==850&&status.skills.entries[1].value==1000&&status.skills.entries[3].value==1000);
 assert(status.skills.entries[0].lock==SkillLock::Down&&status.skills.entries[1].lock==SkillLock::Locked&&status.skills.entries[unsigned(Skill::Healing)].lock==SkillLock::Up);
 assert(status.skills.entries[unsigned(Skill::Healing)].checks==0&&status.skills.entries[unsigned(Skill::Healing)].wait==0);
 assert(!menu.quote(shop,inv,status)); // 30.0 cannot be exceeded, even with donors and money.
 Inventory reloaded;reloaded.init(w,path);assert(reloaded.healthy()&&reloaded.restored&&reloaded.trainingSerial==1&&PlayerStatus::gold(&reloaded)==700);PlayerStatus crashed;crashed.skills=full();assert(crashed.recoverTraining(reloaded)&&wire(crashed.skills)==wire(status.skills)&&crashed.skills.total()==7000);assert(!crashed.recoverTraining(reloaded));
 // Use free points first, and release only the remaining required tenths.
 status.skills=full();status.skills.entries[0].value=980;status.skills.entries[0].lock=SkillLock::Down;inv.backpack={{0,1000}};assert(menu.quote(shop,inv,status));menu.quantity=70;assert(menu.execute(shop,inv,status)&&status.skills.total()==7000&&status.skills.value(Skill::Healing)==70&&status.skills.entries[0].value==930&&PlayerStatus::gold(&inv)==930);
 // Limited donors and limited gold both produce affordable partial lessons.
 status.skills=full();status.skills.entries[0].value=100;status.skills.entries[0].lock=SkillLock::Down;status.skills.entries[7].value=850;status.skills.entries[8].value=50;status.skills.entries[8].lock=SkillLock::Down;assert(status.skills.total()==7000);inv.backpack={{0,1000}};assert(menu.quote(shop,inv,status)&&menu.maximum==150);menu.back();inv.backpack={{0,50}};assert(menu.quote(shop,inv,status)&&menu.maximum==50);assert(menu.execute(shop,inv,status)&&status.skills.total()==7000&&status.skills.value(Skill::Healing)==50&&status.skills.entries[0].value==50&&status.skills.entries[8].value==50&&PlayerStatus::gold(&inv)==0);
 // Spend all available Down points across several small donor skills.
 status.skills=full();status.skills.entries[0].value=125;status.skills.entries[0].lock=SkillLock::Down;status.skills.entries[2].value=175;status.skills.entries[2].lock=SkillLock::Down;status.skills.entries[7].value=status.skills.entries[8].value=850;inv.backpack={{0,1000}};assert(menu.quote(shop,inv,status)&&menu.quantity==300);assert(menu.execute(shop,inv,status)&&status.skills.total()==7000&&status.skills.entries[0].value==0&&status.skills.entries[2].value==0);
 // Confirmation must recheck changed locks before touching money or live stats.
 status.skills=full();status.skills.entries[0].lock=SkillLock::Down;assert(menu.quote(shop,inv,status));status.skills.entries[0].lock=SkillLock::Locked;original=wire(status.skills);auto gold=PlayerStatus::gold(&inv);assert(!menu.execute(shop,inv,status)&&wire(status.skills)==original&&PlayerStatus::gold(&inv)==gold);
 status.skills.entries[unsigned(Skill::Healing)].lock=SkillLock::Down;assert(!menu.quote(shop,inv,status));status.skills.entries[unsigned(Skill::Healing)].lock=SkillLock::Locked;assert(!menu.quote(shop,inv,status));
 Skills below;below.start(5);below.entries[0].value=100;below.entries[0].lock=SkillLock::Down;auto sum=below.total();assert(below.train(Skill::Healing,300)&&below.entries[0].value==100&&below.total()==sum+300);
 Skills invalid=full();invalid.entries[0].lock=SkillLock::Down;original=wire(invalid);assert(!invalid.train(Skill::Healing,301)&&!invalid.train(Skill::Healing,0)&&!invalid.train(Skill::Count,1)&&wire(invalid)==original);
 std::cout<<"Training551: 700-cap Down donors, free space first, partial donors/gold, multi-skill depletion, Up/Locked protection, cancel/stale locks/SD rollback, exact gold and durable gain+loss receipt passed\n";
}
