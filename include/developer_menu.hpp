// SPDX-License-Identifier: MIT
#pragma once
#include "npcs.hpp"
#include <algorithm>
#include <cctype>
namespace lab {
struct DebugDestination {std::string name;Actor target;bool available=false;};
inline bool safeTeleportTile(const World&w,const Npcs&npcs,const Actor&target,Actor&result){
    if(!w.contains(target.x,target.y))return false;
    for(int radius=0;radius<=12;++radius){bool found=false;int best=100000;Actor chosen;
        for(int y=-radius;y<=radius;++y)for(int x=-radius;x<=radius;++x){if(std::max(std::abs(x),std::abs(y))!=radius)continue;int px=target.x+x,py=target.y+y;if(!w.contains(px,py))continue;
            auto consider=[&](int z){if(std::abs(z-target.z)>8)return;Actor at{px,py,z};if(!canStand(w,at)||npcs.occupied(at))return;for(const auto&n:npcs.all())if(n.alive&&n.position.x==at.x&&n.position.y==at.y&&std::abs(n.position.z-at.z)<16)return;int score=(x*x+y*y)*32+std::abs(z-target.z);if(score<best){best=score;chosen=at;found=true;}};
            consider(target.z);consider(heights(w,px,py).average);
            for(auto i:w.cells[py*w.width+px]){const auto&o=w.objects[i];if(o.flags&(Surface|Bridge))consider(o.z+((o.flags&Bridge)?o.height/2:o.height));}
        }
        if(found){result=chosen;return true;}
    }return false;
}
struct DeveloperMenu {
    std::vector<DebugDestination>destinations;int selected=0;std::string message;
    DeveloperMenu(const World&w,const Npcs&npcs){
        auto fixed=[&](const char*name,int x,int y,int z){Actor target{x-w.originX,y-w.originY,z},safe;destinations.push_back({name,target,safeTeleportTile(w,npcs,target,safe)});};
        destinations.push_back({"PUNTO DE INICIO",{w.startX,w.startY,w.startZ},false});Actor safe;destinations.back().available=safeTeleportTile(w,npcs,destinations.back().target,safe);
        auto vendor=[&](const char*label,const std::vector<std::string>&titles){int best=100000000;const Npc*chosen=nullptr;for(const auto&n:npcs.all())if(n.alive&&!n.animal){std::string title=n.title;std::transform(title.begin(),title.end(),title.begin(),[](unsigned char c){return char(std::tolower(c));});bool matches=false;for(const auto&t:titles)if(title.find(t)!=std::string::npos)matches=true;if(!matches)continue;int dx=n.position.x-w.startX,dy=n.position.y-w.startY,score=dx*dx+dy*dy;if(score<best){best=score;chosen=&n;}}
            Actor target=chosen?chosen->position:Actor{},landing;destinations.push_back({label,target,chosen&&safeTeleportTile(w,npcs,target,landing)});};
        vendor("HERRERO / ARMAS",{"smith","armorer"});vendor("PROVEEDOR",{"provisioner"});vendor("SASTRE",{"tailor"});vendor("JOYAS",{"jewel","provisioner"});vendor("SANADOR",{"healer"});
        fixed("CEMENTERIO",1378,1488,10);fixed("BOSQUE OESTE / ENEMIGOS",1240,1463,0);fixed("BOSQUE NORTE / ENEMIGOS",1377,1381,0);fixed("PUERTO DE BRITAIN",1485,1762,0);
        if(w.width>512||w.height>512)for(bool hostile:{true,false}){
            const Npc*chosen=nullptr;int best=2147483647;
            for(const auto&n:npcs.all()){int x=n.position.x+w.originX,y=n.position.y+w.originY;if((x>=1216&&x<1728&&y>=1344&&y<1856)||n.waterOnly||!n.alive)continue;
                if(hostile?!n.aggressive:n.animal)continue;
                int dx=n.position.x-w.startX,dy=n.position.y-w.startY,score=dx*dx+dy*dy;
                if(score<best){best=score;chosen=&n;}}
            Actor target=chosen?chosen->position:Actor{},landing;destinations.push_back({hostile?"NUEVA ZONA / ENEMIGOS":"NUEVA ZONA / POBLADORES",target,chosen&&safeTeleportTile(w,npcs,target,landing)});
        }
    }
    int page()const{return selected/5;}
    void move(int delta){selected=(selected+delta+int(destinations.size()))%int(destinations.size());message.clear();}
    void pageMove(){selected=((page()+1)%int((destinations.size()+4)/5))*5;}
    int tap(int x,int y){if(y>=215&&y<235){if(x>=18&&x<150)return -1;if(x>=170&&x<303)return 1;}if(y>=27&&y<44&&x>=245){pageMove();return 0;}if(x>=18&&x<303&&y>=49&&y<189){int row=(y-49)/28,index=page()*5+row;if(index<int(destinations.size())){selected=index;message.clear();}}return 0;}
    bool resolve(const World&w,const Npcs&npcs,Actor&landing){const auto&d=destinations[selected];if(!d.available||!safeTeleportTile(w,npcs,d.target,landing)){message="DESTINO NO DISPONIBLE";return false;}return true;}
};
}
