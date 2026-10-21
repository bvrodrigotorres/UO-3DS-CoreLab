// SPDX-License-Identifier: MIT
#include "npcs.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
int main(int argc,char**argv){
    if(argc!=4)return 2;
    World w;std::string error;if(!loadWorld(argv[1],w,error)){std::cerr<<error;return 2;}
    std::ifstream f(argv[2],std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(f)),{});if(b.empty()||b.size()%44)return 2;
    auto n=[&](size_t p){return unsigned(b[p])|unsigned(b[p+1])<<8;};
    auto put=[&](size_t p,int v){b[p]=v&255;b[p+1]=(v>>8)&255;};
    std::vector<Actor>placed;uint32_t seed=1947;
    auto taken=[&](const Actor&a){for(const auto&p:placed)if(p.x==a.x&&p.y==a.y&&std::abs(p.z-a.z)<16)return true;return false;};
    for(size_t at=0;at<b.size();at+=44){
        Actor home{int(n(at)),int(n(at+2)),int(int16_t(n(at+4)))},chosen;unsigned range=n(at+8);bool animal=n(at+10)!=0,found=false;
        auto tryCell=[&](int x,int y){
            if(!w.contains(x,y))return false;
            auto h=heights(w,x,y);std::vector<int>zs{h.average};
            if(!animal){zs.push_back(home.z);for(auto i:w.cells[y*w.width+x]){const auto&o=w.objects[i];if(o.flags&Surface)zs.push_back(o.z+((o.flags&Bridge)?o.height/2:o.height));}
                std::stable_sort(zs.begin(),zs.end(),[&](int a,int b){return std::abs(a-home.z)<std::abs(b-home.z);});}
            for(int z:zs){Actor a{x,y,z};if(canStand(w,a)&&!taken(a)){chosen=a;return true;}}
            return false;
        };
        if(animal&&range){for(int attempt=0;attempt<4096&&!found;++attempt){seed=seed*1664525+1013904223;int x=home.x+int(seed%(2*range+1))-int(range);seed=seed*1664525+1013904223;int y=home.y+int(seed%(2*range+1))-int(range);found=tryCell(x,y);}}
        if(!found)for(int r=0;r<=int(std::max(3u,range))&&!found;++r)for(int y=-r;y<=r&&!found;++y)for(int x=-r;x<=r&&!found;++x){if(std::max(std::abs(x),std::abs(y))!=r)continue;found=tryCell(home.x+x,home.y+y);}
        if(!found){std::cerr<<"No valid placement for "<<(const char*)&b[at+12]<<" at "<<home.x<<","<<home.y<<","<<home.z<<"\n";return 1;}
        placed.push_back(chosen);put(at,chosen.x);put(at+2,chosen.y);put(at+4,chosen.z);
    }
    std::ofstream out(argv[3],std::ios::binary);out.write((const char*)b.data(),b.size());if(!out)return 2;
    std::cout<<"Placed "<<placed.size()<<" original-reference mobiles on valid, distinct supports\n";
}
