// SPDX-License-Identifier: MIT
#include "character.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
namespace {
int fail(const char*,const char*){return -1;}
unsigned count(const Inventory&i,int t){unsigned n=0;for(auto x:i.backpack)if(x.type==t)n+=x.count;for(int x:i.equipment)if(x==t)++n;return n;}
void put(std::vector<uint8_t>&b,size_t p,uint32_t n){for(int i=0;i<4;++i)b[p+i]=n>>(8*i);}
void word(std::vector<uint8_t>&b,unsigned v){b.push_back(v);b.push_back(v>>8);}
uint32_t hash(const std::vector<uint8_t>&b,size_t start){uint32_t h=2166136261;for(size_t n=start;n<b.size();++n)h=(h^b[n])*16777619;return h;}
void write(const std::string&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write((const char*)b.data(),b.size());}
void layer(const std::string&root,int id,uint16_t first,uint16_t second){
    std::vector<uint8_t>b(604);std::copy_n("UAP3",4,b.begin());b[8]=144;
    for(unsigned sample=0;sample<145;++sample){
        put(b,20+sample*4,b.size()-604);unsigned area=sample==144?260*237:96*112;
        word(b,0);word(b,2);word(b,first);word(b,second);word(b,area-2);word(b,0);
    }
    put(b,600,b.size()-604);put(b,12,b.size()-20);put(b,16,hash(b,20));write(root+"0-"+std::to_string(id)+".bin",b);
}
}
int main(int argc,char**argv){
    assert(argc==2);std::string root=std::string(argv[1])+"/",path=root+"wardrobe26.sav";
    World w=demoWorld();Inventory i;i.init(w,path);i.equipment[5]=8;i.backpack={{9,1},{13,1},{7,1},{12,1},{5,1}};assert(i.checkpoint());
    Appearance a;a.realClothes=a.enabled=true;AppearanceEditor editor(a,false);editor.useWardrobe(i);editor.page=2;
    assert(editor.rowCount()==8&&editor.step()==16);
    for(int r=0;r<8;++r){editor.row=r;assert(editor.hit(200,58+r*16)==r);for(int n=0;n<10;++n){editor.change(1);for(int t:editor.outfit)if(t>=0)assert(count(i,t)>0);}}
    assert(i.equipment[5]==8&&i.backpack.size()==5); // preview/cancel never touches live inventory
    auto desired=i.equipment;desired[5]=9;desired[3]=13;desired[2]=7;
    auto old=i.equipment;auto bag=i.backpack;assert(!i.setOutfit(desired,fail)&&i.equipment==old&&i.backpack.size()==bag.size());
    assert(i.setOutfit(desired));for(int t:{5,7,8,9,12,13})assert(count(i,t)==1);
    Inventory restored;restored.init(w,path);assert(restored.healthy()&&restored.equipment==desired);
    auto impossible=desired;impossible[4]=14;assert(!i.setOutfit(impossible)&&i.equipment==desired&&count(i,14)==0);
    impossible[4]=13;assert(!i.setOutfit(impossible)&&i.equipment==desired);
    i.backpack.assign(BagSlots,{1,1});desired=i.equipment;desired[5]=-1;assert(!i.setOutfit(desired)&&i.equipment[5]==9&&i.backpack.size()==BagSlots);
    i.backpack.back()={8,1};desired=i.equipment;desired[5]=8;assert(i.setOutfit(desired)&&i.backpack.size()==BagSlots&&count(i,8)==1&&count(i,9)==1);
    std::vector<uint8_t>hues(16+3000*64);std::copy_n("UHP1",4,hues.begin());put(hues,4,3000);put(hues,8,3000*64);
    for(size_t p=16;p<hues.size();p+=2){hues[p]=0;hues[p+1]=0x7c;}put(hues,12,hash(hues,16));write(root+"hues.bin",hues);
    layer(root,400,1,1);layer(root,404,63,uint16_t((10<<11)|(10<<6)|(10<<1)|1));layer(root,900,65535,65535);
    CharacterAssets assets;assert(assets.load(root));a.hair=0;a.beard=0;a.shirt=a.pants=a.shoes=0;
    std::vector<uint16_t>bare,afro,world;
    assert(assets.compose(a,8,bare,true));a.hair=7;assert(assets.compose(a,8,afro,true));assert(bare==afro);
    assert(afro[0]==0xf801&&afro[1]==0xf801); // the entire hat is dyed: its dark crown must not stay black
    assert(assets.compose(a,8,world,false,0)&&world[0]==0xf801); // world tint remains unchanged
    std::cout<<"Wardrobe 26: owned-only preview, all slots, atomic save/rollback, full bag swap, no creation, hat occlusion and detail tint passed\n";
}
