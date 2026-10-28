// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
namespace lab {
constexpr size_t SdTransferBytes=16*1024;
struct PlatformIoStats {uint64_t calls=0,readBytes=0,writtenBytes=0,failures=0;size_t largestTransfer=0;};
PlatformIoStats platformIoStats();
}
