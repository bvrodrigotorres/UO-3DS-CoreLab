// SPDX-License-Identifier: MIT
#include "world_expedition.hpp"
#include "view.hpp"
#include "atlas_layout.hpp"
#include "developer_menu.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace lab;
static uint32_t get(const std::vector<uint8_t>&b,size_t p,unsigned n){uint32_t v=0;for(unsigned i=0;i<n;++i)v|=uint32_t(b[p+i])<<(i*8);return v;}
int main(int argc,char**argv){if(argc!=3)return 1;std::string root=std::string(argv[2])+"/",error;WorldArchive archive;WorldGraphics graphics;if(!archive.open(argv[1],error)||!graphics.load(root,error)){std::cerr<<error;return 2;}
 std::ifstream file(root+"image_sizes.bin",std::ios::binary|std::ios::ate);std::vector<uint8_t>b(size_t(file.tellg()));file.seekg(0);file.read((char*)b.data(),b.size());unsigned images=get(b,4,2),pages=get(b,6,2);std::vector<std::array<unsigned,2>>sizes;std::vector<unsigned>costs;for(unsigned i=0;i<images;++i)sizes.push_back({get(b,8+i*4,2),get(b,10+i*4,2)});for(unsigned i=0;i<pages;++i)costs.push_back(get(b,8+images*4+i*4,4));
 const Actor destinations[]={{1496,1628,20},{1845,2745,0},{4467,1283,0},{2476,413,15},{633,858,0},{635,2236,0},{2880,700,0},{1380,3815,0},{3732,2140,20},{2707,2162,0},{6208,3776,0}};
 std::ifstream lf(root+"atlas_layout.bin",std::ios::binary);std::vector<uint8_t>lb((std::istreambuf_iterator<char>(lf)),{});unsigned slots=32;if(!lb.empty()&&!readAtlasLayout(lb,slots))return 4;
 unsigned maximum=0;size_t maxPages=0;for(auto g:destinations){World w;if(!archive.window(ExpeditionWindow::origin(g.x,archive.width()),ExpeditionWindow::origin(g.y,archive.height()),128,w,error)||!graphics.bind(w,error))return 3;Actor a=ExpeditionWindow::local(w,g),safe;Npcs empty;bool landing=safeTeleportTile(w,empty,a,safe);if(!landing){a.z=heights(w,a.x,a.y).average;landing=safeTeleportTile(w,empty,a,safe);}if(landing)a=safe;std::set<unsigned>needed;
 for(const auto&d:renderList(w,a,false))if(worldVisible(w,d,a.x,a.y,a.z,sizes)){auto id=d.kind==Kind::Land?w.land[d.index].image:d.kind==Kind::Static?w.objects[d.index].image:NoImage;if(id!=NoImage)needed.insert(id/slots);}unsigned total=0;for(auto n:needed)total+=costs[n];maximum=std::max(maximum,total);maxPages=std::max(maxPages,needed.size());std::cout<<g.x<<","<<g.y<<" landing="<<landing<<" pages="<<needed.size()<<" raw_atlas_file_bytes="<<total<<"\n";}
 std::cout<<"Maximum sampled visible atlas file bytes="<<maximum<<", pages="<<maxPages<<" (not GPU allocation measurement)\n";
}
