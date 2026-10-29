// SPDX-License-Identifier: MIT
#pragma once
#include "startup.hpp"
#include "journey.hpp"
namespace lab {
// A menu hint, not authorization to load/write a campaign: full validation still
// happens after selection. Never read geometry, atlases or inventory payloads.
inline bool startupFile(const std::string&path){struct stat s;return ::stat(path.c_str(),&s)==0&&S_ISREG(s.st_mode);}
inline bool startupSave(const std::string&path){return startupFile(path)||startupFile(path+".bak");}
inline std::string startupWorldRoot(const std::string&base){
    for(const auto&folder:{"britain-complete/","britain-realm/"})if(startupFile(base+folder+"ready.bin"))return base+folder;
    for(const auto&folder:{"britain-expanded/","britain-large/"})if(startupFile(base+folder+"scene.bin"))return base+folder;
    return base;
}
struct StartupProbe {
    std::string worldRoot,saveRoot,optionsRoot,prefix;GameOptions options;Journey journey;
    bool canContinue=false,campaignValid=true,journeyValid=true,authoritative=false;std::string message;
    explicit StartupProbe(const std::string&base){
        worldRoot=startupWorldRoot(base);saveRoot=worldRoot;
        auto saved=[](const std::string&r){return startupSave(r+"campaign.sav")||startupSave(r+"inventory-private.sav");};
        if(!saved(saveRoot)){
            if(worldRoot==base+"britain-complete/")saveRoot=saved(base+"britain-realm/")?base+"britain-realm/":base+"britain-expanded/";
            else if(worldRoot==base+"britain-realm/")saveRoot=base+"britain-expanded/";
            else if(worldRoot==base+"britain-expanded/")saveRoot=base+"britain-large/";
            if(!saved(saveRoot))saveRoot=worldRoot;
        }
        Campaign campaign(saveRoot);campaignValid=campaign.load();prefix=campaign.prefix();optionsRoot=startupSave(worldRoot+"options.sav")?worldRoot:saveRoot;options.load(optionsRoot+"options.sav");
        authoritative=saveRoot==worldRoot&&startupSave(worldRoot+"campaign.sav");
        journeyValid=journey.load(saveRoot+prefix+"journey.sav");
        auto tag=!startupFile(worldRoot+"scene.bin")?"demo":journey.zone?"sewers":"private";
        canContinue=campaignValid&&journeyValid&&(startupSave(saveRoot+prefix+"position-"+tag+".sav")||startupSave(saveRoot+prefix+"inventory-"+tag+".sav"));
        if(!campaignValid)message="ERROR AL LEER LA PARTIDA GUARDADA";
        else if(!journeyValid)message="REGISTRO DE VIAJE INVALIDO: PARTIDA CONSERVADA";
        else if(journey.zone&&!startupFile(base+"britain-sewers/ready.bin")){canContinue=false;message="INSTALA EL MAPA DE ALCANTARILLAS";}
    }
    bool directSewers(MenuAction action)const{return action==MenuAction::Continue&&canContinue&&authoritative&&journey.zone==1;}
};
}
