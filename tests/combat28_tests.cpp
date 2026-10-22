// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    assert(argc==2);PlayerStatus status;status.hits=10;status.tookDamage();assert(status.recoveryGoal==23);
    status.healthVisible=0;assert(status.showHealth());status.hits=22;status.recovered();assert(status.showHealth());status.hits=23;status.recovered();assert(!status.showHealth());
    status.hits=47;status.tookDamage();assert(status.recoveryGoal==50);status.healthVisible=0;status.hits=50;status.recovered();assert(!status.showHealth());
    World w;w.width=w.height=16;w.vertices.assign(289,0);w.land.resize(256);w.index();Npcs npcs;npcs.combatants().resize(1);auto&n=npcs.combatants()[0];n.position=n.from=n.home={8,8,0};n.name="a rat";n.animal=true;n.wait=100;
    Actor player{7,8,0};Combat c;c.reset(npcs);Inventory inv;inv.init(w,std::string(argv[1])+"/effects28.sav");status=PlayerStatus{};
    // Retaliation emits the actual damage amount and pins the bar to recovery.
    n.hostile=true;int old=status.hits;for(int i=0;i<30&&status.hits==old;++i)c.update(w,npcs,player,false,.05f,status,inv);
    assert(status.hits<old&&status.recoveryGoal==std::min(status.hitsMax,status.hits+13));bool incoming=false;
    for(const auto&e:c.damage){if(e.active&&e.player){incoming=true;assert(e.amount==old-status.hits&&e.npc==-1&&e.position.x==player.x);}}assert(incoming);
    // No invisible-bar condition may block healing while recovery is pinned.
    n.hostile=false;player={2,2,0};int injured=status.hits;for(int i=0;i<130;++i)c.update(w,npcs,player,false,.05f,status,inv);assert(status.hits>injured&&status.showHealth());
    // A lethal hit emits an outgoing number, burst and a three-second pool.
    player={7,8,0};status=PlayerStatus{};c.reset(npcs);c.toggle(npcs,player);bool outgoing=false;
    for(int tick=0;tick<100&&n.alive;++tick){int before=c.vitals()[0].hits;c.update(w,npcs,player,false,.05f,status,inv);int lost=before-c.vitals()[0].hits;
        if(lost>0){for(const auto&e:c.damage)if(e.active&&!e.player&&e.age==0){outgoing=true;assert(e.amount==lost&&e.npc==0&&e.amount>0);}}
    }
    assert(outgoing&&!n.alive);size_t pool=32,body=32;for(size_t i=0;i<32;++i){if(c.blood[i].active)pool=i;if(c.corpses[i].active)body=i;}assert(pool<32&&body<32&&c.blood[pool].variant<7);
    c.update(w,npcs,player,false,0,status,inv);assert(c.blood[pool].age==0);
    player={2,2,0};for(int i=0;i<50;++i)c.update(w,npcs,player,false,.05f,status,inv);assert(c.blood[pool].active);for(int i=0;i<15;++i)c.update(w,npcs,player,false,.05f,status,inv);assert(!c.blood[pool].active&&c.corpses[body].active);
    for(const auto&e:c.damage)assert(!e.active);
    status.hits=20;assert(saveVitals(std::string(argv[1])+"/vitals28.sav",status));PlayerStatus loaded;assert(loadVitals(std::string(argv[1])+"/vitals28.sav",loaded)&&loaded.hits==20&&loaded.recoveryGoal==33&&loaded.showHealth());
    c.reset(npcs);for(const auto&e:c.damage)assert(!e.active);for(const auto&p:c.blood)assert(!p.active);
    std::cout<<"Combat 28: recovery threshold/full health, healing with pinned bar, exact dealt/received damage, pause, timed blood, corpse preservation and continue passed\n";
}
