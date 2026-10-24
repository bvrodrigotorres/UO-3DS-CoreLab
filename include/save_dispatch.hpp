// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace lab {
using SaveSink=std::function<bool(const std::string&,std::vector<uint8_t>)>;
}
