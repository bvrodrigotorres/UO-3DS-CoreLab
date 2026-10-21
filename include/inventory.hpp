// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include "exploration.hpp"
#include <array>
#include <functional>
namespace lab {
constexpr size_t ItemTypes=8, BagSlots=9;
struct ItemDef {uint16_t art;const char* name;bool stackable;int slot=-1;};
extern const std::array<ItemDef,ItemTypes> itemDefs;
struct ItemStack {uint16_t type=0,count=0;};
using Bag=std::vector<ItemStack>;
struct Chest {size_t object;Bag items;};
struct ItemIcons {std::vector<uint8_t> rgba;bool load(const std::string&);};
class Inventory {
    uint32_t identity=0;
    std::string path;
    bool writable=true;
    void seedGear();
    bool persist(std::string&,SaveRename);
public:
    std::array<int,3>equipment{{-1,-1,-1}};
    bool equipMode=false,restored=false;
    bool healthy()const{return writable;}
    bool checkpoint(){if(!writable)return false;return persist(message,std::rename);}
    std::function<bool(unsigned)>prepareEquipment;
    unsigned equipmentMask()const;
    bool equip(size_t,SaveRename=std::rename);
    bool unequip(size_t,SaveRename=std::rename);
    Bag backpack;
    std::vector<Chest> chests;
    int opened=-1;
    std::string message;
    void init(const World&,const std::string&);
    bool reachable(const World&,const Actor&,size_t)const;
    int nearest(const World&,const Actor&)const;
    void update(const World&,const Actor&);
    void open(const World&,const Actor&);
    bool transfer(const World&,const Actor&,bool fromChest,size_t,SaveRename=std::rename);
    void tap(const World&,const Actor&,int x,int y);
};
}
