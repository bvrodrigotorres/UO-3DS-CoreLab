// SPDX-License-Identifier: MIT
#include "expedition_ui.hpp"
#include "hud.hpp"
#include "developer_menu.hpp"
#include "resource_preparation.hpp"
#include "mount.hpp"
#include "performance.hpp"
#include "world_stream.hpp"
#include "version.hpp"
#include "npc_push.hpp"
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
constexpr Destination destinations[]={{"BRITAIN",{1496,1628,20}},{"CEMENTERIO",{1378,1488,10}},{"BOSQUE OESTE",{1240,1463,0}},{"BOSQUE NORTE",{1377,1381,0}},{"TRINSIC",{1845,2745,0}},{"MOONGLOW",{4467,1283,0}},{"MINOC",{2476,413,15}},{"YEW",{633,858,0}},{"SKARA BRAE",{635,2236,0}},{"VESPER",{2880,700,0}},{"JHELOM",{1380,3815,0}},{"MAGINCIA",{3732,2140,20}},{"BUCCANEER",{2707,2162,0}}};
}
std::string exploreBritannia(C3D_RenderTarget*target,const std::string&base,const std::string&save,const Actor&start,const PlayerStatus&campaignStatus,const Inventory&campaignBag,const EquipmentArt&equipment,const StatusArt&statusArt,RoofMode roofs,const Npcs&regional,const World&region,const ItemIcons&icons,const SaveSink&sink){
    loading("ABRIENDO EL MAPA MUNDIAL");auto archive=std::make_unique<WorldArchive>();auto graphics=std::make_unique<WorldGraphics>();std::string error;
    const auto root=base+"britannia/";
    if(!std::ifstream(root+"britannia.uow").good())return "INSTALA EL PAQUETE MAPA MUNDIAL";
    if(!archive->open(root+"britannia.uow",error)||!graphics->load(root,error))return error;
    World positionWorld;positionWorld.width=archive->width();positionWorld.height=archive->height();positionWorld.compatiblePosition=archive->identity();positionWorld.compatibleInventory=archive->identity()^0x50494c54u;
    // Large fixed combat/ground pools live on the heap, never the ARM11 stack.
    struct Actors{Npcs npcs;Combat combat;Inventory bag;PlayerStatus status;Doors doors;NpcPush push;};auto actors=std::make_unique<Actors>();
    auto&npcs=actors->npcs;auto&combat=actors->combat;auto&bag=actors->bag;auto&status=actors->status;
    npcs.importWindowSpawns(regional,region.originX,region.originY);combat.reset(npcs);status=campaignStatus;
    bag.init(positionWorld,save+".inventory.sav");if(!bag.healthy())return "MOCHILA DE PRUEBA INVALIDA: ARCHIVO CONSERVADO";
    if(!bag.restored){bag.backpack=campaignBag.backpack;bag.equipment=campaignBag.equipment;bag.lastEquipment=campaignBag.lastEquipment;bag.dressed=campaignBag.dressed;bag.accessories=campaignBag.accessories;}
    // Ground drops and chests are not part of this regional corpse-loot pilot.
    bag.ground={};bag.saveSink=sink;
    const auto vitals=save+".vitals.sav";
    if(!loadVitals(vitals,status))return "VIDA DE PRUEBA INVALIDA: ARCHIVO CONSERVADO";
    auto queueVitals=[&](){return saveVitals(vitals,status,std::rename,sink);};
    bag.prepareEquipment=[&](unsigned mask){auto key=mask|(status.mounted?MountedBank:0);if(!prepareEquipment(key,equipment,error))return false;warmEquipment(key,equipment);return true;};
    bag.useItem=[&](size_t slot){if(!combat.useHealing(bag,slot,status))return false;queueVitals();return true;};
    if(!bag.prepareEquipment(bag.equipmentMask())){status.mounted=false;if(!bag.prepareEquipment(bag.equipmentMask()))return "NO SE PUDO PREPARAR EL EQUIPO DE PRUEBA";}
    Actor global=start,restored;if(loadPosition(save,positionWorld,restored,error))global=restored;
    const Actor reset{region.originX+region.startX,region.originY+region.startY,region.startZ};
    auto active=std::make_unique<StreamFrame>();World*scene=&active->world;Journal*radar=&active->radar;std::unique_ptr<WorldStream>stream;
    Actor actor,from;float progress=0,cameraX=0,cameraY=0,foot=0,phase=0;bool moving=false;int facing=4,populationX=0,populationY=0;
    auto rebase=[&](){int dx=populationX-scene->originX,dy=populationY-scene->originY;npcs.shiftWindow(dx,dy);combat.shiftWindow(dx,dy);bag.closeContainer();populationX=scene->originX;populationY=scene->originY;actors->push.reset();npcs.prepareWindow(*scene,actor);};
    auto trace=[&](const char*stage,const Actor&where){auto memory=mallinfo();FILE*f=std::fopen((base+"expedition-load.log").c_str(),"w");if(f){std::fprintf(f,"v%s %s XY %d,%d Z %d heap_used %d heap_free %d linear_free %lu NPC %u/%lu\n",Version,stage,where.x,where.y,where.z,memory.uordblks,memory.fordblks,(unsigned long)linearSpaceFree(),npcs.activeCount(),(unsigned long)npcs.all().size());std::fclose(f);}};
    auto load=[&](const Actor&where,bool safe){
        loading("CARGANDO DESTINO");if(stream)stream->cancelAndWait();setWorldLookAhead(0,0);releaseWorldResources();
        const bool previous=scene&&!scene->land.empty();const Actor old=previous?ExpeditionWindow::global(*scene,actor):where;
        int oldX=previous?scene->originX:0,oldY=previous?scene->originY:0;active.reset();scene=nullptr;radar=nullptr;
        auto attempt=[&](const Actor&point,bool safeLanding,int x,int y){
            auto next=prepareStreamFrame(*archive,*graphics,x,y,WorldArchive::MaxObjects,error);if(!next)return false;Actor candidate=ExpeditionWindow::local(next->world,point);
            if(safeLanding){Npcs empty;Actor landing;if(!safeTeleportTile(next->world,empty,candidate,landing)){candidate.z=heights(next->world,candidate.x,candidate.y).average;if(!safeTeleportTile(next->world,empty,candidate,landing)){error="NO HAY UN PUNTO SEGURO";return false;}}candidate=landing;}
            active=std::move(next);scene=&active->world;radar=&active->radar;actor=from=candidate;progress=0;moving=false;rebase();
            if(safeLanding&&npcs.occupied(actor)){Actor landing;if(!safeTeleportTile(*scene,npcs,actor,landing)){error="NO HAY LLEGADA LIBRE DE NPC";return false;}actor=from=landing;npcs.prepareWindow(*scene,actor);}
            cameraX=actor.x;cameraY=actor.y;foot=actor.z;adoptWorldView(std::move(active->roofs));return true;
        };
        if(!attempt(where,safe,StreamPlan::origin(where.x,archive->width()),StreamPlan::origin(where.y,archive->height()))){auto failure=error;if(previous)attempt(old,false,oldX,oldY);error=failure;return false;}
        trace("destination-ready",where);return true;
    };
    if(!load(global,true))return error;
    if(!loadAtlases(graphics->images(),error,root))return error;
    auto warm=[&](){trace("warming",ExpeditionWindow::global(*scene,actor));ResourcePreparation prep;u64 until=osGetTime()+10000;while(aptMainLoop()&&osGetTime()<until&&prep.pending()){
        npcs.prepareWindow(*scene,actor);drawWorld(target,*scene,actor,cameraX,cameraY,foot,roofs,facing,0,true,false,false,&npcs,&combat,&status,&bag,false);prep.observe(rendererResourceWaits(),rendererTextureError());loading("PREPARANDO TERRENO Y CRIATURAS");
    }trace("ready",ExpeditionWindow::global(*scene,actor));};warm();
    stream=std::make_unique<WorldStream>([&](int x,int y,std::string&failure){return prepareStreamFrame(*archive,*graphics,x,y,StreamPlan::MaxAheadObjects,failure);});
    const bool async=stream->start();PositionCheckpoint checkpoint;checkpoint.baseline(ExpeditionWindow::global(*scene,actor),osGetTime());
    std::string message="PRUEBA NPC: BRITAIN Y ALREDEDORES";bool paused=false,travel=false,saveAfterPause=false;int choice=0,zoom=2,page=0;u64 repeat=0,last=osGetTime(),lastAuto=last;FramePacer pacer;
    auto saveAll=[&](){error.clear();bool ok=savePosition(save,positionWorld,ExpeditionWindow::global(*scene,actor),error,std::rename,sink)&&bag.checkpoint()&&queueVitals();if(!ok&&error.empty())error="GUARDADO DE PRUEBA PENDIENTE";return ok;};
    constexpr int destinationCount=sizeof(destinations)/sizeof(destinations[0]);
    while(aptMainLoop()){
        auto delay=pacer.delay(osGetTime()*1000);if(delay)svcSleepThread(delay*1000);
        u64 now=osGetTime();float dt=std::min(.05f,(now-last)/1000.f);last=now;hidScanInput();auto down=hidKeysDown(),held=hidKeysHeld();circlePosition pad;hidCircleRead(&pad);
        if(down&KEY_START){paused=!paused;travel=false;if(paused){saveAfterPause=true;message="GUARDANDO PRUEBA...";}}
        if(paused){
            setWorldLookAhead(0,0);lower({"PAUSA EXPLORACION","PRUEBA REGIONAL",{message,"","START / A: CONTINUAR","B: VOLVER A TU PARTIDA","","MOCHILA DE PRUEBA SEPARADA","NPC: ESTADO DE ESTA EXPLORACION"}});
            if(saveAfterPause){saveAfterPause=false;message=saveAll()?"PRUEBA GUARDADA":error;}
            if(down&KEY_A)paused=false;
            if(down&KEY_B){if(saveAll())return "DE REGRESO A BRITAIN";message=error;}
            last=osGetTime();continue;
        }
        const bool travelWasOpen=travel;
        if((down&KEY_X)&&!moving){travel=!travel;repeat=0;}
        bool touchTravel=false;
        if((down&KEY_TOUCH)&&page==0&&!travel){touchPosition t;hidTouchRead(&t);auto action=expeditionTouch(t.px,t.py);
            if(action==ExpeditionTouch::Mount){
                if(moving)message="DETENTE PARA MONTAR O DESMONTAR";
                else {unsigned key=bag.equipmentMask()|(status.mounted?0:MountedBank);
                    if(prepareEquipment(key,equipment,error)){status.mounted=!status.mounted;warmEquipment(key,equipment);phase=0;queueVitals();message=status.mounted?"A CABALLO | B: GALOPAR":"HAS DESMONTADO";}
                    else message="CABALLO: REVISA CHARACTER/MOUNTED";
                    last=osGetTime();
                }
            }else if(!moving){if(action==ExpeditionTouch::ZoomOut)zoom=std::max(1,zoom-1);if(action==ExpeditionTouch::ZoomIn)zoom=std::min(4,zoom+1);if(action==ExpeditionTouch::Travel)travel=true;}
        }
        if((down&KEY_TOUCH)&&travel){touchPosition t;hidTouchRead(&t);int hit=expeditionDestinationHit(travelWasOpen,t.px,t.py,destinationCount);if(hit>=0){choice=hit;touchTravel=true;}}
        if(travel){
            setWorldLookAhead(0,0);int d=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);if(!d)repeat=0;else if(now>=repeat){choice=(choice+d+destinationCount)%destinationCount;repeat=now+180;}
            if(down&KEY_B)travel=false;
            if((down&KEY_A)||touchTravel){combat.attacking=false;combat.target=-1;if(load(destinations[choice].position,true)){saveAll();warm();travel=false;}else{message=error;if(!scene)return error;warm();travel=false;paused=true;}last=osGetTime();}
            std::vector<std::string>lines{"PAD: ELEGIR | A: VIAJAR | B: VOLVER"};for(int i=0;i<destinationCount;++i)lines.push_back(std::string(i==choice?"> ":"  ")+destinations[i].name);lower({"DESTINOS","EXPLORACION",lines});continue;
        }
        if(down&KEY_SELECT){roofs=RoofMode((int(roofs)+1)%3);invalidateRenderList();}
        if(down&(KEY_L|KEY_R)){page=(page+((down&KEY_R)?1:2))%3;bag.closeContainer();}
        combat.locate(actor);
        if((down&KEY_TOUCH)&&page==1){touchPosition t;hidTouchRead(&t);bag.tap(*scene,actor,t.px,t.py);message=bag.message;queueVitals();}
        if(down&KEY_Y){combat.repeatHealing(bag,status);message=bag.message;queueVitals();}
        if(down&KEY_A){combat.toggle(npcs,actor);message=combat.message;}
        int sx=((held&KEY_RIGHT)||pad.dx>35)-((held&KEY_LEFT)||pad.dx< -35),sy=((held&KEY_DOWN)||pad.dy< -35)-((held&KEY_UP)||pad.dy>35);
        const int walkX=moving?(actor.x>from.x)-(actor.x<from.x):std::max(-1,std::min(1,sx+sy));
        const int walkY=moving?(actor.y>from.y)-(actor.y<from.y):std::max(-1,std::min(1,sy-sx));setWorldLookAhead(walkX,walkY);
        if(async&&(walkX||walkY)){const auto wanted=StreamPlan::wanted(*scene,actor,walkX,walkY,archive->width(),archive->height());if(wanted.x!=scene->originX||wanted.y!=scene->originY)stream->request(wanted.x,wanted.y);}
        npcs.update(*scene,actor,from,moving,dt,actors->doors);bool waiting=false;
        if(!moving&&(sx||sy)){
            int dx=std::max(-1,std::min(1,sx+sy)),dy=std::max(-1,std::min(1,sy-sx));const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};int direction=0;for(int i=0;i<8;++i)if(xs[i]==dx&&ys[i]==dy)direction=i;facing=(direction+1)&7;
            Actor next;int actual;if(step(*scene,actor,direction,next,actual)){
                if(!StreamPlan::insideGuard(*scene,next,archive->width(),archive->height()))waiting=true;
                else if(actors->push.allow(npcs.occupied(next),actor,next,direction,dt)){from=actor;actor=next;moving=true;progress=0;facing=(actual+1)&7;}
            }else actors->push.reset();
        }else if(!moving)actors->push.reset();
        bool animating=moving,running=(held&KEY_B)!=0;
        if(moving){progress=std::min(1.f,progress+dt/movementSeconds(running,status.mounted));cameraX=from.x+(actor.x-from.x)*progress;cameraY=from.y+(actor.y-from.y)*progress;foot=from.z+(actor.z-from.z)*progress;if(progress>=1)moving=false;}
        if(!moving&&StreamPlan::due(*scene,actor,archive->width(),archive->height())){
            const auto state=stream->status();if(state.ready){auto next=stream->take();if(next&&StreamPlan::accepts(*scene,actor,next->world,archive->width(),archive->height())){
                const auto g=ExpeditionWindow::global(*scene,actor);active=std::move(next);scene=&active->world;radar=&active->radar;actor=from=ExpeditionWindow::local(*scene,g);cameraX=actor.x;cameraY=actor.y;foot=actor.z;progress=0;rebase();adoptWorldView(std::move(active->roofs));waiting=false;
            }else stream->cancel();}else if(!async||!state.error.empty()){const auto g=ExpeditionWindow::global(*scene,actor);if(!load(g,false)){message=error;if(!scene)return error;paused=true;}warm();last=osGetTime();continue;}
        }
        if(combat.update(*scene,npcs,actor,moving,dt,status,bag)){
            bag.prepareEquipment(bag.equipmentMask());if(!load(reset,true))return error;saveAll();warm();last=osGetTime();message="HAS MUERTO | REGRESAS AL INICIO";continue;
        }
        if(combat.attacking&&combat.target>=0&&!moving){const auto&enemy=npcs.all()[combat.target];int dx=(enemy.position.x>actor.x)-(enemy.position.x<actor.x),dy=(enemy.position.y>actor.y)-(enemy.position.y<actor.y);const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};for(int d=0;d<8;++d)if(xs[d]==dx&&ys[d]==dy)facing=(d+1)&7;}
        if(page==1)combat.inspectNearest(*scene,actor,bag);
        if(moving||sx||sy)checkpoint.active(now);
        const auto position=ExpeditionWindow::global(*scene,actor);
        if(!moving&&checkpoint.due(position,now)){checkpoint.result(position,now,saveAll());lastAuto=now;}
        else if(!moving&&!combat.attacking&&now-lastAuto>=60000){saveAll();lastAuto=now;}
        if(waiting)message="PREPARANDO TERRENO...";
        if(animating)phase=std::fmod(phase+dt*(status.mounted?(running?15.f:10.f):(running?13.f:8.f)),status.mounted?5.f:4.f);
        unsigned key=bag.equipmentMask()|(status.mounted?MountedBank:0);if(combat.combatPose()&&!moving&&!status.mounted){if(!selectEquipment(key|4096u))selectEquipment(key);}else selectEquipment(key);
        drawWorld(target,*scene,actor,cameraX,cameraY,foot,roofs,facing,int(phase),true,animating,running,&npcs,&combat,&status,&bag);
        auto*b=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,nullptr,nullptr);
        if(page==0)drawJournal(b,*scene,actor,radar,zoom,message,&bag,&equipment,&status,&statusArt,&npcs,true);
        else if(page==1)drawInventory(b,*scene,actor,bag,&icons,nullptr,&status,true);
        else {auto g=ExpeditionWindow::global(*scene,actor);drawExpeditionHud(b,{"BRITANNIA / PILOTO NPC","PRUEBA SEPARADA",{"XY "+std::to_string(g.x)+","+std::to_string(g.y)+" Z "+std::to_string(g.z),"NPC ACTIVOS "+std::to_string(npcs.activeCount())+" / "+std::to_string(npcs.all().size()),"BRITAIN, CEMENTERIO Y BOSQUES",message,"A: ATACAR / PARAR | Y: CURAR","R/L: MAPA, MOCHILA, INFORMACION","TOCA EL CABALLO PARA MONTAR","","MOCHILA DE PRUEBA GUARDADA APARTE","NPC SE REINICIAN AL VOLVER A ENTRAR","LA CAMPANA NORMAL NO CAMBIA"}});}
        GSPGPU_FlushDataCache(b,320*240*3);gfxScreenSwapBuffers(GFX_BOTTOM,false);gspWaitForVBlank();
    }
    saveAll();return "EXPLORACION FINALIZADA";
}
}
