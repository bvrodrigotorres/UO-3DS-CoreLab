// SPDX-License-Identifier: MIT
#include "world_expedition.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
static World fixture(int x,int y){World w;w.width=w.height=128;w.originX=x;w.originY=y;w.land.resize(128*128);w.vertices.assign(129*129,0);w.objects.resize(100);w.index();return w;}
static void empty(const World&w,const Journal&j){assert(w.land.capacity()==0&&w.objects.capacity()==0&&w.vertices.capacity()==0&&w.cells.capacity()==0&&j.map.capacity()==0);}
int main(int argc,char**argv){
 World scene=fixture(128,256);Actor actor{104,60,20};Journal map;map.map.resize(128*128*3);std::string error;
 Actor wanted{232,316,20};unsigned calls=0;
 auto build=[&](const Actor&g,bool,int ox,int oy){empty(scene,map);++calls;scene=fixture(ox<0?ExpeditionWindow::origin(g.x,7168):ox,oy<0?ExpeditionWindow::origin(g.y,4096):oy);actor=ExpeditionWindow::local(scene,g);map.map.resize(128*128*3);return true;};
 assert(replaceExpeditionWindow(scene,actor,map,wanted,false,error,build)==WindowReplacement::Loaded&&calls==1);assert(ExpeditionWindow::global(scene,actor).x==232&&actor.x==40);
 scene=fixture(128,256);actor={104,60,20};const auto before=ExpeditionWindow::global(scene,actor);calls=0;
 auto failure=[&](const Actor&g,bool safe,int ox,int oy){empty(scene,map);++calls;if(calls==1){assert(safe);scene=fixture(800,800);map.map.resize(99);error="SD READ FAILED";return false;}assert(!safe&&g.x==before.x&&g.y==before.y&&g.z==before.z);assert(ox==128&&oy==256);return build(g,false,ox,oy);};
 assert(replaceExpeditionWindow(scene,actor,map,{900,900,0},true,error,failure)==WindowReplacement::Recovered);assert(error=="SD READ FAILED"&&ExpeditionWindow::global(scene,actor).x==before.x);
 calls=0;auto corrupt=[&](const Actor&,bool,int,int){empty(scene,map);++calls;error="CORRUPT SECTOR";return false;};
 assert(replaceExpeditionWindow(scene,actor,map,{900,900,0},true,error,corrupt)==WindowReplacement::Unavailable&&calls==2);empty(scene,map);
 calls=0;assert(replaceExpeditionWindow(scene,actor,map,{900,900,0},true,error,corrupt)==WindowReplacement::Unavailable&&calls==1);
 RenderCache cache;World simple=demoWorld();Actor a{simple.startX,simple.startY,simple.startZ};cache.get(simple,a,false);auto builds=cache.rebuilds;cache.release();cache.get(simple,a,false);assert(cache.rebuilds==builds+1);
 if(argc==3){WorldArchive archive;WorldGraphics graphics;assert(archive.open(argv[1],error)&&graphics.load(std::string(argv[2])+"/",error));size_t largest=0;uint64_t visited=0;
  auto decode=[&](const Actor&g,bool,int ox,int oy){empty(scene,map);World next;assert(archive.window(ox<0?ExpeditionWindow::origin(g.x,archive.width()):ox,oy<0?ExpeditionWindow::origin(g.y,archive.height()):oy,128,next,error));assert(graphics.bind(next,error));assert(next.objects.capacity()==next.objects.size());largest=std::max(largest,next.objects.size());scene=std::move(next);actor=ExpeditionWindow::local(scene,g);map=graphics.map(scene);++visited;return true;};
  const int xs[]={1472,1536,1472,1408,1472,1472,6208},ys[]={1600,1600,1664,1600,1536,1600,3776};
  for(unsigned n=0;n<280;++n){unsigned i=n%7;assert(replaceExpeditionWindow(scene,actor,map,{xs[i],ys[i],20},false,error,decode)==WindowReplacement::Loaded);assert(ExpeditionWindow::global(scene,actor).x==xs[i]);assert(archive.cacheBytes()<=WorldArchive::CacheBudget);}
  std::cout<<"PRIVATE: "<<visited<<" sector replacements around Britain and dense-map region; max "<<largest<<" statics, no concurrent scene payloads\n";
 }
 std::cout<<"Transitions: release before load, exact reservation, failed-read rollback, fatal recovery, global position and render-cache reset passed\n";
}
