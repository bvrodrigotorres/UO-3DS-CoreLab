// SPDX-License-Identifier: MIT
#pragma once
#include "core.hpp"
bool referenceSolve(const std::vector<lab::Candidate>&,int z,int minZ,int maxZ,int&nextZ);
std::vector<int> referenceOrder(const std::vector<lab::Draw>&,const std::vector<int>&height,
                              const std::vector<uint64_t>&flags,const std::vector<int>&average,
                              const std::vector<bool>&slope);
