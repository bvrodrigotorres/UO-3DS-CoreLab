// SPDX-License-Identifier: MIT
// Serialize the newlib/devoptab boundary, including calls made by linked image
// libraries. The CPU/GPU workers remain independent; only each SD operation is
// protected. Never hold this lock while doing scene decoding or GPU work.
#include "platform_io.hpp"
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#ifdef __3DS__
#include <3ds.h>
#include <reent.h>
#else
#include <mutex>
#include <sys/types.h>
struct _reent {int _errno=0;};
using _ssize_t=ssize_t;
using _off_t=off_t;
#endif
namespace {
#ifdef __3DS__
LightLock diskLock=0;
void lock(){LightLock_Lock(&diskLock);}void unlock(){LightLock_Unlock(&diskLock);}
#else
std::mutex diskLock;
void lock(){diskLock.lock();}void unlock(){diskLock.unlock();}
#endif
// Device callbacks can query metadata recursively in the same thread.
thread_local unsigned depth=0;
struct Guard {Guard(){if(depth++==0)lock();}~Guard(){if(--depth==0)unlock();}};
// FS only sees page-aligned application memory, never a transient UI stack or
// an unaligned allocation crossing into unrelated heap objects.
alignas(4096) unsigned char staging[lab::SdTransferBytes];
lab::PlatformIoStats stats;
void failure(long long result){if(result<0)++stats.failures;}
}
extern "C" {
int __real__open_r(_reent*,const char*,int,int);
int __real__close_r(_reent*,int);
_ssize_t __real__read_r(_reent*,int,void*,size_t);
_ssize_t __real__write_r(_reent*,int,const void*,size_t);
_off_t __real__lseek_r(_reent*,int,_off_t,int);
int __real__fstat_r(_reent*,int,struct stat*);
int __real__stat_r(_reent*,const char*,struct stat*);
int __real__unlink_r(_reent*,const char*);
int __real__rename_r(_reent*,const char*,const char*);
int __wrap__open_r(_reent*r,const char*p,int f,int m){Guard guard;++stats.calls;int result=__real__open_r(r,p,f,m);failure(result);return result;}
int __wrap__close_r(_reent*r,int fd){Guard guard;++stats.calls;int result=__real__close_r(r,fd);failure(result);return result;}
_off_t __wrap__lseek_r(_reent*r,int fd,_off_t offset,int whence){Guard guard;++stats.calls;auto result=__real__lseek_r(r,fd,offset,whence);failure(result);return result;}
int __wrap__fstat_r(_reent*r,int fd,struct stat*s){Guard guard;++stats.calls;int result=__real__fstat_r(r,fd,s);failure(result);return result;}
int __wrap__stat_r(_reent*r,const char*p,struct stat*s){Guard guard;++stats.calls;int result=__real__stat_r(r,p,s);failure(result);return result;}
int __wrap__unlink_r(_reent*r,const char*p){Guard guard;++stats.calls;int result=__real__unlink_r(r,p);failure(result);return result;}
int __wrap__rename_r(_reent*r,const char*a,const char*b){Guard guard;++stats.calls;int result=__real__rename_r(r,a,b);failure(result);return result;}
_ssize_t __wrap__read_r(_reent*r,int fd,void*out,size_t bytes){
    if(!bytes)return 0;
    if(!out){r->_errno=EFAULT;return -1;}
    size_t done=0;
    while(done<bytes){_ssize_t amount;
        {Guard guard;size_t count=std::min(bytes-done,lab::SdTransferBytes);++stats.calls;stats.largestTransfer=std::max(stats.largestTransfer,count);
            amount=__real__read_r(r,fd,staging,count);
            if(amount<0){failure(amount);return done?_ssize_t(done):amount;}
            if(size_t(amount)>count){++stats.failures;r->_errno=EIO;return done?_ssize_t(done):-1;}
            std::memcpy(static_cast<unsigned char*>(out)+done,staging,size_t(amount));stats.readBytes+=size_t(amount);
        }
        if(!amount)break;
        done+=size_t(amount);
    }
    return _ssize_t(done);
}
_ssize_t __wrap__write_r(_reent*r,int fd,const void*in,size_t bytes){
    if(!bytes)return 0;
    if(!in){r->_errno=EFAULT;return -1;}
    size_t done=0;
    while(done<bytes){_ssize_t amount;
        {Guard guard;size_t count=std::min(bytes-done,lab::SdTransferBytes);std::memcpy(staging,static_cast<const unsigned char*>(in)+done,count);++stats.calls;stats.largestTransfer=std::max(stats.largestTransfer,count);
            amount=__real__write_r(r,fd,staging,count);
            if(amount<0){failure(amount);return done?_ssize_t(done):amount;}
            if(size_t(amount)>count){++stats.failures;r->_errno=EIO;return done?_ssize_t(done):-1;}
            stats.writtenBytes+=size_t(amount);
        }
        if(!amount)break;
        done+=size_t(amount);
    }
    return _ssize_t(done);
}
}
namespace lab {PlatformIoStats platformIoStats(){Guard guard;return stats;}}
