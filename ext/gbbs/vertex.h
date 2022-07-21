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
#include "intersect.h"
#include "macros.h"

namespace gbbs {
namespace vertex_ops {

// allocate temporary space for vertices with degree > kAllocThreshold
const constexpr size_t kAllocThreshold = 10000;
static constexpr uintE kBlockSize = 1024;

template <class W>
struct iter {
  std::tuple<uintE, W>* edges;
  uintE degree;
  uintE proc;
  std::tuple<uintE, W> last_edge;
  iter(std::tuple<uintE, W>* _e, uintE _d) : edges(_e), degree(_d), proc(0) {
    if (degree > 0) {
      last_edge = *edges;
      edges++;
      proc++;
    }
  }

  inline std::tuple<uintE, W> cur() { return last_edge; }

  inline std::tuple<uintE, W> next() {
    auto edge = *edges;
    edges++;
    proc++;
    last_edge = edge;
    return edge;
  }

  inline bool has_next() { return proc < degree; }
};

template <class W>
inline iter<W> get_iter(std::tuple<uintE, W>* edges, uintE degree) {
  return iter<W>(edges, degree);
}
}  // namespace vertex_ops

template <class W>
struct uncompressed_neighbors {
  using edge_type = std::tuple<uintE, W>;

  uintE id;              // this vertex's id
  uintE degree;          // this vertex's (in/out) degree
  edge_type* neighbors;  // the (in/out) neighbors

  uncompressed_neighbors(uintE id, uintE degree, edge_type* neighbors)
      : id(id), degree(degree), neighbors(neighbors) {}

  // move constructor
  uncompressed_neighbors(uncompressed_neighbors&& a)
      : id(a.id), degree(a.degree), neighbors(a.neighbors) {}

  uintE get_neighbor(uintE i) { return std::get<0>(neighbors[i]); }
  W get_weight(uintE i) { return std::get<1>(neighbors[i]); }
  std::tuple<uintE, W> get_ith_neighbor(uintE i) { return neighbors[i]; }

  uintE get_degree() { return degree; }
  uintE get_virtual_degree() { return degree; }

  uintE get_num_blocks() {
    return parlay::num_blocks(degree, vertex_ops::kBlockSize);
  }

  uintE block_degree(uintE block_num) {
    uintE block_start = block_num * vertex_ops::kBlockSize;
    uintE block_end = std::min(block_start + vertex_ops::kBlockSize, degree);
    return block_end - block_start;
  }

  vertex_ops::iter<W> get_iter() {
    return vertex_ops::get_iter(neighbors, degree);
  }

  template <class F>
  size_t intersect(uncompressed_neighbors<W>* other, const F& f) {
    return intersection::intersect(this, other);
  }

  template <class F>
  size_t intersect_f(uncompressed_neighbors<W>* other, const F& f) {
    return intersection::intersect_f(this, other, f);
  }

  template <class F>
  size_t intersect_f_par(uncompressed_neighbors<W>* other, const F& f) {
    return intersection::intersect_f_par(this, other, f);
  }

  static constexpr size_t kAllocThreshold = 10000;
  static constexpr uintE kBlockSize = 1024;

  template <class F>
  size_t count(F f, bool parallel = true) {
    if (degree == 0) return 0;
    auto im_f = [&](size_t i) -> size_t {
      auto nw = neighbors[i];
      return f(id, std::get<0>(nw), std::get<1>(nw));
    };
    auto im = parlay::delayed_seq<size_t>(degree, im_f);
    return parlay::reduce(im);
  }

  template <class M, class Monoid>
  typename Monoid::T reduce(M m, Monoid reduce) {
    using T = typename Monoid::T;
    if (degree == 0) return reduce.identity;
    auto im_f = [&](size_t i) {
      auto nw = neighbors[i];
      return m(id, std::get<0>(nw), std::get<1>(nw));
    };
    auto im = parlay::delayed_seq<T>(degree, im_f);
    return parlay::reduce(im, reduce);
  }

