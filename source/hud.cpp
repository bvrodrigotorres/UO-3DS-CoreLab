// SPDX-License-Identifier: MIT
#include "hud.hpp"
#include "mount.hpp"
#include "radar_markers.hpp"
#include "commerce.hpp"
#include "equipment_menu.hpp"
#include "developer_menu.hpp"
#include "inventory_drag.hpp"
#include "character.hpp"
#include "overhead_font.hpp"
#include "combat.hpp"
#include "exploration.hpp"
#include "version.hpp"
#include "title_art.hpp"
#include <cmath>
#include <memory>
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
 {'$',"040F140E051E04"},{'@',"0E11171D17100F"},{'_',"0000000000001F"},{'?',"0E110102040004"},
 {'<',"01020408040201"},{'\'',"04040000000000"},{'>',"10080402040810"},{'=',"00001F001F0000"},{'!',"04040404040004"},{'%',"19190204081313"}
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
static void drawHudFrame(uint8_t*fb,const HudInfo&info,bool paused,bool compact=false){
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
    if(compact)return;
    if(paused){text(fb,116,18,"PAUSA",ink,3);}else{
    text(fb,19,15,"BRITANNIA",ink,2);
    text(fb,225,20,std::string("C.LAB V")+Version,ink,1,13);
    text(fb,19,34,info.scene+" | "+info.roofs,ink);
    }
    box(fb,17,45,286,1,ink);box(fb,17,47,286,1,gold);
    int y=53,spacing=info.lines.size()>14?8:9;
    for(const auto&line:info.lines){if(y>174)break;text(fb,20,y,line,ink);y+=spacing;}
    box(fb,17,181,286,1,ink);box(fb,17,183,286,1,gold);
    text(fb,20,189,"PAD: WALK | B: RUN | R: PAGE",ink);
    text(fb,20,199,"Y: CURAR | A: ATACAR | X: OBJ",ink);
    text(fb,20,209,"SELECT: TECHOS | L/R: PAGINAS",ink);
    text(fb,20,219,"START: PAUSA | DESLIZA 1/6",ink);
}
void drawHud(uint8_t*fb,const HudInfo&info){drawHudFrame(fb,info,false);}
void drawExpeditionHud(uint8_t*fb,const HudInfo&info){
    drawHud(fb,info);const Color ink={55,37,22};box(fb,18,185,284,45,{218,195,148});
    text(fb,20,189,"PAD: CAMINAR | B: CORRER",ink);text(fb,20,199,"A: ATACAR | Y: CURAR | L/R: HOJAS",ink);
    text(fb,20,209,"X: DESTINOS | START: PAUSA",ink);text(fb,20,219,"EN PAUSA: B VUELVE A TU PARTIDA",ink);
}
void drawSkills(uint8_t*fb,const Skills&skills,const SkillsMenu&menu,bool meditating){
    drawHudFrame(fb,{},false,true);const Color ink={55,37,22},gold={174,131,64},muted={118,108,92};
    text(fb,20,14,"HABILIDADES",ink,2);text(fb,20,34,"TOTAL "+skillValue(skills.total())+" / 700.0",ink);text(fb,216,34,"MAX 100.0",ink);
    auto rows=menu.list();for(int row=0;row<6;++row){unsigned entry=unsigned(menu.sheet*6+row);if(entry>=rows.size())break;unsigned i=rows[entry];auto&s=skills.entries[i];int y=45+row*20;
        if(int(i)==menu.selected)box(fb,18,y,284,19,{224,204,159});
        const Color c=skillDefs[i].active?ink:muted;text(fb,21,y+6,skillDefs[i].name,c,1,21);
        box(fb,153,y+8,49,4,{174,160,134});box(fb,153,y+8,int(s.value)*49/1000,4,gold);
        text(fb,211,y+6,skillValue(s.value),c);text(fb,281,y+4,s.lock==SkillLock::Up?"+":s.lock==SkillLock::Down?"-":"=",c,2);
    }
    box(fb,18,167,284,1,gold);unsigned selected=unsigned(std::max(0,std::min(int(SkillCount)-1,menu.selected)));
    text(fb,20,173,skillDefs[selected].original,ink,1,46);text(fb,20,184,skillDefs[selected].help,ink,1,46);
    std::string info=!menu.message.empty()?menu.message:skills.notice>0?skills.message:meditating?"MEDITANDO: QUIETO, SIN COMBATE":skills.entries[selected].value>=1000?"GRAN MAESTRO | + SUBIR - BAJAR = FIJO":"TOCA MODO: + SUBIR - BAJAR = FIJO";
    text(fb,20,195,info,ink,1,46);
    box(fb,18,207,85,24,gold);box(fb,107,207,105,24,gold);box(fb,216,207,86,24,gold);
    text(fb,24,215,menu.all?"VER ACTIVAS":"VER TODAS",ink);text(fb,114,215,meditating?"MEDITANDO":"MEDITAR",ink);text(fb,222,215,"HOJA "+std::to_string(menu.sheet+1)+"/"+std::to_string(menu.sheets()),ink);
}
void drawJournal(uint8_t*fb,const World&w,const Actor&a,const Journal*j,int zoom,const std::string&message,const Inventory*inv,const EquipmentArt*equipment,const PlayerStatus*status,const StatusArt*statusArt,const Npcs*npcs,bool expedition){
    drawHudFrame(fb,{},false,true);
    const Color ink={55,37,22},gold={174,131,64},dark={42,29,21};

    text(fb,20,14,"MINIMAPA",ink);text(fb,210,14,equipment&&!equipment->playerName.empty()?equipment->playerName:"PERSONAJE",ink,1,16);
    box(fb,192,25,1,116,gold);
    // Inverse isometric projection: same world axes as the top screen.
    // Sampling is bounded to the exported radar area; black outside it.
    float scale=float(std::max(1,std::min(4,zoom)));
    struct RadarCache{const Journal*j=nullptr;const World*w=nullptr;const uint8_t*data=nullptr;Actor actor{};int zoom=0,originX=0,originY=0;bool valid=false;std::array<uint8_t,166*(JournalLayout::MapBottom-JournalLayout::MapTop)*3>pixels{};};
    static RadarCache radar;
    if(!radar.valid||radar.j!=j||radar.w!=&w||radar.data!=(j?j->map.data():nullptr)||radar.actor.x!=a.x||radar.actor.y!=a.y||radar.actor.z!=a.z||radar.zoom!=zoom||radar.originX!=w.originX||radar.originY!=w.originY){
    for(int y=JournalLayout::MapTop;y<JournalLayout::MapBottom;++y)for(int x=20;x<186;++x){
        float sx=(x-103.f)/scale,sy=(y-float(JournalLayout::MapCenter))/scale;
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
        size_t at=((y-JournalLayout::MapTop)*166+x-20)*3;radar.pixels[at]=c.b;radar.pixels[at+1]=c.g;radar.pixels[at+2]=c.r;
    }
        radar.j=j;radar.w=&w;radar.data=j?j->map.data():nullptr;radar.actor=a;radar.zoom=zoom;radar.originX=w.originX;radar.originY=w.originY;radar.valid=true;
    }
    for(int y=JournalLayout::MapTop;y<JournalLayout::MapBottom;++y)for(int x=20;x<186;++x){size_t at=((y-JournalLayout::MapTop)*166+x-20)*3;pixel(fb,x,y,{radar.pixels[at+2],radar.pixels[at+1],radar.pixels[at]});}
    // Dynamic markers overlay the cached terrain; no map rebuild or texture upload.
    if(npcs)for(int priority=0;priority<3;++priority)for(const auto&n:npcs->all()){
        RadarMarker m;if(!radarMarker(n,a,zoom,m)||m.priority!=priority)continue;
        for(int y=-2;y<=2;++y)for(int x=-2;x<=2;++x){int px=m.x+x,py=m.y+y;
            if(px<20||px>=186||py<JournalLayout::MapTop||py>=JournalLayout::MapBottom)continue;
            bool center=std::abs(x)<=1&&std::abs(y)<=1;
            pixel(fb,px,py,center?Color{m.color[0],m.color[1],m.color[2]}:dark);
        }
    }
    for(int d=-4;d<=4;++d){pixel(fb,103+d,JournalLayout::MapCenter,gold);pixel(fb,103,JournalLayout::MapCenter+d,gold);}
    box(fb,102,JournalLayout::MapCenter-1,3,3,{35,225,245});text(fb,175,JournalLayout::MapTop+2,"N",{244,222,160});
    text(fb,23,JournalLayout::ZoomTop,"-",ink);text(fb,153,JournalLayout::ZoomTop,"+",ink);
    text(fb,56,JournalLayout::ZoomTop,message=="PREPARANDO TERRENO..."?"CARGANDO...":"ZOOM "+std::to_string(zoom)+"X",ink);
    // The private image is the original human paperdoll gump, cropped/resized
    // by the exporter, not an invented equipment state or animated world sprite.
    if(inv&&equipment&&((equipment->currentKey==inv->equipmentMask()&&equipment->currentDoll.size()==76*104*4)||equipment->dolls.size()==8*76*104*4)){
        bool current=equipment->currentKey==inv->equipmentMask()&&equipment->currentDoll.size()==76*104*4;
        const auto&rgba=current?equipment->currentDoll:equipment->dolls;size_t base=current?0:(inv->equipmentMask()&7)*76*104*4;
        for(int y=0;y<104;++y)for(int x=0;x<76;++x){size_t p=base+(y*76+x)*4;
            if(rgba[p+3]>127)pixel(fb,211+x,JournalLayout::MapTop+y,{rgba[p],rgba[p+1],rgba[p+2]});}
    }else if(j){
        int ox=249-j->dollWidth/2;
        for(int y=0;y<j->dollHeight;++y)for(int x=0;x<j->dollWidth;++x){size_t p=(y*j->dollWidth+x)*4;
            if(j->doll[p+3]>127)pixel(fb,ox+x,JournalLayout::MapTop+y,{j->doll[p],j->doll[p+1],j->doll[p+2]});}
    }else text(fb,205,86,"SIN IMAGEN",ink);
    text(fb,205,JournalLayout::ZoomTop,expedition?"TOCA: VIAJAR":"TOCA: EQUIPO",ink,1,17);
    const PlayerStatus initial;const auto&stats=status?*status:initial;
    constexpr int ox=10,oy=146;
    if(statusArt&&statusArt->rgba.size()==300*82*4){
        for(int y=0;y<82;++y)for(int x=0;x<300;++x){size_t p=(y*300+x)*4;if(statusArt->rgba[p+3])pixel(fb,ox+x,oy+y,{statusArt->rgba[p],statusArt->rgba[p+1],statusArt->rgba[p+2]});}
    }else{
        box(fb,ox,oy,300,82,{185,174,148});text(fb,ox+86,oy+4,"CHARACTER STATUS",ink);
        const char*icons[]={"F","D","I","V","R","M","C","S","P","A","O","T"};
        for(int col=0;col<4;++col)for(int row=0;row<3;++row){int x=ox+8+col*60,y=oy+17+row*20;box(fb,x,y,60,20,ink);box(fb,x+1,y+1,58,18,{181,177,162});text(fb,x+3,y+7,icons[col*3+row],ink);}
        for(int row=0;row<5;++row)text(fb,ox+250,oy+19+row*12,std::string(1,"PFCVE"[row]),ink);
    }
    auto value=[&](int col,int row,const std::string&v){text(fb,ox+8+col*60+25,oy+17+row*20+7,v,ink,1,6);};
    auto pair=[&](int col,int row,int current,int maximum){int x=ox+8+col*60+29,y=oy+17+row*20;text(fb,x,y+1,std::to_string(current),ink,1,5);box(fb,x-2,y+9,29,1,ink);text(fb,x,y+11,std::to_string(maximum),ink,1,5);};
    value(0,0,std::to_string(stats.strength));value(0,1,std::to_string(stats.dexterity));value(0,2,std::to_string(stats.intelligence));
    pair(1,0,stats.hits,stats.hitsMax);pair(1,1,stats.stamina,stats.staminaMax);pair(1,2,stats.mana,stats.manaMax);
    value(2,0,std::to_string(stats.statCap));value(2,1,std::to_string(stats.luck));pair(2,2,PlayerStatus::weight(inv),stats.weightMax());
    value(3,0,std::to_string(stats.damageMin(inv))+"-"+std::to_string(stats.damageMax(inv)));value(3,1,std::to_string(PlayerStatus::gold(inv)));value(3,2,std::to_string(stats.followers+(stats.mounted?1:0))+"/"+std::to_string(stats.followersMax));
    if(stats.mounted){box(fb,199,204,58,1,gold);box(fb,199,222,58,1,gold);}
    for(int row=0;row<5;++row)text(fb,ox+270,oy+18+row*12,std::to_string(stats.resistance(inv,row)),ink,1,3);

}

