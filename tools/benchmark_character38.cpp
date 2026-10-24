// SPDX-License-Identifier: MIT
#include "character.hpp"
#include <chrono>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){if(argc!=2)return 1;CharacterAssets assets;if(!assets.load(argv[1]))return 2;Appearance a;a.enabled=a.realClothes=true;a.hair=3;unsigned masks[]={0,1|4|8|16|32|64|128,1|2|4|8|16|32|64|128|(1u<<17),1|4|8|16|32|64|128|(3u<<15)|(2u<<17)|(1u<<19)|(1u<<20)};uint32_t hash=2166136261u;size_t pixels=0;auto start=std::chrono::steady_clock::now();
 for(unsigned sex=0;sex<2;++sex){a.female=sex;a.beard=sex?0:1;for(unsigned mask:masks)for(int bank=0;bank<3;++bank){std::vector<uint16_t>out;if(!assets.compose(a,mask,out,bank==2,-1,bank==1))return 3;for(auto pixel:out){hash=(hash^pixel)*16777619u;}pixels+=out.size();}}
 std::cout<<"character_ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<",pixels="<<pixels<<",hash="<<hash<<"\n";
}
