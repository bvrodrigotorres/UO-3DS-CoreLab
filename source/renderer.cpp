// SPDX-License-Identifier: MIT
#include "renderer.hpp"
#include "avatar.hpp"
#include "repairs.hpp"
#include "world_shbin.h"
#include <3ds.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
namespace lab {
struct Vertex {float x,y,z,u,v,r,g,b,a;};
struct Batch {C3D_Tex*texture;int first,count;};
static Vertex*buffer;
static DVLB_s* shader;
static shaderProgram_s program;
static C3D_Mtx projection;
static int uniformProjection;
static C3D_Tex white;
static std::vector<C2D_SpriteSheet> sheets;
static C2D_SpriteSheet avatar;
static std::vector<C2D_SpriteSheet>npcSheets;
static size_t npcBytes;
static bool runVerified=false;
static size_t bytes,objects;
static int used;
static bool overflow;
static std::vector<Batch> batches;
static RenderCache renderCache;
static constexpr int Capacity=65536;
static void push(Vertex v,C3D_Tex*texture){
    if(used>=Capacity){overflow=true;return;}
    if(batches.empty()||batches.back().texture!=texture)batches.push_back({texture,used,0});
    buffer[used++]=v;++batches.back().count;
}
struct Point {float x,y;};
static void quad(const Point*p,const Point*uv,C3D_Tex*texture,float r,float g,float b,float a){
    const int indices[]={0,1,2,2,3,0};
    for(int i:indices)push({p[i].x,p[i].y,0.f,uv[i].x,uv[i].y,r,g,b,a},texture);
}
static void uvFor(C2D_Image image,Point*uv){
    Tex3DS_SubTextureTopLeft(image.subtex,&uv[0].x,&uv[0].y);
    Tex3DS_SubTextureTopRight(image.subtex,&uv[1].x,&uv[1].y);
    Tex3DS_SubTextureBottomRight(image.subtex,&uv[2].x,&uv[2].y);
    Tex3DS_SubTextureBottomLeft(image.subtex,&uv[3].x,&uv[3].y);
}
static bool imageFor(unsigned id,C2D_Image&image){
    if(id==NoImage||id/32>=sheets.size())return false;
    auto sheet=sheets[id/32];if(id%32>=C2D_SpriteSheetCount(sheet))return false;
    image=C2D_SpriteSheetGetImage(sheet,id%32);return true;
}
static const Point whiteUV[]={{0,1},{1,1},{1,0},{0,0}};
static void rect(float x,float y,float w,float h,float r,float g,float b,float a=1){
    Point p[]={{x,y},{x+w,y},{x+w,y+h},{x,y+h}};quad(p,whiteUV,&white,r,g,b,a);
}
bool rendererInit(){
    buffer=(Vertex*)linearAlloc(sizeof(Vertex)*Capacity);if(!buffer)return false;
    shader=DVLB_ParseFile((u32*)world_shbin,world_shbin_size);if(!shader){linearFree(buffer);buffer=nullptr;return false;}
    shaderProgramInit(&program);shaderProgramSetVsh(&program,&shader->DVLE[0]);
    uniformProjection=shaderInstanceGetUniformLocation(program.vertexShader,"projection");
    Mtx_OrthoTilt(&projection,0,400,240,0,-1,1,true);
    if(!C3D_TexInit(&white,8,8,GPU_RGBA8))return false;
    std::memset(white.data,255,white.size);C3D_TexFlush(&white);
    C3D_TexSetFilter(&white,GPU_NEAREST,GPU_NEAREST);
    avatar=C2D_SpriteSheetLoad("sdmc:/3ds/UO-3DS-CoreLab/player_anim.t3x");
    if(avatar&&C2D_SpriteSheetCount(avatar)<40){C2D_SpriteSheetFree(avatar);avatar=nullptr;}
    runVerified=avatar&&C2D_SpriteSheetCount(avatar)==72&&verifyRunAtlas(
        "sdmc:/3ds/UO-3DS-CoreLab/player_anim.t3x","sdmc:/3ds/UO-3DS-CoreLab/player_anim.meta");
    batches.reserve(512);return true;
}
void rendererShutdown(){
    C3D_FrameSync();for(auto s:sheets)C2D_SpriteSheetFree(s);sheets.clear();
    if(avatar)C2D_SpriteSheetFree(avatar);
    for(auto s:npcSheets)C2D_SpriteSheetFree(s);
    npcSheets.clear();
    C3D_TexDelete(&white);if(buffer)linearFree(buffer);
    shaderProgramFree(&program);if(shader)DVLB_Free(shader);
}
bool loadAtlases(int images,std::string&error){
    std::vector<C2D_SpriteSheet> next;size_t size=0;
    for(int i=0;i<(images+31)/32;++i){
        char path[128];std::snprintf(path,sizeof(path),"sdmc:/3ds/UO-3DS-CoreLab/atlas_%02d.t3x",i);
        auto sheet=C2D_SpriteSheetLoad(path);int required=std::min(32,images-i*32);
        if(!sheet||C2D_SpriteSheetCount(sheet)<(size_t)required){
            if(sheet)C2D_SpriteSheetFree(sheet);
            for(auto s:next)C2D_SpriteSheetFree(s);
            error="Missing/invalid atlas; private scene disabled";return false;
        }
        auto image=C2D_SpriteSheetGetImage(sheet,0);size+=image.tex->size;
        C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);next.push_back(sheet);
    }
    C3D_FrameSync();for(auto s:sheets)C2D_SpriteSheetFree(s);
    sheets=std::move(next);bytes=size;return true;
}
bool loadNpcAtlases(std::string&error){
    std::vector<C2D_SpriteSheet>next;size_t usedBytes=0;
    for(int i=0;i<3;++i){
        char path[128];std::snprintf(path,sizeof(path),"sdmc:/3ds/UO-3DS-CoreLab/npc_%02d.t3x",i);
        auto s=C2D_SpriteSheetLoad(path);
        if(!s||C2D_SpriteSheetCount(s)!=40){if(s)C2D_SpriteSheetFree(s);for(auto t:next)C2D_SpriteSheetFree(t);error="Missing/invalid NPC atlas";return false;}
        auto image=C2D_SpriteSheetGetImage(s,0);C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);usedBytes+=image.tex->size;next.push_back(s);
    }
    for(auto s:npcSheets)C2D_SpriteSheetFree(s);
    npcSheets=std::move(next);npcBytes=usedBytes;error.clear();return true;
}
size_t textureBytes(){return bytes+npcBytes;}
size_t drawnObjects(){return objects;}
bool rendererOverflow(){return overflow;}
bool hasRunAnimation(){return runVerified;}
unsigned renderListBuilds(){return renderCache.rebuilds;}
void invalidateRenderList(){renderCache.invalidate();}
void drawWorld(C3D_RenderTarget*target,const World&w,const Actor&actor,float cameraX,float cameraY,float footZ,
               RoofMode roofs,int direction,int frame,bool authentic,bool moving,bool running,const Npcs*npcs){
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_RenderTargetClear(target,C3D_CLEAR_ALL,0x14212fff,0);
    C3D_FrameDrawOn(target);used=0;objects=0;overflow=false;batches.clear();
    auto project=[&](float x,float y,float z)->Point{return {200+(x-cameraX-y+cameraY)*22,126+(x-cameraX+y-cameraY)*22-(z-footZ)*4};};
    const auto&list=renderCache.get(w,actor,roofs!=RoofMode::Full);
    bool afterActor=false;
    auto npcList=npcs?npcs->drawList(w,actor,roofs!=RoofMode::Full):std::vector<Draw>{};
    for(size_t wi=0,ni=0;wi<list.size()||ni<npcList.size();){
        bool takeNpc=ni<npcList.size()&&(wi==list.size()||drawBefore(npcList[ni],list[wi]));
        const auto&d=takeNpc?npcList[ni++]:list[wi++];
        auto center=project(d.x,d.y,d.z);
        if(center.x< -180||center.x>580||center.y< -220||center.y>460)continue;
        ++objects;
        if(d.kind==Kind::Land){
            auto h=heights(w,d.x,d.y);const auto&land=w.land[d.index];
            if(authentic&&land.art==2)continue;
            Point p[]={project(d.x,d.y,h.top),project(d.x+1,d.y,h.right),project(d.x+1,d.y+1,h.bottom),project(d.x,d.y+1,h.left)};
            for(auto&v:p)v.y-=22; // tile origin is the diamond center
            C2D_Image image;
            if(authentic&&imageFor(land.image,image)){
                Point uv[4];uvFor(image,uv);
                if(!land.texture){Point rotated[4];for(int i=0;i<4;++i)rotated[i]={(uv[i].x+uv[(i+1)%4].x)*.5f,(uv[i].y+uv[(i+1)%4].y)*.5f};std::copy(rotated,rotated+4,uv);}
                if(std::abs(h.top-h.bottom)>std::abs(h.left-h.right)){std::rotate(p,p+1,p+4);std::rotate(uv,uv+1,uv+4);}
                quad(p,uv,image.tex,1,1,1,1);
            }else{
                if(std::abs(h.top-h.bottom)>std::abs(h.left-h.right))std::rotate(p,p+1,p+4);
                float shade=((d.x+d.y)&1)?.8f:1.f;
                quad(p,whiteUV,&white,.23f*shade,.48f*shade,.31f*shade,1);
            }
        }else if(d.kind==Kind::Static){
            const auto&o=w.objects[d.index];C2D_Image image;
            if(authentic&&imageFor(o.image,image)){
                float width=image.subtex->width,height=image.subtex->height;
                Point p[]={{center.x-width/2,center.y+22-height},{center.x+width/2,center.y+22-height},{center.x+width/2,center.y+22},{center.x-width/2,center.y+22}};
                float alpha=objectOpacity(o,footZ,roofs,afterActor,{p[0].x,p[0].y,p[2].x,p[2].y});
                if(alpha==0)continue;
                // Fill only the reported exposed wall footprint, before drawing
                // the wall. Hidden upper levels suppress both pieces together.
                if(const auto*floor=floorUnderWall(w,o)){
                    C2D_Image wood;
                    if(imageFor(floor->image,wood)){
                        float fw=wood.subtex->width,fh=wood.subtex->height;
                        Point fp[]={{center.x-fw/2,center.y+22-fh},{center.x+fw/2,center.y+22-fh},{center.x+fw/2,center.y+22},{center.x-fw/2,center.y+22}};
                        float fa=objectOpacity(*floor,footZ,roofs,afterActor,{fp[0].x,fp[0].y,fp[2].x,fp[2].y});
                        Point fuv[4];uvFor(wood,fuv);if(fa>0){quad(fp,fuv,wood.tex,1,1,1,fa);++objects;}
                    }
                }
                Point uv[4];uvFor(image,uv);quad(p,uv,image.tex,1,1,1,alpha);
            }else{
                if(o.flags&Wall){
                    Point p[]={{center.x-22,center.y},{center.x,center.y+22},{center.x,center.y+22-o.height*4},{center.x-22,center.y-o.height*4}};
                    float alpha=objectOpacity(o,footZ,roofs,afterActor,{center.x-22,center.y-o.height*4,center.x,center.y+22});
                    if(alpha>0)quad(p,whiteUV,&white,.58f,.62f,.66f,alpha);
                }else{
                    Point p[]={{center.x,center.y-22},{center.x+22,center.y},{center.x,center.y+22},{center.x-22,center.y}};
                    bool roof=o.flags&Roof;bool rug=o.art==5;
                    float alpha=objectOpacity(o,footZ,roofs,afterActor,{center.x-22,center.y-22,center.x+22,center.y+22});
                    if(alpha>0)quad(p,whiteUV,&white,rug?.18f:roof?.55f:.63f,rug?.3f:roof?.24f:.44f,rug?.8f:roof?.17f:.24f,alpha);
                }
            }
        }else if(d.kind==Kind::Npc){
            const auto&n=npcs->all()[d.index];
            if((size_t)n.style>=npcSheets.size())continue;
            float t=n.progress;
            auto foot=project(n.from.x+(n.position.x-n.from.x)*t,n.from.y+(n.position.y-n.from.y)*t,n.from.z+(n.position.z-n.from.z)*t);
            auto image=C2D_SpriteSheetGetImage(npcSheets[n.style],avatarImage(n.direction,n.moving,false,(unsigned)n.animation,40));
            float width=image.subtex->width,height=image.subtex->height;
            Point p[]={{foot.x-width/2,foot.y-height+5},{foot.x+width/2,foot.y-height+5},{foot.x+width/2,foot.y+5},{foot.x-width/2,foot.y+5}};
            Point uv[4];uvFor(image,uv);quad(p,uv,image.tex,1,1,1,1);
        }else{
            afterActor=true;
            Point foot=project(cameraX,cameraY,footZ);
            if(avatar){
                unsigned index=avatarImage(direction,moving,running&&runVerified,frame,C2D_SpriteSheetCount(avatar));
                auto image=C2D_SpriteSheetGetImage(avatar,index);
                float width=image.subtex->width,height=image.subtex->height;
                Point p[]={{foot.x-width/2,foot.y-height+5},{foot.x+width/2,foot.y-height+5},{foot.x+width/2,foot.y+5},{foot.x-width/2,foot.y+5}};
                Point uv[4];uvFor(image,uv);quad(p,uv,image.tex,1,1,1,1);
            }else{rect(foot.x-5,foot.y-24,10,24,.2f,.45f,.95f);rect(foot.x-4,foot.y-32,8,8,.95f,.75f,.55f);}
        }
    }
    C3D_BindProgram(&program);C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,uniformProjection,&projection);
    auto attrs=C3D_GetAttrInfo();AttrInfo_Init(attrs);AttrInfo_AddLoader(attrs,0,GPU_FLOAT,3);AttrInfo_AddLoader(attrs,1,GPU_FLOAT,2);AttrInfo_AddLoader(attrs,2,GPU_FLOAT,4);
    auto buffers=C3D_GetBufInfo();BufInfo_Init(buffers);BufInfo_Add(buffers,buffer,sizeof(Vertex),3,0x210);
    C3D_CullFace(GPU_CULL_NONE);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_AlphaTest(true,GPU_GREATER,0);C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
    auto env=C3D_GetTexEnv(0);C3D_TexEnvInit(env);C3D_TexEnvSrc(env,C3D_Both,GPU_TEXTURE0,GPU_PRIMARY_COLOR,GPU_PRIMARY_COLOR);C3D_TexEnvFunc(env,C3D_Both,GPU_MODULATE);
    GSPGPU_FlushDataCache(buffer,used*sizeof(Vertex));
    for(const auto&batch:batches){C3D_TexBind(0,batch.texture);C3D_DrawArrays(GPU_TRIANGLES,batch.first,batch.count);}
    C3D_FrameEnd(0);
}
}
