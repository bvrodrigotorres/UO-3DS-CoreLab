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
static int canvasWidth=320;
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
 {'>',"10080402040810"},{'=',"00001F001F0000"},{'!',"04040404040004"},{'%',"19190204081313"}
};
int hex(char c){return c<='9'?c-'0':c-'A'+10;}
void pixel(uint8_t*fb,int x,int y,Color c){
    if(x<0||x>=canvasWidth||y<0||y>=240)return;
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
    text(fb,225,20,"C.LAB V0.18",ink,1,13);
    text(fb,19,34,info.scene+" | "+info.roofs,ink);
    box(fb,17,45,286,1,ink);box(fb,17,47,286,1,gold);
    int y=53,spacing=info.lines.size()>14?8:9;
    for(const auto&line:info.lines){if(y>174)break;text(fb,20,y,line,ink);y+=spacing;}
    box(fb,17,181,286,1,ink);box(fb,17,183,286,1,gold);
    text(fb,20,189,"PAD: WALK | B: RUN | R: PAGE",ink);
    text(fb,20,199,"Y: ROOFS | A: RESET | X: SCENE",ink);
    text(fb,20,209,"SELECT: SAVE | L: RESTORE",ink);
    text(fb,20,219,"START: PAUSA | DESLIZA 1/3",ink);
}
void drawJournal(uint8_t*fb,const World&w,const Actor&a,const Journal*j,int zoom,const std::string&status,const Inventory*inv,const EquipmentArt*equipment){
    drawHud(fb,{"MAPA Y PERSONAJE","PAGINA 2/3",{}});
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
    if(inv&&equipment&&equipment->dolls.size()==8*76*104*4){
        size_t base=inv->equipmentMask()*76*104*4;
        for(int y=0;y<104;++y)for(int x=0;x<76;++x){size_t p=base+(y*76+x)*4;
            if(equipment->dolls[p+3]>127)pixel(fb,211+x,65+y,{equipment->dolls[p],equipment->dolls[p+1],equipment->dolls[p+2]});}
    }else if(j){
        int ox=249-j->dollWidth/2;
        for(int y=0;y<j->dollHeight;++y)for(int x=0;x<j->dollWidth;++x){size_t p=(y*j->dollWidth+x)*4;
            if(j->doll[p+3]>127)pixel(fb,ox+x,65+y,{j->doll[p],j->doll[p+1],j->doll[p+2]});}
    }else text(fb,205,114,"SIN IMAGEN",ink);
    const char*labels[]={"M","R","B"};
    for(int slot=0;slot<3;++slot){int y=72+34*slot;bool worn=inv&&inv->equipment[slot]>=0;
        box(fb,284,y,18,18,ink);box(fb,285,y+1,16,16,worn?gold:Color{161,143,108});
        text(fb,290,y+5,labels[slot],ink);
    }
    text(fb,198,173,inv&&inv->equipmentMask()?"TOCA PARA QUITAR":"SIN EQUIPO",ink,1,17);
    // Replace the shared footer without regenerating the paper background.
    box(fb,18,187,284,41,{218,195,148});
    char coordinates[64];std::snprintf(coordinates,sizeof(coordinates),"XY %d,%d Z %d",a.x+w.originX,a.y+w.originY,a.z);
    text(fb,20,189,coordinates,ink);
    text(fb,20,199,status.empty()?(j?"RADAR UO | VISTA DEL PERSONAJE":"MAPA BASICO | FALTA JOURNAL.BIN"):status,ink);
    text(fb,20,209,"SELECT: SAVE | L: RESTORE",ink);
    text(fb,20,219,"R: PAGINA | DESLIZA 2/3",ink);
}

