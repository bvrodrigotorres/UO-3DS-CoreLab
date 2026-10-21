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
enum class MenuPage {Main,Options,Confirm};
enum class MenuAction {None,NewGame,Continue};
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
