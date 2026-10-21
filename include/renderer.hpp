// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <citro2d.h>
namespace lab {
bool rendererInit();
void rendererShutdown();
bool loadAtlases(int images,std::string&error);
size_t textureBytes();
void drawWorld(C3D_RenderTarget*,const World&,const Actor&,float cameraX,float cameraY,float footZ,
               bool cutaway,int direction,int frame,bool authentic,bool moving,bool running);
size_t drawnObjects();
bool rendererOverflow();
bool hasRunAnimation();
unsigned renderListBuilds();
}
