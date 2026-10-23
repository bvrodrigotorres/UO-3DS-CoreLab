// SPDX-License-Identifier: MIT
#include "equipment_menu.hpp"
#include "hud.hpp"
#include "character.hpp"
#include "combat.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int fail(const char*,const char*){return -1;}
int main(int argc,char**argv){
 assert(argc>=2);std::string root=std::string(argv[1])+"/";World w;w.width=w.height=16;w.vertices.resize(289);w.land.resize(256);w.index();Inventory bag;bag.init(w,root+"gear.sav");bag.backpack={{5,1},{8,1},{26,1},{27,1},{28,1},{29,1}};assert(bag.checkpoint());
 // Old eight-slot records are accepted and remain unchanged until Apply.
 Inventory old;old.init(w,root+"gear.sav");assert(old.restored&&old.equipment.size()==11&&old.equipment[8]==-1&&old.equipment[10]==-1);
 EquipmentMenu editor(old);editor.row=9;editor.change(1);assert(editor.desired[9]==28&&old.equipment[9]==-1);editor.row=8;editor.change(1);assert(editor.desired[8]==26);editor.change(1);assert(editor.desired[8]==27);editor.row=10;editor.change(1);editor.row=0;editor.change(1);editor.row=5;editor.change(1);
 unsigned prepared=0;old.prepareEquipment=[&](unsigned key){prepared=key;return true;};assert(editor.apply(old)&&prepared==old.equipmentMask());
 assert(old.equipment[0]==5&&old.equipment[8]==27&&old.equipment[9]==28&&old.equipment[10]==29&&old.backpack.size()==1&&old.backpack[0].type==26);
 PlayerStatus status;assert(status.damageMin(&old)==9&&status.damageMax(&old)==13&&status.resistance(&old)==15);
 unsigned mask=old.equipmentMask();assert(((mask>>17)&3)==2&&(mask&(1u<<19))&&(mask&(1u<<20)));
 Inventory loaded;loaded.init(w,root+"gear.sav");assert(loaded.restored&&loaded.healthy()&&loaded.equipment==old.equipment&&loaded.equipmentMask()==mask);
 assert(loaded.unequip(9)&&loaded.equipment[9]==-1&&status.damageMin(&loaded)==7);assert(loaded.toggleEquipment(9)&&loaded.equipment[9]==28);
 auto previous=loaded.equipment;auto count=loaded.backpack.size();unsigned last=~0u;loaded.prepareEquipment=[&](unsigned key){last=key;return true;};auto desired=loaded.equipment;desired[10]=-1;assert(!loaded.setOutfit(desired,fail)&&loaded.equipment==previous&&loaded.backpack.size()==count&&last==loaded.equipmentMask());
 desired[10]=28;assert(!loaded.setOutfit(desired)&&loaded.equipment==previous); // wrong slot rejected
 loaded.backpack.assign(BagSlots,{4,1});desired=loaded.equipment;desired[9]=-1;assert(!loaded.setOutfit(desired)&&loaded.equipment==previous); // no space to remove jewelry
 EquipmentMenu empty(loaded);empty.row=3;empty.change(1);assert(empty.desired[3]==-1); // cannot invent unowned hats
 // Cancellation does not touch bag/equipment; arrows/pages remain within all slots.
 for(int n=0;n<100;++n){empty.move(1);assert(empty.row>=0&&empty.row<11);empty.change(-1);}assert(loaded.equipment==previous);
 empty.row=0;assert(empty.tap(250,30)==0&&empty.page()==1);assert(empty.tap(30,220)==-1&&empty.tap(200,220)==1);
 PauseMenu pause;pause.open();pause.selected=2;assert(pause.activate()==PauseAction::Equipment&&pause.hit(60,99)==2);pause.move(1);assert(pause.activate()==PauseAction::Developer);pause.move(1);assert(pause.activate()==PauseAction::MainMenu);pause.move(1);assert(pause.activate()==PauseAction::Exit);
 // Both full screen renderers stay inside their framebuffers for all 11 rows.
 std::vector<uint8_t>top(400*240*3+32,0xa5),bottom(320*240*3+32,0xa5);Appearance a;a.enabled=a.realClothes=true;for(int row=0;row<11;++row){empty.row=row;drawEquipmentMenu(top.data()+16,bottom.data()+16,empty,a,loaded,status,{},{});}for(int i=0;i<16;++i){assert(top[i]==0xa5&&top[top.size()-1-i]==0xa5&&bottom[i]==0xa5&&bottom[bottom.size()-1-i]==0xa5);}
 if(argc==3){CharacterAssets assets;assert(assets.load(argv[2]));for(int sex=0;sex<2;++sex){a.female=sex;for(unsigned sleeve=1;sleeve<=2;++sleeve){unsigned key=1|32|64|(1u<<19)|(1u<<20)|(sleeve<<17);std::vector<uint16_t>pixels;assert(assets.compose(a,key,pixels,true));assert(assets.compose(a,key,pixels,false,-1,false));assert(assets.compose(a,key,pixels,false,-1,true));}}}
 std::cout<<"Equipment 31: 11 owned-only slots, staged/cancelled changes, atomic apply, UIN5 migration/UIN6 reload, SD/GPU rollback, ring damage, bracelet/sleeve defence, capacity and pause/touch/render bounds passed\n";
}