void drawInventory(uint8_t*fb,const World&w,const Actor&a,const Inventory&inv,const ItemIcons*icons){
    drawHud(fb,{"MOCHILA Y COFRES","PAGINA 3/3",{}});
    const Color ink={55,37,22},gold={174,131,64},slotColor={151,118,76};
    text(fb,20,53,"MOCHILA "+std::to_string(inv.backpack.size())+"/9",ink);
    text(fb,166,53,inv.opened<0?"COFRE CERRADO":"COFRE "+std::to_string(inv.opened+1),ink);
    box(fb,151,52,1,127,gold);
    auto bag=[&](const Bag&items,int left){
        for(int n=0;n<9;++n){int x=left+(n%3)*38,y=65+(n/3)*32;
            box(fb,x,y,35,30,ink);box(fb,x+1,y+1,33,28,slotColor);
            if(size_t(n)>=items.size())continue;
            auto item=items[n];if(item.type>=ItemTypes)continue;
            if(icons&&icons->rgba.size()==ItemTypes*24*24*4){
                for(int iy=0;iy<24;++iy)for(int ix=0;ix<24;++ix){size_t p=(item.type*24*24+iy*24+ix)*4;
                    if(icons->rgba[p+3]>127)pixel(fb,x+5+ix,y+1+iy,{icons->rgba[p],icons->rgba[p+1],icons->rgba[p+2]});}
            }else text(fb,x+2,y+5,itemDefs[item.type].name,{244,222,160},1,5);
            std::string count=std::to_string(item.count);box(fb,x+34-int(count.size())*6,y+21,int(count.size())*6,8,ink);
            text(fb,x+34-int(count.size())*6,y+22,count,{244,222,160},1,5);
        }
    };
    bag(inv.backpack,20);
    static const Bag empty;
    bag(inv.opened>=0&&size_t(inv.opened)<inv.chests.size()?inv.chests[inv.opened].items:empty,166);
    box(fb,20,163,130,17,ink);box(fb,21,164,128,15,gold);
    text(fb,26,168,inv.equipMode?"MODO: EQUIPAR":"MODO: TRANSFERIR",ink);
    box(fb,166,163,134,17,ink);box(fb,167,164,132,15,gold);
    text(fb,174,168,inv.opened>=0?"CERRAR COFRE":"ABRIR CERCANO",ink);
    box(fb,18,187,284,41,{218,195,148});
    text(fb,20,189,inv.message,ink);
    text(fb,20,199,inv.equipMode?"TOCA UN EQUIPABLE DE LA MOCHILA":"TOCA UNA PILA PARA TRANSFERIR",ink);
    text(fb,20,209,inv.nearest(w,a)>=0?"COFRE AL ALCANCE | AUTOGUARDADO":"ACERCATE A UN COFRE DEL MAPA",ink);
    text(fb,20,219,"R: PAGINA | DESLIZA 3/3",ink);
}

