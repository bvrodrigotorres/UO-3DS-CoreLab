// SPDX-License-Identifier: MIT
#pragma once
#include "inventory.hpp"
namespace lab {
struct InventoryDrag {
    bool active=false,dragging=false,fromContainer=false;size_t index=0;
    int startX=0,startY=0,x=0,y=0;ItemStack item;const Bag*container=nullptr;
    void cancel(){active=dragging=false;container=nullptr;}
    bool begin(const Inventory&inv,int px,int py){
        cancel();int left=InventoryLayout::slot(px,py,18),right=InventoryLayout::slot(px,py,170);
        const Bag*source=nullptr;
        if(left>=0){index=size_t(inv.bagPage*12+left);source=&inv.backpack;}
        else if(right>=0&&inv.hasContainer()){index=size_t(right);source=inv.containerIdentity();fromContainer=true;}
        if(!source||index>=source->size())return false;
        fromContainer=right>=0;item=(*source)[index];container=inv.containerIdentity();startX=x=px;startY=y=py;active=true;return true;
    }
    void move(int px,int py){x=px;y=py;int dx=x-startX,dy=y-startY;if(dx*dx+dy*dy>=64)dragging=true;}
    bool finish(const World&w,const Actor&a,Inventory&inv,SaveRename renameFile=std::rename){
        if(!active)return false;
        active=false;
        if(!dragging)return false;
        inv.update(w,a);
        const Bag*source=fromContainer?inv.containerIdentity():&inv.backpack;
        if((fromContainer&&source!=container)||!source||index>=source->size()||(*source)[index].type!=item.type||(*source)[index].count!=item.count){inv.message="ARRASTRE CANCELADO: OBJETO CAMBIO";return true;}
        if(InventoryLayout::trash(x,y)){inv.drop(w,a,fromContainer,index,renameFile);return true;}
        bool left=x>=18&&x<150&&y>=38&&y<190,right=x>=170&&x<302&&y>=38&&y<190;
        if((fromContainer&&left)||(!fromContainer&&right)){
            if(inv.containerIdentity()!=container){inv.message="ARRASTRE CANCELADO: CONTENEDOR CAMBIO";return true;}
            inv.transfer(w,a,fromContainer,index,renameFile);
        }else inv.message="ARRASTRE CANCELADO";
        return true;
    }
};
inline bool characterPaperdollHit(int x,int y){return x>=196&&x<306&&y>=25&&y<130;}
}
