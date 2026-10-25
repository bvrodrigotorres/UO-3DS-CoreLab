// SPDX-License-Identifier: MIT
#pragma once
#include "renderer.hpp"
#include "world_expedition.hpp"
namespace lab {
// Runs while the campaign is paused. No reference to mutable inventory/NPCs.
std::string exploreBritannia(C3D_RenderTarget*,const std::string&base,const std::string&save,const Actor&start,const PlayerStatus&,const Inventory&,const EquipmentArt&,const StatusArt&,RoofMode);
}
