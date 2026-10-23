// SPDX-License-Identifier: MIT
#pragma once
#include "combat.hpp"
#include "exploration.hpp"
namespace lab {
// Each existing save stays atomic. Attempt every component, reporting partial failure.
inline bool saveSession(const std::string&positionPath,const std::string&vitalsPath,const World&w,const Actor&actor,Inventory&bag,const PlayerStatus&status,std::string&message){
    bool position=savePosition(positionPath,w,actor,message);
    bool items=bag.healthy()&&bag.checkpoint();
    bool vitals=saveVitals(vitalsPath,status);
    bool ok=position&&items&&vitals;
    message=ok?"PARTIDA GUARDADA":"ERROR SD: GUARDADO INCOMPLETO";return ok;
}
}
