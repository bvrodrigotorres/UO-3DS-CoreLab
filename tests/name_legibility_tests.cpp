// SPDX-License-Identifier: MIT
#include "overhead_font.hpp"
#include "npcs.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    OverheadFont font;assert(font.width("a rat")>0);
    std::vector<bool>addresses(512*128);
    for(unsigned y=0;y<128;++y)for(unsigned x=0;x<512;++x){auto i=overheadPixelOffset(x,y);assert(i<addresses.size()&&!addresses[i]);addresses[i]=true;}
    if(argc==2){assert(font.load(argv[1]));assert(font.step(' ')==4);unsigned before=font.step('A');assert(before>=2);
        size_t fill=0,edge=0;
        for(int y=0;y<16;++y)for(int x=0;x<16;++x){fill+=font.pixel('A',x,y);edge+=font.outline('A',x,y);assert(!(font.pixel('A',x,y)&&font.outline('A',x,y)));}
        assert(fill&&edge);assert(font.width("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")<=360);
        std::ifstream in(argv[1],std::ios::binary);std::vector<char>b((std::istreambuf_iterator<char>(in)),{});b.back()^=1;
        auto bad=std::string(argv[1])+".bad";std::ofstream out(bad,std::ios::binary);out.write(b.data(),b.size());out.close();assert(!font.load(bad)&&font.step('A')==before);
    }
    NamePresence label;for(int i=0;i<119;++i){label.update(true,1.f/60);assert(label.visible());}
    label.update(true,1.f/60);assert(!label.visible());label.update(true,0);assert(!label.visible());label.update(false,0);assert(label.visible());
    World w;w.width=w.height=32;w.originX=1488;w.originY=1596;w.vertices.assign(1089,20);w.land.resize(1024);w.index();
    Npcs people;people.reset(w,{0,0,20});assert(people.all().size()==3);auto first=people.all()[0].position;Actor near{first.x-1,first.y,first.z};Doors doors;
    for(int frame=0;frame<120;++frame)people.update(w,near,near,false,1.f/60,doors);
    assert(!people.all()[0].namePresence.visible());people.update(w,{0,0,20},{0,0,20},false,1.f/60,doors);assert(people.all()[0].namePresence.visible());
    std::cout<<"Variable-width original font, outline masks, texture addressing, corruption and 2s proximity labels passed\n";
}
