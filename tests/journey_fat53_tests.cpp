// SPDX-License-Identifier: MIT
#include "journey.hpp"
#include <cassert>
#include <iostream>
namespace {
unsigned calls=0,failMask=0;
int sdRename(const char*source,const char*destination){
    ++calls;if(failMask&(1u<<calls)){errno=EIO;return -1;}
    struct stat st;if(::stat(destination,&st)==0){errno=EEXIST;return -1;}
    return std::rename(source,destination);
}
void failAt(unsigned mask){calls=0;failMask=mask;}
}
int main(int argc,char**argv){
    assert(argc==2);std::string path=std::string(argv[1])+"/fat-journey.sav";
    lab::Journey journey,restored;assert(journey.load(path)&&journey.zone==0);
    failAt(0);assert(journey.save(path,sdRename));
    // The 0.52 writer failed on this second write: SD rename refuses overwrite.
    // Reboot/reload after every crossing and repeat with an existing .bak.
    for(unsigned i=0;i<80;++i){journey.zone=1-journey.zone;journey.returnPoint={1491+int(i%2),1640,24};failAt(0);
        assert(journey.save(path,sdRename)&&calls==2);restored={};assert(restored.load(path)&&restored.zone==journey.zone&&restored.returnPoint.x==journey.returnPoint.x);}
    lab::Journey next=journey;next.zone=1-journey.zone;
    // Failure rotating the primary may not change the active zone.
    failAt(1u<<1);assert(!next.save(path,sdRename));assert(restored.load(path)&&restored.zone==journey.zone);
    // Failure installing the new record restores the primary under SD rules.
    failAt(1u<<2);assert(!next.save(path,sdRename)&&calls==3);assert(restored.load(path)&&restored.zone==journey.zone);
    // If restoring also fails, the old zone survives in .bak and loads safely.
    failAt((1u<<2)|(1u<<3));assert(!next.save(path,sdRename));struct stat st;assert(::stat(path.c_str(),&st)!=0&&errno==ENOENT);assert(restored.load(path)&&restored.zone==journey.zone);
    // Commit from the recovered backup without deleting that recovery copy.
    failAt(0);assert(next.save(path,sdRename)&&calls==1);assert(restored.load(path)&&restored.zone==next.zone);
    // Simulate power loss after rotating the old authority, before installing new.
    failAt(0);assert(journey.save(path,sdRename));std::remove((path+".bak").c_str());assert(sdRename(path.c_str(),(path+".bak").c_str())==0);assert(restored.load(path)&&restored.zone==journey.zone);
    failAt(0);assert(journey.save(path,sdRename));
    {std::ofstream corrupt(path,std::ios::binary);corrupt<<"broken";}assert(!restored.load(path));
    std::cout<<"Journey SD: 80 return trips/reloads, FAT no-overwrite, backup/commit/restore failures, interrupted commit recovery and corrupt-primary rejection passed\n";
}
