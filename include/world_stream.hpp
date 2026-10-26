// SPDX-License-Identifier: MIT
#pragma once
#include "world_expedition.hpp"
#include "roof_cutaway.hpp"
#include <functional>
#include <memory>
namespace lab {
struct StreamFrame {
    World world;Journal radar;RoofCutaway roofs;
    size_t bytes()const{size_t n=world.vertices.capacity()*sizeof(int)+world.land.capacity()*sizeof(Land)+world.objects.capacity()*sizeof(Object)+world.cells.capacity()*sizeof(std::vector<size_t>)+radar.map.capacity()+roofs.bytes();for(const auto&c:world.cells)n+=c.capacity()*sizeof(size_t);return n;}
};
struct StreamPlan {
    static constexpr int Side=64,Stride=16,Guard=16,MaxAheadObjects=12288;
    static int origin(int global,int extent){return std::max(0,std::min(extent-Side,((global-24)/Stride)*Stride));}
    static Actor wanted(const World&w,const Actor&a,int dx,int dy,int width,int height){
        int x=w.originX,y=w.originY;
        if(a.x>=44||(dx>0&&a.x>=32))x+=Stride;
        else if(a.x<20||(dx<0&&a.x<=32))x-=Stride;
        if(a.y>=44||(dy>0&&a.y>=32))y+=Stride;
        else if(a.y<20||(dy<0&&a.y<=32))y-=Stride;
        return {std::max(0,std::min(width-Side,x)),std::max(0,std::min(height-Side,y)),0};
    }
    static bool due(const World&w,const Actor&a,int width,int height){return (a.x<20&&w.originX>0)||(a.y<20&&w.originY>0)||(a.x>=44&&w.originX+Side<width)||(a.y>=44&&w.originY+Side<height);}
    static bool insideGuard(const World&w,const Actor&a,int width,int height){return (a.x>=Guard||w.originX==0)&&(a.y>=Guard||w.originY==0)&&(a.x<Side-Guard||w.originX+Side==width)&&(a.y<Side-Guard||w.originY+Side==height);}
    static bool accepts(const World&old,const Actor&a,const World&next,int width,int height){
        auto g=ExpeditionWindow::global(old,a);auto local=ExpeditionWindow::local(next,g);
        return insideGuard(next,local,width,height)&&
            (!(a.x<20&&old.originX>0)||next.originX<old.originX)&&(!(a.y<20&&old.originY>0)||next.originY<old.originY)&&
            (!(a.x>=44&&old.originX+Side<width)||next.originX>old.originX)&&(!(a.y>=44&&old.originY+Side<height)||next.originY>old.originY);
    }
};
class WorldStream {
    struct Impl;std::unique_ptr<Impl>impl;
public:
    using Builder=std::function<std::unique_ptr<StreamFrame>(int,int,std::string&)>;
    struct Status {int x=-1,y=-1;bool ready=false,busy=false;std::string error;};
    explicit WorldStream(Builder);~WorldStream();
    bool start();bool request(int,int);Status status()const;
    std::unique_ptr<StreamFrame> take();
    // Called only behind a loading screen, before synchronous archive access.
    void cancel();void cancelAndWait();
};
std::unique_ptr<StreamFrame> prepareStreamFrame(WorldArchive&,const WorldGraphics&,int,int,unsigned,std::string&);
}