  template <class F>
  inline void map(F f, bool parallel = true) {
    size_t granularity =
        parallel ? kDefaultGranularity : std::numeric_limits<size_t>::max();
    parallel_for(0, degree,
                 [&](size_t j) {
                   const std::tuple<uintE, W>& neighbor = neighbors[j];
                   f(id, std::get<0>(neighbor), std::get<1>(neighbor));
                 },
                 granularity);
  }

  // Applies `f` to neighbors of vertex `v`. The difference between this and
  // `map` is that `f` takes an additional argument: the index of the neighbor
  // in `v`'s neighbor list.
  //
  // Arguments:
  //   v
  //     Vertex whose neighbors we want to map over.
  //   v_id
  //     ID of `v`.
  //   nghs
  //     Neighbors of `v`, either in-neighbors or out-neighbors.
  //   d
  //     Length of `nghs`, i.e., the degree of `v`.
  //   f: (uintE, uintE, W, uintE) -> void
  //     Function to apply to each neighbor. The function will be called as
  //     `f(v_id, neighbor_vertex_id, weight, neighbor_index)` where
  //     `nghs[neighbor_index] == (neighbor_vertex_id, weight)`.
  //   parallel
  //     Whether to run this function with parallelism.
  template <class F>
  inline void map_with_index(F f, bool parallel = true) {
    size_t granularity =
        parallel ? kDefaultGranularity : std::numeric_limits<size_t>::max();
    parallel_for(0, degree,
                 [&](size_t j) {
                   const std::tuple<uintE, W>& neighbor = neighbors[j];
                   f(id, std::get<0>(neighbor), std::get<1>(neighbor), j);
                 },
                 granularity);
  }

  // Expects that out has enough space to hold the output of the filter
  template <class P, class O>
  inline void filter(P p, O& out, std::tuple<uintE, W>* tmp) {
    if (degree > 0) {
      if (degree < vertex_ops::kAllocThreshold) {
        size_t k = 0;
        for (size_t i = 0; i < degree; i++) {
          auto nw = neighbors[i];
          if (p(id, std::get<0>(nw), std::get<1>(nw))) {
            out(k++, nw);
          }
        }
      } else {
        auto pc = [&](const std::tuple<uintE, W>& nw) {
          return p(id, std::get<0>(nw), std::get<1>(nw));
        };
        auto in_im = gbbs::make_slice(neighbors, degree);
        size_t k = parlay::filter_out(in_im, gbbs::make_slice(tmp, degree), pc);
        parallel_for(0, k, [&](size_t i) { out(i, tmp[i]); });
      }
    }
  }

  // Caller is responsible for setting the degree on the vertex object enclosing
  // this uncompressed_neighbors
  template <class P>
  inline size_t pack(P& p, std::tuple<uintE, W>* tmp) {
    if (degree < vertex_ops::kAllocThreshold) {
      uintE k = 0;
      for (size_t i = 0; i < degree; i++) {
        auto nw = neighbors[i];
        uintE ngh = std::get<0>(nw);
        W wgh = std::get<1>(nw);
        if (p(id, ngh, wgh)) {
          neighbors[k++] = std::make_tuple(ngh, wgh);
        }
      }
      degree = k;
      return k;
    } else {
      // copy to tmp
      parallel_for(0, degree, [&](size_t i) { tmp[i] = neighbors[i]; });
      auto pc = [&](const std::tuple<uintE, W>& nw) {
        return p(id, std::get<0>(nw), std::get<1>(nw));
      };
      size_t k = parlay::filterf(tmp, neighbors, degree, pc);
      degree = k;
      return k;
    }
  }

  template <class F, class G>
  inline void copy(uintT offset, F f, G g) {
    parallel_for(0, degree, [&](size_t j) {
      auto nw = neighbors[j];
      uintE ngh = std::get<0>(nw);
      auto val = f(id, ngh, std::get<1>(nw));
      g(ngh, offset + j, val);
    });
  }

  inline size_t calculateTemporarySpace() {
    return (degree < vertex_ops::kAllocThreshold) ? 0 : degree;
  }

