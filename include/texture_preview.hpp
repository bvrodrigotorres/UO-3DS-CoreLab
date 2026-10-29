// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <algorithm>
namespace lab {
constexpr unsigned PreviewCell=4,PreviewWidth=512,PreviewColumns=PreviewWidth/PreviewCell,PreviewLimit=32768;
inline uint32_t previewHash(const uint8_t*data,size_t size){uint32_t h=2166136261u;for(size_t i=0;i<size;++i)h=(h^data[i])*16777619u;return h;}
inline unsigned previewHeight(unsigned count){unsigned height=8;while(height<((count+PreviewColumns-1)/PreviewColumns)*PreviewCell)height*=2;return height;}
struct PreviewUv {float left,top,right,bottom;};
inline PreviewUv previewUv(unsigned id,unsigned height){unsigned x=(id%PreviewColumns)*PreviewCell,y=(id/PreviewColumns)*PreviewCell;return {(x+.5f)/PreviewWidth,1-(y+.5f)/height,(x+PreviewCell-.5f)/PreviewWidth,1-(y+PreviewCell-.5f)/height};}
// UPV1 binds these private thumbnails to the exact installed image catalogue.
inline bool readPreviews(const std::vector<uint8_t>&b,unsigned count,uint32_t catalogueHash,std::vector<uint16_t>&out){
    if(!count||count>PreviewLimit||b.size()!=16+size_t(count)*PreviewCell*PreviewCell*2||!std::equal(b.begin(),b.begin()+4,"UPV1"))return false;
    auto n=[&](size_t p){return uint32_t(b[p])|uint32_t(b[p+1])<<8|uint32_t(b[p+2])<<16|uint32_t(b[p+3])<<24;};
    if(n(4)!=count||n(8)!=catalogueHash||n(12)!=previewHash(b.data()+16,b.size()-16))return false;
    std::vector<uint16_t>next(count*PreviewCell*PreviewCell);for(size_t i=0;i<next.size();++i)next[i]=uint16_t(b[16+i*2])|uint16_t(b[17+i*2])<<8;
    out=std::move(next);return true;
}
}
