// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include "exploration.hpp"
#include <array>
#include <functional>
namespace lab {
constexpr size_t ItemTypes=30, BagSlots=24, ChestSlots=12, BagPageSlots=12;
struct InventoryLayout {
    static constexpr int Left=18,Right=170,Top=38,StepX=44,StepY=38,Width=41,Height=35;
    static int slot(int x,int y,int left){if(x<left||x>=left+3*StepX||y<Top||y>=Top+4*StepY)return -1;int dx=x-left,dy=y-Top;if(dx%StepX>=Width||dy%StepY>=Height)return -1;return (dy/StepY)*3+dx/StepX;}
    static bool page(int x,int y){return x>=18&&x<148&&y>=194&&y<210;}
    static bool mode(int x,int y){return x>=18&&x<148&&y>=214&&y<230;}
    static bool take(int x,int y){return x>=170&&x<300&&y>=194&&y<230;}
};
struct ItemDef {uint16_t art;const char* name;bool stackable;int slot=-1;
    int damageMin=0,damageMax=0;float delay=.85f;int protection=0,weight=1,price=1;
};
constexpr unsigned EquipmentAppearanceMask=2047u|(3u<<13)|(3u<<15)|(3u<<17)|(1u<<19)|(1u<<20);
extern const std::array<ItemDef,ItemTypes> itemDefs;
uint32_t inventoryWorldId(const World&);
struct ItemStack {uint16_t type=0,count=0;};
using Bag=std::vector<ItemStack>;
struct Chest {size_t object;Bag items;};
struct ItemIcons {std::vector<uint8_t> rgba;bool load(const std::string&);};
class Inventory {
    uint32_t identity=0;
    std::string path;
    bool writable=true;
    void seedGear();
    Bag*external=nullptr;Actor externalPosition;std::string externalTitle;
    bool persist(std::string&,SaveRename);
public:
    std::array<int,11>equipment{{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}},lastEquipment{{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}};
    bool equipMode=false,restored=false,dressed=false,accessories=false;
    void initialDress(int shirt,int pants,bool shoes,bool robe=false);
    void addAccessories();
    unsigned prospectiveKey(size_t slot,int type)const;
    bool healthy()const{return writable;}
    bool checkpoint(){if(!writable)return false;return persist(message,std::rename);}
    std::function<bool(unsigned)>prepareEquipment;
    unsigned equipmentMask()const;
    bool setOutfit(const std::array<int,11>&,SaveRename=std::rename);
    bool equip(size_t,SaveRename=std::rename);
    bool unequip(size_t,SaveRename=std::rename);
    bool toggleEquipment(size_t,SaveRename=std::rename);
    Bag backpack;
    std::vector<Chest> chests;
    int opened=-1,bagPage=0,detailType=-1;
    bool hasContainer()const{return opened>=0||(opened==-2&&external);}
    const Bag&containerItems()const{static const Bag empty;return opened==-2&&external?*external:opened>=0&&size_t(opened)<chests.size()?chests[opened].items:empty;}
    std::string containerName()const{return opened==-2?externalTitle:"COFRE";}
    bool isExternal(const Bag*bag)const{return opened==-2&&external==bag;}
    bool followChest(int index){if(index<0||size_t(index)>=chests.size()){bool changed=hasContainer();closeContainer();return changed;}if(opened==index&&index>=0)return false;closeContainer();opened=index;return true;}
    void closeContainer(){opened=-1;external=nullptr;}
    void closeExternal(const Bag*bag){if(external==bag)closeContainer();}
    bool openExternal(const World&,const Actor&,Bag&,const Actor&,const std::string&);
    std::string message;
    void init(const World&,const std::string&);
    bool reachable(const World&,const Actor&,size_t)const;
    int nearest(const World&,const Actor&)const;
    void update(const World&,const Actor&);
    void open(const World&,const Actor&);
    bool transfer(const World&,const Actor&,bool fromChest,size_t,SaveRename=std::rename);
    bool takeAll(const World&,const Actor&,SaveRename=std::rename);
    bool trade(bool buy,unsigned type,unsigned price,SaveRename=std::rename);
    void tap(const World&,const Actor&,int x,int y);
};
}
