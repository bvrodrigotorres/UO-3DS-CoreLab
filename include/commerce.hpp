// SPDX-License-Identifier: MIT
#pragma once
#include "combat.hpp"
namespace lab {
class Commerce {
    int vendor=-1,selectedType=-1;bool buying=true;std::string identity;
public:
    std::vector<unsigned>stock;std::vector<Skill>lessons;std::string profession;std::string title="SIN COMERCIANTE",message="ACERCATE A UN VENDEDOR";
    int bagPage=0;
    bool available()const{return vendor>=0;}
    int selected()const{return selectedType;}
    bool purchase()const{return buying;}
    unsigned price(unsigned type,bool buy)const{return type>0&&type<ItemTypes?unsigned(buy?itemDefs[type].price:std::max(1,itemDefs[type].price/2)):0;}
    bool accepts(unsigned type)const{return std::find(stock.begin(),stock.end(),unsigned(itemBase(type)))!=stock.end();}
    static bool previewHit(int x,int y){return x>=24&&x<296&&y>=190&&y<222;}
    bool update(const World&,const Npcs&,const Actor&);
    void tap(const World&,const Npcs&,const Actor&,Inventory&,int,int);
};
// Headless state machine: renderer and input share one quote/confirmation state.
struct TradeMenu {
    int tab=0,row=0;bool confirming=false;unsigned quantity=1,maximum=0,unitPrice=0,entry=0;
    std::vector<unsigned>rows;std::string message;
    void refresh(const Commerce&,const Inventory&);
    void switchTab(int,const Commerce&,const Inventory&);
    void move(int);
    unsigned owned(const Inventory&inv,unsigned type)const{unsigned total=0;for(auto s:inv.backpack)if(s.type==type)total+=s.count;return total;}
    bool quote(const Commerce&,const Inventory&,const PlayerStatus&);
    void changeQuantity(int);
    bool execute(const Commerce&,Inventory&,PlayerStatus&,SaveRename=std::rename);
    bool back(){if(confirming){confirming=false;message.clear();return false;}return true;}
};
}
