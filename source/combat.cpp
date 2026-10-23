// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <fstream>
#include <cerrno>
namespace lab {
namespace {
uint32_t hash(const uint8_t*p,size_t n){uint32_t h=2166136261;while(n--)h=(h^*p++)*16777619;return h;}
uint32_t get(const uint8_t*p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
void put(uint8_t*p,uint32_t n){for(int i=0;i<4;++i)p[i]=uint8_t(n>>(8*i));}
int distance(const Actor&a,const Actor&b){return std::max(std::abs(a.x-b.x),std::abs(a.y-b.y));}
int maximum(const Npc&n){if(n.maximumHits>0)return n.maximumHits;return n.name.find("dog")!=std::string::npos?30:n.name.find("cat")!=std::string::npos?20:n.name.find("rat")!=std::string::npos?18:12;}
}
bool meleeReach(const World&w,const Actor&a,const Actor&b){
    if(std::abs(a.z-b.z)>8||distance(a,b)>1||!w.contains(a.x,a.y)||!w.contains(b.x,b.y))return false;
    if(a.x==b.x&&a.y==b.y)return true;
    const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};
    for(int d=0;d<8;++d)if(a.x+xs[d]==b.x&&a.y+ys[d]==b.y){Actor next;int actual;return step(w,a,d,next,actual)&&next.x==b.x&&next.y==b.y&&std::abs(next.z-b.z)<=8;}
    return false;
}
int Combat::roll(int lo,int hi){random=random*1664525u+1013904223u;return lo+int((random>>16)%unsigned(hi-lo+1));}
void Combat::damageVisual(const Actor&position,int amount,bool player,int npc){
    if(amount<=0)return;
    auto slot=std::find_if(damage.begin(),damage.end(),[](const DamageVisual&e){return !e.active;});
    if(slot==damage.end())slot=std::max_element(damage.begin(),damage.end(),[](const DamageVisual&a,const DamageVisual&b){return a.age<b.age;});
    *slot={true,player,position,npc,amount,0,unsigned(roll(0,65535))};
}
void Combat::reset(const Npcs&npcs){
    mob.clear();for(const auto&n:npcs.all()){MobVitals v;v.hits=v.maximum=maximum(n);mob.push_back(v);}
    for(auto&c:corpses)c=Corpse{};
    for(auto&e:damage)e=DamageVisual{};
    for(auto&p:blood)p=BloodPool{};
    target=-1;attacking=false;swing=regen=strike=0;message.clear();
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
    auto pool=std::find_if(blood.begin(),blood.end(),[](const BloodPool&p){return !p.active;});
    if(pool==blood.end())pool=std::max_element(blood.begin(),blood.end(),[](const BloodPool&a,const BloodPool&b){return a.age<b.age;});
    *pool={true,npcs.all()[index].position,0,unsigned(roll(0,6))};
    auto&n=npcs.combatants()[index];n.alive=false;n.hostile=false;n.moving=false;n.progress=1;n.from=n.position;
    auto&v=mob[index];v.hits=0;v.respawn=float(n.respawnSeconds);
    auto slot=std::find_if(corpses.begin(),corpses.end(),[](const Corpse&c){return !c.active;});
    if(slot==corpses.end())slot=std::min_element(corpses.begin(),corpses.end(),[](const Corpse&a,const Corpse&b){return a.decay<b.decay;});
    inv.closeExternal(&slot->loot);*slot=Corpse{};slot->active=true;slot->position=n.position;slot->style=n.style;slot->direction=n.direction;slot->decay=90;slot->name=n.name;
    // Local finite loot: passive fauna do not carry coins or manufactured gear.
    std::string species=n.name;std::transform(species.begin(),species.end(),species.begin(),[](unsigned char c){return char(std::tolower(c));});
    auto is=[&](const char*s){return species.find(s)!=std::string::npos;};
    bool undead=is("skeleton")||is("zombie")||is("wraith")||is("spectre");
    bool armed=is("orc")||is("ratman")||is("lizardman")||is("ogre")||is("ettin")||is("troll");
    if(undead||armed||is("gazer")||is("gargoyle")||is("harpy")||is("mongbat")){
        int tier=n.maximumHits>=150?3:n.maximumHits>=60?2:1;
        slot->loot.push_back({0,uint16_t(roll(8*tier,20*tier))});
        if(armed)slot->loot.push_back({3,1});
        if(is("harpy"))slot->loot.push_back({24,uint16_t(roll(5,12))});
        if(undead){slot->loot.push_back({25,uint16_t(roll(1,3))});slot->loot.push_back({3,uint16_t(roll(1,3))});if(roll(0,3)==0)slot->loot.push_back({4,1});}
        if((armed||undead)&&roll(0,7)==0)slot->loot.push_back({uint16_t(roll(28,29)),1});
        if(armed||is("skeleton")){if(roll(0,2)==0)slot->loot.push_back({uint16_t(tier==1?5:roll(16,18)),1});
            if(roll(0,3)==0)slot->loot.push_back({uint16_t(tier==1?19:tier==2?20:21),1});
            if(roll(0,4)==0)slot->loot.push_back({uint16_t(tier==1?26:27),1});}
    }else if(is("corpser")||is("reaper")){slot->loot={{3,uint16_t(roll(2,5))}};}
    else {slot->loot={{22,uint16_t(roll(1,2))}};if(is("bird")||is("chicken"))slot->loot.push_back({24,uint16_t(roll(3,8))});
        else if(is("cow")||is("bull")||is("sheep")||is("deer")||is("hind")||is("bear")||is("wolf")||is("goat"))slot->loot.push_back({23,uint16_t(roll(1,3))});}
    message="CRIATURA MUERTA | MOCHILA: ABRIR CADAVER";
    if(target==int(index)){target=-1;attacking=false;}
}
bool Combat::update(const World&w,Npcs&npcs,Actor&player,bool moving,float dt,PlayerStatus&status,Inventory&inv){
    if(mob.size()!=npcs.all().size())reset(npcs);
    if(dt<=0)return false;
    for(auto&e:damage)if(e.active){e.age+=dt;if(e.age>=1.2f)e.active=false;}
    for(auto&p:blood)if(p.active){p.age+=dt;if(p.age>=3)p.active=false;}
    status.healthVisible=std::max(0.f,status.healthVisible-dt);status.flash=std::max(0.f,status.flash-dt);swing=std::max(0.f,swing-dt);strike=std::max(0.f,strike-dt);
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
    if(attacking&&target>=0){auto&enemy=npcs.combatants()[target];if(!enemy.hostile)enemy.wait=0;enemy.hostile=true;}
    if(attacking&&target>=0&&!moving&&!npcs.all()[target].moving&&swing<=0&&meleeReach(w,player,npcs.all()[target].position)){
        swing=status.attackDelay(&inv);
        if(status.stamina>0){auto&n=npcs.combatants()[target];auto&v=mob[target];n.hostile=true;strike=.45f;--status.stamina;
            int dealt=std::min(v.hits,roll(status.damageMin(&inv),status.damageMax(&inv)));v.hits-=dealt;damageVisual(n.position,dealt,false,target);v.flash=.15f;
            status.healthVisible=6;
            if(v.hits==0)kill(size_t(target),npcs,inv);
        }
    }
    for(size_t i=0;i<mob.size();++i){auto&n=npcs.combatants()[i];auto&v=mob[i];
        if((n.maximumHits>0&&n.meleeMax==0)||!n.alive||!n.animal||!n.hostile||n.moving||moving||v.cooldown>0||!meleeReach(w,n.position,player))continue;
        v.cooldown=1.25f;int damage=roll(n.meleeMin?n.meleeMin:n.name.find("dog")!=std::string::npos?4:2,n.meleeMax?n.meleeMax:n.name.find("dog")!=std::string::npos?7:4);
        damage=std::max(1,damage*(100-status.resistance(&inv))/100);
        damage=std::min(status.hits,damage);status.hits-=damage;status.tookDamage();damageVisual(player,damage,true,-1);status.flash=.15f;message="RECIBES "+std::to_string(damage)+" DE DANO";
        if(status.hits==0){
            status.hits=status.hitsMax;status.mana=status.manaMax;status.stamina=status.staminaMax;status.healthVisible=6;status.recoveryGoal=0;
            for(auto&other:npcs.combatants())other.hostile=false;
            attacking=false;target=-1;strike=0;inv.closeContainer();player={w.startX,w.startY,w.startZ};regen=0;message="HAS MUERTO | REGRESAS AL PUNTO INICIAL";return true;
        }
    }
    regen+=dt;if(regen>=1){regen-=1;status.mana=std::min(status.manaMax,status.mana+1);status.stamina=std::min(status.staminaMax,status.stamina+1);if(status.healthVisible<=0)status.hits=std::min(status.hitsMax,status.hits+1);}
    status.recovered();
    return false;
}
int Combat::nearestCorpse(const World&w,const Actor&a)const{
    int best=-1,range=100000;
    for(size_t i=0;i<corpses.size();++i){const auto&c=corpses[i];if(!c.active||!meleeReach(w,a,c.position))continue;
        int d=(a.x-c.position.x)*(a.x-c.position.x)+(a.y-c.position.y)*(a.y-c.position.y);
        if(best<0||d<range||(d==range&&((corpses[best].loot.empty()&&!c.loot.empty())||(corpses[best].loot.empty()==c.loot.empty()&&c.decay>corpses[best].decay)))){best=int(i);range=d;}
    }return best;
}
bool Combat::inspectNearest(const World&w,const Actor&a,Inventory&inv){
    int body=nearestCorpse(w,a),chest=inv.nearest(w,a);
    if(body>=0&&chest>=0){const auto&c=corpses[body].position;const auto&o=w.objects[inv.chests[chest].object];
        int cd=(c.x-a.x)*(c.x-a.x)+(c.y-a.y)*(c.y-a.y),bd=(o.x-a.x)*(o.x-a.x)+(o.y-a.y)*(o.y-a.y);if(bd<cd)body=-1;
    }
    if(body>=0){auto&c=corpses[body];if(inv.isExternal(&c.loot))return false;return inv.openExternal(w,a,c.loot,c.position,c.name);}
    if(chest>=0)return inv.followChest(chest);
    bool changed=inv.hasContainer();inv.closeContainer();return changed;
}
bool Combat::openCorpse(const World&w,const Actor&a,Inventory&inv){int i=nearestCorpse(w,a);return i>=0&&inv.openExternal(w,a,corpses[i].loot,corpses[i].position,corpses[i].name.empty()?"CADAVER":corpses[i].name);}
bool StatusArt::load(const std::string&path){
    std::ifstream f(path,std::ios::binary);std::vector<uint8_t>b(16+300*82*4);rgba.clear();
    if(!f.read((char*)b.data(),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UST1",4)||b[4]!=44||b[5]!=1||b[6]!=82||b[7]||get(b.data()+8)!=300*82*4||get(b.data()+12)!=hash(b.data()+16,b.size()-16))return false;
    rgba.assign(b.begin()+16,b.end());return true;
}
bool loadVitals(const std::string&path,PlayerStatus&status){
    std::ifstream f(path,std::ios::binary);if(!f){f.open(path+".bak",std::ios::binary);if(!f)return true;}
    uint8_t b[20];if(!f.read((char*)b,20)||f.peek()!=EOF||std::memcmp(b,"UVT1",4)||get(b+16)!=hash(b,16))return false;
    unsigned h=get(b+4),m=get(b+8),s=get(b+12);if(h==0||h>unsigned(status.hitsMax)||m>unsigned(status.manaMax)||s>unsigned(status.staminaMax))return false;
    status.hits=h;if(status.hits<status.hitsMax)status.tookDamage();status.mana=m;status.stamina=s;return true;
}
bool saveVitals(const std::string&path,const PlayerStatus&status,SaveRename renameFile){
    uint8_t b[20]={'U','V','T','1'};put(b+4,status.hits);put(b+8,status.mana);put(b+12,status.stamina);put(b+16,hash(b,16));
    std::string tmp=path+".tmp",bak=path+".bak";FILE*f=std::fopen(tmp.c_str(),"wb");if(!f)return false;
    bool ok=std::fwrite(b,1,20,f)==20;if(std::fclose(f))ok=false;if(!ok){std::remove(tmp.c_str());return false;}
    bool had=std::ifstream(path).good();if(had&&((std::remove(bak.c_str())&&errno!=ENOENT)||renameFile(path.c_str(),bak.c_str()))){std::remove(tmp.c_str());return false;}
    if(renameFile(tmp.c_str(),path.c_str())){if(had)renameFile(bak.c_str(),path.c_str());std::remove(tmp.c_str());return false;}return true;
}
}
