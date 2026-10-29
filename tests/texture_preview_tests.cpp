// SPDX-License-Identifier: MIT
#include "texture_preview.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
using namespace lab;
static void set(std::vector<uint8_t>&b,size_t p,uint32_t n){for(unsigned i=0;i<4;++i)b[p+i]=uint8_t(n>>(8*i));}
int main(int argc,char**argv){
    assert(previewHash(reinterpret_cast<const uint8_t*>("hello"),5)==0x4f9f2cabu);
    std::vector<uint8_t>b(16+14596*32);b[0]='U';b[1]='P';b[2]='V';b[3]='1';set(b,4,14596);set(b,8,123);
    for(size_t i=16;i<b.size();++i)b[i]=uint8_t(i*17);
    set(b,12,previewHash(b.data()+16,b.size()-16));std::vector<uint16_t>pixels;
    assert(readPreviews(b,14596,123,pixels)&&pixels.size()==14596*16);assert(pixels[0]==uint16_t(b[16]|unsigned(b[17])<<8));auto valid=pixels;
    assert(!readPreviews(b,14595,123,pixels)&&pixels==valid);assert(!readPreviews(b,14596,124,pixels)&&pixels==valid);
    b.back()^=1;assert(!readPreviews(b,14596,123,pixels)&&pixels==valid);b.back()^=1;
    b.push_back(0);assert(!readPreviews(b,14596,123,pixels));b.pop_back();b.resize(8);assert(!readPreviews(b,14596,123,pixels));
    assert(!readPreviews({},0,0,pixels)&&!readPreviews({},PreviewLimit+1,0,pixels));
    for(unsigned count:{1u,8388u,14596u,PreviewLimit}){
        unsigned height=previewHeight(count);assert(height>=8&&height<=1024&&(height&(height-1))==0);
        for(unsigned i=0;i<count;++i){auto uv=previewUv(i,height);unsigned x=(i%PreviewColumns)*4,y=(i/PreviewColumns)*4;
            assert(uv.left*512==x+.5f&&uv.right*512==x+3.5f);assert((1-uv.top)*height==y+.5f&&(1-uv.bottom)*height==y+3.5f);
            assert(uv.left>0&&uv.right<1&&uv.bottom>0&&uv.top<1); // Bilinear taps never cross into another sprite.
        }
    }
    assert(PreviewWidth*previewHeight(14596)*2==512*1024); // Fixed small residency, no full-resolution duplicates.
    if(argc==3){std::ifstream m(argv[1],std::ios::binary),f(argv[2],std::ios::binary);std::vector<uint8_t>metadata{std::istreambuf_iterator<char>(m),{}},file{std::istreambuf_iterator<char>(f),{}};assert(metadata.size()>=8);unsigned n=metadata[4]|unsigned(metadata[5])<<8;assert(readPreviews(file,n,previewHash(metadata.data(),metadata.size()),pixels));}
    std::cout<<"Texture previews: catalogue/checksum validation, corruption rollback, bounded memory and bleed-free UVs passed\n";
}
