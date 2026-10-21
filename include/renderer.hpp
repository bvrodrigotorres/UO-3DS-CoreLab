// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include "visibility.hpp"
#include "doors.hpp"
#include "npcs.hpp"
#include <citro2d.h>
namespace lab {
bool rendererInit();
void rendererShutdown();
bool loadNpcAtlases(std::string&error);
bool loadAtlases(int images,std::string&error);
size_t textureBytes();
void drawWorld(C3D_RenderTarget*,const World&,const Actor&,float cameraX,float cameraY,float footZ,
               RoofMode roofs,int direction,int frame,bool authentic,bool moving,bool running,const Npcs*npcs=nullptr);
size_t drawnObjects();
bool rendererOverflow();
bool hasRunAnimation();
unsigned renderListBuilds();
void invalidateRenderList();
}
