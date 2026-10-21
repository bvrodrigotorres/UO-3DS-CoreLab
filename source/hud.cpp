// SPDX-License-Identifier: MIT
#include "hud.hpp"
#include "exploration.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
namespace lab {
namespace {
struct Color {uint8_t r,g,b;};
struct Glyph {char letter;const char*rows;};
// Original compact bitmap alphabet. Row values are five-bit hexadecimal masks.
const Glyph alphabet[]={
 {'A',"0E11111F111111"},{'B',"1E11111E11111E"},{'C',"0E11101010110E"},
 {'D',"1E11111111111E"},{'E',"1F10101E10101F"},{'F',"1F10101E101010"},
 {'G',"0E11101711110F"},{'H',"1111111F111111"},{'I',"0E04040404040E"},
 {'J',"0702020212120C"},{'K',"11121418141211"},{'L',"1010101010101F"},
 {'M',"111B1515111111"},{'N',"11191915131311"},{'O',"0E11111111110E"},
 {'P',"1E11111E101010"},{'Q',"0E11111115120D"},{'R',"1E11111E141211"},
 {'S',"0F10100E01011E"},{'T',"1F040404040404"},{'U',"1111111111110E"},
 {'V',"11111111110A04"},{'W',"11111115151B11"},{'X',"11110A040A1111"},
 {'Y',"11110A04040404"},{'Z',"1F01020408101F"},
 {'0',"0E11131519110E"},{'1',"040C040404040E"},{'2',"0E11010204081F"},
 {'3',"1E01010601011E"},{'4',"02060A121F0202"},{'5',"1F10101E01011E"},
 {'6',"0E10101E11110E"},{'7',"1F010204080808"},{'8',"0E11110E11110E"},
 {'9',"0E11110F01010E"},{'.',"00000000000C0C"},{',',"00000000000C08"},
 {':',"000C0C000C0C00"},{'-',"0000001F000000"},{'/',"01010204081010"},
 {'|',"04040404040404"},{'(',"02040808080402"},{')',"08040202020408"},
 {'+',"0004041F040400"},{'~',"00000916000000"},{'#',"0A0A1F0A1F0A0A"},
 {'@',"0E11171D17100F"},{'_',"0000000000001F"},{'?',"0E110102040004"},
 {'=',"00001F001F0000"},{'!',"04040404040004"},{'%',"19190204081313"}
};
int hex(char c){return c<='9'?c-'0':c-'A'+10;}
void pixel(uint8_t*fb,int x,int y,Color c){
    if(x<0||x>=320||y<0||y>=240)return;
    auto p=fb+3*(x*240+239-y);p[0]=c.b;p[1]=c.g;p[2]=c.r;
}
void box(uint8_t*fb,int x,int y,int w,int h,Color c){for(int j=y;j<y+h;++j)for(int i=x;i<x+w;++i)pixel(fb,i,j,c);}
void text(uint8_t*fb,int x,int y,const std::string&s,Color color,int scale=1,int limit=46){
    int n=0;
    for(unsigned char raw:s){
        if(n++>=limit)break;
        char ch=std::toupper(raw);const char*rows=nullptr;
        for(const auto&g:alphabet)if(g.letter==ch){rows=g.rows;break;}
        if(rows)for(int j=0;j<7;++j){int mask=hex(rows[j*2])*16+hex(rows[j*2+1]);for(int i=0;i<5;++i)if(mask&(1<<(4-i)))box(fb,x+i*scale,y+j*scale,scale,scale,color);}
        x+=6*scale;
    }
}
}
void drawHud(uint8_t*fb,const HudInfo&info){
    const Color ink={55,37,22},gold={174,131,64},dark={42,29,21};
    static std::array<uint8_t,320*240*3> paper;
    static bool ready=false;
    auto output=fb;fb=paper.data();
    if(!ready){
    for(int y=0;y<240;++y)for(int x=0;x<320;++x){
        // Deterministic paper grain; no random generator or image decoding.
        unsigned grain=((x*17+y*31+(x*y)%23)%11);
        int edge=std::min(std::min(x,319-x),std::min(y,239-y));
        int shade=edge<16?(16-edge)*2:0;
        pixel(fb,x,y,{uint8_t(213+grain-shade),uint8_t(190+grain-shade),uint8_t(143+grain-shade)});
    }
    box(fb,0,0,320,5,dark);box(fb,0,235,320,5,dark);box(fb,0,0,5,240,dark);box(fb,315,0,5,240,dark);
    for(int x:{9,307}){box(fb,x,8,3,224,gold);box(fb,x+1,9,1,222,ink);}
    for(int y:{8,230})box(fb,9,y,301,2,gold);
    for(int x:{8,306})for(int y:{7,229}){box(fb,x,y,6,6,ink);box(fb,x+1,y+1,3,3,gold);}
    ready=true;
    }
    std::memcpy(output,paper.data(),paper.size());fb=output;
    text(fb,19,15,"BRITANNIA",ink,2);
    text(fb,225,20,"CORELAB V0.9.1",ink,1,13);
    text(fb,19,34,info.scene+" | "+info.roofs,ink);
    box(fb,17,45,286,1,ink);box(fb,17,47,286,1,gold);
    int y=53,spacing=info.lines.size()>14?8:9;
    for(const auto&line:info.lines){if(y>174)break;text(fb,20,y,line,ink);y+=spacing;}
    box(fb,17,181,286,1,ink);box(fb,17,183,286,1,gold);
    text(fb,20,189,"PAD: WALK | B: RUN | R: PAGE",ink);
    text(fb,20,199,"Y: ROOFS | A: RESET | X: SCENE",ink);
    text(fb,20,209,"SELECT: SAVE | L: RESTORE",ink);
    text(fb,20,219,"START: EXIT | DESLIZA 1/2",ink);
}
void drawJournal(uint8_t*fb,const World&w,const Actor&a,const Journal*j,int zoom,const std::string&status){
    drawHud(fb,{"MAPA Y PERSONAJE","PAGINA 2/2",{}});
    const Color ink={55,37,22},gold={174,131,64},dark={42,29,21};
    text(fb,20,53,"MINIMAPA",ink);text(fb,200,53,"PERSONAJE",ink);
    box(fb,192,53,1,127,gold);
    // Inverse isometric projection: same world axes as the top screen.
    // Sampling is bounded to the exported radar area; black outside it.
    float scale=float(std::max(1,std::min(4,zoom)));
    for(int y=65;y<169;++y)for(int x=20;x<186;++x){
        float sx=(x-103.f)/scale,sy=(y-117.f)/scale;
        int wx=int(std::floor(a.x+w.originX+(sx+sy)*.5f));
        int wy=int(std::floor(a.y+w.originY+(sy-sx)*.5f));
        Color c=dark;
        if(j){
            int mx=wx-j->originX,my=wy-j->originY;
            if(mx>=0&&my>=0&&mx<j->width&&my<j->height){size_t p=(my*j->width+mx)*3;c={j->map[p],j->map[p+1],j->map[p+2]};}
        }else if(w.contains(wx-w.originX,wy-w.originY)){
            char tile=mapCell(w,a,wx-w.originX,wy-w.originY);
            c=tile=='~'?Color{45,83,126}:tile=='#'?Color{108,100,89}:tile=='+'?Color{156,119,72}:Color{83,113,63};
        }
        pixel(fb,x,y,c);
    }
    for(int d=-4;d<=4;++d){pixel(fb,103+d,117,gold);pixel(fb,103,117+d,gold);}
    box(fb,102,116,3,3,{220,38,28});text(fb,175,67,"N",{244,222,160});
    text(fb,23,173,"-",ink);text(fb,153,173,"+",ink);
    text(fb,56,173,"ZOOM "+std::to_string(zoom)+"X",ink);
    // The private image is the original human paperdoll gump, cropped/resized
    // by the exporter, not an invented equipment state or animated world sprite.
    if(j){
        int ox=249-j->dollWidth/2;
        for(int y=0;y<j->dollHeight;++y)for(int x=0;x<j->dollWidth;++x){
            size_t p=(y*j->dollWidth+x)*4;
            if(j->doll[p+3]>127)pixel(fb,ox+x,65+y,{j->doll[p],j->doll[p+1],j->doll[p+2]});
        }
    }else{text(fb,205,114,"SIN IMAGEN",ink);}
    for(int x:{198,290})for(int y:{74,105,136}){
        box(fb,x,y,10,10,ink);box(fb,x+1,y+1,8,8,gold);
    }
    text(fb,216,173,"SIN EQUIPO",ink);
    // Replace the shared footer without regenerating the paper background.
    box(fb,18,187,284,41,{218,195,148});
    char coordinates[64];std::snprintf(coordinates,sizeof(coordinates),"XY %d,%d Z %d",a.x+w.originX,a.y+w.originY,a.z);
    text(fb,20,189,coordinates,ink);
    text(fb,20,199,status.empty()?(j?"RADAR UO | VISTA DEL PERSONAJE":"MAPA BASICO | FALTA JOURNAL.BIN"):status,ink);
    text(fb,20,209,"SELECT: SAVE | L: RESTORE",ink);
    text(fb,20,219,"R: PAGINA | DESLIZA 2/2",ink);
}

}
