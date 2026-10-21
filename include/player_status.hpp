// SPDX-License-Identifier: MIT
#pragma once
#include "inventory.hpp"
#include <algorithm>
namespace lab {
// Initial local character attributes. Combat/spells will update these resources.
struct PlayerStatus {
    int strength=50,dexterity=50,intelligence=50;
    int hits=50,hitsMax=50,mana=50,manaMax=50,stamina=50,staminaMax=50;
    static unsigned gold(const Inventory*bag){unsigned total=0;if(bag)for(const auto&item:bag->backpack)if(item.type==0)total+=item.count;return total;}
    static int barWidth(int value,int maximum,int width){if(maximum<=0)return 0;return int(int64_t(std::max(0,std::min(value,maximum)))*width/maximum);}
};
}
