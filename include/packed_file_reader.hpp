// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <algorithm>
namespace lab {
// Owned exclusively by the disk worker. One open file and 64 KiB read-ahead,
// independent of the size of the world or the number of texture pages.
class PackedFileReader {
    std::ifstream file;std::string path;uint64_t length=0,start=UINT64_MAX;size_t available=0;
    std::vector<uint8_t>cache=std::vector<uint8_t>(64*1024);
    unsigned openCount=0,readCount=0;
public:
    unsigned opens()const{return openCount;}unsigned reads()const{return readCount;}
    bool read(const std::string&next,uint64_t offset,unsigned size,std::vector<uint8_t>&out){
        out.clear();
        if(path!=next||!file.is_open()){
            file.close();file.clear();path.clear();start=UINT64_MAX;available=0;
            file.open(next,std::ios::binary|std::ios::ate);if(!file)return false;
            auto end=file.tellg();if(end<0){file.close();return false;}length=uint64_t(end);path=next;++openCount;
        }
        if(!size||size>4*1024*1024||offset>length||size>length-offset)return false;
        out.resize(size);size_t copied=0;
        while(copied<size){uint64_t at=offset+copied;
            if(start==UINT64_MAX||at<start||at>=start+available){
                start=at/cache.size()*cache.size();available=size_t(std::min<uint64_t>(cache.size(),length-start));file.clear();file.seekg(std::streamoff(start));
                if(!file.read(reinterpret_cast<char*>(cache.data()),available)){out.clear();file.close();path.clear();return false;}++readCount;
            }
            size_t local=size_t(at-start),amount=std::min(size-copied,available-local);
            std::copy_n(cache.data()+local,amount,out.data()+copied);copied+=amount;
        }
        return true;
    }
};
}