void drawTitle(uint8_t*fb,bool loading){
    canvasWidth=400;
    const Color gold={211,170,86},darkGold={110,75,33},stone={27,38,46};
    for(int y=0;y<240;++y)for(int x=0;x<400;++x){int glow=std::max(0,80-std::abs(x-200)/2-std::abs(y-90)/2);
        pixel(fb,x,y,{uint8_t(7+glow/6),uint8_t(14+glow/5),uint8_t(23+glow/4)});}
    for(int x=15;x<390;x+=37){int y=18+(x*13)%43;pixel(fb,x,y,gold);}
    // Original code-drawn battlements, without external title assets.
    for(int side:{0,1}){int x=side?332:20;box(fb,x,157,48,76,stone);for(int n=0;n<4;++n)box(fb,x+n*14,148,7,14,stone);
        for(int row=0;row<3;++row)box(fb,x+19,170+row*20,6,10,{8,15,23});}
    box(fb,72,205,256,28,stone);for(int x=75;x<330;x+=18)box(fb,x,198,9,10,stone);
    for(int y:{8,231}){box(fb,8,y,384,2,darkGold);box(fb,10,y+2,380,1,gold);}
    for(int x:{8,390})box(fb,x,8,2,226,darkGold);
    text(fb,46,73,"ULTIMA ONLINE",{0,0,0},4);
    text(fb,44,71,"ULTIMA ONLINE",gold,4);
    box(fb,108,111,184,1,darkGold);box(fb,140,114,120,1,gold);
    text(fb,173,125,"3DS",gold,3);
    std::string subtitle=loading?"PREPARANDO BRITANNIA":"AVENTURAS EN BRITANNIA";
    text(fb,(400-int(subtitle.size())*6)/2,173,subtitle,gold,1,36);
    const std::string version="CORELAB V0.18";
    text(fb,(400-int(version.size())*6)/2,217,version,gold);
    canvasWidth=320;
}
void drawLoading(uint8_t*fb,const std::string&stage,int progress){
    drawHud(fb,{"ULTIMA ONLINE 3DS","CARGANDO",{}});
    const Color ink={55,37,22},gold={174,131,64};
    text(fb,32,83,"PREPARANDO TU AVENTURA",ink,1,42);
    text(fb,32,108,stage,ink,1,42);
    box(fb,30,137,260,16,ink);box(fb,32,139,256,12,{143,112,69});
    box(fb,33,140,254*std::max(0,std::min(5,progress))/5,10,gold);
    box(fb,18,187,284,41,{218,195,148});text(fb,47,205,"ESPERA UN MOMENTO...",ink);
}
void drawPause(uint8_t*fb,const PauseMenu&m){
    drawHud(fb,{"ULTIMA ONLINE 3DS","PAUSA",{}});
    const Color ink={55,37,22},gold={174,131,64};
    const char*rows[]={"CONTINUAR","VOLVER AL MENU PRINCIPAL","SALIR DEL JUEGO"};
    for(int i=0;i<3;++i){int y=65+i*38;box(fb,25,y,270,28,ink);box(fb,27,y+2,266,24,i==m.selected?gold:Color{196,168,117});text(fb,36,y+10,i==m.selected?">":" ",ink);text(fb,50,y+10,rows[i],ink);}
    box(fb,18,187,284,41,{218,195,148});text(fb,23,189,m.message.empty()?"EL MUNDO ESTA EN PAUSA":m.message,ink,1,45);
    text(fb,23,207,"PAD: ELEGIR | A: ACEPTAR | TACTIL",ink);text(fb,23,219,"START / B: CONTINUAR",ink);
}
void drawStartMenu(uint8_t*fb,const StartMenu&m,bool canContinue){
    bool opts=m.page==MenuPage::Options,confirm=m.page==MenuPage::Confirm;
    drawHud(fb,{"ULTIMA ONLINE 3DS",opts?"OPCIONES":confirm?"NUEVA PARTIDA":"INICIO",{}});
    const Color ink={55,37,22},gold={174,131,64};
    std::vector<std::string>rows;
    if(opts){const char*roofs[]={"AUTO","OCULTOS","COMPLETOS"};const char*pages[]={"DIAGNOSTICO","MAPA","INVENTARIO"};
        rows={std::string("TECHOS: ")+roofs[m.options.roofs],"ZOOM DEL MAPA: "+std::to_string(m.options.zoom)+"X",std::string("PAGINA: ")+pages[m.options.page],"VOLVER"};
    }else if(confirm)rows={"CANCELAR","SI, NUEVO JUEGO"};
    else rows={"NUEVO JUEGO",canContinue?"CONTINUAR":"CONTINUAR - SIN PARTIDA","OPCIONES"};
    for(size_t i=0;i<rows.size();++i){int y=opts?58+int(i)*30:65+int(i)*38;bool selected=int(i)==m.selected;
        box(fb,25,y,270,28,ink);box(fb,27,y+2,266,24,selected?gold:Color{196,168,117});
        text(fb,36,y+10,selected?">":" ",ink);text(fb,50,y+10,rows[i],ink,1,40);}
    if(confirm)text(fb,30,149,"MOCHILA, EQUIPO Y COFRES DESDE CERO",ink,1,43);
    box(fb,18,187,284,41,{218,195,148});
    text(fb,23,189,m.message.empty()?(confirm?"CONFIRMA PARA EMPEZAR DESDE CERO":opts?"LAS OPCIONES SE GUARDAN":"ELIGE TU AVENTURA"):m.message,ink,1,45);
    text(fb,23,207,"PAD: ELEGIR | A: ACEPTAR | TACTIL",ink);
    text(fb,23,219,"B: VOLVER | START: SALIR",ink);
}

}
