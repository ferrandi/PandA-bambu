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

#include "bridge.h"
#include "macros.h"

namespace gbbs {

// Edge Array Representation
template <class W>
struct edge_array {
  using weight_type = W;
  using edge = std::tuple<uintE, uintE, W>;

  // A sequence of edge tuples.
  sequence<edge> E;

  size_t n;  // num vertices.

  edge_array(sequence<edge>&& _E, size_t _n) : E(_E), n(_n) {}

  edge_array() {}

  size_t size() { return E.size(); }

  // Clears the edge array.
  sequence<edge>&& to_seq() {
    n = 0;
    return std::move(E);
  }

  template <class F>
  void map_edges(F f, bool parallel_inner_map = true) {
    size_t m = size();
    parallel_for(0, m, [&](size_t i) {
      uintE u, v;
      W w;
      std::tie(u, v, w) = E[i];
      f(u, v, w);
    });
  }
};

template <class W, class Graph>
inline edge_array<W> to_edge_array(Graph& G) {
  using edge = std::tuple<uintE, uintE, W>;

  size_t n = G.n;
  auto sizes = sequence<uintT>::uninitialized(n);
  parallel_for(0, n,
               [&](size_t i) { sizes[i] = G.get_vertex(i).out_degree(); });
  size_t m = parlay::scan_inplace(make_slice(sizes));
  assert(m == G.m);

  auto arr = sequence<edge>::uninitialized(m);
  parallel_for(0, n, [&](size_t i) {
    size_t idx = 0;
    uintT offset = sizes[i];
    auto map_f = [&](const uintE& u, const uintE& v, const W& wgh) {
      arr[offset + idx] = std::make_tuple(u, v, wgh);
      idx++;
    };
    G.get_vertex(i).out_neighbors().map(map_f, /* parallel = */ false);
  });
  return edge_array<W>(std::move(arr), n);
}

}  // namespace gbbs
