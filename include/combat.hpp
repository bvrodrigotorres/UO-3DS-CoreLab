// SPDX-License-Identifier: MIT
#pragma once
#include "npcs.hpp"
#include "player_status.hpp"
#include <array>
namespace lab {
struct Corpse {bool active=false;Actor position;int style=0,direction=0;float decay=0;Bag loot;std::string name;};
enum class CombatText {Damage,Miss,Healing};
struct DamageVisual {bool active=false,player=false;Actor position;int npc=-1,amount=0;float age=0;unsigned seed=0;CombatText kind=CombatText::Damage;};
inline std::string combatText(const DamageVisual&e){return e.kind==CombatText::Miss?"MISS":e.kind==CombatText::Healing?"+"+std::to_string(e.amount):std::to_string(e.amount);}
inline std::array<float,3>combatTextColor(const DamageVisual&e){if(e.kind==CombatText::Healing)return {{.25f,.85f,1.f}};if(e.player)return {{1.f,.18f,.12f}};return e.kind==CombatText::Miss?std::array<float,3>{{1.f,1.f,0.f}}:std::array<float,3>{{1.f,.9f,.2f}};}
struct BloodPool {bool active=false;Actor position;float age=0;unsigned variant=0;};
struct MobVitals {int hits=0,maximum=0;float respawn=0,cooldown=1,flash=0;};
class Combat {
    std::vector<MobVitals>mob;float swing=0,regen=0,strike=0;uint32_t random=0x234567;
    bool automaticDefense=false,suppressDefense=false;float healCooldown=0;
    void defend(Npcs&,const Actor&);
    Actor playerPosition{};unsigned visualSerial=0;
    void damageVisual(const Actor&,int amount,bool player,int npc,CombatText kind=CombatText::Damage);
    int roll(int lo,int hi);void kill(size_t,Npcs&,Inventory&);
public:
    std::array<DamageVisual,32>damage{};std::array<BloodPool,32>blood{};
    std::array<Corpse,32>corpses{};int target=-1;bool attacking=false;std::string message;
    void locate(const Actor&a){playerPosition=a;}
    bool useHealing(Inventory&,size_t,PlayerStatus&,SaveRename=std::rename);
    bool meditating=false;float meditationPulse=0;
    bool meditate(PlayerStatus&,const Inventory&);
    bool repeatHealing(Inventory&,PlayerStatus&,SaveRename=std::rename);
    float healingWait()const{return healCooldown;}
    bool striking()const{return strike>0;}
    bool combatPose()const{return attacking||striking();}
    int strikeFrame()const{return std::max(0,std::min(3,int((.45f-strike)*4/.45f)));}
    void reset(const Npcs&);
    bool attackable(const Npc&n)const{return n.alive&&n.animal;}
    void select(const Npcs&,const Actor&,bool cycle=false);
    void toggle(const Npcs&,const Actor&);
    // True when player died and must be relocated to the world's reset point.
    bool update(const World&,Npcs&,Actor&,bool moving,float,PlayerStatus&,Inventory&);
    const std::vector<MobVitals>&vitals()const{return mob;}
    int nearestCorpse(const World&,const Actor&)const;
    bool inspectNearest(const World&,const Actor&,Inventory&);
    bool openCorpse(const World&,const Actor&,Inventory&);
};
bool meleeReach(const World&,const Actor&,const Actor&);
bool loadVitals(const std::string&,PlayerStatus&);
bool saveVitals(const std::string&,const PlayerStatus&,SaveRename=std::rename,const SaveSink& = {});
struct StatusArt {std::vector<uint8_t>rgba;bool load(const std::string&);};
}
