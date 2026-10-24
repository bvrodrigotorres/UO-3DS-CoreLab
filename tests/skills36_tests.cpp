// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include "hud.hpp"
#include "startup.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
static int fail(const char*,const char*){return -1;}
static unsigned id(Skill s){return unsigned(s);}
static bool train(Skills&s,Skill skill,int difficulty){for(int n=0;n<40;++n){s.tick(2);if(s.practice(skill,difficulty))return true;}return false;}
static std::vector<char>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
int main(int argc,char**argv){
 assert(argc>=2);std::string root=std::string(argv[1])+"/";
 assert(SkillCount==49&&id(Skill::Swords)==40&&id(Skill::Healing)==17&&id(Skill::Meditation)==46);assert(weaponSkill(5)==Skill::Fencing&&weaponSkill(16)==Skill::Swords&&weaponSkill(18)==Skill::Swords&&weaponSkill(17)==Skill::Macing&&weaponSkill(-1)==Skill::Wrestling);
 Skills s;s.start(5);assert(s.total()==1000&&s.value(Skill::Fencing)==500&&s.value(Skill::Tactics)==250&&s.value(Skill::Anatomy)==250);s.start(16);assert(s.value(Skill::Swords)==0&&s.total()==1000);
 assert(train(s,Skill::Fencing,700)&&s.value(Skill::Fencing)==501);auto value=s.value(Skill::Fencing);for(int n=0;n<100;++n)assert(!s.practice(Skill::Fencing,700));assert(s.value(Skill::Fencing)==value);s.tick(0);assert(!s.practice(Skill::Fencing,700));
 s.entries[id(Skill::Fencing)].wait=0;assert(!s.practice(Skill::Fencing,100)&&s.value(Skill::Fencing)==value);assert(!s.practice(Skill::Mining,1000)&&s.value(Skill::Mining)==0);
 s.entries[id(Skill::Fencing)].lock=SkillLock::Locked;assert(!train(s,Skill::Fencing,1000));s.entries[id(Skill::Fencing)].lock=SkillLock::Down;assert(!train(s,Skill::Fencing,1000));
 // At total cap, only Down skills can pay for one tenth gained; no silent loss.
 Skills cap;cap.ready=true;for(unsigned i=0;i<7;++i)cap.entries[i].value=1000;cap.entries[id(Skill::Swords)].checks=40;
 assert(cap.total()==7000&&!cap.practice(Skill::Swords,1000)&&cap.value(Skill::Swords)==0);cap.entries[0].lock=SkillLock::Down;cap.tick(2);assert(cap.practice(Skill::Swords,1000)&&cap.total()==7000&&cap.entries[0].value==999&&cap.value(Skill::Swords)==1);
 cap.entries[id(Skill::Swords)].value=1000;cap.entries[1].value=1;cap.tick(2);assert(!cap.practice(Skill::Swords,1000)&&cap.value(Skill::Swords)==1000);
 // Probability, damage and bandage strength improve with the relevant real values.
 Skills low,high;low.ready=high.ready=true;for(auto skill:{Skill::Swords,Skill::Tactics,Skill::Anatomy,Skill::Healing})high.entries[id(skill)].value=1000;
 assert(high.hitChance(Skill::Swords,500)>low.hitChance(Skill::Swords,500)&&high.damagePercent()==223&&low.damagePercent()==100&&high.bandageAmount()==29&&low.bandageAmount()==20);
 PlayerStatus status;status.skills=high;status.skills.entries[id(Skill::Swords)].lock=SkillLock::Locked;status.skills.entries[id(Skill::Healing)].checks=4;status.hits=23;status.lastHealing=3;assert(saveVitals(root+"skills.sav",status));PlayerStatus restored;assert(loadVitals(root+"skills.sav",restored)&&restored.hits==23&&restored.lastHealing==3&&restored.skills.total()==4000&&restored.skills.entries[id(Skill::Swords)].lock==SkillLock::Locked&&restored.skills.entries[id(Skill::Healing)].checks==4);
 auto original=read(root+"skills.sav");assert(!saveVitals(root+"skills.sav",status,fail)&&read(root+"skills.sav")==original);auto corrupt=original;corrupt[22]^=1;{std::ofstream f(root+"bad.sav",std::ios::binary);f.write(corrupt.data(),corrupt.size());}assert(!loadVitals(root+"bad.sav",restored)&&restored.skills.total()==4000);
 std::vector<uint8_t>wire;high.encode(wire);Skills decoded;assert(decoded.decode(wire.data(),wire.size()));wire[5]=255;wire[6]=255;assert(!decoded.decode(wire.data(),wire.size())&&decoded.total()==4000);assert(!decoded.decode(wire.data(),wire.size()-1));wire.clear();high.encode(wire);wire[7]=3;assert(!decoded.decode(wire.data(),wire.size()));wire.clear();high.encode(wire);for(unsigned i=0;i<8;++i){wire[5+i*5]=232;wire[6+i*5]=3;}assert(!decoded.decode(wire.data(),wire.size()));
 // The v0.35 UVT2 resources file migrates without losing the healing shortcut.
 auto hash=[](const unsigned char*b,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;};auto put=[](unsigned char*b,uint32_t n){for(int i=0;i<4;++i)b[i]=n>>(8*i);};unsigned char old[24]={'U','V','T','2'};put(old+4,17);put(old+8,40);put(old+12,39);put(old+16,3);put(old+20,hash(old,20));{std::ofstream f(root+"v035.sav",std::ios::binary);f.write((char*)old,24);}PlayerStatus migrated;assert(loadVitals(root+"v035.sav",migrated)&&migrated.hits==17&&migrated.lastHealing==3&&!migrated.skills.ready);migrated.skills.start(5);assert(migrated.hits==17&&migrated.skills.total()==1000);
 World w;w.width=w.height=24;w.land.resize(576);w.vertices.resize(625);w.index();Actor player{8,8,0};Inventory bag;bag.init(w,root+"bag.sav");bag.backpack={{3,100},{4,1},{1,1},{2,1}};Npcs quiet;Combat combat;combat.reset(quiet);PlayerStatus healer;healer.skills.start(5);
 for(int n=0;n<60;++n){combat.update(w,quiet,player,false,3.1f,healer,bag);healer.hits=1;assert(combat.useHealing(bag,0,healer));}assert(healer.skills.value(Skill::Healing)>0&&healer.skills.value(Skill::Anatomy)>250);auto trained=healer.skills.value(Skill::Healing);healer.hits=healer.hitsMax;assert(!combat.useHealing(bag,0,healer)&&healer.skills.value(Skill::Healing)==trained);
 // No alchemy/cooking gains for drinking/eating; bandage gain follows successful consumption.
 healer.hits=1;combat.update(w,quiet,player,false,3.1f,healer,bag);unsigned checks=healer.skills.entries[id(Skill::Healing)].checks;assert(!combat.useHealing(bag,0,healer,fail)&&healer.skills.entries[id(Skill::Healing)].checks==checks);
 healer.mana=1;assert(combat.meditate(healer,bag));for(int n=0;n<20;++n)combat.update(w,quiet,player,false,1,healer,bag);assert(healer.mana>1&&healer.skills.value(Skill::Meditation)>0);auto mana=healer.mana;combat.update(w,quiet,player,false,0,healer,bag);assert(healer.mana==mana);combat.update(w,quiet,player,true,.05f,healer,bag);assert(!combat.meditating);bag.equipment[5]=21;healer.mana=1;assert(!combat.meditate(healer,bag));bag.equipment[5]=-1;assert(combat.meditate(healer,bag));bag.equipment[8]=27;combat.update(w,quiet,player,false,.05f,healer,bag);assert(!combat.meditating);bag.equipment[8]=-1;
 // Real combat, four weapon families, no free practice through a wall.
 Npcs enemies;enemies.combatants().resize(1);auto&enemy=enemies.combatants()[0];enemy.position=enemy.from=enemy.home={9,8,0};enemy.animal=enemy.stationary=enemy.hostile=true;enemy.maximumHits=10000;enemy.meleeMin=enemy.meleeMax=0;enemy.name="training hostile";
 PlayerStatus fighter;fighter.skills.start(16);for(int weapon:{16,17,5,-1}){bag.equipment[0]=weapon;Combat fight;fight.reset(enemies);fight.toggle(enemies,player);for(int n=0;n<250;++n){fighter.stamina=50;fight.update(w,enemies,player,false,.1f,fighter,bag);}assert(fighter.skills.value(weaponSkill(weapon))>(weapon==16?500u:0u));}
 enemy.position.z=30;auto before=fighter.skills.total();Combat blocked;blocked.reset(enemies);blocked.target=0;blocked.attacking=true;for(int n=0;n<100;++n)blocked.update(w,enemies,player,false,.1f,fighter,bag);assert(fighter.skills.total()==before);
 SkillsMenu menu;assert(menu.list().size()==8&&menu.sheets()==2);assert(menu.tap(s,35,215)==3&&menu.all&&menu.sheets()==9);assert(menu.tap(s,230,215)==3&&menu.sheet==1);auto list=menu.list();unsigned row=list[6];auto lock=s.entries[row].lock;assert(menu.tap(s,280,50)==1&&s.entries[row].lock!=lock);assert(menu.tap(s,150,215)==2);
 std::vector<uint8_t>fb(320*240*3+32,0xa5);for(int page=0;page<9;++page){menu.sheet=page;drawSkills(fb.data()+16,s,menu,page%2);}for(int i=0;i<16;++i)assert(fb[i]==0xa5&&fb[fb.size()-1-i]==0xa5);
 GameOptions options;options.page=4;std::string error;assert(options.save(root+"options.sav",error));GameOptions loaded;assert(loaded.load(root+"options.sav")&&loaded.page==4);
 if(argc>=3){SkillsMenu preview;preview.selected=int(Skill::Swords);for(int page=0;page<2;++page){preview.sheet=page;drawSkills(fb.data()+16,high,preview,page==1);std::ofstream ppm(std::string(argv[2])+std::to_string(page)+".ppm",std::ios::binary);ppm<<"P6\n320 240\n255\n";for(int y=0;y<240;++y)for(int x=0;x<320;++x){const auto*p=fb.data()+16+3*(x*240+239-y);char rgb[3]={char(p[2]),char(p[1]),char(p[0])};ppm.write(rgb,3);}}}
 std::cout<<"Skills 36: classic IDs, eight real practice hooks, cooldown/difficulty, caps/locks/decay, bonuses, meditation interruption, UVT3 rollback/integrity, UI bounds and five-page options passed\n";
}
