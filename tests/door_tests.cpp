// SPDX-License-Identifier: MIT
#include "doors.hpp"
#include "exploration.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstring>
using namespace lab;
static void put(std::vector<uint8_t>&b,size_t p,uint32_t n,int count){for(int i=0;i<count;++i)b[p+i]=(n>>(8*i))&255;}
static void write(const std::string&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write((const char*)b.data(),b.size());}
int main(int argc,char**argv){
    assert(argc==2||argc==4);std::string root=argv[1],error;World w;
    if(argc==4){
        assert(loadWorld(argv[2],w,error));Doors d;assert(d.load(argv[3],argv[2],w,error));assert(d.count()==14&&d.baseObjects==2042);
        World legacy=w;legacy.objects.resize(d.baseObjects);legacy.index();
        Actor a{11,15,40},b;
        assert(savePosition(root+"/old.sav",legacy,a,error));assert(loadPosition(root+"/old.sav",legacy,b,error));
        assert(!loadPosition(root+"/old.sav",w,b,error));
        assert(savePosition(root+"/new.sav",w,a,error));World canonical=w;
        assert(d.tryOpen({13,15,40},0,1));assert(d.update(w,.2f));
        assert(loadPosition(root+"/new.sav",canonical,b,error)&&b.z==40);
        for(const auto&o:canonical.objects)if(o.flags&Door){
            Doors one;World runtime=canonical;assert(one.load(argv[3],argv[2],runtime,error));
            assert(one.tryOpen({o.x,o.y-1,o.z},0,1));assert(one.update(runtime,.2f));
            const auto&opened=runtime.objects[&o-canonical.objects.data()];assert(opened.art==o.art+1);
        }
        std::cout<<"Private region: all 14 original door states and legacy save identity verified\n";return 0;
    }
    w.width=w.height=8;w.images=2;w.vertices.assign(81,0);w.land.resize(64);
    Object door;door.x=3;door.y=3;door.art=1701;door.image=0;door.flags=Door|Impassable;door.height=20;w.objects={door};w.index();
    std::string scene=root+"/door-scene",file=root+"/doors.bin";std::vector<uint8_t>raw={1,2,3};write(scene,raw);
    uint32_t hash=2166136261u;for(auto v:raw)hash=(hash^v)*16777619u;
    std::vector<uint8_t>b(28);std::memcpy(b.data(),"UOD1",4);put(b,4,1,2);put(b,6,1,2);put(b,8,0,4);put(b,12,hash,4);put(b,16,0,4);put(b,20,1702,2);put(b,22,1,2);b[24]=255;b[25]=1;put(b,26,65535,2);write(file,b);
    Doors d;assert(d.load(file,scene,w,error));Actor a{3,2,0},next;int actual=0;
    assert(!step(w,a,4,next,actual));assert(!d.tryOpen({3,2,40},0,1));
    assert(d.tryOpen(a,0,1));assert(!d.update(w,.08f));assert(!step(w,a,4,next,actual));
    DoorVisual v;assert(d.visual(0,v)&&v.x<3&&v.image==0);assert(!d.visual(99,v));
    assert(d.update(w,.11f));assert(w.objects[0].x==2&&w.objects[0].y==4&&w.objects[0].art==1702);
    assert(step(w,a,4,next,actual)&&next.x==3&&next.y==3);
    assert(!d.tryOpen(a,0,1)&&!d.update(w,1));assert(w.cells[3*8+3].empty());
    // Closing waits for both current and reserved crossing cells.
    assert(!d.update(w,3.f,{{3,3,0},{3,2,0}}));
    assert(w.objects[0].art==1702);
    assert(!d.update(w,2.4f,{{3,2,0}}));
    assert(!d.update(w,.11f,{{3,2,0}}));
    assert(d.visual(0,v)&&v.x>2&&v.x<3);
    assert(!d.update(w,.1f,{{3,3,0}})); // someone enters during closing: reopen
    assert(w.objects[0].art==1702);
    assert(d.update(w,3.f,{{3,2,0}}));
    assert(w.objects[0].art==1701&&!step(w,a,4,next,actual));
    assert(d.tryOpen(a,0,1)&&d.update(w,.2f)); // repeatable opening
    auto invalid=[&](std::vector<uint8_t>bad){write(file,bad);Doors other;assert(!other.load(file,scene,w,error));};
    invalid(std::vector<uint8_t>(b.begin(),b.end()-1));auto bad=b;put(bad,16,90,4);invalid(bad);bad=b;put(bad,22,30,2);invalid(bad);bad=b;put(bad,26,0,2);invalid(bad);bad=b;put(bad,12,123,4);invalid(bad);
    // Linked double leaves open together and leave both original cells free.
    w.objects={door,door};w.objects[1].x=4;w.index();b.resize(40);put(b,6,2,2);put(b,26,1,2);
    put(b,28,1,4);put(b,32,1704,2);put(b,34,1,2);b[36]=1;b[37]=1;put(b,38,0,2);write(file,b);
    Doors pair;assert(pair.load(file,scene,w,error));assert(pair.tryOpen(a,0,1));assert(pair.update(w,.2f));assert(w.objects[0].x==2&&w.objects[1].x==5);
    assert(!pair.update(w,3.f,{{4,3,0}})); // second leaf protects the whole pair
    assert(w.objects[0].x==2&&w.objects[1].x==5);
    assert(pair.update(w,3.f));assert(w.objects[0].x==3&&w.objects[1].x==4);
    std::cout<<"Door opening delay, closing safety, collision, original frames, pairs and invalid data passed\n";
}
