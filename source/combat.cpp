// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <cerrno>
namespace lab {
namespace {
uint32_t hash(const uint8_t*p,size_t n){uint32_t h=2166136261;while(n--)h=(h^*p++)*16777619;return h;}
uint32_t get(const uint8_t*p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
void put(uint8_t*p,uint32_t n){for(int i=0;i<4;++i)p[i]=uint8_t(n>>(8*i));}
int distance(const Actor&a,const Actor&b){return std::max(std::abs(a.x-b.x),std::abs(a.y-b.y));}
int maximum(const Npc&n){return n.name.find("dog")!=std::string::npos?30:n.name.find("cat")!=std::string::npos?20:n.name.find("rat")!=std::string::npos?18:12;}
}
bool meleeReach(const World&w,const Actor&a,const Actor&b){
    if(std::abs(a.z-b.z)>8||distance(a,b)>1||!w.contains(a.x,a.y)||!w.contains(b.x,b.y))return false;
    if(a.x==b.x&&a.y==b.y)return true;
    const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};
    for(int d=0;d<8;++d)if(a.x+xs[d]==b.x&&a.y+ys[d]==b.y){Actor next;int actual;return step(w,a,d,next,actual)&&next.x==b.x&&next.y==b.y&&std::abs(next.z-b.z)<=8;}
    return false;
}
int Combat::roll(int lo,int hi){random=random*1664525u+1013904223u;return lo+int((random>>16)%unsigned(hi-lo+1));}
void Combat::reset(const Npcs&npcs){
    mob.clear();for(const auto&n:npcs.all()){MobVitals v;v.hits=v.maximum=maximum(n);mob.push_back(v);}
    for(auto&c:corpses)c=Corpse{};
    target=-1;attacking=false;swing=regen=0;message.clear();
}
void Combat::select(const Npcs&npcs,const Actor&a,bool cycle){
    std::vector<int>choices;for(size_t i=0;i<npcs.all().size();++i){const auto&n=npcs.all()[i];if(attackable(n)&&distance(n.position,a)<=8&&std::abs(n.position.z-a.z)<=8)choices.push_back(int(i));}
    if(choices.empty()){target=-1;attacking=false;message="NO HAY CRIATURAS CERCANAS";return;}
    std::stable_sort(choices.begin(),choices.end(),[&](int l,int r){return distance(npcs.all()[l].position,a)<distance(npcs.all()[r].position,a);});
    auto found=std::find(choices.begin(),choices.end(),target);
    target=cycle&&found!=choices.end()?choices[(size_t(found-choices.begin())+1)%choices.size()]:choices[0];
    message="OBJETIVO: "+npcs.all()[target].name;
}
void Combat::toggle(const Npcs&npcs,const Actor&a){
    if(attacking){attacking=false;message="ATAQUE DETENIDO";return;}
    if(target<0||target>=int(npcs.all().size())||!attackable(npcs.all()[target])||distance(a,npcs.all()[target].position)>8)select(npcs,a);
    if(target>=0){attacking=true;swing=0;message="ACERCATE AL OBJETIVO | A: PARAR";}
}
void Combat::kill(size_t index,Npcs&npcs,Inventory&inv){
    auto&n=npcs.combatants()[index];n.alive=false;n.hostile=false;n.moving=false;n.progress=1;n.from=n.position;
    auto&v=mob[index];v.hits=0;v.respawn=20;
    auto slot=std::find_if(corpses.begin(),corpses.end(),[](const Corpse&c){return !c.active;});
    if(slot==corpses.end())slot=std::min_element(corpses.begin(),corpses.end(),[](const Corpse&a,const Corpse&b){return a.decay<b.decay;});
    inv.closeExternal(&slot->loot);*slot=Corpse{};slot->active=true;slot->position=n.position;slot->style=n.style;slot->direction=n.direction;slot->decay=90;slot->name=n.name;
    // Finite starter supplies using the existing eight-item inventory catalogue.
    slot->loot={{0,uint16_t(roll(2,8))},{3,uint16_t(roll(1,2))}};
    if(roll(0,2)==0)slot->loot.push_back({uint16_t(roll(1,2)),1});
    message="CRIATURA MUERTA | MOCHILA: ABRIR CADAVER";
    if(target==int(index)){target=-1;attacking=false;}
}
bool Combat::update(const World&w,Npcs&npcs,Actor&player,bool moving,float dt,PlayerStatus&status,Inventory&inv){
    if(mob.size()!=npcs.all().size())reset(npcs);
    if(dt<=0)return false;
    status.healthVisible=std::max(0.f,status.healthVisible-dt);status.flash=std::max(0.f,status.flash-dt);swing=std::max(0.f,swing-dt);
    for(auto&c:corpses)if(c.active){c.decay-=dt;if(c.decay<=0){inv.closeExternal(&c.loot);c.active=false;c.loot.clear();}}
    for(size_t i=0;i<mob.size();++i){auto&v=mob[i];auto&n=npcs.combatants()[i];v.flash=std::max(0.f,v.flash-dt);v.cooldown=std::max(0.f,v.cooldown-dt);
        if(!n.alive){v.respawn-=dt;if(v.respawn>0)continue;bool placed=false;
            for(int radius=0;radius<=3&&!placed;++radius){for(int y=-radius;y<=radius&&!placed;++y){for(int x=-radius;x<=radius&&!placed;++x){
                Actor a{n.home.x+x,n.home.y+y,n.home.z};
                if(canStand(w,a)&&!npcs.occupied(a)&&!(distance(a,player)==0&&std::abs(a.z-player.z)<16)){n.position=n.from=a;n.alive=true;n.hostile=false;n.moving=false;n.progress=1;n.wait=1;n.namePresence={};v.hits=v.maximum;v.cooldown=1;placed=true;}
            }}}
            if(!placed)v.respawn=1;
        }
    }
    if(target>=0&&(target>=int(mob.size())||!attackable(npcs.all()[target])||distance(player,npcs.all()[target].position)>10)){target=-1;attacking=false;}
    if(attacking&&target>=0&&!moving&&!npcs.all()[target].moving&&swing<=0&&meleeReach(w,player,npcs.all()[target].position)){
        swing=.85f;
        if(status.stamina>0){auto&n=npcs.combatants()[target];auto&v=mob[target];n.hostile=true;--status.stamina;
            v.hits=std::max(0,v.hits-roll(status.damageMin(&inv),status.damageMax(&inv)));v.flash=.15f;
            status.healthVisible=6;
            if(v.hits==0)kill(size_t(target),npcs,inv);
        }
    }
    for(size_t i=0;i<mob.size();++i){auto&n=npcs.combatants()[i];auto&v=mob[i];
        if(!n.alive||!n.animal||!n.hostile||n.moving||moving||v.cooldown>0||!meleeReach(w,n.position,player))continue;
        v.cooldown=1.25f;int damage=roll(n.name.find("dog")!=std::string::npos?4:2,n.name.find("dog")!=std::string::npos?7:4);
        damage=std::max(1,damage*(100-std::max(0,std::min(70,status.resistances[0])))/100);
        status.hits=std::max(0,status.hits-damage);status.healthVisible=6;status.flash=.15f;message="RECIBES "+std::to_string(damage)+" DE DANO";
        if(status.hits==0){
            status.hits=status.hitsMax;status.mana=status.manaMax;status.stamina=status.staminaMax;status.healthVisible=6;
            for(auto&other:npcs.combatants())other.hostile=false;
            attacking=false;target=-1;inv.closeContainer();player={w.startX,w.startY,w.startZ};regen=0;message="HAS MUERTO | REGRESAS AL PUNTO INICIAL";return true;
        }
    }
    regen+=dt;if(regen>=1){regen-=1;status.mana=std::min(status.manaMax,status.mana+1);status.stamina=std::min(status.staminaMax,status.stamina+1);if(status.healthVisible<=0)status.hits=std::min(status.hitsMax,status.hits+1);}
    return false;
}
int Combat::nearestCorpse(const World&w,const Actor&a)const{int best=-1;for(size_t i=0;i<corpses.size();++i)if(corpses[i].active&&meleeReach(w,a,corpses[i].position)&&(best<0||distance(a,corpses[i].position)<distance(a,corpses[best].position)||(distance(a,corpses[i].position)==distance(a,corpses[best].position)&&((corpses[best].loot.empty()&&!corpses[i].loot.empty())||(corpses[best].loot.empty()==corpses[i].loot.empty()&&corpses[i].decay>corpses[best].decay)))))best=int(i);return best;}
bool Combat::openCorpse(const World&w,const Actor&a,Inventory&inv){int i=nearestCorpse(w,a);return i>=0&&inv.openExternal(w,a,corpses[i].loot,corpses[i].position,"CADAVER");}
bool StatusArt::load(const std::string&path){
    std::ifstream f(path,std::ios::binary);std::vector<uint8_t>b(16+300*82*4);rgba.clear();
    if(!f.read((char*)b.data(),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UST1",4)||b[4]!=44||b[5]!=1||b[6]!=82||b[7]||get(b.data()+8)!=300*82*4||get(b.data()+12)!=hash(b.data()+16,b.size()-16))return false;
    rgba.assign(b.begin()+16,b.end());return true;
}
bool loadVitals(const std::string&path,PlayerStatus&status){
    std::ifstream f(path,std::ios::binary);if(!f){f.open(path+".bak",std::ios::binary);if(!f)return true;}
    uint8_t b[20];if(!f.read((char*)b,20)||f.peek()!=EOF||std::memcmp(b,"UVT1",4)||get(b+16)!=hash(b,16))return false;
    unsigned h=get(b+4),m=get(b+8),s=get(b+12);if(h==0||h>unsigned(status.hitsMax)||m>unsigned(status.manaMax)||s>unsigned(status.staminaMax))return false;
    status.hits=h;status.mana=m;status.stamina=s;return true;
}
bool saveVitals(const std::string&path,const PlayerStatus&status,SaveRename renameFile){
    uint8_t b[20]={'U','V','T','1'};put(b+4,status.hits);put(b+8,status.mana);put(b+12,status.stamina);put(b+16,hash(b,16));
    std::string tmp=path+".tmp",bak=path+".bak";FILE*f=std::fopen(tmp.c_str(),"wb");if(!f)return false;
    bool ok=std::fwrite(b,1,20,f)==20;if(std::fclose(f))ok=false;if(!ok){std::remove(tmp.c_str());return false;}
    bool had=std::ifstream(path).good();if(had&&((std::remove(bak.c_str())&&errno!=ENOENT)||renameFile(path.c_str(),bak.c_str()))){std::remove(tmp.c_str());return false;}
    if(renameFile(tmp.c_str(),path.c_str())){if(had)renameFile(bak.c_str(),path.c_str());std::remove(tmp.c_str());return false;}return true;
}
}
