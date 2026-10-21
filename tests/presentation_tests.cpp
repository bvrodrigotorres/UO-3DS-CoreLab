// SPDX-License-Identifier: MIT
#include "visibility.hpp"
#include "repairs.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    World repair;repair.width=repair.height=16;repair.originX=1488;repair.originY=1596;
    repair.vertices.assign(17*17,0);repair.land.resize(16*16);
    Object wall;wall.x=13;wall.y=15;wall.z=40;wall.art=308;wall.flags=Wall|Impassable;wall.height=20;
    Object wood;wood.x=12;wood.y=15;wood.z=40;wood.art=1206;wood.image=141;wood.flags=Surface;
    repair.objects={wall,wood};repair.index();
    auto count=repair.objects.size();assert(floorUnderWall(repair,repair.objects[0])==&repair.objects[1]);
    assert(repair.objects.size()==count&&repair.cells[15*16+13].size()==1);
    assert(!floorUnderWall(repair,repair.objects[1]));
    repair.objects[1].image=NoImage;assert(!floorUnderWall(repair,repair.objects[0]));
    repair.objects[1]=wood;wood.x=13;repair.objects.push_back(wood);repair.index();
    assert(!floorUnderWall(repair,repair.objects[0])); // never duplicate an existing floor
    repair.objects.pop_back();repair.index();repair.originX=0;
    assert(!floorUnderWall(repair,repair.objects[0])); // no global hole-filling heuristic
    TouchPages pages;
    pages.begin(260,110);pages.move(50,112);assert(pages.end()&&pages.page==1);
    assert(!pages.end()); // releasing twice must not change the page
    pages.begin(50,110);pages.move(55,200);assert(!pages.end()&&pages.page==1);
    pages.begin(30,150);assert(pages.end()&&pages.zoom==1);
    pages.begin(30,150);assert(!pages.end()&&pages.zoom==1);
    pages.begin(160,150);assert(pages.end()&&pages.zoom==2);
    pages.begin(50,110);pages.move(260,112);assert(pages.end()&&pages.page==0);
    pages.begin(250,220);assert(pages.end()&&pages.page==1);
    Object roof;roof.flags=Roof;roof.z=60;
    ScreenRect covering{100,20,260,140},away{230,20,270,70};
    assert(objectOpacity(roof,18,RoofMode::Auto,true,covering)==.18f);
    assert(objectOpacity(roof,18,RoofMode::Auto,false,covering)==1);
    assert(objectOpacity(roof,18,RoofMode::Auto,true,away)==1);
    assert(objectOpacity(roof,18,RoofMode::Hidden,false,away)==0);
    assert(objectOpacity(roof,18,RoofMode::Full,true,covering)==1);
    roof.flags=Surface; // upper floors can lack the Roof flag in tiledata
    assert(objectOpacity(roof,18,RoofMode::Auto,true,covering)==.18f);
    roof.z=18;
    for(auto mode:{RoofMode::Auto,RoofMode::Hidden,RoofMode::Full})assert(objectOpacity(roof,18,mode,true,covering)==1);
    roof.flags=Wall;roof.z=18;roof.height=20;
    assert(objectOpacity(roof,18,RoofMode::Auto,true,covering)==.18f);
    roof.flags=Wall|Door;
    for(bool front:{false,true}){
        assert(objectOpacity(roof,18,RoofMode::Auto,front,covering)==1);
        assert(objectOpacity(roof,18,RoofMode::Hidden,front,covering)==1);
    }
    roof.z=40;assert(objectOpacity(roof,18,RoofMode::Hidden,true,covering)==0);
    assert(objectOpacity(roof,18,RoofMode::Full,true,covering)==1);
    roof.z=18;roof.flags=Wall;
    roof.height=1;assert(objectOpacity(roof,18,RoofMode::Auto,true,covering)==1);
    roof.flags=0;roof.height=80;assert(objectOpacity(roof,18,RoofMode::Auto,true,covering)==1);
    // At Z12 outside: ground-floor walls at Z20 remain, the entire next
    // storey at Z40 disappears, regardless of flags or screen overlap.
    roof.flags=Wall;roof.z=20;roof.height=20;
    assert(objectOpacity(roof,12,RoofMode::Hidden,false,away)==1);
    roof.z=40;
    assert(objectOpacity(roof,12,RoofMode::Hidden,false,away)==0);
    roof.flags=0;
    assert(objectOpacity(roof,12,RoofMode::Hidden,false,away)==0);
    assert(objectOpacity(roof,40,RoofMode::Hidden,false,away)==1);
    assert(objectOpacity(roof,12,RoofMode::Full,true,covering)==1);
    assert(nextRoofMode(nextRoofMode(nextRoofMode(RoofMode::Auto)))==RoofMode::Auto);
    // Overhang regression: object on a neighboring cell can cover the actor.
    roof.flags=Roof;roof.z=58;roof.x=5;roof.y=7;
    assert(objectOpacity(roof,18,RoofMode::Auto,true,covering)==.18f);
    // Same framebuffer layout used on hardware; protect both ends from overflow.
    std::vector<uint8_t> pixels(320*240*3+32,0xAA);
    HudInfo info{"PRIVATE UO SCENE","ROOFS AUTO",{
        "XY 1489,1619   Z 18 | --","FPS 47.9 | DRAW 942","CPU 13.68MS | GPU 5.12MS",
        "ATLAS 2.00 MIB | LISTS 474","LINEAR FREE 23.96 MIB","HEAP USED 6.32 MIB",
        "HEAP REUSABLE 0.28 MIB","RUN ART: ACTION 2 VERIFIED","","POSITION RESTORED"}};
    drawHud(pixels.data()+16,info);
    for(int i=0;i<16;++i){assert(pixels[i]==0xAA);assert(pixels[pixels.size()-1-i]==0xAA);}
    if(argc==2){
        std::ofstream f(argv[1],std::ios::binary);f<<"P6\n320 240\n255\n";
        for(int y=0;y<240;++y)for(int x=0;x<320;++x){auto p=pixels.data()+16+3*(x*240+239-y);char rgb[]={char(p[2]),char(p[1]),char(p[0])};f.write(rgb,3);}
    }
    World demo=demoWorld();
    for(int zoom=1;zoom<=4;++zoom)drawJournal(pixels.data()+16,demo,{0,0,0},nullptr,zoom);
    for(int i=0;i<16;++i){assert(pixels[i]==0xAA);assert(pixels[pixels.size()-1-i]==0xAA);}
    info.lines.assign(16,std::string(300,'W'));drawHud(pixels.data()+16,info);
    for(int i=0;i<16;++i){assert(pixels[i]==0xAA);assert(pixels[pixels.size()-1-i]==0xAA);}
    std::cout<<"Exterior occlusion, roof modes, support floors and HUD bounds passed\n";
}
