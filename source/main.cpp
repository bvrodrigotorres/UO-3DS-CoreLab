// SPDX-License-Identifier: MIT
#include "core.hpp"
#include "renderer.hpp"
#include "character_depth.hpp"
#include "performance.hpp"
#include "mount.hpp"
#include "npc_push.hpp"
#include "combat.hpp"
#include "session_save.hpp"
#include "save_queue.hpp"
#include "commerce.hpp"
#include "equipment_menu.hpp"
#include "inventory_drag.hpp"
#include "developer_menu.hpp"
#include "exploration.hpp"
#include "hud.hpp"
#include "region_migration.hpp"
#include "resource_preparation.hpp"
#include "quests.hpp"
#include <3ds.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <malloc.h>
#include <fstream>
#include <functional>
#include <memory>

// libctru main-thread stack: keep explicit headroom for SDK/iostream call chains.
extern "C" { u32 __stacksize__=128*1024; }

namespace {
bool graphicsReady=false;
void startupFrame(const lab::StartMenu*menu,bool canContinue,const std::string&stage="",int progress=0){
    // Complete any outstanding world transfer before CPU-owned title buffers.
    if(graphicsReady)C3D_FrameSync();
    auto top=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,nullptr,nullptr);
    auto bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
    lab::drawTitle(top,menu==nullptr);
    if(menu)lab::drawStartMenu(bottom,*menu,canContinue);else lab::drawLoading(bottom,stage,progress);
    GSPGPU_FlushDataCache(top,400*240*3);GSPGPU_FlushDataCache(bottom,320*240*3);
    gfxScreenSwapBuffers(GFX_TOP,false);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
}
void optionsEditor(lab::GameOptions&options,const std::string&path){
    lab::StartMenu menu;menu.page=lab::MenuPage::Options;menu.options=options;u64 repeat=0;
    while(aptMainLoop()){
        auto fb=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);lab::drawStartMenu(fb,menu,true);GSPGPU_FlushDataCache(fb,320*240*3);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
        hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();u64 now=osGetTime();if(down&(KEY_B|KEY_START))return;
        circlePosition pad;hidCircleRead(&pad);int d=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);if(!d)repeat=0;else if(now>=repeat){menu.move(d);repeat=now+180;}
        bool accept=down&KEY_A;if(down&KEY_TOUCH){touchPosition t;hidTouchRead(&t);int row=menu.hit(t.px,t.py);if(row>=0){menu.selected=row;accept=true;}}
        if(accept){if(menu.selected==3)return;auto previous=menu.options;menu.activate(true);std::string error;if(menu.options.save(path,error)){options=menu.options;menu.message="OPCIONES GUARDADAS";}else{menu.options=previous;menu.message=error;}}
    }
}
bool developerEditor(const lab::World&w,const lab::Npcs&npcs,lab::Actor&landing){
    auto menu=std::make_unique<lab::DeveloperMenu>(w,npcs);u64 repeat=0;
    while(aptMainLoop()){
        auto bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);lab::drawDeveloperMenu(bottom,*menu,w);GSPGPU_FlushDataCache(bottom,320*240*3);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
        hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();u64 now=osGetTime();if(down&KEY_B)return false;
        if(down&(KEY_L|KEY_R))menu->pageMove();
        circlePosition pad;hidCircleRead(&pad);int vertical=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);if(!vertical)repeat=0;else if(now>=repeat){menu->move(vertical);repeat=now+170;}
        bool confirm=down&(KEY_A|KEY_START);if(down&KEY_TOUCH){touchPosition t;hidTouchRead(&t);int action=menu->tap(t.px,t.py);if(action<0)return false;confirm=confirm||action>0;}
        if(confirm&&menu->resolve(w,npcs,landing))return true;
    }return false;
}
bool equipmentEditor(lab::CharacterAssets&assets,const lab::Appearance&appearance,lab::Inventory&bag,const lab::PlayerStatus&status){
    C3D_FrameSync();lab::EquipmentMenu editor(bag);std::vector<uint16_t>doll,sprite;bool changed=true,valid=false;u64 repeat=0;
    while(aptMainLoop()){
        if(changed){unsigned key=lab::Inventory::appearanceKey(editor.desired);valid=assets.compose(appearance,key,doll,true)&&assets.compose(appearance,key,sprite,false,editor.direction*5);if(!valid)editor.message="FALTAN RECURSOS DE EQUIPO";changed=false;}
        auto top=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,nullptr,nullptr),bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
        lab::drawEquipmentMenu(top,bottom,editor,appearance,bag,status,doll,sprite);
        GSPGPU_FlushDataCache(top,400*240*3);GSPGPU_FlushDataCache(bottom,320*240*3);gfxScreenSwapBuffers(GFX_TOP,false);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
        hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();u64 now=osGetTime();if(down&KEY_B)return false;
        if(down&(KEY_L|KEY_R)){editor.pageMove();changed=true;}
        if(down&KEY_X){editor.direction=(editor.direction+1)%8;changed=true;}
        circlePosition pad;hidCircleRead(&pad);int vertical=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35),horizontal=((held&KEY_RIGHT)||pad.dx>35)-((held&KEY_LEFT)||pad.dx< -35);
        if(!vertical&&!horizontal)repeat=0;else if(now>=repeat){if(vertical)editor.move(vertical);else editor.change(horizontal);changed=true;repeat=now+170;}
        if(down&KEY_A){editor.change(1);changed=true;}
        bool apply=down&KEY_START;
        if(down&KEY_TOUCH){touchPosition touch;hidTouchRead(&touch);int action=editor.tap(touch.px,touch.py);if(action<0)return false;apply=apply||action>0;changed=true;}
        if(apply){
            unsigned key=lab::Inventory::appearanceKey(editor.desired);
            valid=assets.compose(appearance,key,doll,true)&&assets.compose(appearance,key,sprite,false,editor.direction*5);
            if(valid&&editor.apply(bag))return true;
            if(!valid)editor.message="FALTAN RECURSOS DE EQUIPO";
        }
    }return false;
}
bool appearanceEditor(lab::CharacterAssets&assets,const lab::Appearance&initial,unsigned equipment,bool fresh,const std::function<bool(const lab::Appearance&,const std::array<int,11>&,std::string&)>&commit,const lab::Inventory*owned=nullptr){
    if(!assets.ready()||!assets.fullPaperdoll())return false;
    C3D_FrameSync();lab::AppearanceEditor editor(initial,fresh);editor.equipment=equipment;if(owned)editor.useWardrobe(*owned);if(!fresh)editor.draft.robe=(equipment&2)?1:0;std::vector<uint16_t>doll,sprite;
    u64 repeat=0;bool changed=true,valid=false;
    while(aptMainLoop()){
        if(changed){
            if(fresh&&editor.draft.realClothes){lab::Inventory initial;initial.initialDress(editor.draft.shirt,editor.draft.pants,editor.draft.shoes,editor.draft.robe);equipment=initial.equipmentMask();}
            if(editor.wardrobe)equipment=editor.equipment;
            unsigned mask=equipment;editor.equipment=equipment;
            valid=assets.compose(editor.draft,mask,doll,true)&&assets.compose(editor.draft,mask,sprite,false,editor.direction*5);
            if(!valid)editor.message="FALTAN ARCHIVOS CHARACTER";
            changed=false;
        }
        auto top=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,nullptr,nullptr),bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
        lab::drawAppearance(top,bottom,editor,doll,sprite,fresh);
        GSPGPU_FlushDataCache(top,400*240*3);GSPGPU_FlushDataCache(bottom,320*240*3);
        gfxScreenSwapBuffers(GFX_TOP,false);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
        hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();u64 now=osGetTime();
        if(down&KEY_B)return false;
        if(down&KEY_L){editor.page=(editor.page+3)%4;editor.row=0;}
        if(down&KEY_R){editor.page=(editor.page+1)%4;editor.row=0;}
        if(down&KEY_X){editor.direction=(editor.direction+1)%8;changed=true;}
        if(down&KEY_Y){editor.randomize(now);changed=true;}
        circlePosition stick;hidCircleRead(&stick);
        int vertical=((held&KEY_DOWN)||stick.dy< -35)-((held&KEY_UP)||stick.dy>35);
        int horizontal=((held&KEY_RIGHT)||stick.dx>35)-((held&KEY_LEFT)||stick.dx< -35);
        if(!vertical&&!horizontal)repeat=0;
        else if(now>=repeat){if(vertical)editor.move(vertical);else{editor.change(horizontal);changed=true;}repeat=now+170;}
        bool confirm=(down&KEY_START)!=0,editName=false;
        if(down&KEY_A){if(editor.page==0&&editor.row==0)editName=true;else{editor.change(1);changed=true;}}
        if(down&KEY_TOUCH){touchPosition t;hidTouchRead(&t);int hit=editor.hit(t.px,t.py);
            if(hit==20)return false;
            if(hit==21)confirm=true;
            if(hit>=10&&hit<=13){editor.page=hit-10;editor.row=0;}
            if(hit>=0&&hit<editor.rowCount()){editor.row=hit;if(editor.page==0&&hit==0)editName=true;else{editor.change(t.px<145?-1:1);changed=true;}}
        }
        if(editName){
            SwkbdState keyboard;char name[65]{};swkbdInit(&keyboard,SWKBD_TYPE_NORMAL,2,16);
            swkbdSetInitialText(&keyboard,editor.draft.name.c_str());swkbdSetHintText(&keyboard,"Nombre: 1 a 16 letras");
            swkbdSetButton(&keyboard,SWKBD_BUTTON_LEFT,"Cancelar",false);swkbdSetButton(&keyboard,SWKBD_BUTTON_RIGHT,"Aceptar",true);
            if(swkbdInputText(&keyboard,name,sizeof name)==SWKBD_BUTTON_RIGHT){if(lab::Appearance::validName(name)){editor.draft.name=name;editor.message.clear();}else editor.message="USA LETRAS A-Z, ESPACIOS O GUIONES";}
            hidScanInput();repeat=0;
        }
        if(confirm){
            if(!valid||!editor.draft.valid()){editor.message="REVISA EL NOMBRE Y LOS RECURSOS";continue;}
            lab::drawLoading(gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr),"GUARDANDO TU PERSONAJE",4);
            auto fb=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);GSPGPU_FlushDataCache(fb,320*240*3);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
            if(commit(editor.draft,editor.outfit,editor.message))return true;
        }
    }
    return false;
}

}

