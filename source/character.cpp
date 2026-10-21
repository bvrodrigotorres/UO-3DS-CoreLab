// SPDX-License-Identifier: MIT
#include "character.hpp"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cerrno>
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
bool Appearance::valid()const{return validName(name)&&female<2&&skin<64&&hair<10&&hairColor<48&&beard<8&&beardColor<48&&shirt<3&&shirtColor<32&&pants<3&&pantsColor<32&&shoes<2&&bootsColor<32&&robeColor<32;}
bool Appearance::load(const std::string&p){
    if(!exists(p)&&!exists(p+".bak"))return true;
    std::ifstream f(exists(p)?p:p+".bak",std::ios::binary);std::vector<uint8_t>b(56);
    if(!f.read((char*)b.data(),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UCA1",4)||get(b,52)!=hash(b.data(),52)||b[4]>1)return false;
    Appearance a;a.enabled=b[4];a.female=b[5];a.skin=b[6];a.hair=b[7];a.hairColor=b[8];a.beard=b[9];a.beardColor=b[10];a.shirt=b[11];a.shirtColor=b[12];a.pants=b[13];a.pantsColor=b[14];a.shoes=b[15];a.bootsColor=b[16];a.robeColor=b[17];
    if(b[18]>16)return false;
    a.name.assign((char*)b.data()+20,b[18]);for(size_t i=20+b[18];i<52;++i)if(b[i])return false;
    if(b[19]||!a.valid())return false;
    *this=a;return true;
}
bool Appearance::save(const std::string&p,std::string&error,SaveRename renameFile)const{
    if(!valid()){error="PERSONAJE INVALIDO";return false;}
    std::vector<uint8_t>b(56);std::memcpy(b.data(),"UCA1",4);b[4]=enabled;b[5]=female;b[6]=skin;b[7]=hair;b[8]=hairColor;b[9]=beard;b[10]=beardColor;b[11]=shirt;b[12]=shirtColor;b[13]=pants;b[14]=pantsColor;b[15]=shoes;b[16]=bootsColor;b[17]=robeColor;b[18]=name.size();std::copy(name.begin(),name.end(),b.begin()+20);put(b,52,hash(b.data(),52));
    auto tmp=p+".tmp",bak=p+".bak";FILE*f=std::fopen(tmp.c_str(),"wb");if(!f){error="ERROR SD: PERSONALIZACION";return false;}
    bool ok=std::fwrite(b.data(),1,b.size(),f)==b.size();if(std::fclose(f))ok=false;
    if(!ok){std::remove(tmp.c_str());error="ERROR SD: PERSONALIZACION";return false;}
    bool had=exists(p);if(had&&((std::remove(bak.c_str())&&errno!=ENOENT)||renameFile(p.c_str(),bak.c_str()))){std::remove(tmp.c_str());error="ERROR SD: RESPALDO";return false;}
    if(renameFile(tmp.c_str(),p.c_str())){if(had)renameFile(bak.c_str(),p.c_str());std::remove(tmp.c_str());error="ERROR SD: CAMBIOS SIN GUARDAR";return false;}
    error.clear();return true;
}
void AppearanceEditor::change(int delta){
    int f=page*5+row;uint8_t*p=nullptr;int n=0;
    switch(f){case 1:p=&draft.female;n=2;break;case 2:p=&draft.skin;n=64;break;case 3:p=&draft.hair;n=10;break;case 4:p=&draft.hairColor;n=48;break;case 5:if(!draft.female){p=&draft.beard;n=8;}break;case 6:if(!draft.female){p=&draft.beardColor;n=48;}break;case 7:p=&draft.shirt;n=3;break;case 8:p=&draft.shirtColor;n=32;break;case 9:p=&draft.pants;n=3;break;case 10:p=&draft.pantsColor;n=32;break;case 11:p=&draft.shoes;n=2;break;case 12:p=&draft.bootsColor;n=32;break;case 13:p=&draft.robeColor;n=32;break;case 14:preview=(preview+delta+3)%3;break;default:break;}
    if(p)*p=(*p+delta+n)%n;
    message.clear();
}
void AppearanceEditor::randomize(uint32_t seed){auto next=[&](unsigned n){seed=seed*1664525+1013904223;return (seed>>8)%n;};draft.female=next(2);draft.skin=next(64);draft.hair=next(10);draft.hairColor=next(48);draft.beard=next(8);draft.beardColor=draft.hairColor;draft.shirt=1+next(2);draft.pants=1+next(2);draft.shoes=1;draft.shirtColor=next(32);draft.pantsColor=next(32);draft.bootsColor=next(32);draft.robeColor=next(32);}
std::string AppearanceEditor::label(int f)const{const char*labels[]={"NOMBRE","CUERPO","PIEL","PEINADO","COLOR DEL PELO","BARBA","COLOR DE BARBA","CAMISA","COLOR CAMISA","PRENDA INFERIOR","COLOR INFERIOR","CALZADO","COLOR CALZADO","COLOR TUNICA","VISTA DE EQUIPO"};return f>=0&&f<15?labels[f]:"";}
std::string AppearanceEditor::value(int f)const{
    const char*mh[]={"SIN PELO","CORTO","LARGO","COLETA","MOHICANO","RAYA AL LADO","CORTO RIZADO","TRENZA","DOS COLETAS","CORTO LISO"};
    const char*fh[]={"SIN PELO","CORTO","LARGO","COLETA","MOHICANO","RAYA AL LADO","CORTO RIZADO","TRENZA","CORTO LISO","MONO"};
    const char*bh[]={"SIN BARBA","CORTA","LARGA","PERILLA","BIGOTE","CORTA Y BIGOTE","LARGA Y BIGOTE","PATILLAS"};
    auto count=[](int x,int n){return std::to_string(x+1)+" / "+std::to_string(n);};
    switch(f){case 0:return draft.name;case 1:return draft.female?"MUJER HUMANA":"HOMBRE HUMANO";case 2:return count(draft.skin,64);case 3:return (draft.female?fh:mh)[draft.hair];case 4:return count(draft.hairColor,48);case 5:return draft.female?"NO DISPONIBLE":bh[draft.beard];case 6:return draft.female?"NO DISPONIBLE":count(draft.beardColor,48);case 7:return draft.shirt==0?"SIN CAMISA":draft.shirt==1?"CAMISA":"CAMISA ELEGANTE";case 8:return count(draft.shirtColor,32);case 9:return draft.pants==0?"SIN PRENDA":draft.pants==1?"PANTALON":"FALDA";case 10:return count(draft.pantsColor,32);case 11:return draft.shoes?"ZAPATOS":"DESCALZO";case 12:return count(draft.bootsColor,32);case 13:return count(draft.robeColor,32);case 14:return preview==0?"EQUIPO ACTUAL":preview==1?"EQUIPO COMPLETO":"SIN EQUIPO";default:return "";}
}
int AppearanceEditor::hit(int x,int y)const{if(x<12||x>=308)return -1;if(y>=27&&y<49)return 10+std::min(2,(x-12)/99);if(y>=57&&y<182){int r=(y-57)/25;return y<57+r*25+23?r:-1;}if(y>=207&&y<231)return x<160?20:21;return -1;}
bool CharacterAssets::load(const std::string&p){
    std::ifstream f(p+"hues.bin",std::ios::binary);std::vector<uint8_t>b(16+3000*64);palettes.clear();cache.clear();
    if(!f.read((char*)b.data(),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UHP1",4)||get(b,4)!=3000||get(b,8)!=3000*64||get(b,12)!=hash(b.data()+16,b.size()-16))return false;
    palettes.resize(3000*32);for(size_t i=0;i<palettes.size();++i)palettes[i]=b[16+i*2]|uint16_t(b[17+i*2])<<8;root=p;return true;
}
uint16_t CharacterAssets::color(const Appearance&a,int field)const{switch(field){case 2:return skins[a.skin];case 4:return hairColors[a.hairColor];case 6:return hairColors[a.beardColor];case 8:return clothColors[a.shirtColor];case 10:return clothColors[a.pantsColor];case 12:return clothColors[a.bootsColor];case 13:return clothColors[a.robeColor];default:return 0;}}
bool CharacterAssets::overlay(std::vector<uint16_t>&out,const std::string&file,int sample,uint16_t hue)const{
    auto found=cache.find(file);
    if(found==cache.end()){
        std::ifstream f(root+file,std::ios::binary|std::ios::ate);if(!f||f.tellg()<604||f.tellg()>8*1024*1024)return false;
        std::vector<uint8_t>b(size_t(f.tellg()));f.seekg(0);f.read((char*)b.data(),b.size());
        if(!f||std::memcmp(b.data(),"UAP1",4)||b[8]!=144||b[9]||b[10]||b[11]||get(b,12)!=b.size()-20||get(b,16)!=hash(b.data()+20,b.size()-20))return false;
        if(cache.size()>=24)cache.clear();
        found=cache.emplace(file,std::move(b)).first;
    }
    const auto&b=found->second;
    if(sample<0||sample>144||hue>3000)return false;
    uint32_t begin=get(b,20+sample*4),end=get(b,24+sample*4);if(begin>end||end>b.size()-604)return false;
    std::vector<uint16_t>layer(sample==144?106*174:96*112);
    if(!sparse(b,604+begin,604+end,layer)||out.size()!=layer.size())return false;
    for(size_t i=0;i<layer.size();++i)if(layer[i]&1){uint16_t c=layer[i];if(hue){uint16_t h=palettes[(hue-1)*32+(c>>11)];c=((h>>10&31)<<11)|((h>>5&31)<<6)|((h&31)<<1)|1;}out[i]=c;}
    return true;
}
bool CharacterAssets::compose(const Appearance&a,unsigned mask,std::vector<uint16_t>&out,bool doll,int onlyFrame)const{
    if(!ready()||!a.valid()||mask>7||onlyFrame>71)return false;
    const size_t area=doll?106*174:96*112;int count=doll||onlyFrame>=0?1:72;out.assign(area*count,0);
    std::vector<std::pair<int,uint16_t>>layers;
    layers.push_back({400+a.female,color(a,2)});
    if(a.shoes&&!(mask&4))layers.push_back({480,color(a,12)});
    if(mask&4)layers.push_back({477,color(a,12)});
    if(a.pants)layers.push_back({a.pants==1?430:449,color(a,10)});
    if(a.shirt)layers.push_back({a.shirt==1?434:435,color(a,8)});
    if(mask&2)layers.push_back({469,color(a,13)});
    if(a.beard&&!a.female)layers.push_back({beards[a.beard],color(a,6)});
    if(a.hair)layers.push_back({(a.female?femaleHair:maleHair)[a.hair],color(a,4)});
    if(mask&1)layers.push_back({622,0});
    // Read each checked sparse layer once, then compose requested poses.
    // overlay() validates each individual frame; files stay outside GPU memory.
    for(int i=0;i<count;++i){int frame=onlyFrame>=0?onlyFrame:i;int sample=doll?144:frame+((mask&1)?72:0);std::vector<uint16_t>canvas(area);
        auto order=layers;
        static const int originalDirections[]={3,2,1,0,1,2,3,4};int d=originalDirections[frame<40?frame/5:(frame-40)/4];
        if(!doll&&(mask&1)&&d>=3){auto weapon=order.back();order.pop_back();order.insert(order.begin(),weapon);}
        for(auto l:order)if(!overlay(canvas,std::to_string(a.female)+"-"+std::to_string(l.first)+".bin",sample,l.second)){out.clear();return false;}
        std::copy(canvas.begin(),canvas.end(),out.begin()+i*area);
    }
    return true;
}
bool CharacterAssets::dolls(const Appearance&a,std::vector<uint8_t>&out)const{
    out.assign(8*76*104*4,0);std::vector<uint16_t>d;
    for(unsigned mask=0;mask<8;++mask){if(!compose(a,mask,d,true)){out.clear();return false;}
        for(int y=0;y<104;++y)for(int x=0;x<63;++x){int sx=x*106/63,sy=y*174/104;uint16_t c=d[sy*106+sx];size_t at=(mask*76*104+y*76+x+6)*4;out[at]=(c>>11)*255/31;out[at+1]=(c>>6&31)*255/31;out[at+2]=(c>>1&31)*255/31;out[at+3]=(c&1)?255:0;}}
    return true;
}
}
