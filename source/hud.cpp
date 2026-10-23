// SPDX-License-Identifier: MIT
#include "hud.hpp"
#include "commerce.hpp"
#include "equipment_menu.hpp"
#include "character.hpp"
#include "overhead_font.hpp"
#include "combat.hpp"
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
    text(fb,225,20,"C.LAB V0.31",ink,1,13);
    text(fb,19,34,info.scene+" | "+info.roofs,ink);
    }
    box(fb,17,45,286,1,ink);box(fb,17,47,286,1,gold);
    int y=53,spacing=info.lines.size()>14?8:9;
    for(const auto&line:info.lines){if(y>174)break;text(fb,20,y,line,ink);y+=spacing;}
    box(fb,17,181,286,1,ink);box(fb,17,183,286,1,gold);
    text(fb,20,189,"PAD: WALK | B: RUN | R: PAGE",ink);
    text(fb,20,199,"Y: ROOFS | A: ATACAR | X: OBJ",ink);
    text(fb,20,209,"SELECT: SAVE | L: RESTORE",ink);
    text(fb,20,219,"START: PAUSA | DESLIZA 1/4",ink);
}
void drawHud(uint8_t*fb,const HudInfo&info){drawHudFrame(fb,info,false);}
void drawJournal(uint8_t*fb,const World&w,const Actor&a,const Journal*j,int zoom,const std::string&message,const Inventory*inv,const EquipmentArt*equipment,const PlayerStatus*status,const StatusArt*statusArt){
    drawHudFrame(fb,{},false,true);
    const Color ink={55,37,22},gold={174,131,64},dark={42,29,21};
    (void)message;
    text(fb,20,14,"MINIMAPA",ink);text(fb,164,14,"2/4",ink);text(fb,210,14,equipment&&!equipment->playerName.empty()?equipment->playerName:"PERSONAJE",ink,1,16);
    box(fb,192,25,1,116,gold);
    // Inverse isometric projection: same world axes as the top screen.
    // Sampling is bounded to the exported radar area; black outside it.
    float scale=float(std::max(1,std::min(4,zoom)));
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
        pixel(fb,x,y,c);
    }
    for(int d=-4;d<=4;++d){pixel(fb,103+d,JournalLayout::MapCenter,gold);pixel(fb,103,JournalLayout::MapCenter+d,gold);}
    box(fb,102,JournalLayout::MapCenter-1,3,3,{220,38,28});text(fb,175,JournalLayout::MapTop+2,"N",{244,222,160});
    text(fb,23,JournalLayout::ZoomTop,"-",ink);text(fb,153,JournalLayout::ZoomTop,"+",ink);
    text(fb,56,JournalLayout::ZoomTop,"ZOOM "+std::to_string(zoom)+"X",ink);
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
    const char*labels[]={"AR","TU","BO","GO","CU","PE","GU","PA"};
    const int positions[8][2]={{284,30},{284,54},{284,102},{198,30},{198,54},{284,78},{198,78},{198,102}};
    for(int slot=0;slot<8;++slot){int x=positions[slot][0],y=positions[slot][1];bool worn=inv&&inv->equipment[slot]>=0;
        box(fb,x,y,20,20,ink);box(fb,x+1,y+1,18,18,worn?gold:Color{161,143,108});text(fb,x+4,y+6,labels[slot],ink);}
    text(fb,198,JournalLayout::ZoomTop,inv&&inv->equipmentMask()?"TOCA: QUITA / PON":"SIN EQUIPO",ink,1,17);
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
    value(3,0,std::to_string(stats.damageMin(inv))+"-"+std::to_string(stats.damageMax(inv)));value(3,1,std::to_string(PlayerStatus::gold(inv)));value(3,2,std::to_string(stats.followers)+"/"+std::to_string(stats.followersMax));
    for(int row=0;row<5;++row)text(fb,ox+270,oy+18+row*12,std::to_string(stats.resistance(inv,row)),ink,1,3);

}

