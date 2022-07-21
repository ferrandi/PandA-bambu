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
#include <stdlib.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "bridge.h"
#include "edge_map_blocked.h"
#include "edge_map_utils.h"
#include "flags.h"
#include "vertex_subset.h"

namespace gbbs {

template <class Data /* per-vertex data in the emitted vertex_subset */,
          class Graph /* graph type */, class VS /* vertex_subset type */,
          class F /* edgeMap struct */>
inline vertexSubsetData<Data> edgeMapDense(Graph& GA, VS& vertexSubset, F& f,
                                           const flags fl) {
  using D = typename vertexSubsetData<Data>::D;
  size_t n = GA.n;
  auto dense_par = fl & dense_parallel;
  if (should_output(fl)) {
    auto next = sequence<D>::from_function(n, [&](size_t i) {
      if
        constexpr(std::is_same<Data, gbbs::empty>()) return 0;
      else
        return std::make_tuple<uintE, Data>(0, Data());
    });
    auto g = get_emdense_gen<Data>(next.begin());
    parallel_for(0, n,
                 [&](size_t v) {
                   if (f.cond(v)) {
                     auto neighbors = (fl & in_edges)
                                          ? GA.get_vertex(v).out_neighbors()
                                          : GA.get_vertex(v).in_neighbors();
                     neighbors.decodeBreakEarly(vertexSubset, f, g, dense_par);
                   }
                 },
                 (fl & fine_parallel) ? 1 : 2048);
    return vertexSubsetData<Data>(n, std::move(next));
  } else {
    auto g = get_emdense_nooutput_gen<Data>();
    parallel_for(0, n,
                 [&](size_t v) {
                   if (f.cond(v)) {
                     auto neighbors = (fl & in_edges)
                                          ? GA.get_vertex(v).out_neighbors()
                                          : GA.get_vertex(v).in_neighbors();
                     neighbors.decodeBreakEarly(vertexSubset, f, g, dense_par);
                   }
                 },
                 (fl & fine_parallel) ? 1 : 2048);
    return vertexSubsetData<Data>(n);
  }
}

template <class Data /* per-vertex data in the emitted vertex_subset */,
          class Graph /* graph type */, class VS /* vertex_subset type */,
          class F /* edgeMap struct */>
inline vertexSubsetData<Data> edgeMapDenseForward(Graph& GA, VS& vertexSubset,
                                                  F& f, const flags fl) {
  debug(std::cout << "# dense forward" << std::endl;);
  using D = typename vertexSubsetData<Data>::D;
  size_t n = GA.n;
  if (should_output(fl)) {
    auto next = sequence<D>(n);
    auto g = get_emdense_forward_gen<Data>(next.begin());
    if
      constexpr(std::is_same<Data, gbbs::empty>()) {
        parallel_for(0, n, [&](size_t i) { next[i] = 0; }, kDefaultGranularity);
      }
    else {
      parallel_for(0, n, [&](size_t i) { std::get<0>(next[i]) = 0; },
                   kDefaultGranularity);
    }
    parallel_for(0, n,
                 [&](size_t i) {
                   if (vertexSubset.isIn(i)) {
                     auto neighbors = (fl & in_edges)
                                          ? GA.get_vertex(i).in_neighbors()
                                          : GA.get_vertex(i).out_neighbors();
                     neighbors.decode(f, g);
                   }
                 },
                 1);
    return vertexSubsetData<Data>(n, std::move(next));
  } else {
    auto g = get_emdense_forward_nooutput_gen<Data>();
    parallel_for(0, n,
                 [&](size_t i) {
                   if (vertexSubset.isIn(i)) {
                     auto neighbors = (fl & in_edges)
                                          ? GA.get_vertex(i).in_neighbors()
                                          : GA.get_vertex(i).out_neighbors();
                     neighbors.decode(f, g);
                   }
                 },
                 1);
    return vertexSubsetData<Data>(n);
  }
}

// Decides on sparse or dense base on number of nonzeros in the active vertices.
template <
    class Data /* data associated with vertices in the output vertex_subset */,
    class Graph /* graph type */, class VS /* vertex_subset type */,
    class F /* edgeMap struct */>
inline vertexSubsetData<Data> edgeMapData(Graph& GA, VS& vs, F f,
                                          intT threshold = -1,
                                          const flags& fl = 0) {
  size_t numVertices = GA.n, numEdges = GA.m, m = vs.numNonzeros();
  size_t dense_threshold = threshold;
  if (threshold == -1) dense_threshold = numEdges / 20;
  if (vs.size() == 0) return vertexSubsetData<Data>(numVertices);

  if ((fl & dense_only) || (vs.isDense && vs.size() > numVertices / 10)) {
    vs.toDense();
    timer dt;
    dt.start();
    auto ret = (fl & dense_forward)
                   ? edgeMapDenseForward<Data, Graph, VS, F>(GA, vs, f, fl)
                   : edgeMapDense<Data, Graph, VS, F>(GA, vs, f, fl);
    dt.stop();
    debug(dt.next("dense time"););
    return ret;
  }

  timer st;
  st.start();
  size_t out_degrees = 0;
  if (vs.out_degrees_set()) {
    out_degrees = vs.get_out_degrees();
  } else {
    vs.toSparse();
    auto degree_f = [&](size_t i) {
      return (fl & in_edges) ? GA.get_vertex(vs.vtx(i)).in_degree()
                             : GA.get_vertex(vs.vtx(i)).out_degree();
    };
    auto degree_im = parlay::delayed_seq<size_t>(vs.size(), degree_f);
    out_degrees = parlay::reduce(degree_im);
    vs.set_out_degrees(out_degrees);
  }

  if (out_degrees == 0) return vertexSubsetData<Data>(numVertices);
  if (m + out_degrees > dense_threshold && !(fl & no_dense)) {
    vs.toDense();
    auto ret = (fl & dense_forward)
                   ? edgeMapDenseForward<Data, Graph, VS, F>(GA, vs, f, fl)
                   : edgeMapDense<Data, Graph, VS, F>(GA, vs, f, fl);
    st.stop();
    debug(st.next("dense convert time"););
    return ret;
  } else {
    auto vs_out = edgeMapChunked<Data, Graph, VS, F>(GA, vs, f, fl);
    st.stop();
    debug(st.next("sparse time"););
    //    auto vs_out = edgeMapBlocked<Data, Graph, VS, F>(GA, vs, f, fl);
    //    auto vs_out = edgeMapSparse<Data, Graph, VS, F>(GA, vs, f, fl);
    return vs_out;
  }
}

// Regular edgeMap, where no extra data is stored per vertex.
template <class Graph /* graph type */, class VS /* vertex_subset type */,
          class F /* edgeMap struct */>
inline vertexSubset edgeMap(Graph& GA, VS& vs, F f, intT threshold = -1,
                            const flags& fl = 0) {
  return edgeMapData<gbbs::empty>(GA, vs, f, threshold, fl);
}

// Adds vertices to a vertexSubset vs.
// Caller must ensure that every v in new_verts is not already in vs
// Note: Mutates the given vertexSubset.
void add_to_vsubset(vertexSubset& vs, uintE* new_verts, uintE num_new_verts);

// cond function that always returns true
inline bool cond_true(intT d) { return 1; }

// Sugar to pass in a single f and get a struct suitable for edgeMap.
template <class W, class F>
struct EdgeMap_F {
  F f;
  EdgeMap_F(F _f) : f(_f) {}
  inline bool update(const uintE& s, const uintE& d, const W& wgh) {
    return f(s, d, wgh);
  }

  inline bool updateAtomic(const uintE& s, const uintE& d, const W& wgh) {
    return f(s, d, wgh);
  }

  inline bool cond(const uintE& d) const { return true; }
};

}  // namespace gbbs
