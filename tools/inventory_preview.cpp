// SPDX-License-Identifier: MIT
#include "hud.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){
    if(argc!=5)return 2;
    lab::World w;std::string error;
    if(!lab::loadWorld(argv[1],w,error)){std::cerr<<error;return 1;}
    lab::Inventory inv;inv.init(w,argv[3]);lab::ItemIcons icons;icons.load(argv[2]);
    lab::Actor a{14,14,40};inv.open(w,a);
    if(inv.opened>=0){inv.transfer(w,a,true,0);inv.transfer(w,a,true,0);}
    std::vector<uint8_t>b(320*240*3);lab::drawInventory(b.data(),w,a,inv,&icons);
    std::ofstream out(argv[4],std::ios::binary);out<<"P6\n320 240\n255\n";
    for(int y=0;y<240;++y)for(int x=0;x<320;++x){auto p=b.data()+3*(x*240+239-y);char rgb[]={char(p[2]),char(p[1]),char(p[0])};out.write(rgb,3);}
}