void drawInventory(uint8_t*fb,const World&w,const Actor&a,const Inventory&inv,const ItemIcons*icons,const InventoryDrag*drag,const PlayerStatus*status,bool trial){
    (void)w;(void)a;drawHudFrame(fb,{},false,true);
    const Color ink={55,37,22},gold={174,131,64},slotColor={151,118,76};
    text(fb,18,18,std::string(trial?"PRUEBA ":"MOCHILA ")+std::to_string(inv.backpack.size())+"/24",ink);
    text(fb,170,18,inv.hasContainer()?inv.containerName():"SIN BOTIN CERCA",ink,1,22);
    box(fb,156,34,1,154,gold);
    auto bag=[&](const Bag&items,int left,size_t offset){
        for(int n=0;n<12;++n){int x=left+(n%3)*InventoryLayout::StepX,y=InventoryLayout::Top+(n/3)*InventoryLayout::StepY;
            box(fb,x,y,InventoryLayout::Width,InventoryLayout::Height,ink);box(fb,x+1,y+1,InventoryLayout::Width-2,InventoryLayout::Height-2,slotColor);
            if(offset+n>=items.size())continue;
            auto item=items[offset+n];if(item.type>=ItemTypes)continue;
            if(icons&&icons->rgba.size()==ItemTypes*24*24*4){
                for(int iy=0;iy<24;++iy)for(int ix=0;ix<24;++ix){size_t p=(item.type*24*24+iy*24+ix)*4;
                    if(icons->rgba[p+3]>127)pixel(fb,x+8+ix,y+3+iy,{icons->rgba[p],icons->rgba[p+1],icons->rgba[p+2]});}
            }else text(fb,x+3,y+7,itemDefs[item.type].name,{244,222,160},1,6);
            std::string count=std::to_string(item.count);box(fb,x+40-int(count.size())*6,y+26,int(count.size())*6,8,ink);
            text(fb,x+40-int(count.size())*6,y+27,count,{244,222,160},1,5);
        }
    };
    bag(inv.backpack,InventoryLayout::Left,inv.bagPage*BagPageSlots);bag(inv.containerItems(),InventoryLayout::Right,0);
    auto button=[&](int x,int y,int width,int height,const std::string&label,bool active){box(fb,x,y,width,height,ink);box(fb,x+1,y+1,width-2,height-2,active?gold:Color{161,143,108});text(fb,x+7,y+(height-7)/2,label,ink);};
    button(18,194,130,16,"HOJA "+std::to_string(inv.bagPage+1)+"/2  >",true);
    button(18,214,90,16,inv.useMode?"USAR":inv.equipMode?"EQUIPAR":"TRANSFERIR",true);
    if(!trial){box(fb,116,214,32,17,ink);box(fb,118,216,28,13,{161,143,108});
    box(fb,126,219,13,9,ink);box(fb,124,217,17,2,ink);box(fb,130,215,5,2,ink);
    for(int x:{129,133,137})box(fb,x,221,1,5,{244,222,160});}
    button(170,194,130,36,"SACAR TODO",inv.hasContainer()&&!inv.containerItems().empty());
    bool failed=inv.message.find("ERROR")!=std::string::npos||inv.message.find("LLENA")!=std::string::npos||inv.message.find("ESPACIO")!=std::string::npos;
    if(!failed&&inv.useMode){text(fb,18,29,inv.message,ink,1,47);}
    else if(!failed&&inv.detailType>=0&&inv.detailType<int(ItemTypes)){const auto&d=itemDefs[inv.detailType];
        std::string detail=d.name;if(itemHealing(unsigned(inv.detailType)))detail+=" +"+std::to_string(inv.detailType==3&&status?status->skills.bandageAmount():itemHealing(unsigned(inv.detailType)))+" VIDA";else if(inv.detailType==28)detail+=" +2 DANO";else if(d.damageMax)detail+=" "+std::to_string(d.damageMin)+"-"+std::to_string(d.damageMax)+" / "+std::to_string(int(d.delay*1000))+"MS";else if(d.protection)detail+=" PROT "+std::to_string(d.protection)+"%";
        text(fb,18,29,detail,ink,1,47);}
    // Capacity/write failures remain visible without restoring the old footer.
    if(inv.message.find("ERROR")!=std::string::npos||inv.message.find("LLENA")!=std::string::npos||inv.message.find("ESPACIO")!=std::string::npos)
        text(fb,18,29,inv.message,ink,1,46);
    if(inv.saveFailed){box(fb,18,28,282,9,{231,213,177});text(fb,18,29,"ERROR SD: CAMBIOS PENDIENTES",{170,25,20});}
    else if(inv.savePending)box(fb,145,19,4,5,gold);
    if(drag&&drag->active&&drag->dragging){int x=std::max(0,std::min(295,drag->x-12)),y=std::max(0,std::min(215,drag->y-12));
        box(fb,x,y,25,25,gold);if(icons&&icons->rgba.size()==ItemTypes*24*24*4&&drag->item.type<ItemTypes)for(int yy=0;yy<24;++yy)for(int xx=0;xx<24;++xx){size_t p=(drag->item.type*576+yy*24+xx)*4;if(icons->rgba[p+3]>127)pixel(fb,x+xx,y+yy,{icons->rgba[p],icons->rgba[p+1],icons->rgba[p+2]});}
    }

}

