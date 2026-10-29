// SPDX-License-Identifier: MIT
#include "texture_pack.hpp"
#include "packed_file_reader.hpp"
#include <cassert>
#include <iostream>
#include <iterator>
int main(int argc,char**argv){assert(argc==3);std::string source=std::string(argv[1])+"/",packed=std::string(argv[2])+"/";
 std::ifstream sizes(source+"image_sizes.bin",std::ios::binary);std::vector<uint8_t>b((std::istreambuf_iterator<char>(sizes)),{});
 auto n=[&](size_t p,unsigned k){uint32_t v=0;for(unsigned i=0;i<k;++i)v|=uint32_t(b[p+i])<<(i*8);return v;};unsigned images=n(4,2),pages=n(6,2);
 std::vector<size_t>costs;for(unsigned i=0;i<pages;++i)costs.push_back(n(8+images*4+i*4,4));
 std::ifstream archive(packed+"textures.pack",std::ios::binary|std::ios::ate);assert(archive);uint64_t length=uint64_t(archive.tellg());
 std::vector<lab::TextureSlice>slices;assert(lab::readTextureIndex(packed+"textures.idx",length,costs,slices));
 lab::PackedFileReader reader;std::vector<uint8_t>bytes;uint64_t total=0;
 for(unsigned i=0;i<pages;++i){auto s=slices[i];assert(reader.read(packed+"textures.pack",s.offset,s.size,bytes));uint32_t h=2166136261u;for(uint8_t v:bytes)h=(h^v)*16777619u;assert(h==s.checksum);
  char name[64];std::snprintf(name,sizeof name,"atlas_%02u.t3x",i);std::ifstream original(source+name,std::ios::binary);std::vector<uint8_t>old((std::istreambuf_iterator<char>(original)),{});assert(bytes==old);total+=bytes.size();}
 assert(reader.opens()==1);std::cout<<pages<<" actual pages verified byte-for-byte and by checksum, "<<total<<" bytes, "<<reader.opens()<<" archive open, "<<reader.reads()<<" block reads\n";
}
