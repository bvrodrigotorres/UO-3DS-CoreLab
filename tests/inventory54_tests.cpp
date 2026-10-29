// SPDX-License-Identifier: MIT
#include "commerce.hpp"
#include "inventory_drag.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
static int fail54(const char*,const char*){return -1;}
static unsigned count54(const Bag&bag,unsigned type){unsigned n=0;for(auto s:bag)if(s.type==type)n+=s.count;return n;}
int main(int argc,char**argv){
 assert(argc==2);std::string root=std::string(argv[1])+"/";World w;w.width=w.height=16;w.land.resize(256);w.vertices.resize(289);Object chest;chest.art=0xe40;chest.x=5;chest.y=5;w.objects={chest};w.index();Actor player{4,5,0};
 assert(BagSlots==36&&BagPages==3&&itemDefs[4].stackable);Inventory inv;inv.init(w,root+"old-potions.sav");inv.chests[0].items={{4,1},{4,1},{3,8}};inv.backpack={{0,300},{4,1},{4,1},{4,1}};assert(inv.checkpoint());
 // This is a pre-stacking legacy layout: duplicate single potions, old-format header.
 {std::ifstream f(root+"old-potions.sav",std::ios::binary);char head[4];f.read(head,4);assert(head[3]=='3');}
 Inventory loaded;loaded.init(w,root+"old-potions.sav");assert(loaded.restored&&loaded.healthy()&&loaded.backpack.size()==2&&count54(loaded.backpack,4)==3&&loaded.chests[0].items.size()==2&&count54(loaded.chests[0].items,4)==2);
 assert(loaded.checkpoint());{std::ifstream f(root+"old-potions.sav",std::ios::binary);char head[4];f.read(head,4);assert(head[3]=='C');}
 Inventory migrated;migrated.init(w,root+"old-potions.sav");assert(migrated.healthy()&&migrated.restored&&count54(migrated.backpack,4)==3);
 assert(migrated.followChest(0)&&migrated.takeAll(w,player)&&count54(migrated.backpack,4)==5);
 assert(migrated.tradeQuantity(true,4,25,4)&&count54(migrated.backpack,4)==9&&PlayerStatus::gold(&migrated)==200);
 auto coins=PlayerStatus::gold(&migrated);assert(!migrated.tradeQuantity(false,4,12,3,fail54)&&count54(migrated.backpack,4)==9&&PlayerStatus::gold(&migrated)==coins);
 assert(migrated.tradeQuantity(false,4,12,3)&&count54(migrated.backpack,4)==6&&PlayerStatus::gold(&migrated)==236);
 // All three pages survive save/reload; page three touches and drags use slot 35.
 migrated.backpack.assign(36,{5,1});migrated.backpack[35]={7,1};assert(migrated.checkpoint());Inventory pages;pages.init(w,root+"old-potions.sav");assert(pages.healthy()&&pages.backpack.size()==36&&pages.backpack[35].type==7);
 pages.tap(w,player,25,200);assert(pages.bagPage==1);pages.tap(w,player,25,200);assert(pages.bagPage==2);
 int x=InventoryLayout::Left+2*44+3,y=InventoryLayout::Top+3*38+3;InventoryDrag drag;assert(drag.begin(pages,x,y)&&drag.index==35&&drag.item.type==7);drag.cancel();
 pages.equipMode=true;pages.tap(w,player,x,y);assert(pages.equipment[2]==7&&pages.backpack.size()==35);assert(pages.unequip(2)&&pages.backpack.size()==36);pages.tap(w,player,25,200);assert(pages.bagPage==0);
 // Potion consumption removes one from the stack. Y always selects bandages.
 Npcs quiet;Combat combat;combat.reset(quiet);PlayerStatus status;status.skills.start(5);status.hits=1;pages.backpack={{4,4},{3,3},{1,2}};
 assert(combat.useHealing(pages,0,status)&&status.hits==31&&count54(pages.backpack,4)==3&&status.lastHealing==4);
 combat.update(w,quiet,player,false,3.1f,status,pages);status.hits=1;
 assert(combat.repeatHealing(pages,status)&&combat.bandageWait()>0&&status.hits==1&&count54(pages.backpack,3)==2&&count54(pages.backpack,4)==3);
 combat.update(w,quiet,player,true,3,status,pages);assert(status.hits==16);status.hits=1;
 assert(!combat.repeatHealing(pages,status,fail54)&&count54(pages.backpack,3)==2&&status.hits==1);
 pages.backpack={{4,3},{1,2}};status.lastHealing=4;assert(!combat.repeatHealing(pages,status)&&count54(pages.backpack,4)==3&&status.hits==1&&pages.message=="NO TIENES VENDAS");
 // Expanded format carries paid training and magical gear unchanged across zones.
 Skills trained;trained.start(5);trained.entries[unsigned(Skill::Healing)].value=300;pages.backpack={{0,500},{4,3},{uint16_t(magicItem(16,3,6)),1}};assert(pages.commitTraining(trained,300));Inventory trainedBag;trainedBag.init(w,root+"old-potions.sav");assert(trainedBag.healthy()&&trainedBag.trainingSerial==1&&trainedBag.trainingSkills.value(Skill::Healing)==300&&count54(trainedBag.backpack,4)==3&&trainedBag.backpack[2].type==magicItem(16,3,6));
 Inventory other;other.init(w,root+"other54.sav");other.copyPlayerFrom(trainedBag);assert(other.checkpoint());Inventory arrived;arrived.init(w,root+"other54.sav");assert(arrived.healthy()&&arrived.trainingSerial==1&&count54(arrived.backpack,4)==3);
 assert(arrived.drop(w,player,false,1)&&count54(arrived.backpack,4)==0);Inventory floor;floor.init(w,root+"other54.sav");assert(floor.healthy()&&floor.ground[0].items[0].type==4&&floor.ground[0].items[0].count==3&&floor.ground[0].remaining[0]==180);assert(floor.followGround(0)&&floor.takeAll(w,player)&&count54(floor.backpack,4)==3);
 std::vector<uint8_t>fb(320*240*3+32,0xa5);pages.bagPage=2;drawInventory(fb.data()+16,w,player,pages);for(int i=0;i<16;++i)assert(fb[i]==0xa5&&fb[fb.size()-1-i]==0xa5);
 std::cout<<"Inventory54: legacy potion migration, stack trades/loot/consumption/rollback; 36 slots, page3 equip/drag/reload; bandage-only Y after potion; UINC training/gear/zone preservation passed\n";
}
