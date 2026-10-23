// SPDX-License-Identifier: MIT
#include "startup.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
using namespace lab;
namespace {
int fail(const char*,const char*){return -1;}
std::vector<char>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void ppm(const std::string&p,const std::vector<uint8_t>&b,int width){std::ofstream f(p,std::ios::binary);f<<"P6\n"<<width<<" 240\n255\n";for(int y=0;y<240;++y)for(int x=0;x<width;++x){auto q=b.data()+16+3*(x*240+239-y);char rgb[]={char(q[2]),char(q[1]),char(q[0])};f.write(rgb,3);}}
}
int main(int argc,char**argv){
    assert(argc==2);std::string root=std::string(argv[1])+"/";
    GameOptions options;assert(!options.load(root+"options.sav")&&options.zoom==2);
    std::string error;options.roofs=2;options.zoom=4;options.page=2;assert(options.save(root+"options.sav",error));
    GameOptions loaded;assert(loaded.load(root+"options.sav")&&loaded.roofs==2&&loaded.zoom==4&&loaded.page==2);
    options.zoom=1;assert(!options.save(root+"options.sav",error,fail));assert(loaded.load(root+"options.sav")&&loaded.zoom==4);
    StartMenu menu;assert(menu.activate(false)==MenuAction::None&&menu.page==MenuPage::Confirm);assert(menu.activate(false)==MenuAction::None&&menu.page==MenuPage::Main);
    menu.selected=1;assert(menu.activate(false)==MenuAction::None);assert(menu.activate(true)==MenuAction::Continue);
    menu.selected=0;assert(menu.activate(true)==MenuAction::None);menu.move(1);assert(menu.activate(true)==MenuAction::NewGame);
    menu.back();menu.selected=2;menu.activate(true);assert(menu.page==MenuPage::Options);
    menu.selected=0;menu.activate(true);assert(menu.options.roofs==1);
    menu.selected=1;menu.activate(true);assert(menu.options.zoom==3);
    menu.selected=2;menu.activate(true);assert(menu.options.page==1);
    assert(menu.hit(100,60)==0&&menu.hit(100,90)==1&&menu.hit(5,60)==-1&&menu.hit(100,200)==-1);
    menu.back();menu.move(-1);assert(menu.selected==3);assert(menu.activate(true)==MenuAction::Exit);menu.move(1);assert(menu.selected==0);
    World w=demoWorld();Object chest;chest.art=0xE42;chest.x=5;chest.y=5;w.objects={chest};w.index();
    Inventory old;old.init(w,root+"inventory-private.sav");old.backpack={{5,1},{0,30}};old.equipment={{-1,6,7,-1,-1,-1,-1,-1,-1,-1,-1}};old.chests[0].items.clear();assert(old.checkpoint());
    Actor location{6,5,0};assert(savePosition(root+"position-private.sav",w,location,error));auto bytes=read(root+"inventory-private.sav");
    Campaign campaign(root);assert(campaign.load()&&campaign.generation==0&&campaign.prefix().empty());
    uint32_t next=campaign.unusedGeneration();assert(next==1);auto prefix=campaign.prefix(next);
    Inventory fresh;fresh.init(w,root+prefix+"inventory-private.sav");assert(fresh.backpack.empty()&&fresh.equipmentMask()==0&&fresh.chests[0].items.size()==7);assert(fresh.checkpoint());
    assert(savePosition(root+prefix+"position-private.sav",w,{w.startX,w.startY,w.startZ},error));
    assert(!campaign.commit(next,error,fail)&&campaign.generation==0);
    Campaign unchanged(root);assert(unchanged.load()&&unchanged.generation==0);
    Inventory continued;continued.init(w,root+unchanged.prefix()+"inventory-private.sav");assert(continued.restored&&continued.equipmentMask()==6&&continued.backpack.size()==2&&continued.chests[0].items.empty());
    assert(read(root+"inventory-private.sav")==bytes&&campaign.unusedGeneration()==2);
    assert(campaign.commit(next,error));Campaign selected(root);assert(selected.load()&&selected.generation==next);
    Inventory resumed;resumed.init(w,root+selected.prefix()+"inventory-private.sav");assert(resumed.restored&&resumed.backpack.empty()&&resumed.equipmentMask()==0&&resumed.chests[0].items.size()==7);
    assert(read(root+"inventory-private.sav")==bytes);Actor spawn;assert(loadPosition(root+selected.prefix()+"position-private.sav",w,spawn,error)&&spawn.x==w.startX&&spawn.y==w.startY);
    assert(!campaign.commit(2,error,fail));Campaign kept(root);assert(kept.load()&&kept.generation==1);
    {std::ofstream f(root+"campaign.sav",std::ios::binary);f<<"broken";}Campaign broken(root);assert(!broken.load()&&!broken.valid);
    std::vector<uint8_t>top(400*240*3+32,0xA5),bottom(320*240*3+32,0xA5);
    drawTitle(top.data()+16);ppm(root+"title.ppm",top,400);
    for(auto page:{MenuPage::Main,MenuPage::Options,MenuPage::Confirm}){menu.page=page;menu.selected=0;drawStartMenu(bottom.data()+16,menu,true);}
    for(int stage=-1;stage<8;++stage)drawLoading(bottom.data()+16,"CARGANDO BRITANNIA",stage);
    menu.back();drawStartMenu(bottom.data()+16,menu,true);ppm(root+"menu.ppm",bottom,320);
    for(int i=0;i<16;++i)assert(top[i]==0xA5&&top[top.size()-1-i]==0xA5&&bottom[i]==0xA5&&bottom[bottom.size()-1-i]==0xA5);
    PauseMenu pause;pause.open();std::vector<uint8_t>paused(320*240*3+32,0xCD);
    for(int row=0;row<5;++row){pause.selected=row;drawPause(paused.data()+16,pause);}
    ppm(root+"pause.ppm",paused,320);
    for(int i=0;i<16;++i)assert(paused[i]==0xCD&&paused[paused.size()-1-i]==0xCD);
    std::cout<<"Startup: navigation, touch, cancellation, options, exact continue, safe new game, SD failure, legacy saves and framebuffer bounds passed\n";
}
