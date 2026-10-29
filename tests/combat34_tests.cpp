// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
int fail(const char*,const char*){return -1;}
int main(int argc,char**argv){
 assert(argc>=2);std::string root=std::string(argv[1])+"/";World w;w.width=w.height=32;w.land.resize(1024);w.vertices.resize(1089);w.index();Doors doors;Npcs npcs;npcs.combatants().resize(3);
 for(int i=0;i<3;++i){auto&n=npcs.combatants()[i];n.animal=n.aggressive=n.stationary=true;n.maximumHits=100;n.meleeMin=n.meleeMax=1;n.name="enemy "+std::to_string(i);n.position=n.from=n.home={12+i*2,8,0};}npcs.combatants()[2].position=npcs.combatants()[2].from=npcs.combatants()[2].home={8,8,0};
 Actor player{5,8,0};npcs.update(w,player,player,false,.05f,doors);assert(npcs.all()[2].hostile&&!npcs.all()[0].hostile);auto order=npcs.all()[2].aggressionOrder;
 player={6,8,0};npcs.update(w,player,player,false,.05f,doors);assert(npcs.all()[0].hostile&&order<npcs.all()[0].aggressionOrder);
 Combat combat;combat.reset(npcs);Inventory inv;inv.init(w,root+"combat.sav");PlayerStatus status;combat.update(w,npcs,player,false,.01f,status,inv);assert(combat.attacking&&combat.target==2); // sight order, not numeric index
 player={7,8,0};int hp=combat.vitals()[2].hits;combat.update(w,npcs,player,false,.01f,status,inv);assert(combat.vitals()[2].hits<hp&&combat.target==2&&combat.striking());
 npcs.combatants()[0].position=npcs.combatants()[0].from={7,7,0};combat.update(w,npcs,player,false,1.3f,status,inv);assert(combat.target==2); // second hitter cannot steal focus
 npcs.combatants()[2].alive=false;combat.update(w,npcs,player,false,.01f,status,inv);assert(combat.target==0&&combat.attacking);
 npcs.combatants()[0].hostile=false;npcs.combatants()[0].aggressive=false;npcs.combatants()[1].hostile=false;combat.update(w,npcs,player,false,.01f,status,inv);assert(!combat.attacking&&combat.target==-1); // passive bystander is not next
 // A kill immediately switches to an aggressor still pursuing; no attack on a neutral animal.
 npcs.combatants()[2].alive=true;npcs.combatants()[2].maximumHits=1;npcs.combatants()[2].hostile=false;npcs.engage(2);npcs.engage(0);combat.reset(npcs);combat.update(w,npcs,player,false,.01f,status,inv);assert(!npcs.all()[2].alive&&combat.target==0&&combat.attacking);
 // Leaving range drops automatic focus. Explicit manual stop lasts until another hit.
 npcs.combatants()[0].position={25,25,0};combat.update(w,npcs,player,false,.01f,status,inv);assert(combat.target==-1&&!combat.attacking);
 npcs.combatants()[0].position=npcs.combatants()[0].from={7,7,0};combat.update(w,npcs,player,false,.01f,status,inv);assert(combat.attacking);combat.toggle(npcs,player);combat.update(w,npcs,player,false,.01f,status,inv);assert(!combat.attacking);combat.update(w,npcs,player,false,1.3f,status,inv);assert(combat.attacking&&combat.target==0);
 Npcs quiet;Combat healing;healing.reset(quiet);Inventory bag;bag.init(w,root+"healing.sav");bag.backpack={{4,1},{3,3},{1,3},{2,3},{5,1}};assert(bag.checkpoint());PlayerStatus stats;
 auto find=[&](int type){for(size_t i=0;i<bag.backpack.size();++i)if(bag.backpack[i].type==type)return i;return bag.backpack.size();};
 for(int type:{4,3,1,2}){
 healing.update(w,quiet,player,false,3.1f,stats,bag);stats.hits=1;size_t index=find(type);auto count=bag.backpack[index].count;
 assert(healing.useHealing(bag,index,stats)&&stats.hits==(type==3?1:1+itemHealing(type))&&healing.healingWait()==3);
 if(count>1)assert(bag.backpack[find(type)].count==count-1);else assert(find(type)==bag.backpack.size());
 auto hpNow=stats.hits;size_t size=bag.backpack.size();assert(!healing.useHealing(bag,find(3),stats)&&stats.hits==hpNow&&bag.backpack.size()==size);
 healing.update(w,quiet,player,false,0,stats,bag);assert(healing.healingWait()==3);
 if(type==3){healing.update(w,quiet,player,false,3,stats,bag);assert(stats.hits==16);}
 }
 healing.update(w,quiet,player,false,3.1f,stats,bag);stats.hits=stats.hitsMax;auto count=bag.backpack[find(3)].count;assert(!healing.useHealing(bag,find(3),stats)&&bag.backpack[find(3)].count==count);
 stats.hits=49;assert(healing.useHealing(bag,find(3),stats)&&stats.hits==49);healing.update(w,quiet,player,false,3,stats,bag);assert(stats.hits==50);healing.update(w,quiet,player,false,3.1f,stats,bag);stats.hits=10;count=bag.backpack[find(3)].count;assert(!healing.useHealing(bag,find(3),stats,fail)&&stats.hits==10&&bag.backpack[find(3)].count==count&&healing.healingWait()==0);assert(!healing.useHealing(bag,find(5),stats)&&stats.hits==10);
 Inventory resumed;resumed.init(w,root+"healing.sav");assert(resumed.restored&&resumed.backpack.size()==bag.backpack.size());for(size_t i=0;i<bag.backpack.size();++i)assert(resumed.backpack[i].type==bag.backpack[i].type&&resumed.backpack[i].count==bag.backpack[i].count);
 bag.equipMode=bag.useMode=false;bag.tap(w,player,25,220);assert(bag.equipMode&&!bag.useMode);bag.tap(w,player,25,220);assert(!bag.equipMode&&bag.useMode);bool invoked=false;bag.useItem=[&](size_t i){invoked=true;return healing.useHealing(bag,i,stats);};bag.tap(w,player,20,40);assert(invoked);bag.tap(w,player,25,220);assert(!bag.equipMode&&!bag.useMode);
 std::cout<<"Combat 34: sight-order automatic retaliation, sticky first target, active-aggressor-only chaining, range disengage, ranked healing, cooldown/pause, cap/no waste, exact consumption/reload and SD rollback passed\n";
}
