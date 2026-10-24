// SPDX-License-Identifier: MIT
#pragma once
#include "save_dispatch.hpp"
#include <memory>
namespace lab {
struct SaveQueueStatus {unsigned pending=0,writes=0,failures=0;float lastMilliseconds=0;bool error=false;};
class AsyncSaves {
    struct Impl;std::unique_ptr<Impl>impl;
public:
    using Writer=std::function<bool(const std::string&,const std::vector<uint8_t>&,std::string&)>;
    explicit AsyncSaves(Writer={});~AsyncSaves();
    AsyncSaves(const AsyncSaves&)=delete;AsyncSaves&operator=(const AsyncSaves&)=delete;
    bool start();
    bool enqueue(const std::string&,std::vector<uint8_t>);
    bool flush(unsigned timeoutMilliseconds=5000);
    SaveQueueStatus status()const;
};
bool atomicSaveBytes(const std::string&,const std::vector<uint8_t>&,std::string&);
}

namespace lab {
struct ResourceRead {std::vector<uint8_t>bytes;bool valid=false;};
class AsyncResources {
    struct Impl;std::unique_ptr<Impl>impl;
public:
    AsyncResources();~AsyncResources();
    bool start();
    // At most four outstanding reads and 4 MiB completed data, never GPU work.
    bool request(const std::string&,unsigned maximumBytes,uint32_t checksum=0);
    bool take(const std::string&,ResourceRead&);
};
}
