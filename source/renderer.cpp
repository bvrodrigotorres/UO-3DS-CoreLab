// SPDX-License-Identifier: MIT
#include "renderer.hpp"
#include "avatar.hpp"
#include "character.hpp"
#include "repairs.hpp"
#include "character_depth.hpp"
#include "performance.hpp"
#include "mount.hpp"
#include "npc_pose_cache.hpp"
#include "view.hpp"
#include "save_queue.hpp"
#include <memory>
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
static C3D_Tex white,labelFont,groundIcons;
static bool groundIconsReady=false;static std::array<Tex3DS_SubTexture,ItemTypes>groundFrames;
static OverheadFont overheadFont;
static std::vector<C2D_SpriteSheet> sheets;
static bool streamed=false,textureFailure=false;
static std::string atlasRoot;
static std::vector<std::array<unsigned,2>>imageSizes;
static std::vector<size_t>sheetCosts;
static std::vector<uint64_t>sheetUsed;
static uint64_t textureClock=0;
static std::vector<uint64_t>worldRetry,npcRetry;
static std::vector<bool>npcVerified;
static unsigned textureLoadsRemaining=0;
static constexpr size_t WorldTextureBudget=10*1024*1024;
struct Outfit {
    C2D_SpriteSheet legacy=nullptr;
    C3D_Tex texture{},second{};
    std::array<Tex3DS_SubTexture,128>frames{};
    bool mounted=false;
    bool custom=false;
};
static C2D_SpriteSheet avatar;
static Outfit*outfit=nullptr;
static const CharacterAssets*characterAssets=nullptr;
static Appearance characterAppearance;
static void freeOutfit(Outfit*p){if(!p)return;if(p->custom){C3D_TexDelete(&p->texture);if(p->mounted)C3D_TexDelete(&p->second);}else if(p->legacy)C2D_SpriteSheetFree(p->legacy);delete p;}
static C2D_Image outfitImage(Outfit*p,unsigned n){return p->custom?C2D_Image{p->mounted&&n>=64?&p->second:&p->texture,&p->frames[n]}:C2D_SpriteSheetGetImage(p->legacy,n);}
static size_t outfitBytes(Outfit*p){return p->custom?p->texture.size+(p->mounted?p->second.size:0):C2D_SpriteSheetGetImage(p->legacy,0).tex->size;}
static Outfit*customOutfit(const std::vector<uint16_t>&pixels,bool mounted=false){
    if(pixels.size()!=(mounted?128*128*128:72*96*112))return nullptr;
    auto*p=new Outfit;
    if(!C3D_TexInit(&p->texture,1024,1024,GPU_RGBA5551)){delete p;return nullptr;}
    if(mounted&&!C3D_TexInit(&p->second,1024,1024,GPU_RGBA5551)){C3D_TexDelete(&p->texture);delete p;return nullptr;}
    p->custom=true;p->mounted=mounted;std::memset(p->texture.data,0,p->texture.size);
    if(mounted)std::memset(p->second.data,0,p->second.size);
    auto*data=static_cast<uint16_t*>(p->texture.data);
    const unsigned width=mounted?128:96,height=mounted?128:112,count=mounted?128:72,across=mounted?8:10;
    for(unsigned n=0;n<count;++n){unsigned index=mounted?n%64:n,left=index%across*width,top=index/across*height;
        if(mounted&&n==64)data=static_cast<uint16_t*>(p->second.data);
        p->frames[n]={static_cast<uint16_t>(width),static_cast<uint16_t>(height),left/1024.f,1-top/1024.f,(left+width)/1024.f,1-(top+height)/1024.f};
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)data[characterTexel(left+x,top+y,1024)]=pixels[n*width*height+y*width+x];}
    if(mounted){C3D_TexFlush(&p->second);C3D_TexSetFilter(&p->second,GPU_NEAREST,GPU_NEAREST);}
    C3D_TexFlush(&p->texture);C3D_TexSetFilter(&p->texture,GPU_NEAREST,GPU_NEAREST);return p;
}
static unsigned outfitMask=~0u;
static EquipmentCache<Outfit*>outfitCache;
static void clearOutfits(){while(outfitCache.victim(~0u)>=0)freeOutfit(outfitCache.remove(unsigned(outfitCache.victim(~0u))));}
static std::vector<C2D_SpriteSheet>npcSheets;
static size_t npcBytes;static std::string npcRoot;static std::vector<NpcStyle>npcMeta;static std::vector<uint64_t>npcUsed;static uint64_t npcClock=0;static unsigned npcFrameCount=40;
static std::unique_ptr<AsyncResources>resourceReader;static unsigned resourceWaits=0,resourceUploads=0;
static NpcPoseCache poseCache;
static C3D_Tex poseTextures[2]{};
static bool poseStreaming=false;
static std::vector<uint32_t>poseChecksums;
static std::vector<uint64_t>poseRetry;
static std::array<Tex3DS_SubTexture,NpcPoseCache::Slots>poseFrames{};
static void clearNpcPoses(){if(poseStreaming){C3D_TexDelete(&poseTextures[0]);C3D_TexDelete(&poseTextures[1]);}poseStreaming=false;poseChecksums.clear();poseRetry.clear();poseCache=NpcPoseCache{};}
static bool loadNpcPoses(const Npcs&population,const std::string&root){
    if(!resourceReader)return false;
    std::ifstream f(root+"npc-poses.bin",std::ios::binary|std::ios::ate);
    const auto count=population.styles().size();const unsigned frames=population.frameCount();
    if(!f||f.tellg()!=std::streamoff(8+count*(16+4*frames)))return false;
    std::vector<uint8_t>b(size_t(f.tellg()));f.seekg(0);f.read(reinterpret_cast<char*>(b.data()),b.size());
    std::vector<uint32_t>checksums;
    if(!f||!readNpcPoseManifest(b,population.styles(),frames,checksums))return false;
    if(!C3D_TexInit(&poseTextures[0],1024,1024,GPU_RGBA5551))return false;
    if(!C3D_TexInit(&poseTextures[1],1024,1024,GPU_RGBA5551)){C3D_TexDelete(&poseTextures[0]);return false;}
    for(auto&t:poseTextures){std::memset(t.data,0,t.size);C3D_TexFlush(&t);C3D_TexSetFilter(&t,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&t,GPU_CLAMP_TO_BORDER,GPU_CLAMP_TO_BORDER);}
    poseStreaming=true;poseChecksums=std::move(checksums);poseRetry.assign(count*frames,0);return true;
}
static bool npcImage(int style,unsigned frame,C2D_Image&image,bool fallback=true){
    if(style<0||size_t(style)>=npcSheets.size()||frame>=npcFrameCount)return false;
    if(poseStreaming){int slot=poseCache.find(style*npcFrameCount+frame);if(slot<0&&fallback){slot=poseCache.fallback(style,npcFrameCount,frame>=40);}if(slot<0)return false;poseCache.touch(slot);image={&poseTextures[unsigned(slot)/56],&poseFrames[unsigned(slot)]};return true;}
    if(!npcSheets[style])return false;
    image=C2D_SpriteSheetGetImage(npcSheets[style],frame);return true;
}
static void prepareNpcPoses(const Npcs&npcs,const Combat*combat,const std::vector<Draw>&draws,float cameraX,float cameraY,float footZ){
    struct Need{int key;float distance;};std::vector<Need>needed;std::vector<int>pinned;
    for(const auto&d:draws){int style=-1;unsigned frame=0;float x=d.x,y=d.y,z=d.z;
        if(d.kind==Kind::Npc){const auto&n=npcs.all()[d.index];style=n.style;frame=avatarImage(n.direction,n.moving,false,unsigned(n.animation),40);x=n.from.x+(n.position.x-n.from.x)*n.progress;y=n.from.y+(n.position.y-n.from.y)*n.progress;z=n.from.z+(n.position.z-n.from.z)*n.progress;}
        else if(d.kind==Kind::Corpse&&combat&&npcs.hasDeathFrames()){const auto&c=combat->corpses[d.index];style=c.style;frame=40+(c.direction&7);}
        if(style<0||size_t(style)>=npcMeta.size())continue;
        const auto&m=npcMeta[style];float left=200+(x-cameraX-y+cameraY)*22-m.pivotX,top=126+(x-cameraX+y-cameraY)*22-(z-footZ)*4-m.pivotY;
        if(left>=400||left+m.width<=0||top>=240||top+m.height<=0)continue;
        int key=style*npcFrameCount+frame;pinned.push_back(key);int old=poseCache.fallback(style,npcFrameCount,frame>=40);if(old>=0)pinned.push_back(poseCache.slots[old].key);
        if(poseCache.find(key)>=0)continue;
        if(std::find_if(needed.begin(),needed.end(),[&](const Need&n){return n.key==key;})==needed.end())needed.push_back({key,(x-cameraX)*(x-cameraX)+(y-cameraY)*(y-cameraY)});
    }
    std::stable_sort(needed.begin(),needed.end(),[](const Need&a,const Need&b){return a.distance<b.distance;});
    std::vector<std::string>liveReads;liveReads.reserve(needed.size());
    for(const auto&n:needed){char name[64];std::snprintf(name,sizeof name,"npc-poses/%02u/%02u.bin",unsigned(n.key)/npcFrameCount,unsigned(n.key)%npcFrameCount);liveReads.push_back(npcRoot+name);}
    if(resourceReader)resourceReader->retain(liveReads);
    unsigned uploads=0;bool sync=false;uint64_t now=osGetTime();
    for(const auto&n:needed){if(uploads>=4)break;if(now<poseRetry[n.key]){textureFailure=true;continue;}
        unsigned style=unsigned(n.key)/npcFrameCount,frame=unsigned(n.key)%npcFrameCount;const auto&m=npcMeta[style];char name[64];std::snprintf(name,sizeof name,"npc-poses/%02u/%02u.bin",style,frame);const auto path=npcRoot+name;ResourceRead file;
        if(!resourceReader||!resourceReader->request(path,m.width*m.height*2,poseChecksums[n.key],true)||!resourceReader->take(path,file)){++resourceWaits;continue;}
        if(!file.valid||file.bytes.size()!=size_t(m.width*m.height*2)){textureFailure=true;poseRetry[n.key]=now+3000;continue;}
        int slot=poseCache.victim(pinned);if(slot<0){++resourceWaits;continue;}
        if(!sync){C3D_FrameSync();sync=true;}
        unsigned bank=unsigned(slot)/56,cell=unsigned(slot)%56,left=cell%8*128,top=cell/8*144;
        auto*pixels=static_cast<uint16_t*>(poseTextures[bank].data);
        for(int y=0;y<m.height;++y)for(int x=0;x<m.width;++x){unsigned i=unsigned((y*m.width+x)*2);pixels[characterTexel(left+x,top+y,1024)]=uint16_t(file.bytes[i])|uint16_t(file.bytes[i+1])<<8;}
        poseFrames[slot]={uint16_t(m.width),uint16_t(m.height),left/1024.f,1-top/1024.f,(left+m.width)/1024.f,1-(top+m.height)/1024.f};
        // Flush the modified tile rows, not both entire 2 MiB atlases.
        const unsigned first=characterTexel(left,top,1024),last=characterTexel(left+127,top+((m.height+7)/8)*8-1,1024);
        GSPGPU_FlushDataCache(pixels+first,(last-first+1)*sizeof(uint16_t));
        poseCache.assign(slot,n.key);pinned.push_back(n.key);++uploads;++resourceUploads;
    }
}
static C2D_SpriteSheet bloodSheet=nullptr;static size_t bloodBytes=0;
static C2D_SpriteSheet corpseSheet=nullptr;static std::vector<uint8_t>corpseTypes;static std::array<std::array<int,2>,5>corpsePivots{};static size_t corpseBytes=0;
static bool runVerified=false;
static size_t bytes,objects;
static int used;
static bool overflow;
static std::vector<Batch> batches;
static RenderCache renderCache;
static RoofCutaway roofCutaway;
static std::vector<Draw>mergedDraws,visibleDraws;
static std::vector<unsigned char>neededWorld;
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
        const bool mounted=outfit&&outfit->mounted;
        if(right<=(mounted?136:152)||left>=(mounted?264:248)||bottom<=18||top>=(mounted?146:131))pixelMode=PixelMode::World;
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
static void labelText(float x,float y,const std::string&text,bool outline,float r,float g,float b,float alpha=1){
    unsigned width=0;
    for(size_t i=0;i<text.size()&&i<30;++i){unsigned char c=text[i];if(c>=128)c='?';unsigned advance=overheadFont.step(c);if(width+advance>360)break;
        if(c!=' '){float u=float((c%16)*16+(outline?256:0))/512,v=1-float(c/16)*16/128;
            Point p[]={{x,y},{x+16,y},{x+16,y+16},{x,y+16}};
            Point uv[]={{u,v},{u+16.f/512,v},{u+16.f/512,v-16.f/128},{u,v-16.f/128}};
            quad(p,uv,&labelFont,r,g,b,alpha);
        }
        width+=advance;x+=advance;
    }
}

