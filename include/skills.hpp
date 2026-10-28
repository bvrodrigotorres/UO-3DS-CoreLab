// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
namespace lab {
// Stable classic UO IDs, in tenths: 1000 = 100.0. Reserved systems never train.
enum class Skill:unsigned {Alchemy,Anatomy,AnimalLore,ItemID,ArmsLore,Parrying,Begging,Blacksmith,Bowcraft,Peacemaking,Camping,Carpentry,Cartography,Cooking,DetectHidden,Discordance,EvalInt,Healing,Fishing,Forensics,Herding,Hiding,Provocation,Inscription,Lockpicking,Magery,MagicResist,Tactics,Snooping,Musicianship,Poisoning,Archery,SpiritSpeak,Stealing,Tailoring,AnimalTaming,TasteID,Tinkering,Tracking,Veterinary,Swords,Macing,Fencing,Wrestling,Lumberjacking,Mining,Meditation,Stealth,RemoveTrap,Count};
constexpr unsigned SkillCount=unsigned(Skill::Count);
enum class SkillLock:uint8_t {Up,Down,Locked};
struct SkillDefinition {const char*name;const char*original;bool active;const char*help;};
inline const std::array<SkillDefinition,SkillCount>skillDefs{{
 {"ALQUIMIA","Alchemy",false,"PENDIENTE: FABRICAR POCIONES"},{"ANATOMIA","Anatomy",true,"GOLPES Y VENDAS: PRACTICA Y DANO"},{"SABER ANIMAL","Animal Lore",false,"PENDIENTE: INSPECCION ANIMAL"},{"IDENTIFICACION","Item Identification",false,"PENDIENTE: IDENTIFICAR OBJETOS"},{"SABER DE ARMAS","Arms Lore",false,"PENDIENTE: INSPECCION DE ARMAS"},{"PARRY","Parrying",false,"PENDIENTE: ESCUDOS Y BLOQUEO"},{"MENDICIDAD","Begging",false,"PENDIENTE: PEDIR LIMOSNA"},{"HERRERIA","Blacksmithy",false,"PENDIENTE: FORJA"},{"ARCOS Y FLECHAS","Bowcraft",false,"PENDIENTE: FABRICACION"},{"PACIFICACION","Peacemaking",false,"PENDIENTE: MUSICA"},{"CAMPAMENTO","Camping",false,"PENDIENTE: CAMPAMENTOS"},{"CARPINTERIA","Carpentry",false,"PENDIENTE: FABRICACION"},{"CARTOGRAFIA","Cartography",false,"PENDIENTE: CREAR MAPAS"},{"COCINA","Cooking",false,"PENDIENTE: COCINAR"},{"DETECTAR OCULTOS","Detect Hidden",false,"PENDIENTE: OCULTACION"},{"DISCORDANCIA","Discordance",false,"PENDIENTE: MUSICA"},{"EVALUAR INTELECTO","Evaluating Intelligence",false,"PENDIENTE: MAGIA"},{"CURACION","Healing",true,"VENDAS: 3-1.5 S | 15-25 VIDA"},{"PESCA","Fishing",false,"PENDIENTE: PESCAR"},{"FORENSE","Forensic Evaluation",false,"PENDIENTE: INVESTIGAR CADAVERES"},{"PASTOREO","Herding",false,"PENDIENTE: GUIAR ANIMALES"},{"OCULTARSE","Hiding",false,"PENDIENTE: OCULTACION"},{"PROVOCACION","Provocation",false,"PENDIENTE: MUSICA"},{"INSCRIPCION","Inscription",false,"PENDIENTE: PERGAMINOS"},{"CERRAJERIA","Lockpicking",false,"PENDIENTE: CERRADURAS"},{"MAGIA","Magery",false,"PENDIENTE: HECHIZOS"},{"RESISTIR MAGIA","Resisting Spells",false,"PENDIENTE: ATAQUES MAGICOS"},{"TACTICAS","Tactics",true,"ACERTAR GOLPES: MAS DANO"},{"HUSMEAR","Snooping",false,"PENDIENTE: HURTO"},{"MUSICA","Musicianship",false,"PENDIENTE: INSTRUMENTOS"},{"ENVENENAR","Poisoning",false,"PENDIENTE: VENENOS"},{"ARQUERIA","Archery",false,"PENDIENTE: COMBATE A DISTANCIA"},{"ESPIRITISMO","Spirit Speak",false,"PENDIENTE: ESPIRITUS"},{"ROBAR","Stealing",false,"PENDIENTE: HURTO"},{"SASTRERIA","Tailoring",false,"PENDIENTE: CONFECCION"},{"DOMAR ANIMALES","Animal Taming",false,"PENDIENTE: DOMESTICACION"},{"CATAR","Taste Identification",false,"PENDIENTE: IDENTIFICAR COMIDA"},{"ARTESANIA","Tinkering",false,"PENDIENTE: FABRICACION"},{"RASTREAR","Tracking",false,"PENDIENTE: RASTREO"},{"VETERINARIA","Veterinary",false,"PENDIENTE: CURAR MASCOTAS"},{"ESPADAS","Swordsmanship",true,"ESPADAS Y HACHAS: PRECISION"},{"MAZAS","Mace Fighting",true,"MAZAS: PRECISION"},{"ESGRIMA","Fencing",true,"DAGAS: PRECISION"},{"LUCHA SIN ARMAS","Wrestling",true,"MANOS VACIAS: PRECISION"},{"TALAR","Lumberjacking",false,"PENDIENTE: RECOLECTAR MADERA"},{"MINERIA","Mining",false,"PENDIENTE: RECOLECTAR MINERAL"},{"MEDITACION","Meditation",true,"RECUPERAR MANA: QUIETO, SIN COMBATE"},{"SIGILO","Stealth",false,"PENDIENTE: OCULTACION"},{"DESARMAR TRAMPAS","Remove Trap",false,"PENDIENTE: TRAMPAS"}
}};
inline Skill weaponSkill(int item){return item==5?Skill::Fencing:item==17?Skill::Macing:item==16||item==18?Skill::Swords:Skill::Wrestling;}
inline std::string skillValue(unsigned n){return std::to_string(n/10)+"."+std::to_string(n%10);}
struct SkillState {uint16_t value=0,checks=0;SkillLock lock=SkillLock::Up;float wait=0;};
struct Skills {
    static constexpr unsigned IndividualCap=1000,TotalCap=7000,WireSize=1+4+SkillCount*5;
    std::array<SkillState,SkillCount>entries{};bool ready=false;uint32_t random=0x7315ac39;std::string message;float notice=0;
    unsigned value(Skill s)const{return entries[unsigned(s)].value;}
    unsigned total()const{unsigned n=0;for(auto&s:entries)n+=s.value;return n;}
    void start(int weapon){if(ready)return;ready=true;entries[unsigned(weaponSkill(weapon))].value=500;entries[unsigned(Skill::Tactics)].value=250;entries[unsigned(Skill::Anatomy)].value=250;}
    void tick(float dt){if(dt<=0)return;for(auto&s:entries)s.wait=std::max(0.f,s.wait-dt);notice=std::max(0.f,notice-dt);}
    bool cycle(unsigned index){if(index>=SkillCount)return false;auto&s=entries[index];s.lock=SkillLock((unsigned(s.lock)+1)%3);return true;}
    unsigned nextRandom(){random=random*1664525u+1013904223u;return random>>16;}
    // Chance plus bounded eligible-use guarantee. Difficulty stops easy-target GM farming.
    bool practice(Skill id,int difficulty,bool success=true){
        unsigned i=unsigned(id);if(!ready||i>=SkillCount||!skillDefs[i].active)return false;auto&s=entries[i];
        if(s.lock!=SkillLock::Up||s.value>=IndividualCap||s.wait>0||int(s.value)>difficulty+250)return false;
        s.wait=2;unsigned threshold=8+s.value/40;unsigned chance=std::max(15,400-int(s.value)/3);if(!success)chance/=2;
        if(++s.checks<threshold&&nextRandom()%1000>=chance)return false;
        int donor=-1;if(total()>=TotalCap){for(unsigned j=0;j<SkillCount;++j)if(j!=i&&entries[j].lock==SkillLock::Down&&entries[j].value&&(donor<0||entries[j].value>entries[unsigned(donor)].value))donor=int(j);if(donor<0){s.checks=std::min<unsigned>(s.checks,threshold);message="LIMITE 700.0: MARCA OTRA EN BAJAR";notice=6;return false;}}
        if(donor>=0)--entries[unsigned(donor)].value;
        ++s.value;s.checks=0;message=std::string(skillDefs[i].name)+" +0.1 = "+skillValue(s.value);notice=6;return true;
    }
    int hitChance(Skill id,int opposing)const{if(!ready)return 1000;return std::max(100,std::min(950,int((value(id)+500)*1000/(2*(std::max(0,opposing)+500)))));}
    int damagePercent()const{if(!ready)return 100;unsigned t=value(Skill::Tactics),a=value(Skill::Anatomy);return 100+int(t/16+a/20)+(t>=1000?6:0)+(a>=1000?5:0);}
    int bandageAmount()const{return 15+int(std::min(1000u,ready?value(Skill::Healing):0u)/100);}
    float bandageSeconds()const{return 3.f-1.5f*std::min(1000u,ready?value(Skill::Healing):0u)/1000.f;}
    void encode(std::vector<uint8_t>&b)const{b.push_back(ready);for(unsigned k=0;k<4;++k)b.push_back(uint8_t(random>>(8*k)));for(auto&s:entries){b.push_back(s.value&255);b.push_back(s.value>>8);b.push_back(uint8_t(s.lock));b.push_back(s.checks&255);b.push_back(s.checks>>8);}}
    bool decode(const uint8_t*b,size_t size){if(size!=WireSize||b[0]>1)return false;Skills next;next.ready=b[0];next.random=uint32_t(b[1])|uint32_t(b[2])<<8|uint32_t(b[3])<<16|uint32_t(b[4])<<24;for(unsigned i=0;i<SkillCount;++i){auto&s=next.entries[i];auto p=b+5+i*5;s.value=unsigned(p[0])|unsigned(p[1])<<8;s.lock=SkillLock(p[2]);s.checks=unsigned(p[3])|unsigned(p[4])<<8;if(s.value>IndividualCap||p[2]>2||s.checks>40)return false;}if(next.total()>TotalCap||(!next.ready&&next.total()))return false;*this=std::move(next);return true;}
};
struct SkillsMenu {
    bool all=false;int sheet=0,selected=int(Skill::Swords);std::string message;
    std::vector<unsigned>list()const{std::vector<unsigned>out;for(unsigned i=0;i<SkillCount;++i)if(all||skillDefs[i].active)out.push_back(i);return out;}
    int sheets()const{return (int(list().size())+5)/6;}
    // 1 changed lock; 2 request meditation; 3 navigation only.
    int tap(Skills&s,int x,int y){
        if(x<15||x>=305)return 0;
        if(y>=207&&y<231){if(x<105){all=!all;sheet=0;selected=int(list()[0]);return 3;}if(x<215)return 2;sheet=(sheet+1)%sheets();selected=int(list()[unsigned(sheet*6)]);return 3;}
        if(y>=45&&y<165){auto rows=list();unsigned row=sheet*6+(y-45)/20;if(row>=rows.size())return 0;selected=int(rows[row]);if(x>=264){s.cycle(unsigned(selected));return 1;}return 3;}return 0;
    }
};
}
