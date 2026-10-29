// SPDX-License-Identifier: MIT
#include "world_stream.hpp"
#include "view.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>
#include <iostream>
using namespace lab;
static void waitFor(const std::function<bool()>&ready){auto until=std::chrono::steady_clock::now()+std::chrono::seconds(10);while(!ready()){assert(std::chrono::steady_clock::now()<until);std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
int main(int argc,char**argv){
 World active;active.width=active.height=64;active.originX=active.originY=128;Actor player{32,32,20};
 auto p=StreamPlan::wanted(active,player,1,0,7168,4096);assert(p.x==144&&p.y==128);player={44,44,20};p=StreamPlan::wanted(active,player,1,0,7168,4096);assert(p.x==144&&p.y==144);assert(StreamPlan::due(active,player,7168,4096));World next=active;next.originX+=16;next.originY+=16;assert(StreamPlan::accepts(active,player,next,7168,4096));player={48,30,20};assert(!StreamPlan::insideGuard(active,player,7168,4096));player={47,30,20};assert(StreamPlan::insideGuard(active,player,7168,4096));
 std::atomic<bool>entered{false},release{false};std::atomic<unsigned>calls{0},working{0},maximum{0};
 WorldStream stream([&](int x,int y,std::string&error){unsigned busy=++working;maximum.store(std::max(maximum.load(),busy));unsigned n=++calls;if(n==1){entered=true;while(!release.load())std::this_thread::sleep_for(std::chrono::milliseconds(1));}auto frame=std::make_unique<StreamFrame>();frame->world.originX=x;frame->world.originY=y;--working;if(x==999){error="SD FAILURE";return std::unique_ptr<StreamFrame>{};}return frame;});
 assert(stream.start()&&stream.request(128,128));waitFor([&]{return entered.load();});auto began=std::chrono::steady_clock::now();assert(stream.request(144,128)&&stream.request(160,128));assert(std::chrono::steady_clock::now()-began<std::chrono::milliseconds(100));release=true;
 waitFor([&]{return stream.status().ready;});auto frame=stream.take();assert(frame&&frame->world.originX==160&&maximum==1);assert(calls<=3);unsigned before=calls;assert(stream.request(160,128));std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(calls==before);stream.cancel();assert(stream.request(160,128));waitFor([&]{return stream.status().ready;});assert(stream.take());assert(stream.request(999,128));waitFor([&]{return !stream.status().error.empty();});assert(!stream.status().ready);stream.cancelAndWait();assert(!stream.status().ready&&stream.status().error.empty());
 // Run the production planning/guard logic against a deliberately slow builder.
 // Virtual walking and mounted running must cross boundaries without stopping.
 unsigned seamless=0;
 for(int interval:{12,3})for(int direction=0;direction<8;++direction){
  const int dxs[]={0,1,1,1,0,-1,-1,-1},dys[]={-1,-1,0,1,1,1,0,-1};int dx=dxs[direction],dy=dys[direction];
  auto make=[](int x,int y){auto f=std::make_unique<StreamFrame>();f->world.width=f->world.height=64;f->world.originX=x;f->world.originY=y;f->world.land.resize(4096);f->world.vertices.assign(4225,0);f->world.index();return f;};
  auto current=make(128,128);Actor a{32,32,0};WorldStream slow([&](int x,int y,std::string&){std::this_thread::sleep_for(std::chrono::milliseconds(25));return make(x,y);});assert(slow.start());unsigned moved=0,waited=0;
  for(unsigned tick=0;tick<2000&&moved<24;++tick){auto target=StreamPlan::wanted(current->world,a,dx,dy,7168,4096);if(target.x!=current->world.originX||target.y!=current->world.originY)assert(slow.request(target.x,target.y));
   if(tick%unsigned(interval)==0){Actor to;int actual;assert(step(current->world,a,direction,to,actual));if(StreamPlan::insideGuard(current->world,to,7168,4096)){a=to;++moved;}else ++waited;}
   if(StreamPlan::due(current->world,a,7168,4096)&&slow.status().ready){auto ready=slow.take();assert(ready&&StreamPlan::accepts(current->world,a,ready->world,7168,4096));auto g=ExpeditionWindow::global(current->world,a);current=std::move(ready);a=ExpeditionWindow::local(current->world,g);assert(ExpeditionWindow::global(current->world,a).x==g.x);++seamless;}
   std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }assert(moved==24&&waited==0);
 }
 std::cout<<"Walking/mounted simulation: "<<seamless<<" seamless handoffs in eight directions with delayed reads; no guard stops\n";
 if(argc==3){WorldArchive archive;WorldGraphics graphics;std::string error;assert(archive.open(argv[1],error)&&graphics.load(std::string(argv[2])+"/",error));auto current=prepareStreamFrame(archive,graphics,StreamPlan::origin(1496,archive.width()),StreamPlan::origin(1628,archive.height()),WorldArchive::MaxObjects,error);assert(current);size_t peak=current->bytes();unsigned switches=0;
  // Exercise production builder and overlap in every direction, with a stable
  // global elevation/collision result on shared cells, including negative z.
  int dxs[]={1,0,-1,0,1,1,-1,-1},dys[]={0,1,0,-1,1,-1,1,-1};
  for(unsigned i=0;i<320;++i){int dx=dxs[(i/20)%8],dy=dys[(i/20)%8];int x=std::max(0,std::min(archive.width()-64,current->world.originX+dx*16)),y=std::max(0,std::min(archive.height()-64,current->world.originY+dy*16));auto ahead=prepareStreamFrame(archive,graphics,x,y,StreamPlan::MaxAheadObjects,error);assert(ahead);peak=std::max(peak,current->bytes()+ahead->bytes());
   for(int gy=std::max(y,current->world.originY);gy<std::min(y+64,current->world.originY+64);gy+=4)for(int gx=std::max(x,current->world.originX);gx<std::min(x+64,current->world.originX+64);gx+=4){int ax=gx-current->world.originX,ay=gy-current->world.originY,bx=gx-x,by=gy-y;assert(current->world.vertex(ax,ay)==ahead->world.vertex(bx,by));const auto&la=current->world.land[ay*64+ax];const auto&lb=ahead->world.land[by*64+bx];assert(la.art==lb.art&&la.image==lb.image&&la.flags==lb.flags);}
   // Radar generated directly from sector records equals scene raster samples.
   auto exact=graphics.map(ahead->world);for(int yy=0;yy<64;yy+=7)for(int xx=0;xx<64;xx+=7){size_t a=(yy*64+xx)*3,b=((yy+y-ahead->radar.originY)*ahead->radar.width+xx+x-ahead->radar.originX)*3;for(unsigned c=0;c<3;++c)assert(exact.map[a+c]==ahead->radar.map[b+c]);}
   current=std::move(ahead);++switches;assert(archive.cacheBytes()<=WorldArchive::CacheBudget);
  }
  World unchanged;unchanged.width=7;assert(!archive.window(6144,3712,128,unchanged,error,1)&&unchanged.width==7);
  std::cout<<"PRIVATE: "<<switches<<" sliding 64x64 views, terrain/graphics/radar seams match; peak native two-frame payload "<<peak<<" bytes; raw cache <=512 KiB\n";
 }
 std::cout<<"Streaming: nonblocking delayed reads, coalesced direction changes, stale-result rejection, one builder, errors, exact guard and diagonal planning passed\n";
}
