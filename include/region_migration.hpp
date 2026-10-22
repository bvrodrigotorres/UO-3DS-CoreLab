// SPDX-License-Identifier: MIT
#pragma once
#include "character.hpp"
#include "startup.hpp"
namespace lab {
bool inheritRegionInventory(const World&oldWorld,const Inventory&oldInventory,const World&newWorld,Inventory&next,std::string&error);
// Activates imported files only after every save succeeds; the old region is read-only.
bool migrateRegion(const std::string&oldRoot,const std::string&newRoot,const World&,std::string&message);
}
