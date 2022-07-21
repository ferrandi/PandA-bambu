// This code is part of the project "Theoretically Efficient Parallel Graph
// Algorithms Can Be Fast and Scalable", presented at Symposium on Parallelism
// in Algorithms and Architectures, 2018.
// Copyright (c) 2018 Laxman Dhulipala, Guy Blelloch, and Julian Shun
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all  copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <limits.h>
#include "bridge.h"

namespace gbbs {
#define LONG 1

#ifndef NDEBUG
#define debug(_body) _body;
#else
#define debug(_body)
#endif

typedef unsigned int uint;
typedef unsigned long ulong;

// size of edge-offsets.
// If the number of edges is more than sizeof(MAX_UINT),
// you should set the LONG flag on the command line.
#if defined(LONG)
typedef long intT;
typedef unsigned long uintT;
#define INT_T_MAX LONG_MAX
#define UINT_T_MAX ULONG_MAX
#else
typedef int intT;
typedef unsigned int uintT;
#define INT_T_MAX INT_MAX
#define UINT_T_MAX UINT_MAX
#endif

// edge size macros.
// If the number of vertices is more than sizeof(MAX_UINT)
// you should set the EDGELONG flag on the command line.
#if defined(EDGELONG)
typedef long intE;
typedef unsigned long uintE;
#define INT_E_MAX LONG_MAX
#define UINT_E_MAX ULONG_MAX
#else
typedef int intE;
typedef unsigned int uintE;
#define INT_E_MAX INT_MAX
#define UINT_E_MAX UINT_MAX
#endif

struct vertex_data {
  size_t offset;  // offset into the edges (static)
  uintE degree;   // possibly decreased by a (mutable) algorithm.
};

// Default granularity of a parallel for loop.
constexpr const size_t kDefaultGranularity = 2048;

// edgemap_sparse_blocked granularity macro
constexpr const size_t kEMBlockSize = 4000;

// ======= compression macros and constants =======
constexpr const size_t PARALLEL_DEGREE = 1000;
// Take care in pushing this threshold too high; vertices with degree <
// pack_threshold stack allocate these bytes.
constexpr const size_t PD_PACK_THRESHOLD = 10000;

// Each vertex larger than PD_PACK_THRESHOLD is allocated deg(v) /
// kTempSpaceConstant
constexpr const size_t kTemporarySpaceConstant = 10;
typedef unsigned char uchar;

#define LAST_BIT_SET(b) (b & (0x80))
#define EDGE_SIZE_PER_BYTE 7

#if !defined(PD) && !defined(AMORTIZEDPD)
#define compression byte
#else
#ifdef AMORTIZEDPD
#define compression bytepd_amortized
#else
#define compression bytepd
#endif
#endif
}  // namespace gbbs
