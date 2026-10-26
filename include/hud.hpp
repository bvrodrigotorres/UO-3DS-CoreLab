// SPDX-License-Identifier: MIT
#pragma once
#include "journal.hpp"
#include "inventory.hpp"
#include "equipment.hpp"
#include "startup.hpp"
#include "player_status.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace lab {
void drawSkills(uint8_t*,const Skills&,const SkillsMenu&,bool meditating=false);
class Npcs;struct StatusArt;struct DeveloperMenu;struct InventoryDrag;class Commerce;struct EquipmentMenu;struct Appearance;
inline int equipmentHit(int x,int y){int side=x>=280&&x<305?1:x>=196&&x<220?0:-1;if(side<0||y<30||y>=126)return -1;int row=(y-30)/24;if((y-30)%24>=20)return -1;const int slots[2][4]={{3,4,6,7},{0,1,5,2}};return slots[side][row];}
void drawEquipmentMenu(uint8_t*,uint8_t*,const EquipmentMenu&,const Appearance&,const Inventory&,const PlayerStatus&,const std::vector<uint16_t>&,const std::vector<uint16_t>&);
void drawCommerce(uint8_t*,const Inventory&,const Commerce&,const ItemIcons* = nullptr);
void drawQuests(uint8_t*,const QuestPanel&);
void drawTitle(uint8_t*,bool loading=false);
void drawStartMenu(uint8_t*,const StartMenu&,bool canContinue);
void drawDeveloperMenu(uint8_t*,const DeveloperMenu&,const World&);
void drawPause(uint8_t*,const PauseMenu&);
void drawLoading(uint8_t*,const std::string&,int stage);
struct HudInfo {std::string scene,roofs;std::vector<std::string> lines;};
// Writes a 320x240 BGR8 framebuffer in the native 3DS column layout.
// No GPU state, extra textures, external font files or original UO artwork.
void drawHud(uint8_t*framebuffer,const HudInfo&);
void drawExpeditionHud(uint8_t*,const HudInfo&);
void drawJournal(uint8_t*framebuffer,const World&,const Actor&,const Journal*,int zoom,const std::string&status="",const Inventory* =nullptr,const EquipmentArt* =nullptr,const PlayerStatus* =nullptr,const StatusArt* =nullptr,const Npcs* =nullptr,bool expedition=false);
void drawInventory(uint8_t*,const World&,const Actor&,const Inventory&,const ItemIcons* = nullptr,const InventoryDrag* = nullptr,const PlayerStatus* = nullptr,bool trial=false);
}
