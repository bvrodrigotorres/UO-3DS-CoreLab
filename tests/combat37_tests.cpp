// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
static int fail(const char*,const char*){return -1;}
int main(int argc,char**argv){
 assert(argc==2);World w;w.width=w.height=24;w.land.resize(576);w.vertices.resize(625);w.index();Actor player{8,8,0};Inventory inv;inv.init(w,std::string(argv[1])+"/bag.sav");Npcs npcs;npcs.combatants().resize(1);auto&n=npcs.combatants()[0];n.animal=n.hostile=n.stationary=true;n.position=n.from=n.home={9,8,0};n.maximumHits=10000;n.meleeMax=0;n.name="a troll";
 PlayerStatus stats;stats.skills.ready=true;Combat fight;fight.reset(npcs);fight.toggle(npcs,player);bool outgoing=false;
 for(int i=0;i<200&&!outgoing;++i){stats.stamina=50;fight.update(w,npcs,player,false,.05f,stats,inv);for(auto&e:fight.damage)if(e.active&&e.kind==CombatText::Miss&&!e.player){outgoing=true;assert(e.npc==0&&e.amount==0&&combatText(e)=="MISS");auto color=combatTextColor(e);assert(color[0]==1&&color[1]==1&&color[2]==0);}}
 assert(outgoing);
 // An enemy's failed attack creates a red label above the player, with no HP loss.
 stats.skills.entries[unsigned(Skill::Wrestling)].value=1000;stats.hits=50;n.meleeMin=n.meleeMax=1;fight.reset(npcs);bool incoming=false;
 for(int i=0;i<200&&!incoming;++i){stats.hits=50;stats.stamina=50;fight.update(w,npcs,player,false,.05f,stats,inv);for(auto&e:fight.damage)if(e.active&&e.age==0&&e.kind==CombatText::Miss&&e.player){incoming=true;assert(stats.hits==50&&e.npc==-1&&e.position.x==8&&e.position.y==8);auto color=combatTextColor(e);assert(color[0]==1&&color[1]<.3f&&color[2]<.3f);}}
 assert(incoming);
 Npcs quiet;Combat healing;healing.reset(quiet);healing.locate(player);inv.backpack={{4,2},{3,3},{1,3},{2,3}};PlayerStatus s;
 for(int type:{4,3,1,2}){healing.reset(quiet);healing.locate(player);s.hits=47;size_t index=0;while(inv.backpack[index].type!=type)++index;assert(healing.useHealing(inv,index,s));if(type==3){assert(s.hits==47);healing.update(w,quiet,player,false,3,s,inv);}assert(s.hits==50);unsigned labels=0;for(auto&e:healing.damage)if(e.active){++labels;assert(e.kind==CombatText::Healing&&e.amount==3&&e.player&&e.npc==-1&&e.position.x==8&&combatText(e)=="+3");auto color=combatTextColor(e);assert(color[0]<.5f&&color[1]>.8f&&color[2]==1);}assert(labels==1);for(auto&p:healing.blood)assert(!p.active);
 healing.update(w,quiet,player,false,0,s,inv);for(auto&e:healing.damage)if(e.active)assert(e.age==0);assert(!healing.useHealing(inv,0,s));healing.update(w,quiet,player,false,1.3f,s,inv);for(auto&e:healing.damage)assert(!e.active);}
 healing.reset(quiet);healing.locate(player);s.hits=1;assert(!healing.useHealing(inv,0,s,fail)&&s.hits==1);for(auto&e:healing.damage)assert(!e.active);
 // Repeating the saved healing shortcut also emits the actual restored amount.
 s.lastHealing=3;assert(healing.repeatHealing(inv,s));healing.update(w,quiet,player,false,3,s,inv);bool shortcut=false;for(auto&e:healing.damage)if(e.active){shortcut=true;assert(e.kind==CombatText::Healing&&e.amount==15);}assert(shortcut);
 healing.reset(quiet);for(auto&e:healing.damage)assert(!e.active);
 std::cout<<"Combat 37: real player/enemy misses, yellow/red labels, four healing types/actual cap, cyan shortcut feedback, no failed-use label, pause/expiry and no healing blood passed\n";
}
