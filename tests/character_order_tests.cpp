// SPDX-License-Identifier: MIT
#include "character_order.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <tuple>
using namespace lab;
namespace {
using Key=std::tuple<int,size_t,int,int,int,uint32_t>;
Key key(const Draw&d){return {int(d.kind),d.index,d.x,d.y,d.z,d.order};}
void invariant(const std::vector<Draw>&before,const std::vector<Draw>&after){
    assert(before.size()==after.size());std::map<Key,int>counts;
    std::map<std::pair<int,int>,std::vector<Key>>oldColumns,newColumns;
    for(const auto&d:before){++counts[key(d)];if(d.kind!=Kind::Actor)oldColumns[{d.x,d.y}].push_back(key(d));}
    for(const auto&d:after){--counts[key(d)];if(d.kind!=Kind::Actor)newColumns[{d.x,d.y}].push_back(key(d));}
    for(auto entry:counts)assert(entry.second==0);
    assert(oldColumns==newColumns); // never expose contents below a prefetched floor
}
bool before(const Draw&a,const Draw&b){
    if(a.x+a.y!=b.x+b.y)return a.x+a.y<b.x+b.y;
    if(a.x!=b.x)return a.x<b.x;
    if(a.priority!=b.priority)return a.priority<b.priority;
    if((a.kind==Kind::Land)!=(b.kind==Kind::Land))return a.kind==Kind::Land;
    return a.order<b.order;
}
}
int main(int argc,char**argv){
    World w=demoWorld();w.objects.clear();w.vertices.assign((w.width+1)*(w.height+1),0);
    auto add=[&](int x,int y,int z,int height,uint64_t flags){Object o;o.x=x;o.y=y;o.z=z;o.height=height;o.flags=flags;o.order=w.objects.size();w.objects.push_back(o);return w.objects.size()-1;};
    auto chair=add(9,8,20,5,Surface),floor=add(9,8,40,0,Surface|Background);
    auto chest=add(9,8,40,10,Surface|Impassable),roof=add(9,8,60,0,Roof|Surface);
    auto far=add(13,13,20,0,Surface),wall=add(8,9,40,20,Wall|Impassable);
    auto upper=add(8,9,60,0,Surface),step=add(9,9,35,0,Surface);
    auto zeroWall=add(9,7,40,0,Wall),zeroDoor=add(10,6,40,0,Door);
    w.index();Actor actor{8,8,40};auto list=renderList(w,actor,false);
    list.push_back({Kind::Npc,9,8,20,21,0,0xffffff00u});std::stable_sort(list.begin(),list.end(),before);
    CharacterOrder order;auto output=order.build(w,list,40);invariant(list,output);
    auto find=[&](Kind kind,size_t index){for(size_t n=0;n<output.size();++n)if(output[n].kind==kind&&output[n].index==index)return n;return output.size();};
    auto player=find(Kind::Actor,0);assert(find(Kind::Static,chair)<find(Kind::Static,floor));
    assert(find(Kind::Npc,0)<find(Kind::Static,floor)&&find(Kind::Static,floor)<player);
    assert(player<find(Kind::Static,chest)&&player<find(Kind::Static,roof));
    assert(player<find(Kind::Static,wall)&&player<find(Kind::Static,upper)&&player<find(Kind::Static,far));
    assert(find(Kind::Static,step)<player);
    assert(player<find(Kind::Static,zeroWall)&&player<find(Kind::Static,zeroDoor));
    actor.z=27;list=renderList(w,actor,false);output=order.build(w,list,37);invariant(list,output);
    assert(find(Kind::Static,step)<find(Kind::Actor,0));
    output=order.build(w,list,27);assert(find(Kind::Actor,0)<find(Kind::Static,step)); // upper floor remains in front when standing below
    World slope=demoWorld();Actor hill{11,10,1};auto hillList=renderList(slope,hill,false);
    auto hillOrder=order.build(slope,hillList,3);invariant(hillList,hillOrder);
    size_t landAt=hillOrder.size(),actorAt=hillOrder.size();
    for(size_t n=0;n<hillOrder.size();++n){if(hillOrder[n].kind==Kind::Actor)actorAt=n;if(hillOrder[n].kind==Kind::Land&&hillOrder[n].x==12&&hillOrder[n].y==10)landAt=n;}
    assert(landAt<actorAt); // visible source slope cannot cover the descending sprite
    unsigned seed=47;for(int i=0;i<1000;++i){seed=seed*1664525u+1013904223u;actor={2+int(seed%10),2+int((seed>>8)%10),int((seed>>16)%60)};list=renderList(w,actor,bool(i%2));output=order.build(w,list,actor.z+(i%3-1)*5);invariant(list,output);}
    if(argc==2){std::string e;assert(loadWorld(argv[1],w,e));size_t cases=0;
        for(int z:{20,22,27,32,37,40})for(int y=6;y<=10;++y)for(int x=5;x<=10;++x){actor={x,y,z};list=renderList(w,actor,true);output=order.build(w,list,z);invariant(list,output);++cases;}
        std::cout<<"Private Britain stairs/landing column invariants: "<<cases<<" cases\n";
    }
    std::cout<<"Character neighbor scheduling: exact-once, lower NPC/items hidden by floors, upper floors, walls, chests, landings and 1000 randomized cases passed\n";
}
