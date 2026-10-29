// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cstddef>
namespace lab {
constexpr size_t BaseItemTypes=30, ItemPowers=10;
inline constexpr std::array<int,21>GearBases{{5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,26,27,28,29}};
constexpr size_t ItemTypes=BaseItemTypes+GearBases.size()*4*ItemPowers;
inline int itemBase(int type){return type<0?-1:type<int(BaseItemTypes)?type:type<int(ItemTypes)?GearBases[(type-BaseItemTypes)%GearBases.size()]:-1;}
inline unsigned itemQuality(unsigned type){return type<BaseItemTypes?0:type<ItemTypes?1+(type-BaseItemTypes)/(GearBases.size()*ItemPowers):0;}
inline unsigned itemPower(unsigned type){return type<BaseItemTypes?0:((type-BaseItemTypes)/GearBases.size())%ItemPowers;}
inline unsigned magicItem(unsigned base,unsigned quality,unsigned power){if(!quality)return base;for(unsigned i=0;i<GearBases.size();++i)if(unsigned(GearBases[i])==base&&quality<=4&&power<ItemPowers)return BaseItemTypes+i+GearBases.size()*(power+ItemPowers*(quality-1));return base;}
}
