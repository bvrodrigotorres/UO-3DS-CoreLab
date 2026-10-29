// SPDX-License-Identifier: MIT
// Native inspection tool; this does not activate the archive in the game.
#include "world_archive.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
using namespace lab;
void append(std::vector<uint8_t>&b,uint64_t value,unsigned bytes){for(unsigned i=0;i<bytes;++i)b.push_back(uint8_t(value>>(8*i)));}
uint32_t hash(const std::vector<uint8_t>&b){uint32_t h=2166136261;for(auto v:b)h=(h^v)*16777619u;return h;}
int main(int argc,char**argv){
 if(argc!=5){std::cerr<<"Usage: world_archive_probe archive.uow x y side\n";return 2;}
 WorldArchive archive;World world;std::string error;
 if(!archive.open(argv[1],error)||!archive.window(std::atoi(argv[2]),std::atoi(argv[3]),unsigned(std::atoi(argv[4])),world,error)){std::cerr<<error<<"\n";return 1;}
 std::vector<uint8_t>terrain,statics;std::vector<std::vector<uint8_t>>rows;
 for(int y=0;y<=world.height;++y)for(int x=0;x<=world.width;++x){append(terrain,world.vertex(x,y),1);if(x<world.width&&y<world.height){const auto&l=world.land[y*world.width+x];append(terrain,l.art,2);append(terrain,l.flags,8);append(terrain,l.texture,1);}}
 for(const auto&o:world.objects){std::vector<uint8_t>b;append(b,o.art,2);append(b,o.x,2);append(b,o.y,2);append(b,o.z,1);append(b,o.height,1);append(b,o.hue,2);append(b,o.flags,8);rows.push_back(b);}
 std::sort(rows.begin(),rows.end());for(const auto&row:rows)statics.insert(statics.end(),row.begin(),row.end());
 std::printf("{\"terrain\":%u,\"statics\":%u,\"objects\":%zu,\"cache\":%zu,\"reads\":%u}\n",hash(terrain),hash(statics),world.objects.size(),archive.cacheBytes(),archive.diskReads());
}
