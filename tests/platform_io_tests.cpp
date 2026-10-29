// SPDX-License-Identifier: MIT
#include "platform_io.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <atomic>
#include <thread>
#include <vector>
#include <array>
#include <cassert>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <iostream>
struct _reent {int _errno=0;};
extern "C" {
int __wrap__open_r(_reent*,const char*,int,int);int __wrap__close_r(_reent*,int);
ssize_t __wrap__read_r(_reent*,int,void*,size_t);ssize_t __wrap__write_r(_reent*,int,const void*,size_t);
off_t __wrap__lseek_r(_reent*,int,off_t,int);
int __wrap__stat_r(_reent*,const char*,struct stat*);int __wrap__fstat_r(_reent*,int,struct stat*);
int __wrap__rename_r(_reent*,const char*,const char*);int __wrap__unlink_r(_reent*,const char*);
}
static std::atomic<int>active{0},peak{0};
struct Backend {Backend(){int n=++active;int old=peak.load();while(old<n&&!peak.compare_exchange_weak(old,n)){}std::this_thread::yield();}~Backend(){--active;}};
struct File {std::vector<uint8_t>bytes;size_t at=0;};static std::array<File,16>files;
static bool shortIo=false;static int readError=-1,writeError=-1;
extern "C" {
int __real__open_r(_reent*,const char*,int,int){Backend b;return 3;}
int __real__close_r(_reent*,int){Backend b;return 0;}
int __real__stat_r(_reent*,const char*,struct stat*s){Backend b;std::memset(s,0,sizeof(*s));return 0;}
int __real__fstat_r(_reent*,int,struct stat*s){Backend b;std::memset(s,0,sizeof(*s));return 0;}
off_t __real__lseek_r(_reent*,int fd,off_t offset,int){Backend b;files[unsigned(fd)].at=size_t(offset);return offset;}
int __real__rename_r(_reent*,const char*,const char*){Backend b;return 0;}
int __real__unlink_r(_reent*,const char*){Backend b;return 0;}
ssize_t __real__read_r(_reent*r,int fd,void*data,size_t count){Backend b;assert(count<=lab::SdTransferBytes&&(uintptr_t(data)&4095)==0);
 if(readError>=0&&readError--==0){r->_errno=EIO;return -1;}auto&f=files[unsigned(fd)];size_t n=std::min(count,f.bytes.size()-f.at);if(shortIo)n=std::min<size_t>(n,113);std::memcpy(data,f.bytes.data()+f.at,n);f.at+=n;return ssize_t(n);}
ssize_t __real__write_r(_reent*r,int fd,const void*data,size_t count){Backend b;assert(count<=lab::SdTransferBytes&&(uintptr_t(data)&4095)==0);
 if(writeError>=0&&writeError--==0){r->_errno=EIO;return -1;}auto&f=files[unsigned(fd)];size_t n=shortIo?std::min<size_t>(count,97):count;if(f.at+n>f.bytes.size())f.bytes.resize(f.at+n);std::memcpy(f.bytes.data()+f.at,data,n);f.at+=n;return ssize_t(n);}
}
int main(){
 // Simulate concurrent terrain, NPC animation, mounted appearance and save I/O.
 // Different descriptors have independent streams; no two service calls overlap.
 std::vector<uint8_t>expected(350123);for(size_t i=0;i<expected.size();++i)expected[i]=uint8_t(i*13+(i>>7));
 for(unsigned i=0;i<4;++i)files[i].bytes=expected;
 std::vector<std::thread>workers;
 for(unsigned id=0;id<8;++id)workers.emplace_back([&,id]{_reent r;struct stat s;std::vector<uint8_t>unaligned(expected.size()+2,0xa5);
  for(int repeat=0;repeat<10;++repeat){__wrap__open_r(&r,"asset.bin",0,0);__wrap__lseek_r(&r,int(id),0,0);__wrap__stat_r(&r,"asset.bin",&s);__wrap__fstat_r(&r,int(id),&s);
   if(id<4){assert(__wrap__read_r(&r,int(id),unaligned.data()+1,expected.size())==ssize_t(expected.size()));assert(std::equal(expected.begin(),expected.end(),unaligned.begin()+1));assert(unaligned.front()==0xa5&&unaligned.back()==0xa5);}
   else assert(__wrap__write_r(&r,int(id),expected.data(),expected.size())==ssize_t(expected.size()));
   __wrap__rename_r(&r,"save.tmp","save.sav");__wrap__unlink_r(&r,"save.bak");__wrap__close_r(&r,int(id));
  }
 });
 for(auto&t:workers)t.join();
 assert(peak==1&&active==0);
 for(unsigned i=4;i<8;++i)assert(files[i].bytes==expected);
 _reent r;std::vector<uint8_t>out(expected.size()+20,0xa5);files[0].at=0;
 assert(__wrap__read_r(&r,0,out.data(),out.size())==ssize_t(expected.size()));assert(std::equal(expected.begin(),expected.end(),out.begin()));
 shortIo=true;files[0].at=0;assert(__wrap__read_r(&r,0,out.data(),expected.size())==ssize_t(expected.size()));files[8]={};assert(__wrap__write_r(&r,8,expected.data(),expected.size())==ssize_t(expected.size())&&files[8].bytes==expected);shortIo=false;
 files[0].at=0;readError=1;assert(__wrap__read_r(&r,0,out.data(),expected.size())==ssize_t(lab::SdTransferBytes)&&r._errno==EIO);readError=-1;
 files[9]={};writeError=1;assert(__wrap__write_r(&r,9,expected.data(),expected.size())==ssize_t(lab::SdTransferBytes));writeError=-1;
 assert(__wrap__read_r(&r,0,nullptr,0)==0&&__wrap__write_r(&r,0,nullptr,0)==0);assert(__wrap__read_r(&r,0,nullptr,1)==-1&&r._errno==EFAULT);
 auto stats=lab::platformIoStats();assert(stats.largestTransfer==lab::SdTransferBytes&&stats.readBytes>4*expected.size()*10&&stats.writtenBytes>4*expected.size()*10&&stats.failures>=2);
 std::cout<<"Platform I/O: eight concurrent menu/mount/map/save streams serialize to one service operation; page-aligned 16-KiB transfers, short I/O, EOF, failures and byte equality passed\n";
}
