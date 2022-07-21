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

#include "macros.h"

namespace gbbs {
namespace intersection {

template <class Nghs>
inline size_t intersect(Nghs* A, Nghs* B, uintE a, uintE b) {
  uintT i = 0, j = 0, nA = A->degree, nB = B->degree;
  auto nghA = A->neighbors;
  auto nghB = B->neighbors;
  size_t ans = 0;
  while (i < nA && j < nB) {
    if (std::get<0>(nghA[i]) == std::get<0>(nghB[j]))
      i++, j++, ans++;
    else if (std::get<0>(nghA[i]) < std::get<0>(nghB[j]))
      i++;
    else
      j++;
  }
  return ans;
}

template <class Nghs, class F>
inline size_t intersect_f(Nghs* A, Nghs* B, const F& f) {
  uintT i = 0, j = 0, nA = A->degree, nB = B->degree;
  auto nghA = A->neighbors;
  auto nghB = B->neighbors;
  uintE a = A->id, b = B->id;
  size_t ans = 0;
  while (i < nA && j < nB) {
    if (std::get<0>(nghA[i]) == std::get<0>(nghB[j])) {
      f(a, b, std::get<0>(nghA[i]));
      i++, j++, ans++;
    } else if (std::get<0>(nghA[i]) < std::get<0>(nghB[j])) {
      i++;
    } else {
      j++;
    }
  }
  return ans;
}

constexpr const size_t _bs_merge_base = 32;
constexpr const size_t _seq_merge_thresh = 2048;

template <class SeqA, class SeqB, class F>
size_t seq_merge_full(SeqA& A, SeqB& B, F& f) {
  using T = typename SeqA::value_type;
  size_t nA = A.size(), nB = B.size();
  size_t i = 0, j = 0;
  size_t ct = 0;
  while (i < nA && j < nB) {
    const T& a = A[i];
    const T& b = B[j];
    if (a == b) {
      f(a);
      i++;
      j++;
      ct++;
    } else if (a < b) {
      i++;
    } else {
      j++;
    }
  }
  return ct;
}

template <class SeqA, class SeqB, class F>
size_t seq_merge(const SeqA& A, const SeqB& B, const F& f) {
  using T = typename SeqA::value_type;
  size_t nA = A.size();
  size_t ct = 0;
  for (size_t i = 0; i < nA; i++) {
    const T& a = A[i];
    size_t mB = parlay::binary_search(B, a, std::less<T>());
    if (mB < B.size() && a == B[mB]) {
      f(a);
      ct++;
    }
  }
  return ct;
}

template <class SeqA, class SeqB, class F>
size_t merge(const SeqA& A, const SeqB& B, const F& f) {
  using T = typename SeqA::value_type;
  size_t nA = A.size();
  size_t nB = B.size();
  size_t nR = nA + nB;
  if (nR < _seq_merge_thresh) {  // handles (small, small) using linear-merge
    return intersection::seq_merge_full(A, B, f);
  } else if (nB < nA) {
    return intersection::merge(B, A, f);
  } else if (nA < _bs_merge_base) {
    return intersection::seq_merge(A, B, f);
  } else {
    size_t mA = nA / 2;
    size_t mB = parlay::binary_search(B, A[mA], std::less<T>());
    size_t m_left = 0;
    size_t m_right = 0;
    par_do(
        [&]() { m_left = intersection::merge(A.cut(0, mA), B.cut(0, mB), f); },
        [&]() {
          m_right = intersection::merge(A.cut(mA, nA), B.cut(mB, nB), f);
        });
    return m_left + m_right;
  }
}

template <class Nghs, class F>
inline size_t intersect_f_par(Nghs* A, Nghs* B, const F& f) {
  uintT nA = A->degree, nB = B->degree;
  uintE* nghA = (uintE*)(A->neighbors);
  uintE* nghB = (uintE*)(B->neighbors);

  // Will not work if W is not gbbs::empty, should assert.
  auto seqA = gbbs::make_slice<uintE>(nghA, nA);
  auto seqB = gbbs::make_slice<uintE>(nghB, nB);

  uintE a = A->id;
  uintE b = B->id;
  auto merge_f = [&](uintE ngh) { f(a, b, ngh); };
  return intersection::merge(seqA, seqB, merge_f);
}

}  // namespace intersection
}  // namespace gbbs