int main(){
    // The software HUD writes BGR bytes directly: use the proven RAM buffers.
    gfxInitDefault();bool new3ds=false;APT_CheckNew3DS(&new3ds);
    // Draw the entire lower page into the hidden buffer, then present it.
    gfxSetDoubleBuffering(GFX_BOTTOM,true);
    startupFrame(nullptr,false,"INICIANDO EL JUEGO",0);
    if(!C3D_Init(lab::FrameCommandBytes)){gfxExit();return 1;}
    graphicsReady=true;
    auto target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH16);
    if(!target||!lab::rendererInit()){consoleInit(GFX_BOTTOM,nullptr);std::printf("CoreLab: GPU init failed\nSTART to exit\n");while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_START)break;}C3D_Fini();gfxExit();return 1;}
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,GX_TRANSFER_FLIP_VERT(0)|GX_TRANSFER_OUT_TILED(0)|GX_TRANSFER_RAW_COPY(0)|GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)|GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    startupFrame(nullptr,false,"CARGANDO BRITANNIA",1);
    lab::World demo=lab::demoWorld(),privateWorld;
    const std::string baseRoot="sdmc:/3ds/UO-3DS-CoreLab/";
    const std::string worldRoot=std::ifstream(baseRoot+"britain-expanded/scene.bin").good()?baseRoot+"britain-expanded/":std::ifstream(baseRoot+"britain-large/scene.bin").good()?baseRoot+"britain-large/":baseRoot;
    std::string error;bool available=lab::loadWorld(worldRoot+"scene.bin",privateWorld,error);
    std::string migrationMessage;if(available&&worldRoot==baseRoot+"britain-expanded/")lab::migrateRegion(baseRoot+"britain-large/",worldRoot,privateWorld,migrationMessage);
    if(available)available=lab::loadAtlases(privateWorld.images,error,worldRoot);
    const lab::World canonicalPrivate=lab::positionSnapshot(privateWorld);
    lab::World legacyPrivate;lab::Doors doors;std::string doorError;
    bool hasDoors=available&&doors.load(worldRoot+"doors.bin",worldRoot+"scene.bin",privateWorld,doorError);
    if(hasDoors)legacyPrivate=lab::positionSnapshot(privateWorld,doors.baseObjects);
    privateWorld.compatiblePosition=canonicalPrivate.compatiblePosition;privateWorld.compatibleInventory=lab::inventoryWorldId(privateWorld);

    startupFrame(nullptr,false,"MAPA Y PERSONAJE",2);
    lab::RoofMode roofs=lab::RoofMode::Auto;
    lab::TouchPages pages;pages.count=6;lab::Journal journal;std::string journalError;
    bool hasJournal=available&&lab::loadJournal(worldRoot+"journal.bin",worldRoot+"scene.bin",journal,journalError);
    const std::string saveRoot=worldRoot;
    lab::Campaign campaign(saveRoot);campaign.load();std::string savePrefix=campaign.prefix();
    bool authentic=available,moving=false;int facing=4,actual=0,frames=0;
    lab::World*w=authentic?&privateWorld:&demo;
    lab::Actor actor={w->startX,w->startY,w->startZ},from=actor;
    float progress=0,cameraX=actor.x,cameraY=actor.y,foot=actor.z,fps=0,animationPhase=0;
    std::string saveMessage;
    lab::PositionCheckpoint checkpoints[2];
    auto positionCheckpoint=[&]()->lab::PositionCheckpoint&{return checkpoints[authentic?1:0];};
    auto positionPath=[&](){return saveRoot+savePrefix+(authentic?"position-private.sav":"position-demo.sav");};
    auto savedWorld=[&]()->const lab::World&{return authentic?canonicalPrivate:demo;};
    auto restorePosition=[&](){
        if(lab::loadPosition(positionPath(),savedWorld(),actor,saveMessage)){positionCheckpoint().baseline(actor,osGetTime());return true;}
        if(authentic&&hasDoors&&lab::loadPosition(positionPath(),legacyPrivate,actor,saveMessage)){
            if(lab::savePosition(positionPath(),canonicalPrivate,actor,saveMessage))saveMessage="Position migrated to decorated world";
            positionCheckpoint().baseline(actor,osGetTime());return true;
        }
        return false;
    };
    auto alignCamera=[&](){from=actor;cameraX=actor.x;cameraY=actor.y;foot=actor.z;progress=0;};
    startupFrame(nullptr,false,"POBLACION DE BRITANNIA",3);
    lab::Npcs npcs;std::string npcError;
    bool hasNpcs=false;
    if(available){
        if(std::ifstream(worldRoot+"population.bin").good())
            hasNpcs=npcs.loadPopulation(worldRoot+"population.bin",worldRoot+"scene.bin",privateWorld,npcError)&&lab::loadPopulationAtlases(npcs,worldRoot,npcError);
        else hasNpcs=lab::loadNpcAtlases(npcError);
    }
    if(hasNpcs&&npcs.hasPopulation())npcs.loadNames(worldRoot+"npc_names.bin",worldRoot+"scene.bin",worldRoot+"population.bin",npcError);
    if(hasNpcs)npcs.reset(privateWorld,actor);
    lab::Inventory inventory;inventory.init(privateWorld,saveRoot+savePrefix+"inventory-private.sav");
    lab::Inventory demoInventory;demoInventory.init(demo,saveRoot+savePrefix+"inventory-demo.sav");
    lab::ItemIcons itemIcons;itemIcons.load("sdmc:/3ds/UO-3DS-CoreLab/items.bin");lab::loadGroundIcons(itemIcons);
    startupFrame(nullptr,false,"MOCHILA Y EQUIPAMIENTO",4);
    lab::EquipmentArt equipmentArt;std::string equipmentError;
    bool hasEquipment=equipmentArt.load("sdmc:/3ds/UO-3DS-CoreLab/equipment.bin");
    if(!hasEquipment)equipmentError="FALTA EQUIPMENT.BIN";
    lab::CharacterAssets characterAssets;characterAssets.load(baseRoot+"character/");
    lab::Appearance appearance;
    bool appearanceOk=appearance.load(saveRoot+savePrefix+"appearance.sav");
    lab::StatusArt statusArt;statusArt.load(baseRoot+"status.bin");
    std::string combatArtError;if(hasNpcs&&npcs.hasPopulation()){if(!npcs.hasDeathFrames())lab::loadCombatArt(baseRoot,worldRoot+"population.bin",combatArtError);std::string bloodError;if(!lab::loadBloodArt(baseRoot,bloodError))combatArtError=bloodError;}
    startupFrame(nullptr,false,"PREPARADO",5);
    lab::StartMenu menu;menu.options.load(saveRoot+"options.sav");menu.message=migrationMessage;
    lab::Actor probe;std::string probeMessage;
    bool positionSaved=lab::loadPosition(positionPath(),savedWorld(),probe,probeMessage);
    if(!positionSaved&&authentic&&hasDoors)positionSaved=lab::loadPosition(positionPath(),legacyPrivate,probe,probeMessage);
    bool canContinue=appearanceOk&&campaign.valid&&(authentic?inventory.healthy():demoInventory.healthy())&&(positionSaved||(authentic?inventory.restored:demoInventory.restored));
    menu.selected=canContinue?1:0;
    if(!appearanceOk)menu.message="PERSONALIZACION GUARDADA INVALIDA";
    else if(!campaign.valid)menu.message="ERROR AL LEER LA PARTIDA GUARDADA";
    else if(!(authentic?inventory.healthy():demoInventory.healthy()))menu.message="ERROR EN INVENTARIO GUARDADO";
    auto freshGame=[&](const lab::Appearance&created){
        uint32_t generation=campaign.unusedGeneration();
        if(!generation){menu.message="NO SE PUDO PREPARAR LA PARTIDA";return false;}
        std::string prefix=campaign.prefix(generation),error;
        lab::Inventory freshPrivate,freshDemo;
        freshDemo.init(demo,saveRoot+prefix+"inventory-demo.sav");
        freshDemo.initialDress(created.shirt,created.pants,created.shoes,created.robe);freshDemo.addAccessories();
        if(!freshDemo.checkpoint()||!lab::savePosition(saveRoot+prefix+"position-demo.sav",demo,{demo.startX,demo.startY,demo.startZ},error)){
            menu.message="ERROR SD: PARTIDA ANTERIOR CONSERVADA";return false;
        }
        if(available){
            freshPrivate.init(privateWorld,saveRoot+prefix+"inventory-private.sav");
            freshPrivate.initialDress(created.shirt,created.pants,created.shoes,created.robe);freshPrivate.addAccessories();
            if(!freshPrivate.checkpoint()||!lab::savePosition(saveRoot+prefix+"position-private.sav",canonicalPrivate,{canonicalPrivate.startX,canonicalPrivate.startY,canonicalPrivate.startZ},error)){
                menu.message="ERROR SD: PARTIDA ANTERIOR CONSERVADA";return false;
            }
        }
        if(!created.save(saveRoot+prefix+"appearance.sav",error)){menu.message=error;return false;}
        if(!campaign.commit(generation,error)){menu.message=error;return false;}
        appearance=created;savePrefix=prefix;inventory=std::move(freshPrivate);demoInventory=std::move(freshDemo);
        if(hasDoors)doors.reset(privateWorld);
        if(available)hasDoors=doors.load(saveRoot+"doors.bin",saveRoot+"scene.bin",privateWorld,doorError);
        actor={w->startX,w->startY,w->startZ};alignCamera();lab::invalidateRenderList();
        checkpoints[0].baseline({demo.startX,demo.startY,demo.startZ},osGetTime());
        if(available)checkpoints[1].baseline({canonicalPrivate.startX,canonicalPrivate.startY,canonicalPrivate.startZ},osGetTime());
        saveMessage="NUEVA AVENTURA EN BRITANNIA";return true;
    };
    bool quit=false;
    while(!quit){
    bool started=false,touching=false;int touchX=0,touchY=0,touchLastX=0,touchLastY=0;
    u64 nextPad=0;
    while(aptMainLoop()&&!started&&!quit){
        startupFrame(&menu,canContinue);hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();
        if(down&KEY_START){quit=true;continue;}
        if(down&KEY_B)menu.back();
        circlePosition pad;hidCircleRead(&pad);u64 now=osGetTime();
        int direction=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);
        if(!direction)nextPad=0;
        else if(now>=nextPad){menu.move(direction);nextPad=now+180;}
        bool accept=(down&KEY_A)!=0;
        if(down&KEY_TOUCH){touchPosition t;hidTouchRead(&t);touchX=touchLastX=t.px;touchY=touchLastY=t.py;touching=true;}
        if((held&KEY_TOUCH)&&touching){touchPosition t;hidTouchRead(&t);touchLastX=t.px;touchLastY=t.py;}
        if((hidKeysUp()&KEY_TOUCH)&&touching){touching=false;int hit=menu.hit(touchX,touchY);if(hit>=0&&std::abs(touchLastX-touchX)<12&&std::abs(touchLastY-touchY)<12){menu.selected=hit;accept=true;}}
        if(!accept)continue;
        auto oldOptions=menu.options;auto action=menu.activate(canContinue);
        if(menu.options.roofs!=oldOptions.roofs||menu.options.zoom!=oldOptions.zoom||menu.options.page!=oldOptions.page){
            std::string message;if(!menu.options.save(saveRoot+"options.sav",message)){menu.options=oldOptions;menu.message=message;}else menu.message="OPCIONES GUARDADAS";
        }
        if(action==lab::MenuAction::Exit){quit=true;continue;}
        if(action==lab::MenuAction::NewGame){
            if(!characterAssets.ready()||!characterAssets.fullPaperdoll()){
                if(!available){lab::Appearance diagnostic;started=freshGame(diagnostic);continue;}
                menu.message="ACTUALIZA CHARACTER A V0.25";continue;
            }
            lab::Appearance created;created.enabled=true;created.realClothes=true;
            started=appearanceEditor(characterAssets,created,0,true,[&](const lab::Appearance&a,const std::array<int,11>&,std::string&message){
                std::vector<uint8_t>dolls;std::vector<uint16_t>frames;
                if(!characterAssets.dolls(a,dolls)||!characterAssets.compose(a,7,frames)){message="FALTAN CAPAS DE EQUIPAMIENTO";return false;}
                bool ok=freshGame(a);if(!ok)message=menu.message;return ok;
            });
            if(!started)menu.back();
        }
        if(action==lab::MenuAction::Continue){if(restorePosition())alignCamera();started=true;}
    }
    if(!started)break;
    moving=false;alignCamera();
    startupFrame(nullptr,false,"ENTRANDO EN BRITANNIA",5);
    roofs=static_cast<lab::RoofMode>(menu.options.roofs);pages.zoom=menu.options.zoom;pages.page=menu.options.page;
    if(hasNpcs)npcs.reset(privateWorld,actor);
    if(appearance.enabled&&characterAssets.ready()){
        auto&bag=authentic?inventory:demoInventory;
        if(!bag.dressed||!bag.accessories){
            auto oldBag=bag;bag.initialDress(appearance.shirt,appearance.pants,appearance.shoes);bag.addAccessories();
            if(!bag.checkpoint()){bag=oldBag;menu.back();menu.message="ERROR AL MIGRAR VESTIMENTA";continue;}
        }
        if(!appearance.realClothes){auto updated=appearance;updated.realClothes=true;std::string migrationError;
            if(!updated.save(saveRoot+savePrefix+"appearance.sav",migrationError)){menu.back();menu.message=migrationError;continue;}appearance=updated;}
    }
    if(appearance.enabled){
        if(!characterAssets.fullPaperdoll()){menu.back();menu.message="ACTUALIZA CHARACTER A V0.25";continue;}
        if(!lab::applyAppearance(characterAssets,appearance,equipmentArt,inventory.equipmentMask(),equipmentError)){
            menu.back();menu.message=equipmentError;continue;
        }
        hasEquipment=true;
    }else {lab::clearAppearance();hasEquipment=equipmentArt.load(baseRoot+"equipment.bin");}
    if(hasEquipment){
        startupFrame(nullptr,false,"PREPARANDO EQUIPAMIENTO",5);
        lab::warmEquipment(inventory.equipmentMask(),equipmentArt);
        lab::prepareEquipment(inventory.equipmentMask(),equipmentArt,equipmentError);
    }
    std::vector<lab::Actor>occupants;occupants.reserve(npcs.all().size()*2+2);
    lab::PlayerStatus playerStatus;lab::NpcPush npcPush;lab::Commerce commerce;lab::Combat combat;combat.reset(npcs);combat.locate(actor);
    lab::Quests quests;quests.setup(*w,npcs);lab::setQuestMarker({});
    combat.onKill=[&](const lab::Npc&npc){if(authentic)quests.killed(*w,npc,inventory);};
    auto vitalsPath=[&](){return saveRoot+savePrefix+(authentic?"vitals-private.sav":"vitals-demo.sav");};
    lab::AsyncSaves*asyncWriter=nullptr;
    auto persistVitals=[&](){if(asyncWriter&&!asyncWriter->flush()){saveMessage="ERROR SD: GUARDADO PENDIENTE";return false;}bool ok=lab::saveVitals(vitalsPath(),playerStatus);if(!ok)saveMessage="ERROR SD: ESTADISTICAS NO GUARDADAS";return ok;};
    if(!lab::loadVitals(vitalsPath(),playerStatus)){menu.back();menu.message="ESTADISTICAS INVALIDAS: PARTIDA CONSERVADA";continue;}
    auto saves=std::make_unique<lab::AsyncSaves>();bool asyncReady=saves->start();
    lab::SaveSink sink;if(asyncReady){asyncWriter=saves.get();sink=[&](const std::string&path,std::vector<uint8_t>b){return saves->enqueue(path,std::move(b));};inventory.saveSink=sink;demoInventory.saveSink=sink;}
    auto queueVitals=[&](){bool ok=lab::saveVitals(vitalsPath(),playerStatus,std::rename,sink);if(!ok)saveMessage="ERROR SD: ESTADISTICAS PENDIENTES";return ok;};
    inventory.useItem=[&](size_t index){bool used=combat.useHealing(inventory,index,playerStatus);if(used&&!queueVitals())inventory.message="CURADO | VIDA PENDIENTE DE GUARDAR";return used;};
    inventory.prepareEquipment=[&](unsigned mask){unsigned key=mask|(playerStatus.mounted?lab::MountedBank:0);if(!lab::prepareEquipment(key,equipmentArt,equipmentError))return false;lab::warmEquipment(key,equipmentArt);return true;};
    if(playerStatus.mounted&&!inventory.prepareEquipment(inventory.equipmentMask())){playerStatus.mounted=false;saveMessage="CABALLO: ACTUALIZA LA CARPETA CHARACTER/MOUNTED";}
    lab::SkillsMenu skillsMenu;
    playerStatus.skills.start((authentic?inventory:demoInventory).equipment[0]);
    if(authentic){u64 warmUntil=osGetTime()+10000;lab::ResourcePreparation preparation;while(aptMainLoop()&&osGetTime()<warmUntil&&preparation.pending()){
        lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,0,true,false,false,hasNpcs?&npcs:nullptr,&combat,&playerStatus,&inventory,false);
        preparation.observe(lab::rendererResourceWaits(),lab::rendererTextureError());startupFrame(nullptr,false,"PREPARANDO LA REGION",5);
    }}
    lab::InventoryDrag inventoryDrag;lab::PauseMenu pause;u64 pauseAt=0,pausePad=0;
    lab::ResourcePreparation pausedResources;
    auto saveAtPause=[&](u64 at){auto&bag=authentic?inventory:demoInventory;bool ok=lab::saveSession(positionPath(),vitalsPath(),savedWorld(),actor,bag,playerStatus,saveMessage,sink);positionCheckpoint().result(actor,at,ok);pause.message=saveMessage;};
    auto presentPause=[&](){
        lab::setQuestMarker(authentic?quests.marker(npcs,inventory):lab::QuestMarker{});
        // Pump only rendering/I/O. Never advance NPCs, combat, doors or timers
        // while paused, including after a developer teleport.
        if(pausedResources.pending()){
            lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,int(animationPhase),authentic,false,false,authentic&&hasNpcs?&npcs:nullptr,authentic?&combat:nullptr,&playerStatus,authentic?&inventory:nullptr);
            pausedResources.observe(lab::rendererResourceWaits(),lab::rendererTextureError());
        }
        auto bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);lab::drawPause(bottom,pause);GSPGPU_FlushDataCache(bottom,320*240*3);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();if(pause.presented())saveAtPause(pauseAt);
    };
    lab::FramePacer framePacer;lab::FrameStats frameStats;u64 lastHud=0;
    u64 last=osGetTime(),sample=last;positionCheckpoint().active(last);int sampleFrames=0;const char*blocked="--";
    while(aptMainLoop()){
        if(!new3ds&&!pause.active){auto delay=framePacer.delay(osGetTime()*1000);if(delay)svcSleepThread(delay*1000);}else framePacer.reset();
        u64 now=osGetTime();frameStats.observe(float(now-last));float dt=std::min(.05f,(now-last)/1000.f);last=now;
        hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();
        auto saveState=saves->status();bool saveNotice=inventory.savePending!=(saveState.pending>0)||inventory.saveFailed!=saveState.error;inventory.savePending=saveState.pending>0;inventory.saveFailed=saveState.error;if(saveState.error)saveMessage="ERROR SD: GUARDADO PENDIENTE";
        combat.locate(actor);
        bool wasPaused=pause.active;
        if((down&KEY_START)&&!pause.active){pause.open();pausedResources.restart();pauseAt=now;pages.active=false;inventoryDrag.cancel();presentPause();last=osGetTime();continue;}
        else if((down&KEY_START)&&pause.active)pause.active=false;
        if(pause.active||wasPaused){
            pause.holdDeveloper((held&(KEY_L|KEY_R))==(KEY_L|KEY_R),now);
            if(saveState.pending&&!saveState.error&&(pause.message.empty()||pause.message=="PARTIDA GUARDADA"))pause.message="GUARDANDO PARTIDA...";
            if(pause.message=="GUARDANDO PARTIDA..."&&!saveState.pending&&!saveState.error)pause.message="PARTIDA GUARDADA";
            if(saveState.error)pause.message="ERROR SD: GUARDADO PENDIENTE";
            lab::PauseAction action=lab::PauseAction::None;
            if(!pause.active||(down&KEY_B))action=lab::PauseAction::Resume;
            if(pause.active){
                circlePosition stick;hidCircleRead(&stick);
                int direction=((held&KEY_DOWN)||stick.dy< -35)-((held&KEY_UP)||stick.dy>35);
                if(!direction)pausePad=0;
                else if(now>=pausePad){pause.move(direction);pausePad=now+180;}
                if(down&KEY_A)action=pause.activate();
                if(down&KEY_TOUCH){touchPosition t;hidTouchRead(&t);int row=pause.hit(t.px,t.py);if(row>=0){pause.selected=row;action=pause.activate();}}
            }
            if(action==lab::PauseAction::Resume){
                pause.active=false;positionCheckpoint().suspend(now-pauseAt);pages.active=false;frames=0;
            }else if(action==lab::PauseAction::Equipment){
                auto&bag=authentic?inventory:demoInventory;
                if(!appearance.enabled||!characterAssets.ready())pause.message="PERSONALIZA EL PERSONAJE PRIMERO";
                else equipmentEditor(characterAssets,appearance,bag,playerStatus);
                now=osGetTime();pausePad=0;frames=0;
                lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,int(animationPhase),authentic,false,false,authentic&&hasNpcs?&npcs:nullptr,authentic?&combat:nullptr,&playerStatus,authentic?&inventory:nullptr);
            }else if(action==lab::PauseAction::Options){
                menu.options.roofs=int(roofs);menu.options.zoom=pages.zoom;menu.options.page=pages.page;
                optionsEditor(menu.options,saveRoot+"options.sav");roofs=static_cast<lab::RoofMode>(menu.options.roofs);pages.zoom=menu.options.zoom;pages.page=menu.options.page;lab::invalidateRenderList();
                now=osGetTime();pausePad=0;frames=0;
                lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,int(animationPhase),authentic,false,false,authentic&&hasNpcs?&npcs:nullptr,authentic?&combat:nullptr,&playerStatus,authentic?&inventory:nullptr);
            }else if(action==lab::PauseAction::Developer){
                lab::Actor destination;if(developerEditor(*w,npcs,destination)){
                    actor=destination;from=actor;moving=false;progress=1;animationPhase=0;alignCamera();npcPush.reset();inventoryDrag.cancel();inventory.closeContainer();combat.attacking=false;combat.target=-1;
                    for(auto&n:npcs.combatants())n.hostile=false;
                    lab::invalidateRenderList();saveMessage="TELETRANSPORTE COMPLETADO";
                    bool saved=lab::savePosition(positionPath(),savedWorld(),actor,saveMessage,std::rename,sink)&&persistVitals();positionCheckpoint().result(actor,pauseAt,saved);
                }
                now=osGetTime();pausePad=0;frames=0;
                lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,int(animationPhase),authentic,false,false,authentic&&hasNpcs?&npcs:nullptr,authentic?&combat:nullptr,&playerStatus,authentic?&inventory:nullptr);
            }else if(action==lab::PauseAction::Customize){
                auto&bag=authentic?inventory:demoInventory;
                if(!characterAssets.ready()||!characterAssets.fullPaperdoll())pause.message="ACTUALIZA CHARACTER A V0.25";
                else appearanceEditor(characterAssets,appearance,bag.equipmentMask(),false,[&](const lab::Appearance&a,const std::array<int,11>&outfit,std::string&message){
                    const auto previous=appearance;unsigned key=lab::Inventory::appearanceKey(outfit);
                    if(!lab::applyAppearance(characterAssets,a,equipmentArt,key,message))return false;
                    auto restore=[&](){std::string ignored;if(previous.enabled)lab::applyAppearance(characterAssets,previous,equipmentArt,bag.equipmentMask(),ignored);
                        else {lab::clearAppearance();equipmentArt.load(baseRoot+"equipment.bin");lab::prepareEquipment(bag.equipmentMask(),equipmentArt,ignored);}
                        lab::warmEquipment(bag.equipmentMask()|(playerStatus.mounted?lab::MountedBank:0),equipmentArt);};
                    if(playerStatus.mounted&&!lab::prepareEquipment(key|lab::MountedBank,equipmentArt,message)){restore();return false;}
                    const auto appearancePath=saveRoot+savePrefix+"appearance.sav";
                    if(!a.save(appearancePath,message)){restore();return false;}
                    if(!bag.setOutfit(outfit)){
                        message=bag.message;std::string rollback;
                        if(!previous.save(appearancePath,rollback))message="ERROR SD: REVISA GUARDADO DE ASPECTO";
                        restore();return false;
                    }
                    appearance=a;hasEquipment=true;lab::warmEquipment(bag.equipmentMask()|(playerStatus.mounted?lab::MountedBank:0),equipmentArt);return true;
                },&bag);
                now=osGetTime();pausePad=0;frames=0;
                lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,int(animationPhase),authentic,false,false,authentic&&hasNpcs?&npcs:nullptr,authentic?&combat:nullptr,&playerStatus,authentic?&inventory:nullptr);
            }else if(action==lab::PauseAction::MainMenu||action==lab::PauseAction::Exit){
                bool saved=true;
                if(positionCheckpoint().dirty(actor)){saved=lab::savePosition(positionPath(),savedWorld(),actor,saveMessage,std::rename,sink);positionCheckpoint().result(actor,pauseAt,saved);}
                auto&bag=authentic?inventory:demoInventory;
                if(saved&&bag.healthy())saved=bag.checkpoint();
                if(saved)saved=persistVitals();
                if(!saves->flush())saved=false;
                if(saved){inventory.closeContainer();C3D_FrameSync();quit=action==lab::PauseAction::Exit;menu.back();menu.selected=1;canContinue=true;moving=false;break;}
                pause.message="ERROR SD: NO SE PUDO GUARDAR";
            }
            if(action!=lab::PauseAction::None&&action!=lab::PauseAction::Resume)pausedResources.restart();
            last=sample=now;sampleFrames=0;
            presentPause();
            continue;
        }
        if(down&(KEY_X|KEY_A))npcPush.reset();
        if(down&KEY_SELECT)roofs=lab::nextRoofMode(roofs);
        if(authentic&&(down&KEY_Y)){bool used=combat.repeatHealing(inventory,playerStatus);if(used&&!queueVitals())inventory.message="CURADO | VIDA PENDIENTE DE GUARDAR";saveMessage=inventory.message;}
        bool pageChanged=false;
        if(down&(KEY_R|KEY_L)){pages.page=(pages.page+((down&KEY_R)?1:pages.count-1))%pages.count;pageChanged=true;pages.active=false;inventoryDrag.cancel();}
        touchPosition touch;
        if(held&KEY_TOUCH){hidTouchRead(&touch);if(down&KEY_TOUCH){pages.begin(touch.px,touch.py);if(pages.page==2&&authentic)inventoryDrag.begin(inventory,touch.px,touch.py);}else {pages.move(touch.px,touch.py);if(inventoryDrag.active)inventoryDrag.move(touch.px,touch.py);}}
        if((hidKeysUp()&KEY_TOUCH)&&pages.active){
            int oldPage=pages.page,dx=pages.lastX-pages.startX,dy=pages.lastY-pages.startY;
            bool dragged=false,changed=false;
            if(inventoryDrag.active){dragged=inventoryDrag.finish(*w,actor,inventory);pages.active=false;}
            else changed=pages.end();
            if(!dragged&&!changed&&oldPage==2&&authentic&&dx*dx+dy*dy<144)
                inventory.tap(*w,actor,pages.startX,pages.startY);
            if(!changed&&oldPage==1&&authentic&&dx*dx+dy*dy<144){
                if(lab::horseIconHit(pages.startX,pages.startY)){
                    if(moving)saveMessage="DETENTE PARA MONTAR O DESMONTAR";
                    else if(!appearance.enabled)saveMessage="PERSONALIZA EL PERSONAJE PRIMERO";
                    else {
                        auto fb=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);lab::drawLoading(fb,"PREPARANDO CABALLO",4);GSPGPU_FlushDataCache(fb,320*240*3);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
                        bool mounted=!playerStatus.mounted;unsigned key=inventory.equipmentMask()|(mounted?lab::MountedBank:0);
                        if(lab::prepareEquipment(key,equipmentArt,equipmentError)){playerStatus.mounted=mounted;lab::warmEquipment(key,equipmentArt);animationPhase=0;queueVitals();saveMessage=mounted?"A CABALLO | B: GALOPAR":"HAS DESMONTADO";}
                        else saveMessage="CABALLO: REVISA CHARACTER/MOUNTED";
                        last=sample=osGetTime();frames=0;
                    }
                }else if(lab::characterPaperdollHit(pages.startX,pages.startY)){
                    pause.open();pausedResources.restart();pause.selected=2;pauseAt=now;presentPause();
                    if(appearance.enabled&&characterAssets.ready())equipmentEditor(characterAssets,appearance,inventory,playerStatus);
                    else pause.message="PERSONALIZA EL PERSONAJE PRIMERO";
                    pausedResources.restart();
                    now=osGetTime();last=sample=now;pausePad=0;frames=0;
                    lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,int(animationPhase),authentic,false,false,authentic&&hasNpcs?&npcs:nullptr,authentic?&combat:nullptr,&playerStatus,authentic?&inventory:nullptr);
                    continue;
                }
            }
            if(!changed&&oldPage==4&&dx*dx+dy*dy<144){
                unsigned selected=unsigned(skillsMenu.selected);auto previous=playerStatus.skills.entries;int action=skillsMenu.tap(playerStatus.skills,pages.startX,pages.startY);
                if(action==1){if(!persistVitals()){playerStatus.skills.entries=previous;skillsMenu.message="ERROR SD: CAMBIO CANCELADO";}else skillsMenu.message.clear();}
                else if(action==2){if(moving)skillsMenu.message="DETENTE PARA MEDITAR";else {if(combat.meditate(playerStatus,inventory))skillsMenu.message.clear();else skillsMenu.message=combat.message;}}
                else if(selected!=unsigned(skillsMenu.selected))skillsMenu.message.clear();
            }
            if(!changed&&oldPage==3&&authentic&&dx*dx+dy*dy<144)commerce.tap(*w,npcs,actor,inventory,pages.startX,pages.startY);
            if(!changed&&oldPage==5&&authentic&&dx*dx+dy*dy<144){
                if(pages.startY>=30&&pages.startY<52&&pages.startX>=18&&pages.startX<302)quests.select(pages.startX>=162?1:0);
                if(pages.startY>=201&&pages.startY<226&&pages.startX>=18&&pages.startX<302)quests.interact(*w,npcs,actor,inventory);
            }
            pageChanged=true;
        }
        if(authentic)inventory.tickGround(dt);
        if(authentic)inventory.update(*w,actor);else inventory.opened=-1;
        if(authentic&&(down&KEY_X))combat.select(npcs,actor,true);
        if(authentic&&(down&KEY_A)){if(pages.page==5)quests.interact(*w,npcs,actor,inventory);else combat.toggle(npcs,actor);}
        if(authentic){
            occupants.clear();occupants.push_back(actor);if(moving)occupants.push_back(from);npcs.addOccupants(occupants);
            if(doors.update(privateWorld,dt,occupants))lab::invalidateRenderList();
            if(hasNpcs)npcs.update(privateWorld,actor,from,moving,dt,doors);
        }
        circlePosition pad;hidCircleRead(&pad);
        int sx=((held&KEY_RIGHT)||pad.dx>35)-((held&KEY_LEFT)||pad.dx< -35);
        int sy=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);
        if(!sx&&!sy)npcPush.reset();
        if(!moving&&(sx||sy)){
            int dx=std::max(-1,std::min(1,sx+sy)),dy=std::max(-1,std::min(1,sy-sx));
            const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};
            int direction=0;for(int i=0;i<8;++i)if(xs[i]==dx&&ys[i]==dy)direction=i;
            // Turning responds to intent even when movement is blocked.
            facing=(direction+1)&7;
            lab::Actor next;
            if(authentic&&doors.tryOpen(actor,dx,dy)){npcPush.reset();blocked="OPENING DOOR";}
            else if(lab::step(*w,actor,direction,next,actual)){
                if(!npcPush.allow(authentic&&npcs.occupied(next),actor,next,direction,dt)){blocked="NPC: INSISTE PARA PASAR";}else{
                from=actor;actor=next;moving=true;progress=0;facing=(actual+1)&7;blocked="--";
                }
            }else {npcPush.reset();blocked="BLOCKED";}
        }
        bool animateThisFrame=moving,running=(held&KEY_B)!=0;
        if(moving){progress+=dt/lab::movementSeconds(running,playerStatus.mounted);if(progress>=1){progress=1;moving=false;}
            cameraX=from.x+(actor.x-from.x)*progress;cameraY=from.y+(actor.y-from.y)*progress;foot=from.z+(actor.z-from.z)*progress;}
        if(authentic&&hasNpcs&&combat.update(*w,npcs,actor,moving,dt,playerStatus,inventory)){
            actor={w->startX,w->startY,w->startZ};moving=false;alignCamera();npcPush.reset();frames=0;animateThisFrame=false;lab::warmEquipment(inventory.equipmentMask(),equipmentArt);
        }
        if(combat.attacking&&combat.target>=0&&!moving){
            const auto&enemy=npcs.all()[combat.target];int tx=(enemy.position.x>actor.x)-(enemy.position.x<actor.x),ty=(enemy.position.y>actor.y)-(enemy.position.y<actor.y);
            const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};for(int d=0;d<8;++d)if(xs[d]==tx&&ys[d]==ty)facing=(d+1)&7;
        }
        if(authentic&&pages.page==3&&commerce.update(*w,npcs,actor))pageChanged=true;
        if(authentic&&!inventoryDrag.active&&pages.page==2&&combat.inspectNearest(*w,actor,inventory))pageChanged=true;
        if(animateThisFrame||sx||sy||(held&KEY_TOUCH)||down)positionCheckpoint().active(now);
        if(!moving&&positionCheckpoint().due(actor,now)){
            bool ok=lab::savePosition(positionPath(),savedWorld(),actor,saveMessage,std::rename,sink)&&queueVitals();
            positionCheckpoint().result(actor,now,ok);
        }
        if(animateThisFrame)animationPhase=std::fmod(animationPhase+dt*(playerStatus.mounted?(running?15.f:10.f):(running?13.f:8.f)),playerStatus.mounted?5.f:4.f);
        int animation=(int)animationPhase;
        if(authentic&&hasEquipment){
            unsigned key=inventory.equipmentMask()|(playerStatus.mounted?lab::MountedBank:0);
            // No SD reads, CPU composition or GPU allocations in an attack frame.
            if(combat.combatPose()&&!moving&&appearance.enabled&&!playerStatus.mounted){
                if(!lab::selectEquipment(key|4096))lab::selectEquipment(key);
            }else lab::selectEquipment(key);
        }
        lab::setQuestMarker(authentic?quests.marker(npcs,inventory):lab::QuestMarker{});
        lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,animation,authentic,animateThisFrame,running,authentic&&hasNpcs?&npcs:nullptr,authentic?&combat:nullptr,&playerStatus,authentic?&inventory:nullptr);
        ++frames;++sampleFrames;if(now-sample>=500){fps=sampleFrames*1000.f/(now-sample);sample=now;sampleFrames=0;}
        if(frames==1||now-lastHud>=200||down||pageChanged||saveNotice){
            lastHud=now;
            lab::HudInfo hud{authentic?(w->width>32?"BRITAIN "+std::to_string(w->width)+"X"+std::to_string(w->height):"PRIVATE UO SCENE"):"SYNTHETIC TEST",lab::roofModeName(roofs),{}};
            if(pages.page==0){
            char line[160];
            auto add=[&](){hud.lines.emplace_back(line);};
            std::snprintf(line,sizeof(line),"XY %d,%d   Z %d | %s",actor.x+w->originX,actor.y+w->originY,actor.z,blocked);add();
            std::snprintf(line,sizeof(line),"FPS %.1f DRAW %lu CMD %.0f%%",fps,(unsigned long)lab::drawnObjects(),C3D_GetCmdBufUsage()*100.f);add();
            std::snprintf(line,sizeof(line),"CPU %.2fMS | GPU %.2fMS",C3D_GetProcessingTime(),C3D_GetDrawingTime());add();
            {
                auto heap=mallinfo();
                std::snprintf(line,sizeof(line),"ATLAS %.2F MIB | LISTS %u",lab::textureBytes()/1048576.f,lab::renderListBuilds());add();
                std::snprintf(line,sizeof(line),"LINEAR FREE %.2F MIB",linearSpaceFree()/1048576.f);add();
                std::snprintf(line,sizeof(line),"HEAP USED %.2F MIB",heap.uordblks/1048576.f);add();
                std::snprintf(line,sizeof(line),"HEAP REUSABLE %.2F MIB",heap.fordblks/1048576.f);add();
                std::snprintf(line,sizeof(line),"RUN ART: %s",lab::hasRunAnimation()?"ACTION 2 VERIFIED":"UNVERIFIED (WALK)");add();
                if(authentic&&hasDoors){std::snprintf(line,sizeof(line),"DECOR %lu | DOORS %lu",(unsigned long)(privateWorld.objects.size()-doors.baseObjects-doors.count()),(unsigned long)doors.count());add();}
                else hud.lines.emplace_back("");

            }
            if(authentic&&hasNpcs){
                auto greeting=npcs.greeting(actor);
                hud.lines.push_back(greeting.empty()?"NPC "+std::to_string(npcs.all().size())+" | POBLACION LOCAL":greeting);
            }
            if(authentic&&hasNpcs)hud.lines.emplace_back(lab::rendererNpcPoseStreaming()?"NPC SPRITES: CACHE 4 MIB":"NPC SPRITES: LEGACY (SIN POSES)");
            if(!combatArtError.empty())hud.lines.push_back(combatArtError);
            if(!combat.message.empty())hud.lines.push_back(combat.message);
            if(!quests.message.empty())hud.lines.push_back(quests.message);
            if(playerStatus.skills.notice>0)hud.lines.push_back(playerStatus.skills.message);
            if(!saveMessage.empty())hud.lines.push_back(saveMessage);
            if(authentic&&!equipmentError.empty())hud.lines.push_back(equipmentError);
            if(!available)hud.lines.push_back(error);
            if(authentic&&!hasDoors)hud.lines.push_back(doorError);
            if(authentic&&!npcError.empty())hud.lines.push_back(npcError);
            if(lab::rendererTextureError())hud.lines.emplace_back("TEXTURAS: ERROR SD/MEMORIA");
            if(lab::rendererOverflow())hud.lines.emplace_back("ERROR: VERTEX BUDGET EXCEEDED");
            std::snprintf(line,sizeof(line),"P95 %.0fMS SD %u %.0fMS",frameStats.p95(),saveState.pending,saveState.lastMilliseconds);add();
            if(saveState.error)hud.lines.emplace_back("ERROR SD: GUARDADO PENDIENTE");
            }
            auto bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
            if(pages.page==5)lab::drawQuests(bottom,quests.panel(*w,npcs,actor,inventory));
            else if(pages.page==4)lab::drawSkills(bottom,playerStatus.skills,skillsMenu,combat.meditating);
            else if(pages.page==1)lab::drawJournal(bottom,*w,actor,authentic&&hasJournal?&journal:nullptr,pages.zoom,equipmentError.empty()?saveMessage:equipmentError,authentic?&inventory:nullptr,authentic&&hasEquipment?&equipmentArt:nullptr,&playerStatus,&statusArt,authentic&&hasNpcs?&npcs:nullptr);
            else if(pages.page==3)lab::drawCommerce(bottom,inventory,commerce,&itemIcons);
            else if(pages.page==2)lab::drawInventory(bottom,*w,actor,authentic?inventory:demoInventory,&itemIcons,&inventoryDrag,&playerStatus);
            else lab::drawHud(bottom,hud);
            GSPGPU_FlushDataCache(bottom,320*240*3);
            gfxScreenSwapBuffers(GFX_BOTTOM,false);
        }
    }
    inventory.closeContainer();
    if(positionCheckpoint().dirty(actor))lab::savePosition(positionPath(),savedWorld(),actor,saveMessage,std::rename,sink);
    if(saves->flush())persistVitals();
    inventory.saveSink={};demoInventory.saveSink={};inventory.prepareEquipment={};inventory.useItem={};
    }
    lab::rendererShutdown();C3D_RenderTargetDelete(target);C3D_Fini();gfxExit();return 0;
}
