// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <cstdio>
namespace lab {
using SaveRename=int(*)(const char*,const char*);
bool savePosition(const std::string&,const World&,const Actor&,std::string&,SaveRename=std::rename);
bool loadPosition(const std::string&,const World&,Actor&,std::string&);
char mapCell(const World&,const Actor&,int x,int y);
}
