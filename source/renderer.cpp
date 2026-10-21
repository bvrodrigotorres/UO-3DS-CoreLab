// SPDX-License-Identifier: MIT
#include "renderer.hpp"
#include "avatar.hpp"
#include "repairs.hpp"
#include "character_depth.hpp"
#include "performance.hpp"
#include "view.hpp"
#include "roof_cutaway.hpp"
#include "overhead_font.hpp"
#include "world_shbin.h"
#include <3ds.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
namespace lab {
struct Vertex {float x,y,z,u,v,r,g,b,a;};
enum class PixelMode { World,Support,Character,Translucent };
struct Batch {C3D_Tex*texture;int first,count;PixelMode mode;};
static PixelMode pixelMode=PixelMode::World;
static Vertex*buffer;
static DVLB_s* shader;
static shaderProgram_s program;
static C3D_Mtx projection;
static int uniformProjection;
static C3D_Tex white,labelFont;
static OverheadFont overheadFont;
static std::vector<C2D_SpriteSheet> sheets;
static bool streamed=false,textureFailure=false;
static std::string atlasRoot;
static std::vector<std::array<unsigned,2>>imageSizes;
static std::vector<size_t>sheetCosts;
static std::vector<uint64_t>sheetUsed;
static uint64_t textureClock=0;
static constexpr size_t WorldTextureBudget=10*1024*1024;
static C2D_SpriteSheet avatar,outfit;
static unsigned outfitMask=99;
static EquipmentCache<C2D_SpriteSheet>outfitCache;
static std::vector<C2D_SpriteSheet>npcSheets;
static size_t npcBytes;
static bool runVerified=false;
static size_t bytes,objects;
static int used;
static bool overflow;
static std::vector<Batch> batches;
static RenderCache renderCache;
static RoofCutaway roofCutaway;
static std::vector<Draw>mergedDraws;
static constexpr int Capacity=65536;
static void push(Vertex v,C3D_Tex*texture){
    if(used>=Capacity){overflow=true;return;}
    const auto state=pixelMode==PixelMode::Character?PixelMode::World:pixelMode;
    if(batches.empty()||batches.back().texture!=texture||batches.back().mode!=state)batches.push_back({texture,used,0,state});
    buffer[used++]=v;++batches.back().count;
}
struct Point {float x,y;};
static void quad(const Point*p,const Point*uv,C3D_Tex*texture,float r,float g,float b,float a){
    const auto savedMode=pixelMode;
    if(pixelMode==PixelMode::Support){
        float left=p[0].x,right=left,top=p[0].y,bottom=top;
        for(int i=1;i<4;++i){left=std::min(left,p[i].x);right=std::max(right,p[i].x);top=std::min(top,p[i].y);bottom=std::max(bottom,p[i].y);}
        // No protected player texels exist outside its sprite footprint.
        if(right<=152||left>=248||bottom<=19||top>=131)pixelMode=PixelMode::World;
    }
    const int indices[]={0,1,2,2,3,0};
    for(int i:indices)push({p[i].x,p[i].y,pixelMode==PixelMode::Character?CharacterDepthZ:WorldDepthZ,uv[i].x,uv[i].y,r,g,b,a},texture);
    pixelMode=savedMode;
}
static void uvFor(C2D_Image image,Point*uv){
    Tex3DS_SubTextureTopLeft(image.subtex,&uv[0].x,&uv[0].y);
    Tex3DS_SubTextureTopRight(image.subtex,&uv[1].x,&uv[1].y);
    Tex3DS_SubTextureBottomRight(image.subtex,&uv[2].x,&uv[2].y);
    Tex3DS_SubTextureBottomLeft(image.subtex,&uv[3].x,&uv[3].y);
}
static bool imageFor(unsigned id,C2D_Image&image){
    if(id==NoImage||id/32>=sheets.size())return false;
    auto sheet=sheets[id/32];if(!sheet)return false;if(id%32>=C2D_SpriteSheetCount(sheet))return false;
    image=C2D_SpriteSheetGetImage(sheet,id%32);return true;
}
static const Point whiteUV[]={{0,1},{1,1},{1,0},{0,0}};
static void rect(float x,float y,float w,float h,float r,float g,float b,float a=1){
    Point p[]={{x,y},{x+w,y},{x+w,y+h},{x,y+h}};quad(p,whiteUV,&white,r,g,b,a);
}
static void labelText(float x,float y,const std::string&text,bool outline,float r,float g,float b){
    unsigned width=0;
    for(size_t i=0;i<text.size()&&i<30;++i){unsigned char c=text[i];if(c>=128)c='?';unsigned advance=overheadFont.step(c);if(width+advance>360)break;
        if(c!=' '){float u=float((c%16)*16+(outline?256:0))/512,v=1-float(c/16)*16/128;
            Point p[]={{x,y},{x+16,y},{x+16,y+16},{x,y+16}};
            Point uv[]={{u,v},{u+16.f/512,v},{u+16.f/512,v-16.f/128},{u,v-16.f/128}};
            quad(p,uv,&labelFont,r,g,b,1);
        }
        width+=advance;x+=advance;
    }
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
    overheadFont.load("sdmc:/3ds/UO-3DS-CoreLab/npc_font.bin");
    // Explicit white RGB plus alpha; A8 supplies no usable RGB on this path.
    if(!C3D_TexInit(&labelFont,512,128,GPU_RGBA5551))return false;
    std::memset(labelFont.data,0,labelFont.size);
    for(unsigned c=0;c<128;++c)for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x){
        unsigned xx=(c%16)*16+x,yy=(c/16)*16+y;
        if(overheadFont.pixel(c,x,y))static_cast<uint16_t*>(labelFont.data)[overheadPixelOffset(xx,yy)]=0xFFFF;
        if(overheadFont.outline(c,x,y))static_cast<uint16_t*>(labelFont.data)[overheadPixelOffset(xx+256,yy)]=0xFFFF;
    }
    C3D_TexFlush(&labelFont);C3D_TexSetFilter(&labelFont,GPU_NEAREST,GPU_NEAREST);
    avatar=C2D_SpriteSheetLoad("sdmc:/3ds/UO-3DS-CoreLab/player_anim.t3x");
    if(avatar&&C2D_SpriteSheetCount(avatar)<40){C2D_SpriteSheetFree(avatar);avatar=nullptr;}
    runVerified=avatar&&C2D_SpriteSheetCount(avatar)==72&&verifyRunAtlas(
        "sdmc:/3ds/UO-3DS-CoreLab/player_anim.t3x","sdmc:/3ds/UO-3DS-CoreLab/player_anim.meta");
    batches.reserve(512);return true;
}
void rendererShutdown(){
    C3D_FrameSync();for(auto s:sheets)if(s)C2D_SpriteSheetFree(s);sheets.clear();
    if(avatar)C2D_SpriteSheetFree(avatar);
    for(unsigned i=0;i<8;++i)if(auto cached=outfitCache.remove(i))C2D_SpriteSheetFree(cached);
    for(auto s:npcSheets)C2D_SpriteSheetFree(s);
    npcSheets.clear();
    C3D_TexDelete(&labelFont);C3D_TexDelete(&white);if(buffer)linearFree(buffer);
    shaderProgramFree(&program);if(shader)DVLB_Free(shader);
}
bool loadAtlases(int images,std::string&error,const std::string&root){
    streamed=images>512;atlasRoot=root;textureFailure=false;
    if(streamed){
        std::ifstream f(root+"image_sizes.bin",std::ios::binary);
        std::vector<unsigned char>b(8+size_t(images)*4+size_t((images+31)/32)*4);
        if(!f.read((char*)b.data(),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UOI1",4)){
            error="Invalid large-map image metadata";return false;
        }
        auto n=[&](size_t at,int count){unsigned value=0;for(int i=0;i<count;++i)value|=unsigned(b[at+i])<<(8*i);return value;};
        unsigned pages=(images+31)/32;
        if(n(4,2)!=unsigned(images)||n(6,2)!=pages){error="Large-map image count mismatch";return false;}
        imageSizes.clear();sheetCosts.clear();
        for(int i=0;i<images;++i){unsigned w=n(8+i*4,2),h=n(10+i*4,2);if(!w||!h||w>1024||h>1024){error="Invalid image size";return false;}imageSizes.push_back({w,h});}
        for(unsigned i=0;i<pages;++i){unsigned cost=n(8+images*4+i*4,4);if(!cost||cost>3*1024*1024){error="Invalid atlas size";return false;}char name[32];std::snprintf(name,sizeof name,"atlas_%02u.t3x",i);
            std::ifstream file(root+name,std::ios::binary|std::ios::ate);
            if(!file||file.tellg()!=std::streamoff(cost)){error="Missing/incomplete large-map atlas";return false;}
            sheetCosts.push_back(cost);}
        C3D_FrameSync();for(auto sheet:sheets)if(sheet)C2D_SpriteSheetFree(sheet);
        sheets.assign(pages,nullptr);sheetUsed.assign(pages,0);bytes=0;error.clear();return true;
    }
    std::vector<C2D_SpriteSheet> next;size_t size=0;
    for(int i=0;i<(images+31)/32;++i){
        char name[32];std::snprintf(name,sizeof(name),"atlas_%02d.t3x",i);std::string path=root+name;
        auto sheet=C2D_SpriteSheetLoad(path.c_str());int required=std::min(32,images-i*32);
        if(!sheet||C2D_SpriteSheetCount(sheet)<(size_t)required){
            if(sheet)C2D_SpriteSheetFree(sheet);
            for(auto s:next)C2D_SpriteSheetFree(s);
            error="Missing/invalid atlas; private scene disabled";return false;
        }
        auto image=C2D_SpriteSheetGetImage(sheet,0);size+=image.tex->size;
        C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);next.push_back(sheet);
    }
    C3D_FrameSync();for(auto s:sheets)if(s)C2D_SpriteSheetFree(s);
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
bool loadPopulationAtlases(const Npcs&population,const std::string&root,std::string&error){
    std::vector<C2D_SpriteSheet>next;size_t used=0;
    for(size_t i=0;i<population.styles().size();++i){
        const auto&meta=population.styles()[i];char name[32];std::snprintf(name,sizeof name,"pop_%02u.t3x",unsigned(i));
        auto path=root+name;
        auto sheet=equipmentChecksum(path,meta.checksum)?C2D_SpriteSheetLoad(path.c_str()):nullptr;
        if(!sheet||C2D_SpriteSheetCount(sheet)!=40){
            if(sheet)C2D_SpriteSheetFree(sheet);
            for(auto old:next)C2D_SpriteSheetFree(old);
            error="POBLACION: RECURSOS INVALIDOS";return false;
        }
        auto image=C2D_SpriteSheetGetImage(sheet,0);C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);
        used+=image.tex->size;next.push_back(sheet);
    }
    C3D_FrameSync();for(auto old:npcSheets)C2D_SpriteSheetFree(old);
    npcSheets=std::move(next);npcBytes=used;error.clear();return true;
}
bool prepareEquipment(unsigned mask,const EquipmentArt&art,std::string&error){
    if(mask>7)return false;
    if(auto cached=outfitCache.get(mask)){outfit=cached;outfitMask=mask;error.clear();return true;}
    if(art.dolls.empty()){error="FALTAN RECURSOS DE EQUIPO";return mask==0;}
    char path[128];std::snprintf(path,sizeof path,"sdmc:/3ds/UO-3DS-CoreLab/equip_%02u.t3x",mask);
    if(!equipmentChecksum(path,art.checksums[mask])){error="EQUIPO: ATLAS INVALIDO";return false;}
    // Keep the current outfit intact if a new load fails. Leave at least 2 MiB
    // of linear memory for gameplay; cold misses evict only inactive outfits.
    C3D_FrameSync();
    constexpr size_t required=2*1024*1024,reserve=2*1024*1024;
    while(linearSpaceFree()<required+reserve||(streamed&&outfitCache.bytes()>=8*1024*1024)){
        int victim=outfitCache.victim(outfitMask);if(victim<0)break;
        C2D_SpriteSheetFree(outfitCache.remove(unsigned(victim)));
    }
    if(linearSpaceFree()<required+reserve){error="EQUIPO: MEMORIA RESERVADA";return false;}
    auto next=C2D_SpriteSheetLoad(path);
    if(!next||C2D_SpriteSheetCount(next)!=72){if(next)C2D_SpriteSheetFree(next);error="EQUIPO: SIN MEMORIA O ATLAS INVALIDO";return false;}
    auto image=C2D_SpriteSheetGetImage(next,0);C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);
    outfitCache.insert(mask,next,image.tex->size);
    outfit=next;outfitMask=mask;error.clear();return true;
}
void warmEquipment(unsigned current,const EquipmentArt&art){
    std::string ignored;
    if(!prepareEquipment(current,art,ignored))return;
    const unsigned order[]={current^1u,current^2u,current^4u,current^3u,current^5u,current^6u,current^7u};
    for(unsigned mask:order){
        if(linearSpaceFree()<4*1024*1024||(streamed&&outfitCache.bytes()>=8*1024*1024))break;
        prepareEquipment(mask,art,ignored);
    }
    prepareEquipment(current,art,ignored);
}
static void prepareWorldPages(const std::vector<unsigned char>&needed,int images){
    if(!streamed)return;
    ++textureClock;size_t incoming=0;bool cold=false;
    for(size_t i=0;i<sheets.size();++i)if(needed[i]){sheetUsed[i]=textureClock;if(!sheets[i]){cold=true;incoming+=sheetCosts[i];}}
    if(!cold){textureFailure=false;return;}
    C3D_FrameSync();textureFailure=false;
    while(bytes+incoming>WorldTextureBudget){
        int victim=-1;for(size_t i=0;i<sheets.size();++i)if(sheets[i]&&!needed[i]&&(victim<0||sheetUsed[i]<sheetUsed[victim]))victim=int(i);
        if(victim<0)break;
        auto image=C2D_SpriteSheetGetImage(sheets[victim],0);bytes-=image.tex->size;
        C2D_SpriteSheetFree(sheets[victim]);sheets[victim]=nullptr;
    }
    for(size_t i=0;i<sheets.size();++i)if(needed[i]&&!sheets[i]){
        while(linearSpaceFree()<sheetCosts[i]+2*1024*1024){
            int victim=-1;for(size_t j=0;j<sheets.size();++j)if(sheets[j]&&!needed[j]&&(victim<0||sheetUsed[j]<sheetUsed[victim]))victim=int(j);
            if(victim>=0){auto image=C2D_SpriteSheetGetImage(sheets[victim],0);bytes-=image.tex->size;C2D_SpriteSheetFree(sheets[victim]);sheets[victim]=nullptr;continue;}
            victim=outfitCache.victim(outfitMask);if(victim<0)break;
            C2D_SpriteSheetFree(outfitCache.remove(unsigned(victim)));
        }
        if(linearSpaceFree()<sheetCosts[i]+2*1024*1024){textureFailure=true;continue;}
        char name[32];std::snprintf(name,sizeof name,"atlas_%02u.t3x",unsigned(i));
        auto sheet=C2D_SpriteSheetLoad((atlasRoot+name).c_str());
        if(!sheet||C2D_SpriteSheetCount(sheet)<size_t(std::min(32,images-int(i)*32))){if(sheet)C2D_SpriteSheetFree(sheet);textureFailure=true;continue;}
        auto image=C2D_SpriteSheetGetImage(sheet,0);C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);
        sheets[i]=sheet;bytes+=image.tex->size;
    }
}
bool rendererTextureError(){return textureFailure;}
size_t textureBytes(){return bytes+npcBytes+outfitCache.bytes();}
size_t drawnObjects(){return objects;}
bool rendererOverflow(){return overflow;}
bool hasRunAnimation(){return runVerified;}
unsigned renderListBuilds(){return renderCache.rebuilds;}
void invalidateRenderList(){renderCache.invalidate();}
void drawWorld(C3D_RenderTarget*target,const World&w,const Actor&actor,float cameraX,float cameraY,float footZ,
               RoofMode roofs,int direction,int frame,bool authentic,bool moving,bool running,const Npcs*npcs){
    used=0;objects=0;overflow=false;batches.clear();
    auto project=[&](float x,float y,float z)->Point{return {200+(x-cameraX-y+cameraY)*22,126+(x-cameraX+y-cameraY)*22-(z-footZ)*4};};
    const auto&list=renderCache.get(w,actor,roofs!=RoofMode::Full);
    roofCutaway.update(w,actor,roofs);
    bool afterActor=false;
    auto npcList=npcs?npcs->drawList(w,actor,roofs!=RoofMode::Full):std::vector<Draw>{};
    mergedDraws.clear();mergedDraws.reserve(list.size()+npcList.size());
    for(size_t wi=0,ni=0;wi<list.size()||ni<npcList.size();){
        bool takeNpc=ni<npcList.size()&&(wi==list.size()||drawBefore(npcList[ni],list[wi]));
        mergedDraws.push_back(takeNpc?npcList[ni++]:list[wi++]);
    }
    auto visible=[&](const Draw&d){
        if(d.kind==Kind::Static&&(roofCutaway.hidden(w,d.index,actor,roofs)||(roofs==RoofMode::Hidden&&w.objects[d.index].z>=footZ+14)))return false;
        return !streamed||worldVisible(w,d,cameraX,cameraY,footZ,imageSizes);
    };
    if(authentic&&streamed){
        std::vector<unsigned char>needed(sheets.size());
        for(const auto&d:mergedDraws)if(visible(d)){
            unsigned id=d.kind==Kind::Land?w.land[d.index].image:d.kind==Kind::Static?w.objects[d.index].image:NoImage;
            if(id!=NoImage&&id/32<needed.size())needed[id/32]=1;
        }
        prepareWorldPages(needed,w.images);
    }
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_RenderTargetClear(target,C3D_CLEAR_ALL,0x14212fff,0xffffffff);
    C3D_FrameDrawOn(target);
    for(const auto&d:mergedDraws){
        if(!visible(d))continue;
        pixelMode=d.kind==Kind::Actor?PixelMode::Character:
            afterActor&&supportingFragment(w,d,actor,footZ)?PixelMode::Support:PixelMode::World;
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
                if(alpha<1)pixelMode=PixelMode::Translucent;
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
            float left=foot.x-width/2,top=foot.y-height+5;
            if(npcs->hasPopulation()){const auto&style=npcs->styles()[n.style];left=foot.x-style.pivotX;top=foot.y-style.pivotY;}
            Point p[]={{left,top},{left+width,top},{left+width,top+height},{left,top+height}};
            Point uv[4];uvFor(image,uv);quad(p,uv,image.tex,1,1,1,1);
        }else{
            afterActor=true;
            Point foot=project(cameraX,cameraY,footZ);
            auto player=authentic&&outfit?outfit:avatar;
            if(player){
                unsigned index=avatarImage(direction,moving,running&&(authentic&&outfit?true:runVerified),frame,C2D_SpriteSheetCount(player));
                auto image=C2D_SpriteSheetGetImage(player,index);
                float width=image.subtex->width,height=image.subtex->height;
                Point p[]={{foot.x-width/2,foot.y-height+5},{foot.x+width/2,foot.y-height+5},{foot.x+width/2,foot.y+5},{foot.x-width/2,foot.y+5}};
                Point uv[4];uvFor(image,uv);quad(p,uv,image.tex,1,1,1,1);
            }else{rect(foot.x-5,foot.y-24,10,24,.2f,.45f,.95f);rect(foot.x-4,foot.y-32,8,8,.95f,.75f,.55f);}
        }
    }
    // Overhead text is a final color-only overlay; it cannot alter floor depth.
    pixelMode=PixelMode::Translucent;
    for(const auto&d:npcList){const auto&n=npcs->all()[d.index];if(!n.namePresence.visible())continue;float t=n.progress;
        auto foot=project(n.from.x+(n.position.x-n.from.x)*t,n.from.y+(n.position.y-n.from.y)*t,n.from.z+(n.position.z-n.from.z)*t);
        float pivot=npcs->hasPopulation()?npcs->styles()[n.style].pivotY:64;
        float head=foot.y-pivot;if(foot.x<0||foot.x>400||head< -8||head>240)continue;
        size_t characters=std::min(size_t(30),n.name.size())+std::min(size_t(30),n.title.size());
        if(used+int(characters*18)>=Capacity)break;
        auto line=[&](const std::string&text,float y){float width=overheadFont.width(text);
            float x=std::max(2.f,std::min(398-width,foot.x-width/2));y=std::max(2.f,y);
            labelText(x+1,y+2,text,false,0,0,0);labelText(x,y,text,true,1,1,1);labelText(x,y,text,false,n.nameColor[0]/255.f,n.nameColor[1]/255.f,n.nameColor[2]/255.f);
        };
        float top=std::max(2.f,head-(n.title.empty()?17:33));
        line(n.name,top);if(!n.title.empty())line(n.title,top+16);
    }
    C3D_BindProgram(&program);C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,uniformProjection,&projection);
    auto attrs=C3D_GetAttrInfo();AttrInfo_Init(attrs);AttrInfo_AddLoader(attrs,0,GPU_FLOAT,3);AttrInfo_AddLoader(attrs,1,GPU_FLOAT,2);AttrInfo_AddLoader(attrs,2,GPU_FLOAT,4);
    auto buffers=C3D_GetBufInfo();BufInfo_Init(buffers);BufInfo_Add(buffers,buffer,sizeof(Vertex),3,0x210);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true,-1.f,0.f);C3D_EarlyDepthTest(false,GPU_EARLYDEPTH_GEQUAL,0);
    C3D_AlphaTest(true,GPU_GREATER,0);C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
    auto env=C3D_GetTexEnv(0);C3D_TexEnvInit(env);C3D_TexEnvSrc(env,C3D_Both,GPU_TEXTURE0,GPU_PRIMARY_COLOR,GPU_PRIMARY_COLOR);C3D_TexEnvFunc(env,C3D_Both,GPU_MODULATE);
    GSPGPU_FlushDataCache(buffer,used*sizeof(Vertex));
    DepthStateCache depthState;
    for(const auto&batch:batches){
        // Alpha rejection precedes depth writes, so transparent sprite texels
        // never protect an empty rectangle. Foreground walls/items/NPC reset
        // the marker at their own opaque pixels, retaining normal occlusion.
        const bool support=batch.mode==PixelMode::Support,write=batch.mode!=PixelMode::Translucent;
        if(depthState.change(support,write))
            C3D_DepthTest(true,support?GPU_LEQUAL:GPU_ALWAYS,write?GPU_WRITE_ALL:GPU_WRITE_COLOR);
        C3D_TexBind(0,batch.texture);C3D_DrawArrays(GPU_TRIANGLES,batch.first,batch.count);
    }
    C3D_FrameEnd(0);
}
}
