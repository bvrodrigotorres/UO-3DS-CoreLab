// SPDX-License-Identifier: MIT
#include "session_save.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
static int fail(const char*,const char*){return -1;}
int main(int argc,char**argv){
 assert(argc==2);std::string root=std::string(argv[1])+"/";World w;w.width=w.height=32;w.land.resize(1024);w.vertices.resize(1089);w.index();Actor player{8,8,0};Npcs npcs;npcs.combatants().resize(2);
 auto&troll=npcs.combatants()[0];troll.name="a troll";troll.animal=troll.hostile=true;troll.maximumHits=100;troll.meleeMin=troll.meleeMax=2;troll.position=troll.from={9,8,0};troll.home={29,29,0};
 auto&rat=npcs.combatants()[1];rat.name="a rat";rat.animal=true;rat.maximumHits=20;rat.position=rat.from=rat.home={13,8,0};
 Inventory bag;bag.init(w,root+"bag.sav");PlayerStatus status;Combat combat;combat.reset(npcs);
 // A stale manual target must not hide an attacking troll, even beyond its home leash.
 combat.target=1;combat.attacking=true;combat.update(w,npcs,player,false,.05f,status,bag);assert(combat.target==0&&combat.attacking&&combat.vitals()[0].hits<100);
 // A second active enemy cannot steal this first focus; neutral animals are never queued.
 npcs.engage(1);combat.update(w,npcs,player,false,.05f,status,bag);assert(combat.target==0);rat.hostile=false;troll.alive=false;combat.update(w,npcs,player,false,.05f,status,bag);assert(!combat.attacking&&combat.target==-1);
 Npcs quiet;Combat heal;heal.reset(quiet);bag.backpack={{5,1},{3,3},{2,3}};status.hits=1;assert(!heal.repeatHealing(bag,status));assert(heal.useHealing(bag,1,status)&&status.lastHealing==3);
 bag.backpack.erase(bag.backpack.begin()); // Remember type, not previous slot.
 heal.update(w,quiet,player,true,3.1f,status,bag);status.hits=1;assert(heal.repeatHealing(bag,status)&&status.hits==21&&bag.backpack[0].count==1);
 heal.update(w,quiet,player,false,3.1f,status,bag);status.hits=1;assert(!heal.repeatHealing(bag,status,fail)&&bag.backpack[0].count==1&&status.hits==1&&status.lastHealing==3);
 assert(heal.repeatHealing(bag,status)&&bag.backpack[0].type==2);heal.update(w,quiet,player,false,3.1f,status,bag);status.hits=1;assert(!heal.repeatHealing(bag,status)&&bag.backpack[0].count==3); // No fallback to apples.
 // Pause saves landed position, resources, equipped items and the remembered consumable.
 bag.equipment[0]=5;status.hits=17;std::string message;assert(saveSession(root+"position.sav",root+"vitals.sav",w,player,bag,status,message));
 PlayerStatus loaded;Inventory restored;restored.init(w,root+"bag.sav");Actor at;assert(loadPosition(root+"position.sav",w,at,message)&&at.x==8&&at.y==8);assert(loadVitals(root+"vitals.sav",loaded)&&loaded.hits==17&&loaded.lastHealing==3&&restored.equipment[0]==5);
 // Legacy UVT1 stays readable; invalid/trailing preferences cannot alter live state.
 auto hash=[](const unsigned char*b,size_t count){uint32_t h=2166136261u;for(size_t i=0;i<count;++i)h=(h^b[i])*16777619u;return h;};
 auto put=[](unsigned char*b,uint32_t v){for(int i=0;i<4;++i)b[i]=v>>(8*i);};unsigned char old[20]={'U','V','T','1'};put(old+4,22);put(old+8,30);put(old+12,40);put(old+16,hash(old,16));{std::ofstream f(root+"old.sav",std::ios::binary);f.write((char*)old,20);}assert(loadVitals(root+"old.sav",loaded)&&loaded.hits==22&&loaded.lastHealing==0);
 {std::ofstream f(root+"vitals.sav",std::ios::binary|std::ios::app);f.put('x');}assert(!loadVitals(root+"vitals.sav",loaded)&&loaded.hits==22);
 assert(!saveSession(root+"missing/position.sav",root+"retry.sav",w,player,bag,status,message)&&message.find("ERROR SD")!=std::string::npos);assert(loadVitals(root+"retry.sav",loaded)&&loaded.lastHealing==3);
 std::cout<<"Controls 35: distant troll retaliation/stale manual focus, sticky aggression, repeat healing by type, no fallback, SD rollback, pause checkpoint and UVT1/UVT2 compatibility passed\n";
}
