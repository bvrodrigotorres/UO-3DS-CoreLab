// SPDX-License-Identifier: MIT
#include "world_expedition.hpp"
#include "atlas_layout.hpp"
#include <fstream>
#include <cassert>
#include <iostream>
using namespace lab;
static void put(std::vector<uint8_t>&b,unsigned n,unsigned bytes){for(unsigned i=0;i<bytes;++i)b.push_back(uint8_t(n>>(8*i)));}
static void write(const std::string&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write((char*)b.data(),b.size());}
int main(int argc,char**argv){
 assert(expeditionDestinationHit(false,220,80)==-1);assert(expeditionDestinationHit(true,220,80)==2);assert(expeditionDestinationHit(true,20,151)==9);assert(expeditionDestinationHit(true,20,152)==-1);
 unsigned slots=32;assert(readAtlasLayout({'U','W','P','1',2,0,0,0},slots)&&slots==2);assert(!readAtlasLayout({'U','W','P','1',3,0,0,0},slots)&&slots==2);assert(!readAtlasLayout({},slots));
 assert(argc>=2);std::string root=std::string(argv[1])+"/",error;std::vector<uint8_t>body;for(auto r:{std::array<unsigned,3>{0,3,0},{1,100,1}})for(auto n:r)put(body,n,2);
 uint32_t hash=2166136261u;for(auto c:body)hash=(hash^c)*16777619u;
 std::vector<uint8_t>b{'U','W','G','1'};put(b,32,4);put(b,2,4);put(b,hash,4);b.insert(b.end(),body.begin(),body.end());write(root+"graphics.bin",b);write(root+"radar.bin",std::vector<uint8_t>(65536,255));
 WorldGraphics gfx;assert(gfx.load(root,error)&&gfx.images()==32);World w;w.width=w.height=2;w.vertices.assign(9,0);w.land.resize(4);w.land[0].texture=true;Object o;o.art=100;o.x=1;o.y=1;w.objects.push_back(o);w.index();assert(gfx.bind(w,error)&&w.land[0].image==0&&!w.land[0].texture&&w.objects[0].image==1);
 auto j=gfx.map(w);assert(j.width==2&&j.map.size()==12&&j.map[0]==255);w.land[3].art=2;assert(gfx.bind(w,error)&&w.land[3].image==NoImage);w.objects[0].art=101;assert(!gfx.bind(w,error)&&w.objects[0].image==1);w.objects[0].art=100;
 b.back()^=1;write(root+"graphics.bin",b);assert(!gfx.load(root,error)&&gfx.images()==32);assert(gfx.bind(w,error));write(root+"graphics.bin",{});assert(!gfx.load(root,error));
 for(int x=0;x<7168;++x){int origin=ExpeditionWindow::origin(x,7168);assert(origin>=0&&origin%64==0&&origin<=7040&&x>=origin&&x<origin+128);}
 World a;a.width=a.height=128;a.originX=a.originY=128;Actor local{103,50,20};assert(!ExpeditionWindow::needs(a,local,7168,4096));local.x=104;assert(ExpeditionWindow::needs(a,local,7168,4096));auto global=ExpeditionWindow::global(a,local);World next;next.width=next.height=128;next.originX=ExpeditionWindow::origin(global.x,7168);next.originY=ExpeditionWindow::origin(global.y,4096);auto shifted=ExpeditionWindow::local(next,global);assert(shifted.x==40&&!ExpeditionWindow::needs(next,shifted,7168,4096));shifted.x--;assert(!ExpeditionWindow::needs(next,shifted,7168,4096));assert(ExpeditionWindow::global(next,ExpeditionWindow::local(next,global)).x==global.x);
 if(argc==4){WorldArchive archive;WorldGraphics actual;assert(archive.open(argv[2],error)&&actual.load(std::string(argv[3])+"/",error));uint64_t objects=0;for(int x=0;x<archive.width();x+=128)for(int y=0;y<archive.height();y+=128){World view;assert(archive.window(x,y,128,view,error)&&actual.bind(view,error));objects+=view.objects.size();}assert(objects==archive.objectCount());std::cout<<"FULL WORLD: graphics bound to "<<objects<<" original static objects\n";}
 std::cout<<"Expedition: catalogue validation, transactional failure, radar, map edges and transition hysteresis passed\n";
}
