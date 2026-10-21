// SPDX-License-Identifier: MIT
#include "core.hpp"
#include <cstdlib>
#include <iostream>
int main(int argc,char**argv){
    lab::World w;std::string error;
    if(argc<6){std::cerr<<"trace scene.bin localX localY Z cutaway\n";return 2;}
    if(!lab::loadWorld(argv[1],w,error)){std::cerr<<error<<"\n";return 1;}
    lab::Actor actor={std::atoi(argv[2]),std::atoi(argv[3]),std::atoi(argv[4])};
    if(!w.contains(actor.x,actor.y))return 2;
    auto list=lab::renderList(w,actor,std::atoi(argv[5])!=0);
    std::cout<<"{\"actor\":["<<actor.x<<","<<actor.y<<","<<actor.z<<"],\"entries\":[";
    bool first=true;
    for(const auto&d:list){
        if(!first)std::cout<<",";first=false;
        std::cout<<"{\"kind\":"<<(int)d.kind<<",\"x\":"<<d.x<<",\"y\":"<<d.y<<",\"z\":"<<d.z<<",\"priority\":"<<d.priority;
        if(d.kind==lab::Kind::Land){const auto&l=w.land[d.index];auto h=lab::heights(w,d.x,d.y);std::cout<<",\"image\":"<<l.image<<",\"art\":"<<l.art<<",\"texture\":"<<l.texture<<",\"corners\":["<<h.top<<","<<h.right<<","<<h.bottom<<","<<h.left<<"]";}
        else if(d.kind==lab::Kind::Static){const auto&o=w.objects[d.index];std::cout<<",\"image\":"<<o.image<<",\"art\":"<<o.art<<",\"hue\":"<<o.hue<<",\"height\":"<<o.height<<",\"order\":"<<o.order;}
        std::cout<<"}";
    }
    int traversable=0;for(int direction=0;direction<8;++direction){lab::Actor next;int actual;if(lab::step(w,actor,direction,next,actual))++traversable;}
    std::cout<<"],\"available_directions\":"<<traversable<<"}\n";
}
