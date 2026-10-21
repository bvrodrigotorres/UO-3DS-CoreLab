// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
#include <algorithm>
namespace lab {
struct Journal {
    int width=0,height=0,originX=0,originY=0,dollWidth=0,dollHeight=0;
    std::vector<uint8_t> map,doll;
};
bool loadJournal(const std::string&path,const std::string&scene,Journal&,std::string&error);
// One horizontal gesture changes one page; vertical drags and short taps do not.
struct TouchPages {
    int page=0,zoom=2,startX=0,startY=0,lastX=0,lastY=0;bool active=false;
    void begin(int x,int y){startX=lastX=x;startY=lastY=y;active=true;}
    void move(int x,int y){if(active){lastX=x;lastY=y;}}
    bool end(){
        if(!active)return false;
        active=false;
        int dx=lastX-startX,dy=lastY-startY;
        if((dx>=40||dx<=-40)&&dx*dx>dy*dy){page=std::max(0,std::min(2,page+(dx<0?1:-1)));return true;}
        if(dx*dx+dy*dy<144){
            if(startY>=215&&startX>=190){page=(page+1)%3;return true;}
            if(page==1&&startY>=169&&startY<=182){
                if(startX>=20&&startX<=47&&zoom>1){--zoom;return true;}
                if(startX>=147&&startX<=178&&zoom<4){++zoom;return true;}
            }
        }
        return false;
    }
};
}
