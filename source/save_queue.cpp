// SPDX-License-Identifier: MIT
#include "save_queue.hpp"
#include <algorithm>
#include <cstdio>
#include <cerrno>
#include <sys/stat.h>
#ifdef __3DS__
#include <3ds.h>
#else
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#endif
namespace lab {
namespace {
#ifdef __3DS__
struct Mutex{LightLock value;Mutex(){LightLock_Init(&value);}void lock(){LightLock_Lock(&value);}void unlock(){LightLock_Unlock(&value);}};
uint64_t milliseconds(){return osGetTime();}
void nap(unsigned ms){svcSleepThread(uint64_t(ms)*1000000);}
#else
using Mutex=std::mutex;
uint64_t milliseconds(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void nap(unsigned ms){std::this_thread::sleep_for(std::chrono::milliseconds(ms));}
#endif
struct Guard{Mutex&m;explicit Guard(Mutex&v):m(v){m.lock();}~Guard(){m.unlock();}};
}
bool atomicSaveBytes(const std::string&path,const std::vector<uint8_t>&b,std::string&error){
    std::string tmp=path+".tmp",bak=path+".bak";FILE*f=std::fopen(tmp.c_str(),"wb");if(!f){error="ERROR SD: GUARDADO PENDIENTE";return false;}
    bool ok=std::fwrite(b.data(),1,b.size(),f)==b.size();if(std::fclose(f))ok=false;if(!ok){std::remove(tmp.c_str());error="ERROR SD: ESCRITURA";return false;}
    struct stat info;bool had=stat(path.c_str(),&info)==0;
    if(had&&((std::remove(bak.c_str())&&errno!=ENOENT)||std::rename(path.c_str(),bak.c_str()))){std::remove(tmp.c_str());error="ERROR SD: RESPALDO";return false;}
    if(std::rename(tmp.c_str(),path.c_str())){if(had)std::rename(bak.c_str(),path.c_str());std::remove(tmp.c_str());error="ERROR SD: CONFIRMACION";return false;}
    return true;
}
struct AsyncSaves::Impl {
    struct Job{std::string path;std::vector<uint8_t>bytes;uint64_t revision=0;};
    mutable Mutex mutex;std::vector<Job>pending;Writer writer;bool running=false,stop=false,busy=false,error=false,force=false;unsigned writes=0,failures=0;uint64_t serial=0,retryAt=0;size_t activeBytes=0;float lastMs=0;
#ifdef __3DS__
    Thread thread=nullptr;LightEvent event;Impl(){LightEvent_Init(&event,RESET_ONESHOT);}
    void wake(){LightEvent_Signal(&event);}void wait(){LightEvent_WaitTimeout(&event,1000000000);}
#else
    std::thread thread;std::condition_variable event;std::mutex eventMutex;bool signaled=false;
    void wake(){{std::lock_guard<std::mutex>g(eventMutex);signaled=true;}event.notify_one();}
    void wait(){std::unique_lock<std::mutex>g(eventMutex);event.wait_for(g,std::chrono::seconds(1),[&]{return signaled;});signaled=false;}
#endif
    void loop(){
        for(;;){Job job;bool have=false;
            {Guard guard(mutex);if(stop)return;if(!pending.empty()&&(force||milliseconds()>=retryAt)){job=std::move(pending.front());pending.erase(pending.begin());busy=have=true;activeBytes=job.bytes.size();force=false;}}
            if(!have){wait();continue;}
            uint64_t before=milliseconds();std::string detail;bool ok=writer(job.path,job.bytes,detail);uint64_t after=milliseconds();
            {Guard guard(mutex);busy=false;activeBytes=0;lastMs=float(after-before);
                if(ok){++writes;retryAt=0;if(pending.empty())error=false;}
                else {++failures;error=true;retryAt=after+1000;auto next=std::find_if(pending.begin(),pending.end(),[&](const Job&v){return v.path==job.path;});if(next==pending.end())pending.push_back(std::move(job));}
            }
        }
    }
};
AsyncSaves::AsyncSaves(Writer writer):impl(new Impl){impl->writer=writer?std::move(writer):atomicSaveBytes;impl->pending.reserve(4);}
AsyncSaves::~AsyncSaves(){if(!impl->running)return;flush();{Guard guard(impl->mutex);impl->stop=true;}impl->wake();
#ifdef __3DS__
    threadJoin(impl->thread,U64_MAX);threadFree(impl->thread);
#else
    impl->thread.join();
#endif
}
bool AsyncSaves::start(){if(impl->running)return true;
#ifdef __3DS__
    s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    impl->thread=threadCreate([](void*p){static_cast<Impl*>(p)->loop();},impl.get(),64*1024,std::min(0x3f,int(priority)+2),0,false);impl->running=impl->thread!=nullptr;
#else
    impl->thread=std::thread([this]{impl->loop();});impl->running=true;
#endif
    return impl->running;
}
bool AsyncSaves::enqueue(const std::string&path,std::vector<uint8_t>b){
    {Guard guard(impl->mutex);if(!impl->running||impl->stop||b.size()>524288)return false;size_t bytes=b.size()+impl->activeBytes;auto found=impl->pending.end();for(auto it=impl->pending.begin();it!=impl->pending.end();++it){if(it->path==path)found=it;else bytes+=it->bytes.size();}
        if(bytes>1024*1024||(found==impl->pending.end()&&impl->pending.size()+unsigned(impl->busy)>=4))return false;
        Impl::Job job{path,std::move(b),++impl->serial};if(found==impl->pending.end())impl->pending.push_back(std::move(job));else *found=std::move(job);
    }impl->wake();return true;
}
bool AsyncSaves::flush(unsigned timeout){if(!impl->running)return true;unsigned failures;{Guard guard(impl->mutex);failures=impl->failures;impl->force=true;}impl->wake();uint64_t limit=milliseconds()+timeout;
    for(;;){{Guard guard(impl->mutex);if(!impl->busy&&impl->pending.empty())return true;if(impl->failures>failures)return false;}if(milliseconds()>=limit)return false;nap(2);}
}
SaveQueueStatus AsyncSaves::status()const{Guard guard(impl->mutex);return {unsigned(impl->pending.size())+unsigned(impl->busy),impl->writes,impl->failures,impl->lastMs,impl->error};}
}

#include <fstream>
#include <map>
namespace lab {
struct AsyncResources::Impl {
    struct Ready{std::string path;ResourceRead file;};
    Mutex mutex;std::map<std::string,unsigned>requested;std::vector<Ready>ready;
    AsyncSaves worker;
    Impl():worker([this](const std::string&path,const std::vector<uint8_t>&meta,std::string&){
        auto get=[&](unsigned i){return uint32_t(meta[i])|uint32_t(meta[i+1])<<8|uint32_t(meta[i+2])<<16|uint32_t(meta[i+3])<<24;};
        unsigned maximum=get(0);uint32_t expected=get(4);ResourceRead result;std::ifstream f(path,std::ios::binary|std::ios::ate);
        if(f&&f.tellg()>0&&f.tellg()<=std::streamoff(maximum)){result.bytes.resize(size_t(f.tellg()));f.seekg(0);f.read((char*)result.bytes.data(),result.bytes.size());result.valid=bool(f);if(result.valid&&expected){uint32_t h=2166136261u;for(auto b:result.bytes)h=(h^b)*16777619u;result.valid=h==expected;}}
        if(!result.valid)result.bytes.clear();
        Guard guard(mutex);size_t bytes=result.bytes.size();for(const auto&r:ready)bytes+=r.file.bytes.size();
        while(bytes>4*1024*1024&&!ready.empty()){bytes-=ready.front().file.bytes.size();auto path=ready.front().path;requested.erase(path);ready.erase(ready.begin());}
        ready.push_back({path,std::move(result)});return true;
    }){ready.reserve(4);}
};
AsyncResources::AsyncResources():impl(new Impl){}AsyncResources::~AsyncResources()=default;
bool AsyncResources::start(){return impl->worker.start();}
bool AsyncResources::request(const std::string&path,unsigned maximum,uint32_t checksum){
    {Guard guard(impl->mutex);if(impl->requested.find(path)!=impl->requested.end())return true;if(!maximum||maximum>4*1024*1024)return false;
        size_t reserved=maximum;for(const auto&r:impl->requested)reserved+=r.second;
        while((impl->requested.size()>=4||reserved>4*1024*1024)&&!impl->ready.empty()){const auto&stale=impl->ready.front().path;auto found=impl->requested.find(stale);if(found!=impl->requested.end()){reserved-=found->second;impl->requested.erase(found);}impl->ready.erase(impl->ready.begin());}
        if(impl->requested.size()>=4||reserved>4*1024*1024)return false;
        impl->requested.emplace(path,maximum);}
    std::vector<uint8_t>meta;for(unsigned n:{maximum,unsigned(checksum)})for(unsigned i=0;i<4;++i)meta.push_back(uint8_t(n>>(8*i)));
    if(impl->worker.enqueue(path,std::move(meta)))return true;
    Guard guard(impl->mutex);impl->requested.erase(path);return false;
}
bool AsyncResources::take(const std::string&path,ResourceRead&out){Guard guard(impl->mutex);auto found=std::find_if(impl->ready.begin(),impl->ready.end(),[&](const Impl::Ready&r){return r.path==path;});if(found==impl->ready.end())return false;out=std::move(found->file);impl->ready.erase(found);impl->requested.erase(path);return true;}
}
