// SPDX-License-Identifier: MIT
#pragma once
#include "combat.hpp"
namespace lab {
class Quests {
    std::array<int,QuestCount>offers{{-1,-1}};
    unsigned topic=0;
public:
    unsigned selected=0;
    std::string message;
    static uint32_t signature(const Npc&);
    static bool cemetery(const World&,const Npc&);
    static unsigned owned(const Inventory&,unsigned);
    void setup(const World&,const Npcs&);
    int giver(unsigned,const Npcs&,const Inventory&)const;
    int nearby(const World&,const Npcs&,const Actor&,const Inventory&)const;
    bool ready(unsigned,const Inventory&)const;
    bool interact(const World&,const Npcs&,const Actor&,Inventory&,SaveRename=std::rename);
    bool killed(const World&,const Npc&,Inventory&,SaveRename=std::rename);
    QuestMarker marker(const Npcs&,const Inventory&)const;
    QuestPanel panel(const World&,const Npcs&,const Actor&,const Inventory&)const;
    void select(unsigned id){selected=id%QuestCount;topic=0;message.clear();}
};
}
