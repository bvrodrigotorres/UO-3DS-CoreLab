// SPDX-License-Identifier: MIT
#pragma once
#include "exploration.hpp"
namespace lab {
struct GameOptions {
    int roofs=0,zoom=2,page=0;
    bool load(const std::string&);
    bool save(const std::string&,std::string&,SaveRename=std::rename)const;
};
struct Campaign {
    uint32_t generation=0;
    bool valid=true;
    std::string root;
    explicit Campaign(std::string directory):root(std::move(directory)){}
    bool load();
    std::string prefix(uint32_t)const;
    std::string prefix()const{return prefix(generation);}
    uint32_t unusedGeneration()const;
    bool commit(uint32_t,std::string&,SaveRename=std::rename);
};
enum class PauseAction {None,Resume,Customize,Equipment,Options,Developer,Explore,MainMenu,Exit};
struct PauseMenu {
    bool active=false,developer=false,saveOnPresent=false;int selected=0;uint64_t unlockSince=0;std::string message;
    void open(){active=true;selected=0;developer=false;unlockSince=0;saveOnPresent=true;message="GUARDANDO PARTIDA...";}
    bool presented(){bool save=saveOnPresent;saveOnPresent=false;return save;}
    void holdDeveloper(bool both,uint64_t now){if(!both){unlockSince=0;return;}if(!unlockSince)unlockSince=now;if(now-unlockSince>=2000)developer=true;}
    void move(int delta){selected=(selected+delta+7)%7;}
    int hit(int x,int y)const{if(x<25||x>=295)return -1;for(int i=0;i<7;++i)if(y>=50+i*19&&y<67+i*19)return i;return -1;}
    PauseAction activate()const{return selected==0?PauseAction::Resume:selected==1?PauseAction::Customize:selected==2?PauseAction::Equipment:selected==3?(developer?PauseAction::Developer:PauseAction::Options):selected==4?PauseAction::Explore:selected==5?PauseAction::MainMenu:PauseAction::Exit;}
};
enum class MenuPage {Main,Options,Confirm};
enum class MenuAction {None,NewGame,Continue,Exit};
struct StartMenu {
    MenuPage page=MenuPage::Main;
    int selected=0;
    GameOptions options;
    std::string message;
    void move(int delta);
    void back();
    MenuAction activate(bool canContinue);
    int hit(int x,int y)const;
};
}
