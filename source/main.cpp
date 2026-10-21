// SPDX-License-Identifier: MIT
#include "core.hpp"
#include "renderer.hpp"
#include "exploration.hpp"
#include "hud.hpp"
#include <3ds.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <malloc.h>

int main(){
    gfxInitDefault();
    // Draw the entire lower page into the hidden buffer, then present it.
    gfxSetDoubleBuffering(GFX_BOTTOM,true);
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)){gfxExit();return 1;}
    auto target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH16);
    if(!target||!lab::rendererInit()){consoleInit(GFX_BOTTOM,nullptr);std::printf("CoreLab: GPU init failed\nSTART to exit\n");while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_START)break;}C3D_Fini();gfxExit();return 1;}
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,GX_TRANSFER_FLIP_VERT(0)|GX_TRANSFER_OUT_TILED(0)|GX_TRANSFER_RAW_COPY(0)|GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)|GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    lab::World demo=lab::demoWorld(),privateWorld;
    std::string error;bool available=lab::loadWorld("sdmc:/3ds/UO-3DS-CoreLab/scene.bin",privateWorld,error);
    if(available)available=lab::loadAtlases(privateWorld.images,error);
    const lab::World canonicalPrivate=privateWorld;
    lab::World legacyPrivate=privateWorld;lab::Doors doors;std::string doorError;
    bool hasDoors=available&&doors.load("sdmc:/3ds/UO-3DS-CoreLab/doors.bin","sdmc:/3ds/UO-3DS-CoreLab/scene.bin",privateWorld,doorError);
    if(hasDoors){legacyPrivate.objects.resize(doors.baseObjects);legacyPrivate.index();}
    lab::RoofMode roofs=lab::RoofMode::Auto;
    lab::TouchPages pages;lab::Journal journal;std::string journalError;
    bool hasJournal=available&&lab::loadJournal("sdmc:/3ds/UO-3DS-CoreLab/journal.bin","sdmc:/3ds/UO-3DS-CoreLab/scene.bin",journal,journalError);
    bool authentic=available,moving=false;int facing=4,actual=0,frames=0,spot=0;
    lab::World*w=authentic?&privateWorld:&demo;
    lab::Actor actor={w->startX,w->startY,w->startZ},from=actor;
    float progress=0,cameraX=actor.x,cameraY=actor.y,foot=actor.z,fps=0,animationPhase=0;
    std::string saveMessage;
    auto positionPath=[&](){return std::string("sdmc:/3ds/UO-3DS-CoreLab/")+(authentic?"position-private.sav":"position-demo.sav");};
    auto savedWorld=[&]()->const lab::World&{return authentic?canonicalPrivate:demo;};
    auto restorePosition=[&](){
        if(lab::loadPosition(positionPath(),savedWorld(),actor,saveMessage))return true;
        if(authentic&&hasDoors&&lab::loadPosition(positionPath(),legacyPrivate,actor,saveMessage)){
            if(lab::savePosition(positionPath(),canonicalPrivate,actor,saveMessage))saveMessage="Position migrated to decorated world";
            return true;
        }
        return false;
    };
    auto alignCamera=[&](){from=actor;cameraX=actor.x;cameraY=actor.y;foot=actor.z;progress=0;};
    if(restorePosition())alignCamera();
    lab::Npcs npcs;std::string npcError;
    bool hasNpcs=available&&lab::loadNpcAtlases(npcError);
    if(hasNpcs)npcs.reset(privateWorld,actor);
    lab::Inventory inventory;inventory.init(canonicalPrivate,"sdmc:/3ds/UO-3DS-CoreLab/inventory-private.sav");
    lab::Inventory demoInventory;demoInventory.init(demo,"sdmc:/3ds/UO-3DS-CoreLab/inventory-demo.sav");
    lab::ItemIcons itemIcons;itemIcons.load("sdmc:/3ds/UO-3DS-CoreLab/items.bin");
    std::vector<lab::Actor>occupants;occupants.reserve(16);
    u64 last=osGetTime(),sample=last;int sampleFrames=0;const char*blocked="--";
    while(aptMainLoop()){
        u64 now=osGetTime();float dt=std::min(.05f,(now-last)/1000.f);last=now;
        hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();
        if(down&KEY_START)break;
        if(down&KEY_Y)roofs=lab::nextRoofMode(roofs);
        bool pageChanged=false;
        if(down&KEY_R){pages.page=(pages.page+1)%3;pageChanged=true;}
        touchPosition touch;
        if(held&KEY_TOUCH){hidTouchRead(&touch);if(down&KEY_TOUCH)pages.begin(touch.px,touch.py);else pages.move(touch.px,touch.py);}
        if(hidKeysUp()&KEY_TOUCH){
            int oldPage=pages.page,dx=pages.lastX-pages.startX,dy=pages.lastY-pages.startY;
            bool changed=pages.end();
            if(!changed&&oldPage==2&&authentic&&!moving&&dx*dx+dy*dy<144)
                inventory.tap(*w,actor,pages.startX,pages.startY);
            pageChanged=true;
        }
        if(authentic)inventory.update(*w,actor);else inventory.opened=-1;
        if(down&KEY_SELECT){
            if(moving)saveMessage="Stop moving before saving";
            else lab::savePosition(positionPath(),savedWorld(),actor,saveMessage);
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
        if(animateThisFrame)animationPhase=std::fmod(animationPhase+dt*(running?13.f:8.f),4.f);
        int animation=(int)animationPhase;
        lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,animation,authentic,animateThisFrame,running,authentic&&hasNpcs?&npcs:nullptr);
        ++frames;++sampleFrames;if(now-sample>=500){fps=sampleFrames*1000.f/(now-sample);sample=now;sampleFrames=0;}
        if(frames==1||frames%12==0||down||pageChanged){
            lab::HudInfo hud{authentic?"PRIVATE UO SCENE":"SYNTHETIC TEST",lab::roofModeName(roofs),{}};
            char line[160];
            auto add=[&](){hud.lines.emplace_back(line);};
            std::snprintf(line,sizeof(line),"XY %d,%d   Z %d | %s",actor.x+w->originX,actor.y+w->originY,actor.z,blocked);add();
            std::snprintf(line,sizeof(line),"FPS %.1f | DRAW %lu",fps,(unsigned long)lab::drawnObjects());add();
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
            if(!available)hud.lines.push_back(error);
            if(authentic&&!hasDoors)hud.lines.push_back(doorError);
            if(authentic&&!hasNpcs)hud.lines.push_back(npcError);
            if(lab::rendererOverflow())hud.lines.emplace_back("ERROR: VERTEX BUDGET EXCEEDED");
            auto bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
            if(pages.page==1)lab::drawJournal(bottom,*w,actor,authentic&&hasJournal?&journal:nullptr,pages.zoom,saveMessage);
            else if(pages.page==2)lab::drawInventory(bottom,*w,actor,authentic?inventory:demoInventory,&itemIcons);
            else lab::drawHud(bottom,hud);
            GSPGPU_FlushDataCache(bottom,320*240*3);
            gfxScreenSwapBuffers(GFX_BOTTOM,false);
        }
    }
    lab::rendererShutdown();C3D_RenderTargetDelete(target);C3D_Fini();gfxExit();return 0;
}
