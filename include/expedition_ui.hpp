// SPDX-License-Identifier: MIT
#pragma once
#include "renderer.hpp"
#include "world_expedition.hpp"
namespace lab {
// Regional NPC pilot while the campaign is paused; separate inventory/vitals.
std::string exploreBritannia(C3D_RenderTarget*,const std::string&base,const std::string&save,const Actor&start,const PlayerStatus&,const Inventory&,const EquipmentArt&,const StatusArt&,RoofMode,const Npcs&,const World&,const ItemIcons&,const SaveSink& = {});
}
