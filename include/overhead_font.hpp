// SPDX-License-Identifier: MIT
#pragma once
#include "label_font.hpp"
#include <array>
#include <fstream>
#include <cstring>
#include <string>
namespace lab {
class OverheadFont {
    std::array<std::array<uint16_t,16>,128>masks{};
    std::array<uint8_t,128>advance{};
public:
    OverheadFont(){advance.fill(6);for(unsigned c=0;c<128;++c)for(unsigned y=0;y<7;++y)for(unsigned x=0;x<5;++x)if(labelPixel(c,x,y))masks[c][y+1]|=1<<(14-x);}
    bool load(const std::string&path){
        std::array<unsigned char,4240>b{};std::ifstream f(path,std::ios::binary);
        if(!f.read((char*)b.data(),b.size())||f.peek()!=EOF||std::memcmp(b.data(),"UOF1",4)||b[4]!=1||b[5]||b[6]!=128||b[7])return false;
        auto u=[&](unsigned at){return uint32_t(b[at])|uint32_t(b[at+1])<<8|uint32_t(b[at+2])<<16|uint32_t(b[at+3])<<24;};
        uint32_t hash=2166136261;for(size_t i=16;i<b.size();++i)hash=(hash^b[i])*16777619;
        if(u(8)!=4224||u(12)!=hash)return false;
        for(unsigned c=0;c<128;++c)if(b[16+c]<2||b[16+c]>16)return false;
        for(unsigned c=0;c<128;++c){advance[c]=b[16+c];for(unsigned y=0;y<16;++y){unsigned at=144+c*32+y*2;masks[c][y]=uint16_t(b[at])|uint16_t(b[at+1])<<8;}}
        return true;
    }
    unsigned step(unsigned c)const{return advance[c<128?c:'?'];}
    bool pixel(unsigned c,int x,int y)const{return c<128&&x>=0&&x<16&&y>=0&&y<16&&(masks[c][y]&(1<<(15-x)));}
    bool outline(unsigned c,int x,int y)const{if(pixel(c,x,y))return false;for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(pixel(c,x+dx,y+dy))return true;return false;}
    unsigned width(const std::string&s)const{unsigned width=0;for(size_t i=0;i<s.size()&&i<30;++i){unsigned next=step((unsigned char)s[i]);if(width+next>360)break;width+=next;}return width;}
};
inline size_t overheadPixelOffset(unsigned x,unsigned y){unsigned morton=0;for(unsigned b=0;b<3;++b){morton|=((x>>b)&1)<<(2*b);morton|=((y>>b)&1)<<(2*b+1);}return ((y/8)*64+x/8)*64+morton;}
struct NamePresence {
    float nearby=0;
    void update(bool near,float dt){if(!near)nearby=0;else if(dt>0)nearby=nearby+dt>2?2:nearby+dt;}
    bool visible()const{return nearby<1.9999f;}
};
}
