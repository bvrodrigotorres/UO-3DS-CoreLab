// SPDX-License-Identifier: MIT
#include "character.hpp"
#include "mount.hpp"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cerrno>
#ifdef __3DS__
#include <3ds.h>
#include <malloc.h>
extern "C" u32 __ctru_heap_size;
#endif
namespace lab {
namespace {
// Original human creation hue/style identifiers, cross-checked against
// MobileUO/ClassicUO CharacterCreationValues.cs; see THIRD_PARTY.md.
const uint16_t skins[]={0x3E9,0x3F1,0x3F9,0x401,0x409,0x411,0x419,0x421,0x3EA,0x3F2,0x3FA,0x402,0x40A,0x412,0x41A,0x421,0x3EB,0x3F3,0x3FB,0x403,0x40B,0x413,0x41B,0x421,0x3EC,0x3F4,0x3FC,0x404,0x40C,0x414,0x41C,0x421,0x3ED,0x3F5,0x3FD,0x405,0x40D,0x415,0x41D,0x421,0x3EE,0x3F6,0x3FE,0x406,0x40E,0x416,0x41E,0x421,0x3EF,0x3F7,0x3FF,0x407,0x40F,0x417,0x41F,0x421,0x3F0,0x3F8,0x400,0x408,0x410,0x418,0x420,0x421};
const uint16_t hairColors[]={0x44D,0x455,0x45D,0x465,0x46D,0x475,0x44E,0x456,0x45E,0x466,0x46E,0x476,0x44F,0x457,0x45F,0x467,0x46F,0x477,0x450,0x458,0x460,0x468,0x470,0x478,0x451,0x459,0x461,0x469,0x471,0x479,0x452,0x45A,0x462,0x46A,0x472,0x47A,0x453,0x45B,0x463,0x46B,0x473,0x47B,0x454,0x45C,0x464,0x46C,0x474,0x47C};
const uint16_t clothColors[]={0x455,0x481,0x3E9,0x59B,0x21,0x3F,0x26,0x35,0x48,0x59,0x62,0x71,0x81,0x91,0xA1,0xB1,0xC1,0xD1,0xE1,0xF1,0x101,0x111,0x121,0x131,0x141,0x151,0x161,0x171,0x181,0x191,0x1A1,0x1B1};
const int maleHair[]={0,700,701,702,703,710,903,900,901,902},femaleHair[]={0,700,701,702,703,710,903,900,902,712},beards[]={0,800,801,802,803,904,905,906};
uint32_t get(const std::vector<uint8_t>&v,size_t at){return uint32_t(v[at])|uint32_t(v[at+1])<<8|uint32_t(v[at+2])<<16|uint32_t(v[at+3])<<24;}
uint32_t hash(const uint8_t*p,size_t n){uint32_t h=2166136261;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619;return h;}
void put(std::vector<uint8_t>&v,int at,uint32_t x){for(int i=0;i<4;++i)v[at+i]=x>>(8*i);}
bool exists(const std::string&p){std::ifstream f(p,std::ios::binary);return f.good();}
bool sparse(const std::vector<uint8_t>&b,size_t begin,size_t end,std::vector<uint16_t>&out){
    size_t at=0;while(begin<end){if(end-begin<4)return false;unsigned skip=b[begin]|unsigned(b[begin+1])<<8,count=b[begin+2]|unsigned(b[begin+3])<<8;begin+=4;
        if((!skip&&!count)||skip>out.size()-at)return false;
        at+=skip;if(count>out.size()-at||end-begin<size_t(count)*2)return false;
        for(unsigned i=0;i<count;++i){out[at++]=b[begin]|uint16_t(b[begin+1])<<8;begin+=2;}}
    return at==out.size();
}
}
bool Appearance::validName(const std::string&s){if(s.empty()||s.size()>16||s.front()==' '||s.back()==' ')return false;for(unsigned char c:s)if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||c==' '||c=='\''||c=='-'))return false;return true;}
bool Appearance::valid()const{return validName(name)&&female<2&&skin<64&&hair<10&&hairColor<48&&beard<8&&beardColor<48&&shirt<3&&shirtColor<32&&pants<3&&pantsColor<32&&shoes<2&&bootsColor<32&&robeColor<32&&hatColor<32&&neckColor<32&&glovesColor<32&&robe<2;}
bool Appearance::load(const std::string&p){
    if(!exists(p)&&!exists(p+".bak"))return true;
    std::ifstream f(exists(p)?p:p+".bak",std::ios::binary);std::vector<uint8_t>b(56);
    if(!f.read((char*)b.data(),b.size())||f.peek()!=EOF||(std::memcmp(b.data(),"UCA1",4)&&std::memcmp(b.data(),"UCA2",4)&&std::memcmp(b.data(),"UCA3",4))||get(b,52)!=hash(b.data(),52)||b[4]>1)return false;
    Appearance a;a.enabled=b[4];a.female=b[5];a.skin=b[6];a.hair=b[7];a.hairColor=b[8];a.beard=b[9];a.beardColor=b[10];a.shirt=b[11];a.shirtColor=b[12];a.pants=b[13];a.pantsColor=b[14];a.shoes=b[15];a.bootsColor=b[16];a.robeColor=b[17];
    if(b[18]>16)return false;
    a.name.assign((char*)b.data()+20,b[18]);for(size_t i=20+b[18];i<36;++i)if(b[i])return false;
    a.realClothes=b[19];a.hatColor=b[36];a.neckColor=b[37];a.glovesColor=b[38];a.robe=b[39];for(size_t i=40;i<52;++i)if(b[i])return false;
    if(b[19]>1||!a.valid())return false;
    *this=a;return true;
}
bool Appearance::save(const std::string&p,std::string&error,SaveRename renameFile)const{
    if(!valid()){error="PERSONAJE INVALIDO";return false;}
    std::vector<uint8_t>b(56);std::memcpy(b.data(),"UCA3",4);b[39]=robe;b[19]=realClothes;b[36]=hatColor;b[37]=neckColor;b[38]=glovesColor;b[4]=enabled;b[5]=female;b[6]=skin;b[7]=hair;b[8]=hairColor;b[9]=beard;b[10]=beardColor;b[11]=shirt;b[12]=shirtColor;b[13]=pants;b[14]=pantsColor;b[15]=shoes;b[16]=bootsColor;b[17]=robeColor;b[18]=name.size();std::copy(name.begin(),name.end(),b.begin()+20);put(b,52,hash(b.data(),52));
    auto tmp=p+".tmp",bak=p+".bak";FILE*f=std::fopen(tmp.c_str(),"wb");if(!f){error="ERROR SD: PERSONALIZACION";return false;}
    bool ok=std::fwrite(b.data(),1,b.size(),f)==b.size();if(std::fclose(f))ok=false;
    if(!ok){std::remove(tmp.c_str());error="ERROR SD: PERSONALIZACION";return false;}
    bool had=exists(p);if(had&&((std::remove(bak.c_str())&&errno!=ENOENT)||renameFile(p.c_str(),bak.c_str()))){std::remove(tmp.c_str());error="ERROR SD: RESPALDO";return false;}
    if(renameFile(tmp.c_str(),p.c_str())){if(had)renameFile(bak.c_str(),p.c_str());std::remove(tmp.c_str());error="ERROR SD: CAMBIOS SIN GUARDAR";return false;}
    error.clear();return true;
}
void AppearanceEditor::useWardrobe(const Inventory&inventory){
    wardrobe=true;outfit=inventory.equipment;equipment=inventory.equipmentMask();
    for(size_t slot=0;slot<owned.size();++slot){
        auto&types=owned[slot];types={-1};
        auto add=[&](int type){if(type>=0&&type<int(ItemTypes)&&itemDefs[type].slot==int(slot)&&std::find(types.begin(),types.end(),type)==types.end())types.push_back(type);};
        add(outfit[slot]);for(auto item:inventory.backpack)if(item.count)add(item.type);
        std::sort(types.begin()+1,types.end());
    }
}
int AppearanceEditor::fieldAt(int r)const{
    if(page==2&&wardrobe){static const int slots[]={3,4,5,6,7,2,1,0};return r>=0&&r<8?19+slots[r]:-1;}
    static const int fields[4][7]={{0,1,2,-1,-1,-1,-1},{3,4,5,6,-1,-1,-1},{7,9,11,18,-1,-1,-1},{8,10,12,13,15,16,17}};
    return page>=0&&page<4&&r>=0&&r<rowCount()?fields[page][r]:-1;
}
bool AppearanceEditor::enabled(int f)const{
    if(f>=19&&f<27)return wardrobe&&owned[f-19].size()>1;
    if(!creation&&(f==7||f==9||f==11||f==18))return false;
    if(draft.female&&(f==5||f==6))return false;
    if(f>=15&&f<=17){unsigned bit=f==15?8:f==16?16:64;return !creation&&(equipment&bit);}
    return f>=0&&f!=14;
}
void AppearanceEditor::change(int delta){
    int f=field();
    if(wardrobe&&f>=19&&f<27){
        auto&types=owned[f-19];auto found=std::find(types.begin(),types.end(),outfit[f-19]);int n=int(types.size()),index=int(found-types.begin());
        outfit[f-19]=types[(index+delta%n+n)%n];equipment=Inventory::appearanceKey(outfit);message.clear();return;
    }
if(!enabled(f)){message=(f==7||f==9||f==11||f==18)?"CAMBIA LAS PRENDAS EN INVENTARIO":"OPCION NO DISPONIBLE";return;}uint8_t*p=nullptr;int n=0;
    switch(f){case 1:p=&draft.female;n=2;break;case 2:p=&draft.skin;n=64;break;case 3:p=&draft.hair;n=10;break;case 4:p=&draft.hairColor;n=48;break;case 5:if(!draft.female){p=&draft.beard;n=8;}break;case 6:if(!draft.female){p=&draft.beardColor;n=48;}break;case 7:p=&draft.shirt;n=3;break;case 8:p=&draft.shirtColor;n=32;break;case 9:p=&draft.pants;n=3;break;case 10:p=&draft.pantsColor;n=32;break;case 11:p=&draft.shoes;n=2;break;case 12:p=&draft.bootsColor;n=32;break;case 13:p=&draft.robeColor;n=32;break;case 15:p=&draft.hatColor;n=32;break;case 16:p=&draft.neckColor;n=32;break;case 17:p=&draft.glovesColor;n=32;break;case 18:p=&draft.robe;n=2;break;default:break;}
    if(p)*p=(*p+delta+n)%n;
    message.clear();
}
void AppearanceEditor::randomize(uint32_t seed){auto old=draft;auto next=[&](unsigned n){seed=seed*1664525+1013904223;return (seed>>8)%n;};draft.female=next(2);draft.skin=next(64);draft.hair=next(10);draft.hairColor=next(48);draft.beard=next(8);draft.beardColor=draft.hairColor;draft.robe=next(2);draft.shirt=1+next(2);draft.pants=1+next(2);draft.shoes=1;draft.shirtColor=next(32);draft.pantsColor=next(32);draft.bootsColor=next(32);draft.robeColor=next(32);draft.hatColor=next(32);draft.neckColor=next(32);draft.glovesColor=next(32);if(!enabled(15))draft.hatColor=old.hatColor;if(!enabled(16))draft.neckColor=old.neckColor;if(!enabled(17))draft.glovesColor=old.glovesColor;if(!creation){draft.robe=old.robe;draft.shirt=old.shirt;draft.pants=old.pants;draft.shoes=old.shoes;}}
std::string AppearanceEditor::label(int f)const{if(f>=19&&f<27){const char*slots[]={"ARMA","TUNICA","CALZADO","GORRO","CUELLO","PECHO","GUANTES","INFERIOR"};return slots[f-19];}const char*labels[]={"NOMBRE","CUERPO","PIEL","PEINADO","COLOR DEL PELO","BARBA","COLOR DE BARBA","CAMISA","COLOR CAMISA","PRENDA INFERIOR","COLOR INFERIOR","CALZADO","COLOR CALZADO","COLOR TUNICA","","COLOR GORRO","COLOR CUELLO","COLOR GUANTES","TUNICA"};return f>=0&&f<19?labels[f]:"";}
std::string AppearanceEditor::value(int f)const{
    if(f>=19&&f<27){int type=outfit[f-19];return type>=0?itemDefs[type].name:"SIN EQUIPAR";}
    const char*mh[]={"SIN PELO","CORTO","LARGO","COLETA","MOHICANO","MEDIA MELENA","NUDO ALTO","AFRO","ENTRADAS","DOS COLETAS"};
    const char*fh[]={"SIN PELO","CORTO","LARGO","COLETA","MOHICANO","MEDIA MELENA","NUDO ALTO","AFRO","DOS COLETAS","MONO"};
    const char*bh[]={"SIN BARBA","CORTA","LARGA","PERILLA","BIGOTE","CORTA Y BIGOTE","LARGA Y BIGOTE","PERILLA Y BIGOTE"};
    auto count=[](int x,int n){return std::to_string(x+1)+" / "+std::to_string(n);};
    if(!creation&&(f==7||f==9||f==11||f==18))return "SOLO INVENTARIO";
    if(!enabled(f))return "NO DISPONIBLE";
    switch(f){case 18:return draft.robe?"SI":"NO";case 15:return count(draft.hatColor,32);case 16:return count(draft.neckColor,32);case 17:return count(draft.glovesColor,32);case 0:return draft.name;case 1:return draft.female?"MUJER HUMANA":"HOMBRE HUMANO";case 2:return count(draft.skin,64);case 3:return (draft.female?fh:mh)[draft.hair];case 4:return count(draft.hairColor,48);case 5:return draft.female?"NO DISPONIBLE":bh[draft.beard];case 6:return draft.female?"NO DISPONIBLE":count(draft.beardColor,48);case 7:return draft.shirt==0?"SIN CAMISA":draft.shirt==1?"CAMISA":"CAMISA ELEGANTE";case 8:return count(draft.shirtColor,32);case 9:return draft.pants==0?"SIN PRENDA":draft.pants==1?"PANTALON":"FALDA";case 10:return count(draft.pantsColor,32);case 11:return draft.shoes?"ZAPATOS":"DESCALZO";case 12:return count(draft.bootsColor,32);case 13:return count(draft.robeColor,32);default:return "";}
}
int AppearanceEditor::hit(int x,int y)const{if(x<12||x>=308)return -1;if(y>=27&&y<49)return 10+std::min(3,(x-12)/74);if(y>=57&&y<182){int r=(y-57)/step();if(r>=rowCount())return -1;return y<57+r*step()+step()-2?r:-1;}if(y>=207&&y<231)return x<160?20:21;return -1;}
bool CharacterAssets::load(const std::string&p){
    std::ifstream f(p+"hues.bin",std::ios::binary);std::vector<uint8_t>b(16+3000*64);palettes.clear();cache.clear();cacheUsed.clear();cacheClock=cacheBytes=0;layerScratch.clear();
    if(!f.read((char*)b.data(),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UHP1",4)||get(b,4)!=3000||get(b,8)!=3000*64||get(b,12)!=hash(b.data()+16,b.size()-16))return false;
    palettes.resize(3000*32);for(size_t i=0;i<palettes.size();++i)palettes[i]=b[16+i*2]|uint16_t(b[17+i*2])<<8;root=p;return true;
}
bool CharacterAssets::fullPaperdoll()const{
    for(int sex=0;sex<2;++sex)for(int id:{400+sex,700,701,702,703,710,903,900,901,902,712,404,546,545,434,435,430,449,480,477,469,622}){
        std::ifstream f(root+std::to_string(sex)+"-"+std::to_string(id)+".bin",std::ios::binary);char magic[4];if(!f.read(magic,4)||std::memcmp(magic,"UAP3",4))return false;
    }
    return ready();
}
uint16_t CharacterAssets::color(const Appearance&a,int field)const{switch(field){case 15:return clothColors[a.hatColor];case 16:return clothColors[a.neckColor];case 17:return clothColors[a.glovesColor];case 2:return skins[a.skin];case 4:return hairColors[a.hairColor]+1;case 6:return hairColors[a.beardColor]+1;case 8:return clothColors[a.shirtColor];case 10:return clothColors[a.pantsColor];case 12:return clothColors[a.bootsColor];case 13:return clothColors[a.robeColor];default:return 0;}}
bool CharacterAssets::room(size_t bytes)const{
#ifdef __3DS__
    auto available=[](){auto info=mallinfo();return __ctru_heap_size>size_t(info.uordblks)?__ctru_heap_size-size_t(info.uordblks):0;};
    if(available()<bytes+2*1024*1024)trimCache();
    return available()>=bytes+2*1024*1024;
#else
    (void)bytes;return true;
#endif
}
bool CharacterAssets::overlay(std::vector<uint16_t>&out,const std::string&file,int sample,uint16_t hue,bool partial,size_t destination)const{
    auto found=cache.find(file);
    if(found==cache.end()){
        std::ifstream f(root+file,std::ios::binary|std::ios::ate);if(!f||f.tellg()<604||f.tellg()>8*1024*1024)return false;
        const size_t incoming=size_t(f.tellg());if(incoming>CacheBudget)return false;
        while(!cache.empty()&&(cache.size()>=24||cacheBytes+incoming>CacheBudget)){auto oldest=std::min_element(cacheUsed.begin(),cacheUsed.end(),[](const auto&a,const auto&b){return a.second<b.second;});auto victim=cache.find(oldest->first);cacheBytes-=victim->second.size();cache.erase(victim);cacheUsed.erase(oldest);}
        if(!room(incoming))return false;
        std::vector<uint8_t>b(incoming);f.seekg(0);f.read((char*)b.data(),b.size());
        bool mount=!std::memcmp(b.data(),"UAM1",4);
        if(!f||(!mount&&std::memcmp(b.data(),"UAP1",4)&&std::memcmp(b.data(),"UAP2",4)&&std::memcmp(b.data(),"UAP3",4))||
           (b[8]|unsigned(b[9])<<8)!=(mount?256u:144u)||b[10]||b[11]||b.size()<(mount?1048u:604u)||get(b,12)!=b.size()-20||get(b,16)!=hash(b.data()+20,b.size()-20))return false;
        cacheBytes+=b.size();found=cache.emplace(file,std::move(b)).first;
    }
    cacheUsed[file]=++cacheClock;const auto&b=found->second;
    bool mount=!std::memcmp(b.data(),"UAM1",4);size_t header=mount?1048:604;
    if(sample<0||sample>(mount?255:144)||hue>3000)return false;
    uint32_t begin=get(b,20+sample*4),end=get(b,24+sample*4);if(begin>end||end>b.size()-header)return false;
    bool full=b[3]=='3',extended=b[3]=='2';
    auto&layer=layerScratch;layer.assign(mount?128*128:sample==144?(full?260*237:106*(extended?198:174)):96*112,0);
    if(!sparse(b,header+begin,header+end,layer))return false;
    if(!mount&&sample==144&&!full){
        if(out.size()!=260*237)return false;
        std::vector<uint16_t>expanded(260*237);int y0=extended?28:52,h=extended?198:174;
        for(int y=0;y<h;++y)std::copy(layer.begin()+y*106,layer.begin()+(y+1)*106,expanded.begin()+(y+y0)*260+38);
        layer=std::move(expanded);
    }
    if(destination>out.size()||out.size()-destination<layer.size())return false;
    for(size_t i=0;i<layer.size();++i)if(layer[i]&1){uint16_t c=layer[i];if(hue&&(!partial||((c>>11)==(c>>6&31)&&(c>>11)==(c>>1&31)))){uint16_t h=palettes[(hue-1)*32+(c>>11)];c=((h>>10&31)<<11)|((h>>5&31)<<6)|((h&31)<<1)|1;}out[destination+i]=c;}
    return true;
}
bool CharacterAssets::compose(const Appearance&a,unsigned mask,std::vector<uint16_t>&out,bool doll,int onlyFrame,bool combat,const LayerPainter&painter)const{
    bool mounted=!doll&&(mask&MountedBank);mask&=~MountedBank;
    if(!ready()||!a.valid()||(mask&~EquipmentAppearanceMask)||onlyFrame>int(mounted?MountedFrames-1:71))return false;
    const size_t area=doll?260*237:mounted?128*128:96*112;int count=doll||onlyFrame>=0?1:mounted?MountedFrames:72;
    const size_t output=area*(painter?1:count);
    if(out.capacity()<output&&!room(output*sizeof(uint16_t)))return false;
    out.assign(output,0);
    std::vector<std::pair<int,uint16_t>>layers;
    layers.push_back({400+a.female,color(a,2)});
    if(!a.realClothes&&a.shoes&&!(mask&4))layers.push_back({480,color(a,12)});
    if(mask&4)layers.push_back({(mask&1024)?480:477,color(a,12)});
    if((a.realClothes&&(mask&128))||(!a.realClothes&&a.pants))layers.push_back({a.realClothes?((mask&512)?449:430):(a.pants==1?430:449),color(a,10)});
    const unsigned armor=(mask>>15)&3;
    if((a.realClothes&&(mask&32))||(!a.realClothes&&a.shirt))layers.push_back({a.realClothes?(armor?std::array<int,3>{{542,538,527}}[armor-1]:((mask&256)?435:434)):(a.shirt==1?434:435),uint16_t(armor>=2?0:color(a,8))});
    unsigned sleeves=(mask>>17)&3;
    if(sleeves>2)return false;
    if(sleeves)layers.push_back({sleeves==1?544:528,uint16_t(sleeves==1?color(a,8):0)});
    if(doll&&(mask&(1u<<19)))layers.push_back({498,0});
    if(doll&&(mask&(1u<<20)))layers.push_back({495,0});
    if(mask&16)layers.push_back({546,color(a,16)});
    if(mask&64)layers.push_back({545,color(a,17)});
    if(mask&2)layers.push_back({469,color(a,13)});
    if(a.beard&&!a.female)layers.push_back({beards[a.beard],color(a,6)});
    // In the paperdoll, keep bulky hair from protruding through the hat crown.
    if(a.hair&&!(doll&&(mask&8)))layers.push_back({(a.female?femaleHair:maleHair)[a.hair],color(a,4)});
    if(mask&8)layers.push_back({404,color(a,15)});
    if(mask&1)layers.push_back({std::array<int,4>{{622,618,631,653}}[(mask>>13)&3],0});
    // Layer-major composition keeps each source resident even for mounted atlases.
    // Weapon goes behind the rider in rear views, but always in front of the horse.
    auto rear=[&](int frame){static const int directions[]={3,2,1,0,1,2,3,4};unsigned d=mounted?(frame<88?frame/11:(frame-88)/5):frame<40?frame/5:(frame-40)/4;return !doll&&directions[d]>=3;};
    auto paint=[&](int graphic,uint16_t hue,int weaponPass=0){
        for(int i=0;i<count;++i){int frame=onlyFrame>=0?onlyFrame:i;
            if(weaponPass&&(rear(frame)!=(weaponPass<0)))continue;
            int sample=doll?144:frame+((mask&1)?(mounted?128:72):0);
            const std::string folder=mounted?"mounted/":combat&&!doll?"combat/":"";
            int sex=graphic==200&&mounted?0:a.female;
            if(painter)std::fill(out.begin(),out.end(),0);
            if(!overlay(out,folder+std::to_string(sex)+"-"+std::to_string(graphic)+".bin",sample,hue,false,painter?0:i*area))return false;
            if(painter)painter(unsigned(i),out);
        }return true;
    };
    if(mounted&&!paint(200,0)){out.clear();return false;}
    std::pair<int,uint16_t>weapon{};if(mask&1){weapon=layers.back();layers.pop_back();if(!paint(weapon.first,weapon.second,-1)){out.clear();return false;}}
    for(auto l:layers)if(!paint(l.first,l.second)){out.clear();return false;}
    if((mask&1)&&!paint(weapon.first,weapon.second,1)){out.clear();return false;}
    return true;
}
bool CharacterAssets::portrait(const Appearance&a,unsigned key,std::vector<uint8_t>&out)const{
    std::vector<uint16_t>d;if(!compose(a,key,d,true))return false;out.assign(76*104*4,0);
    int left=260,top=237,right=0,bottom=0;
    for(int y=0;y<237;++y)for(int x=0;x<260;++x)if(d[y*260+x]&1){left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+1);bottom=std::max(bottom,y+1);}
    if(left>=right||top>=bottom)return false;
    int width=right-left,height=bottom-top,w=std::min(76,std::max(1,width*104/height)),h=std::min(104,std::max(1,height*w/width));
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){uint16_t c=d[(top+y*height/h)*260+left+x*width/w];size_t at=((y+104-h)*76+x+(76-w)/2)*4;out[at]=(c>>11)*255/31;out[at+1]=(c>>6&31)*255/31;out[at+2]=(c>>1&31)*255/31;out[at+3]=(c&1)?255:0;}
    return true;
}
bool CharacterAssets::dolls(const Appearance&a,std::vector<uint8_t>&out)const{
    out.clear();std::vector<uint8_t>d;for(unsigned mask=0;mask<8;++mask){if(!portrait(a,mask,d))return false;out.insert(out.end(),d.begin(),d.end());}return true;
}
}
