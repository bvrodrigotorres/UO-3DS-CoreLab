// SPDX-License-Identifier: MIT
#include "mount.hpp"
#include "startup.hpp"
#include "session_save.hpp"
#include "save_queue.hpp"
#include "hud.hpp"
#include <atomic>
#include <thread>
#include <chrono>
#include <fstream>
#include <cassert>
#include <iostream>
using namespace lab;
static std::vector<uint8_t>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
static uint32_t hash(const std::vector<uint8_t>&b,size_t n){uint32_t h=2166136261;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619;return h;}
static void put(std::vector<uint8_t>&b,size_t at,uint32_t n){for(int i=0;i<4;++i)b[at+i]=n>>(8*i);}
int main(int argc,char**argv){
 assert(argc==2);std::string root=std::string(argv[1])+"/";
 assert(movementSeconds(false,false)==.4f&&movementSeconds(true,false)==.2f&&movementSeconds(false,true)==.2f&&movementSeconds(true,true)==.1f);
 bool poses[128]{};for(int d=0;d<8;++d){poses[mountedFrame(d,false,false,99)]=true;for(unsigned n=0;n<5;++n){poses[mountedFrame(d,true,false,n)]=true;poses[mountedFrame(d,true,true,n)]=true;poses[mountedFrame(d,false,false,n,true)]=true;assert(mountedFrame(d,true,true,n,true)==mountedFrame(d,true,true,n));}}for(bool p:poses)assert(p);
 assert(horseIconHit(215,212)&&!horseIconHit(215,202)&&!horseIconHit(258,220)&&!horseIconHit(220,90));
 PauseMenu pause;pause.open();pause.selected=3;assert(pause.activate()==PauseAction::Options&&pause.saveOnPresent);pause.holdDeveloper(true,100);pause.holdDeveloper(true,2099);assert(!pause.developer);pause.holdDeveloper(false,2100);pause.holdDeveloper(true,3000);pause.holdDeveloper(true,5000);assert(pause.activate()==PauseAction::Developer);pause.open();assert(!pause.developer&&pause.activate()==PauseAction::Resume);
 World w;w.width=w.height=16;w.land.resize(256);w.vertices.resize(289);w.index();Actor actor{5,6,0};Inventory bag;bag.init(w,root+"bag.sav");PlayerStatus status;status.mounted=true;status.hits=27;status.skills.start(5);
 std::atomic<bool>entered{false},release{false};AsyncSaves saves([&](const std::string&p,const std::vector<uint8_t>&b,std::string&e){entered=true;while(!release)std::this_thread::sleep_for(std::chrono::milliseconds(1));return atomicSaveBytes(p,b,e);});assert(saves.start());SaveSink sink=[&](const std::string&p,std::vector<uint8_t>b){return saves.enqueue(p,std::move(b));};bag.saveSink=sink;
 // The first paused framebuffer exists before submitting any disk work; writes may block without blocking input.
 std::vector<uint8_t>fb(320*240*3+32,0xa5);drawPause(fb.data()+16,pause);assert(!entered&&pause.presented()&&!pause.presented());std::string message;assert(saveSession(root+"position.sav",root+"vitals.sav",w,actor,bag,status,message,sink));assert(message=="GUARDANDO PARTIDA...");auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(!entered){assert(std::chrono::steady_clock::now()<until);std::this_thread::yield();}assert(saves.status().pending==3&&read(root+"position.sav").empty());pause.move(1);assert(pause.selected==1);release=true;assert(saves.flush());
 PlayerStatus loaded;assert(loadVitals(root+"vitals.sav",loaded)&&loaded.mounted&&loaded.hits==27&&loaded.skills.total()==1000);Actor at;assert(loadPosition(root+"position.sav",w,at,message)&&at.x==5&&at.y==6);
 auto bytes=read(root+"vitals.sav");assert(std::string(bytes.begin(),bytes.begin()+4)=="UVT4");auto corrupt=bytes;put(corrupt,20+Skills::WireSize,2);put(corrupt,corrupt.size()-4,hash(corrupt,corrupt.size()-4));{std::ofstream f(root+"corrupt.sav",std::ios::binary);f.write((char*)corrupt.data(),corrupt.size());}assert(!loadVitals(root+"corrupt.sav",loaded)&&loaded.mounted);
 // Backward compatibility: UVT3 skill data and healing shortcut remain unchanged; old profiles start on foot.
 bytes.erase(bytes.end()-8,bytes.end()-4);bytes[3]='3';put(bytes,bytes.size()-4,hash(bytes,bytes.size()-4));{std::ofstream f(root+"old.sav",std::ios::binary);f.write((char*)bytes.data(),bytes.size());}assert(loadVitals(root+"old.sav",loaded)&&!loaded.mounted&&loaded.skills.total()==1000&&loaded.hits==27);
 for(bool hidden:{false,true}){pause.developer=hidden;for(int i=0;i<6;++i){pause.selected=i;drawPause(fb.data()+16,pause);}}for(int i=0;i<16;++i)assert(fb[i]==0xa5&&fb[fb.size()-1-i]==0xa5);
 std::cout<<"Mount 39: all 128 poses, original speeds, icon bounds, hidden developer hold, pause-before-save with stalled SD, UVT4/UVT3 compatibility and UI bounds passed\n";
}
