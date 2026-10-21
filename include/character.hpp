// SPDX-License-Identifier: MIT
#pragma once
#include "exploration.hpp"
#include <array>
#include <map>
#include <vector>
#include <cstdint>
#include <string>
namespace lab {
struct Appearance {
    bool enabled=false;
    std::string name="Avatar";
    uint8_t female=0,skin=1,hair=2,hairColor=1,beard=0,beardColor=1;
    uint8_t shirt=1,shirtColor=9,pants=1,pantsColor=3,shoes=1,bootsColor=0,robeColor=9;
    bool valid()const;
    bool load(const std::string&);
    bool save(const std::string&,std::string&,SaveRename=std::rename)const;
    static bool validName(const std::string&);
};
struct AppearanceEditor {
    Appearance draft;
    int page=0,row=0,direction=4,preview=0;
    std::string message;
    explicit AppearanceEditor(const Appearance&a):draft(a){draft.enabled=true;}
    void move(int delta){row=(row+delta+5)%5;}
    void change(int delta);
    void randomize(uint32_t seed);
    std::string label(int field)const;
    std::string value(int field)const;
    int hit(int x,int y)const;
};
class CharacterAssets {
    std::string root;
    std::vector<uint16_t>palettes;
    mutable std::map<std::string,std::vector<uint8_t>>cache;
    bool overlay(std::vector<uint16_t>&,const std::string&,int sample,uint16_t hue)const;
public:
    bool load(const std::string&);
    bool ready()const{return !palettes.empty();}
    // Fixed original anchor and dimensions, shared by every outfit and layer.
    bool compose(const Appearance&,unsigned mask,std::vector<uint16_t>&frames,bool doll=false,int onlyFrame=-1)const;
    bool dolls(const Appearance&,std::vector<uint8_t>&)const;
    uint16_t color(const Appearance&,int field)const;
};
// RGB5551 pixels are row-major; GPU storage uses 8x8 Morton tiles.
inline size_t characterTexel(unsigned x,unsigned y,unsigned width){unsigned n=0;for(unsigned b=0;b<3;++b){n|=((x>>b)&1)<<(2*b);n|=((y>>b)&1)<<(2*b+1);}return ((y/8)*(width/8)+x/8)*64+n;}
void drawAppearance(uint8_t*top,uint8_t*bottom,const AppearanceEditor&,const std::vector<uint16_t>&doll,const std::vector<uint16_t>&sprite,bool fresh);
}
