// SPDX-License-Identifier: MIT
#include "core.hpp"
#include "renderer.hpp"
#include "exploration.hpp"
#include <3ds.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <malloc.h>

int main(){
    gfxInitDefault();consoleInit(GFX_BOTTOM,nullptr);
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)){gfxExit();return 1;}
    auto target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH16);
    if(!target||!lab::rendererInit()){std::printf("CoreLab: GPU init failed\nSTART to exit\n");while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_START)break;}C3D_Fini();gfxExit();return 1;}
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,GX_TRANSFER_FLIP_VERT(0)|GX_TRANSFER_OUT_TILED(0)|GX_TRANSFER_RAW_COPY(0)|GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)|GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    lab::World demo=lab::demoWorld(),privateWorld;
    std::string error;bool available=lab::loadWorld("sdmc:/3ds/UO-3DS-CoreLab/scene.bin",privateWorld,error);
    if(available)available=lab::loadAtlases(privateWorld.images,error);
    bool authentic=available,cutaway=true,moving=false,showMap=false;int facing=4,actual=0,frames=0,spot=0;
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
        if(down&KEY_Y)cutaway=!cutaway;
        if(down&KEY_R)showMap=!showMap;
        if((down&KEY_SELECT)&&!moving)lab::savePosition(positionPath(),*w,actor,saveMessage);
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
        lab::drawWorld(target,*w,actor,cameraX,cameraY,foot,cutaway,facing,animation,authentic,animateThisFrame,running);
        ++frames;++sampleFrames;if(now-sample>=500){fps=sampleFrames*1000.f/(now-sample);sample=now;sampleFrames=0;}
        if(frames%12==0){
            std::printf("\x1b[H\x1b[2JUO-3DS CoreLab v0.3\n%s | %s\n\n",authentic?"PRIVATE UO SCENE":"SYNTHETIC TEST",cutaway?"CUTAWAY ON":"ALL HEIGHTS");
            std::printf("XY %d,%d   Z %d\n%s\nFPS %.1f | Draw %lu\nCPU %.2fms GPU %.2fms\n",actor.x+w->originX,actor.y+w->originY,actor.z,blocked,fps,(unsigned long)lab::drawnObjects(),C3D_GetProcessingTime(),C3D_GetDrawingTime());
            if(showMap){
                std::printf("Map: world X right / Y down\n");
                for(int y=actor.y-4;y<=actor.y+4;++y){
                    std::printf("       ");
                    for(int x=actor.x-7;x<=actor.x+7;++x)std::putchar(lab::mapCell(*w,actor,x,y));
                    std::putchar('\n');
                }
                std::printf("@ you # block + floor ~ water\n");
            }else{
                auto heap=mallinfo();
                std::printf("Atlas %.2f MiB | Lists %u\nLinear free %.2f MiB\nHeap used %.2f MiB\nHeap reusable %.2f MiB\nRun art: %s\n\n",lab::textureBytes()/1048576.f,lab::renderListBuilds(),linearSpaceFree()/1048576.f,heap.uordblks/1048576.f,heap.fordblks/1048576.f,lab::hasRunAnimation()?"ACTION 2 VERIFIED":"UNVERIFIED (walk)");
            }
            std::printf("Pad: walk | B: run | R: map\nY: roofs | A: reset | X: scene\nSELECT: save | L: restore\nSTART: exit\n%s\n",saveMessage.c_str());
            if(!available)std::printf("%s\n",error.c_str());
            if(lab::rendererOverflow())std::printf("ERROR: vertex budget exceeded\n");
        }
    }
    lab::rendererShutdown();C3D_RenderTargetDelete(target);C3D_Fini();gfxExit();return 0;
}
