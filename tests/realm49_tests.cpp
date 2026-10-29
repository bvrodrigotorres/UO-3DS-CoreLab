// SPDX-License-Identifier: MIT
#include "combat.hpp"
#include "file_hash.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
using namespace lab;
int main(int argc,char**argv){
 assert(sizeof(Land)==16&&sizeof(Object)==24);
 World large;large.width=large.height=768;large.land.resize(768*768);large.vertices.assign(769*769,0);large.objects.resize(180000);
 for(size_t i=0;i<large.objects.size();++i){large.objects[i].x=int(i*37%768);large.objects[i].y=int(i*17%768);}
 large.index();assert(large.cells.size()==768*768&&large.cells.bytes()<=4*(768*768+1+180000));
 size_t total=0;for(size_t i=0;i<large.cells.size();++i){auto cell=large.cells[i];assert(std::is_sorted(cell.begin(),cell.end()));total+=cell.size();}assert(total==180000);
 std::mt19937 rng(49);
 for(unsigned k=0;k<1000;++k){size_t i=rng()%large.objects.size();auto&o=large.objects[i];int oldX=o.x,oldY=o.y;o.x=rng()%768;o.y=rng()%768;large.moved(i,oldX,oldY);}
 World rebuilt=large;rebuilt.index();assert(rebuilt.cells==large.cells);assert(large.cells.bytes()<5*1024*1024);
 World w;w.width=w.height=8;w.land.resize(64);w.vertices.assign(81,0);for(int y=0;y<8;++y)for(int x=0;x<4;++x)w.land[y*8+x].flags=Wet|Impassable;w.index();
 Actor water{3,3,0},land{4,3,0},next;int actual;
 assert(!canStand(w,water)&&canStandMobile(w,water,true,true)&&!canStandMobile(w,land,true,true));
 assert(!step(w,land,6,next,actual)&&!stepMobile(w,water,2,next,actual,true,true));
 assert(stepMobile(w,water,2,next,actual,true,false)&&next.x==4);assert(stepMobile(w,land,6,next,actual,true,false)&&next.x==3);
 assert(meleeReach(w,land,water)&&meleeReach(w,water,land));
 Object wall;wall.x=3;wall.y=3;wall.z=0;wall.height=20;wall.flags=Wall|Impassable;w.objects.push_back(wall);w.index();assert(!canStandMobile(w,water,true,true)&&!meleeReach(w,land,water));
 if(argc==2){std::string file=std::string(argv[1])+"/hash.bin";std::ofstream f(file,std::ios::binary);std::string block(4096,'x');for(int i=0;i<2304;++i)f.write(block.data(),block.size());f.close();uint32_t hash=0;assert(fileHash(file,24*1024*1024,hash)&&hash);uint32_t before=hash;assert(!fileHash(file,8*1024*1024,hash)&&hash==before);}
 std::cout<<"Realm 49: compact 768-square index, 1000 moving-door edits, land/water movement, coast reach, wall collision and bounded streaming file hashes passed\n";
}
