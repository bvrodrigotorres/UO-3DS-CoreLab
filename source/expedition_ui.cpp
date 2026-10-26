// SPDX-License-Identifier: MIT
#include "expedition_ui.hpp"
#include "hud.hpp"
#include "developer_menu.hpp"
#include "resource_preparation.hpp"
#include "mount.hpp"
#include "performance.hpp"
#include "world_stream.hpp"
#include "version.hpp"
#include <3ds.h>
#include <cmath>
#include <memory>
#include <fstream>
#include <malloc.h>
#include <cstdio>
namespace lab {
namespace {
void lower(const HudInfo&info){auto*b=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);drawExpeditionHud(b,info);GSPGPU_FlushDataCache(b,320*240*3);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();}
void loading(const std::string&stage){
    C3D_FrameSync();auto*t=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,nullptr,nullptr);auto*b=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
    drawTitle(t,true);drawLoading(b,stage,3);GSPGPU_FlushDataCache(t,400*240*3);GSPGPU_FlushDataCache(b,320*240*3);gfxScreenSwapBuffers(GFX_TOP,false);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
}
struct Destination{const char*name;Actor position;};
constexpr Destination destinations[]={{"BRITAIN",{1496,1628,20}},{"TRINSIC",{1845,2745,0}},{"MOONGLOW",{4467,1283,0}},{"MINOC",{2476,413,15}},{"YEW",{633,858,0}},{"SKARA BRAE",{635,2236,0}},{"VESPER",{2880,700,0}},{"JHELOM",{1380,3815,0}},{"MAGINCIA",{3732,2140,20}},{"BUCCANEER",{2707,2162,0}}};
}
std::string exploreBritannia(C3D_RenderTarget*target,const std::string&base,const std::string&save,const Actor&start,const PlayerStatus&status,const Inventory&bag,const EquipmentArt&equipment,const StatusArt&statusArt,RoofMode roofs){
    loading("ABRIENDO EL MAPA MUNDIAL");auto archive=std::make_unique<WorldArchive>();auto graphics=std::make_unique<WorldGraphics>();std::string error;
    const auto root=base+"britannia/";
    if(!std::ifstream(root+"britannia.uow").good())return "INSTALA EL PAQUETE MAPA MUNDIAL";
    if(!archive->open(root+"britannia.uow",error)||!graphics->load(root,error))return error;
    World positionWorld;positionWorld.width=archive->width();positionWorld.height=archive->height();positionWorld.compatiblePosition=archive->identity();
    Actor global=start,restored;if(loadPosition(save,positionWorld,restored,error))global=restored;
    auto active=std::make_unique<StreamFrame>();World*scene=&active->world;Journal*radar=&active->radar;std::unique_ptr<WorldStream>stream;Actor actor,from;float progress=0,cameraX=0,cameraY=0,foot=0,phase=0;bool moving=false;int facing=4;
    auto trace=[&](const char*stage,const Actor&where){auto memory=mallinfo();FILE*f=std::fopen((base+"expedition-load.log").c_str(),"w");if(f){std::fprintf(f,"v%s %s XY %d,%d Z %d heap_used %d heap_free %d linear_free %lu\n",Version,stage,where.x,where.y,where.z,memory.uordblks,memory.fordblks,(unsigned long)linearSpaceFree());std::fclose(f);}};
    auto load=[&](const Actor&where,bool safe){
        loading("CARGANDO DESTINO");if(stream)stream->cancelAndWait();setWorldLookAhead(0,0);releaseWorldResources();
        const bool previous=scene&&!scene->land.empty();const Actor old=previous?ExpeditionWindow::global(*scene,actor):where;
        int oldX=previous?scene->originX:0,oldY=previous?scene->originY:0;
        active.reset();scene=nullptr;radar=nullptr;
        auto attempt=[&](const Actor&point,bool safeLanding,int x,int y){
            auto next=prepareStreamFrame(*archive,*graphics,x,y,WorldArchive::MaxObjects,error);if(!next)return false;
            Actor candidate=ExpeditionWindow::local(next->world,point);
            if(safeLanding){Npcs empty;Actor landing;if(!safeTeleportTile(next->world,empty,candidate,landing)){candidate.z=heights(next->world,candidate.x,candidate.y).average;if(!safeTeleportTile(next->world,empty,candidate,landing)){error="NO HAY UN PUNTO SEGURO";return false;}}candidate=landing;}
            active=std::move(next);scene=&active->world;radar=&active->radar;actor=from=candidate;cameraX=actor.x;cameraY=actor.y;foot=actor.z;progress=0;moving=false;adoptWorldView(std::move(active->roofs));return true;
        };
        if(!attempt(where,safe,StreamPlan::origin(where.x,archive->width()),StreamPlan::origin(where.y,archive->height()))){auto failure=error;if(previous)attempt(old,false,oldX,oldY);error=failure;return false;}
        trace("destination-ready",where);return true;
    };
    if(!load(global,true))return error;
    if(!loadAtlases(graphics->images(),error,root))return error;
    auto warm=[&](){trace("warming",ExpeditionWindow::global(*scene,actor));ResourcePreparation prep;u64 until=osGetTime()+10000;while(aptMainLoop()&&osGetTime()<until&&prep.pending()){
        drawWorld(target,*scene,actor,cameraX,cameraY,foot,roofs,facing,0,true,false,false,nullptr,nullptr,&status,nullptr,false);
        prep.observe(rendererResourceWaits(),rendererTextureError());loading("PREPARANDO TEXTURAS DEL SECTOR");
    }trace("ready",ExpeditionWindow::global(*scene,actor));};warm();
    stream=std::make_unique<WorldStream>([&](int x,int y,std::string&failure){return prepareStreamFrame(*archive,*graphics,x,y,StreamPlan::MaxAheadObjects,failure);});
    const bool async=stream->start();PositionCheckpoint checkpoint;checkpoint.baseline(ExpeditionWindow::global(*scene,actor),osGetTime());
    std::string message="MAPA EXPERIMENTAL: PRECARGA ACTIVA";bool paused=false,map=true,travel=false;int choice=0,zoom=2;u64 repeat=0,last=osGetTime();FramePacer pacer;
    auto savePositionNow=[&](){return savePosition(save,positionWorld,ExpeditionWindow::global(*scene,actor),error);};
    while(aptMainLoop()){
        auto delay=pacer.delay(osGetTime()*1000);if(delay)svcSleepThread(delay*1000);
        u64 now=osGetTime();float dt=std::min(.05f,(now-last)/1000.f);last=now;hidScanInput();auto down=hidKeysDown(),held=hidKeysHeld();circlePosition pad;hidCircleRead(&pad);
        if(down&KEY_START){paused=!paused;travel=false;if(paused)message=savePositionNow()?"EXPLORACION GUARDADA":error;}
        if(paused){setWorldLookAhead(0,0);lower({"PAUSA EXPLORACION","BRITANNIA",{message,"","START / A: CONTINUAR","B: VOLVER A TU PARTIDA","","MOCHILA Y MISIONES CONSERVADAS"}});if(down&KEY_A)paused=false;if(down&KEY_B){if(savePositionNow())return "DE REGRESO A BRITAIN";message=error;}last=osGetTime();continue;}
        const bool travelWasOpen=travel;
        if((down&KEY_X)&&!moving){travel=!travel;repeat=0;}
        bool touchTravel=false;
        if((down&KEY_TOUCH)&&map&&!moving&&!travel){touchPosition t;hidTouchRead(&t);if(t.py>=JournalLayout::ZoomTop&&t.py<=JournalLayout::ZoomTop+13){if(t.px>=20&&t.px<=47)zoom=std::max(1,zoom-1);if(t.px>=147&&t.px<=178)zoom=std::min(4,zoom+1);}if(t.px>=196&&t.px<305&&t.py>=25&&t.py<146)travel=true;}
        if((down&KEY_TOUCH)&&travel){touchPosition t;hidTouchRead(&t);int hit=expeditionDestinationHit(travelWasOpen,t.px,t.py);if(hit>=0){choice=hit;touchTravel=true;}}
        if(travel){
            setWorldLookAhead(0,0);
            int d=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);if(!d)repeat=0;else if(now>=repeat){choice=(choice+d+10)%10;repeat=now+180;}
            if(down&KEY_B)travel=false;
            if((down&KEY_A)||touchTravel){if(load(destinations[choice].position,true)){savePositionNow();warm();travel=false;}else{message=error;if(!scene)return error;warm();travel=false;paused=true;}last=osGetTime();}
            std::vector<std::string>lines{"PAD: ELEGIR | A: VIAJAR | B: VOLVER"};for(int i=0;i<10;++i)lines.push_back(std::string(i==choice?"> ":"  ")+destinations[i].name);lower({"DESTINOS","EXPLORACION",lines});continue;
        }
        if(down&KEY_SELECT){roofs=RoofMode((int(roofs)+1)%3);invalidateRenderList();}
        if(down&(KEY_L|KEY_R))map=!map;
        int sx=((held&KEY_RIGHT)||pad.dx>35)-((held&KEY_LEFT)||pad.dx< -35),sy=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);
        const int walkX=moving?(actor.x>from.x)-(actor.x<from.x):std::max(-1,std::min(1,sx+sy));
        const int walkY=moving?(actor.y>from.y)-(actor.y<from.y):std::max(-1,std::min(1,sy-sx));
        setWorldLookAhead(walkX,walkY);
        if(async&&(walkX||walkY)){
            const auto wanted=StreamPlan::wanted(*scene,actor,walkX,walkY,archive->width(),archive->height());
            if(wanted.x!=scene->originX||wanted.y!=scene->originY)stream->request(wanted.x,wanted.y);
        }
        bool waiting=false;
        if(!moving&&(sx||sy)){
            int dx=std::max(-1,std::min(1,sx+sy)),dy=std::max(-1,std::min(1,sy-sx));const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};int direction=0;for(int i=0;i<8;++i)if(xs[i]==dx&&ys[i]==dy)direction=i;facing=(direction+1)&7;
            Actor next;int actual;if(step(*scene,actor,direction,next,actual)){
                if(StreamPlan::insideGuard(*scene,next,archive->width(),archive->height())){from=actor;actor=next;moving=true;progress=0;facing=(actual+1)&7;}
                else waiting=true;
            }
        }
        bool animating=moving,running=(held&KEY_B)!=0;
        if(moving){progress=std::min(1.f,progress+dt/movementSeconds(running,status.mounted));cameraX=from.x+(actor.x-from.x)*progress;cameraY=from.y+(actor.y-from.y)*progress;foot=from.z+(actor.z-from.z)*progress;if(progress>=1)moving=false;}
        if(!moving&&StreamPlan::due(*scene,actor,archive->width(),archive->height())){
            const auto state=stream->status();
            if(state.ready){auto next=stream->take();if(next&&StreamPlan::accepts(*scene,actor,next->world,archive->width(),archive->height())){
                const auto g=ExpeditionWindow::global(*scene,actor);active=std::move(next);scene=&active->world;radar=&active->radar;actor=from=ExpeditionWindow::local(*scene,g);cameraX=actor.x;cameraY=actor.y;foot=actor.z;progress=0;adoptWorldView(std::move(active->roofs));waiting=false;
            }else stream->cancel();}else if(!async||!state.error.empty()){
                const auto g=ExpeditionWindow::global(*scene,actor);if(!load(g,false)){message=error;if(!scene)return error;paused=true;}warm();last=osGetTime();continue;
            }
        }
        if(moving||sx||sy)checkpoint.active(now);
        const auto position=ExpeditionWindow::global(*scene,actor);
        if(!moving&&checkpoint.due(position,now))checkpoint.result(position,now,savePositionNow());
        if(waiting)message="PREPARANDO TERRENO...";else if(!paused)message="START: PAUSA | X: DESTINOS";
        if(animating)phase=std::fmod(phase+dt*(status.mounted?(running?15.f:10.f):(running?13.f:8.f)),status.mounted?5.f:4.f);
        drawWorld(target,*scene,actor,cameraX,cameraY,foot,roofs,facing,int(phase),true,animating,running,nullptr,nullptr,&status);
        auto*b=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
        if(map)drawJournal(b,*scene,actor,radar,zoom,message,&bag,&equipment,&status,&statusArt,nullptr,true);
        else{auto g=ExpeditionWindow::global(*scene,actor);drawExpeditionHud(b,{"BRITANNIA COMPLETA","EXPLORACION EXPERIMENTAL",{"XY "+std::to_string(g.x)+","+std::to_string(g.y)+" Z "+std::to_string(g.z),"VISTA 64X64 / PRECARGA POR DELANTE",message,"PAD: CAMINAR | B: CORRER","X: DESTINOS | START: PAUSA","L / R: MAPA | SELECT: TECHOS","","SIN NPC, LOOT NI MISIONES FUERA","DE LA PARTIDA NORMAL DE BRITAIN","","EL PROGRESO ORIGINAL NO CAMBIA"}});}
        GSPGPU_FlushDataCache(b,320*240*3);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
    }
    savePositionNow();return "EXPLORACION FINALIZADA";
}
}
