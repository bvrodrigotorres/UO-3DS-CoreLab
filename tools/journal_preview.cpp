// SPDX-License-Identifier: MIT
#include "hud.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){
    if(argc!=4)return 2;
    lab::World w;lab::Journal j;std::string e;
    if(!lab::loadWorld(argv[1],w,e)||!lab::loadJournal(argv[2],argv[1],j,e)){std::cerr<<e;return 1;}
    std::vector<uint8_t>b(320*240*3);lab::drawJournal(b.data(),w,{13,15,40},&j,2);
    std::ofstream out(argv[3],std::ios::binary);out<<"P6\n320 240\n255\n";
    for(int y=0;y<240;++y)for(int x=0;x<320;++x){auto p=b.data()+3*(x*240+239-y);char rgb[]={char(p[2]),char(p[1]),char(p[0])};out.write(rgb,3);}
}
