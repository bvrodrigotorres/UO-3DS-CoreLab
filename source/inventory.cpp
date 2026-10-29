// SPDX-License-Identifier: MIT
#include "inventory.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
namespace lab {
static const std::array<ItemDef,BaseItemTypes> baseDefs={{{0xEED,"ORO",true,-1,0,0,.85f,0,0,0},
{0x103B,"PAN",true,-1,0,0,.85f,0,1,6},{0x9D0,"MANZANA",true,-1,0,0,.85f,0,1,4},
{0xE21,"VENDAS",true,-1,0,0,.85f,0,1,8},{0xF0E,"POCION",false,-1,0,0,.85f,0,1,25},
{0xF51,"DAGA",false,0,7,11,.65f,0,1,35},{0x1F03,"TUNICA",false,1,0,0,.85f,0,3,20},
{0x170B,"BOTAS",false,2,0,0,.85f,2,2,18},{0x1517,"CAMISA",false,5,0,0,.85f,0,1,10},
{0x1EFD,"CAMISA ELEGANTE",false,5,0,0,.85f,0,1,16},{0x152F,"PANTALON",false,7,0,0,.85f,0,2,12},
{0x1531,"FALDA",false,7,0,0,.85f,0,2,12},{0x170F,"ZAPATOS",false,2,0,0,.85f,0,1,8},
{0x1713,"GORRO",false,3,0,0,.85f,0,1,12},{0x13C7,"GORJAL",false,4,0,0,.85f,4,1,28},
{0x13C6,"GUANTES",false,6,0,0,.85f,4,1,24},
{0xF61,"ESPADA",false,0,11,17,1.05f,0,7,100},
{0xF5C,"MAZA",false,0,14,20,1.35f,0,9,115},
{0xF49,"HACHA",false,0,17,25,1.65f,0,10,140},
{0x13CC,"PECHERA CUERO",false,5,0,0,.85f,12,6,90},
{0x13BF,"COTA DE MALLA",false,5,0,0,.85f,24,12,180},
{0x1415,"PECHERA PLACAS",false,5,0,0,.85f,36,18,300},
{0x9F1,"CARNE CRUDA",true,-1,0,0,.85f,0,1,4},
{0x1079,"PIELES",true,-1,0,0,.85f,0,1,8},
{0x1BD1,"PLUMAS",true,-1,0,0,.85f,0,1,3},
{0xF7E,"HUESOS",true,-1,0,0,.85f,0,1,5},
{0x13CD,"MANGAS CUERO",false,8,0,0,.85f,6,2,45},
{0x1410,"MANGAS PLACAS",false,8,0,0,.85f,10,5,120},
{0x108A,"ANILLO PODER",false,9,2,2,.85f,0,1,220},
{0x1086,"BRAZALETE GUARDA",false,10,0,0,.85f,5,1,160}}};
const char*qualityName(unsigned q){static const char*names[]={"NORMAL","EXCEPCIONAL","MAGICO","RARO","LEGENDARIO"};return names[std::min(4u,q)];}
const char*powerName(unsigned p){static const char*names[]={"FUERZA","PROTECCION","VIDA","MANA","RAPIDEZ","FORTUNA","LLAMAS","ESCARCHA","VENENO","ENERGIA"};return names[p%ItemPowers];}
ItemCatalogue::ItemCatalogue(){
    for(unsigned i=0;i<BaseItemTypes;++i){data[i]=baseDefs[i];auto&d=data[i];
        d.resists[0]=d.protection;
        // ServUO elemental base values; retain existing physical gear balance.
        if(i==14||i==15||i==19||i==26)d.resists={{d.protection,4,3,3,3}};
        if(i==20)d.resists={{d.protection,4,4,1,2}};
        if(i==21||i==27)d.resists={{d.protection,3,2,3,2}};
    }
    data[28].flatDamage=2;
    for(unsigned i=BaseItemTypes;i<ItemTypes;++i){
        unsigned base=itemBase(i),q=itemQuality(i),p=itemPower(i);auto&d=data[i];d=data[base];
        d.damageMin=d.damageMin*(100+q*12)/100;d.damageMax=d.damageMax*(100+q*12)/100;
        for(auto&r:d.resists){if(r)r+=int(q);}d.protection=d.resists[0];
        d.price=std::max(10,d.price)*(1+int(q*q)*2);
        auto power=[&](unsigned id,int strength){switch(id){
            case 0:d.damageIncrease+=8*strength;d.hitChance+=2*strength;break;
            case 1:d.defenseChance+=3*strength;for(auto&r:d.resists)r+=2*strength;break;
            case 2:if(d.slot==0)d.lifeLeech+=8*strength;else d.regenLife+=strength;break;
            case 3:if(d.slot==0)d.manaLeech+=8*strength;else d.regenMana+=strength;break;
            case 4:d.swingSpeed+=3*strength;d.hitChance+=strength;d.regenStamina+=strength;break;
            case 5:d.luck+=30*strength;d.defenseChance+=strength;break;
            default:{int channel=int(id)-5;d.resists[channel]+=4*strength;d.damageIncrease+=3*strength;
                if(d.slot==0&&!d.element){d.element=channel;d.elementShare=25+strength*10;}break;}
        }};
        power(p,int(q));if(q>=3)power((p+3)%ItemPowers,int(q)-1);if(q==4)power((p+6)%ItemPowers,2);
        d.protection=d.resists[0];
        std::snprintf(names[i].data(),names[i].size(),"%c %s %s","NEMRL"[q],data[base].name,powerName(p));d.name=names[i].data();
    }
}
const ItemCatalogue itemDefs;
unsigned rollEquipment(unsigned rank,uint32_t&seed,int luck,unsigned family){
    rank=std::min(rank,4u);auto roll=[&](unsigned limit){seed=seed*1664525u+1013904223u;return (seed>>8)%limit;};
    static constexpr unsigned limits[5][4]={{8000,9700,9970,9998},{6500,9000,9900,9995},{2000,6000,9500,9980},{0,0,7000,9900},{0,0,0,9500}};
    auto quality=[&](){unsigned value=roll(10000),q=0;while(q<4&&value>=limits[rank][q])++q;return q;};
    unsigned q=quality();if(roll(5000)<unsigned(std::max(0,std::min(1000,luck))))q=std::max(q,quality());
    static constexpr unsigned weapons[]={5,16,17,18},armour[]={6,7,8,9,10,11,12,13,14,15,19,20,21,26,27},jewels[]={28,29};
    if(family>2)family=roll(3);
    unsigned base=family==0?weapons[roll(4)]:family==1?armour[roll(15)]:jewels[roll(2)];
    return magicItem(base,q,roll(ItemPowers));
}
std::vector<std::string>itemDetails(unsigned type){
    if(type>=ItemTypes)return {};
    const auto&d=itemDefs[type];std::vector<std::string>lines;lines.reserve(15);
    lines.push_back(baseDefs[itemBase(type)].name);lines.push_back(qualityName(itemQuality(type)));
    if(itemQuality(type))lines.push_back(std::string("PODER: ")+powerName(itemPower(type)));
    if(d.slot==0){lines.push_back("DANO "+std::to_string(d.damageMin)+"-"+std::to_string(d.damageMax));lines.push_back("TIEMPO BASE "+std::to_string(int(d.delay*1000))+" MS");}
    lines.push_back("FIS "+std::to_string(d.resists[0])+" FUE "+std::to_string(d.resists[1])+" FRI "+std::to_string(d.resists[2]));
    lines.push_back("VEN "+std::to_string(d.resists[3])+" ENE "+std::to_string(d.resists[4]));
    auto add=[&](const char*name,int n,const char*suffix=""){if(n)lines.push_back(std::string(name)+" +"+std::to_string(n)+suffix);};
    add("DANO",d.damageIncrease,"%");add("PRECISION",d.hitChance,"%");add("EVASION",d.defenseChance,"%");add("VELOCIDAD",d.swingSpeed,"%");
    add("ROBO VIDA",d.lifeLeech,"%");add("ROBO MANA",d.manaLeech,"%");add("REGEN VIDA",d.regenLife," /4S");add("REGEN MANA",d.regenMana);add("REGEN VIGOR",d.regenStamina);add("SUERTE",d.luck);add("DANO FIJO",d.flatDamage);
    if(d.element)lines.push_back(std::string("DANO ")+powerName(d.element+5)+" "+std::to_string(d.elementShare)+"%");
    return lines;
}
namespace {
uint32_t hash(const std::vector<uint8_t>&v){uint32_t h=2166136261u;for(auto b:v)h=(h^b)*16777619u;return h;}
void put(std::vector<uint8_t>&b,uint32_t n){for(int i=0;i<4;++i)b.push_back(uint8_t(n>>(i*8)));}
uint32_t get(const std::vector<uint8_t>&b,size_t p){return uint32_t(b[p])|uint32_t(b[p+1])<<8|uint32_t(b[p+2])<<16|uint32_t(b[p+3])<<24;}
bool chestArt(unsigned art){return (art>=0xE40&&art<=0xE43)||art==0xE80||art==0x9AB||art==0xE7C;}
bool exists(const std::string&p){struct stat s;return stat(p.c_str(),&s)==0;}
}
uint32_t inventoryWorldId(const World&w){
    if(w.compatibleInventory)return w.compatibleInventory;
    uint32_t h=2166136261u;auto add=[&](uint32_t value){for(int i=0;i<4;++i)h=(h^uint8_t(value>>(i*8)))*16777619u;};
    add(w.originX);add(w.originY);add(w.width);add(w.height);
    for(const auto&o:w.objects){add(o.art);add(o.x);add(o.y);add(o.z);}return h;
}
bool ItemIcons::load(const std::string&p){
    std::ifstream f(p,std::ios::binary);std::vector<uint8_t>b(8+BaseItemTypes*24*24*4);
    if(!f.read(reinterpret_cast<char*>(b.data()),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UII1",4)||get(b,4)!=BaseItemTypes){rgba.clear();return false;}
    rgba.assign(b.begin()+8,b.end());return true;
}
void Inventory::init(const World&w,const std::string&p){
    path=p;trainingSerial=0;trainingSkills={};ground={};quests={};groundFormat=false;groundRetry=0;closeContainer();backpack.clear();chests.clear();writable=true;restored=false;bagPage=0;equipment.fill(-1);lastEquipment.fill(-1);equipMode=useMode=false;dressed=accessories=false;
    uint32_t canonical=inventoryWorldId(w);
    // Canonical object order identifies containers independently of open doors.
    for(size_t i=0;i<w.objects.size();++i){const auto&o=w.objects[i];
        if(chestArt(o.art)&&chests.size()<1024){
            size_t n=chests.size();
            // Finite local exploration supplies, not server loot tables.
            Bag loot;loot.reserve(ChestSlots);
            loot.push_back({0,uint16_t(15+n*7)});loot.push_back({1,uint16_t(2+n%3)});loot.push_back({3,uint16_t(4+n%4)});
            loot.push_back({uint16_t(n%2?4:2),uint16_t(n%2?1:3)});
            uint32_t seed=canonical^uint32_t(o.x*7919+o.y*104729+o.art*31);
            bool dungeon=w.originX==6016&&w.originY==1408;
            if(dungeon){
                loot={{0,uint16_t(n?240:120)},{3,8},{4,1},{4,1}};
                loot.push_back({uint16_t(rollEquipment(n?3:2,seed,0,n?2:0)),1});
                loot.push_back({uint16_t(rollEquipment(2,seed,0,1)),1});
            }else {
                if(n%3==0)loot.push_back({5,1});
                if(w.width>32&&seed%5==0)loot.push_back({uint16_t(rollEquipment(0,seed)),1});
            }
            chests.push_back({i,loot});
        }
    }
    identity=canonical;message="ACERCATE A UN COFRE";
    std::string input=path;
    if(!exists(input)&&exists(path+".bak"))input=path+".bak";
    if(!exists(input)){seedGear();return;}
    std::ifstream f(input,std::ios::binary);std::vector<uint8_t>b(524289);f.read(reinterpret_cast<char*>(b.data()),b.size());b.resize(size_t(f.gcount()));
    bool ok=b.size()>=20&&b.size()<=524288&&b[0]=='U'&&b[1]=='I'&&b[2]=='N'&&((b[3]>='1'&&b[3]<='9')||b[3]=='A'||b[3]=='B')&&get(b,4)==identity&&get(b,8)==chests.size();
    const unsigned itemLimit=b.size()>=4&&(b[3]=='A'||b[3]=='B')?ItemTypes:BaseItemTypes;
    if(ok){std::vector<uint8_t>body(b.begin(),b.end()-4);ok=get(b,b.size()-4)==hash(body);}
    size_t pos=12;std::vector<Bag>bags;
    if(ok)for(size_t n=0;n<=chests.size();++n){
        if(pos+4>b.size()-4){ok=false;break;}unsigned count=get(b,pos);pos+=4;
        if(count>(n==0?BagSlots:ChestSlots)||pos+count*8>b.size()-4){ok=false;break;}
        Bag bag;std::array<bool,ItemTypes>used{};
        for(unsigned k=0;k<count;++k){unsigned type=get(b,pos),amount=get(b,pos+4);pos+=8;
            if(type>=itemLimit||!amount||amount>65535||(used[type]&&itemDefs[type].stackable)||(!itemDefs[type].stackable&&amount!=1)){ok=false;break;}
            used[type]=true;bag.push_back({uint16_t(type),uint16_t(amount)});
        }
        if(!ok)break;
        bags.push_back(bag);
    }
    bool legacy=b.size()>=4&&b[3]=='1';
    std::array<int,11>loaded;loaded.fill(-1);
    bool hasQuests=b.size()>=4&&(b[3]=='8'||b[3]=='9'||b[3]=='A'||b[3]=='B');
    bool hasGround=b.size()>=4&&(b[3]=='7'||hasQuests);
    bool expanded=b.size()>=4&&(b[3]=='6'||hasGround);
    int slots=expanded?11:8;
    bool remembered=b.size()>=4&&(b[3]=='5'||expanded);
    bool extended=b.size()>=4&&(b[3]=='4'||remembered);
    if(ok&&!legacy){
        size_t equipmentBytes=expanded?92:remembered?68:extended?36:12;
        if((!hasGround&&pos+equipmentBytes!=b.size()-4)||(hasGround&&pos+equipmentBytes+4>b.size()-4))ok=false;
        else for(int slot=0;slot<(extended?slots:3);++slot){uint32_t value=get(b,pos);pos+=4;
            if(value==0xffffffffu)continue;
            if(value>=itemLimit||itemDefs[value].slot!=slot){ok=false;break;}
            loaded[slot]=int(value);
        }
    }
    if(ok&&extended){uint32_t flags=get(b,pos);pos+=4;if(flags>3)ok=false;dressed=flags&1;accessories=flags&2;}
    if(ok&&remembered){for(int slot=0;slot<slots;++slot){uint32_t type=get(b,pos);pos+=4;if(type==0xffffffffu)continue;if(type>=itemLimit||itemDefs[type].slot!=slot){ok=false;break;}lastEquipment[slot]=type;}}
    if(ok&&hasGround){
        if(pos+4>b.size()-4)ok=false;
        unsigned count=ok?get(b,pos):0;pos+=4;if(count>ground.size())ok=false;
        for(unsigned n=0;ok&&n<count;++n){if(pos+16>b.size()-4){ok=false;break;}
            auto&pile=ground[n];pile.position={int(int32_t(get(b,pos))),int(int32_t(get(b,pos+4))),int(int32_t(get(b,pos+8)))};unsigned size=get(b,pos+12);pos+=16;
            if(!w.contains(pile.position.x,pile.position.y)||(pile.position.z< -512||pile.position.z>512)||!size||size>ChestSlots||pos+size*12>b.size()-4){ok=false;break;}
            for(unsigned i=0;i<size;++i){unsigned type=get(b,pos),amount=get(b,pos+4),ms=get(b,pos+8);pos+=12;
                if(type>=itemLimit||!amount||amount>65535||(!itemDefs[type].stackable&&amount!=1)||!ms||ms>180000){ok=false;break;}
                pile.items.push_back({uint16_t(type),uint16_t(amount)});pile.remaining.push_back(ms/1000.f);
            }
        }
        groundFormat=true;
    }
    if(ok&&hasQuests){
        unsigned savedQuests=b[3]=='8'?2:QuestCount;
        if(pos+savedQuests*16+(b[3]=='B'?4+Skills::WireSize:0)!=b.size()-4)ok=false;
        else for(unsigned i=0;i<savedQuests;++i){auto&q=quests[i];q={get(b,pos),get(b,pos+4),get(b,pos+8),get(b,pos+12)};pos+=16;if(!validQuest(i,q))ok=false;}
    }
    if(ok&&b[3]=='B'){trainingSerial=get(b,pos);pos+=4;ok=trainingSerial>0&&trainingSerial<=1000000&&trainingSkills.decode(b.data()+pos,Skills::WireSize)&&trainingSkills.ready;pos+=Skills::WireSize;}
    if(!ok||pos!=b.size()-4){trainingSerial=0;trainingSkills={};writable=false;ground={};quests={};backpack.clear();for(auto&c:chests)c.items.clear();message="ERROR INVENTARIO: ARCHIVO INTACTO";return;}
    backpack=bags[0];for(size_t n=0;n<chests.size();++n)chests[n].items=bags[n+1];
    restored=true;equipment=loaded;if(legacy)seedGear();
    message=input==path?"INVENTARIO RESTAURADO":"INVENTARIO RECUPERADO";
}
void Inventory::seedGear(){
    // One-time additions to old UIN1 and new games, never refill UIN2 saves.
    for(auto&c:chests)if(c.items.size()+2<=ChestSlots){c.items.push_back({6,1});c.items.push_back({7,1});}
}
unsigned Inventory::appearanceKey(const std::array<int,11>&outfit){
    auto equipment=outfit;for(auto&t:equipment)t=itemBase(t);
    unsigned m=0;for(size_t s=0;s<8;++s)if(equipment[s]>=0)m|=1u<<s;
    if(equipment[5]==9)m|=256;
    if(equipment[7]==11)m|=512;
    if(equipment[2]==12)m|=1024;
    if(equipment[0]>=16&&equipment[0]<=18)m|=unsigned(equipment[0]-15)<<13;
    if(equipment[5]>=19&&equipment[5]<=21)m|=unsigned(equipment[5]-18)<<15;
    if(equipment[8]>=26&&equipment[8]<=27)m|=unsigned(equipment[8]-25)<<17;
    if(equipment[9]>=0)m|=1u<<19;
    if(equipment[10]>=0)m|=1u<<20;
    return m;
}
unsigned Inventory::prospectiveKey(size_t slot,int type)const{auto next=equipment;next[slot]=type;return appearanceKey(next);}
void Inventory::initialDress(int shirt,int pants,bool shoes,bool robe){
    if(dressed)return;
    if(equipment[5]<0&&shirt)equipment[5]=shirt==1?8:9;
    if(equipment[7]<0&&pants)equipment[7]=pants==1?10:11;
    if(equipment[2]<0&&shoes)equipment[2]=12;
    if(equipment[1]<0&&robe)equipment[1]=6;
    dressed=true;
}
void Inventory::addAccessories(){
    if(accessories)return;
    for(auto&c:chests)if(c.items.size()+3<=ChestSlots){c.items.push_back({13,1});c.items.push_back({14,1});c.items.push_back({15,1});}
    accessories=true;
}
bool Inventory::setOutfit(const std::array<int,11>&desired,SaveRename renameFile){
    if(!writable){message="ERROR INVENTARIO: ARCHIVO INTACTO";return false;}
    if(desired==equipment)return true;
    Bag pool=backpack;
    for(int type:equipment)if(type>=0)pool.push_back({uint16_t(type),1});
    for(size_t slot=0;slot<desired.size();++slot){
        int type=desired[slot];if(type==-1)continue;
        if(type<0||type>=int(ItemTypes)||itemDefs[type].slot!=int(slot)){message="PRENDA NO VALIDA";return false;}
        auto found=std::find_if(pool.begin(),pool.end(),[&](const ItemStack&i){return i.type==type&&i.count;});
        if(found==pool.end()){message="NO POSEES ESA PRENDA";return false;}
        if(!--found->count)pool.erase(found);
    }
    if(pool.size()>BagSlots){message="MOCHILA LLENA: HAZ ESPACIO";return false;}
    if(prepareEquipment&&!prepareEquipment(appearanceKey(desired))){message="FALTAN RECURSOS DE EQUIPO";return false;}
    auto oldBag=backpack;auto oldEquipment=equipment;auto oldLast=lastEquipment;
    backpack=std::move(pool);equipment=desired;
    for(size_t slot=0;slot<desired.size();++slot)if(desired[slot]>=0)lastEquipment[slot]=desired[slot];else if(oldEquipment[slot]>=0)lastEquipment[slot]=oldEquipment[slot];
    if(!persist(message,renameFile)){backpack=std::move(oldBag);equipment=oldEquipment;lastEquipment=oldLast;if(prepareEquipment)prepareEquipment(equipmentMask());return false;}
    message=saveSink?"EQUIPO ACTUALIZADO":"EQUIPO GUARDADO";return true;
}
bool Inventory::equip(size_t index,SaveRename renameFile){
    if(!writable){message="ERROR INVENTARIO: ARCHIVO INTACTO";return false;}
    if(index>=backpack.size())return false;
    auto item=backpack[index];if(item.type>=ItemTypes)return false;int slot=itemDefs[item.type].slot;
    if(slot<0){message="ESTE OBJETO NO SE PUEDE EQUIPAR";return false;}
    unsigned mask=prospectiveKey(slot,item.type);
    if(prepareEquipment&&!prepareEquipment(mask)){message="FALTAN RECURSOS DE EQUIPO";return false;}
    Bag oldBag=backpack;auto oldEquipment=equipment;auto oldLast=lastEquipment;
    backpack.erase(backpack.begin()+index);
    if(equipment[slot]>=0)backpack.push_back({uint16_t(equipment[slot]),1});
    equipment[slot]=item.type;lastEquipment[slot]=item.type;
    if(!persist(message,renameFile)){backpack=oldBag;equipment=oldEquipment;lastEquipment=oldLast;if(prepareEquipment)prepareEquipment(equipmentMask());return false;}
    message=std::string(itemDefs[item.type].name)+(saveSink?" EQUIPADO":" EQUIPADO - GUARDADO");return true;
}
bool Inventory::unequip(size_t slot,SaveRename renameFile){
    if(!writable||slot>=equipment.size()||equipment[slot]<0)return false;
    if(backpack.size()>=BagSlots){message="MOCHILA LLENA: HAZ ESPACIO";return false;}
    unsigned mask=prospectiveKey(slot,-1);
    if(prepareEquipment&&!prepareEquipment(mask)){message="FALTAN RECURSOS DE EQUIPO";return false;}
    Bag oldBag=backpack;auto oldEquipment=equipment;auto oldLast=lastEquipment;int type=equipment[slot];
    backpack.push_back({uint16_t(type),1});equipment[slot]=-1;lastEquipment[slot]=type;
    if(!persist(message,renameFile)){backpack=oldBag;equipment=oldEquipment;lastEquipment=oldLast;if(prepareEquipment)prepareEquipment(equipmentMask());return false;}
    message=std::string(itemDefs[type].name)+(saveSink?" A MOCHILA":" A MOCHILA - GUARDADO");return true;
}
bool Inventory::toggleEquipment(size_t slot,SaveRename renameFile){
    if(slot>=equipment.size()||!writable)return false;
    if(equipment[slot]>=0)return unequip(slot,renameFile);
    int type=lastEquipment[slot];if(type<0){message="NO HAY PRENDA ANTERIOR EN ESTA RANURA";return false;}
    for(size_t i=0;i<backpack.size();++i)if(backpack[i].type==type)return equip(i,renameFile);
    message="ULTIMA PRENDA NO ESTA EN MOCHILA";return false;
}
static bool meleeContainerReach(const World&w,const Actor&a,const Actor&b){
    if(!w.contains(a.x,a.y)||!w.contains(b.x,b.y)||std::abs(a.z-b.z)>8||std::max(std::abs(a.x-b.x),std::abs(a.y-b.y))>1)return false;
    if(a.x==b.x&&a.y==b.y)return true;
    const int xs[]={0,1,1,1,0,-1,-1,-1},ys[]={-1,-1,0,1,1,1,0,-1};
    for(int d=0;d<8;++d)if(a.x+xs[d]==b.x&&a.y+ys[d]==b.y){Actor next;int actual;return step(w,a,d,next,actual)&&next.x==b.x&&next.y==b.y&&std::abs(next.z-b.z)<=8;}
    return false;
}
bool Inventory::reachable(const World&w,const Actor&a,size_t n)const{
    if(n>=chests.size()||chests[n].object>=w.objects.size())return false;
    const auto&o=w.objects[chests[n].object];
    if(!w.contains(a.x,a.y)||std::abs(a.x-o.x)+std::abs(a.y-o.y)>1||std::abs(a.z-o.z)>8)return false;
    // Do not reach through walls or closed doors sharing either endpoint.
    const int xs[]={a.x,o.x},ys[]={a.y,o.y};
    for(int cell=0;cell<2;++cell)for(size_t index:w.cells[ys[cell]*w.width+xs[cell]]){
        const auto&v=w.objects[index];if(&v!=&o&&(v.flags&(Wall|Door))&&v.z<a.z+14&&v.z+std::max(1,int(v.height))>a.z+2)return false;
    }
    return true;
}
int Inventory::nearest(const World&w,const Actor&a)const{
    int best=-1,distance=100000;for(size_t n=0;n<chests.size();++n)if(reachable(w,a,n)){const auto&o=w.objects[chests[n].object];int d=(o.x-a.x)*(o.x-a.x)+(o.y-a.y)*(o.y-a.y);if(d<distance){distance=d;best=int(n);}}return best;
}
bool Inventory::openExternal(const World&w,const Actor&a,Bag&bag,const Actor&position,const std::string&title){
    if(!writable||!meleeContainerReach(w,a,position))return false;
    external=&bag;externalPosition=position;externalTitle=title;opened=-2;message="CADAVER ABIERTO - TOCA EL BOTIN";return true;
}
void Inventory::update(const World&w,const Actor&a){
    if(onGround()&&!meleeContainerReach(w,a,ground[-opened-3].position)){closeContainer();message="SUELO FUERA DE ALCANCE";}
    if((opened>=0&&!reachable(w,a,size_t(opened)))||(opened==-2&&!meleeContainerReach(w,a,externalPosition))){closeContainer();message="CONTENEDOR FUERA DE ALCANCE";}
}
void Inventory::open(const World&w,const Actor&a){
    if(!writable){message="ERROR INVENTARIO: ARCHIVO INTACTO";return;}
    if(hasContainer()){closeContainer();message="CONTENEDOR CERRADO";return;}
    opened=nearest(w,a);message=opened<0?"ACERCATE AL LADO DE UN COFRE":"COFRE ABIERTO - TOCA UN OBJETO";
}
bool Inventory::persist(std::string&error,SaveRename renameFile){
    bool hasGround=groundFormat;for(const auto&p:ground)if(!p.items.empty())hasGround=true;
    bool magic=false;auto scan=[&](const Bag&items){for(auto s:items)if(s.type>=BaseItemTypes)magic=true;};
    scan(backpack);for(const auto&c:chests)scan(c.items);for(const auto&p:ground)scan(p.items);
    for(int t:equipment)if(t>=int(BaseItemTypes))magic=true;
    for(int t:lastEquipment)if(t>=int(BaseItemTypes))magic=true;
    bool hasQuests=magic||trainingSerial;for(const auto&q:quests)if(q.state)hasQuests=true;
    bool extended=dressed||accessories;for(size_t s=3;s<8;++s)if(equipment[s]>=0)extended=true;
    bool remembered=false;for(int t:lastEquipment)if(t>=0)remembered=true;extended=extended||remembered;
    bool expanded=false;for(size_t s=8;s<11;++s)if(equipment[s]>=0||lastEquipment[s]>=0)expanded=true;
    if(hasGround||hasQuests)expanded=true;
    if(expanded){extended=true;remembered=true;}
    size_t slots=expanded?11:8;
    std::vector<uint8_t>b={'U','I','N',uint8_t(trainingSerial?'B':magic?'A':hasQuests?(quests[2].state?'9':'8'):hasGround?'7':expanded?'6':remembered?'5':extended?'4':'3')};put(b,identity);put(b,chests.size());
    size_t capacity=128+backpack.size()*8;for(const auto&c:chests)capacity+=4+c.items.size()*8;for(const auto&p:ground)capacity+=16+p.items.size()*12;b.reserve(capacity);
    auto append=[&](const Bag&bag){put(b,bag.size());for(auto s:bag){put(b,s.type);put(b,s.count);}};
    append(backpack);for(const auto&c:chests)append(c.items);for(size_t s=0;s<(extended?slots:3u);++s)put(b,uint32_t(equipment[s]));if(extended)put(b,(dressed?1:0)|(accessories?2:0));if(remembered)for(size_t s=0;s<slots;++s)put(b,uint32_t(lastEquipment[s]));
    if(hasGround||hasQuests){unsigned count=0;for(const auto&pile:ground)if(!pile.items.empty())++count;put(b,count);
        for(const auto&pile:ground)if(!pile.items.empty()){put(b,pile.position.x);put(b,pile.position.y);put(b,pile.position.z);put(b,pile.items.size());
            for(size_t i=0;i<pile.items.size();++i){put(b,pile.items[i].type);put(b,pile.items[i].count);put(b,unsigned(std::max(1.f,std::min(180000.f,std::ceil(pile.remaining[i]*1000)))));}}
    }
    if(hasQuests)for(unsigned i=0;i<((magic||trainingSerial||quests[2].state)?QuestCount:2u);++i){const auto&q=quests[i];put(b,q.state);put(b,q.count);put(b,q.giver);put(b,q.signature);}
    if(trainingSerial){put(b,trainingSerial);trainingSkills.encode(b);}
    put(b,hash(b));
    if(saveSink&&renameFile==std::rename){if(saveSink(path,std::move(b)))return true;error="GUARDADO OCUPADO: REINTENTA";return false;}
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
    update(w,a);if(!writable||!hasContainer()){message="ABRE UN COFRE CERCANO";return false;}
    if(onGround()&&!fromChest)return drop(w,a,false,slot,renameFile);
    Bag&container=onGround()?ground[-opened-3].items:opened==-2?*external:chests[opened].items;
    Bag&source=fromChest?container:backpack;Bag&dest=fromChest?backpack:container;
    if(slot>=source.size())return false;
    auto item=source[slot];if(item.type>=ItemTypes)return false;auto found=std::find_if(dest.begin(),dest.end(),[&](ItemStack s){return itemDefs[item.type].stackable&&s.type==item.type;});
    if((found==dest.end()&&dest.size()>=(fromChest?BagSlots:ChestSlots))||(found!=dest.end()&&unsigned(found->count)+item.count>65535)){message="SIN ESPACIO PARA ESA PILA";return false;}
    int groundSlot=groundIndex();std::vector<float>oldTimes=groundSlot>=0?ground[groundSlot].remaining:std::vector<float>{};
    Bag oldSource=source,oldDest=dest;
    if(found==dest.end())dest.push_back(item);else found->count+=item.count;
    source.erase(source.begin()+slot);if(groundSlot>=0)ground[groundSlot].remaining.erase(ground[groundSlot].remaining.begin()+slot);
    if(!persist(message,renameFile)){source=oldSource;dest=oldDest;if(groundSlot>=0)ground[groundSlot].remaining=oldTimes;return false;}
    message=std::string(itemDefs[item.type].name)+(fromChest?(saveSink?" A MOCHILA":" A MOCHILA - GUARDADO"):(saveSink?" AL CONTENEDOR":" AL CONTENEDOR - GUARDADO"));return true;
}
bool Inventory::takeAll(const World&w,const Actor&a,SaveRename renameFile){
    update(w,a);if(!writable||!hasContainer()){message="NO HAY BOTIN AL ALCANCE";return false;}
    Bag&source=onGround()?ground[-opened-3].items:opened==-2?*external:chests[opened].items;
    if(source.empty()){message="CONTENEDOR VACIO";return false;}
    int groundSlot=groundIndex();std::vector<float>oldTimes=groundSlot>=0?ground[groundSlot].remaining:std::vector<float>{};
    Bag oldSource=source,oldBag=backpack;size_t moved=0;
    for(size_t i=0;i<source.size();){auto item=source[i];if(item.type>=ItemTypes){++i;continue;}
        auto found=std::find_if(backpack.begin(),backpack.end(),[&](ItemStack s){return itemDefs[item.type].stackable&&s.type==item.type;});
        if((found==backpack.end()&&backpack.size()>=BagSlots)||(found!=backpack.end()&&unsigned(found->count)+item.count>65535)){++i;continue;}
        if(found==backpack.end())backpack.push_back(item);else found->count+=item.count;
        source.erase(source.begin()+i);if(groundSlot>=0)ground[groundSlot].remaining.erase(ground[groundSlot].remaining.begin()+i);++moved;
    }
    if(!moved){message="MOCHILA LLENA";return false;}
    if(!persist(message,renameFile)){source=oldSource;backpack=oldBag;if(groundSlot>=0)ground[groundSlot].remaining=oldTimes;return false;}
    message=source.empty()?(saveSink?"TODO RECOGIDO":"TODO RECOGIDO - GUARDADO"):"MOCHILA LLENA - QUEDA BOTIN";return true;
}
int Inventory::nearestGround(const World&w,const Actor&a)const{
    int best=-1,distance=100000;for(size_t i=0;i<ground.size();++i){const auto&p=ground[i];if(p.items.empty()||!meleeContainerReach(w,a,p.position))continue;int dx=a.x-p.position.x,dy=a.y-p.position.y,d=dx*dx+dy*dy;if(d<distance){best=int(i);distance=d;}}return best;
}
bool Inventory::followGround(int index){if(index<0||size_t(index)>=ground.size())return false;int next=-index-3;if(opened==next)return false;closeContainer();opened=next;return true;}
bool Inventory::drop(const World&w,const Actor&a,bool fromContainer,size_t slot,SaveRename renameFile){
    update(w,a);if(!writable||!w.contains(a.x,a.y)||(fromContainer&&(!hasContainer()||onGround())))return false;
    Bag&source=fromContainer?(opened==-2?*external:chests[opened].items):backpack;if(slot>=source.size())return false;
    int target=-1;for(size_t i=0;i<ground.size();++i){auto&p=ground[i];if(!p.items.empty()&&p.position.x==a.x&&p.position.y==a.y&&p.position.z==a.z&&p.items.size()<ChestSlots){target=int(i);break;}}
    if(target<0)for(size_t i=0;i<ground.size();++i)if(ground[i].items.empty()){target=int(i);break;}
    if(target<0){message="SUELO LLENO: RECOGE OBJETOS";return false;}
    Bag previous=source;auto previousPile=ground[target];bool previousFormat=groundFormat;auto item=source[slot];auto&pile=ground[target];pile.position=a;pile.items.push_back(item);pile.remaining.push_back(180);source.erase(source.begin()+slot);groundFormat=true;
    if(!persist(message,renameFile)){source=std::move(previous);pile=std::move(previousPile);groundFormat=previousFormat;return false;}
    detailType=item.type;message="OBJETO EN SUELO: 3 MINUTOS";return true;
}
void Inventory::tickGround(float dt,SaveRename renameFile){
    if(dt<=0||!writable)return;
    groundRetry=std::max(0.f,groundRetry-dt);bool expired=false;
    for(auto&p:ground)for(float&t:p.remaining){t=std::max(0.f,t-dt);if(t<=0)expired=true;}
    if(!expired||groundRetry>0)return;
    auto before=ground;for(auto&p:ground)for(size_t i=0;i<p.items.size();)if(p.remaining[i]<=0){p.items.erase(p.items.begin()+i);p.remaining.erase(p.remaining.begin()+i);}else ++i;
    if(!persist(message,renameFile))ground=std::move(before);
    groundRetry=1;
}
bool Inventory::consume(size_t index,SaveRename renameFile){
    if(!writable||index>=backpack.size()||!itemHealing(backpack[index].type)){message="ESTE OBJETO NO CURA";return false;}
    Bag previous=backpack;if(!--backpack[index].count)backpack.erase(backpack.begin()+index);
    if(!persist(message,renameFile)){backpack=std::move(previous);return false;}return true;
}
bool Inventory::commitQuest(unsigned id,const QuestProgress&next,Bag items,SaveRename renameFile){
    if(!writable||id>=QuestCount||!validQuest(id,next)||items.size()>BagSlots)return false;
    std::array<bool,ItemTypes>used{};
    for(auto item:items){if(item.type>=ItemTypes||!item.count||(used[item.type]&&itemDefs[item.type].stackable)||(!itemDefs[item.type].stackable&&item.count!=1))return false;used[item.type]=true;}
    auto before=quests[id];auto previous=backpack;quests[id]=next;backpack=std::move(items);
    if(!persist(message,renameFile)){quests[id]=before;backpack=std::move(previous);return false;}return true;
}
bool Inventory::commitTraining(const Skills&book,unsigned cost,SaveRename renameFile){
    if(saveSink||!writable||!cost||cost>65535||!book.ready||book.total()>Skills::TotalCap||trainingSerial>=1000000){message="ENTRENAMIENTO NO DISPONIBLE";return false;}
    auto coins=std::find_if(backpack.begin(),backpack.end(),[](ItemStack s){return s.type==0;});
    if(coins==backpack.end()||coins->count<cost){message="ORO INSUFICIENTE";return false;}
    Bag oldBag=backpack;auto oldBook=trainingSkills;auto serial=trainingSerial;
    coins->count-=cost;if(!coins->count)backpack.erase(coins);
    trainingSkills=book;++trainingSerial;
    if(!persist(message,renameFile)){backpack=std::move(oldBag);trainingSkills=std::move(oldBook);trainingSerial=serial;return false;}
    message="ENTRENAMIENTO GUARDADO";return true;
}
bool Inventory::trade(bool buy,unsigned type,unsigned price,SaveRename renameFile){return tradeQuantity(buy,type,price,1,renameFile);}
bool Inventory::tradeQuantity(bool buy,unsigned type,unsigned price,unsigned quantity,SaveRename renameFile){
    if(!writable||type==0||type>=ItemTypes||!price||price>65535||!quantity||quantity>65535||(!itemDefs[type].stackable&&quantity!=1)){message="COMERCIO NO DISPONIBLE";return false;}
    uint64_t total=uint64_t(price)*quantity;
    if(total>65535){message="EL TOTAL SUPERA EL LIMITE DE ORO";return false;}
    Bag next=backpack;
    auto find=[&](unsigned t){return std::find_if(next.begin(),next.end(),[&](ItemStack v){return v.type==t;});};
    if(buy){auto coins=find(0);if(coins==next.end()||coins->count<total){message="ORO INSUFICIENTE";return false;}
        coins->count-=total;if(!coins->count)next.erase(coins);
        auto item=find(type);if(itemDefs[type].stackable&&item!=next.end()){if(item->count+quantity>65535){message="PILA LLENA";return false;}item->count+=quantity;}
        else {if(next.size()>=BagSlots){message="MOCHILA LLENA";return false;}next.push_back({uint16_t(type),uint16_t(quantity)});}
    }else{auto item=find(type);if(item==next.end()||item->count<quantity){message="OBJETO NO DISPONIBLE";return false;}
        item->count-=quantity;if(!item->count)next.erase(item);
        auto coins=find(0);if(coins!=next.end()){if(coins->count+total>65535){message="PILA DE ORO LLENA";return false;}coins->count+=total;}
        else{if(next.size()>=BagSlots){message="MOCHILA LLENA";return false;}next.push_back({0,uint16_t(total)});}
    }
    Bag previous=backpack;backpack=std::move(next);
    if(!persist(message,renameFile)){backpack=std::move(previous);return false;}
    detailType=type;message=std::string(buy?"COMPRADO: ":"VENDIDO: ")+itemDefs[type].name;return true;
}
void Inventory::tap(const World&w,const Actor&a,int x,int y){
    if(InventoryLayout::page(x,y)){bagPage=1-bagPage;return;}
    if(InventoryLayout::mode(x,y)){if(equipMode){equipMode=false;useMode=true;}else if(useMode)useMode=false;else equipMode=true;message=useMode?"TOCA POCION, VENDA O COMIDA":equipMode?"TOCA OBJETO PARA EQUIPAR":"TOCA OBJETO PARA TRANSFERIR";return;}
    if(InventoryLayout::take(x,y)){takeAll(w,a);return;}
    int left=InventoryLayout::slot(x,y,InventoryLayout::Left),right=InventoryLayout::slot(x,y,InventoryLayout::Right);
    if(left>=0){size_t index=bagPage*BagPageSlots+left;if(index<backpack.size())detailType=backpack[index].type;if(useMode){if(useItem)useItem(index);else message="CURACION NO DISPONIBLE";}else if(equipMode)equip(index);else transfer(w,a,false,index);}
    else if(right>=0){if(size_t(right)<containerItems().size())detailType=containerItems()[right].type;transfer(w,a,true,size_t(right));}
}
}
