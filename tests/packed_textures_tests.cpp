// SPDX-License-Identifier: MIT
#include "packed_file_reader.hpp"
#include "texture_pack.hpp"
#include "save_queue.hpp"
#include <cassert>
#include <thread>
#include <chrono>
#include <iostream>
using namespace lab;
static void put(std::vector<uint8_t>&b,uint64_t n,unsigned count){for(unsigned i=0;i<count;++i)b.push_back(uint8_t(n>>(i*8)));}
static void write(const std::string&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write((const char*)b.data(),b.size());assert(f.good());}
static uint32_t hash(const std::vector<uint8_t>&b){uint32_t h=2166136261u;for(auto n:b)h=(h^n)*16777619u;return h;}
static void wait(const std::function<bool()>&f){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(8);while(!f()){assert(std::chrono::steady_clock::now()<end);std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
int main(int argc,char**argv){assert(argc==2);std::string root=std::string(argv[1])+"/";
 std::vector<uint8_t>data(192*1024+33);for(size_t i=0;i<data.size();++i)data[i]=uint8_t(i*31+(i>>8));write(root+"textures.pack",data);
 PackedFileReader reader;std::vector<uint8_t>out;
 for(unsigned i=0;i<256;++i){unsigned offset=(i*157)%60000;assert(reader.read(root+"textures.pack",offset,128,out));assert(out==std::vector<uint8_t>(data.begin()+offset,data.begin()+offset+128));}
 assert(reader.opens()==1&&reader.reads()==1);
 assert(reader.read(root+"textures.pack",65520,100000,out));assert(out==std::vector<uint8_t>(data.begin()+65520,data.begin()+165520));assert(reader.opens()==1);
 assert(!reader.read(root+"textures.pack",UINT64_MAX,2,out)&&out.empty());assert(!reader.read(root+"textures.pack",data.size()-1,2,out));assert(!reader.read(root+"missing",0,1,out));
 // Index validation is transactional: duplicates/overflow/truncation and size
 // mismatches cannot mutate the previously usable page table.
 std::vector<uint8_t>index{'U','T','P','1'};put(index,2,4);put(index,0,8);put(index,65536,4);put(index,1,4);put(index,65536,8);put(index,data.size()-65536,4);put(index,2,4);write(root+"textures.idx",index);
 std::vector<size_t>costs{65536,data.size()-65536};std::vector<TextureSlice>slices;
 assert(readTextureIndex(root+"textures.idx",data.size(),costs,slices)&&slices.size()==2);
 auto valid=index;index[8+16+2]=0;write(root+"textures.idx",index);assert(!readTextureIndex(root+"textures.idx",data.size(),costs,slices)&&slices.size()==2);
 index=valid;index.resize(index.size()-1);write(root+"textures.idx",index);assert(!readTextureIndex(root+"textures.idx",data.size(),costs,slices));
 index=valid;for(unsigned i=0;i<8;++i)index[8+16+i]=255;write(root+"textures.idx",index);assert(!readTextureIndex(root+"textures.idx",data.size(),costs,slices));
 write(root+"textures.idx",valid);assert(!readTextureIndex(root+"textures.idx",data.size()-1,costs,slices));
 AsyncResources resources;assert(resources.start());ResourceRead file;
 for(unsigned i=0;i<4;++i){auto bytes=std::vector<uint8_t>(data.begin()+i*512,data.begin()+(i+1)*512);assert(resources.requestSlice(root+"page"+std::to_string(i),root+"textures.pack",i*512,512,hash(bytes)));}
 for(unsigned i=0;i<4;++i){wait([&]{return resources.take(root+"page"+std::to_string(i),file);});assert(file.valid&&file.bytes==std::vector<uint8_t>(data.begin()+i*512,data.begin()+(i+1)*512));}
 assert(resources.requestSlice("bad-checksum",root+"textures.pack",0,32,1));wait([&]{return resources.take("bad-checksum",file);});assert(!file.valid&&file.bytes.empty());
 assert(resources.requestSlice("bad-range",root+"textures.pack",data.size(),32,0));wait([&]{return resources.take("bad-range",file);});assert(!file.valid&&file.bytes.empty());
 assert(resources.request(root+"textures.pack",unsigned(data.size()),hash(data)));wait([&]{return resources.take(root+"textures.pack",file);});assert(file.valid&&file.bytes==data);
 std::cout<<"Packed textures: 256 page reads use one open/one 64-KiB disk block; boundary reads, corruption, transactional index, four async slices and legacy files passed\n";
}
