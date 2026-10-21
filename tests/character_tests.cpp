// SPDX-License-Identifier: MIT
#include "character.hpp"
#include "hud.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <algorithm>
using namespace lab;
namespace {
int fail(const char*,const char*){return -1;}
std::vector<char>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void ppm(const std::string&p,const uint8_t*b,int width){std::ofstream f(p,std::ios::binary);f<<"P6\n"<<width<<" 240\n255\n";for(int y=0;y<240;++y)for(int x=0;x<width;++x){auto q=b+3*(x*240+239-y);char rgb[]={char(q[2]),char(q[1]),char(q[0])};f.write(rgb,3);}}
uint32_t fnv(const std::vector<uint8_t>&v,size_t begin){uint32_t h=2166136261;for(size_t i=begin;i<v.size();++i)h=(h^v[i])*16777619;return h;}
void put(std::vector<uint8_t>&v,size_t p,uint32_t n){for(int i=0;i<4;++i)v[p+i]=n>>(8*i);}
void write(const std::string&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write((const char*)b.data(),b.size());}
void syntheticAssets(const std::string&root){
    std::vector<uint8_t>hues(16+3000*64);std::copy_n("UHP1",4,hues.begin());put(hues,4,3000);put(hues,8,3000*64);put(hues,12,fnv(hues,16));write(root+"hues.bin",hues);
    std::vector<uint8_t>layer(604+145*10);std::copy_n("UAP1",4,layer.begin());layer[6]=144;layer[7]=1;layer[8]=144;
    put(layer,12,layer.size()-20);
    for(unsigned i=0;i<=145;++i)put(layer,20+i*4,i*10);
    for(unsigned i=0;i<145;++i){size_t pos=604+i*10;unsigned last=(i==144?106*174:96*112)-1;layer[pos+2]=1;layer[pos+4]=255;layer[pos+5]=255;layer[pos+6]=last;layer[pos+7]=last>>8;}
    put(layer,16,fnv(layer,20));write(root+"0-400.bin",layer);
    CharacterAssets bank;assert(bank.load(root));Appearance a;a.hair=a.beard=a.shirt=a.pants=a.shoes=0;std::vector<uint16_t>pixels;
    assert(bank.compose(a,0,pixels)&&pixels.size()==72*96*112);assert(bank.compose(a,0,pixels,true)&&pixels.size()==106*198);
    // A valid checksum must not permit a sparse run past the end of a frame.
    layer[604+2]=255;layer[604+3]=255;put(layer,16,fnv(layer,20));write(root+"0-400.bin",layer);assert(bank.load(root));assert(!bank.compose(a,0,pixels));
    // Bad palette checksum and truncated data fail without retaining stale data.
    hues.back()=1;write(root+"hues.bin",hues);assert(!bank.load(root)&&!bank.ready());
}

}
int main(int argc,char**argv){
    assert(argc>=2);std::string root=std::string(argv[1])+"/",path=root+"appearance.sav",error;
    Appearance old;assert(old.load(path)&&!old.enabled&&old.name=="Avatar");
    Appearance a;a.robe=1;a.realClothes=true;a.hatColor=10;a.neckColor=11;a.glovesColor=12;a.enabled=true;a.name="Lady Rowan";a.female=1;a.skin=63;a.hair=9;a.hairColor=47;a.beard=7;a.beardColor=46;a.shirt=2;a.shirtColor=31;a.pants=2;a.pantsColor=30;a.shoes=0;a.bootsColor=29;a.robeColor=28;
    assert(a.valid()&&a.save(path,error));auto original=read(path);
    Appearance b;assert(b.load(path)&&b.enabled&&b.robe==1&&b.realClothes&&b.hatColor==10&&b.neckColor==11&&b.glovesColor==12&&b.name==a.name&&b.female==1&&b.skin==63&&b.hair==9&&b.hairColor==47&&b.beard==7&&b.beardColor==46&&b.shirt==2&&b.shirtColor==31&&b.pants==2&&b.pantsColor==30&&b.shoes==0&&b.bootsColor==29&&b.robeColor==28);
    b.name="Other";assert(!b.save(path,error,fail)&&read(path)==original);
    assert(b.load(path)&&b.name==a.name);
    for(const auto&name:{""," leading","trailing ","0123","abcdefghijklmnopq","Bad\nName","Jos\xC3\xA9"})assert(!Appearance::validName(name));
    assert(Appearance::validName("Anne-Marie")&&Appearance::validName("O'Connor"));
    b.skin=64;assert(!b.save(path,error)&&read(path)==original);
    {std::ofstream f(path,std::ios::binary);f<<"corrupt";}Appearance protectedSave;assert(!protectedSave.load(path)&&protectedSave.name=="Avatar");
    AppearanceEditor editor(a);editor.page=0;editor.row=1;editor.change(1);assert(editor.draft.female==0);editor.change(-1);assert(editor.draft.female==1&&editor.draft.beard==7);
    editor.page=1;editor.row=2;editor.change(1);assert(editor.draft.beard==7);editor.page=0;editor.row=2;editor.change(1);assert(editor.draft.skin==0);
    editor.page=2;editor.row=3;editor.change(-1);assert(editor.draft.robe==0);
    for(unsigned seed=0;seed<1000;++seed){editor.randomize(seed);assert(editor.draft.valid());}
    assert(editor.hit(20,28)==10&&editor.hit(120,28)==11&&editor.hit(220,28)==12&&editor.hit(300,28)==13&&editor.hit(10,28)==-1);
    for(int row=0;row<editor.rowCount();++row)assert(editor.hit(160,58+editor.step()*row)==row);
    assert(editor.hit(20,210)==20&&editor.hit(200,210)==21&&editor.hit(200,204)==-1);
    std::set<size_t>offsets;for(unsigned y=0;y<1024;++y)for(unsigned x=0;x<1024;++x){auto offset=characterTexel(x,y,1024);assert(offset<1024*1024);assert(offsets.insert(offset).second);}
    syntheticAssets(root);
    CharacterAssets assets;assert(!assets.load(root)&&!assets.ready());std::vector<uint16_t>pixels;assert(!assets.compose(a,0,pixels));
    std::vector<uint8_t>top(400*240*3+32,0xAB),bottom(320*240*3+32,0xCD);
    std::vector<uint16_t>doll(106*198,0xFFFF),sprite(96*112,0xFFFF);
    for(int page=0;page<4;++page){editor.page=page;for(int row=0;row<editor.rowCount();++row){editor.row=row;drawAppearance(top.data()+16,bottom.data()+16,editor,doll,sprite,true);}}
    for(int i=0;i<16;++i)assert(top[i]==0xAB&&top[top.size()-1-i]==0xAB&&bottom[i]==0xCD&&bottom[bottom.size()-1-i]==0xCD);
    if(argc==3){assert(assets.load(argv[2]));
        for(unsigned sex=0;sex<2;++sex){a.female=sex;a.shoes=1;
            for(unsigned hair=0;hair<10;++hair){a.hair=hair;assert(assets.compose(a,0,pixels,true)&&pixels.size()==106*198);assert(assets.compose(a,0,pixels,false,0));}
            for(unsigned beard=0;beard<8;++beard){a.beard=beard;assert(assets.compose(a,7,pixels,true));}
            for(unsigned mask=0;mask<8;++mask){assert(assets.compose(a,mask,pixels)&&pixels.size()==72*96*112);assert(std::count_if(pixels.begin(),pixels.end(),[](uint16_t p){return p&1;})>10000);}
            assert(assets.compose(a,255,pixels)&&pixels.size()==72*96*112);assert(assets.compose(a,2047,pixels)&&pixels.size()==72*96*112);
            std::vector<uint8_t>dolls;assert(assets.dolls(a,dolls)&&dolls.size()==8*76*104*4);
        }
        editor=AppearanceEditor(a);editor.draft.name="Lady Rowan";editor.page=0;editor.row=3;
        Inventory starter;starter.initialDress(editor.draft.shirt,editor.draft.pants,editor.draft.shoes);unsigned starterKey=starter.equipmentMask();
        assert(assets.compose(editor.draft,starterKey,doll,true)&&assets.compose(editor.draft,starterKey,sprite,false,20));
        drawAppearance(top.data()+16,bottom.data()+16,editor,doll,sprite,true);ppm(root+"character-top.ppm",top.data()+16,400);ppm(root+"character-bottom.ppm",bottom.data()+16,320);
        editor.page=1;drawAppearance(top.data()+16,bottom.data()+16,editor,doll,sprite,false);ppm(root+"character-aspect.ppm",bottom.data()+16,320);
    }
    std::cout<<"Character: persistence, rollback, legacy defaults, bounds, editor, GPU tiling and safe two-screen drawing passed\n";
}