bool loadGroundIcons(const ItemIcons&icons){
    if(icons.rgba.size()!=ItemTypes*24*24*4)return false;
    C3D_FrameSync();if(groundIconsReady){C3D_TexDelete(&groundIcons);groundIconsReady=false;}
    if(!C3D_TexInit(&groundIcons,256,128,GPU_RGBA5551))return false;
    auto*pixels=static_cast<uint16_t*>(groundIcons.data);std::memset(pixels,0,groundIcons.size);
    for(unsigned n=0;n<ItemTypes;++n){unsigned left=n%10*24,top=n/10*24;
        groundFrames[n]={24,24,left/256.f,1-top/128.f,(left+24)/256.f,1-(top+24)/128.f};
        for(unsigned y=0;y<24;++y)for(unsigned x=0;x<24;++x){size_t p=(n*576+y*24+x)*4;unsigned c=icons.rgba[p+3]>127?((icons.rgba[p]>>3)<<11)|((icons.rgba[p+1]>>3)<<6)|((icons.rgba[p+2]>>3)<<1)|1:0;pixels[characterTexel(left+x,top+y,256)]=c;}}
    C3D_TexFlush(&groundIcons);C3D_TexSetFilter(&groundIcons,GPU_NEAREST,GPU_NEAREST);groundIconsReady=true;return true;
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
    resourceReader.reset(new AsyncResources);if(!resourceReader->start())resourceReader.reset();
    batches.reserve(512);return true;
}
void rendererShutdown(){
    resourceReader.reset();
    C3D_FrameSync();clearNpcPoses();for(auto s:sheets)if(s)C2D_SpriteSheetFree(s);sheets.clear();
    if(avatar)C2D_SpriteSheetFree(avatar);
    clearOutfits();
    for(auto s:npcSheets)if(s)C2D_SpriteSheetFree(s);
    npcSheets.clear();if(corpseSheet)C2D_SpriteSheetFree(corpseSheet);corpseSheet=nullptr;corpseBytes=0;
    if(bloodSheet){C2D_SpriteSheetFree(bloodSheet);}bloodSheet=nullptr;bloodBytes=0;
    if(groundIconsReady){C3D_TexDelete(&groundIcons);groundIconsReady=false;}
    C3D_TexDelete(&labelFont);C3D_TexDelete(&white);if(buffer)linearFree(buffer);
    shaderProgramFree(&program);if(shader)DVLB_Free(shader);
}
bool loadAtlases(int images,std::string&error,const std::string&root){
    streamed=images>512;atlasRoot=root;textureFailure=false;worldRetry.assign((images+31)/32,0);
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
bool loadBloodArt(const std::string&root,std::string&error){
    std::ifstream f(root+"blood.bin",std::ios::binary);uint8_t b[16];
    if(!f.read((char*)b,16)||f.peek()!=EOF||std::memcmp(b,"UBL1",4)){error="FALTA BLOOD.BIN";return false;}
    auto get=[&](int at){return uint32_t(b[at])|uint32_t(b[at+1])<<8|uint32_t(b[at+2])<<16|uint32_t(b[at+3])<<24;};
    if(get(4)!=7||get(8)>128*1024||!equipmentChecksum(root+"blood.t3x",get(12))){error="SANGRE: RECURSOS INVALIDOS";return false;}
    auto sheet=C2D_SpriteSheetLoad((root+"blood.t3x").c_str());
    if(!sheet||C2D_SpriteSheetCount(sheet)!=7){if(sheet)C2D_SpriteSheetFree(sheet);error="SANGRE: ATLAS INVALIDO";return false;}
    auto image=C2D_SpriteSheetGetImage(sheet,0);if(image.tex->size>128*1024){C2D_SpriteSheetFree(sheet);error="SANGRE: ATLAS EXCESIVO";return false;}
    C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);if(bloodSheet)C2D_SpriteSheetFree(bloodSheet);bloodSheet=sheet;bloodBytes=image.tex->size;error.clear();return true;
}
bool loadCombatArt(const std::string&root,const std::string&population,std::string&error){
    std::ifstream f(root+"corpses.bin",std::ios::binary|std::ios::ate);
    if(!f||f.tellg()<44||f.tellg()>108){error="FALTA CORPSES.BIN";return false;}
    std::vector<uint8_t>b(size_t(f.tellg()));f.seekg(0);f.read((char*)b.data(),b.size());
    auto get=[&](size_t i){return uint32_t(b[i])|uint32_t(b[i+1])<<8|uint32_t(b[i+2])<<16|uint32_t(b[i+3])<<24;};
    unsigned count=b[4]|unsigned(b[5])<<8;uint32_t hash=2166136261;for(size_t i=24;i<b.size();++i)hash=(hash^b[i])*16777619;
    if(std::memcmp(b.data(),"UCR1",4)||!count||count>64||b[6]!=5||b[7]||b.size()!=44+count||get(20)!=hash||get(12)>512*1024||!equipmentChecksum(population,get(8))||!equipmentChecksum(root+"corpses.t3x",get(16))){error="CADAVERES: ARCHIVO INVALIDO";return false;}
    for(size_t i=44;i<b.size();++i)if(b[i]!=255&&b[i]>=5){error="CADAVERES: TIPO INVALIDO";return false;}
    auto sheet=C2D_SpriteSheetLoad((root+"corpses.t3x").c_str());if(!sheet||C2D_SpriteSheetCount(sheet)!=40){if(sheet)C2D_SpriteSheetFree(sheet);error="CADAVERES: ATLAS INVALIDO";return false;}
    if(corpseSheet)C2D_SpriteSheetFree(corpseSheet);
    corpseSheet=sheet;corpseTypes.assign(b.begin()+44,b.end());
    for(int i=0;i<5;++i)for(int xy=0;xy<2;++xy){int at=24+i*4+xy*2;corpsePivots[i][xy]=int16_t(unsigned(b[at])|unsigned(b[at+1])<<8);}
    corpseBytes=C2D_SpriteSheetGetImage(sheet,0).tex->size;error.clear();return true;
}
bool loadNpcAtlases(std::string&error){
    std::vector<C2D_SpriteSheet>next;size_t usedBytes=0;
    for(int i=0;i<3;++i){
        char path[128];std::snprintf(path,sizeof(path),"sdmc:/3ds/UO-3DS-CoreLab/npc_%02d.t3x",i);
        auto s=C2D_SpriteSheetLoad(path);
        if(!s||C2D_SpriteSheetCount(s)!=40){if(s)C2D_SpriteSheetFree(s);for(auto t:next)C2D_SpriteSheetFree(t);error="Missing/invalid NPC atlas";return false;}
        auto image=C2D_SpriteSheetGetImage(s,0);C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);usedBytes+=image.tex->size;next.push_back(s);
    }
    for(auto s:npcSheets)if(s)C2D_SpriteSheetFree(s);
    npcSheets=std::move(next);npcBytes=usedBytes;error.clear();return true;
}
bool loadPopulationAtlases(const Npcs&population,const std::string&root,std::string&error){
    for(size_t i=0;i<population.styles().size();++i){char name[32];std::snprintf(name,sizeof name,"pop_%02u.t3x",unsigned(i));std::ifstream f(root+name,std::ios::binary|std::ios::ate);
        if(!f||f.tellg()!=std::streamoff(population.styles()[i].bytes)){error="POBLACION: RECURSOS INVALIDOS";return false;}}
    C3D_FrameSync();for(auto old:npcSheets)if(old)C2D_SpriteSheetFree(old);
    clearNpcPoses();
    npcRoot=root;npcMeta=population.styles();npcFrameCount=population.frameCount();npcSheets.assign(npcMeta.size(),nullptr);npcUsed.assign(npcMeta.size(),0);npcRetry.assign(npcMeta.size(),0);npcVerified.assign(npcMeta.size(),false);npcBytes=0;loadNpcPoses(population,root);error.clear();return true;
}
static void prepareNpcPages(const Npcs&npcs,const Combat*combat,const std::vector<Draw>&draws,float cameraX,float cameraY,float footZ,bool load=true){
    if(npcMeta.empty()||poseStreaming)return;
    std::vector<bool>needed(npcSheets.size());
    for(const auto&d:draws){int style=-1;if(d.kind==Kind::Npc)style=npcs.all()[d.index].style;else if(d.kind==Kind::Corpse&&combat&&npcs.hasDeathFrames())style=combat->corpses[d.index].style;
        if(style>=0&&size_t(style)<needed.size()){
            float x=d.x,y=d.y,z=d.z;if(d.kind==Kind::Npc){const auto&n=npcs.all()[d.index];x=n.from.x+(n.position.x-n.from.x)*n.progress;y=n.from.y+(n.position.y-n.from.y)*n.progress;z=n.from.z+(n.position.z-n.from.z)*n.progress;}
            const auto&meta=npcMeta[style];float left=200+(x-cameraX-y+cameraY)*22-meta.pivotX,top=126+(x-cameraX+y-cameraY)*22-(z-footZ)*4-meta.pivotY;
            if(left<400&&left+meta.width>0&&top<240&&top+meta.height>0)needed[style]=true;
        }}
    if(!load){bool sync=false;
        while(npcBytes>6*1024*1024||linearSpaceFree()<4*1024*1024){int victim=-1;for(size_t j=0;j<npcSheets.size();++j)if(npcSheets[j]&&!needed[j]&&(victim<0||npcUsed[j]<npcUsed[victim]))victim=int(j);if(victim<0)break;
            if(!sync){C3D_FrameSync();sync=true;}auto image=C2D_SpriteSheetGetImage(npcSheets[victim],0);npcBytes-=image.tex->size;C2D_SpriteSheetFree(npcSheets[victim]);npcSheets[victim]=nullptr;}
        return;
    }
    const uint64_t now=osGetTime();
    for(size_t i=0;i<needed.size();++i)if(needed[i]){npcUsed[i]=++npcClock;if(npcSheets[i])continue;
        if(!textureLoadsRemaining){++resourceWaits;continue;}if(now<npcRetry[i]){textureFailure=true;continue;}
        size_t estimate=1;while(estimate<=npcMeta[i].bytes/2)estimate*=2; // Uncompressed T3X pixels plus a small header.
        char name[32];std::snprintf(name,sizeof name,"pop_%02u.t3x",unsigned(i));auto path=npcRoot+name;
        ResourceRead file;
        if(resourceReader){resourceReader->request(path,npcMeta[i].bytes,npcMeta[i].checksum);if(!resourceReader->take(path,file)){++resourceWaits;continue;}}
        C3D_FrameSync();
        while(npcBytes+estimate>6*1024*1024||linearSpaceFree()<estimate+2*1024*1024){
            int victim=-1;for(size_t j=0;j<npcSheets.size();++j)if(npcSheets[j]&&!needed[j]&&(victim<0||npcUsed[j]<npcUsed[victim]))victim=int(j);
            if(victim<0)break;
            auto image=C2D_SpriteSheetGetImage(npcSheets[victim],0);npcBytes-=image.tex->size;C2D_SpriteSheetFree(npcSheets[victim]);npcSheets[victim]=nullptr;
        }
        while(linearSpaceFree()<estimate+2*1024*1024){int victim=outfitCache.victim(outfitMask,outfitMask^4096u);if(victim<0)break;freeOutfit(outfitCache.remove(unsigned(victim)));}
        if(linearSpaceFree()<estimate+2*1024*1024){textureFailure=true;npcRetry[i]=now+1000;continue;}
        --textureLoadsRemaining;
        if(!resourceReader&&!npcVerified[i])npcVerified[i]=equipmentChecksum(path,npcMeta[i].checksum);
        auto sheet=resourceReader?(file.valid?C2D_SpriteSheetLoadFromMem(file.bytes.data(),file.bytes.size()):nullptr):(npcVerified[i]?C2D_SpriteSheetLoad(path.c_str()):nullptr);
        if(!sheet||C2D_SpriteSheetCount(sheet)!=npcFrameCount){if(sheet)C2D_SpriteSheetFree(sheet);textureFailure=true;npcRetry[i]=now+1000;continue;}
        npcRetry[i]=0;
        auto image=C2D_SpriteSheetGetImage(sheet,0);C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);npcBytes+=image.tex->size;npcSheets[i]=sheet;++resourceUploads;break;
    }
}
bool selectEquipment(unsigned key){if(key&MountedBank)key&=~4096u;if(auto cached=outfitCache.get(key)){outfit=cached;outfitMask=key;return true;}return false;}
bool prepareEquipment(unsigned key,const EquipmentArt&art,std::string&error){
    bool mounted=key&MountedBank;if(mounted)key&=~4096u;
    bool combatBank=(key&4096)!=0;unsigned mask=key&EquipmentAppearanceMask;
    if((key&~(EquipmentAppearanceMask|4096u|MountedBank))||(!characterAppearance.enabled&&mask>7))return false;
    std::vector<uint8_t>portrait;
    if(characterAppearance.enabled&&characterAssets&&art.currentKey!=mask&&!characterAssets->portrait(characterAppearance,mask,portrait))return false;
    if(auto cached=outfitCache.get(key)){outfit=cached;outfitMask=key;if(!portrait.empty()){art.currentDoll=std::move(portrait);art.currentKey=mask;}error.clear();return true;}
    if(art.dolls.empty()&&art.currentDoll.empty()){error="FALTAN RECURSOS DE EQUIPO";return mask==0;}
    std::vector<uint16_t>customPixels;
    bool custom=characterAppearance.enabled&&characterAssets;
    if(custom&&!characterAssets->compose(characterAppearance,mask|(mounted?MountedBank:0),customPixels,false,-1,combatBank)){error="PERSONAJE: CAPAS INCOMPLETAS";return false;}
    char path[128];std::snprintf(path,sizeof path,"sdmc:/3ds/UO-3DS-CoreLab/equip_%02u.t3x",mask);
    if(combatBank&&!custom)return false;
    if(!custom&&!equipmentChecksum(path,art.checksums[mask])){error="EQUIPO: ATLAS INVALIDO";return false;}
    // Keep the current outfit intact if a new load fails. Leave at least 2 MiB
    // of linear memory for gameplay; cold misses evict only inactive outfits.
    C3D_FrameSync();
    const size_t required=(mounted?4:2)*1024*1024,reserve=2*1024*1024;
    while(outfitCache.full()||linearSpaceFree()<required+reserve||(streamed&&outfitCache.bytes()+required>8*1024*1024)){
        int victim=outfitCache.victim(outfitMask,key^4096u);if(victim<0)break;
        freeOutfit(outfitCache.remove(unsigned(victim)));
    }
    // A mode change may need four MiB; reclaim only scenery outside the current view.
    while(linearSpaceFree()<required+reserve){int victim=-1;for(size_t i=0;i<sheets.size();++i)if(sheets[i]&&(i>=neededWorld.size()||!neededWorld[i])&&(victim<0||sheetUsed[i]<sheetUsed[victim]))victim=int(i);if(victim<0)break;
        auto image=C2D_SpriteSheetGetImage(sheets[victim],0);bytes-=image.tex->size;C2D_SpriteSheetFree(sheets[victim]);sheets[victim]=nullptr;
    }
    if(linearSpaceFree()<required+reserve){error="EQUIPO: MEMORIA RESERVADA";return false;}
    Outfit*next=nullptr;
    if(custom)next=customOutfit(customPixels,mounted);
    else {auto sheet=C2D_SpriteSheetLoad(path);
        if(sheet&&C2D_SpriteSheetCount(sheet)==72){next=new Outfit;next->legacy=sheet;auto image=outfitImage(next,0);C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);}
        else if(sheet)C2D_SpriteSheetFree(sheet);
    }
    if(!next){error="EQUIPO: SIN MEMORIA O ATLAS INVALIDO";return false;}
    outfitCache.insert(key,next,outfitBytes(next));
    outfit=next;outfitMask=key;if(!portrait.empty()){art.currentDoll=std::move(portrait);art.currentKey=mask;}error.clear();return true;
}
void clearAppearance(){C3D_FrameSync();clearOutfits();outfit=nullptr;outfitMask=99;characterAssets=nullptr;characterAppearance=Appearance{};}
bool applyAppearance(const CharacterAssets&assets,const Appearance&a,EquipmentArt&art,unsigned mask,std::string&error){
    std::vector<uint8_t>dolls;std::vector<uint16_t>pixels;
    if(!assets.portrait(a,mask,dolls)||!assets.compose(a,mask,pixels)){error="FALTAN CAPAS DE PERSONALIZACION";return false;}
    C3D_FrameSync();
    // Editor transitions may discard streamed world pages, never world data.
    if(streamed){for(auto&sheet:sheets)if(sheet){C2D_SpriteSheetFree(sheet);sheet=nullptr;}bytes=0;}
    while(linearSpaceFree()<4*1024*1024){int victim=outfitCache.victim(outfitMask,outfitMask^4096u);if(victim<0)break;freeOutfit(outfitCache.remove(unsigned(victim)));}
    auto*next=customOutfit(pixels);if(!next){error="PERSONALIZACION: SIN MEMORIA";return false;}
    clearOutfits();
    characterAssets=&assets;characterAppearance=a;art.currentDoll=std::move(dolls);art.currentKey=mask;art.playerName=a.name;
    outfitCache.insert(mask,next,outfitBytes(next));outfit=next;outfitMask=mask;error.clear();return true;
}
void warmEquipment(unsigned current,const EquipmentArt&art){
    std::string ignored;
    if(!prepareEquipment(current,art,ignored))return;
    if(!(current&MountedBank)&&characterAppearance.enabled&&linearSpaceFree()>=6*1024*1024)prepareEquipment(current|4096u,art,ignored);
    prepareEquipment(current,art,ignored);
}
static void prepareWorldPages(const std::vector<unsigned char>&needed,int images){
    if(!streamed)return;
    ++textureClock;const uint64_t now=osGetTime();
    for(size_t i=0;i<sheets.size();++i)if(needed[i])sheetUsed[i]=textureClock;
    for(size_t i=0;i<sheets.size();++i)if(needed[i]&&!sheets[i]){
        if(!textureLoadsRemaining){++resourceWaits;continue;}if(now<worldRetry[i]){textureFailure=true;continue;}
        char name[32];std::snprintf(name,sizeof name,"atlas_%02u.t3x",unsigned(i));
        ResourceRead file;auto path=atlasRoot+name;
        if(resourceReader){resourceReader->request(path,sheetCosts[i]);if(!resourceReader->take(path,file)){++resourceWaits;continue;}}
        C3D_FrameSync();
        while(bytes+sheetCosts[i]>WorldTextureBudget||linearSpaceFree()<sheetCosts[i]+2*1024*1024){
            int victim=-1;for(size_t j=0;j<sheets.size();++j)if(sheets[j]&&!needed[j]&&(victim<0||sheetUsed[j]<sheetUsed[victim]))victim=int(j);
            if(victim>=0){auto image=C2D_SpriteSheetGetImage(sheets[victim],0);bytes-=image.tex->size;C2D_SpriteSheetFree(sheets[victim]);sheets[victim]=nullptr;continue;}
            if(linearSpaceFree()>=sheetCosts[i]+2*1024*1024)break;
            victim=outfitCache.victim(outfitMask,outfitMask^4096u);if(victim<0)break;
            freeOutfit(outfitCache.remove(unsigned(victim)));
        }
        if(linearSpaceFree()<sheetCosts[i]+2*1024*1024){textureFailure=true;worldRetry[i]=now+1000;continue;}
        --textureLoadsRemaining;
        auto sheet=resourceReader?(file.valid?C2D_SpriteSheetLoadFromMem(file.bytes.data(),file.bytes.size()):nullptr):C2D_SpriteSheetLoad(path.c_str());
        if(!sheet||C2D_SpriteSheetCount(sheet)<size_t(std::min(32,images-int(i)*32))){if(sheet)C2D_SpriteSheetFree(sheet);textureFailure=true;worldRetry[i]=now+1000;continue;}
        worldRetry[i]=0;
        auto image=C2D_SpriteSheetGetImage(sheet,0);C3D_TexSetFilter(image.tex,GPU_NEAREST,GPU_NEAREST);
        sheets[i]=sheet;bytes+=image.tex->size;++resourceUploads;break;
    }
}
unsigned rendererResourceWaits(){return resourceWaits;}
unsigned rendererUploads(){return resourceUploads;}
bool rendererTextureError(){return textureFailure;}
bool rendererNpcPoseStreaming(){return poseStreaming;}
size_t textureBytes(){return bytes+npcBytes+(poseStreaming?4*1024*1024:0)+corpseBytes+bloodBytes+outfitCache.bytes()+(groundIconsReady?groundIcons.size:0);}
size_t drawnObjects(){return objects;}
bool rendererOverflow(){return overflow;}
bool hasRunAnimation(){return runVerified;}
unsigned renderListBuilds(){return renderCache.rebuilds;}
void invalidateRenderList(){renderCache.invalidate();}
void drawWorld(C3D_RenderTarget*target,const World&w,const Actor&actor,float cameraX,float cameraY,float footZ,
               RoofMode roofs,int direction,int frame,bool authentic,bool moving,bool running,const Npcs*npcs,const Combat*combat,const PlayerStatus*status,const Inventory*inventory){
    used=0;objects=0;overflow=false;batches.clear();resourceWaits=resourceUploads=0;textureLoadsRemaining=1;textureFailure=false;
    auto project=[&](float x,float y,float z)->Point{return {200+(x-cameraX-y+cameraY)*22,126+(x-cameraX+y-cameraY)*22-(z-footZ)*4};};
    const auto&list=renderCache.get(w,actor,roofs!=RoofMode::Full);
    roofCutaway.update(w,actor,roofs);
    bool afterActor=false;
    auto npcList=npcs?npcs->drawList(w,actor,roofs!=RoofMode::Full):std::vector<Draw>{};
    if(combat){for(size_t i=0;i<combat->corpses.size();++i){const auto&c=combat->corpses[i];
        if(c.active&&std::abs(c.position.x-actor.x)<=12&&std::abs(c.position.y-actor.y)<=12&&std::abs(c.position.z-actor.z)<16)npcList.push_back({Kind::Corpse,c.position.x,c.position.y,c.position.z,c.position.z,i,0xfffffe00u+(uint32_t)i});
    }
    for(size_t i=0;i<combat->blood.size();++i){const auto&p=combat->blood[i];if(p.active&&std::abs(p.position.x-actor.x)<=12&&std::abs(p.position.y-actor.y)<=12&&std::abs(p.position.z-actor.z)<16)npcList.push_back({Kind::Blood,p.position.x,p.position.y,p.position.z,p.position.z,i,0xfffffd00u+uint32_t(i)});}
    }
    if(inventory)for(size_t i=0;i<inventory->ground.size();++i){const auto&pile=inventory->ground[i];if(std::abs(pile.position.x-actor.x)>12||std::abs(pile.position.y-actor.y)>12||std::abs(pile.position.z-actor.z)>=16)continue;
        for(size_t j=0;j<pile.items.size();++j)npcList.push_back({Kind::Ground,pile.position.x,pile.position.y,pile.position.z,pile.position.z,i*ChestSlots+j,0xfffffc00u+uint32_t(i*ChestSlots+j)});}
    std::stable_sort(npcList.begin(),npcList.end(),drawBefore);
    mergedDraws.clear();mergedDraws.reserve(list.size()+npcList.size());
    for(size_t wi=0,ni=0;wi<list.size()||ni<npcList.size();){
        bool takeNpc=ni<npcList.size()&&(wi==list.size()||drawBefore(npcList[ni],list[wi]));
        mergedDraws.push_back(takeNpc?npcList[ni++]:list[wi++]);
    }
    auto visible=[&](const Draw&d){
        if(d.kind==Kind::Static&&(roofCutaway.hidden(w,d.index,actor,roofs)||(roofs==RoofMode::Hidden&&w.objects[d.index].z>=footZ+14)))return false;
        return !streamed||worldVisible(w,d,cameraX,cameraY,footZ,imageSizes);
    };
    visibleDraws.clear();visibleDraws.reserve(mergedDraws.size());for(const auto&d:mergedDraws)if(visible(d))visibleDraws.push_back(d);
    if(npcs){prepareNpcPages(*npcs,combat,npcList,cameraX,cameraY,footZ,false);
        if(poseStreaming)prepareNpcPoses(*npcs,combat,npcList,cameraX,cameraY,footZ);
        else if(textureClock%2==0)prepareNpcPages(*npcs,combat,npcList,cameraX,cameraY,footZ);}
    if(authentic&&streamed){
        neededWorld.assign(sheets.size(),0);auto&needed=neededWorld;
        for(const auto&d:visibleDraws){
            unsigned id=d.kind==Kind::Land?w.land[d.index].image:d.kind==Kind::Static?w.objects[d.index].image:NoImage;
            if(id!=NoImage&&id/32<needed.size())needed[id/32]=1;
        }
        prepareWorldPages(needed,w.images);
    }
    if(npcs)prepareNpcPages(*npcs,combat,npcList,cameraX,cameraY,footZ);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_RenderTargetClear(target,C3D_CLEAR_ALL,0x14212fff,0xffffffff);
    C3D_FrameDrawOn(target);
    auto bloodBurst=[&](const Draw&draw){if(!combat)return;const auto saved=pixelMode;pixelMode=PixelMode::Translucent;
        for(const auto&e:combat->damage)if(e.active&&e.kind==CombatText::Damage&&e.age<.38f&&std::abs(e.position.z-actor.z)<16){
            bool victim=e.player?draw.kind==Kind::Actor:draw.kind==Kind::Npc&&int(draw.index)==e.npc;
            if(!victim&&!e.player&&draw.kind==Kind::Corpse&&npcs&&e.npc>=0&&size_t(e.npc)<npcs->all().size()&&!npcs->all()[e.npc].alive)victim=draw.x==e.position.x&&draw.y==e.position.y&&draw.z==e.position.z;
            if(!victim)continue;
            auto p=project(e.position.x,e.position.y,e.position.z);if(p.x< -20||p.x>420||p.y< -20||p.y>280)continue;
            float height=32;
            if(!e.player&&npcs&&e.npc>=0&&size_t(e.npc)<npcs->all().size()&&npcs->hasPopulation()){int style=npcs->all()[e.npc].style;if(style>=0&&size_t(style)<npcs->styles().size())height=std::max(6.f,std::min(36.f,npcs->styles()[style].pivotY*.55f));}
            float t=e.age/.38f;
            for(unsigned i=0;i<7;++i){unsigned seed=e.seed+i*7919;float vx=float(int(seed%17)-8),vy=-float(5+seed%9);float x=p.x+vx*t*2,y=p.y-height+vy*t*2+22*t*t;rect(overheadPixelOrigin(x),overheadPixelOrigin(y),2,i%2?3:2,.7f,.025f,.035f,1-t*.7f);}
        }
        pixelMode=saved;
    };
    for(const auto&d:visibleDraws){
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
        }else if(d.kind==Kind::Ground){
            if(!inventory)continue;
            const auto&pile=inventory->ground[d.index/ChestSlots];size_t slot=d.index%ChestSlots;if(slot>=pile.items.size())continue;
            unsigned type=pile.items[slot].type;if(type>=ItemTypes)continue;
            float x=center.x-12+int(slot%3)*2-2,y=center.y-17+int(slot%2)*2;
            if(groundIconsReady){C2D_Image image{&groundIcons,&groundFrames[type]};Point p[]={{x,y},{x+24,y},{x+24,y+24},{x,y+24}},uv[4];uvFor(image,uv);quad(p,uv,image.tex,1,1,1,1);}
            else rect(x+5,y+12,12,8,.55f,.36f,.13f);
        }else if(d.kind==Kind::Blood){
            const auto&pool=combat->blood[d.index];auto foot=project(pool.position.x,pool.position.y,pool.position.z);pixelMode=PixelMode::Translucent;float alpha=std::min(1.f,(3-pool.age)*2);
            if(bloodSheet){auto image=C2D_SpriteSheetGetImage(bloodSheet,pool.variant%7);float width=image.subtex->width,height=image.subtex->height;float left=foot.x-width/2,top=foot.y-height+5;
                Point p[]={{left,top},{left+width,top},{left+width,top+height},{left,top+height}};Point uv[4];uvFor(image,uv);quad(p,uv,image.tex,1,1,1,alpha);
            }else {rect(foot.x-8,foot.y-2,16,4,.45f,.015f,.02f,alpha);}
        }else if(d.kind==Kind::Corpse){
            const auto&c=combat->corpses[d.index];auto foot=project(c.position.x,c.position.y,c.position.z);
            unsigned type=c.style>=0&&size_t(c.style)<corpseTypes.size()?corpseTypes[c.style]:255;
            C2D_Image corpseImage;
            if(npcs&&npcs->hasDeathFrames()&&npcImage(c.style,40+(c.direction&7),corpseImage,false)){
                auto image=corpseImage;const auto&meta=npcs->styles()[c.style];float left=foot.x-meta.pivotX,top=foot.y-meta.pivotY;
                Point p[]={{left,top},{left+image.subtex->width,top},{left+image.subtex->width,top+image.subtex->height},{left,top+image.subtex->height}};Point uv[4];uvFor(image,uv);quad(p,uv,image.tex,1,1,1,1);
            }else if(corpseSheet&&type<5){auto image=C2D_SpriteSheetGetImage(corpseSheet,type*8+(c.direction&7));float left=foot.x-corpsePivots[type][0],top=foot.y-corpsePivots[type][1];
                Point p[]={{left,top},{left+image.subtex->width,top},{left+image.subtex->width,top+image.subtex->height},{left,top+image.subtex->height}};Point uv[4];uvFor(image,uv);quad(p,uv,image.tex,1,1,1,1);
            }else rect(foot.x-9,foot.y-3,18,5,.35f,.19f,.12f);
        }else if(d.kind==Kind::Npc){
            const auto&n=npcs->all()[d.index];
            C2D_Image image;if(!npcImage(n.style,avatarImage(n.direction,n.moving,false,(unsigned)n.animation,40),image))continue;
            float t=n.progress;
            auto foot=project(n.from.x+(n.position.x-n.from.x)*t,n.from.y+(n.position.y-n.from.y)*t,n.from.z+(n.position.z-n.from.z)*t);
            float width=image.subtex->width,height=image.subtex->height;
            float left=foot.x-width/2,top=foot.y-height+5;
            if(npcs->hasPopulation()){const auto&style=npcs->styles()[n.style];left=foot.x-style.pivotX;top=foot.y-style.pivotY;}
            Point p[]={{left,top},{left+width,top},{left+width,top+height},{left,top+height}};
            Point uv[4];uvFor(image,uv);float tint=combat&&d.index<combat->vitals().size()&&combat->vitals()[d.index].flash>0?.35f:1;quad(p,uv,image.tex,1,tint,tint,1);
        }else{
            afterActor=true;
            Point foot=project(cameraX,cameraY,footZ);
            bool dressed=authentic&&outfit;
            auto player=avatar;
            if(dressed||player){
                bool combatPose=dressed&&(outfitMask&4096)&&!moving;
                unsigned index=avatarImage(direction,combatPose?(combat&&combat->striking()):moving,combatPose?false:running&&(authentic&&outfit?true:runVerified),combatPose?(combat?combat->strikeFrame():0):frame,dressed?72:C2D_SpriteSheetCount(player));
                if(dressed&&outfit->mounted)index=mountedFrame(direction,moving,running,combat&&combat->striking()&&!moving?combat->strikeFrame(5):frame,combat&&combat->striking());
                auto image=dressed?outfitImage(outfit,index):C2D_SpriteSheetGetImage(player,index);
                float width=image.subtex->width,height=image.subtex->height;
                float padding=dressed&&outfit->mounted?20.f:5.f;
                Point p[]={{foot.x-width/2,foot.y-height+padding},{foot.x+width/2,foot.y-height+padding},{foot.x+width/2,foot.y+padding},{foot.x-width/2,foot.y+padding}};
                Point uv[4];uvFor(image,uv);float tint=status&&status->flash>0?.35f:1;quad(p,uv,image.tex,1,tint,tint,1);
            }else{rect(foot.x-5,foot.y-24,10,24,.2f,.45f,.95f);rect(foot.x-4,foot.y-32,8,8,.95f,.75f,.55f);}
        }
        if(d.kind==Kind::Actor||d.kind==Kind::Npc||d.kind==Kind::Corpse)bloodBurst(d);
    }
    // Overhead text is a final color-only overlay; it cannot alter floor depth.
    pixelMode=PixelMode::Translucent;
    for(const auto&d:npcList){if(d.kind!=Kind::Npc)continue;const auto&n=npcs->all()[d.index];if(!n.namePresence.visible())continue;float t=n.progress;
        auto foot=project(n.from.x+(n.position.x-n.from.x)*t,n.from.y+(n.position.y-n.from.y)*t,n.from.z+(n.position.z-n.from.z)*t);
        float pivot=npcs->hasPopulation()?npcs->styles()[n.style].pivotY:64;
        float head=foot.y-pivot;if(foot.x<0||foot.x>400||head< -8||head>240)continue;
        size_t characters=std::min(size_t(30),n.name.size())+std::min(size_t(30),n.title.size());
        if(used+int(characters*18)>=Capacity)break;
        auto line=[&](const std::string&text,float y){float width=overheadFont.width(text);
            float x=overheadPixelOrigin(std::max(2.f,std::min(398-width,foot.x-width/2)));y=overheadPixelOrigin(std::max(2.f,y));
            labelText(x+1,y+1,text,false,0,0,0,.8f);labelText(x,y,text,true,1,1,1,.55f);labelText(x,y,text,false,n.nameColor[0]/255.f,n.nameColor[1]/255.f,n.nameColor[2]/255.f);
        };
        float top=std::max(2.f,head-(n.title.empty()?17:33));
        line(n.name,top);if(!n.title.empty())line(n.title,top+16);
    }
    auto healthBar=[&](float x,float y,int current,int maximum,bool selected){
        x=overheadPixelOrigin(x-23);y=overheadPixelOrigin(y);rect(x,y,46,7,selected?.9f:.1f,selected?.7f:.1f,.1f);
        rect(x+1,y+1,44,5,.2f,.05f,.04f);rect(x+1,y+1,PlayerStatus::barWidth(current,maximum,44),5,.8f,.12f,.08f);
    };
    if(status&&status->showHealth()){auto p=project(cameraX,cameraY,footZ);healthBar(p.x,p.y-(status->mounted?104:82),status->hits,status->hitsMax,false);}
    if(combat&&npcs&&combat->target>=0&&size_t(combat->target)<npcs->all().size()&&size_t(combat->target)<combat->vitals().size()){
        const auto&n=npcs->all()[combat->target];if(n.alive){float t=n.progress;auto p=project(n.from.x+(n.position.x-n.from.x)*t,n.from.y+(n.position.y-n.from.y)*t,n.from.z+(n.position.z-n.from.z)*t);
            if(p.x>=0&&p.x<400&&p.y>=0&&p.y<260){float pivot=npcs->hasPopulation()?npcs->styles()[n.style].pivotY:64;const auto&v=combat->vitals()[combat->target];healthBar(p.x,p.y-pivot-7,v.hits,v.maximum,true);rect(p.x-10,p.y+3,20,2,.95f,.75f,.1f);}
        }
    }
    if(combat)for(const auto&e:combat->damage)if(e.active&&((e.kind==CombatText::Healing&&e.player)||std::abs(e.position.z-actor.z)<16)){
        auto p=e.kind==CombatText::Healing&&e.player?project(cameraX,cameraY,footZ):project(e.position.x,e.position.y,e.position.z);if(p.x<0||p.x>400||p.y< -20||p.y>280)continue;
        float pivot=e.player?(status&&status->mounted?104.f:82.f):64.f;
        if(!e.player&&npcs&&e.npc>=0&&size_t(e.npc)<npcs->all().size()&&npcs->hasPopulation()){int style=npcs->all()[e.npc].style;if(style>=0&&size_t(style)<npcs->styles().size())pivot=npcs->styles()[style].pivotY+14.f;}
        std::string value=combatText(e);const auto color=combatTextColor(e);float x=overheadPixelOrigin(p.x-overheadFont.width(value)/2.f+int(e.seed%9)-4),y=overheadPixelOrigin(p.y-pivot-10-e.age*25),alpha=std::min(1.f,(1.2f-e.age)*4);
        labelText(x+1,y+1,value,false,0,0,0,alpha);labelText(x,y,value,true,0,0,0,alpha);labelText(x,y,value,false,color[0],color[1],color[2],alpha);
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
