// SPDX-License-Identifier: MIT
#include "roof_cutaway.hpp"
#include "label_font.hpp"
#include "performance.hpp"
#include "startup.hpp"
#include "npcs.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    World w;w.width=w.height=12;w.vertices.assign(169,0);w.land.resize(144);
    for(int x=2;x<6;++x){Object o;o.x=x;o.y=3;o.z=30+(x-2)*6;o.flags=Roof;w.objects.push_back(o);}
    Object other=w.objects[0];other.x=9;w.objects.push_back(other);
    Object floor;floor.x=3;floor.y=3;floor.z=20;floor.flags=Surface;w.objects.push_back(floor);w.index();
    RoofCutaway roofs;Actor inside{2,3,0};roofs.update(w,inside,RoofMode::Auto);
    for(size_t i=0;i<4;++i)assert(roofs.hidden(w,i,inside,RoofMode::Auto));
    assert(!roofs.hidden(w,4,inside,RoofMode::Auto));assert(!roofs.hidden(w,5,inside,RoofMode::Auto));
    Actor outside{1,3,0};roofs.update(w,outside,RoofMode::Auto);assert(!roofs.hidden(w,0,outside,RoofMode::Auto));
    Actor onRoof{2,3,30};roofs.update(w,onRoof,RoofMode::Auto);assert(!roofs.hidden(w,0,onRoof,RoofMode::Auto));
    roofs.update(w,inside,RoofMode::Full);assert(!roofs.hidden(w,0,inside,RoofMode::Full));
    assert(roofs.hidden(w,4,inside,RoofMode::Hidden));assert(!roofs.hidden(w,5,inside,RoofMode::Hidden));
    PauseMenu pause;pause.open();assert(pause.active&&pause.selected==0&&pause.activate()==PauseAction::Resume);
    pause.move(-1);assert(pause.activate()==PauseAction::Exit);pause.move(-1);assert(pause.activate()==PauseAction::MainMenu);
    pause.selected=1;assert(pause.activate()==PauseAction::Customize);
    assert(pause.hit(50,70)==0&&pause.hit(50,100)==1&&pause.hit(50,110)==2&&pause.hit(50,140)==3&&pause.hit(50,166)==4&&pause.hit(0,80)==-1);
    PositionCheckpoint checkpoint;checkpoint.baseline({0,0,0},1000);checkpoint.active(2000);Actor moved{1,0,0};
    assert(!checkpoint.due(moved,6000));checkpoint.suspend(120000);assert(!checkpoint.due(moved,126000));assert(checkpoint.due(moved,182000));
    assert(labelPixelOffset(0,0)==0&&labelPixelOffset(127,63)==8191);
    std::vector<bool>pixels(8192);for(unsigned y=0;y<64;++y)for(unsigned x=0;x<128;++x){size_t i=labelPixelOffset(x,y);assert(i<8192&&!pixels[i]);pixels[i]=true;}
    assert(labelPixel('A',2,0)&&!labelPixel(' ',2,0)&&labelPixel('a',2,2));
    if(argc==4){World scene;std::string error;assert(loadWorld(argv[1],scene,error));Npcs n;
        assert(n.loadPopulation(argv[2],argv[1],scene,error));assert(n.loadNames(argv[3],argv[1],argv[2],error));n.reset(scene,{0,0,0});
        assert(n.all()[0].name=="Alice"&&n.all()[0].title=="the mage"&&n.all()[0].nameHue==0x35&&n.all()[0].nameColor[0]==238);
        std::ifstream in(argv[3],std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(in)),{});b.back()^=1;
        std::string bad=std::string(argv[3])+".bad";std::ofstream out(bad,std::ios::binary);out.write((const char*)b.data(),b.size());out.close();
        assert(!n.loadNames(bad,argv[1],argv[2],error));n.reset(scene,{0,0,0});assert(n.all()[0].name=="Alice");
    }
    std::cout<<"Connected stepped roofs, floor preservation, pause actions/clock, font layout and NPC name validation passed\n";
}
