// SPDX-License-Identifier: MIT
#include "world_archive.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
using namespace lab;
namespace {
void put(std::vector<uint8_t>&b,size_t p,uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)b[p+i]=uint8_t(v>>(8*i));}
uint32_t hash(const uint8_t*p,size_t n,uint32_t h=2166136261){for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
std::vector<uint8_t>fixture(){
 const unsigned count=8,tables=64,index=64+4*12+104*12,data=index+count*24;std::vector<uint8_t>b(data);b[0]='U';b[1]='O';b[2]='W';b[3]='1';put(b,4,1,2);put(b,6,64,2);
 const unsigned fields[]={128,256,2,4,4,104,count};for(unsigned i=0;i<7;++i)put(b,8+i*4,fields[i],4);put(b,40,tables,8);put(b,48,index,8);put(b,56,data,8);put(b,64+3*12+8,7,2);
 put(b,64+4*12+100*12,Impassable,8);put(b,64+4*12+100*12+8,20,1);
 for(unsigned key=0;key<count;++key){unsigned ox=(key/4)*64,oy=(key%4)*64;size_t begin=b.size();b.resize(begin+65*65*3+7);
  for(unsigned y=0;y<=64;++y)for(unsigned x=0;x<=64;++x){size_t p=begin+(y*65+x)*3;put(b,p,3,2);put(b,p+2,(std::min(127u,ox+x)+std::min(255u,oy+y))/8%16,1);}
  size_t at=begin+65*65*3;put(b,at,100,2);b[at+2]=10;b[at+3]=10;b[at+4]=0;put(b,at+5,123,2);
  size_t entry=index+key*24;put(b,entry,begin,8);put(b,entry+8,b.size()-begin,4);put(b,entry+12,hash(b.data()+begin,b.size()-begin),4);put(b,entry+16,1,4);put(b,entry+20,key,4);
 }
 put(b,36,hash(b.data()+64,data-64),4);return b;
}
void write(const std::string&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write((char*)b.data(),b.size());}
}
int main(int argc,char**argv){
 assert(argc>=2);std::string root=std::string(argv[1])+"/",error;auto bytes=fixture();write(root+"world.uow",bytes);WorldArchive archive;
 assert(archive.open(root+"world.uow",error)&&archive.width()==128&&archive.height()==256&&archive.sectorCount()==8&&archive.objectCount()==8);
 World view;assert(archive.window(0,0,64,view,error));assert(view.objects.size()==1&&view.objects[0].hue==123&&view.objects[0].flags==Impassable&&view.objects[0].height==20&&view.objects[0].image==NoImage);
 assert(view.vertices.size()==65*65&&view.land.size()==64*64&&view.land[0].image==NoImage);
 for(int y=0;y<=64;++y)for(int x=0;x<=64;++x)assert(view.vertex(x,y)==(x+y)/8%16);
 auto disk=archive.diskReads();assert(archive.window(0,0,64,view,error)&&archive.diskReads()==disk);
 assert(archive.window(48,48,64,view,error));assert(view.width==64&&view.originX==48&&view.objects.size()==1&&view.objects[0].order==5);
 World adjacent;assert(archive.window(49,49,64,adjacent,error));for(int y=0;y<63;++y)for(int x=0;x<63;++x)assert(adjacent.vertex(x,y)==view.vertex(x+1,y+1));
 assert(archive.window(0,128,128,view,error)&&view.objects.size()==4);assert(archive.window(0,0,128,view,error)&&view.objects.size()==4);assert(archive.cacheBytes()<=WorldArchive::CacheBudget);
 int previous=view.originY;auto size=view.objects.size();assert(!archive.window(-1,0,64,view,error)&&view.originY==previous&&view.objects.size()==size);assert(!archive.window(0,0,129,view,error));assert(!archive.window(100,250,64,view,error));
 for(int i=0;i<100;++i){assert(archive.window(0,(i%3)*64,128,view,error));assert(archive.cacheBytes()<=WorldArchive::CacheBudget);}
 assert(archive.verify(error));
 auto bad=bytes;bad[0]='X';write(root+"bad.uow",bad);assert(!archive.open(root+"bad.uow",error));assert(archive.width()==128); // failed open preserves current archive
 bad=bytes;bad.resize(bad.size()-1);write(root+"short.uow",bad);WorldArchive rejected;assert(!rejected.open(root+"short.uow",error));
 bad=bytes;bad.back()^=1;write(root+"corrupt.uow",bad);assert(rejected.open(root+"corrupt.uow",error));assert(!rejected.verify(error));
 previous=view.originY;assert(!rejected.window(64,192,64,view,error)&&view.originY==previous&&view.objects.size()==4);
 bad=bytes;put(bad,8,0xffffffffu,4);write(root+"huge.uow",bad);assert(!rejected.open(root+"huge.uow",error));
 bad=bytes;bad[70]^=1;write(root+"table.uow",bad);assert(!rejected.open(root+"table.uow",error));
 if(argc==3){WorldArchive full;assert(full.open(argv[2],error));assert(full.verify(error));unsigned maximum=0;uint64_t seen=0;
  for(int x=0;x<full.width();x+=128)for(int y=0;y<full.height();y+=128){assert(full.window(x,y,128,view,error));maximum=std::max(maximum,unsigned(view.objects.size()));seen+=view.objects.size();assert(full.cacheBytes()<=WorldArchive::CacheBudget);}
  assert(seen==full.objectCount());std::cout<<"FULL ARCHIVE: "<<full.width()<<"x"<<full.height()<<", "<<full.sectorCount()<<" sectors, "<<seen<<" objects; maximum 128x128 objects "<<maximum<<", cache "<<full.cacheBytes()<<" bytes\n";
 }
 std::cout<<"World archive: bounded LRU, stable object IDs, seam continuity, late corruption, invalid dimensions, complete traversal and transactional window replacement passed\n";
}
