// SPDX-License-Identifier: MIT
#include "world_expedition.hpp"
#include "combat.hpp"
#include "resource_preparation.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
static World flat(int ox=1000,int oy=2000){World w;w.width=w.height=64;w.originX=ox;w.originY=oy;w.land.resize(4096);w.vertices.resize(65*65);w.index();return w;}
static void settle(Npcs&n,const World&w,const Actor&a){for(int i=0;i<30;++i)n.prepareWindow(w,a);}
int main(int argc,char**argv){assert(argc==2);std::string root=std::string(argv[1])+"/";auto w=flat();Actor player{30,30,0};
 Npcs source;
 for(int i=0;i<100;++i){Npc n;n.position=n.from=n.home={12+i%10,12+i/10,0};n.name="a test creature "+std::to_string(i);n.animal=true;n.maximumHits=1;n.meleeMax=0;source.combatants().push_back(n);}
 Npcs trial;trial.importWindowSpawns(source,w.originX,w.originY);trial.shiftWindow(-w.originX,-w.originY);settle(trial,w,player);
 assert(trial.all().size()==100&&trial.activeCount()==64);assert(source.all()[0].position.x==12&&source.all()[0].alive);
 unsigned active=0;for(size_t i=0;i<trial.all().size();++i)active+=trial.simulates(i);assert(active==64);
 // Changing only the view origin preserves stable IDs, interpolation, homes,
 // aggression order and target HP rather than spawning a fresh population.
 Combat combat;combat.reset(trial);combat.target=99;
 auto original=trial.all()[99].position;trial.combatants()[99].hostile=true;trial.engage(99);auto order=trial.all()[99].aggressionOrder;
 combat.corpses[0].active=true;combat.corpses[0].position={22,23,0};combat.corpses[0].loot={{0,7}};combat.corpses[0].name="a rat";combat.blood[0].active=true;combat.blood[0].position={22,23,0};
 trial.shiftWindow(-16,0);combat.shiftWindow(-16,0);auto next=flat(1016,2000);Actor shiftedPlayer{14,30,0};settle(trial,next,shiftedPlayer);
 assert(trial.all().size()==100&&trial.all()[99].position.x==original.x-16&&trial.all()[99].aggressionOrder==order&&combat.target==99);
 assert(combat.corpses[0].position.x==6&&combat.corpses[0].loot[0].count==7&&combat.blood[0].position.x==6);
 trial.shiftWindow(16,0);combat.shiftWindow(16,0);settle(trial,w,player);assert(trial.all()[99].position.x==original.x&&combat.corpses[0].position.x==22);
 // A small independent roster exercises actual combat, corpse UI transfers,
 // respawn and dormant timers when the player leaves the regional pilot.
 Npcs animals;Npc rat;rat.position=rat.from=rat.home={20,20,0};rat.name="a rat";rat.animal=true;rat.maximumHits=1;rat.meleeMax=0;rat.respawnSeconds=2;animals.combatants().push_back(rat);
 Npcs encounter;encounter.importWindowSpawns(animals,0,0);Actor hero{21,20,0};settle(encounter,w,hero);Combat fight;fight.reset(encounter);
 Inventory bag;bag.init(w,root+"trial.sav");PlayerStatus status;fight.toggle(encounter,hero);fight.update(w,encounter,hero,false,.1f,status,bag);assert(!encounter.all()[0].alive&&fight.corpses[0].active);
 assert(fight.openCorpse(w,hero,bag));assert(bag.containerName()=="a rat");assert(bag.takeAll(w,hero)&&fight.corpses[0].loot.empty());
 encounter.shiftWindow(-1000,0);fight.shiftWindow(-1000,0);settle(encounter,w,hero);float cooldown=fight.vitals()[0].respawn;fight.update(w,encounter,hero,false,5,status,bag);assert(fight.vitals()[0].respawn==cooldown&&!encounter.all()[0].alive);
 encounter.shiftWindow(1000,0);fight.shiftWindow(1000,0);settle(encounter,w,hero);fight.update(w,encounter,hero,false,2.1f,status,bag);assert(encounter.all()[0].alive&&encounter.all().size()==1);
 Inventory restored;restored.init(w,root+"trial.sav");assert(restored.restored&&restored.backpack.size()==bag.backpack.size());
 // Real touch coordinates of the horse must route to mounting, not travel.
 assert(expeditionTouch(220,212)==ExpeditionTouch::Mount);assert(expeditionTouch(250,60)==ExpeditionTouch::Travel);assert(expeditionTouch(35,138)==ExpeditionTouch::ZoomOut);
 assert(expeditionDestinationHit(true,25,178,13)==12&&expeditionDestinationHit(true,25,179,13)==-1);
 assert(worldUploadAllowance(0,true)==1&&worldUploadAllowance(0,false)==0&&worldUploadAllowance(1,true)==1);
 std::cout<<"Expedition NPC pilot: stable IDs across seams, 64-near cap, dormant clocks, combat/loot/respawn, isolated inventory, horse touch and terrain admission passed\n";
}
