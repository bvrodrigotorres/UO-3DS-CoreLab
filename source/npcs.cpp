// SPDX-License-Identifier: MIT
#include "npcs.hpp"
#include <algorithm>
#include <cmath>
namespace lab {
static bool same(const Actor&a,const Actor&b){return a.x==b.x&&a.y==b.y&&std::abs(a.z-b.z)<16;}
bool canStand(const World&w,const Actor&a){
    if(!w.contains(a.x,a.y))return false;
    auto h=heights(w,a.x,a.y);const auto&land=w.land[a.y*w.width+a.x];
    bool support=!(land.flags&(Impassable|Wet))&&land.art!=2&&h.average==a.z;
    for(size_t i:w.cells[a.y*w.width+a.x]){
        const auto&o=w.objects[i];
        int top=o.z+((o.flags&Bridge)?o.height/2:o.height);
        if(o.flags&Impassable){if(o.z<a.z+16&&o.z+std::max(1,o.height)>a.z)return false;}
        else if((o.flags&Surface)&&top==a.z)support=true;
    }
    return support;
}
void Npcs::reset(const World&w,const Actor&player){
    people.clear();random=12345;
    const int xs[]={1493,1500,1499},ys[]={1616,1612,1619};
    const char*names[]={"POSADERO","VIAJERO AZUL","VIAJERO VERDE"};
    const char*words[]={"POSADERO: BIENVENIDO A LA POSADA","VIAJERO: BUEN VIAJE","VIAJERO: BRITAIN TE DA LA BIENVENIDA"};
    for(int i=0;i<3;++i){
        Actor home{xs[i]-w.originX,ys[i]-w.originY,20},spot=home;bool found=false;
        for(int r=0;r<=3&&!found;++r)for(int y=-r;y<=r&&!found;++y)for(int x=-r;x<=r&&!found;++x){
            Actor test{home.x+x,home.y+y,20};
            if(canStand(w,test)&&!same(test,player)&&!occupied(test)){spot=test;found=true;}
        }
        if(found){Npc n;n.position=n.from=n.home=spot;n.style=i;n.name=names[i];n.greeting=words[i];n.wait=1+i*.7f;people.push_back(n);}
    }
}
bool Npcs::occupied(const Actor&a,int except)const{
    for(size_t i=0;i<people.size();++i)if((int)i!=except&&(same(a,people[i].position)||(people[i].moving&&same(a,people[i].from))))return true;
    return false;
}
void Npcs::addOccupants(std::vector<Actor>&out)const{
    for(const auto&n:people){out.push_back(n.position);if(n.moving)out.push_back(n.from);}
}
void Npcs::update(const World&w,const Actor&player,const Actor&from,bool playerMoving,float dt,Doors&doors){
    const int dx[]={0,1,1,1,0,-1,-1,-1},dy[]={-1,-1,0,1,1,1,0,-1};
    for(size_t i=0;i<people.size();++i){auto&n=people[i];
        if(n.moving){
            n.progress=std::min(1.f,n.progress+dt/.45f);n.animation=std::fmod(n.animation+dt*8,4.f);
            if(n.progress>=1){n.moving=false;n.wait=1.0f+(random%20)/10.f;}
            continue;
        }
        int px=player.x-n.position.x,py=player.y-n.position.y;
        if(std::abs(player.z-n.position.z)<16&&std::max(std::abs(px),std::abs(py))<=2){
            int x=(px>0)-(px<0),y=(py>0)-(py<0);
            for(int d=0;d<8;++d)if(dx[d]==x&&dy[d]==y)n.direction=(d+1)&7;
            n.wait=1;continue;
        }
        n.wait-=dt;if(n.wait>0)continue;
        random=random*1664525u+1013904223u;int d=(random>>16)&7;n.wait=.5f;
        Actor next;int actual;
        if(doors.tryOpen(n.position,dx[d],dy[d]))continue;
        if(!step(w,n.position,d,next,actual)||next.z!=n.home.z||std::abs(next.x-n.home.x)>3||std::abs(next.y-n.home.y)>3||
           same(next,player)||(playerMoving&&same(next,from))||occupied(next,(int)i))continue;
        if(actual&1){
            Actor s1{n.position.x+dx[actual],n.position.y,n.position.z},s2{n.position.x,n.position.y+dy[actual],n.position.z};
            if(occupied(s1,(int)i)||occupied(s2,(int)i)||same(s1,player)||same(s2,player))continue;
        }
        n.from=n.position;n.position=next;n.progress=0;n.moving=true;n.direction=(actual+1)&7;
    }
}
std::string Npcs::greeting(const Actor&a)const{
    for(const auto&n:people)if(std::abs(n.position.z-a.z)<16&&std::abs(n.position.x-a.x)<=2&&std::abs(n.position.y-a.y)<=2)return n.greeting;
    return "";
}
bool drawBefore(const Draw&a,const Draw&b){
    if(a.x+a.y!=b.x+b.y)return a.x+a.y<b.x+b.y;
    if(a.x!=b.x)return a.x<b.x;
    if(a.priority!=b.priority)return a.priority<b.priority;
    if((a.kind==Kind::Land)!=(b.kind==Kind::Land))return a.kind==Kind::Land;
    return a.order<b.order;
}
std::vector<Draw> Npcs::drawList(const World&w,const Actor&a,bool cutaway)const{
    std::vector<Draw>out;int ceiling=128;
    if(cutaway&&w.contains(a.x,a.y))for(size_t i:w.cells[a.y*w.width+a.x]){
        const auto&o=w.objects[i];if(o.z>a.z+14&&(o.flags&(Roof|Surface)))ceiling=std::min(ceiling,o.z);
    }
    for(size_t i=0;i<people.size();++i){const auto&n=people[i];const auto&p=n.position;
        if(std::abs(p.x-a.x)>12||std::abs(p.y-a.y)>12||(cutaway&&p.z>=ceiling))continue;
        out.push_back({Kind::Npc,p.x,p.y,p.z,p.z+1,i,0xffffff00u+(uint32_t)i});
    }
    std::stable_sort(out.begin(),out.end(),drawBefore);return out;
}
}
