// SPDX-License-Identifier: MIT
#include "commerce.hpp"
#include "character.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int fail(const char*,const char*){return -1;}
int main(int argc,char**argv){
    assert(argc>=2);std::string root=std::string(argv[1])+"/";
    World w;w.width=w.height=24;w.vertices.assign(625,0);w.land.resize(576);w.startX=w.startY=5;w.index();Actor player{5,5,0};
    Inventory bag;bag.init(w,root+"economy.sav");bag.backpack={{0,1000}};
    for(unsigned type=16;type<=21;++type){assert(bag.trade(true,type,itemDefs[type].price));assert(bag.equip(bag.backpack.size()-1));}
    PlayerStatus status;assert(status.damageMin(&bag)==17&&status.damageMax(&bag)==25&&status.attackDelay(&bag)>1.6f&&status.resistance(&bag)==36);
    auto key=bag.equipmentMask();assert(((key>>13)&3)==3&&((key>>15)&3)==3);
    Inventory reloaded;reloaded.init(w,root+"economy.sav");assert(reloaded.restored&&reloaded.equipmentMask()==key&&reloaded.equipment[0]==18&&reloaded.equipment[5]==21);
    assert(bag.unequip(5)&&status.resistance(&bag)==0);assert(bag.toggleEquipment(5)&&status.resistance(&bag)==36);
    auto gold=PlayerStatus::gold(&bag);auto before=bag.backpack;assert(!bag.trade(true,5,35,fail));assert(PlayerStatus::gold(&bag)==gold&&bag.backpack.size()==before.size());
    assert(!bag.trade(true,5,65000));assert(!bag.trade(false,25,2));
    bag.backpack.assign(BagSlots,{5,1});bag.backpack[0]={0,35};assert(bag.trade(true,5,35)&&bag.backpack.size()==BagSlots); // paying frees the coin slot
    bag.backpack.assign(BagSlots,{5,1});assert(bag.trade(false,5,12)&&bag.backpack.size()==BagSlots&&PlayerStatus::gold(&bag)==12);
    bag.backpack={{0,65535},{5,1}};assert(!bag.trade(false,5,17)&&bag.backpack.size()==2);
    bag.backpack={{0,1000}};Npcs npcs;npcs.combatants().clear();Npc smith;smith.position=smith.from=smith.home={6,5,0};smith.name="Smith";smith.title="the blacksmith";npcs.combatants().push_back(smith);Commerce shop;
    assert(shop.update(w,npcs,player)&&shop.available()&&shop.accepts(16)&&!shop.accepts(22));
    shop.tap(w,npcs,player,bag,215,40);assert(shop.selected()==16&&PlayerStatus::gold(&bag)==1000);shop.tap(w,npcs,player,bag,200,202);assert(PlayerStatus::gold(&bag)==900&&bag.backpack.back().type==16);
    shop.tap(w,npcs,player,bag,64,40);assert(!shop.purchase());shop.tap(w,npcs,player,bag,200,202);assert(PlayerStatus::gold(&bag)==950&&bag.backpack.size()==1); // no arbitrage
    shop.tap(w,npcs,player,bag,215,40);npcs.combatants()[0].position={12,12,0};shop.tap(w,npcs,player,bag,200,202);assert(PlayerStatus::gold(&bag)==950&&!shop.available());
    npcs.combatants()[0].position={6,5,0};Object wall;wall.x=6;wall.y=5;wall.flags=Wall|Impassable;wall.height=80;w.objects.push_back(wall);w.index();shop.update(w,npcs,player);assert(!shop.available());w.objects.clear();w.index();
    auto strikes=[&](int weapon){Npcs foes;Npc enemy;enemy.animal=true;enemy.name="a training creature";enemy.maximumHits=2000;enemy.meleeMax=0;enemy.position=enemy.from=enemy.home={6,5,0};foes.combatants().push_back(enemy);Combat c;c.reset(foes);Inventory inv;inv.init(w,root+"cadence.sav");inv.equipment[0]=weapon;PlayerStatus stats;c.toggle(foes,player);for(int i=0;i<180;++i)c.update(w,foes,player,false,1.f/60,stats,inv);return stats.stamina;};
    assert(strikes(5)<strikes(18));
    // Armour mitigation is applied to actual incoming damage.
    auto damage=[&](bool armored){Npcs enemies;Npc e;e.animal=e.hostile=true;e.name="an orc";e.maximumHits=60;e.meleeMin=e.meleeMax=20;e.position=e.from=e.home={6,5,0};enemies.combatants().push_back(e);Combat c;c.reset(enemies);PlayerStatus stats;Inventory inv;inv.init(w,root+(armored?"armored.sav":"plain.sav"));if(armored)inv.equipment[5]=21;c.update(w,enemies,player,false,1.01f,stats,inv);return 50-stats.hits;};
    assert(damage(false)==20&&damage(true)==12);
    // Cadavers use species supplies, with no coins on passive fauna.
    auto loot=[&](const std::string&name){Npcs enemies;Npc e;e.animal=true;e.name=name;e.maximumHits=1;e.position=e.from=e.home={6,5,0};enemies.combatants().push_back(e);Combat c;c.reset(enemies);PlayerStatus stats;Inventory inv;inv.init(w,root+"loot.sav");c.toggle(enemies,player);c.update(w,enemies,player,false,.1f,stats,inv);assert(!enemies.all()[0].alive);return c.corpses[0].loot;};
    auto cow=loot("a cow"),bird=loot("a bird"),orc=loot("an orc"),skeleton=loot("a skeleton");
    auto has=[](const Bag&items,int t){return std::any_of(items.begin(),items.end(),[&](ItemStack x){return x.type==t;});};
    assert(has(cow,22)&&has(cow,23)&&!has(cow,0)&&has(bird,24)&&has(orc,0)&&has(skeleton,25));
    // New trade screen writes only its 320x240 framebuffer.
    std::vector<uint8_t>fb(320*240*3+32,0xa5);ItemIcons icons;shop.update(w,npcs,player);drawCommerce(fb.data()+16,bag,shop,&icons);for(int i=0;i<16;++i)assert(fb[i]==0xa5&&fb[fb.size()-1-i]==0xa5);
    TouchPages pages;pages.count=4;pages.page=2;pages.begin(260,80);pages.move(40,80);assert(pages.end()&&pages.page==3);
    // Optional private verification: actual new sparse layers in both banks.
    if(argc==3){CharacterAssets assets;assert(assets.load(argv[2]));Appearance a;a.enabled=a.realClothes=true;for(int sex=0;sex<2;++sex){a.female=sex;for(unsigned weapon=0;weapon<4;++weapon)for(unsigned armor=0;armor<4;++armor){unsigned mask=(weapon?1u:0u)|32|(weapon<<13)|(armor<<15);std::vector<uint16_t>pixels;assert(assets.compose(a,mask,pixels,false,-1,false)&&pixels.size()==72*96*112);assert(assets.compose(a,mask,pixels,false,-1,true));assert(assets.compose(a,mask,pixels,true));}}}
    std::cout<<"Economy 30: gear stats/cadence/mitigation, saved variants, natural loot, atomic buy/sell, no arbitrage, full bags/coin cap/SD rollback, vendor reach and trade UI passed\n";
}
