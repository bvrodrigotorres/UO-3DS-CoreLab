// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace lab;
static int fail(const char*,const char*){return -1;}
int main(int argc,char**argv){
 assert(argc==2);World w;w.width=w.height=16;w.land.resize(256);w.vertices.resize(289);w.index();Actor player{8,8,0};Npcs quiet;Inventory bag;bag.init(w,std::string(argv[1])+"/bag.sav");bag.backpack={{3,30},{4,2}};PlayerStatus s;Combat c;
 for(unsigned skill:{0u,500u,1000u}){
  c.reset(quiet);s.skills.ready=true;s.skills.entries[unsigned(Skill::Healing)].value=skill;s.skills.entries[unsigned(Skill::Healing)].lock=SkillLock::Locked;s.hits=1;
  float duration=3-1.5f*skill/1000;int amount=15+skill/100;unsigned count=bag.backpack[0].count;
  assert(c.useHealing(bag,0,s)&&bag.backpack[0].count==count-1&&s.hits==1&&bag.message=="*aplicando vendas*");
  assert(std::abs(c.bandageWait()-duration)<.0001f);for(const auto&e:c.damage)assert(!e.active);
  assert(!c.useHealing(bag,0,s)&&bag.backpack[0].count==count-1);assert(!c.useHealing(bag,1,s));
  c.update(w,quiet,player,true,0,s,bag);assert(c.bandageWait()==duration&&s.hits==1);
  c.update(w,quiet,player,true,duration-.01f,s,bag);assert(s.hits==1&&c.bandageWait()>0&&c.healingRevision()==0);
  player.x++;c.update(w,quiet,player,true,.02f,s,bag);assert(s.hits==1+amount&&c.bandageWait()==0&&c.healingRevision()==1);
  unsigned labels=0;for(const auto&e:c.damage)if(e.active){++labels;assert(e.kind==CombatText::Healing&&e.amount==amount&&e.position.x==player.x&&e.age==0);}assert(labels==1);
 }
 // Gain only at completion; clamp the actual visual to missing HP.
 c.reset(quiet);s.skills={};s.skills.start(5);s.skills.entries[unsigned(Skill::Healing)].checks=40;s.hits=47;
 assert(c.useHealing(bag,0,s)&&s.skills.value(Skill::Healing)==0);c.update(w,quiet,player,false,3,s,bag);assert(s.hits==50&&s.skills.value(Skill::Healing)==1);
 for(const auto&e:c.damage)if(e.active)assert(e.amount==3);
 // Failed storage consumes nothing, starts no timer. Reset cancels the pending heal.
 c.reset(quiet);s.hits=1;unsigned count=bag.backpack[0].count;assert(!c.useHealing(bag,0,s,fail)&&bag.backpack[0].count==count&&c.bandageWait()==0);
 assert(c.useHealing(bag,0,s));c.reset(quiet);c.update(w,quiet,player,false,1,s,bag);assert(s.hits==1&&c.bandageWait()==0);
 // Death cancels the bandage; it cannot heal the respawned character later.
 Npcs enemy;enemy.combatants().resize(1);auto&n=enemy.combatants()[0];n.animal=n.hostile=n.stationary=true;n.position=n.from=n.home={player.x+1,player.y,0};n.maximumHits=1000;n.meleeMin=n.meleeMax=1000;n.name="a troll";
 c.reset(enemy);s.skills={};s.hits=1;assert(c.useHealing(bag,0,s));assert(c.update(w,enemy,player,false,1,s,bag));assert(c.bandageWait()==0&&c.healingRevision()==0);
 std::cout<<"Bandages: 3/2.25/1.5 seconds, 15/20/25 HP, literal emote, no early heal/gain, moving/pause, no double use, actual cap, SD rollback and death/reset cancellation passed\n";
}
