// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include "equipment_menu.hpp"
#include "hud.hpp"
#include "character.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int failGear(const char*,const char*){return -1;}
int main(int argc,char**argv){
 assert(argc>=2);std::string root=std::string(argv[1])+"/",error;
 static_assert(ItemTypes==870&&BaseItemTypes==30);
 for(unsigned type=0;type<ItemTypes;++type){int base=itemBase(type);assert(base>=0&&base<30&&itemDefs[type].slot==itemDefs[base].slot&&itemDefs[type].art==itemDefs[base].art);assert(weaponSkill(type)==weaponSkill(base));
  if(type>=30){assert(itemQuality(type)>=1&&itemQuality(type)<=4&&magicItem(base,itemQuality(type),itemPower(type))==type);std::array<int,11>a,b;a.fill(-1);b=a;a[itemDefs[type].slot]=type;b[itemDefs[type].slot]=base;assert(Inventory::appearanceKey(a)==Inventory::appearanceKey(b));assert(!itemDetails(type).empty());}}
 std::array<unsigned,5>counts{};uint32_t seed=0x7315;
 for(unsigned i=0;i<100000;++i){++counts[itemQuality(rollEquipment(0,seed,0,2))];}
 assert(counts[0]>counts[1]&&counts[1]>counts[2]&&counts[2]>counts[3]&&counts[3]>counts[4]&&counts[4]<100);
 unsigned luckyImproved=0;for(unsigned i=0;i<10000;++i){uint32_t a=i*7919+1,b=a;unsigned qa=itemQuality(rollEquipment(0,a,0,2)),qb=itemQuality(rollEquipment(0,b,1000,2));assert(qb>=qa);luckyImproved+=qb>qa;assert(itemQuality(rollEquipment(3,seed))>=2&&itemQuality(rollEquipment(4,seed))>=3);}assert(luckyImproved>0);
 World w;w.width=w.height=16;w.vertices.assign(17*17,0);w.land.resize(256);Object chest;chest.art=0xe40;chest.x=5;chest.y=5;w.objects={chest};w.index();
 Inventory inv;inv.init(w,root+"gear.sav");unsigned sword=magicItem(16,4,6),ring=magicItem(28,3,0),armour=magicItem(21,4,1);
 inv.backpack={{uint16_t(sword),1},{uint16_t(ring),1},{uint16_t(armour),1}};inv.chests[0].items={{uint16_t(magicItem(29,3,5)),1}};
 assert(inv.equip(0)&&inv.equipment[0]==int(sword));assert(inv.equip(0)&&inv.equipment[9]==int(ring));assert(inv.equip(0)&&inv.equipment[5]==int(armour));assert(inv.checkpoint());
 {std::ifstream f(root+"gear.sav",std::ios::binary);char head[4];f.read(head,4);assert(head[3]=='A');}
 Inventory restored;restored.init(w,root+"gear.sav");assert(restored.healthy()&&restored.restored&&restored.equipment==inv.equipment&&restored.chests[0].items[0].type==inv.chests[0].items[0].type);
 assert(!restored.unequip(0,failGear)&&restored.equipment[0]==int(sword));assert(restored.unequip(0)&&restored.toggleEquipment(0)&&restored.equipment[0]==int(sword));
 Inventory destination;destination.init(w,root+"destination.sav");destination.copyPlayerFrom(restored);assert(destination.equipment==restored.equipment&&destination.checkpoint());
 PlayerStatus stats;assert(stats.damageMax(&restored)>stats.damageMax(nullptr)&&stats.resistance(&restored,1)>0);assert(stats.totalLuck(&restored)>=0);
 for(unsigned slot=0;slot<restored.equipment.size();++slot){for(int base:GearBases)if(itemDefs[base].slot==int(slot)){restored.equipment[slot]=magicItem(base,4,1);break;}}
 for(unsigned c=0;c<5;++c){assert(stats.resistance(&restored,c)<=70);}assert(stats.resistance(&restored)==70);
 assert(PlayerStatus::bonus(&restored,&ItemDef::defenseChance,45)==45);
 for(unsigned power:{0u,4u,5u}){for(unsigned slot=0;slot<restored.equipment.size();++slot){for(int base:GearBases)if(itemDefs[base].slot==int(slot)){restored.equipment[slot]=magicItem(base,4,power);break;}}
  if(power==0)assert(PlayerStatus::bonus(&restored,&ItemDef::damageIncrease,100)==100&&PlayerStatus::bonus(&restored,&ItemDef::hitChance,45)==45);
  if(power==4)assert(PlayerStatus::bonus(&restored,&ItemDef::swingSpeed,60)==60&&stats.attackDelay(&restored)<.65f);
  if(power==5)assert(stats.totalLuck(&restored)==1000);
 }
 {Npcs quiet;Combat regeneration;regeneration.reset(quiet);Inventory wearable;wearable.equipment[9]=magicItem(28,2,2);PlayerStatus wounded;wounded.hits=20;wounded.healthVisible=6;Actor at{6,6,0};for(int i=0;i<3;++i){regeneration.update(w,quiet,at,false,1,wounded,wearable);}assert(wounded.hits==20);regeneration.update(w,quiet,at,false,1,wounded,wearable);assert(wounded.hits==22);}
 std::array<uint8_t,5>fire{{0,100,0,0,0}},physical{{100,0,0,0,0}};std::array<int,5>res{{0,70,0,0,0}};assert(elementalDamage(100,fire,res)==30&&elementalDamage(100,physical,res)==100);res[1]=100;assert(elementalDamage(100,fire,res)==0);
 Npc enemy;enemy.name="a rat";enemy.animal=enemy.aggressive=true;enemy.maximumHits=1000;enemy.meleeMax=0;enemy.position=enemy.from=enemy.home={7,6,0};enemy.resistances[0]=100;
 auto attack=[&](int weapon){Npcs npcs;npcs.combatants()={enemy};Combat combat;combat.reset(npcs);Inventory bag;bag.equipment[0]=weapon;PlayerStatus status;Actor player{6,6,0};combat.toggle(npcs,player);combat.update(w,npcs,player,false,.01f,status,bag);return combat.vitals()[0].hits;};assert(attack(5)==1000&&attack(magicItem(5,2,6))<1000);
 auto incoming=[&](std::array<uint8_t,5>types){Npcs npcs;auto attacker=enemy;attacker.resistances={};attacker.hostile=true;attacker.meleeMin=attacker.meleeMax=10;attacker.damageTypes=types;npcs.combatants()={attacker};Combat combat;combat.reset(npcs);Inventory bag;bag.equipment[9]=magicItem(28,2,6);PlayerStatus status;Actor player{6,6,0};for(int i=0;i<30&&status.hits==50;++i)combat.update(w,npcs,player,false,.05f,status,bag);return status.hits;};assert(incoming(fire)>incoming(physical));
 for(unsigned power:{2u,3u}){Npcs npcs;auto victim=enemy;victim.resistances={};victim.maximumHits=100000;npcs.combatants()={victim};Combat combat;combat.reset(npcs);Inventory bag;bag.equipment[0]=magicItem(5,4,power);PlayerStatus status;status.hits=1;status.mana=0;Actor player{6,6,0};combat.toggle(npcs,player);for(int i=0;i<100;++i)combat.update(w,npcs,player,false,1,status,bag);assert(combat.healingRevision()>0);if(power==2)assert(status.hits>1);}
 Npcs boss;auto leader=enemy;leader.name="El Devorador";leader.maximumHits=1;leader.resistances={};boss.combatants()={leader};Combat fight;fight.reset(boss);Inventory bag;PlayerStatus status;Actor player{6,6,0};fight.toggle(boss,player);fight.update(w,boss,player,false,.01f,status,bag);bool rareWeapon=false,magicJewel=false;unsigned gold=0;for(auto item:fight.corpses[0].loot){if(!item.type)gold+=item.count;else if(itemDefs[item.type].slot==0)rareWeapon=itemQuality(item.type)>=3;else if(itemDefs[item.type].slot>=9)magicJewel=itemQuality(item.type)>=2;}assert(gold>=300&&rareWeapon&&magicJewel&&fight.corpses[0].loot.size()<=ChestSlots);
 std::vector<uint8_t>top(400*240*3+32,0xAA),bottom(320*240*3+32,0xAA);EquipmentMenu menu(inv);Appearance appearance;drawEquipmentMenu(top.data()+16,bottom.data()+16,menu,appearance,inv,stats,{},{});for(unsigned i=0;i<16;++i)assert(top[i]==0xAA&&top[top.size()-1-i]==0xAA&&bottom[i]==0xAA&&bottom[bottom.size()-1-i]==0xAA);
 if(argc==4){World town;assert(loadWorld(std::string(argv[2])+"/scene.bin",town,error));Npcs population;assert(population.loadPopulation(std::string(argv[2])+"/population.bin",std::string(argv[2])+"/scene.bin",town,error));assert(population.loadCombatProfiles(argv[3],std::string(argv[2])+"/scene.bin",std::string(argv[2])+"/population.bin",error));population.reset(town,{town.startX,town.startY,town.startZ});bool resistance=false;for(const auto&n:population.all())for(auto r:n.resistances)resistance|=r>0;assert(resistance);
 auto previous=population.all();std::ifstream input(argv[3],std::ios::binary);std::vector<char>bytes{std::istreambuf_iterator<char>(input),{}};bytes.back()^=1;std::string broken=root+"broken-combat.bin";{std::ofstream out(broken,std::ios::binary);out.write(bytes.data(),bytes.size());}
 assert(!population.loadCombatProfiles(broken,std::string(argv[2])+"/scene.bin",std::string(argv[2])+"/population.bin",error));population.reset(town,{town.startX,town.startY,town.startZ});for(unsigned i=0;i<previous.size();++i)assert(previous[i].resistances==population.all()[i].resistances&&previous[i].damageTypes==population.all()[i].damageTypes);
 }
 std::cout<<"Gear 52: 840 variants, legacy-safe UINA, gear transfers/rollback, shared art/skills, rarity/luck distribution, five damage channels, caps, leech, boss loot and UI bounds passed\n";
}