void drawEquipmentMenu(uint8_t*top,uint8_t*bottom,const EquipmentMenu&e,const Appearance&a,const Inventory&bag,const PlayerStatus&status,const std::vector<uint16_t>&doll,const std::vector<uint16_t>&sprite){
    AppearanceEditor preview(a,false);preview.direction=e.direction;drawAppearance(top,bottom,preview,doll,sprite,false);
    const Color ink={55,37,22},paper={231,213,177},gold={174,131,64};
    (void)bag;auto staged=std::make_unique<Inventory>();staged->equipment=e.desired;
    canvasWidth=400;box(top,112,13,268,16,paper);text(top,140,17,"EQUIPAMIENTO",ink);
    box(top,218,178,158,41,{176,160,127});text(top,222,181,"DANO "+std::to_string(status.damageMin(staged.get()))+"-"+std::to_string(status.damageMax(staged.get())),ink);
    text(top,222,194,"PROTECCION "+std::to_string(status.resistance(staged.get()))+"%",ink);
    text(top,222,207,"X: GIRAR",ink);
    canvasWidth=320;drawHudFrame(bottom,{},false,true);text(bottom,18,15,"EQUIPAMIENTO",ink);text(bottom,18,31,"L/R: HOJA | PAD: ELEGIR / CAMBIAR",ink,1,48);
    box(bottom,249,25,54,17,gold);text(bottom,254,30,std::to_string(e.page()+1)+"/2 >",ink);
    for(int r=0;r<6;++r){int slot=e.page()*6+r;if(slot>=11)break;int y=46+r*22;box(bottom,18,y,285,20,slot==e.row?gold:paper);text(bottom,22,y+7,equipmentSlotNames[slot],ink,1,11);text(bottom,133,y+7,"<",ink);text(bottom,146,y+7,e.desired[slot]<0?"SIN EQUIPAR":itemDefs[e.desired[slot]].name,ink,1,24);text(bottom,292,y+7,">",ink);}
    int t=e.desired[e.row];std::string detail=t<0?"SIN OBJETO EN ESTA RANURA":t==28?"EFECTO: +2 AL DANO":t==29?"EFECTO: +5% PROTECCION FISICA":itemDefs[t].slot==0?"DANO "+std::to_string(itemDefs[t].damageMin)+"-"+std::to_string(itemDefs[t].damageMax)+" | "+std::to_string(int(itemDefs[t].delay*1000))+" MS":"PROTECCION FISICA: "+std::to_string(itemDefs[t].protection)+"%";
    text(bottom,20,183,detail,ink,1,47);text(bottom,20,194,"A: CAMBIAR | START: APLICAR",ink);text(bottom,20,205,e.message.empty()?"SOLO OBJETOS QUE YA POSEES":e.message,ink,1,47);
    box(bottom,18,216,132,18,gold);text(bottom,33,222,"B: CANCELAR",ink);box(bottom,170,216,133,18,gold);text(bottom,185,222,"APLICAR",ink);
}

