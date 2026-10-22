// SPDX-License-Identifier: MIT
#include "npcs.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace lab;
int main(int argc,char**argv){
    if(argc!=4&&argc!=5)return 2;
    World w;std::string error;if(!loadWorld(argv[1],w,error)){std::cerr<<error;return 2;}
    std::ifstream f(argv[2],std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(f)),{});if(b.empty()||b.size()%52)return 2;
    auto n=[&](size_t p){return unsigned(b[p])|unsigned(b[p+1])<<8;};
    auto put=[&](size_t p,int v){b[p]=v&255;b[p+1]=(v>>8)&255;};
    std::vector<std::array<int,4>>guards;
    if(argc==5){std::ifstream g(argv[4]);std::string text((std::istreambuf_iterator<char>(g)),{});for(char&c:text)if(c=='['||c==']'||c==',')c=' ';std::istringstream in(text);std::array<int,4>r;while(in>>r[0]>>r[1]>>r[2]>>r[3])guards.push_back(r);}
    std::vector<Actor>placed;std::vector<size_t>accepted;uint32_t seed=1947;
    auto taken=[&](const Actor&a){for(const auto&p:placed)if(p.x==a.x&&p.y==a.y&&std::abs(p.z-a.z)<16)return true;return false;};
    for(size_t at=0;at<b.size();at+=52){
        Actor home{int(n(at)),int(n(at+2)),int(int16_t(n(at+4)))},chosen;unsigned range=n(at+8);bool animal=n(at+10)!=0,found=false;int permitted=255;
        auto tryCell=[&](int x,int y){
            if(!w.contains(x,y))return false;
            if(n(at+10)&2)for(const auto&r:guards)if(x+w.originX>=r[0]&&x+w.originX<r[0]+r[2]&&y+w.originY>=r[1]&&y+w.originY<r[1]+r[3])return false;
            auto h=heights(w,x,y);std::vector<int>zs{h.average};
            {zs.push_back(home.z);for(auto i:w.cells[y*w.width+x]){const auto&o=w.objects[i];if((o.flags&Surface)&&!(o.flags&Roof))zs.push_back(o.z+((o.flags&Bridge)?o.height/2:o.height));}
                std::stable_sort(zs.begin(),zs.end(),[&](int a,int b){return std::abs(a-home.z)<std::abs(b-home.z);});}
            for(int z:zs){if(!animal&&std::abs(z-home.z)>permitted)continue;Actor a{x,y,z};if(canStand(w,a)&&!taken(a)){chosen=a;return true;}}
            return false;
        };
        int minX=n(at+44),minY=n(at+46),maxX=n(at+48),maxY=n(at+50);
        if(minX>maxX||minY>maxY||maxX>=w.width||maxY>=w.height)return 2;
        for(int tolerance:{0,2,8,16,32,64,128,255}){
        permitted=animal?255:tolerance;
        if(minX!=maxX||minY!=maxY){for(int attempt=0;attempt<4096&&!found;++attempt){seed=seed*1664525+1013904223;int x=minX+int(seed%unsigned(maxX-minX+1));seed=seed*1664525+1013904223;int y=minY+int(seed%unsigned(maxY-minY+1));found=tryCell(x,y);}}
        if(!found)for(int r=0;r<=int(std::max(3u,range))&&!found;++r)for(int y=-r;y<=r&&!found;++y)for(int x=-r;x<=r&&!found;++x){if(std::max(std::abs(x),std::abs(y))!=r)continue;if(minX!=maxX||minY!=maxY){if(home.x+x<minX||home.x+x>maxX||home.y+y<minY||home.y+y>maxY)continue;}found=tryCell(home.x+x,home.y+y);}
        if(found)break;
        }
        if(!found){std::cerr<<"No valid placement for "<<(const char*)&b[at+12]<<" at "<<home.x<<","<<home.y<<","<<home.z<<"\n";if(argc==5)continue;return 1;}
        placed.push_back(chosen);accepted.push_back(at);put(at,chosen.x);put(at+2,chosen.y);put(at+4,chosen.z);
    }
    std::ofstream out(argv[3],std::ios::binary);for(size_t at:accepted)out.write((const char*)b.data()+at,44);if(!out)return 2;
    if(argc==5){std::ofstream indices(std::string(argv[3])+".indices");for(auto at:accepted)indices<<at/52<<"\n";}
    std::cout<<"Placed "<<placed.size()<<" original-reference mobiles on valid, distinct supports\n";
}
