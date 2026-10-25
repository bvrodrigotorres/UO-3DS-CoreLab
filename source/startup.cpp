// SPDX-License-Identifier: MIT
#include "startup.hpp"
#include <array>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sys/stat.h>
namespace lab {
namespace {
using Bytes=std::array<unsigned char,16>;
uint32_t get(const Bytes&b,int p){uint32_t n=0;for(int i=0;i<4;++i)n|=uint32_t(b[p+i])<<(i*8);return n;}
void put(Bytes&b,int p,uint32_t v){for(int i=0;i<4;++i)b[p+i]=uint8_t(v>>(i*8));}
uint32_t hash(const Bytes&b){uint32_t n=2166136261u;for(int i=0;i<12;++i)n=(n^b[i])*16777619u;return n;}
bool exists(const std::string&p){struct stat s;return stat(p.c_str(),&s)==0;}
bool read(const std::string&p,const char*magic,Bytes&b){
    std::string path=p;if(!exists(path)&&exists(path+".bak"))path+=".bak";
    std::ifstream f(path,std::ios::binary);
    return bool(f.read(reinterpret_cast<char*>(b.data()),b.size()))&&f.peek()==EOF&&!std::memcmp(b.data(),magic,4)&&get(b,12)==hash(b);
}
bool write(const std::string&p,Bytes b,std::string&error,SaveRename renameFile){
    put(b,12,hash(b));std::string tmp=p+".tmp",bak=p+".bak";
    FILE*f=std::fopen(tmp.c_str(),"wb");if(!f){error="ERROR SD: NO SE PUDO GUARDAR";return false;}
    bool ok=std::fwrite(b.data(),1,b.size(),f)==b.size();if(std::fclose(f))ok=false;
    if(!ok){std::remove(tmp.c_str());error="ERROR SD: ARCHIVO ANTERIOR INTACTO";return false;}
    bool had=exists(p);
    if(had&&((std::remove(bak.c_str())&&errno!=ENOENT)||renameFile(p.c_str(),bak.c_str()))){std::remove(tmp.c_str());error="ERROR SD: RESPALDO FALLIDO";return false;}
    if(renameFile(tmp.c_str(),p.c_str())){
        if(had)renameFile(bak.c_str(),p.c_str());
        std::remove(tmp.c_str());error="ERROR SD: CAMBIO NO CONFIRMADO";return false;
    }
    error.clear();return true;
}
}
bool GameOptions::load(const std::string&p){
    Bytes b{};if(!read(p,"UOP1",b)||b[4]>2||b[5]<1||b[5]>4||b[6]>5||b[7]||get(b,8))return false;
    roofs=b[4];zoom=b[5];page=b[6];return true;
}
bool GameOptions::save(const std::string&p,std::string&error,SaveRename renameFile)const{
    if(roofs<0||roofs>2||zoom<1||zoom>4||page<0||page>5)return false;
    Bytes b{{'U','O','P','1'}};b[4]=roofs;b[5]=zoom;b[6]=page;return write(p,b,error,renameFile);
}
std::string Campaign::prefix(uint32_t gen)const{return gen?"game-"+std::to_string(gen)+"-":"";}
bool Campaign::load(){
    generation=0;valid=true;auto p=root+"campaign.sav";
    if(!exists(p)&&!exists(p+".bak"))return true;
    Bytes b{};valid=read(p,"UOC1",b)&&get(b,4)>0&&get(b,4)<1000000000u&&!get(b,8);
    if(valid)generation=get(b,4);
    return valid;
}
uint32_t Campaign::unusedGeneration()const{
    for(uint32_t next=generation+1,tries=0;tries<1024&&next<1000000000u;++next,++tries){bool occupied=false;
        for(const auto&file:{"position-private.sav","inventory-private.sav","position-demo.sav","inventory-demo.sav","appearance.sav"})
            for(const auto&suffix:{"",".bak",".tmp"})if(exists(root+prefix(next)+file+suffix))occupied=true;
        if(!occupied)return next;
    }
    return 0;
}
bool Campaign::commit(uint32_t next,std::string&error,SaveRename renameFile){
    if(!next||next>=1000000000u)return false;
    Bytes b{{'U','O','C','1'}};put(b,4,next);
    if(!write(root+"campaign.sav",b,error,renameFile))return false;
    generation=next;valid=true;return true;
}
void StartMenu::move(int delta){int count=page==MenuPage::Confirm?2:4;selected=(selected+delta+count)%count;}
void StartMenu::back(){page=MenuPage::Main;selected=0;message.clear();}
int StartMenu::hit(int x,int y)const{
    if(x<25||x>=295)return -1;
    int count=page==MenuPage::Confirm?2:4;
    for(int row=0;row<count;++row){int top=page==MenuPage::Confirm?65+row*38:58+row*30;if(y>=top&&y<top+28)return row;}
    return -1;
}
MenuAction StartMenu::activate(bool canContinue){
    if(page==MenuPage::Confirm){if(selected==1)return MenuAction::NewGame;back();return MenuAction::None;}
    if(page==MenuPage::Options){
        if(selected==0)options.roofs=(options.roofs+1)%3;
        if(selected==1)options.zoom=options.zoom%4+1;
        if(selected==2)options.page=(options.page+1)%6;
        if(selected==3)back();
        return MenuAction::None;
    }
    if(selected==0){page=MenuPage::Confirm;selected=0;message="INICIAR DESDE CERO?";}
    else if(selected==1){if(canContinue)return MenuAction::Continue;message="NO HAY UNA PARTIDA PARA CONTINUAR";}
    else if(selected==3)return MenuAction::Exit;
    else {page=MenuPage::Options;selected=0;message.clear();}
    return MenuAction::None;
}
}