void drawCommerce(uint8_t*fb,const Inventory&inv,const Commerce&shop,const ItemIcons*icons){
    drawHudFrame(fb,{},false,true);const Color ink={55,37,22},gold={174,131,64},paper={226,213,177};
    text(fb,18,12,"MOCHILA",ink);text(fb,170,12,shop.title,ink,1,21);
    text(fb,18,24,"ORO: "+std::to_string(PlayerStatus::gold(&inv)),ink);
    text(fb,170,24,"TIENDA",ink);
    for(int side=0;side<2;++side)for(int n=0;n<12;++n){int x=(side?170:18)+n%3*44,y=38+n/3*38;
        box(fb,x,y,41,35,ink);box(fb,x+1,y+1,39,33,paper);int type=-1;
        if(side&&size_t(n)<shop.stock.size())type=int(shop.stock[n]);
        if(!side&&size_t(n+shop.bagPage*12)<inv.backpack.size())type=inv.backpack[n+shop.bagPage*12].type;
        if(type<0||type>=int(ItemTypes))continue;
        if(shop.selected()==type&&shop.purchase()==bool(side)){box(fb,x+1,y+1,39,33,gold);}
        if(icons&&icons->rgba.size()==ItemTypes*24*24*4)for(int iy=0;iy<24;++iy)for(int ix=0;ix<24;++ix){size_t at=(type*576+iy*24+ix)*4;if(icons->rgba[at+3]>127)pixel(fb,x+8+ix,y+1+iy,{icons->rgba[at],icons->rgba[at+1],icons->rgba[at+2]});}
        else text(fb,x+2,y+5,itemDefs[type].name,ink,1,6);
        unsigned count=0;for(auto owned:inv.backpack)if(owned.type==type)count+=owned.count;
        std::string quantity=std::to_string(count);int qx=x+39-int(quantity.size())*6;box(fb,qx,y+1,int(quantity.size())*6,8,ink);text(fb,qx,y+2,quantity,{244,222,160},1,5);
        text(fb,x+2,y+26,shop.accepts(type)?"$"+std::to_string(shop.price(type,side!=0)):"--",ink,1,6);
    }
    box(fb,18,194,130,16,gold);text(fb,22,198,"HOJA "+std::to_string(shop.bagPage+1)+"/2 >",ink);
    box(fb,170,194,130,16,gold);text(fb,176,198,shop.selected()<0?"ELIGE OBJETO":shop.purchase()?"COMPRAR 1":"VENDER 1",ink);
    std::string message=shop.message;
    if(shop.selected()>0)message=(shop.purchase()?"COMPRAR ":"VENDER ")+std::string(itemDefs[shop.selected()].name)+" POR $"+std::to_string(shop.price(unsigned(shop.selected()),shop.purchase()));
    if(shop.message.find("COMPRADO")==0||shop.message.find("VENDIDO")==0||shop.message.find("NO DISPONIBLE")!=std::string::npos||shop.message.find("NO COMPRA")!=std::string::npos||shop.message.find("ERROR")!=std::string::npos||shop.message.find("INSUFICIENTE")!=std::string::npos||shop.message.find("LLENA")!=std::string::npos)message=shop.message;
    text(fb,18,225,message,ink,1,47);
    if(shop.selected()>0){const auto&d=itemDefs[shop.selected()];std::string info=shop.selected()==28?"EFECTO: +2 DANO":d.damageMax?"DANO "+std::to_string(d.damageMin)+"-"+std::to_string(d.damageMax)+" | "+std::to_string(int(d.delay*1000))+" MS":"PROTECCION "+std::to_string(d.protection)+"%";text(fb,18,214,info,ink,1,47);}
}

