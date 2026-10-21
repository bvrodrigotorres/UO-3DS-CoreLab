// SPDX-License-Identifier: MIT
#include "npcs.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstring>
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
bool Npcs::loadPopulation(const std::string&path,const std::string&scene,const World&w,std::string&error){
    auto read=[](const std::string&p,std::vector<unsigned char>&b,size_t limit){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||f.tellg()>std::streamoff(limit))return false;b.resize(size_t(f.tellg()));f.seekg(0);return bool(f.read((char*)b.data(),b.size()));};
    auto hash=[](const unsigned char*p,size_t count){uint32_t h=2166136261;for(size_t i=0;i<count;++i)h=(h^p[i])*16777619;return h;};
    std::vector<unsigned char>b,raw;
    if(!read(path,b,65536)||b.size()<16||std::memcmp(b.data(),"UNP1",4)){error="POBLACION: ARCHIVO INVALIDO";return false;}
    auto n=[&](size_t p,int count){uint32_t value=0;for(int i=0;i<count;++i)value|=uint32_t(b[p+i])<<(8*i);return value;};
    unsigned count=n(4,2),styles=n(6,2);
    if(!count||count>1024||!styles||styles>64||b.size()!=16+styles*16+count*44||hash(b.data()+16,b.size()-16)!=n(12,4)||!read(scene,raw,8*1024*1024)||hash(raw.data(),raw.size())!=n(8,4)){error="POBLACION: MAPA O CHECKSUM INVALIDO";return false;}
    std::vector<NpcStyle>nextArt;std::vector<Npc>next;
    for(unsigned i=0;i<styles;++i){size_t at=16+i*16;NpcStyle a;a.pivotX=int16_t(n(at,2));a.pivotY=int16_t(n(at+2,2));a.width=n(at+4,2);a.height=n(at+6,2);a.bytes=n(at+8,4);a.checksum=n(at+12,4);
        if(!a.width||!a.height||a.width>192||a.height>192||std::abs(a.pivotX)>256||std::abs(a.pivotY)>256||!a.bytes||a.bytes>2*1024*1024){error="POBLACION: ATLAS INVALIDO";return false;}nextArt.push_back(a);}
    for(unsigned i=0;i<count;++i){size_t at=16+styles*16+i*44;Npc npc;npc.position={int(n(at,2)),int(n(at+2,2)),int(int16_t(n(at+4,2)))};npc.home=npc.from=npc.position;npc.style=n(at+6,2);npc.roam=n(at+8,2);npc.animal=n(at+10,2)!=0;
        if(npc.style>=int(styles)||npc.roam>120||n(at+10,2)>1||!canStand(w,npc.position)||b[at+43]){error="POBLACION: POSICION INVALIDA";return false;}
        npc.name=std::string((const char*)b.data()+at+12);npc.greeting=npc.animal?"":npc.name+": SALUDOS, VIAJERO";npc.wait=1+(i%10)*.15f;next.push_back(npc);}
    art=std::move(nextArt);original=std::move(next);error.clear();return true;
}
void Npcs::reset(const World&w,const Actor&player){
    people.clear();random=12345;
    if(hasPopulation()){
        for(auto n:original){
            if(!same(n.position,player)&&!occupied(n.position)){people.push_back(n);continue;}
            bool found=false;
            for(int r=1;r<=3&&!found;++r)for(int y=-r;y<=r&&!found;++y)for(int x=-r;x<=r&&!found;++x){Actor a{n.home.x+x,n.home.y+y,n.home.z};if(canStand(w,a)&&!same(a,player)&&!occupied(a)){n.position=n.from=n.home=a;people.push_back(n);found=true;}}
            if(!found)people.push_back(n); // preserve the original count; let the player move clear
        }
        return;
    }
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
        if(std::max(std::abs(n.position.x-player.x),std::abs(n.position.y-player.y))>24)continue;
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
        if(!n.roam)continue;
        if(!n.animal&&doors.tryOpen(n.position,dx[d],dy[d]))continue;
        if(!step(w,n.position,d,next,actual)||(!n.animal&&next.z!=n.home.z)||std::abs(next.x-n.home.x)>n.roam||std::abs(next.y-n.home.y)>n.roam||
           same(next,player)||(playerMoving&&same(next,from))||occupied(next,(int)i))continue;
        if(actual&1){
            Actor s1{n.position.x+dx[actual],n.position.y,n.position.z},s2{n.position.x,n.position.y+dy[actual],n.position.z};
            if(occupied(s1,(int)i)||occupied(s2,(int)i)||same(s1,player)||same(s2,player))continue;
        }
        n.from=n.position;n.position=next;n.progress=0;n.moving=true;n.direction=(actual+1)&7;
    }
}
std::string Npcs::greeting(const Actor&a)const{
    for(const auto&n:people)if(!n.animal&&std::abs(n.position.z-a.z)<16&&std::abs(n.position.x-a.x)<=2&&std::abs(n.position.y-a.y)<=2)return n.greeting;
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
