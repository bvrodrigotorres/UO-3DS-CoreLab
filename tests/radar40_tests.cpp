// SPDX-License-Identifier: MIT
#include "avatar.hpp"
#include "mount.hpp"
#include "radar_markers.hpp"
#include "hud.hpp"
#include <cassert>
#include <iostream>
using namespace lab;
int main(){
 // Main movement sets facing=(world direction+1)&7. Horse and foot must select
 // the same exported direction for standing, walking, running and striking.
 for(int world=0;world<8;++world){int facing=(world+1)&7;unsigned bank=avatarImage(facing,false,false,0,72)/5;
  assert(bank==unsigned(world));
  assert(mountedFrame(facing,false,false,0)==bank*11);
  for(unsigned phase=0;phase<5;++phase){
   assert(mountedFrame(facing,true,false,phase)==bank*11+1+phase);
   assert(mountedFrame(facing,true,true,phase)==bank*11+6+phase);
   assert(mountedFrame(facing,false,false,phase,true)==88+bank*5+phase);
  }
 }
 World w;w.width=w.height=128;w.land.resize(128*128);w.vertices.resize(129*129);w.index();Actor a{64,64,0};Npcs population;
 for(int i=0;i<4;++i){Npc n;n.animal=i!=1;n.position=n.from={i==0?70:i==2?58:64,i==1?70:i==3?58:64,0};n.aggressive=i==2;n.hostile=i==3;population.combatants().push_back(n);}
 const std::array<uint8_t,3> gray{{190,190,190}},yellow{{250,220,45}},red{{240,45,35}};
 std::vector<uint8_t>fb(320*240*3+32,0xa5);
 auto at=[&](int x,int y){size_t p=16+3*(x*240+239-y);return std::array<uint8_t,3>{{fb[p+2],fb[p+1],fb[p]}};};
 for(int zoom=1;zoom<=4;++zoom){drawJournal(fb.data()+16,w,a,nullptr,zoom,"",nullptr,nullptr,nullptr,nullptr,&population);
  for(int i=0;i<4;++i){RadarMarker m;assert(radarMarker(population.all()[i],a,zoom,m));auto expected=i==0?gray:i==1?yellow:red;assert(m.color==expected&&at(m.x,m.y)==expected);}
 }
 RadarMarker m;auto&neutral=population.combatants()[0];neutral.hostile=true;assert(radarMarker(neutral,a,2,m)&&m.color==red);neutral.alive=false;assert(!radarMarker(neutral,a,2,m));drawJournal(fb.data()+16,w,a,nullptr,2,"",nullptr,nullptr,nullptr,nullptr,&population);assert(at(115,89)!=red); // Cached terrain must clear old points.
 neutral.alive=true;neutral.hostile=false;neutral.moving=true;neutral.from={64,64,0};neutral.position={70,64,0};neutral.progress=.5f;assert(radarMarker(neutral,a,2,m)&&m.x==109&&m.y==83);
 neutral.moving=false;neutral.position={5000,-5000,0};assert(!radarMarker(neutral,a,4,m));
 // Dots touching a map edge cannot spill into headings, equipment or status.
 for(int x=-40;x<=40;++x)for(int y=-40;y<=40;++y){neutral.position=neutral.from={a.x+x,a.y+y,0};if(radarMarker(neutral,a,2,m))assert(m.x>=20&&m.x<186&&m.y>=JournalLayout::MapTop&&m.y<JournalLayout::MapBottom);}
 neutral.position=neutral.from={45,45,0};drawJournal(fb.data()+16,w,a,nullptr,2,"",nullptr,nullptr,nullptr,nullptr,&population);
 for(int i=0;i<16;++i){assert(fb[i]==0xa5&&fb[fb.size()-1-i]==0xa5);}
 std::cout<<"Radar 40: shared foot/mount directions in all poses, marker colors/hostility, movement, death/trail clearing, zoom and clipped framebuffer bounds passed\n";
}
