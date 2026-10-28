// SPDX-License-Identifier: MIT
#include "region_migration.hpp"
#include "combat.hpp"
#include <fstream>
namespace lab {
bool inheritRegionInventory(const World&oldWorld,const Inventory&oldInventory,const World&newWorld,Inventory&next,std::string&error){
    if(!oldInventory.healthy()||!next.healthy()){error="INVENTARIO ORIGINAL INVALIDO";return false;}
    std::vector<bool>used(next.chests.size());auto contents=next.chests;
    for(const auto&chest:oldInventory.chests){
        const auto&old=oldWorld.objects[chest.object];bool found=false;
        for(size_t i=0;i<contents.size();++i){const auto&candidate=newWorld.objects[contents[i].object];
            if(!used[i]&&old.art==candidate.art&&old.z==candidate.z&&old.x+oldWorld.originX==candidate.x+newWorld.originX&&old.y+oldWorld.originY==candidate.y+newWorld.originY){contents[i].items=chest.items;used[i]=true;found=true;break;}}
        if(!found){error="UN COFRE ORIGINAL NO COINCIDE";return false;}
    }
    next.ground=oldInventory.ground;for(auto&pile:next.ground){pile.position.x+=oldWorld.originX-newWorld.originX;pile.position.y+=oldWorld.originY-newWorld.originY;}
    next.chests=std::move(contents);next.backpack=oldInventory.backpack;next.quests=oldInventory.quests;next.equipment=oldInventory.equipment;next.lastEquipment=oldInventory.lastEquipment;next.dressed=oldInventory.dressed;next.accessories=oldInventory.accessories;error.clear();return true;
}
bool migrateRegion(const std::string&oldRoot,const std::string&newRoot,const World&w,std::string&message){
    if(std::ifstream(newRoot+"campaign.sav").good()||std::ifstream(newRoot+"campaign.sav.bak").good())return true;
    Campaign oldCampaign(oldRoot);if(!oldCampaign.load()){message="PARTIDA ANTERIOR INVALIDA: CONSERVADA";return false;}
    std::string prefix=oldCampaign.prefix();if(!std::ifstream(oldRoot+prefix+"inventory-private.sav").good()&&!std::ifstream(oldRoot+prefix+"inventory-private.sav.bak").good())return true;
    World oldWorld;std::string error;
    if(!loadWorld(oldRoot+"scene.bin",oldWorld,error)){message="NO SE PUDO LEER EL MAPA ANTERIOR";return false;}
    Inventory previous;previous.init(oldWorld,oldRoot+prefix+"inventory-private.sav");if(!previous.healthy()||!previous.restored){message="INVENTARIO ANTERIOR INVALIDO: CONSERVADO";return false;}
    Appearance appearance;if(!appearance.load(oldRoot+prefix+"appearance.sav")){message="ASPECTO ANTERIOR INVALIDO: CONSERVADO";return false;}
    PlayerStatus vitals;if(!loadVitals(oldRoot+prefix+"vitals-private.sav",vitals)){message="VIDA ANTERIOR INVALIDA: CONSERVADA";return false;}
    Actor position{oldWorld.startX,oldWorld.startY,oldWorld.startZ};loadPosition(oldRoot+prefix+"position-private.sav",oldWorld,position,error);
    position.x+=oldWorld.originX-w.originX;position.y+=oldWorld.originY-w.originY;
    if(!w.contains(position.x,position.y)||!canStand(w,position))position={w.startX,w.startY,w.startZ};
    Campaign nextCampaign(newRoot);if(!nextCampaign.load()){message="PARTIDA NUEVA INVALIDA";return false;}auto generation=nextCampaign.unusedGeneration();if(!generation){message="NO SE PUDO IMPORTAR LA PARTIDA";return false;}
    auto target=nextCampaign.prefix(generation);Inventory next;next.init(w,newRoot+target+"inventory-private.sav");next.addAccessories();
    if(!inheritRegionInventory(oldWorld,previous,w,next,error)){message=error+": PARTIDA ORIGINAL CONSERVADA";return false;}
    if(!next.checkpoint()||!appearance.save(newRoot+target+"appearance.sav",error)||!saveVitals(newRoot+target+"vitals-private.sav",vitals)||!savePosition(newRoot+target+"position-private.sav",w,position,error)){
        message="ERROR SD AL IMPORTAR: ORIGINAL CONSERVADO";return false;}
    GameOptions options;if(!std::ifstream(newRoot+"options.sav").good()&&options.load(oldRoot+"options.sav")&&!options.save(newRoot+"options.sav",error)){message="ERROR SD AL IMPORTAR OPCIONES";return false;}
    if(!nextCampaign.commit(generation,error)){message="ERROR SD AL ACTIVAR IMPORTACION";return false;}
    message="PARTIDA IMPORTADA AL MAPA AMPLIADO";return true;
}
}
