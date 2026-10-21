// SPDX-License-Identifier: MIT
#include "visibility.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
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
    info.lines.assign(16,std::string(300,'W'));drawHud(pixels.data()+16,info);
    for(int i=0;i<16;++i){assert(pixels[i]==0xAA);assert(pixels[pixels.size()-1-i]==0xAA);}
    std::cout<<"Exterior occlusion, roof modes, support floors and HUD bounds passed\n";
}
