// SPDX-License-Identifier: MIT
#include "quests.hpp"
#include <cctype>
namespace lab {
namespace {
std::string lower(std::string s){for(char&c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;}
bool human(const Npc&n){return n.alive&&!n.animal;}
bool add(Bag&bag,unsigned type,unsigned count){
    auto at=std::find_if(bag.begin(),bag.end(),[&](ItemStack s){return s.type==type;});
    if(at!=bag.end()){if(unsigned(at->count)+count>65535)return false;at->count+=count;return true;}
    if(bag.size()>=BagSlots)return false;
    bag.push_back({uint16_t(type),uint16_t(count)});return true;
}
void remove(Bag&bag,unsigned type,unsigned count){for(auto at=bag.begin();at!=bag.end()&&count;){if(at->type!=type){++at;continue;}unsigned take=std::min(count,unsigned(at->count));at->count-=take;count-=take;if(!at->count)at=bag.erase(at);else ++at;}}
}
uint32_t Quests::signature(const Npc&n){uint32_t h=2166136261u;for(unsigned char c:n.name+"|"+n.title)h=(h^c)*16777619u;return h?h:1;}
bool Quests::cemetery(const World&w,const Npc&n){
    int x=w.originX+n.position.x,y=w.originY+n.position.y;
    auto name=lower(n.name);
    bool undead=name.find("skeleton")!=std::string::npos||name.find("zombie")!=std::string::npos||name.find("wraith")!=std::string::npos||name.find("spectre")!=std::string::npos||name.find("ghoul")!=std::string::npos;
    return n.animal&&undead&&x>=1330&&x<=1410&&y>=1425&&y<=1520;
}
unsigned Quests::owned(const Inventory&bag,unsigned type){unsigned total=0;for(auto s:bag.backpack)if(s.type==type)total+=s.count;return total;}
void Quests::setup(const World&w,const Npcs&npcs){
    offers={{-1,-1}};selected=0;message.clear();topic=0;
    for(unsigned q=0;q<QuestCount;++q){int best=2147483647;
        if(q==0&&!w.contains(1378-w.originX,1488-w.originY))continue;
        for(size_t i=0;i<npcs.all().size();++i){const auto&n=npcs.all()[i];if(!human(n))continue;auto title=lower(n.title);
            bool preferred=q==0?title.find("healer")!=std::string::npos:title.find("provisioner")!=std::string::npos;
            if(!preferred)continue;
            int dx=n.home.x-w.startX,dy=n.home.y-w.startY,score=dx*dx+dy*dy;
            if(score<best){best=score;offers[q]=int(i);}
        }
    }
}
int Quests::giver(unsigned id,const Npcs&npcs,const Inventory&bag)const{
    if(id>=QuestCount)return -1;
    const auto&q=bag.quests[id];int index=q.state?int(q.giver)-1:offers[id];
    if(index>=0&&size_t(index)<npcs.all().size()){const auto&n=npcs.all()[index];if(!n.animal&&(!q.state||signature(n)==q.signature))return index;}
    // A future population reorder may move an existing contact to another
    // index. Rebind only an unambiguous identity, never reset a paid quest.
    if(q.state){int match=-1;for(size_t i=0;i<npcs.all().size();++i)if(!npcs.all()[i].animal&&signature(npcs.all()[i])==q.signature){if(match>=0)return -1;match=int(i);}return match;}
    return -1;
}
int Quests::nearby(const World&w,const Npcs&npcs,const Actor&a,const Inventory&bag)const{
    int preferred=giver(selected,npcs,bag);
    auto close=[&](const Npc&n){return human(n)&&std::max(std::abs(n.position.x-a.x),std::abs(n.position.y-a.y))<=1&&meleeReach(w,a,n.position);};
    if(preferred>=0&&close(npcs.all()[preferred]))return preferred;
    int nearest=-1,best=10;for(size_t i=0;i<npcs.all().size();++i){const auto&n=npcs.all()[i];int dx=n.position.x-a.x,dy=n.position.y-a.y,d=dx*dx+dy*dy;
        if(d<best&&close(n)){best=d;nearest=int(i);}}
    return nearest;
}
bool Quests::ready(unsigned id,const Inventory&bag)const{return id<QuestCount&&bag.quests[id].state==1&&(id==0?bag.quests[id].count>=5:owned(bag,1)>=3&&owned(bag,3)>=5);}
bool Quests::interact(const World&w,const Npcs&npcs,const Actor&a,Inventory&bag,SaveRename renameFile){
    int npc=nearby(w,npcs,a,bag);if(npc<0){message="ACERCATE AL NPC PARA HABLAR";return false;}
    int expected=giver(selected,npcs,bag);
    if(npc!=expected){++topic;message="CONVERSACION CON "+npcs.all()[npc].name;return false;}
    auto q=bag.quests[selected];
    if(q.state==2){message="GRACIAS DE NUEVO POR TU AYUDA";return false;}
    if(!q.state){q={1,0,uint32_t(npc+1),signature(npcs.all()[npc])};if(!bag.commitQuest(selected,q,bag.backpack,renameFile)){message=bag.message;return false;}message="MISION ACEPTADA";return true;}
    if(!ready(selected,bag)){message=selected==0?"EL CEMENTERIO AUN NO ESTA A SALVO":"NECESITAS 3 PANES Y 5 VENDAS";return false;}
    Bag items=bag.backpack;
    if(selected==1){remove(items,1,3);remove(items,3,5);}
    if(!add(items,0,selected==0?150:80)||(selected==0&&!add(items,3,5))){message="LIBERA ESPACIO O GUARDA ORO";return false;}
    q.state=2;
    if(!bag.commitQuest(selected,q,std::move(items),renameFile)){message=bag.message;return false;}
    message=selected==0?"PREMIO: $150 Y 5 VENDAS":"PREMIO: $80 | SUMINISTROS ENTREGADOS";return true;
}
bool Quests::killed(const World&w,const Npc&n,Inventory&bag,SaveRename renameFile){
    auto q=bag.quests[0];if(q.state!=1||q.count>=5||!cemetery(w,n))return false;
    ++q.count;if(!bag.commitQuest(0,q,bag.backpack,renameFile)){message=bag.message;return false;}
    message=q.count==5?"CEMENTERIO LIMPIO: VUELVE POR TU PREMIO":"CEMENTERIO: "+std::to_string(q.count)+" / 5";return true;
}
QuestMarker Quests::marker(const Npcs&npcs,const Inventory&bag)const{
    for(unsigned step=0;step<QuestCount;++step){unsigned id=(selected+step)%QuestCount;if(!ready(id,bag))continue;int index=giver(id,npcs,bag);if(index<0||!human(npcs.all()[index]))continue;
        const auto&p=npcs.all()[index].position;return {true,p.x,p.y,p.z};}
    return {};
}
QuestPanel Quests::panel(const World&w,const Npcs&npcs,const Actor&a,const Inventory&bag)const{
    QuestPanel p;p.selected=selected;p.heading=selected==0?"LIMPIAR EL CEMENTERIO":"SUMINISTROS PARA BRITAIN";p.ready=ready(selected,bag);p.reward=selected==0?"PREMIO: $150 + 5 VENDAS":"PREMIO: $80";
    const auto&q=bag.quests[selected];int contact=giver(selected,npcs,bag),near=nearby(w,npcs,a,bag);
    p.speaker=near>=0?npcs.all()[near].name:contact>=0?npcs.all()[contact].name:"SIN CONTACTO EN ESTE MAPA";
    if(near>=0)p.profession=npcs.all()[near].title;else if(contact>=0){const auto&n=npcs.all()[contact];p.profession="BUSCA EN "+std::to_string(n.position.x+w.originX)+", "+std::to_string(n.position.y+w.originY);}
    if(near>=0&&near!=contact){
        static const std::array<std::array<const char*,3>,3>words{{{{"Saludos, viajero.","Britain necesita gente valiente.","Nuestros sanadores buscan ayuda."}},{{"En el cementerio no hay descanso.","Se ven muertos entre las tumbas.","Lleva vendas antes de aventurarte."}},{{"Los proveedores necesitan pan","y vendas para los viajeros.","Habla con el contacto de tu mision."}}}};
        for(unsigned i=0;i<3;++i)p.dialogue[i]=words[topic%3][i];
        p.button="HABLAR: OTRO TEMA";
    }else{
        p.dialogue=selected==0?std::array<std::string,3>{{"Los muertos amenazan Britain.","Derrota a 5 no muertos dentro","del cementerio y regresa aqui."}}:std::array<std::string,3>{{"Nos quedan pocos suministros.","Trae 3 panes y 5 vendas de tu","mochila. Te pagare al entregarlos."}};
        p.button=near<0?"ACERCATE PARA HABLAR":q.state==2?"HABLAR":!q.state?"ACEPTAR MISION":p.ready?"COBRAR RECOMPENSA":"PREGUNTAR POR LA MISION";
    }
    if(q.state==2)p.objective="COMPLETADA | PREMIO COBRADO";
    else if(!q.state)p.objective="MISION SIN ACEPTAR";
    else if(selected==0)p.objective="NO MUERTOS: "+std::to_string(q.count)+" / 5";
    else p.objective="PAN "+std::to_string(std::min(3u,owned(bag,1)))+"/3 | VENDAS "+std::to_string(std::min(5u,owned(bag,3)))+"/5";
    p.message=message.empty()?(p.ready?"SIGUE LA FLECHA PARA VOLVER":selected==0?"CEMENTERIO: 1378, 1488":"CONSIGUE SUMINISTROS EN COFRES"):message;
    return p;
}
}
