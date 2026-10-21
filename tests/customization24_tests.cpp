// SPDX-License-Identifier: MIT
#include "character.hpp"
#include "inventory.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace lab;
namespace {int fail(const char*,const char*){return -1;}unsigned count(const Inventory&i,int t){unsigned n=0;for(auto s:i.backpack)if(s.type==t)n+=s.count;for(auto e:i.equipment)if(e==t)++n;return n;}}
int main(int argc,char**argv){
    assert(argc==2);std::string path=std::string(argv[1])+"/toggle.sav";
    Appearance a;a.enabled=a.realClothes=true;AppearanceEditor editor(a);
    const int fields[4][6]={{0,1,2,-1,-1,-1},{3,4,5,6,-1,-1},{7,9,11,18,-1,-1},{8,10,13,15,16,17}};
    for(int p=0;p<4;++p){editor.page=p;int n=p==0?3:p==3?6:4;assert(editor.rowCount()==n);for(int r=0;r<n;++r){editor.row=r;assert(editor.field()==fields[p][r]);assert(editor.hit(200,58+editor.step()*r)==r);}assert(editor.hit(200,58+editor.step()*n)==-1);}
    editor.page=2;editor.row=3;editor.change(1);assert(editor.draft.robe==1);Inventory starter;starter.initialDress(1,1,true,editor.draft.robe);assert(starter.equipment[1]==6);
    for(int f:{15,16,17}){assert(!editor.enabled(f));}
    AppearanceEditor paused(a,false);paused.equipment=8;assert(paused.enabled(15)&&!paused.enabled(16)&&!paused.enabled(17));paused.equipment=16|64;assert(!paused.enabled(15)&&paused.enabled(16)&&paused.enabled(17));paused.page=2;paused.row=3;paused.change(1);assert(paused.draft.robe==0);
    auto original=editor.draft;for(int seed=0;seed<100;++seed){editor.randomize(seed);assert(editor.draft.hatColor==original.hatColor&&editor.draft.neckColor==original.neckColor&&editor.draft.glovesColor==original.glovesColor);}
    World w=demoWorld();Inventory i;i.init(w,path);i.backpack={{9,1},{13,1}};assert(i.equip(0));assert(i.toggleEquipment(5)&&i.equipment[5]<0&&count(i,9)==1&&i.lastEquipment[5]==9);
    Inventory resumed;resumed.init(w,path);assert(resumed.healthy()&&resumed.lastEquipment[5]==9&&resumed.equipment[5]<0);assert(resumed.toggleEquipment(5)&&resumed.equipment[5]==9&&count(resumed,9)==1);
    auto before=resumed.equipment;auto remembered=resumed.lastEquipment;auto size=resumed.backpack.size();assert(!resumed.toggleEquipment(5,fail)&&resumed.equipment==before&&resumed.lastEquipment==remembered&&resumed.backpack.size()==size);
    assert(resumed.toggleEquipment(5));for(auto it=resumed.backpack.begin();it!=resumed.backpack.end();++it)if(it->type==9){resumed.backpack.erase(it);break;}assert(!resumed.toggleEquipment(5)&&resumed.equipment[5]<0&&count(resumed,9)==0);
    assert(!resumed.toggleEquipment(4));assert(!resumed.toggleEquipment(99));
    Inventory all;all.init(w,std::string(argv[1])+"/all.sav");all.backpack={{5,1},{6,1},{7,1},{13,1},{14,1},{8,1},{15,1},{10,1}};
    for(int n=0;n<8;++n){assert(all.equip(0));}
    for(int s=0;s<8;++s){int type=all.equipment[s];assert(all.toggleEquipment(s));assert(all.toggleEquipment(s));assert(all.equipment[s]==type&&count(all,type)==1);}
    std::cout<<"Customization 0.24: exact tabs, six colors, disabled accessories, real starter robe, last-item toggles across reload, missing items and write rollback passed\n";
}
