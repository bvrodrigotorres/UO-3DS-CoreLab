// SPDX-License-Identifier: MIT
#pragma once
#include "journal.hpp"
#include "inventory.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace lab {
struct HudInfo {std::string scene,roofs;std::vector<std::string> lines;};
// Writes a 320x240 BGR8 framebuffer in the native 3DS column layout.
// No GPU state, extra textures, external font files or original UO artwork.
void drawHud(uint8_t*framebuffer,const HudInfo&);
void drawJournal(uint8_t*framebuffer,const World&,const Actor&,const Journal*,int zoom,const std::string&status="");
void drawInventory(uint8_t*,const World&,const Actor&,const Inventory&,const ItemIcons* = nullptr);
}
