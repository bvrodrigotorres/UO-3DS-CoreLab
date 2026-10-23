// SPDX-License-Identifier: MIT
#include "commerce.hpp"
#include <cctype>
namespace lab {
namespace {
std::vector<unsigned>catalogue(const Npc&n){
    if(!n.alive||n.animal)return {};
    std::string title=n.title;std::transform(title.begin(),title.end(),title.begin(),[](unsigned char c){return char(std::tolower(c));});
    auto is=[&](const char*s){return title.find(s)!=std::string::npos;};
    if(is("smith")||is("armorer"))return {5,16,17,18,19,20,21,14,15,26,27};
    if(is("jewel"))return {28,29};
    if(is("tailor")||is("weaver"))return {6,7,8,9,10,11,12,13,23};
    if(is("butcher"))return {22};
    if(is("tanner"))return {19,14,15,23,26};
    if(is("bowyer"))return {24};
    if(is("healer")||is("alchemist"))return {3,4,25};
    if(is("baker")||is("cook")||is("innkeeper")||is("tavern keeper")||is("barkeeper"))return {1,2};
    if(is("provisioner")||is("merchant"))return {1,2,3,4,5,7,8,10,12,28,29};
    return {};
}
}
bool Commerce::update(const World&w,const Npcs&npcs,const Actor&a){
    int nearest=-1,best=10000;std::vector<unsigned>next;
    for(size_t i=0;i<npcs.all().size();++i){const auto&n=npcs.all()[i];int dx=n.position.x-a.x,dy=n.position.y-a.y;
        if(std::max(std::abs(dx),std::abs(dy))>1||!meleeReach(w,a,n.position))continue;
        auto items=catalogue(n);int distance=dx*dx+dy*dy;
        if(!items.empty()&&(distance<best||(distance==best&&int(i)==vendor))){best=distance;nearest=int(i);next=std::move(items);}}
    std::string nextIdentity=nearest>=0?npcs.all()[nearest].name+" "+npcs.all()[nearest].title:"";
    bool changed=vendor!=nearest||identity!=nextIdentity;
    if(changed){vendor=nearest;identity=nextIdentity;stock=std::move(next);selectedType=-1;title=vendor>=0?npcs.all()[vendor].name:"SIN COMERCIANTE";message=vendor>=0?"TOCA OBJETO PARA VER PRECIO":"ACERCATE A UN VENDEDOR";}
    return changed;
}
void Commerce::tap(const World&w,const Npcs&npcs,const Actor&a,Inventory&inv,int x,int y){
    update(w,npcs,a);
    if(InventoryLayout::page(x,y)){bagPage=1-bagPage;selectedType=-1;return;}
    if(!available()){message="ACERCATE A UN VENDEDOR";return;}
    int left=InventoryLayout::slot(x,y,18),right=InventoryLayout::slot(x,y,170);
    int type=-1;bool buy=right>=0;
    if(buy&&size_t(right)<stock.size())type=stock[right];
    if(left>=0&&size_t(left+bagPage*12)<inv.backpack.size())type=inv.backpack[left+bagPage*12].type;
    if(type>=0){if(!accepts(unsigned(type))){selectedType=-1;message="NO COMPRA ESTE OBJETO";return;}
        selectedType=type;buying=buy;message=std::string(buy?"COMPRAR ":"VENDER ")+itemDefs[type].name;return;}
    if(x>=170&&x<300&&y>=194&&y<210&&selectedType>0){
        // Re-evaluate proximity before every transaction; no cached vendor access.
        unsigned cost=price(unsigned(selectedType),buying);
        if(inv.trade(buying,unsigned(selectedType),cost))message=std::string(buying?"COMPRADO ":"VENDIDO ")+itemDefs[selectedType].name+(buying?" -$":" +$")+std::to_string(cost);
        else message=inv.message;
    }
}
}
