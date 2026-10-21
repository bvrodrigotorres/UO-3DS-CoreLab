// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
namespace {
int fail(const char*,const char*){return -1;}
unsigned coins(const Bag&b){unsigned total=0;for(auto s:b)if(s.type==0)total+=s.count;return total;}
std::vector<char>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(int argc,char**argv){
    assert(argc>=2);std::string folder=std::string(argv[1])+"/";
    World w;w.width=w.height=24;w.startX=w.startY=2;w.vertices.assign(625,0);w.land.resize(576);w.index();
    Npcs npcs;npcs.combatants().assign(3,Npc{});
    auto&rat=npcs.combatants()[0];rat.position=rat.from=rat.home={8,8,0};rat.name="a rat";rat.animal=true;rat.wait=100;
    npcs.combatants()[1].position=npcs.combatants()[1].from={18,18,0};npcs.combatants()[2].position=npcs.combatants()[2].from={19,18,0};
    Combat combat;combat.reset(npcs);Actor a{7,8,0};PlayerStatus stats;Inventory inv;inv.init(w,folder+"inventory.sav");inv.backpack.clear();
    combat.select(npcs,a);assert(combat.target==0);combat.toggle(npcs,a);assert(combat.attacking);
    int ticks=0;while(rat.alive&&ticks++<200)assert(!combat.update(w,npcs,a,false,.05f,stats,inv));
    assert(!rat.alive&&ticks<200&&stats.hits<stats.hitsMax&&stats.healthVisible>0&&stats.stamina<stats.staminaMax);
    assert(!npcs.occupied(rat.position));auto draws=npcs.drawList(w,a,false);for(auto d:draws)assert(d.index!=0);
    int corpse=combat.nearestCorpse(w,a);assert(corpse>=0&&combat.corpses[corpse].active);auto&body=combat.corpses[corpse];unsigned loot=coins(body.loot);assert(loot>0);
    assert(combat.openCorpse(w,a,inv)&&inv.opened==-2&&inv.containerName()=="a rat");
    assert(inv.transfer(w,a,true,0));assert(PlayerStatus::gold(&inv)==loot&&coins(body.loot)==0);
    assert(!inv.transfer(w,a,false,0)&&PlayerStatus::gold(&inv)==loot);
    auto oldFile=read(folder+"inventory.sav");auto oldBody=body.loot;auto oldBag=inv.backpack;
    assert(!inv.transfer(w,a,true,0,fail));assert(body.loot.size()==oldBody.size()&&inv.backpack.size()==oldBag.size()&&read(folder+"inventory.sav")==oldFile);
    while(!body.loot.empty()){assert(inv.transfer(w,a,true,0));}
    assert(!inv.transfer(w,a,true,0));
    Inventory resumed;resumed.init(w,folder+"inventory.sav");assert(resumed.restored&&PlayerStatus::gold(&resumed)==loot);
    inv.update(w,{2,2,0});assert(!inv.hasContainer());assert(!inv.openExternal(w,{2,2,0},body.loot,body.position,"CADAVER"));
    // A full bag cannot consume a corpse item; no equipped/stacked items vanish.
    body.loot={{4,1}};inv.backpack.assign(BagSlots,{4,1});assert(combat.openCorpse(w,a,inv));assert(!inv.transfer(w,a,true,0)&&body.loot.size()==1);
    float decay=body.decay;combat.update(w,npcs,a,false,0,stats,inv);assert(body.decay==decay);
    for(int i=0;i<410;++i)combat.update(w,npcs,a,false,.05f,stats,inv);
    assert(rat.alive&&combat.vitals()[0].hits==combat.vitals()[0].maximum&&body.active&&body.loot.size()==1);
    assert(canStand(w,rat.position)&&!(rat.position.x==a.x&&rat.position.y==a.y));
    // Player dies from retaliation, returns to reset, keeps backpack/equipment.
    rat.position=rat.from={8,8,0};rat.moving=false;rat.hostile=true;combat.reset(npcs);stats.hits=1;auto bagCount=inv.backpack.size();bool died=false;
    for(int i=0;i<30&&!died;++i)died=combat.update(w,npcs,a,false,.05f,stats,inv);
    assert(died&&a.x==w.startX&&a.y==w.startY&&a.z==w.startZ&&stats.hits==stats.hitsMax&&inv.backpack.size()==bagCount&&!rat.hostile);
    // No melee through a diagonal corner or different floor.
    a={7,8,0};rat.position=rat.from={8,9,0};rat.alive=true;rat.hostile=true;rat.moving=false;
    Object wall;wall.x=8;wall.y=8;wall.height=40;wall.flags=Wall|Impassable;w.objects={wall};w.index();assert(!meleeReach(w,a,rat.position));assert(!meleeReach(w,a,{7,8,20}));
    combat.reset(npcs);combat.toggle(npcs,a);int hp=stats.hits,enemyHp=combat.vitals()[0].hits;
    for(int i=0;i<40;++i){combat.update(w,npcs,a,false,.05f,stats,inv);}
    assert(stats.hits==hp&&combat.vitals()[0].hits==enemyHp);
    Bag external={{0,4}};assert(!inv.openExternal(w,a,external,rat.position,"CADAVER"));
    // Humans remain protected even when their notoriety is blue or gray.
    rat.animal=false;combat.reset(npcs);combat.select(npcs,a);assert(combat.target==-1);
    // Timed corpse disposal closes its borrowed inventory view safely.
    w.objects.clear();w.index();auto&expired=combat.corpses[0];expired.active=true;expired.position=a;expired.decay=.01f;expired.loot={{0,9}};
    assert(combat.openCorpse(w,a,inv));combat.update(w,npcs,a,false,.05f,stats,inv);assert(!expired.active&&!inv.hasContainer());
    // Newest nonempty corpse wins over an older empty body at the same tile.
    for(int i=0;i<2;++i){auto&c=combat.corpses[i];c.active=true;c.position=a;c.decay=20+i*10;}
    combat.corpses[0].loot.clear();combat.corpses[1].loot={{0,2}};assert(combat.nearestCorpse(w,a)==1);
    stats.hits=23;stats.mana=12;stats.stamina=7;assert(saveVitals(folder+"vitals.sav",stats));PlayerStatus loaded;assert(loadVitals(folder+"vitals.sav",loaded)&&loaded.hits==23&&loaded.mana==12&&loaded.stamina==7);
    auto bytes=read(folder+"vitals.sav");stats.hits=31;assert(!saveVitals(folder+"vitals.sav",stats,fail)&&read(folder+"vitals.sav")==bytes);
    {std::ofstream f(folder+"vitals.sav",std::ios::binary);f<<"bad";}assert(!loadVitals(folder+"vitals.sav",loaded)&&loaded.hits==23);
    assert(loadVitals(folder+"legacy-no-vitals.sav",loaded));
    assert(PlayerStatus::weight(&inv)==BagSlots&&stats.damageMax(&inv)==7);inv.equipment[0]=5;assert(stats.damageMin(&inv)==7&&stats.damageMax(&inv)==11);
    std::vector<uint8_t>fb(320*240*3+32,0xCD);StatusArt art;if(argc==3)assert(art.load(argv[2]));
    drawJournal(fb.data()+16,w,a,nullptr,2,"",&inv,nullptr,&stats,&art);drawInventory(fb.data()+16,w,a,inv);
    for(int i=0;i<16;++i)assert(fb[i]==0xCD&&fb[fb.size()-1-i]==0xCD);
    std::cout<<"Combat damage, retaliation, walls/floors, protected humans, death/reset, corpse loot rollback/full bag, respawn/decay, vitals persistence and status bounds passed\n";
}
