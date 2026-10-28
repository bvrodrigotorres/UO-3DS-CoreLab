// SPDX-License-Identifier: MIT
#include "npcs.hpp"
#include "file_hash.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstring>
namespace lab {
static bool same(const Actor&a,const Actor&b){return a.x==b.x&&a.y==b.y&&std::abs(a.z-b.z)<16;}
void Npcs::importWindowSpawns(const Npcs&source,int dx,int dy){
    art=source.art;deathFrames=source.deathFrames;original=source.original.empty()?source.people:source.original;
    people=original;windowed=true;random=12345;aggressionClock=0;routeCursor=0;
    for(auto&n:people){n.alive=true;n.hostile=false;n.aggressionOrder=0;n.moving=false;n.progress=1;n.from=n.position;n.namePresence={};}
    simulation.assign(people.size(),0);placement.assign(people.size(),1);shiftWindow(dx,dy);
}
void Npcs::shiftWindow(int dx,int dy){
    auto shift=[&](Actor&a){a.x+=dx;a.y+=dy;};
    for(auto&n:people){shift(n.position);shift(n.from);shift(n.home);}
    for(auto&n:original){shift(n.position);shift(n.from);shift(n.home);}
    occupancyDirty=true;simulation.assign(people.size(),0);for(auto&pending:placement)if(pending==2)pending=1;
}
unsigned Npcs::activeCount()const{if(!windowed)return unsigned(people.size());return unsigned(std::count(simulation.begin(),simulation.end(),1));}
void Npcs::prepareWindow(const World&w,const Actor&player){
    if(!windowed)return;
    occupancyDirty=true;simulation.assign(people.size(),0);std::vector<size_t>nearby;nearby.reserve(people.size());
    auto distance=[&](size_t i){const auto&a=people[i].alive?people[i].position:people[i].home;int x=a.x-player.x,y=a.y-player.y;return x*x+y*y;};
    for(size_t i=0;i<people.size();++i){if(placement[i]==2)continue;const auto&a=people[i].alive?people[i].position:people[i].home;
        if(w.contains(a.x,a.y)&&std::max(std::abs(a.x-player.x),std::abs(a.y-player.y))<=24)nearby.push_back(i);
    }
    const auto count=std::min<size_t>(WindowActiveLimit,nearby.size());
    std::partial_sort(nearby.begin(),nearby.begin()+count,nearby.end(),[&](size_t a,size_t b){int da=distance(a),db=distance(b);return da<db||(da==db&&a<b);});
    unsigned placements=0;
    for(size_t j=0;j<count;++j){size_t i=nearby[j];auto&n=people[i];
        if(placement[i]==1){
            if(placements>=4)continue;
            ++placements;
            bool valid=canStandMobile(w,n.position,n.swims,n.waterOnly)&&!same(n.position,player)&&!occupied(n.position);
            for(int radius=1;radius<=3&&!valid;++radius)for(int y=-radius;y<=radius&&!valid;++y)for(int x=-radius;x<=radius&&!valid;++x){
                Actor at{n.home.x+x,n.home.y+y,n.home.z};if(!w.contains(at.x,at.y))continue;
                if(!canStandMobile(w,at,n.swims,n.waterOnly)){at.z=heights(w,at.x,at.y).average;if(std::abs(at.z-n.home.z)>8||!canStandMobile(w,at,n.swims,n.waterOnly))continue;}
                if(!occupied(at)&&!same(at,player)){n.position=n.from=n.home=at;valid=true;occupancyDirty=true;}
            }
            // Invalid dynamic platforms in the campaign must not leave floating
            // sprites in the original map. Retry only after a window change.
            placement[i]=valid?0:2;
        }
        if(placement[i]!=2)simulation[i]=1;
    }
}
bool canStand(const World&w,const Actor&a){
    if(!w.contains(a.x,a.y))return false;
    auto h=heights(w,a.x,a.y);const auto&land=w.land[a.y*w.width+a.x];
    bool support=!(land.flags&(Impassable|Wet))&&land.art!=2&&h.average==a.z;
    for(size_t i:w.cells[a.y*w.width+a.x]){
        const auto&o=w.objects[i];
        int top=o.z+((o.flags&Bridge)?o.height/2:o.height);
        if(o.flags&Impassable){if(o.z<a.z+16&&o.z+std::max(1,int(o.height))>a.z)return false;}
        else if((o.flags&Surface)&&top==a.z)support=true;
    }
    return support;
}
bool canStandMobile(const World&w,const Actor&a,bool swims,bool waterOnly){
    if(!waterOnly&&canStand(w,a))return true;
    if(!swims||!w.contains(a.x,a.y))return false;
    const auto&land=w.land[a.y*w.width+a.x];auto h=heights(w,a.x,a.y);bool support=(land.flags&Wet)&&h.average==a.z;
    for(auto i:w.cells[a.y*w.width+a.x]){const auto&o=w.objects[i];
        if(o.flags&Wet){if(o.z==a.z)support=true;continue;}
        if((o.flags&(Impassable|Wall))&&o.z<a.z+16&&o.z+std::max(1,int(o.height))>a.z)return false;
    }
    return support;
}
bool stepMobile(const World&w,const Actor&a,int direction,Actor&out,int&actual,bool swims,bool waterOnly){
    if(!waterOnly&&step(w,a,direction,out,actual))return true;
    if(!swims)return false;
    const int dx[]={0,1,1,1,0,-1,-1,-1},dy[]={-1,-1,0,1,1,1,0,-1};int d=direction&7,x=a.x+dx[d],y=a.y+dy[d];if(!w.contains(x,y))return false;
    auto valid=[&](int px,int py,Actor&result){if(!w.contains(px,py))return false;Actor next{px,py,heights(w,px,py).average};
        if(std::abs(next.z-a.z)<=8&&canStandMobile(w,next,true,waterOnly)){result=next;return true;}
        for(auto i:w.cells[py*w.width+px]){const auto&o=w.objects[i];next.z=o.z;if((o.flags&Wet)&&std::abs(next.z-a.z)<=8&&canStandMobile(w,next,true,waterOnly)){result=next;return true;}}
        return false;};
    Actor next,side;if(!valid(x,y,next))return false;if((d&1)&&(!valid(x,a.y,side)||!valid(a.x,y,side)))return false;
    out=next;actual=d;return true;
}
bool Npcs::loadPopulation(const std::string&path,const std::string&scene,const World&w,std::string&error){
    auto read=[](const std::string&p,std::vector<unsigned char>&b,size_t limit){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||f.tellg()>std::streamoff(limit))return false;b.resize(size_t(f.tellg()));f.seekg(0);return bool(f.read((char*)b.data(),b.size()));};
    auto hash=[](const unsigned char*p,size_t count){uint32_t h=2166136261;for(size_t i=0;i<count;++i)h=(h^p[i])*16777619;return h;};
    std::vector<unsigned char>b;uint32_t sceneHash=0;
    if(!read(path,b,65536)||b.size()<16||(std::memcmp(b.data(),"UNP1",4)&&std::memcmp(b.data(),"UNP2",4)&&std::memcmp(b.data(),"UNP3",4))){error="POBLACION: ARCHIVO INVALIDO";return false;}
    auto n=[&](size_t p,int count){uint32_t value=0;for(int i=0;i<count;++i)value|=uint32_t(b[p+i])<<(8*i);return value;};
    bool aquatic=b[3]=='3',extended=b[3]!='1';unsigned stride=extended?52:44;
    unsigned count=n(4,2),styles=n(6,2);
    if(!count||count>1024||!styles||styles>64||b.size()!=16+styles*16+count*stride||hash(b.data()+16,b.size()-16)!=n(12,4)||!fileHash(scene,24*1024*1024,sceneHash)||sceneHash!=n(8,4)){error="POBLACION: MAPA O CHECKSUM INVALIDO";return false;}
    std::vector<NpcStyle>nextArt;std::vector<Npc>next;
    for(unsigned i=0;i<styles;++i){size_t at=16+i*16;NpcStyle a;a.pivotX=int16_t(n(at,2));a.pivotY=int16_t(n(at+2,2));a.width=n(at+4,2);a.height=n(at+6,2);a.bytes=n(at+8,4);a.checksum=n(at+12,4);
        if(!a.width||!a.height||a.width>192||a.height>192||std::abs(a.pivotX)>256||std::abs(a.pivotY)>256||!a.bytes||a.bytes>2*1024*1024+4096){error="POBLACION: ATLAS INVALIDO";return false;}nextArt.push_back(a);}
    for(unsigned i=0;i<count;++i){size_t at=16+styles*16+i*stride;Npc npc;npc.position={int(n(at,2)),int(n(at+2,2)),int(int16_t(n(at+4,2)))};npc.home=npc.from=npc.position;npc.style=n(at+6,2);npc.roam=n(at+8,2);npc.animal=(n(at+10,2)&1)!=0;npc.aggressive=extended&&(n(at+10,2)&2);npc.stationary=extended&&(n(at+10,2)&4);npc.swims=aquatic&&(n(at+10,2)&8);npc.waterOnly=aquatic&&(n(at+10,2)&16);
        if((npc.aggressive&&!npc.animal)||(npc.stationary&&!npc.animal)||npc.style>=int(styles)||npc.roam>120||n(at+10,2)>(aquatic?31u:extended?7u:1u)||(npc.waterOnly&&!npc.swims)||!canStandMobile(w,npc.position,npc.swims,npc.waterOnly)||b[at+43]){error="POBLACION: POSICION INVALIDA";return false;}
        if(extended){npc.maximumHits=n(at+44,2);npc.meleeMin=n(at+46,2);npc.meleeMax=n(at+48,2);npc.respawnSeconds=n(at+50,2);if(npc.maximumHits<1||npc.maximumHits>2000||npc.meleeMin<0||npc.meleeMax<npc.meleeMin||npc.meleeMax>100||npc.respawnSeconds<1||npc.respawnSeconds>3600){error="POBLACION: COMBATE INVALIDO";return false;}}
        npc.name=std::string((const char*)b.data()+at+12);npc.greeting=npc.animal?"":npc.name+": SALUDOS, VIAJERO";npc.wait=1+(i%10)*.15f;next.push_back(npc);}
    deathFrames=extended;art=std::move(nextArt);original=std::move(next);error.clear();return true;
}
bool Npcs::loadNames(const std::string&path,const std::string&scene,const std::string&population,std::string&error){
    auto read=[](const std::string&p,std::vector<unsigned char>&b,size_t limit){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||f.tellg()>std::streamoff(limit))return false;b.resize(size_t(f.tellg()));f.seekg(0);return bool(f.read((char*)b.data(),b.size()));};
    auto hash=[](const unsigned char*p,size_t count){uint32_t h=2166136261;for(size_t i=0;i<count;++i)h=(h^p[i])*16777619;return h;};
    std::vector<unsigned char>b,popRaw;uint32_t sceneHash=0;
    auto invalid=[&](){error="NOMBRES: ARCHIVO INVALIDO O AUSENTE";return false;};
    if(!read(path,b,128*1024)||b.size()<20||std::memcmp(b.data(),"UNL1",4))return invalid();
    auto n=[&](size_t p,int count){uint32_t v=0;for(int i=0;i<count;++i)v|=uint32_t(b[p+i])<<(8*i);return v;};
    if(n(4,2)!=original.size()||n(6,2)||b.size()!=20+85*original.size()||hash(b.data()+20,b.size()-20)!=n(16,4)||!fileHash(scene,24*1024*1024,sceneHash)||sceneHash!=n(8,4)||!read(population,popRaw,65536)||hash(popRaw.data(),popRaw.size())!=n(12,4))return invalid();
    auto next=original;
    for(size_t i=0;i<next.size();++i){size_t at=20+85*i;
        if(!b[at]||b[at+31]||b[at+79])return invalid();
        for(size_t j=0;j<80;++j)if(b[at+j]&&(b[at+j]<32||b[at+j]>126))return invalid();
        unsigned hue=n(at+80,2);if(hue!=0x35&&hue!=0x3B2&&hue!=0x59)return invalid();
        next[i].name=(const char*)b.data()+at;next[i].title=(const char*)b.data()+at+32;next[i].nameHue=hue;
        next[i].nameColor={{b[at+82],b[at+83],b[at+84]}};
        next[i].greeting=next[i].animal?"":next[i].name+": SALUDOS, VIAJERO";
    }
    original=std::move(next);error.clear();return true;
}
void Npcs::reset(const World&w,const Actor&player){
    windowed=false;simulation.clear();placement.clear();
    occupancyDirty=true;people.clear();random=12345;aggressionClock=0;routeCursor=0;routeNodes=routeSearches=0;
    if(hasPopulation()){
        for(auto n:original){
            if(!same(n.position,player)&&!occupied(n.position)){people.push_back(n);occupancyDirty=true;continue;}
            bool found=false;
            for(int r=1;r<=3&&!found;++r)for(int y=-r;y<=r&&!found;++y)for(int x=-r;x<=r&&!found;++x){Actor a{n.home.x+x,n.home.y+y,n.home.z};if(canStandMobile(w,a,n.swims,n.waterOnly)&&!same(a,player)&&!occupied(a)){n.position=n.from=n.home=a;people.push_back(n);occupancyDirty=true;found=true;}}
            if(!found){people.push_back(n);occupancyDirty=true;} // preserve the original count; let the player move clear
        }
        if(w.width>512||w.height>512){windowed=true;simulation.assign(people.size(),0);placement.assign(people.size(),0);prepareWindow(w,player);}
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
        if(found){Npc n;n.position=n.from=n.home=spot;n.style=i;n.name=names[i];n.greeting=words[i];n.wait=1+i*.7f;people.push_back(n);occupancyDirty=true;}
    }
}
void Npcs::rebuildOccupancy()const{
    ++occupancyBuilds;std::fill(occupancyHead.begin(),occupancyHead.end(),-1);occupancyNext.assign(people.size()*2,-1);
    for(size_t i=0;i<people.size();++i)if(people[i].alive&&simulates(i)){
        for(int slot=0;slot<(people[i].moving?2:1);++slot){const auto&a=slot?people[i].from:people[i].position;unsigned bin=(unsigned(a.x)*73856093u^unsigned(a.y)*19349663u)&4095;int link=int(i*2+slot);occupancyNext[link]=occupancyHead[bin];occupancyHead[bin]=link;}
    }
    occupancyDirty=false;
}
void Npcs::occupancyMoved(size_t index,const Actor&before,const Actor&from,bool wasMoving){
    if(occupancyDirty)return;
    auto&n=people[index];if(n.position.x==before.x&&n.position.y==before.y&&n.position.z==before.z&&n.moving==wasMoving)return;
    auto remove=[&](const Actor&a,int link){unsigned bin=(unsigned(a.x)*73856093u^unsigned(a.y)*19349663u)&4095;int*current=&occupancyHead[bin];while(*current>=0&&*current!=link)current=&occupancyNext[*current];if(*current==link)*current=occupancyNext[link];};
    remove(before,int(index*2));if(wasMoving)remove(from,int(index*2+1));
    for(int slot=0;slot<(n.moving?2:1);++slot){const auto&a=slot?n.from:n.position;unsigned bin=(unsigned(a.x)*73856093u^unsigned(a.y)*19349663u)&4095;int link=int(index*2+slot);occupancyNext[link]=occupancyHead[bin];occupancyHead[bin]=link;}
}
void Npcs::beginMove(size_t index,const Actor&to,int direction){auto&n=people[index];Actor before=n.position;n.from=before;n.position=to;n.progress=0;n.moving=true;n.direction=(direction+1)&7;occupancyMoved(index,before,before,false);}
bool Npcs::occupied(const Actor&a,int except)const{
    if(!occupancyActive){for(size_t i=0;i<people.size();++i)if(int(i)!=except&&people[i].alive&&simulates(i)&&(same(a,people[i].position)||(people[i].moving&&same(a,people[i].from))))return true;return false;}
    if(occupancyDirty)rebuildOccupancy();
    unsigned bin=(unsigned(a.x)*73856093u^unsigned(a.y)*19349663u)&4095;
    for(int link=occupancyHead[bin];link>=0;link=occupancyNext[link]){int index=link/2;if(index!=except&&same(a,(link&1)?people[index].from:people[index].position))return true;}
    return false;
}
void Npcs::addOccupants(std::vector<Actor>&out)const{
    for(size_t i=0;i<people.size();++i){const auto&n=people[i];if(n.alive&&simulates(i)){out.push_back(n.position);if(n.moving)out.push_back(n.from);}}
}
void Npcs::update(const World&w,const Actor&player,const Actor&from,bool playerMoving,float dt,Doors&doors){
    prepareWindow(w,player);
    routeNodes=routeSearches=occupancyBuilds=0;
    if(dt<=0||people.empty())return;
    struct IndexScope{bool&active;~IndexScope(){active=false;}}scope{occupancyActive};occupancyActive=true;occupancyDirty=true;
    const int dx[]={0,1,1,1,0,-1,-1,-1},dy[]={-1,-1,0,1,1,1,0,-1};
    // Rotate priority so a blocked creature cannot monopolize the shared route budget.
    size_t first=routeCursor++%people.size();
    for(size_t offset=0;offset<people.size();++offset){size_t i=(first+offset)%people.size();auto&n=people[i];if(!n.alive||!simulates(i))continue;
        int px=player.x-n.position.x,py=player.y-n.position.y;
        bool near=std::abs(player.z-n.position.z)<16&&std::max(std::abs(px),std::abs(py))<=2;
        n.namePresence.update(near,dt);
        if(std::max(std::abs(px),std::abs(py))>24)continue;
        if(n.aggressive&&!n.hostile&&std::abs(player.z-n.position.z)<=8&&std::max(std::abs(px),std::abs(py))<=6){engage(i);n.wait=0;}
        if(!n.hostile)n.aggressionOrder=0;
        if(n.moving){
            n.progress=std::min(1.f,n.progress+dt/.45f);n.animation=std::fmod(n.animation+dt*8,4.f);
            if(n.progress>=1){n.moving=false;occupancyMoved(i,n.position,n.from,true);n.wait=n.hostile?0:1.0f+(random%20)/10.f;}
            continue;
        }

        if(n.stationary)continue;
        if(n.hostile){
            if(std::max(std::abs(px),std::abs(py))>8||std::max(std::abs(n.position.x-n.home.x),std::abs(n.position.y-n.home.y))>12){n.hostile=false;n.aggressionOrder=0;}
            else {
                if(n.wait>0){n.wait-=dt;continue;}
                auto canHit=[&](const Actor&a){
                    if(std::abs(a.z-player.z)>8||std::max(std::abs(a.x-player.x),std::abs(a.y-player.y))>1)return false;
                    if(same(a,player))return true;
                    int tx=(player.x>a.x)-(player.x<a.x),ty=(player.y>a.y)-(player.y<a.y);
                    for(int d=0;d<8;++d)if(dx[d]==tx&&dy[d]==ty){Actor next;int actual;return stepMobile(w,a,d,next,actual,n.swims,false)&&next.x==player.x&&next.y==player.y&&std::abs(next.z-player.z)<=8;}
                    return false;
                };
                if(canHit(n.position)){int x=(px>0)-(px<0),y=(py>0)-(py<0);for(int d=0;d<8;++d)if(dx[d]==x&&dy[d]==y)n.direction=(d+1)&7;n.wait=.2f;continue;}
                int aimX=(px>0)-(px<0),aimY=(py>0)-(py<0);bool direct=false;
                for(int d=0;d<8;++d)if(dx[d]==aimX&&dy[d]==aimY){Actor next;int actual;
                    if(stepMobile(w,n.position,d,next,actual,n.swims,n.waterOnly)&&!same(next,player)&&!(playerMoving&&same(next,from))&&!occupied(next,int(i))&&std::abs(next.x-n.home.x)<=12&&std::abs(next.y-n.home.y)<=12&&std::max(std::abs(next.x-player.x),std::abs(next.y-player.y))<std::max(std::abs(px),std::abs(py))){
                        bool clear=true;if(actual&1){Actor s1{n.position.x+dx[actual],n.position.y,n.position.z},s2{n.position.x,n.position.y+dy[actual],n.position.z};clear=!occupied(s1,int(i))&&!occupied(s2,int(i))&&!same(s1,player)&&!same(s2,player);}
                        if(clear){beginMove(i,next,actual);direct=true;}
                    }
                }
                if(direct)continue;
                if(routeSearches>=1){continue;}
                ++routeSearches;
                struct Node{Actor at,first;int direction;};std::vector<Node>queue;queue.reserve(64);queue.push_back({n.position,n.position,0});bool found=false;Node route{};
                for(size_t q=0;q<queue.size()&&!found&&q<64;++q){
                    ++routeNodes;
                    for(int d=0;d<8&&queue.size()<64;++d){Actor next;int actual;
                        if(!stepMobile(w,queue[q].at,d,next,actual,n.swims,n.waterOnly)||same(next,player)||(playerMoving&&same(next,from))||occupied(next,int(i))||std::abs(next.x-n.home.x)>12||std::abs(next.y-n.home.y)>12)continue;
                        if(actual&1){Actor s1{queue[q].at.x+dx[actual],queue[q].at.y,queue[q].at.z},s2{queue[q].at.x,queue[q].at.y+dy[actual],queue[q].at.z};if(occupied(s1,int(i))||occupied(s2,int(i))||same(s1,player)||same(s2,player))continue;}
                        bool seen=false;for(const auto&v:queue)if(v.at.x==next.x&&v.at.y==next.y&&v.at.z==next.z){seen=true;break;}if(seen)continue;
                        Node node{next,q?queue[q].first:next,q?queue[q].direction:actual};queue.push_back(node);
                        if(canHit(next)){route=node;found=true;break;}
                    }
                }
                if(found){beginMove(i,route.first,route.direction);}else n.wait=.75f;
                continue;
            }
        }

        if(near){
            int x=(px>0)-(px<0),y=(py>0)-(py<0);
            for(int d=0;d<8;++d)if(dx[d]==x&&dy[d]==y)n.direction=(d+1)&7;
            n.wait=1;continue;
        }
        n.wait-=dt;if(n.wait>0)continue;
        random=random*1664525u+1013904223u;int d=(random>>16)&7;n.wait=.5f;
        Actor next;int actual;
        if(!n.roam)continue;
        if(!n.animal&&doors.tryOpen(n.position,dx[d],dy[d]))continue;
        if(!stepMobile(w,n.position,d,next,actual,n.swims,n.waterOnly)||(!n.animal&&next.z!=n.home.z)||std::abs(next.x-n.home.x)>n.roam||std::abs(next.y-n.home.y)>n.roam||
           same(next,player)||(playerMoving&&same(next,from))||occupied(next,(int)i))continue;
        if(actual&1){
            Actor s1{n.position.x+dx[actual],n.position.y,n.position.z},s2{n.position.x,n.position.y+dy[actual],n.position.z};
            if(occupied(s1,(int)i)||occupied(s2,(int)i)||same(s1,player)||same(s2,player))continue;
        }
        beginMove(i,next,actual);
    }
}
std::string Npcs::greeting(const Actor&a)const{
    for(size_t i=0;i<people.size();++i){const auto&n=people[i];if(simulates(i)&&n.alive&&!n.animal&&std::abs(n.position.z-a.z)<16&&std::abs(n.position.x-a.x)<=2&&std::abs(n.position.y-a.y)<=2)return n.greeting;}
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
        const auto&o=w.objects[i];if(o.z>a.z+14&&(o.flags&(Roof|Surface)))ceiling=std::min(ceiling,int(o.z));
    }
    for(size_t i=0;i<people.size();++i){const auto&n=people[i];const auto&p=n.position;
        if(!n.alive||!simulates(i))continue;
        if(std::abs(p.x-a.x)>12||std::abs(p.y-a.y)>12||(cutaway&&p.z>=ceiling))continue;
        out.push_back({Kind::Npc,p.x,p.y,p.z,p.z+1,i,0xffffff00u+(uint32_t)i});
    }
    std::stable_sort(out.begin(),out.end(),drawBefore);return out;
}
}
