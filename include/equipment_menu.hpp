// SPDX-License-Identifier: MIT
#pragma once
#include "inventory.hpp"
#include <algorithm>
namespace lab {
inline constexpr const char*equipmentSlotNames[]={"ARMA","TUNICA","CALZADO","GORRO","CUELLO","PECHO","GUANTES","INFERIOR","MANGAS","ANILLO","BRAZALETE"};
struct EquipmentMenu {
    std::array<int,11>desired;
    std::array<std::vector<int>,11>choices;
    int row=0,direction=4;std::string message;
    explicit EquipmentMenu(const Inventory&inv):desired(inv.equipment){
        for(size_t slot=0;slot<choices.size();++slot){auto&list=choices[slot];list.push_back(-1);
            auto add=[&](int type){if(type>=0&&type<int(ItemTypes)&&itemDefs[type].slot==int(slot)&&std::find(list.begin(),list.end(),type)==list.end())list.push_back(type);};
            add(inv.equipment[slot]);for(auto item:inv.backpack)if(item.count)add(item.type);
        }
    }
    int page()const{return row/6;}
    void move(int delta){row=(row+delta+11)%11;}
    void pageMove(){row=row<6?6:0;}
    void change(int delta){auto&list=choices[row];int size=int(list.size());auto found=std::find(list.begin(),list.end(),desired[row]);int index=found==list.end()?0:int(found-list.begin());desired[row]=list[(index+delta%size+size)%size];message.clear();}
    bool apply(Inventory&bag){bool ok=bag.setOutfit(desired);message=bag.message;return ok;}
    // Touch returns 1=apply, -1=cancel, 0=stay.
    int tap(int x,int y){
        if(y>=216&&y<235){if(x>=18&&x<150)return -1;if(x>=170&&x<303)return 1;}
        if(y>=25&&y<42&&x>=230){pageMove();return 0;}
        if(y>=46&&y<178&&x>=18&&x<303){int selected=page()*6+(y-46)/22;if(selected<11){row=selected;change(x<170?-1:1);}}
        return 0;
    }
};
}
