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
    gfxSetDoubleBuffering(GFX_BOTTOM,false);
    gfxScreenSwapBuffers(GFX_BOTTOM,false);
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)){gfxExit();return 1;}
    auto target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH16);
    if(!target||!lab::rendererInit()){consoleInit(GFX_BOTTOM,nullptr);std::printf("CoreLab: GPU init failed\nSTART to exit\n");while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_START)break;}C3D_Fini();gfxExit();return 1;}
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,GX_TRANSFER_FLIP_VERT(0)|GX_TRANSFER_OUT_TILED(0)|GX_TRANSFER_RAW_COPY(0)|GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)|GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    lab::World demo=lab::demoWorld(),privateWorld;
    std::string error;bool available=lab::loadWorld("sdmc:/3ds/UO-3DS-CoreLab/scene.bin",privateWorld,error);
    if(available)available=lab::loadAtlases(privateWorld.images,error);
    lab::RoofMode roofs=lab::RoofMode::Auto;
    bool authentic=available,moving=false,showMap=false;int facing=4,actual=0,frames=0,spot=0;
    lab::World*w=authentic?&privateWorld:&demo;
    lab::Actor actor={w->startX,w->startY,w->startZ},from=actor;
    float progress=0,cameraX=actor.x,cameraY=actor.y,foot=actor.z,fps=0,animationPhase=0;
    std::string saveMessage;
    auto positionPath=[&](){return std::string("sdmc:/3ds/UO-3DS-CoreLab/")+(authentic?"position-private.sav":"position-demo.sav");};
    auto alignCamera=[&](){from=actor;cameraX=actor.x;cameraY=actor.y;foot=actor.z;progress=0;};
    if(lab::loadPosition(positionPath(),*w,actor,saveMessage))alignCamera();
    u64 last=osGetTime(),sample=last;int sampleFrames=0;const char*blocked="--";
    while(aptMainLoop()){
        u64 now=osGetTime();float dt=std::min(.05f,(now-last)/1000.f);last=now;
        hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();
        if(down&KEY_START)break;
        if(down&KEY_Y)roofs=lab::nextRoofMode(roofs);
        if(down&KEY_R)showMap=!showMap;
        if(down&KEY_SELECT){
            if(moving)saveMessage="Stop moving before saving";
            else lab::savePosition(positionPath(),*w,actor,saveMessage);
        }
        if((down&KEY_L)&&!moving&&lab::loadPosition(positionPath(),*w,actor,saveMessage))alignCamera();
        if((down&KEY_X)&&!moving){
            if(available)authentic=!authentic;
            w=authentic?&privateWorld:&demo;actor={w->startX,w->startY,w->startZ};from=actor;
            cameraX=actor.x;cameraY=actor.y;foot=actor.z;blocked="--";spot=0;
            if(lab::loadPosition(positionPath(),*w,actor,saveMessage))alignCamera();
        }
        if((down&KEY_A)&&!moving){
            // Deterministic viewpoints. Reset height to original spawn only.
            if(!authentic){const lab::Actor spots[]={{8,10,0},{7,7,4},{10,6,0},{12,10,2}};actor=spots[(++spot)%4];from=actor;cameraX=actor.x;cameraY=actor.y;foot=actor.z;}
            else {actor={w->startX,w->startY,w->startZ};from=actor;cameraX=actor.x;cameraY=actor.y;foot=actor.z;}
        }
        circlePosition pad;hidCircleRead(&pad);
        int sx=((held&KEY_RIGHT)||pad.dx>35)-((held&KEY_LEFT)||pad.dx< -35);
        int sy=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);
        if(!moving&&(sx||sy)){
            int dx=std::max(-1,std::min(1,sx+sy)),dy=std::max(-1,std::min(1,sy-sx));
            const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};
            int direction=0;for(int i=0;i<8;++i)if(xs[i]==dx&&ys[i]==dy)direction=i;
            lab::Actor next;
            if(lab::step(*w,actor,direction,next,actual)){
                from=actor;actor=next;moving=true;progress=0;facing=(actual+1)&7;blocked="--";
            }else blocked="BLOCKED";
        }
        bool animateThisFrame=moving,running=(held&KEY_B)!=0;
        if(moving){progress+=dt/(running?.20f:.40f);if(progress>=1){progress=1;moving=false;}
            cameraX=from.x+(actor.x-from.x)*progress;cameraY=from.y+(actor.y-from.y)*progress;foot=from.z+(actor.z-from.z)*progress;}
        if(animateThisFrame)animationPhase=std::fmod(animationPhase+dt*(running?13.f:8.f),4.f);
        int animation=(int)animationPhase;
        lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,roofs,facing,animation,authentic,animateThisFrame,running);
        ++frames;++sampleFrames;if(now-sample>=500){fps=sampleFrames*1000.f/(now-sample);sample=now;sampleFrames=0;}
        if(frames==1||frames%12==0||down){
            lab::HudInfo hud{authentic?"PRIVATE UO SCENE":"SYNTHETIC TEST",lab::roofModeName(roofs),{}};
            char line[160];
            auto add=[&](){hud.lines.emplace_back(line);};
            std::snprintf(line,sizeof(line),"XY %d,%d   Z %d | %s",actor.x+w->originX,actor.y+w->originY,actor.z,blocked);add();
            std::snprintf(line,sizeof(line),"FPS %.1f | DRAW %lu",fps,(unsigned long)lab::drawnObjects());add();
            std::snprintf(line,sizeof(line),"CPU %.2fMS | GPU %.2fMS",C3D_GetProcessingTime(),C3D_GetDrawingTime());add();
            if(showMap){
                hud.lines.emplace_back("MAP: WORLD X RIGHT / Y DOWN");
                for(int y=actor.y-4;y<=actor.y+4;++y){
                    std::string row="           ";
                    for(int x=actor.x-7;x<=actor.x+7;++x)row+=lab::mapCell(*w,actor,x,y);
                    hud.lines.push_back(row);
                }
                hud.lines.emplace_back("@ YOU # BLOCK + FLOOR ~ WATER");
            }else{
                auto heap=mallinfo();
                std::snprintf(line,sizeof(line),"ATLAS %.2F MIB | LISTS %u",lab::textureBytes()/1048576.f,lab::renderListBuilds());add();
                std::snprintf(line,sizeof(line),"LINEAR FREE %.2F MIB",linearSpaceFree()/1048576.f);add();
                std::snprintf(line,sizeof(line),"HEAP USED %.2F MIB",heap.uordblks/1048576.f);add();
                std::snprintf(line,sizeof(line),"HEAP REUSABLE %.2F MIB",heap.fordblks/1048576.f);add();
                std::snprintf(line,sizeof(line),"RUN ART: %s",lab::hasRunAnimation()?"ACTION 2 VERIFIED":"UNVERIFIED (WALK)");add();
                hud.lines.emplace_back("");

            }
            if(!saveMessage.empty())hud.lines.push_back(saveMessage);
            if(!available)hud.lines.push_back(error);
            if(lab::rendererOverflow())hud.lines.emplace_back("ERROR: VERTEX BUDGET EXCEEDED");
            auto bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
            lab::drawHud(bottom,hud);
            GSPGPU_FlushDataCache(bottom,320*240*3);
        }
    }
    lab::rendererShutdown();C3D_RenderTargetDelete(target);C3D_Fini();gfxExit();return 0;
}
