// SPDX-License-Identifier: MIT
#pragma once
#include "inventory.hpp"
#include "skills.hpp"
#include <algorithm>
namespace lab {
inline int elementalDamage(int raw,const std::array<uint8_t,5>&types,const std::array<int,5>&resists){
    int scaled=0;for(unsigned c=0;c<5;++c)scaled+=raw*types[c]*(100-std::max(0,std::min(100,resists[c])));
    return scaled?std::max(1,scaled/10000):0;
}
// Initial local character attributes. Combat/spells will update these resources.
struct PlayerStatus {
    Skills skills; uint32_t trainingSerial=0;
    bool recoverTraining(const Inventory&inv){if(inv.trainingSerial<=trainingSerial)return false;skills=inv.trainingSkills;trainingSerial=inv.trainingSerial;return true;}
    int strength=50,dexterity=50,intelligence=50;
    int hits=50,hitsMax=50,mana=50,manaMax=50,stamina=50,staminaMax=50;
    int statCap=225,luck=0,followers=0,followersMax=5;
    std::array<int,5>resistances{{0,0,0,0,0}};
    float healthVisible=0,flash=0;
    int recoveryGoal=0;
    bool mounted=false;
    unsigned lastHealing=0; // Legacy save field; Y always uses bandages.
    bool showHealth()const{return healthVisible>0||(recoveryGoal>0&&hits<recoveryGoal);}
    void tookDamage(){recoveryGoal=std::min(hitsMax,hits+(hitsMax+3)/4);healthVisible=6;}
    void recovered(){if(recoveryGoal>0&&hits>=recoveryGoal)recoveryGoal=0;}
    static int bonus(const Inventory*bag,int ItemDef::*field,int cap){int total=0;if(bag)for(int t:bag->equipment)if(t>=0&&t<int(ItemTypes))total+=itemDefs[t].*field;return std::max(0,std::min(cap,total));}
    int damageBonus(const Inventory*bag)const{return bonus(bag,&ItemDef::flatDamage,30);}
    int damageMin(const Inventory*bag)const{int t=bag?bag->equipment[0]:-1;return std::max(1,((t>=0&&t<int(ItemTypes)?itemDefs[t].damageMin:4)+damageBonus(bag))*skills.damagePercent()*(100+bonus(bag,&ItemDef::damageIncrease,100))/10000);}
    int damageMax(const Inventory*bag)const{int t=bag?bag->equipment[0]:-1;return std::max(1,((t>=0&&t<int(ItemTypes)?itemDefs[t].damageMax:7)+damageBonus(bag))*skills.damagePercent()*(100+bonus(bag,&ItemDef::damageIncrease,100))/10000);}
    float attackDelay(const Inventory*bag)const{int t=bag?bag->equipment[0]:-1;float delay=t>=0&&t<int(ItemTypes)?itemDefs[t].delay:.85f;return std::max(.25f,delay*100/(100+bonus(bag,&ItemDef::swingSpeed,60)));}
    int resistance(const Inventory*bag,int channel=0)const{if(channel<0||channel>=5)return 0;int value=resistances[channel];if(bag)for(int t:bag->equipment)if(t>=0&&t<int(ItemTypes))value+=itemDefs[t].resists[channel];return std::max(0,std::min(70,value));}
    int totalLuck(const Inventory*bag)const{return std::max(0,std::min(1000,luck+bonus(bag,&ItemDef::luck,1000)));}
    int weightMax()const{return 40+strength*7/2;}
    static unsigned weight(const Inventory*bag){
        if(!bag)return 0;
        unsigned units=0,coins=0;
        for(const auto&s:bag->backpack){if(s.type==0)coins+=s.count;else if(s.type<ItemTypes)units+=itemDefs[s.type].weight*s.count;}
        for(int t:bag->equipment)if(t>=0&&t<int(ItemTypes))units+=itemDefs[t].weight;
        return units+(coins+49)/50;
    }
    static unsigned gold(const Inventory*bag){unsigned total=0;if(bag)for(const auto&item:bag->backpack)if(item.type==0)total+=item.count;return total;}
    static int barWidth(int value,int maximum,int width){if(maximum<=0)return 0;return int(int64_t(std::max(0,std::min(value,maximum)))*width/maximum);}
};
}
