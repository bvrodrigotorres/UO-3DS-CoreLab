// SPDX-License-Identifier: MIT
#include "journal.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstring>
using namespace lab;
static uint32_t fnv(const std::vector<uint8_t>&b,size_t start=0){uint32_t h=2166136261u;for(size_t i=start;i<b.size();++i)h=(h^b[i])*16777619u;return h;}
static void put(std::vector<uint8_t>&b,int p,uint32_t n,int count){for(int i=0;i<count;++i)b[p+i]=(n>>(8*i))&255;}
static void write(const std::string&path,const std::vector<uint8_t>&b){std::ofstream f(path,std::ios::binary);f.write((const char*)b.data(),b.size());}
int main(int argc,char**argv){
    assert(argc==2);std::string folder=argv[1],scene=folder+"/journal-scene",path=folder+"/journal.bin",error;
    std::vector<uint8_t>world={1,2,3};write(scene,world);
    std::vector<uint8_t>b(28+2*2*3+1*1*4,42);std::memcpy(b.data(),"UOJ1",4);
    put(b,4,2,2);put(b,6,2,2);put(b,8,1488,4);put(b,12,1596,4);put(b,16,fnv(world),4);put(b,20,fnv(b,28),4);put(b,24,1,2);put(b,26,1,2);
    write(path,b);Journal j;assert(loadJournal(path,scene,j,error));assert(j.width==2&&j.originX==1488&&j.map.size()==12&&j.doll.size()==4);
    auto invalid=[&](std::vector<uint8_t>v){write(path,v);Journal keep;assert(!loadJournal(path,scene,keep,error));assert(keep.map.empty()&&keep.doll.empty());};
    for(size_t n:{size_t(0),size_t(27),b.size()-1})invalid(std::vector<uint8_t>(b.begin(),b.begin()+n));
    auto v=b;v.push_back(1);invalid(v);
    v=b;v[28]^=1;invalid(v);
    v=b;put(v,4,0,2);invalid(v);
    v=b;put(v,24,129,2);invalid(v);
    v=b;put(v,16,1234,4);invalid(v);
    std::cout<<"Journal dimensions, payload integrity and scene identity passed\n";
}
