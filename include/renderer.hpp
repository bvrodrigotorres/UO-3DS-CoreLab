// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include "visibility.hpp"
#include "doors.hpp"
#include "npcs.hpp"
#include "equipment.hpp"
#include "character.hpp"
#include "combat.hpp"
#include <citro2d.h>
namespace lab {
bool rendererInit();
bool loadGroundIcons(const ItemIcons&);
void rendererShutdown();
bool prepareEquipment(unsigned,const EquipmentArt&,std::string&);
bool selectEquipment(unsigned);
void clearAppearance();
bool applyAppearance(const CharacterAssets&,const Appearance&,EquipmentArt&,unsigned,std::string&);
void warmEquipment(unsigned,const EquipmentArt&);
bool loadPopulationAtlases(const Npcs&,const std::string&,std::string&);
bool loadBloodArt(const std::string&,std::string&);
bool loadCombatArt(const std::string&root,const std::string&population,std::string&error);
bool loadNpcAtlases(std::string&error);
bool loadAtlases(int images,std::string&error,const std::string&root="sdmc:/3ds/UO-3DS-CoreLab/");
size_t textureBytes();
void drawWorld(C3D_RenderTarget*,const World&,const Actor&,float cameraX,float cameraY,float footZ,
               RoofMode roofs,int direction,int frame,bool authentic,bool moving,bool running,const Npcs*npcs=nullptr,const Combat*combat=nullptr,const PlayerStatus*status=nullptr,const Inventory*inventory=nullptr);
size_t drawnObjects();
bool rendererOverflow();
bool rendererTextureError();
bool rendererNpcPoseStreaming();
unsigned rendererResourceWaits();
unsigned rendererUploads();
bool hasRunAnimation();
unsigned renderListBuilds();
void invalidateRenderList();
}