  inline size_t calculateTemporarySpaceBytes() {
    return calculateTemporarySpace() * sizeof(std::tuple<uintE, W>);
  }

  // ======== Internal primitives used by EdgeMap implementations =======

  template <class VS, class F, class G>
  void decodeBreakEarly(VS& vs, F& f, const G& g, bool parallel = 0) {
    if (!parallel || degree < 1000) {
      for (size_t j = 0; j < degree; j++) {
        auto nw = neighbors[j];
        uintE ngh = std::get<0>(nw);
        if (vs.isIn(ngh)) {
          auto m = f.update(ngh, id, std::get<1>(nw));
          g(id, m);
        }
        if (!f.cond(id)) break;
      }
    } else {
      size_t b_size = 2048;
      size_t n_blocks = degree / b_size + 1;
      parallel_for(0, n_blocks,
                   [&](size_t b) {
                     if (f.cond(id)) {
                       size_t start = b * b_size;
                       size_t end = std::min((b + 1) * b_size,
                                             static_cast<size_t>(degree));
                       for (size_t j = start; j < end; j++) {
                         if (!f.cond(id)) break;
                         auto nw = neighbors[j];
                         uintE ngh = std::get<0>(nw);
                         if (vs.isIn(ngh)) {
                           auto m = f.updateAtomic(ngh, id, std::get<1>(nw));
                           g(id, m);
                         }
                       }
                     }
                   },
                   1);
    }
  }

  // Used by edgeMapDenseForward. For each out-neighbor satisfying cond, call
  // updateAtomic.
  template <class F, class G>
  void decode(F& f, G& g) {
    parallel_for(0, degree, [&](size_t j) {
      auto nw = neighbors[j];
      uintE ngh = std::get<0>(nw);
      if (f.cond(ngh)) {
        auto m = f.updateAtomic(id, ngh, std::get<1>(nw));
        g(ngh, m);
      }
    });
  }

  // Used by edgeMapSparse. For each out-neighbor satisfying cond, call
  // updateAtomic.
  template <class F, class G, class H>
  void decodeSparse(uintT offset, F& f, const G& g, const H& h,
                    bool parallel = true) {
    size_t granularity =
        parallel ? kDefaultGranularity : std::numeric_limits<size_t>::max();
    parallel_for(0, degree,
                 [&](size_t j) {
                   auto nw = neighbors[j];
                   uintE ngh = std::get<0>(nw);
                   if (f.cond(ngh)) {
                     auto m = f.updateAtomic(id, ngh, std::get<1>(nw));
                     g(ngh, offset + j, m);
                   } else {
                     h(ngh, offset + j);
                   }
                 },
                 granularity);
  }

  // Used by edgeMapSparse_no_filter. Sequentially decode the out-neighbors,
  // and compactly write all neighbors satisfying g().
  template <class F, class G>
  size_t decodeSparseSeq(uintT offset, F& f, const G& g) {
    size_t k = 0;
    for (size_t j = 0; j < degree; j++) {
      auto nw = neighbors[j];
      uintE ngh = std::get<0>(nw);
      if (f.cond(ngh)) {
        auto m = f.updateAtomic(id, ngh, std::get<1>(nw));
        if (g(ngh, offset + k, m)) {  // performed a write
          k++;
        }
      }
    }
    return k;
  }

  // TODO TODO check if this is used anywhere?
  // Used by edgeMapBlocked. Sequentially decode neighbors between
  // [block_num*KBlockSize, block_num*kBlockSize + block_size)
  // and compactly write all neighbors satisfying g().
  template <class F, class G>
  size_t decodeSparseBlock(uintT offset, uintE block_size, uintE block_num,
                           F& f, const G& g) {
    size_t k = 0;
    size_t start = kEMBlockSize * block_num;
    size_t end = start + block_size;
    for (size_t j = start; j < end; j++) {
      auto nw = neighbors[j];
      uintE ngh = std::get<0>(nw);
      if (f.cond(ngh)) {
        auto m = f.updateAtomic(id, ngh, std::get<1>(nw));
        bool wrote = g(ngh, offset + k, m);
        if (wrote) {
          k++;
        }
      }
    }
    return k;
  }

