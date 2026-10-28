// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace lab {
constexpr unsigned QuestCount=3;
// 0: not accepted, 1: active (readiness is derived), 2: reward collected.
struct QuestProgress {uint32_t state=0,count=0,giver=0,signature=0;};
inline bool validQuest(unsigned id,const QuestProgress&q){
    if(q.state>2||(id==2?(q.count&0xffff)>5||(q.count&~0x1ffffu)!=0:q.count>5)||q.giver>1000000)return false;
    if(!q.state)return !q.count&&!q.giver&&!q.signature;
    return q.giver&&q.signature&&(id==0?(q.state!=2||q.count==5):id==2?(q.state!=2||q.count==0x10005u):q.count==0);
}
struct QuestMarker {bool active=false;int x=0,y=0,z=0;};
struct QuestPanel {
    unsigned selected=0;bool ready=false;
    std::string speaker,profession,heading,objective,reward,button,message;
    std::array<std::string,3>dialogue;
};
}
