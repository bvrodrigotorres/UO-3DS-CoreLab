// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
namespace lab {
bool savePosition(const std::string&,const World&,const Actor&,std::string&);
bool loadPosition(const std::string&,const World&,Actor&,std::string&);
char mapCell(const World&,const Actor&,int x,int y);
}
