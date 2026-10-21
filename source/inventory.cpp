// SPDX-License-Identifier: MIT
#include "inventory.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sys/stat.h>
namespace lab {
const std::array<ItemDef,ItemTypes> itemDefs={{{0xEED,"ORO",true},{0x9F1,"PAN",true},{0x9D0,"MANZANA",true},{0xE21,"VENDAS",true},{0xF0E,"POCION",false},{0xF51,"DAGA",false,0},{0x1F03,"TUNICA",false,1},{0x170B,"BOTAS",false,2}}};
namespace {
uint32_t hash(const std::vector<uint8_t>&v){uint32_t h=2166136261u;for(auto b:v)h=(h^b)*16777619u;return h;}
void put(std::vector<uint8_t>&b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(i*8)));}
uint32_t get(const std::vector<uint8_t>&b,size_t p){return uint32_t(b[p])|uint32_t(b[p+1])<<8|uint32_t(b[p+2])<<16|uint32_t(b[p+3])<<24;}
bool chestArt(unsigned art){return (art>=0xE40&&art<=0xE43)||art==0xE80||art==0x9AB||art==0xE7C;}
bool exists(const std::string&p){struct stat s;return stat(p.c_str(),&s)==0;}
}
bool ItemIcons::load(const std::string&p){
    std::ifstream f(p,std::ios::binary);std::vector<uint8_t>b(8+ItemTypes*24*24*4);
    if(!f.read(reinterpret_cast<char*>(b.data()),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UII1",4)||get(b,4)!=ItemTypes){rgba.clear();return false;}
    rgba.assign(b.begin()+8,b.end());return true;
}
void Inventory::init(const World&w,const std::string&p){
    path=p;opened=-1;backpack.clear();chests.clear();writable=true;restored=false;equipment.fill(-1);equipMode=false;
    std::vector<uint8_t>key;put(key,w.originX);put(key,w.originY);put(key,w.width);put(key,w.height);
    // Canonical object order identifies containers independently of open doors.
    for(size_t i=0;i<w.objects.size();++i){const auto&o=w.objects[i];
        put(key,o.art);put(key,o.x);put(key,o.y);put(key,o.z);
        if(chestArt(o.art)&&chests.size()<1024){
            size_t n=chests.size();
            // Finite local exploration supplies, not server loot tables.
            Bag loot;loot.reserve(BagSlots);
            loot.push_back({0,uint16_t(15+n*7)});loot.push_back({1,uint16_t(2+n%3)});loot.push_back({3,uint16_t(4+n%4)});
            loot.push_back({uint16_t(n%2?4:2),uint16_t(n%2?1:3)});
            if(n%3==0)loot.push_back({5,1});
            chests.push_back({i,loot});
        }
    }
    identity=w.compatibleInventory?w.compatibleInventory:hash(key);message="ACERCATE A UN COFRE";
    std::string input=path;
    if(!exists(input)&&exists(path+".bak"))input=path+".bak";
    if(!exists(input)){seedGear();return;}
    std::ifstream f(input,std::ios::binary);std::vector<uint8_t>b(524289);f.read(reinterpret_cast<char*>(b.data()),b.size());b.resize(size_t(f.gcount()));
    bool ok=b.size()>=20&&b.size()<=524288&&(!std::memcmp(b.data(),"UIN1",4)||!std::memcmp(b.data(),"UIN2",4))&&get(b,4)==identity&&get(b,8)==chests.size();
    if(ok){std::vector<uint8_t>body(b.begin(),b.end()-4);ok=get(b,b.size()-4)==hash(body);}
    size_t pos=12;std::vector<Bag>bags;
    if(ok)for(size_t n=0;n<=chests.size();++n){
        if(pos+4>b.size()-4){ok=false;break;}unsigned count=get(b,pos);pos+=4;
        if(count>BagSlots||pos+count*8>b.size()-4){ok=false;break;}
        Bag bag;std::array<bool,ItemTypes>used{};
        for(unsigned k=0;k<count;++k){unsigned type=get(b,pos),amount=get(b,pos+4);pos+=8;
            if(type>=ItemTypes||!amount||amount>65535||(used[type]&&itemDefs[type].stackable)||(!itemDefs[type].stackable&&amount!=1)){ok=false;break;}
            used[type]=true;bag.push_back({uint16_t(type),uint16_t(amount)});
        }
        if(!ok)break;
        bags.push_back(bag);
    }
    bool legacy=b.size()>=4&&b[3]=='1';
    std::array<int,3>loaded{{-1,-1,-1}};
    if(ok&&!legacy){
        if(pos+12!=b.size()-4)ok=false;
        else for(int slot=0;slot<3;++slot){uint32_t value=get(b,pos);pos+=4;
            if(value==0xffffffffu)continue;
            if(value>=ItemTypes||itemDefs[value].slot!=slot){ok=false;break;}
            loaded[slot]=int(value);
        }
    }
    if(!ok||pos!=b.size()-4){writable=false;backpack.clear();for(auto&c:chests)c.items.clear();message="ERROR INVENTARIO: ARCHIVO INTACTO";return;}
    backpack=bags[0];for(size_t n=0;n<chests.size();++n)chests[n].items=bags[n+1];
    restored=true;equipment=loaded;if(legacy)seedGear();
    message=input==path?"INVENTARIO RESTAURADO":"INVENTARIO RECUPERADO";
}
void Inventory::seedGear(){
    // One-time additions to old UIN1 and new games, never refill UIN2 saves.
    for(auto&c:chests)if(c.items.size()+2<=BagSlots){c.items.push_back({6,1});c.items.push_back({7,1});}
}
unsigned Inventory::equipmentMask()const{unsigned m=0;for(size_t s=0;s<equipment.size();++s)if(equipment[s]>=0)m|=1u<<s;return m;}
bool Inventory::equip(size_t index,SaveRename renameFile){
    if(!writable){message="ERROR INVENTARIO: ARCHIVO INTACTO";return false;}
    if(index>=backpack.size())return false;
    auto item=backpack[index];int slot=itemDefs[item.type].slot;
    if(slot<0){message="ESTE OBJETO NO SE PUEDE EQUIPAR";return false;}
    unsigned mask=equipmentMask()|(1u<<slot);
    if(prepareEquipment&&!prepareEquipment(mask)){message="FALTAN RECURSOS DE EQUIPO";return false;}
    Bag oldBag=backpack;auto oldEquipment=equipment;
    backpack.erase(backpack.begin()+index);
    if(equipment[slot]>=0)backpack.push_back({uint16_t(equipment[slot]),1});
    equipment[slot]=item.type;
    if(!persist(message,renameFile)){backpack=oldBag;equipment=oldEquipment;return false;}
    message=std::string(itemDefs[item.type].name)+" EQUIPADO - GUARDADO";return true;
}
bool Inventory::unequip(size_t slot,SaveRename renameFile){
    if(!writable||slot>=equipment.size()||equipment[slot]<0)return false;
    if(backpack.size()>=BagSlots){message="MOCHILA LLENA: HAZ ESPACIO";return false;}
    unsigned mask=equipmentMask()&~(1u<<slot);
    if(prepareEquipment&&!prepareEquipment(mask)){message="FALTAN RECURSOS DE EQUIPO";return false;}
    Bag oldBag=backpack;auto oldEquipment=equipment;int type=equipment[slot];
    backpack.push_back({uint16_t(type),1});equipment[slot]=-1;
    if(!persist(message,renameFile)){backpack=oldBag;equipment=oldEquipment;return false;}
    message=std::string(itemDefs[type].name)+" A MOCHILA - GUARDADO";return true;
}
bool Inventory::reachable(const World&w,const Actor&a,size_t n)const{
    if(n>=chests.size()||chests[n].object>=w.objects.size())return false;
    const auto&o=w.objects[chests[n].object];
    if(!w.contains(a.x,a.y)||std::abs(a.x-o.x)+std::abs(a.y-o.y)>1||std::abs(a.z-o.z)>8)return false;
    // Do not reach through walls or closed doors sharing either endpoint.
    const int xs[]={a.x,o.x},ys[]={a.y,o.y};
    for(int cell=0;cell<2;++cell)for(size_t index:w.cells[ys[cell]*w.width+xs[cell]]){
        const auto&v=w.objects[index];if(&v!=&o&&(v.flags&(Wall|Door))&&v.z<a.z+14&&v.z+std::max(1,v.height)>a.z+2)return false;
    }
    return true;
}
int Inventory::nearest(const World&w,const Actor&a)const{for(size_t n=0;n<chests.size();++n)if(reachable(w,a,n))return int(n);return -1;}
void Inventory::update(const World&w,const Actor&a){if(opened>=0&&!reachable(w,a,size_t(opened))){opened=-1;message="COFRE FUERA DE ALCANCE";}}
void Inventory::open(const World&w,const Actor&a){
    if(!writable){message="ERROR INVENTARIO: ARCHIVO INTACTO";return;}
    if(opened>=0){opened=-1;message="COFRE CERRADO";return;}
    opened=nearest(w,a);message=opened<0?"ACERCATE AL LADO DE UN COFRE":"COFRE ABIERTO - TOCA UN OBJETO";
}
bool Inventory::persist(std::string&error,SaveRename renameFile){
    std::vector<uint8_t>b={'U','I','N','2'};put(b,identity);put(b,chests.size());
    auto append=[&](const Bag&bag){put(b,bag.size());for(auto s:bag){put(b,s.type);put(b,s.count);}};
    append(backpack);for(const auto&c:chests)append(c.items);for(int type:equipment)put(b,uint32_t(type));put(b,hash(b));
    std::string tmp=path+".tmp",bak=path+".bak";
    FILE*f=std::fopen(tmp.c_str(),"wb");if(!f){error="ERROR SD: NO SE MOVIO EL OBJETO";return false;}
    bool ok=std::fwrite(b.data(),1,b.size(),f)==b.size();if(std::fclose(f))ok=false;
    if(!ok){std::remove(tmp.c_str());error="ERROR SD: NO SE MOVIO EL OBJETO";return false;}
    bool had=exists(path);
    if(had&&((std::remove(bak.c_str())&&errno!=ENOENT)||renameFile(path.c_str(),bak.c_str()))){std::remove(tmp.c_str());error="ERROR SD: RESPALDO FALLIDO";return false;}
    if(renameFile(tmp.c_str(),path.c_str())){
        bool restored=!had||renameFile(bak.c_str(),path.c_str())==0;
        std::remove(tmp.c_str());error="ERROR SD: NO SE MOVIO EL OBJETO";
        // After a failed rollback require a reload; preserve the recovery file.
        if(!restored)writable=false;
        return false;
    }
    return true;
}
bool Inventory::transfer(const World&w,const Actor&a,bool fromChest,size_t slot,SaveRename renameFile){
    update(w,a);if(!writable||opened<0){message="ABRE UN COFRE CERCANO";return false;}
    Bag&source=fromChest?chests[opened].items:backpack;Bag&dest=fromChest?backpack:chests[opened].items;
    if(slot>=source.size())return false;
    auto item=source[slot];auto found=std::find_if(dest.begin(),dest.end(),[&](ItemStack s){return itemDefs[item.type].stackable&&s.type==item.type;});
    if((found==dest.end()&&dest.size()>=BagSlots)||(found!=dest.end()&&unsigned(found->count)+item.count>65535)){message="SIN ESPACIO PARA ESA PILA";return false;}
    Bag oldSource=source,oldDest=dest;
    if(found==dest.end())dest.push_back(item);else found->count+=item.count;
    source.erase(source.begin()+slot);
    if(!persist(message,renameFile)){source=oldSource;dest=oldDest;return false;}
    message=std::string(itemDefs[item.type].name)+(fromChest?" A MOCHILA - GUARDADO":" AL COFRE - GUARDADO");return true;
}
void Inventory::tap(const World&w,const Actor&a,int x,int y){
    if(y>=163&&y<181&&x>=20&&x<150){equipMode=!equipMode;message=equipMode?"TOCA ROPA O ARMA DE TU MOCHILA":"MODO TRANSFERIR AL COFRE";return;}
    if(y>=163&&y<181&&x>=166&&x<301){open(w,a);return;}
    if(y<65||y>=161)return;
    bool right=x>=166;int left=right?166:20;
    if(x<left||x>=left+114)return;
    int col=(x-left)/38,row=(y-65)/32;
    if(!right&&equipMode)equip(size_t(row*3+col));
    else transfer(w,a,right,size_t(row*3+col));
}
}
