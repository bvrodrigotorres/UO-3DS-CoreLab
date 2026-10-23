// SPDX-License-Identifier: MIT
#pragma once
#include "npcs.hpp"
#include "player_status.hpp"
#include <array>
namespace lab {
struct Corpse {bool active=false;Actor position;int style=0,direction=0;float decay=0;Bag loot;std::string name;};
struct DamageVisual {bool active=false,player=false;Actor position;int npc=-1,amount=0;float age=0;unsigned seed=0;};
struct BloodPool {bool active=false;Actor position;float age=0;unsigned variant=0;};
struct MobVitals {int hits=0,maximum=0;float respawn=0,cooldown=1,flash=0;};
class Combat {
    std::vector<MobVitals>mob;float swing=0,regen=0,strike=0;uint32_t random=0x234567;
    bool automaticDefense=false,suppressDefense=false;float healCooldown=0;
    void defend(Npcs&,const Actor&);
    void damageVisual(const Actor&,int amount,bool player,int npc);
    int roll(int lo,int hi);void kill(size_t,Npcs&,Inventory&);
public:
    std::array<DamageVisual,32>damage{};std::array<BloodPool,32>blood{};
    std::array<Corpse,32>corpses{};int target=-1;bool attacking=false;std::string message;
    bool useHealing(Inventory&,size_t,PlayerStatus&,SaveRename=std::rename);
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
bool saveVitals(const std::string&,const PlayerStatus&,SaveRename=std::rename);
struct StatusArt {std::vector<uint8_t>rgba;bool load(const std::string&);};
}
