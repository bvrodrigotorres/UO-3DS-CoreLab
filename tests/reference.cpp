// SPDX-License-Identifier: MIT
// Test adapters around two unmodified CrossUO functions; never linked to 3DS.
#include "reference.hpp"
#include <algorithm>
#include <cstdlib>
#include <memory>

struct CRenderWorldObject {
    lab::Kind kind=lab::Kind::Land;int z=0,id=0,PriorityZ=0;
    uint64_t flags=0;CRenderWorldObject *m_Next=nullptr,*m_Prev=nullptr,*m_NextXY=nullptr,*m_PrevXY=nullptr;
    virtual ~CRenderWorldObject()=default;
    void RemoveRender(){}
    int GetZ()const{return z;}
    bool IsLandObject()const{return kind==lab::Kind::Land;}
    bool IsStaticGroupObject()const{return kind!=lab::Kind::Land;}
    bool IsGameObject()const{return kind==lab::Kind::Actor;}
    bool IsMultiObject()const{return false;}
    bool IsEffectObject()const{return false;}
    bool IsBackground()const{return flags&lab::Background;}
    bool IsDoor()const{return false;}
};
struct CLandObject:CRenderWorldObject {bool IsStretched=false;int AverageZ=0;};
struct CRenderStaticObject:CRenderWorldObject {int height=0;int GetStaticHeight()const{return height;}};
struct CGameObject:CRenderStaticObject {bool NPC=true;bool IsCorpse()const{return false;}};
struct CMultiObject:CRenderStaticObject {int State=0;};
constexpr int CHMOF_GENERIC_INTERNAL=1;
struct CMapBlock {CRenderWorldObject*Block[1][1]={{nullptr}};void AddRender(CRenderWorldObject*,int,int);};
#include "upstream/add_render.inc"

std::vector<int> referenceOrder(const std::vector<lab::Draw>&input,const std::vector<int>&height,
                              const std::vector<uint64_t>&flags,const std::vector<int>&average,
                              const std::vector<bool>&slope){
    CMapBlock b;std::vector<std::unique_ptr<CRenderWorldObject>> objects;
    for(size_t i=0;i<input.size();++i){
        const auto&d=input[i];CRenderWorldObject*p;
        if(d.kind==lab::Kind::Land){auto land=new CLandObject;land->IsStretched=slope[i];land->AverageZ=average[i];p=land;}
        else if(d.kind==lab::Kind::Actor)p=new CGameObject;
        else{auto s=new CRenderStaticObject;s->height=height[i];p=s;}
        p->kind=d.kind;p->z=d.z;p->id=i;p->flags=flags[i];objects.emplace_back(p);
        b.AddRender(p,0,0);if(!b.Block[0][0])b.Block[0][0]=p;
    }
    auto p=b.Block[0][0];while(p&&p->m_PrevXY)p=p->m_PrevXY;
    std::vector<int> result;while(p){result.push_back(p->id);p=p->m_NextXY;}return result;
}

constexpr int PSS_NORMAL=0,PSS_DEAD_OR_GM=1,PSS_FLYING=2,PSS_ON_SEA_HORSE=3,OL_MOUNT=0;
constexpr unsigned POF_IMPASSABLE_OR_SURFACE=1,POF_SURFACE=2,POF_BRIDGE=4,POF_NO_DIAGONAL=8;
constexpr int DEFAULT_BLOCK_HEIGHT=16;
struct CGameItem {int Graphic=0;};
struct MockPlayer {int Graphic=0;bool IsDead(){return false;}bool IsFlying(){return false;}CGameItem*FindLayer(int){return nullptr;}};
static MockPlayer player;static MockPlayer*g_Player=&player;
struct XY {int X,Y;};struct House {XY StartPos,EndPos;};static House*g_CustomHouseGump=nullptr;
struct SDL_Rect {int x,y,w,h;};struct SDL_Point {int x,y;};
static bool SDL_PointInRect(SDL_Point*,SDL_Rect*){return true;}
struct Config {bool AutoOpenDoors=false;};static Config g_ConfigManager;
struct Game {void OpenDoor(){}};static Game g_Game;
struct CPathObject {
    uint32_t Flags;int Z,AverageZ,Height;CRenderWorldObject*m_Object;
    CPathObject(unsigned f,int z,int average,int height,CRenderWorldObject*p):Flags(f),Z(z),AverageZ(average),Height(height),m_Object(p){}
};
struct CPathFinder {
    int minimum,maximum;std::vector<CPathObject> input;
    void CalculateMinMaxZ(int&a,int&b,int,int,int,int,int){a=minimum;b=maximum;}
    bool CreateItemsList(std::vector<CPathObject>&out,int,int,int){out=input;return !out.empty();}
    bool CalculateNewZ(int,int,char&,int);
};
#include "upstream/calculate_new_z.inc"
bool referenceSolve(const std::vector<lab::Candidate>&input,int z,int minimum,int maximum,int&nextZ){
    CPathFinder p;p.minimum=minimum;p.maximum=maximum;
    for(const auto&v:input)p.input.emplace_back(v.flags,v.z,v.average,v.height,nullptr);
    char out=(char)z;bool ok=p.CalculateNewZ(0,0,out,0);nextZ=(int)(signed char)out;return ok;
}
