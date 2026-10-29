// SPDX-License-Identifier: MIT
#pragma once
#include <vector>
#include <map>
#include <algorithm>
#include <cstdint>
#include <cstddef>
namespace lab {
// Immutable packed columns plus sparse overrides for moving doors. Empty map
// squares cost four bytes, rather than one heap-owning vector per square.
class CellIndex {
    std::vector<uint32_t>offsets,items;
    std::map<size_t,std::vector<uint32_t>>changed;
public:
    struct Range {const uint32_t*data=nullptr;size_t count=0;const uint32_t*begin()const{return data;}const uint32_t*end()const{return count?data+count:data;}size_t size()const{return count;}bool empty()const{return !count;}};
    size_t size()const{return offsets.empty()?0:offsets.size()-1;}
    size_t capacity()const{return offsets.empty()?0:offsets.capacity()-1;}
    bool empty()const{return !size();}
    size_t bytes()const{size_t n=offsets.capacity()*4+items.capacity()*4;for(const auto&v:changed)n+=64+v.second.capacity()*4;return n;}
    Range operator[](size_t cell)const{
        if(offsets[cell]&0x80000000u){const auto&v=changed.find(cell)->second;return {v.data(),v.size()};}
        size_t start=offsets[cell]&0x7fffffffu,count=(offsets[cell+1]&0x7fffffffu)-start;return {count?items.data()+start:nullptr,count};
    }
    template<class Locate> void build(size_t cells,size_t count,Locate locate){
        changed.clear();offsets.assign(cells+1,0);size_t total=0;
        for(size_t i=0;i<count;++i){int c=locate(i);if(c>=0){++offsets[size_t(c)+1];++total;}}
        for(size_t i=1;i<offsets.size();++i)offsets[i]+=offsets[i-1];
        items.resize(total);
        for(size_t i=count;i>0;--i){int c=locate(i-1);if(c>=0)items[--offsets[size_t(c)+1]]=uint32_t(i-1);}
        for(size_t i=0;i<cells;++i)offsets[i]=offsets[i+1];
        offsets[cells]=uint32_t(total);
    }
    void remove(size_t cell,size_t item){auto&v=edit(cell);v.erase(std::remove(v.begin(),v.end(),item),v.end());}
    void add(size_t cell,size_t item){auto&v=edit(cell);auto at=std::lower_bound(v.begin(),v.end(),item);if(at==v.end()||*at!=item)v.insert(at,uint32_t(item));}
    bool operator==(const CellIndex&other)const{if(size()!=other.size())return false;for(size_t i=0;i<size();++i){auto a=(*this)[i],b=other[i];if(a.size()!=b.size()||!std::equal(a.begin(),a.end(),b.begin()))return false;}return true;}
private:
    std::vector<uint32_t>&edit(size_t cell){auto at=changed.find(cell);if(at!=changed.end())return at->second;auto r=(*this)[cell];auto&v=changed[cell];if(!r.empty())v.assign(r.begin(),r.end());offsets[cell]|=0x80000000u;return v;}
};
}
