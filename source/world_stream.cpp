// SPDX-License-Identifier: MIT
#include "world_stream.hpp"
#include "save_queue.hpp"
#ifdef __3DS__
#include <3ds.h>
#else
#include <mutex>
#endif
namespace lab {
namespace {
#ifdef __3DS__
struct Mutex{LightLock lock;Mutex(){LightLock_Init(&lock);}void acquire(){LightLock_Lock(&lock);}void release(){LightLock_Unlock(&lock);}};
#else
struct Mutex{std::mutex lock;void acquire(){lock.lock();}void release(){lock.unlock();}};
#endif
struct Guard{Mutex&m;explicit Guard(Mutex&v):m(v){m.acquire();}~Guard(){m.release();}};
}
struct WorldStream::Impl {
    mutable Mutex mutex;Builder build;uint32_t revision=0;int x=-1,y=-1;bool working=false,started=false;
    std::unique_ptr<StreamFrame>ready;std::string error;
    AsyncSaves worker;
    explicit Impl(Builder b):build(std::move(b)),worker([this](const std::string&,const std::vector<uint8_t>&bytes,std::string&){
        auto get=[&](unsigned i){uint32_t n=0;for(unsigned j=0;j<4;++j)n|=uint32_t(bytes[i+j])<<(j*8);return n;};
        uint32_t serial=get(0);int ox=int(get(4)),oy=int(get(8));
        {Guard g(mutex);if(serial!=revision)return true;working=true;}
        std::string failure;auto next=build(ox,oy,failure);
        {Guard g(mutex);working=false;if(serial==revision){ready=std::move(next);error=ready?std::string{}:failure;}}
        return true;
    }){}
};
WorldStream::WorldStream(Builder b):impl(new Impl(std::move(b))){}WorldStream::~WorldStream()=default;
bool WorldStream::start(){impl->started=impl->worker.start();return impl->started;}
bool WorldStream::request(int x,int y){
    if(!impl->started)return false;
    std::vector<uint8_t>bytes;
    {Guard g(impl->mutex);if(impl->x==x&&impl->y==y)return true;
        impl->x=x;impl->y=y;impl->ready.reset();impl->error.clear();++impl->revision;
        for(uint32_t n:{impl->revision,uint32_t(x),uint32_t(y)})for(unsigned i=0;i<4;++i)bytes.push_back(uint8_t(n>>(8*i)));
    }
    return impl->worker.enqueue("world-prefetch",std::move(bytes));
}
WorldStream::Status WorldStream::status()const{Guard g(impl->mutex);return {impl->x,impl->y,impl->ready!=nullptr,impl->working,impl->error};}
std::unique_ptr<StreamFrame> WorldStream::take(){Guard g(impl->mutex);return std::move(impl->ready);}
void WorldStream::cancel(){Guard g(impl->mutex);++impl->revision;impl->x=impl->y=-1;impl->ready.reset();impl->error.clear();}
void WorldStream::cancelAndWait(){
    cancel();
    // Superseded pending jobs are no-ops. Running reads must finish before the
    // caller accesses the archive synchronously. Destruction also joins them.
    while(!impl->worker.flush(1000)){}
}
std::unique_ptr<StreamFrame>prepareStreamFrame(WorldArchive&archive,const WorldGraphics&graphics,int x,int y,unsigned limit,std::string&error){
    auto frame=std::make_unique<StreamFrame>();
    if(!archive.window(x,y,StreamPlan::Side,frame->world,error,limit)||!graphics.bind(frame->world,error))return nullptr;
    if(!archive.radar(std::max(0,std::min(archive.width()-96,x-16)),std::max(0,std::min(archive.height()-96,y-16)),std::min(96,std::min(archive.width(),archive.height())),graphics.radarColors(),frame->radar,error))return nullptr;
    frame->roofs.build(frame->world);return frame;
}
}
