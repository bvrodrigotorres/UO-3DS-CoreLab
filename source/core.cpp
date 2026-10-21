// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2016 Hotride
// SPDX-FileCopyrightText: 2026 UO-3DS-CoreLab contributors
// Adapted from CrossUO MapBlock.cpp, LandObject.cpp and PathFinder.cpp.
// See THIRD_PARTY.md for pinned revisions, changes and excluded behavior.
#include "core.hpp"
#include <algorithm>
#include <cmath>

namespace lab {
bool World::contains(int x,int y) const {return x>=0&&y>=0&&x<width&&y<height;}
int World::vertex(int x,int y) const {return vertices[y*(width+1)+x];}
void World::index(){
    cells.assign(width*height,{});
    for(size_t i=0;i<objects.size();++i)if(contains(objects[i].x,objects[i].y))
        cells[objects[i].y*width+objects[i].x].push_back(i);
}
void World::moved(size_t i,int oldX,int oldY){
    if(i>=objects.size())return;
    if(contains(oldX,oldY)){auto&v=cells[oldY*width+oldX];v.erase(std::remove(v.begin(),v.end(),i),v.end());}
    const auto&o=objects[i];if(contains(o.x,o.y)){auto&v=cells[o.y*width+o.x];auto at=std::lower_bound(v.begin(),v.end(),i);v.insert(at,i);}
}
Heights heights(const World&w,int x,int y){
    Heights h={w.vertex(x,y),w.vertex(x+1,y),w.vertex(x+1,y+1),w.vertex(x,y+1),0,0,false};
    h.minimum=std::min(std::min(h.top,h.right),std::min(h.bottom,h.left));
    // Match arithmetic right-shift (floor), including negative odd heights.
    int sum=std::abs(h.top-h.bottom)<=std::abs(h.left-h.right)?h.top+h.bottom:h.left+h.right;
    h.average=(int)std::floor(sum/2.0);
    h.slope=!(h.top==h.right&&h.top==h.bottom&&h.top==h.left);
    return h;
}
int directionalHeight(const Heights&h,int d){
    const int corners[]={h.top,h.right,h.bottom,h.left};
    int a=corners[((d>>1)+1)&3];
    return d&1?a:(int)std::floor((a+corners[(d>>1)&3])/2.0);
}
int priority(Kind k,int z,int height,uint64_t flags,int average,bool slope){
    if(k==Kind::Land)return (slope?average:z)-1;
    if(k==Kind::Actor)return z+1;
    return z-((flags&Background)?1:0)+(height?1:0);
}
std::vector<Draw> renderList(const World&w,const Actor&a,bool cutaway){
    std::vector<Draw> out;out.reserve(2048);
    // Experimental height ceiling: retain all data; hide only in presentation.
    int ceiling=128;
    if(cutaway&&w.contains(a.x,a.y))for(size_t i:w.cells[a.y*w.width+a.x]){
        const auto&o=w.objects[i];
        if(o.z>a.z+14&&(o.flags&(Roof|Surface)))ceiling=std::min(ceiling,o.z);
    }
    const int left=std::max(0,a.x-12),right=std::min(w.width-1,a.x+12);
    const int top=std::max(0,a.y-12),bottom=std::min(w.height-1,a.y+12);
    for(int sum=left+top;sum<=right+bottom;++sum)for(int x=left;x<=right;++x){
        int y=sum-x;if(!w.contains(x,y))continue;
        size_t cell=y*w.width+x;
        if(std::abs(x-a.x)>12||std::abs(y-a.y)>12)continue;
        auto h=heights(w,x,y);
        std::vector<Draw> column;
        column.push_back({Kind::Land,x,y,h.top,priority(Kind::Land,h.top,0,0,h.average,h.slope),cell,0});
        for(size_t i:w.cells[cell]){
            const auto&o=w.objects[i];
            if(cutaway&&o.z>=ceiling)continue;
            column.push_back({Kind::Static,x,y,o.z,priority(Kind::Static,o.z,o.height,o.flags,0,false),i,o.order+1});
        }
        if(x==a.x&&y==a.y)column.push_back({Kind::Actor,x,y,a.z,a.z+1,0,0xffffffffu});
        std::stable_sort(column.begin(),column.end(),[](const Draw&p,const Draw&q){
            if(p.priority!=q.priority)return p.priority<q.priority;
            if((p.kind==Kind::Land)!=(q.kind==Kind::Land))return p.kind==Kind::Land;
            return p.order<q.order;
        });
        out.insert(out.end(),column.begin(),column.end());
    }
    return out;
}
bool solveCandidates(std::vector<Candidate> list,int currentZ,int minZ,int maxZ,int& nextZ){
    if(list.empty())return false;
    std::stable_sort(list.begin(),list.end(),[](const Candidate&a,const Candidate&b){
        return a.z!=b.z?a.z<b.z:a.height<b.height;
    });
    list.push_back({1,128,128,128});
    int z=std::max(currentZ,minZ),result=-128,best=1000000,current=-128;
    for(size_t i=0;i<list.size();++i){
        const auto&o=list[i];
        if(!(o.flags&1))continue;
        if(o.z-minZ>=16)for(int j=(int)i-1;j>=0;--j){
            const auto&t=list[j];
            if((t.flags&6)&&t.average>=current&&o.z-t.average>=16&&
               ((t.average<=maxZ&&(t.flags&2))||((t.flags&4)&&t.z<=maxZ))){
                int delta=std::abs(z-t.average);
                if(delta<best){best=delta;result=t.average;}
            }
        }
        minZ=std::max(minZ,o.average);current=std::max(current,o.average);
    }
    if(result==-128)return false;
    nextZ=result;return true;
}
static std::vector<Candidate> candidates(const World&w,int x,int y){
    std::vector<Candidate> list;if(!w.contains(x,y))return list;
    auto h=heights(w,x,y);const auto&land=w.land[y*w.width+x];
    if((land.art<0x1ae&&land.art!=2)||(land.art>0x1b5&&land.art!=0x1db))
        list.push_back({unsigned((land.flags&Impassable)?1:7),h.minimum,h.average,h.average-h.minimum,true});
    for(size_t i:w.cells[y*w.width+x]){
        const auto&o=w.objects[i];unsigned f=0;
        if(o.flags&(Impassable|Surface))f=1;
        if(!(o.flags&Impassable)){if(o.flags&Surface)f|=2;if(o.flags&Bridge)f|=4;}
        if(f)list.push_back({f,o.z,o.z+((o.flags&Bridge)?o.height/2:o.height),o.height});
    }
    return list;
}
static bool probe(const World&w,const Actor&a,int x,int y,int direction,int&z){
    if(!w.contains(x,y)||!w.contains(a.x,a.y))return false;
    int minimum=-128,maximum=a.z;
    auto h=heights(w,a.x,a.y);
    auto source=candidates(w,a.x,a.y);
    for(size_t i=0;i<source.size();++i){
        const auto&o=source[i];
        if(o.terrain&&h.slope&&o.average<=a.z){
            int average=directionalHeight(h,direction);
            minimum=std::max(minimum,average);maximum=std::max(maximum,average);
        }else{
            if((o.flags&1)&&o.average<=a.z)minimum=std::max(minimum,o.average);
            if((o.flags&4)&&a.z==o.average){maximum=std::max(maximum,o.z+o.height);minimum=std::min(minimum,o.z);}
        }
    }
    return solveCandidates(candidates(w,x,y),a.z,minimum,maximum+2,z);
}
bool step(const World&w,const Actor&a,int direction,Actor&result,int&actualDirection){
    if(direction<0||direction>7)return false;
    const int dx[]={0,1,1,1,0,-1,-1,-1},dy[]={-1,-1,0,1,1,1,0,-1};
    auto attempt=[&](int d,Actor&next){next={a.x+dx[d],a.y+dy[d],a.z};return probe(w,a,next.x,next.y,d,next.z);};
    Actor next;bool passed=attempt(direction,next);int chosen=direction;
    if(direction&1){
        if(passed)for(int offset:{1,7}){Actor side;if(!attempt((direction+offset)&7,side)){passed=false;break;}}
        if(!passed)for(int offset:{1,7})if(attempt((direction+offset)&7,next)){passed=true;chosen=(direction+offset)&7;break;}
    }
    if(passed){result=next;actualDirection=chosen;}return passed;
}
World demoWorld(){
    World w;w.width=w.height=16;w.startX=8;w.startY=10;w.startZ=0;
    w.vertices.assign(17*17,0);w.land.resize(256);
    for(int y=0;y<=16;++y)for(int x=0;x<=16;++x)if(x>=11)w.vertices[y*17+x]=std::min(8,(x-11)*2);
    auto add=[&](int x,int y,int z,int height,uint64_t flags,int art){
        Object o;o.x=x;o.y=y;o.z=z;o.height=height;o.flags=flags;o.art=art;o.order=w.objects.size();w.objects.push_back(o);
    };
    // A small room, two open doorways, elevated floor and staircase.
    for(int y=4;y<=8;++y)for(int x=4;x<=8;++x){
        add(x,y,4,0,Surface,1);
        if(x==4||y==4||(x==8&&y!=7)||(y==8&&x!=7))add(x,y,4,20,Wall|Impassable,2);
        add(x,y,24,0,Roof|Surface,3);
    }
    add(7,9,0,4,Bridge|Surface,4);
    add(7,8,2,4,Bridge|Surface,4);
    add(6,6,5,0,Background,5); // rug above the floor
    add(10,6,0,0,Surface,1);add(10,6,24,0,Surface|Roof,3); // two supports at one XY
    w.index();return w;
}
}
