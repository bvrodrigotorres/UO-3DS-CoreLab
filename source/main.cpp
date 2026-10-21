// SPDX-License-Identifier: MIT
#include "core.hpp"
#include "renderer.hpp"
#include <3ds.h>
#include <cstdio>
#include <cmath>
#include <algorithm>

int main(){
    gfxInitDefault();consoleInit(GFX_BOTTOM,nullptr);
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)){gfxExit();return 1;}
    auto target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH16);
    if(!target||!lab::rendererInit()){std::printf("CoreLab: GPU init failed\nSTART to exit\n");while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_START)break;}C3D_Fini();gfxExit();return 1;}
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,GX_TRANSFER_FLIP_VERT(0)|GX_TRANSFER_OUT_TILED(0)|GX_TRANSFER_RAW_COPY(0)|GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)|GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    lab::World demo=lab::demoWorld(),privateWorld;
    std::string error;bool available=lab::loadWorld("sdmc:/3ds/UO-3DS-CoreLab/scene.bin",privateWorld,error);
    if(available)available=lab::loadAtlases(privateWorld.images,error);
    bool authentic=available,cutaway=true,moving=false;int facing=4,actual=0,frames=0,spot=0;
    lab::World*w=authentic?&privateWorld:&demo;
    lab::Actor actor={w->startX,w->startY,w->startZ},from=actor;
    float progress=0,cameraX=actor.x,cameraY=actor.y,foot=actor.z,fps=0;
    u64 last=osGetTime(),sample=last;int sampleFrames=0;const char*blocked="--";
    while(aptMainLoop()){
        u64 now=osGetTime();float dt=std::min(.05f,(now-last)/1000.f);last=now;
        hidScanInput();u32 down=hidKeysDown(),held=hidKeysHeld();
        if(down&KEY_START)break;
        if(down&KEY_Y)cutaway=!cutaway;
        if((down&KEY_X)&&!moving){
            if(available)authentic=!authentic;
            w=authentic?&privateWorld:&demo;actor={w->startX,w->startY,w->startZ};from=actor;
            cameraX=actor.x;cameraY=actor.y;foot=actor.z;blocked="--";spot=0;
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
        if(moving){progress+=dt/((held&KEY_B)?.20f:.40f);if(progress>=1){progress=1;moving=false;}
            cameraX=from.x+(actor.x-from.x)*progress;cameraY=from.y+(actor.y-from.y)*progress;foot=from.z+(actor.z-from.z)*progress;}
        int animation=moving?1+(int)(now/125)%4:0;
        lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,cutaway,facing,animation,authentic);
        ++frames;++sampleFrames;if(now-sample>=500){fps=sampleFrames*1000.f/(now-sample);sample=now;sampleFrames=0;}
        if(frames%12==0){
            std::printf("\x1b[H\x1b[2JUO-3DS CoreLab v0.1\n%s | %s\n\n",authentic?"PRIVATE UO SCENE":"SYNTHETIC TEST",cutaway?"CUTAWAY ON":"ALL HEIGHTS");
            std::printf("XY %d,%d   Z %d\n%s\nFPS %.1f | Draw %lu\nCPU %.2fms GPU %.2fms\n",actor.x+w->originX,actor.y+w->originY,actor.z,blocked,fps,(unsigned long)lab::drawnObjects(),C3D_GetProcessingTime(),C3D_GetDrawingTime());
            std::printf("Atlas %.2f MiB\nLinear free %.2f MiB\nApp free %.2f MiB\n\n",lab::textureBytes()/1048576.f,linearSpaceFree()/1048576.f,osGetMemRegionFree(MEMREGION_APPLICATION)/1048576.f);
            std::printf("Pad: walk | B: run\nY: roofs | A: test spot/reset\nX: demo / private scene\nSTART: exit\n\n");
            if(!available)std::printf("%s\n",error.c_str());
            if(lab::rendererOverflow())std::printf("ERROR: vertex budget exceeded\n");
        }
    }
    lab::rendererShutdown();C3D_RenderTargetDelete(target);C3D_Fini();gfxExit();return 0;
}
