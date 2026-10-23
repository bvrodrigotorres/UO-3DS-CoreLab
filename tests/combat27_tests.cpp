// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    assert(argc==2);World w;w.width=w.height=24;w.startX=w.startY=2;w.vertices.assign(625,0);w.land.resize(576);w.index();
    Npcs npcs;npcs.combatants().resize(1);auto&n=npcs.combatants()[0];n.position=n.from=n.home={10,10,0};n.animal=true;n.name="a rat";n.wait=100;
    Actor player{4,10,0};Combat c;c.reset(npcs);Inventory inv;inv.init(w,std::string(argv[1])+"/combat27.sav");PlayerStatus status;Doors doors;
    c.toggle(npcs,player);c.update(w,npcs,player,false,.05f,status,inv);assert(n.hostile&&c.combatPose()&&!c.striking());
    auto before=n.position;npcs.update(w,player,player,false,0,doors);assert(n.position.x==before.x&&!n.moving);
    for(int i=0;i<150&&!meleeReach(w,n.position,player);++i){npcs.update(w,player,player,false,.05f,doors);assert(canStand(w,n.position));}
    assert(meleeReach(w,n.position,player));
    bool swing=false;for(int i=0;i<40;++i){npcs.update(w,player,player,false,.05f,doors);c.update(w,npcs,player,false,.05f,status,inv);if(c.striking()){swing=true;assert(c.strikeFrame()>=0&&c.strikeFrame()<4);break;}}
    assert(swing);c.toggle(npcs,player);n.hostile=false;for(int i=0;i<12;++i)c.update(w,npcs,player,false,.05f,status,inv);assert(!c.striking()&&!c.combatPose());
    // A wall forces a detour; no path can cross the blocked tile or the player.
    n.position=n.from=n.home={10,10,0};n.moving=false;n.alive=true;n.hostile=true;player={6,10,0};
    Object wall;wall.x=8;wall.y=10;wall.height=40;wall.flags=Wall|Impassable;w.objects={wall};w.index();
    bool detour=false;for(int i=0;i<200&&!meleeReach(w,n.position,player);++i){npcs.update(w,player,player,false,.05f,doors);assert(canStand(w,n.position));assert(!(n.position.x==player.x&&n.position.y==player.y));detour|=n.position.y!=10;}
    assert(detour&&meleeReach(w,n.position,player));
    // A sealed room and a different floor cannot be crossed by aggression.
    w.objects.clear();for(int y=0;y<24;++y){wall.x=8;wall.y=y;w.objects.push_back(wall);}w.index();n.position=n.from=n.home={10,10,0};n.moving=false;n.hostile=true;
    c.reset(npcs);c.toggle(npcs,player);auto hp=status.hits,enemy=c.vitals()[0].hits;
    for(int i=0;i<80;++i){npcs.update(w,player,player,false,.05f,doors);c.update(w,npcs,player,false,.05f,status,inv);assert(n.position.x>8&&!c.striking());}
    assert(status.hits==hp&&c.vitals()[0].hits==enemy);
    assert(!meleeReach(w,n.position,{n.position.x,n.position.y,20}));
    std::cout<<"Combat 27: remote aggression, pursuit, obstacle detour, sealed walls, pause and timed swing poses passed\n";
}
