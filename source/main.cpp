// SPDX-License-Identifier: MIT
#include "core.hpp"
#include "renderer.hpp"
#include "character_depth.hpp"
#include "performance.hpp"
#include "exploration.hpp"
#include "hud.hpp"
#include <3ds.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <malloc.h>
#include <fstream>

namespace {
void startupFrame(const lab::StartMenu*menu,bool canContinue,const std::string&stage="",int progress=0){
    auto top=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,nullptr,nullptr);
    auto bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
    lab::drawTitle(top,menu==nullptr);
    if(menu)lab::drawStartMenu(bottom,*menu,canContinue);else lab::drawLoading(bottom,stage,progress);
    GSPGPU_FlushDataCache(top,400*240*3);GSPGPU_FlushDataCache(bottom,320*240*3);
    gfxScreenSwapBuffers(GFX_TOP,false);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
}
}

int main(){
    gfxInitDefault();
    // Draw the entire lower page into the hidden buffer, then present it.
    gfxSetDoubleBuffering(GFX_BOTTOM,true);
    startupFrame(nullptr,false,"INICIANDO EL JUEGO",0);
    if(!C3D_Init(lab::FrameCommandBytes)){gfxExit();return 1;}
    auto target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH16);
    if(!target||!lab::rendererInit()){consoleInit(GFX_BOTTOM,nullptr);std::printf("CoreLab: GPU init failed\nSTART to exit\n");while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_START)break;}C3D_Fini();gfxExit();return 1;}
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,GX_TRANSFER_FLIP_VERT(0)|GX_TRANSFER_OUT_TILED(0)|GX_TRANSFER_RAW_COPY(0)|GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)|GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    startupFrame(nullptr,false,"CARGANDO BRITANNIA",1);
    lab::World demo=lab::demoWorld(),privateWorld;
    const std::string baseRoot="sdmc:/3ds/UO-3DS-CoreLab/";
    const std::string worldRoot=std::ifstream(baseRoot+"britain-large/scene.bin").good()?baseRoot+"britain-large/":baseRoot;
    std::string error;bool available=lab::loadWorld(worldRoot+"scene.bin",privateWorld,error);
    if(available)available=lab::loadAtlases(privateWorld.images,error,worldRoot);
    const lab::World canonicalPrivate=privateWorld;
    lab::World legacyPrivate=privateWorld;lab::Doors doors;std::string doorError;
    bool hasDoors=available&&doors.load(worldRoot+"doors.bin",worldRoot+"scene.bin",privateWorld,doorError);
    if(hasDoors){legacyPrivate.objects.resize(doors.baseObjects);legacyPrivate.index();}
    startupFrame(nullptr,false,"MAPA Y PERSONAJE",2);
    lab::RoofMode roofs=lab::RoofMode::Auto;
    lab::TouchPages pages;lab::Journal journal;std::string journalError;
    bool hasJournal=available&&lab::loadJournal(worldRoot+"journal.bin",worldRoot+"scene.bin",journal,journalError);
    const std::string saveRoot=worldRoot;
    lab::Campaign campaign(saveRoot);campaign.load();std::string savePrefix=campaign.prefix();
    bool authentic=available,moving=false;int facing=4,actual=0,frames=0,spot=0;
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
    lab::Inventory inventory;inventory.init(canonicalPrivate,saveRoot+savePrefix+"inventory-private.sav");
    lab::Inventory demoInventory;demoInventory.init(demo,saveRoot+savePrefix+"inventory-demo.sav");
    lab::ItemIcons itemIcons;itemIcons.load("sdmc:/3ds/UO-3DS-CoreLab/items.bin");
    startupFrame(nullptr,false,"MOCHILA Y EQUIPAMIENTO",4);
    lab::EquipmentArt equipmentArt;std::string equipmentError;
    bool hasEquipment=equipmentArt.load("sdmc:/3ds/UO-3DS-CoreLab/equipment.bin");
    if(!hasEquipment)equipmentError="FALTA EQUIPMENT.BIN";
    startupFrame(nullptr,false,"PREPARADO",5);
    lab::StartMenu menu;menu.options.load(saveRoot+"options.sav");
    lab::Actor probe;std::string probeMessage;
    bool positionSaved=lab::loadPosition(positionPath(),savedWorld(),probe,probeMessage);
    if(!positionSaved&&authentic&&hasDoors)positionSaved=lab::loadPosition(positionPath(),legacyPrivate,probe,probeMessage);
    bool canContinue=campaign.valid&&(authentic?inventory.healthy():demoInventory.healthy())&&(positionSaved||(authentic?inventory.restored:demoInventory.restored));
    menu.selected=canContinue?1:0;
    if(!campaign.valid)menu.message="ERROR AL LEER LA PARTIDA GUARDADA";
    else if(!(authentic?inventory.healthy():demoInventory.healthy()))menu.message="ERROR EN INVENTARIO GUARDADO";
    auto freshGame=[&](){
        uint32_t generation=campaign.unusedGeneration();
        if(!generation){menu.message="NO SE PUDO PREPARAR LA PARTIDA";return false;}
        std::string prefix=campaign.prefix(generation),error;
        lab::Inventory freshPrivate,freshDemo;
        freshDemo.init(demo,saveRoot+prefix+"inventory-demo.sav");
        if(!freshDemo.checkpoint()||!lab::savePosition(saveRoot+prefix+"position-demo.sav",demo,{demo.startX,demo.startY,demo.startZ},error)){
            menu.message="ERROR SD: PARTIDA ANTERIOR CONSERVADA";return false;
        }
        if(available){
            freshPrivate.init(canonicalPrivate,saveRoot+prefix+"inventory-private.sav");
            if(!freshPrivate.checkpoint()||!lab::savePosition(saveRoot+prefix+"position-private.sav",canonicalPrivate,{canonicalPrivate.startX,canonicalPrivate.startY,canonicalPrivate.startZ},error)){
                menu.message="ERROR SD: PARTIDA ANTERIOR CONSERVADA";return false;
            }
        }
        if(!campaign.commit(generation,error)){menu.message=error;return false;}
        savePrefix=prefix;inventory=std::move(freshPrivate);demoInventory=std::move(freshDemo);
        privateWorld=canonicalPrivate;
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
        if(action==lab::MenuAction::NewGame)started=freshGame();
        if(action==lab::MenuAction::Continue){if(restorePosition())alignCamera();started=true;}
    }
    if(!started)break;
    moving=false;alignCamera();
    startupFrame(nullptr,false,"ENTRANDO EN BRITANNIA",5);
    roofs=static_cast<lab::RoofMode>(menu.options.roofs);pages.zoom=menu.options.zoom;pages.page=menu.options.page;
    if(hasNpcs)npcs.reset(privateWorld,actor);
    inventory.prepareEquipment=[&](unsigned mask){return lab::prepareEquipment(mask,equipmentArt,equipmentError);};
    if(hasEquipment){
        startupFrame(nullptr,false,"PREPARANDO EQUIPAMIENTO",5);
        lab::warmEquipment(inventory.equipmentMask(),equipmentArt);
        lab::prepareEquipment(inventory.equipmentMask(),equipmentArt,equipmentError);
    }
    std::vector<lab::Actor>occupants;occupants.reserve(16);
    lab::PauseMenu pause;u64 pauseAt=0,pausePad=0;
    u64 last=osGetTime(),sample=last;positionCheckpoint().active(last);int sampleFrames=0;const char*blocked="--";
    while(aptMainLoop()){
        u64 now=osGetTime();float dt=std::min(.05f,(now-last)/1000.f);last=now;
        hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();
        bool wasPaused=pause.active;
        if((down&KEY_START)&&!pause.active){pause.open();pauseAt=now;pages.active=false;}
        else if((down&KEY_START)&&pause.active)pause.active=false;
        if(pause.active||wasPaused){
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
            }else if(action==lab::PauseAction::MainMenu||action==lab::PauseAction::Exit){
                bool saved=true;
                if(positionCheckpoint().dirty(actor)){saved=lab::savePosition(positionPath(),savedWorld(),actor,saveMessage);positionCheckpoint().result(actor,pauseAt,saved);}
                auto&bag=authentic?inventory:demoInventory;
                if(saved&&bag.healthy())saved=bag.checkpoint();
                if(saved){C3D_FrameSync();quit=action==lab::PauseAction::Exit;menu.back();menu.selected=1;canContinue=true;moving=false;break;}
                pause.message="ERROR SD: NO SE PUDO GUARDAR";
            }
            last=sample=now;sampleFrames=0;
            auto bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);lab::drawPause(bottom,pause);
            GSPGPU_FlushDataCache(bottom,320*240*3);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
            continue;
        }
        if(down&KEY_Y)roofs=lab::nextRoofMode(roofs);
        bool pageChanged=false;
        if(down&KEY_R){pages.page=(pages.page+1)%3;pageChanged=true;}
        touchPosition touch;
        if(held&KEY_TOUCH){hidTouchRead(&touch);if(down&KEY_TOUCH)pages.begin(touch.px,touch.py);else pages.move(touch.px,touch.py);}
        if(hidKeysUp()&KEY_TOUCH){
            int oldPage=pages.page,dx=pages.lastX-pages.startX,dy=pages.lastY-pages.startY;
            bool changed=pages.end();
            if(!changed&&oldPage==2&&authentic&&(!moving||lab::equipmentTouchWhileMoving(inventory.equipMode,pages.startX,pages.startY))&&dx*dx+dy*dy<144)
                inventory.tap(*w,actor,pages.startX,pages.startY);
            if(!changed&&oldPage==1&&authentic&&dx*dx+dy*dy<144&&pages.startX>=280&&pages.startX<=304){
                for(int slot=0;slot<3;++slot)if(pages.startY>=68+slot*34&&pages.startY<94+slot*34){inventory.unequip(slot);saveMessage=inventory.message;}
            }
            pageChanged=true;
        }
        if(authentic)inventory.update(*w,actor);else inventory.opened=-1;
        if(down&KEY_SELECT){
            if(moving)saveMessage="Stop moving before saving";
            else {bool ok=lab::savePosition(positionPath(),savedWorld(),actor,saveMessage);positionCheckpoint().result(actor,now,ok);}
        }
        if((down&KEY_L)&&!moving&&restorePosition())alignCamera();
        if((down&KEY_X)&&!moving){
            if(available)authentic=!authentic;
            w=authentic?&privateWorld:&demo;actor={w->startX,w->startY,w->startZ};from=actor;
            cameraX=actor.x;cameraY=actor.y;foot=actor.z;blocked="--";spot=0;
            if(restorePosition())alignCamera();
        }
        if((down&KEY_A)&&!moving){
            // Deterministic viewpoints. Reset height to original spawn only.
            if(!authentic){const lab::Actor spots[]={{8,10,0},{7,7,4},{10,6,0},{12,10,2}};actor=spots[(++spot)%4];from=actor;cameraX=actor.x;cameraY=actor.y;foot=actor.z;}
            else {actor={w->startX,w->startY,w->startZ};from=actor;cameraX=actor.x;cameraY=actor.y;foot=actor.z;}
        }
        if(authentic){
            occupants.clear();occupants.push_back(actor);if(moving)occupants.push_back(from);npcs.addOccupants(occupants);
            if(doors.update(privateWorld,dt,occupants))lab::invalidateRenderList();
            if(hasNpcs)npcs.update(privateWorld,actor,from,moving,dt,doors);
        }
        circlePosition pad;hidCircleRead(&pad);
        int sx=((held&KEY_RIGHT)||pad.dx>35)-((held&KEY_LEFT)||pad.dx< -35);
        int sy=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);
        if(!moving&&(sx||sy)){
            int dx=std::max(-1,std::min(1,sx+sy)),dy=std::max(-1,std::min(1,sy-sx));
            const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};
            int direction=0;for(int i=0;i<8;++i)if(xs[i]==dx&&ys[i]==dy)direction=i;
            // Turning responds to intent even when movement is blocked.
            facing=(direction+1)&7;
            lab::Actor next;
            if(authentic&&doors.tryOpen(actor,dx,dy)){blocked="OPENING DOOR";}
            else if(lab::step(*w,actor,direction,next,actual)&&(!authentic||!npcs.occupied(next))){
                from=actor;actor=next;moving=true;progress=0;facing=(actual+1)&7;blocked="--";
            }else blocked="BLOCKED";
        }
        bool animateThisFrame=moving,running=(held&KEY_B)!=0;
        if(moving){progress+=dt/(running?.20f:.40f);if(progress>=1){progress=1;moving=false;}
            cameraX=from.x+(actor.x-from.x)*progress;cameraY=from.y+(actor.y-from.y)*progress;foot=from.z+(actor.z-from.z)*progress;}
        if(animateThisFrame||sx||sy||(held&KEY_TOUCH)||down)positionCheckpoint().active(now);
        if(!moving&&positionCheckpoint().due(actor,now)){
            bool ok=lab::savePosition(positionPath(),savedWorld(),actor,saveMessage);
            positionCheckpoint().result(actor,now,ok);
        }
        if(animateThisFrame)animationPhase=std::fmod(animationPhase+dt*(running?13.f:8.f),4.f);
        int animation=(int)animationPhase;
        if(authentic&&hasEquipment)lab::prepareEquipment(inventory.equipmentMask(),equipmentArt,equipmentError);
        lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,animation,authentic,animateThisFrame,running,authentic&&hasNpcs?&npcs:nullptr);
        ++frames;++sampleFrames;if(now-sample>=500){fps=sampleFrames*1000.f/(now-sample);sample=now;sampleFrames=0;}
        if(frames==1||frames%12==0||down||pageChanged){
            lab::HudInfo hud{authentic?(w->width>32?"BRITAIN 256X256":"PRIVATE UO SCENE"):"SYNTHETIC TEST",lab::roofModeName(roofs),{}};
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
                if(authentic&&hasDoors){std::snprintf(line,sizeof(line),"DECOR %lu | DOORS %lu",(unsigned long)(canonicalPrivate.objects.size()-doors.baseObjects-doors.count()),(unsigned long)doors.count());add();}
                else hud.lines.emplace_back("");

            }
            if(authentic&&hasNpcs){
                auto greeting=npcs.greeting(actor);
                hud.lines.push_back(greeting.empty()?"NPC "+std::to_string(npcs.all().size())+" | POBLACION LOCAL":greeting);
            }
            if(!saveMessage.empty())hud.lines.push_back(saveMessage);
            if(authentic&&!equipmentError.empty())hud.lines.push_back(equipmentError);
            if(!available)hud.lines.push_back(error);
            if(authentic&&!hasDoors)hud.lines.push_back(doorError);
            if(authentic&&!npcError.empty())hud.lines.push_back(npcError);
            if(lab::rendererTextureError())hud.lines.emplace_back("TEXTURAS: ERROR SD/MEMORIA");
            if(lab::rendererOverflow())hud.lines.emplace_back("ERROR: VERTEX BUDGET EXCEEDED");
            auto bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
            if(pages.page==1)lab::drawJournal(bottom,*w,actor,authentic&&hasJournal?&journal:nullptr,pages.zoom,equipmentError.empty()?saveMessage:equipmentError,authentic?&inventory:nullptr,authentic&&hasEquipment?&equipmentArt:nullptr);
            else if(pages.page==2)lab::drawInventory(bottom,*w,actor,authentic?inventory:demoInventory,&itemIcons);
            else lab::drawHud(bottom,hud);
            GSPGPU_FlushDataCache(bottom,320*240*3);
            gfxScreenSwapBuffers(GFX_BOTTOM,false);
        }
    }
    if(positionCheckpoint().dirty(actor))lab::savePosition(positionPath(),savedWorld(),actor,saveMessage);
    }
    lab::rendererShutdown();C3D_RenderTargetDelete(target);C3D_Fini();gfxExit();return 0;
}
