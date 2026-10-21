// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include "visibility.hpp"
#include <citro2d.h>
namespace lab {
bool rendererInit();
void rendererShutdown();
bool loadAtlases(int images,std::string&error);
size_t textureBytes();
void drawWorld(C3D_RenderTarget*,const World&,const Actor&,float cameraX,float cameraY,float footZ,
               RoofMode roofs,int direction,int frame,bool authentic,bool moving,bool running);
size_t drawnObjects();
bool rendererOverflow();
bool hasRunAnimation();
unsigned renderListBuilds();
}