void drawQuests(uint8_t*fb,const QuestPanel&p){
    drawHudFrame(fb,{},false,true);const Color ink={55,37,22},gold={174,131,64},paper={231,213,177};
    text(fb,20,14,"CONVERSACIONES Y MISIONES",ink);
    for(unsigned i=0;i<2;++i){box(fb,18+i*144,30,140,22,p.selected==i?gold:paper);text(fb,24+i*144,38,i?"SUMINISTROS":"CEMENTERIO",ink);}
    text(fb,20,60,p.speaker,ink,1,46);text(fb,20,72,p.profession,ink,1,46);box(fb,18,85,284,1,gold);
    for(unsigned i=0;i<3;++i)text(fb,20,94+i*13,p.dialogue[i],ink,1,46);
    text(fb,20,139,p.heading,ink,1,46);text(fb,20,153,p.objective,ink,1,46);text(fb,20,167,p.reward,ink,1,46);
    text(fb,20,184,p.message,ink,1,46);
    box(fb,18,201,284,21,gold);text(fb,24,208,p.button,ink,1,44);
    text(fb,20,223,"A: HABLAR | TOCA MISION | L/R: PAGINA",ink,1,46);
}
void drawTitle(uint8_t*fb,bool loading){
    canvasWidth=400;
    const Color gold={238,205,128};
    // Embedded RGB565: available on the very first frame, no SD read or GPU
    // texture allocation, 187.5 KiB in read-only program data.
    for(int y=0;y<240;++y)for(int x=0;x<400;++x){uint16_t c=TitleArt[y*400+x];
        pixel(fb,x,y,{uint8_t(((c>>11)&31)*255/31),uint8_t(((c>>5)&63)*255/63),uint8_t((c&31)*255/31)});}
    std::string subtitle=loading?"PREPARANDO BRITANNIA":"AVENTURAS EN BRITANNIA";
    int x=(400-int(subtitle.size())*6)/2;
    text(fb,x+1,183,subtitle,{0,0,0},1,36);text(fb,x,182,subtitle,gold,1,36);
    const std::string version=std::string("CORELAB V")+Version;
    x=(400-int(version.size())*6)/2;
    text(fb,x+1,223,version,{0,0,0});text(fb,x,222,version,gold);
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
void drawDeveloperMenu(uint8_t*fb,const DeveloperMenu&m,const World&w){
    drawHudFrame(fb,{},false,true);const Color ink={55,37,22},gold={174,131,64},paper={231,213,177};
    text(fb,18,15,"DESARROLLADOR",ink);text(fb,18,32,"TELETRANSPORTE | MUNDO PAUSADO",ink,1,38);box(fb,251,27,51,17,gold);text(fb,256,32,std::to_string(m.page()+1)+"/"+std::to_string((m.destinations.size()+4)/5)+" >",ink);
    for(int row=0;row<5;++row){int index=m.page()*5+row;if(index>=int(m.destinations.size()))break;int y=49+row*28;const auto&d=m.destinations[index];box(fb,18,y,285,25,index==m.selected?gold:paper);text(fb,24,y+4,d.name,ink,1,44);text(fb,24,y+15,d.available?"XY "+std::to_string(d.target.x+w.originX)+","+std::to_string(d.target.y+w.originY)+" Z "+std::to_string(d.target.z):"NO DISPONIBLE EN ESTE MAPA",ink,1,44);}
    text(fb,20,191,"PAD: ELEGIR | L/R: HOJA",ink);text(fb,20,202,m.message.empty()?"A / START: TELETRANSPORTAR":m.message,ink,1,46);
    box(fb,18,215,132,19,gold);text(fb,37,221,"B: VOLVER",ink);box(fb,170,215,133,19,gold);text(fb,178,221,"TELETRANSPORTAR",ink);
}
void drawPause(uint8_t*fb,const PauseMenu&m){
    drawHudFrame(fb,{},true);
    const Color ink={55,37,22},gold={174,131,64};
    const char*rows[]={"CONTINUAR","PERSONALIZAR PERSONAJE","EQUIPAMIENTO",m.developer?"DESARROLLADOR":"OPCIONES","EXPLORAR BRITANNIA (PRUEBA)","VOLVER AL MENU PRINCIPAL","SALIR DEL JUEGO"};
    for(int i=0;i<7;++i){int y=50+i*19;box(fb,25,y,270,17,ink);box(fb,27,y+2,266,13,i==m.selected?gold:Color{196,168,117});text(fb,36,y+5,i==m.selected?">":" ",ink);text(fb,50,y+5,rows[i],ink);}
    box(fb,18,187,284,41,{218,195,148});text(fb,23,189,m.message.empty()?"EL MUNDO ESTA EN PAUSA":m.message,ink,1,45);
    text(fb,23,207,"PAD: ELEGIR | A: ACEPTAR | TACTIL",ink);text(fb,23,219,"START / B: CONTINUAR",ink);
}
void drawStartMenu(uint8_t*fb,const StartMenu&m,bool canContinue){
    bool opts=m.page==MenuPage::Options,confirm=m.page==MenuPage::Confirm;
    drawHud(fb,{"ULTIMA ONLINE 3DS",opts?"OPCIONES":confirm?"NUEVA PARTIDA":"INICIO",{}});
    const Color ink={55,37,22},gold={174,131,64};
    std::vector<std::string>rows;
    if(opts){const char*roofs[]={"AUTO","OCULTOS","COMPLETOS"};const char*pages[]={"DIAGNOSTICO","MAPA","INVENTARIO","COMERCIO","HABILIDADES","MISIONES"};
        rows={std::string("TECHOS: ")+roofs[m.options.roofs],"ZOOM DEL MAPA: "+std::to_string(m.options.zoom)+"X",std::string("PAGINA: ")+pages[m.options.page],"VOLVER"};
    }else if(confirm)rows={"CANCELAR","SI, NUEVO JUEGO"};
    else rows={"NUEVO JUEGO",canContinue?"CONTINUAR":"CONTINUAR - SIN PARTIDA","OPCIONES","SALIR DEL JUEGO"};
    for(size_t i=0;i<rows.size();++i){int y=confirm?65+int(i)*38:58+int(i)*30;bool selected=int(i)==m.selected;
        box(fb,25,y,270,28,ink);box(fb,27,y+2,266,24,selected?gold:Color{196,168,117});
        text(fb,36,y+10,selected?">":" ",ink);text(fb,50,y+10,rows[i],ink,1,40);}
    if(confirm)text(fb,30,149,"MOCHILA, EQUIPO Y COFRES DESDE CERO",ink,1,43);
    box(fb,18,187,284,41,{218,195,148});
    text(fb,23,189,m.message.empty()?(confirm?"CONFIRMA PARA EMPEZAR DESDE CERO":opts?"LAS OPCIONES SE GUARDAN":"ELIGE TU AVENTURA"):m.message,ink,1,45);
    text(fb,23,207,"PAD: ELEGIR | A: ACEPTAR | TACTIL",ink);
    text(fb,23,219,"B: VOLVER | START: SALIR",ink);
}

void drawAppearance(uint8_t*top,uint8_t*bottom,const AppearanceEditor&e,const std::vector<uint16_t>&doll,const std::vector<uint16_t>&sprite,bool fresh){
    const Color ink={55,37,22},gold={174,131,64},light={231,213,177};
    auto image=[&](uint8_t*fb,const std::vector<uint16_t>&v,int w,int h,int left,int above,int scale){
        if(v.size()!=size_t(w*h))return;
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){uint16_t c=v[y*w+x];if(c&1)box(fb,left+x*scale,above+y*scale,scale,scale,{uint8_t((c>>11)*255/31),uint8_t((c>>6&31)*255/31),uint8_t((c>>1&31)*255/31)});}
    };
    canvasWidth=400;
    for(int y=0;y<240;++y)for(int x=0;x<400;++x)pixel(top,x,y,{uint8_t(21+(x+y)%4),uint8_t(31+(x+y)%4),uint8_t(38+(x+y)%4)});
    box(top,8,8,384,228,ink);box(top,10,10,380,224,light);
    text(top,122,17,"PERSONALIZA TU PERSONAJE",ink,1);
    static OverheadFont medieval;static bool tried=false;if(!tried){medieval.load("sdmc:/3ds/UO-3DS-CoreLab/npc_font.bin");tried=true;}
    int nameX=(400-int(medieval.width(e.draft.name))*2)/2;for(unsigned char c:e.draft.name){for(int y=0;y<16;++y)for(int x=0;x<16;++x)if(medieval.pixel(c,x,y))box(top,nameX+x*2,31+y*2,2,2,ink);nameX+=medieval.step(c)*2;}
    box(top,22,66,178,160,{191,169,132});box(top,210,66,168,160,{176,160,127});
    // Preserve every source paperdoll pixel, including hair/hat outside the panel.
    if(doll.size()==260*237){for(int y=0;y<213;++y)for(int x=0;x<234;++x){uint16_t c=doll[(y*237/213)*260+x*260/234];if(c&1)pixel(top,28+x,21+y,{uint8_t((c>>11)*255/31),uint8_t((c>>6&31)*255/31),uint8_t((c>>1&31)*255/31)});}}image(top,sprite,96,112,246,55,1);
    text(top,226,78,"VISTA EN EL MUNDO",ink);
    text(top,222,181,"X: GIRAR",ink);text(top,222,193,"HUMANO - CLASICO",ink);

    canvasWidth=320;
    drawHudFrame(bottom,{},false,true);
    text(bottom,18,13,fresh?"CREAR PERSONAJE":"PERSONALIZACION",ink,1,40);
    const char*tabs[]={"IDENTIDAD","ASPECTO","ROPA","COLORES"};
    for(int i=0;i<4;++i){box(bottom,12+i*74,27,72,22,ink);box(bottom,14+i*74,29,68,18,i==e.page?gold:light);text(bottom,19+i*74,35,tabs[i],ink);}
    for(int r=0;r<e.rowCount();++r){int f=e.fieldAt(r),y=57+r*e.step();bool active=e.enabled(f);Color fieldInk=active?ink:Color{120,111,91};
        box(bottom,12,y,296,e.step()-2,r==e.row?gold:Color{218,195,148});int baseline=y+(e.step()-9)/2;
        text(bottom,18,baseline,e.label(f),fieldInk,1,20);
        text(bottom,144,baseline,"<",fieldInk);
        const auto value=e.value(f);text(bottom,224-int(value.size())*3,baseline,value,fieldInk,1,20);
        text(bottom,295,baseline,">",fieldInk);
    }
    text(bottom,15,187,e.message.empty()?"L/R: PAGINA  X: GIRAR  Y: AZAR":e.message,ink,1,48);
    text(bottom,15,197,"PAD: CAMBIAR  A: EDITAR  START: GUARDAR",ink,1,48);
    for(int i=0;i<2;++i){box(bottom,12+i*151,207,145,24,ink);box(bottom,14+i*151,209,141,20,i?gold:light);text(bottom,35+i*151,216,i?(fresh?"CREAR JUEGO":"GUARDAR"):"B: CANCELAR",ink);}
}

}