  // Used in edge_map_blocked.h
  template <class F, class G>
  size_t decode_block(uintT offset, uintE block_num, F& f, const G& g) {
    size_t k = 0;
    uintE start = vertex_ops::kBlockSize * block_num;
    uintE end = std::min(start + vertex_ops::kBlockSize, degree);
    for (uintE j = start; j < end; j++) {
      auto nw = neighbors[j];
      uintE ngh = std::get<0>(nw);
      if (f.cond(ngh)) {
        auto m = f.updateAtomic(id, ngh, std::get<1>(nw));
        if (g(ngh, offset + k, m)) {  // wrote
          k++;
        }
      }
    }
    return k;
  }

};  // struct uncompressed_neighbors

template <class W>
struct symmetric_vertex {
  using vertex = symmetric_vertex<W>;
  using edge_type = std::tuple<uintE, W>;

  uintE id;
  uintE degree;
  edge_type* neighbors;

  symmetric_vertex()
      : id(std::numeric_limits<uintE>::max()), degree(0), neighbors(nullptr) {}

  symmetric_vertex(edge_type* n, vertex_data vdata, uintE _id) {
    neighbors = (n + vdata.offset);
    degree = vdata.degree;
    id = _id;
  }

  /* Unlikely to be necessary since neighbors are likely flat
   * allocated in a shared array */
  void clear() {
    exit(-1);
    // gbbs::free_array(neighbors);
  }

  uncompressed_neighbors<W> in_neighbors() {
    return uncompressed_neighbors<W>(id, degree, neighbors);
  }
  uncompressed_neighbors<W> out_neighbors() { return in_neighbors(); }

  uintE in_degree() { return degree; }
  uintE out_degree() { return degree; }

  constexpr static uintE getInternalBlockSize() {
    return vertex_ops::kBlockSize;
  }
  inline uintE in_block_degree(uintE block_num) {
    uintE block_start = block_num * vertex_ops::kBlockSize;
    uintE block_end = std::min(block_start + vertex_ops::kBlockSize, degree);
    return block_end - block_start;  // TODO: check
  }
  inline uintE out_block_degree(uintE block_num) {
    return in_block_degree(block_num);
  }
};

template <class W>
struct asymmetric_vertex {
  using vertex = asymmetric_vertex<W>;
  using edge_type = std::tuple<uintE, W>;

  edge_type* in_nghs;
  edge_type* out_nghs;

  uintE in_deg;
  uintE out_deg;

  uintE id;

  asymmetric_vertex()
      : in_nghs(nullptr),
        out_nghs(nullptr),
        in_deg(0),
        out_deg(0),
        id(std::numeric_limits<uintE>::max()) {}

  asymmetric_vertex(edge_type* out_neighbors_, vertex_data out_data,
                    edge_type* in_neighbors_, vertex_data in_data, uintE _id) {
    in_nghs = in_neighbors_ + in_data.offset;
    out_nghs = out_neighbors_ + out_data.offset;

    in_deg = in_data.degree;
    out_deg = out_data.degree;

    id = _id;
  }

  /* Unlikely to be necessary since neighbors are likely flat
   * allocated in a shared array */
  void clear() {
    std::cout << "unexpected call to clear(); deprecated" << std::endl;
    exit(-1);
  }

  uncompressed_neighbors<W> in_neighbors() {
    return uncompressed_neighbors<W>(id, in_deg, in_nghs);
  }
  uncompressed_neighbors<W> out_neighbors() {
    return uncompressed_neighbors<W>(id, out_deg, out_nghs);
  }

  uintE in_degree() { return in_deg; }
  uintE out_degree() { return out_deg; }

  constexpr static uintE getInternalBlockSize() {
    return vertex_ops::kBlockSize;
  }
};

}  // namespace gbbs