void drawInventory(uint8_t*fb,const World&w,const Actor&a,const Inventory&inv,const ItemIcons*icons){
    (void)w;(void)a;drawHudFrame(fb,{},false,true);
    const Color ink={55,37,22},gold={174,131,64},slotColor={151,118,76};
    text(fb,18,18,"MOCHILA "+std::to_string(inv.backpack.size())+"/24",ink);
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
    button(18,214,130,16,inv.equipMode?"MODO: EQUIPAR":"MODO: TRANSFERIR",true);
    button(170,194,130,36,"SACAR TODO",inv.hasContainer()&&!inv.containerItems().empty());
    bool failed=inv.message.find("ERROR")!=std::string::npos||inv.message.find("LLENA")!=std::string::npos||inv.message.find("ESPACIO")!=std::string::npos;
    if(!failed&&inv.detailType>=0&&inv.detailType<int(ItemTypes)){const auto&d=itemDefs[inv.detailType];
        std::string detail=d.name;if(inv.detailType==28)detail+=" +2 DANO";else if(d.damageMax)detail+=" "+std::to_string(d.damageMin)+"-"+std::to_string(d.damageMax)+" / "+std::to_string(int(d.delay*1000))+"MS";else if(d.protection)detail+=" PROT "+std::to_string(d.protection)+"%";
        text(fb,18,29,detail,ink,1,47);}
    // Capacity/write failures remain visible without restoring the old footer.
    if(inv.message.find("ERROR")!=std::string::npos||inv.message.find("LLENA")!=std::string::npos||inv.message.find("ESPACIO")!=std::string::npos)
        text(fb,18,29,inv.message,ink,1,46);
}

void drawEquipmentMenu(uint8_t*top,uint8_t*bottom,const EquipmentMenu&e,const Appearance&a,const Inventory&bag,const PlayerStatus&status,const std::vector<uint16_t>&doll,const std::vector<uint16_t>&sprite){
    AppearanceEditor preview(a,false);preview.direction=e.direction;drawAppearance(top,bottom,preview,doll,sprite,false);
    const Color ink={55,37,22},paper={231,213,177},gold={174,131,64};
    Inventory staged;staged.equipment=e.desired;staged.backpack=bag.backpack;
    canvasWidth=400;box(top,112,13,268,16,paper);text(top,140,17,"EQUIPAMIENTO",ink);
    box(top,218,178,158,41,{176,160,127});text(top,222,181,"DANO "+std::to_string(status.damageMin(&staged))+"-"+std::to_string(status.damageMax(&staged)),ink);
    text(top,222,194,"PROTECCION "+std::to_string(status.resistance(&staged))+"%",ink);
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
    text(fb,170,24,"TIENDA | 4/4",ink);
    for(int side=0;side<2;++side)for(int n=0;n<12;++n){int x=(side?170:18)+n%3*44,y=38+n/3*38;
        box(fb,x,y,41,35,ink);box(fb,x+1,y+1,39,33,paper);int type=-1;
        if(side&&size_t(n)<shop.stock.size())type=int(shop.stock[n]);
        if(!side&&size_t(n+shop.bagPage*12)<inv.backpack.size())type=inv.backpack[n+shop.bagPage*12].type;
        if(type<0||type>=int(ItemTypes))continue;
        if(shop.selected()==type&&shop.purchase()==bool(side)){box(fb,x+1,y+1,39,33,gold);}
        if(icons&&icons->rgba.size()==ItemTypes*24*24*4)for(int iy=0;iy<24;++iy)for(int ix=0;ix<24;++ix){size_t at=(type*576+iy*24+ix)*4;if(icons->rgba[at+3]>127)pixel(fb,x+8+ix,y+1+iy,{icons->rgba[at],icons->rgba[at+1],icons->rgba[at+2]});}
        else text(fb,x+2,y+5,itemDefs[type].name,ink,1,6);
        text(fb,x+2,y+26,shop.accepts(type)?std::to_string(shop.price(type,side!=0))+" O":"--",ink,1,6);
    }
    box(fb,18,194,130,16,gold);text(fb,22,198,"HOJA "+std::to_string(shop.bagPage+1)+"/2 >",ink);
    box(fb,170,194,130,16,gold);text(fb,176,198,shop.selected()<0?"ELIGE OBJETO":shop.purchase()?"COMPRAR 1":"VENDER 1",ink);
    text(fb,18,225,shop.message,ink,1,47);
    if(shop.selected()>0){const auto&d=itemDefs[shop.selected()];std::string info=shop.selected()==28?"EFECTO: +2 DANO":d.damageMax?"DANO "+std::to_string(d.damageMin)+"-"+std::to_string(d.damageMax)+" | "+std::to_string(int(d.delay*1000))+" MS":"PROTECCION "+std::to_string(d.protection)+"%";text(fb,18,214,info,ink,1,47);}
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
    const std::string version="CORELAB V0.31";
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
    drawHudFrame(fb,{},true);
    const Color ink={55,37,22},gold={174,131,64};
    const char*rows[]={"CONTINUAR","PERSONALIZAR PERSONAJE","EQUIPAMIENTO","VOLVER AL MENU PRINCIPAL","SALIR DEL JUEGO"};
    for(int i=0;i<5;++i){int y=52+i*26;box(fb,25,y,270,24,ink);box(fb,27,y+2,266,20,i==m.selected?gold:Color{196,168,117});text(fb,36,y+10,i==m.selected?">":" ",ink);text(fb,50,y+10,rows[i],ink);}
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
