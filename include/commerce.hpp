// SPDX-License-Identifier: MIT
#pragma once
#include "combat.hpp"
namespace lab {
class Commerce {
    int vendor=-1,selectedType=-1;bool buying=true;std::string identity;
public:
    std::vector<unsigned>stock;std::string title="SIN COMERCIANTE",message="ACERCATE A UN VENDEDOR";
    int bagPage=0;
    bool available()const{return vendor>=0;}
    int selected()const{return selectedType;}
    bool purchase()const{return buying;}
    unsigned price(unsigned type,bool buy)const{return type>0&&type<ItemTypes?unsigned(buy?itemDefs[type].price:std::max(1,itemDefs[type].price/2)):0;}
    bool accepts(unsigned type)const{return std::find(stock.begin(),stock.end(),unsigned(itemBase(type)))!=stock.end();}
    bool update(const World&,const Npcs&,const Actor&);
    void tap(const World&,const Npcs&,const Actor&,Inventory&,int,int);
};
}
