#pragma once

#include "bridge.h"
#include "edge_map_utils.h"
#include "flags.h"
#include "vertex_subset.h"

#include <vector>

namespace gbbs {

template <
    class data /* data associated with vertices in the output vertex_subset */,
    class Graph /* graph type */, class VS /* vertex_subset type */,
    class F /* edgeMap struct */>
inline vertexSubsetData<data> edgeMapSparse(Graph& G, VS& indices, F& f,
                                            const flags fl) {
  using S = typename vertexSubsetData<data>::S;
  size_t n = indices.n;
  size_t m = indices.size();

  if (should_output(fl)) {
    auto offsets = parlay::sequence<uintT>(indices.size(), [&](size_t i) {
      return (fl & in_edges) ? G.get_vertex(indices.vtx(i)).in_degree()
                             : G.get_vertex(indices.vtx(i)).out_degree();
    });
    size_t outEdgeCount = parlay::scan_inplace(make_slice(offsets));

    auto outEdges = parlay::sequence<S>(outEdgeCount);
    auto g = get_emsparse_gen_full<data>(outEdges.begin());
    auto h = get_emsparse_gen_empty<data>(outEdges.begin());
    parallel_for(0, m,
                 [&](size_t i) {
                   uintT v = indices.vtx(i);
                   uintT o = offsets[i];
                   auto neighbors = (fl & in_edges)
                                        ? G.get_vertex(v).in_neighbors()
                                        : G.get_vertex(v).out_neighbors();
                   neighbors.decodeSparse(o, f, g, h);
                 },
                 1);

    auto p = [](std::tuple<uintE, data>& v) {
      return std::get<0>(v) != UINT_E_MAX;
    };
    auto nextIndices = parlay::filter(outEdges, p);
    return vertexSubsetData<data>(n, std::move(nextIndices));
  }

  auto g = get_emsparse_nooutput_gen<data>();
  auto h = get_emsparse_nooutput_gen_empty<data>();
  parallel_for(0, m,
               [&](size_t i) {
                 uintT v = indices.vtx(i);
                 auto neighbors = (fl & in_edges)
                                      ? G.get_vertex(v).in_neighbors()
                                      : G.get_vertex(v).out_neighbors();
                 neighbors.decodeSparse(0, f, g, h);
               },
               1);
  return vertexSubsetData<data>(n);
}

template <
    class data /* data associated with vertices in the output vertex_subset */,
    class Graph /* graph type */, class VS /* vertex_subset type */,
    class F /* edgeMap struct */>
inline vertexSubsetData<data> edgeMapSparseNoOutput(Graph& G, VS& indices, F& f,
                                                    const flags fl) {
  size_t m = indices.numNonzeros();
  bool inner_parallel = true;

  auto n = G.n;
  auto g = get_emsparse_nooutput_gen<data>();
  auto h = get_emsparse_nooutput_gen_empty<data>();
  parallel_for(0, m,
               [&](size_t i) {
                 uintT v = indices.vtx(i);
                 auto neighbors = (fl & in_edges)
                                      ? G.get_vertex(v).in_neighbors()
                                      : G.get_vertex(v).out_neighbors();
                 neighbors.decodeSparse(0, f, g, h, inner_parallel);
               },
               1);
  return vertexSubsetData<data>(n);
}

struct block {
  uintE id;
  uintE block_num;
  block(uintE _id, uintE _b) : id(_id), block_num(_b) {}
  block() {}
  void print() { std::cout << id << " " << block_num << "\n"; }
};

template <
    class data /* data associated with vertices in the output vertex_subset */,
    class Graph /* graph type */, class VS /* vertex_subset type */,
    class F /* edgeMap struct */>
inline vertexSubsetData<data> edgeMapBlocked(Graph& G, VS& indices, F& f,
                                             const flags fl) {
  if (fl & no_output) {
    return edgeMapSparseNoOutput<data, Graph, VS, F>(G, indices, f, fl);
  }
  using S = std::tuple<uintE, data>;
  size_t n = indices.n;

  auto block_f = [&](size_t i) -> size_t {
    return (fl & in_edges)
               ? G.get_vertex(indices.vtx(i)).in_neighbors().get_num_blocks()
               : G.get_vertex(indices.vtx(i)).out_neighbors().get_num_blocks();
  };
  auto block_imap = parlay::delayed_seq<uintE>(indices.size(), block_f);

  // 1. Compute the number of blocks each vertex is subdivided into.
  auto vertex_offs = parlay::sequence<uintE>(indices.size() + 1);
  parallel_for(0, indices.size(),
               [&](size_t i) { vertex_offs[i] = block_imap[i]; });
  vertex_offs[indices.size()] = 0;
  size_t num_blocks = parlay::scan_inplace(make_slice(vertex_offs));
  std::cout << "# num_blocks = " << num_blocks << std::endl;

  auto blocks = parlay::sequence<block>(num_blocks);
  auto degrees = parlay::sequence<uintT>(num_blocks);

  // 2. Write each block to blocks and scan degree array.
  parallel_for(
      0, indices.size(),
      [&](size_t i) {
        size_t vtx_off = vertex_offs[i];
        size_t num_vertex_blocks = vertex_offs[i + 1] - vtx_off;
        uintE vtx_id = indices.vtx(i);
        assert(vtx_id < n);
        auto neighbors = (fl & in_edges) ? G.get_vertex(vtx_id).in_neighbors()
                                         : G.get_vertex(vtx_id).out_neighbors();
        parallel_for(0, num_vertex_blocks, [&](size_t j) {
          size_t block_deg = neighbors.block_degree(j);
          // assert(block_deg <= PARALLEL_DEGREE); // only for compressed
          blocks[vtx_off + j] = block(i, j);  // j-th block of the i-th vertex.
          degrees[vtx_off + j] = block_deg;
        });
      },
      1);
  parlay::scan_inclusive_inplace(make_slice(degrees));
  size_t outEdgeCount = degrees[num_blocks - 1];

  // 3. Compute the number of threads, binary search for offsets.
  size_t n_threads = parlay::num_blocks(outEdgeCount, kEMBlockSize);
  auto thread_offs = parlay::sequence<size_t>::uninitialized(n_threads + 1);
  auto lt = [](const uintT& l, const uintT& r) { return l < r; };
  parallel_for(0, n_threads,
               [&](size_t i) {
                 size_t start_off = i * kEMBlockSize;
                 thread_offs[i] = parlay::binary_search(degrees, start_off, lt);
               },
               1);
  thread_offs[n_threads] = num_blocks;

  // 4. Run each thread in parallel
  auto cts = parlay::sequence<uintE>(n_threads + 1);
  auto outEdges = parlay::sequence<S>::uninitialized(outEdgeCount);
  auto g = get_emsparse_blocked_gen<data>(outEdges);
  parallel_for(0, n_threads,
               [&](size_t i) {
                 size_t start = thread_offs[i];
                 size_t end = thread_offs[i + 1];
                 // <= kEMBlockSize edges in this range, sequentially process
                 if (start != end && start != num_blocks) {
                   size_t start_offset = (start == 0) ? 0 : degrees[start - 1];
                   size_t k = start_offset;
                   for (size_t j = start; j < end; j++) {
                     auto& block = blocks[j];
                     uintE id = block.id;  // id in vset
                     uintE block_num = block.block_num;

                     uintE vtx_id =
                         indices.vtx(id);  // actual vtx_id corresponding to id

                     auto our_nghs = (fl & in_edges)
                                         ? G.get_vertex(vtx_id).in_neighbors()
                                         : G.get_vertex(vtx_id).out_neighbors();
                     size_t num_in = our_nghs.decode_block(k, block_num, f, g);
                     k += num_in;
                   }
                   cts[i] = k - start_offset;
                 } else {
                   cts[i] = 0;
                 }
               },
               1);
  cts[n_threads] = 0;
  size_t out_size = parlay::scan_inplace(make_slice(cts));

  // 5. Use cts to get
  auto out = parlay::sequence<S>::uninitialized(out_size);
  parallel_for(0, n_threads,
               [&](size_t i) {
                 size_t start = thread_offs[i];
                 size_t end = thread_offs[i + 1];
                 if (start != end) {
                   size_t start_offset = (start == 0) ? 0 : degrees[start - 1];
                   size_t out_offset = cts[i];
                   size_t num_live = cts[i + 1] - out_offset;
                   for (size_t j = 0; j < num_live; j++) {
                     out[out_offset + j] = outEdges[start_offset + j];
                   }
                 }
               },
               1);
  cts.clear();
  vertex_offs.clear();
  blocks.clear();
  degrees.clear();

  return vertexSubsetData<data>(n, out_size, out);
}

constexpr size_t kDataBlockSizeBytes = 16384;
struct em_data_block {
  size_t block_size;
  uint8_t data[kDataBlockSizeBytes];
};

// block format:
// size_t block_size (8 bytes for alignment)
// remainder is used as std::tuple<uintE, data>

template <class data, class Graph>
struct emhelper {
  using thread_blocks = std::vector<em_data_block*>;
  using ngh_data = std::tuple<uintE, data>;
  static constexpr size_t max_block_size =
      kDataBlockSizeBytes / sizeof(ngh_data);

  using vertex = typename Graph::vertex;
  static constexpr size_t work_block_size = vertex::getInternalBlockSize();

  uintE n_groups;
  parlay::sequence<thread_blocks> perthread_blocks;
  parlay::sequence<size_t> perthread_counts;

  static constexpr size_t kPerThreadStride = 128 / sizeof(size_t);
  static constexpr size_t kThreadBlockStride = 1;  // 128/sizeof(thread_blocks);

  emhelper(size_t n_groups) : n_groups(n_groups) {
    perthread_blocks =
        parlay::sequence<thread_blocks>(n_groups * kThreadBlockStride);
    perthread_counts = parlay::sequence<size_t>(n_groups);
  }

  inline size_t scan_perthread_blocks() {
    size_t ct = 0;
    for (size_t i = 0; i < n_groups; i++) {
      size_t i_sz = perthread_blocks[i * kThreadBlockStride].size();
      perthread_counts[i] = ct;
      ct += i_sz;
    }
    return ct;
  }

  auto get_all_blocks() {
    size_t total_blocks = scan_perthread_blocks();
    auto all_blocks = parlay::sequence<em_data_block*>(total_blocks);
    if (total_blocks < 1000) {  // handle sequentially
      size_t k = 0;
      for (size_t i = 0; i < n_groups; i++) {
        auto& vec = perthread_blocks[i * kThreadBlockStride];
        size_t this_thread_size = vec.size();
        for (size_t j = 0; j < this_thread_size; j++) {
          all_blocks[k++] = vec[j];
        }
        vec.clear();
      }
    } else {
      parallel_for(0, n_groups,
                   [&](size_t thread_id) {
                     size_t this_thread_offset = perthread_counts[thread_id];
                     auto& vec =
                         perthread_blocks[thread_id * kThreadBlockStride];
                     size_t this_thread_size = vec.size();
                     for (size_t j = 0; j < this_thread_size; j++) {
                       all_blocks[this_thread_offset + j] = vec[j];
                     }
                     vec.clear();
                   },
                   1);
    }
    return all_blocks;
  }

  // returns the next block for this group, reallocates if nec.
  em_data_block* get_block_and_offset_for_group(size_t group_id) {
    // fetch current block for thread
    em_data_block* block_ptr;
    size_t offset = 0;
    auto& vec = perthread_blocks[group_id * kThreadBlockStride];
    if (vec.size() == 0) {  // alloc new
      block_ptr = gbbs::new_array_no_init<em_data_block>(1);
      vec.emplace_back(block_ptr);
      block_ptr->block_size = 0;
    } else {
      block_ptr = vec.back();
      offset = block_ptr->block_size;
      if (offset + work_block_size > max_block_size) {  // realloc
        block_ptr = gbbs::new_array_no_init<em_data_block>(1);
        vec.emplace_back(block_ptr);
        block_ptr->block_size = 0;
        offset = 0;
      }
    }
    return block_ptr;
  }
};

template <
    class data /* data associated with vertices in the output vertex_subset */,
    class Graph /* graph type */, class VS /* vertex_subset type */,
    class F /* edgeMap struct */>
inline vertexSubsetData<data> edgeMapChunked(Graph& G, VS& indices, F& f,
                                             const flags fl) {
  if (fl & no_output) {
    return edgeMapSparseNoOutput<data, Graph, VS, F>(G, indices, f, fl);
  }
  using S = typename vertexSubsetData<data>::S;
  size_t n = indices.n;

  auto block_f = [&](size_t i) -> size_t {
    uintE vtx_id = indices.vtx(i);
    auto nghs = (fl & in_edges) ? G.get_vertex(vtx_id).in_neighbors()
                                : G.get_vertex(vtx_id).out_neighbors();
    return nghs.get_num_blocks();
  };
  auto block_imap = parlay::delayed_seq<uintE>(indices.size(), block_f);

  // 1. Compute the number of blocks each vertex is subdivided into.
  auto vertex_offs = parlay::sequence<uintE>(indices.size() + 1);
  parallel_for(0, indices.size(),
               [&](size_t i) { vertex_offs[i] = block_imap[i]; });
  vertex_offs[indices.size()] = 0;
  size_t num_blocks = parlay::scan_inplace(make_slice(vertex_offs));

  auto blocks = parlay::sequence<block>(num_blocks);
  auto degrees = parlay::sequence<uintT>(num_blocks);

  // 2. Write each block to blocks and scan degree array.
  parallel_for(0, indices.size(), [&](size_t i) {
    size_t vtx_off = vertex_offs[i];
    size_t num_vertex_blocks = vertex_offs[i + 1] - vtx_off;
    uintE vtx_id = indices.vtx(i);
    assert(vtx_id < n);
    auto neighbors = (fl & in_edges) ? G.get_vertex(vtx_id).in_neighbors()
                                     : G.get_vertex(vtx_id).out_neighbors();
    parallel_for(0, num_vertex_blocks, [&](size_t j) {
      size_t block_deg = neighbors.block_degree(j);
      // assert(block_deg <= PARALLEL_DEGREE); // only for compressed
      blocks[vtx_off + j] = block(i, j);  // j-th block of the i-th vertex.
      degrees[vtx_off + j] = block_deg;
    });
  });
  parlay::scan_inclusive_inplace(make_slice(degrees));
  vertex_offs.clear();
  size_t outEdgeCount = degrees[num_blocks - 1];

  // 3. Compute the number of threads, binary search for offsets.
  // try to use 8*p threads, fewer only if the blocksize guess is smaller than
  // kEMBlockSize
  size_t edge_block_size_guess =
      parlay::num_blocks(outEdgeCount, num_workers() << 3);
  size_t edge_block_size = std::max(kEMBlockSize, edge_block_size_guess);
  size_t n_groups = parlay::num_blocks(outEdgeCount, edge_block_size);

  //  std::cout << "outEdgeCount = " << outEdgeCount << std::endl;
  //  std::cout << "n_blocks = " << num_blocks << std::endl;
  //  std::cout << "n_groups = " << n_groups << std::endl;

  auto our_emhelper = emhelper<data, Graph>(n_groups);

  // Run each thread in parallel
  auto lt = [](const uintT& l, const uintT& r) { return l < r; };
  parallel_for(
      0, n_groups,
      [&](size_t group_id) {
        size_t start_off = group_id * edge_block_size;
        size_t our_start = parlay::binary_search(degrees, start_off, lt);
        size_t our_end;
        if (group_id < (n_groups - 1)) {
          size_t next_start_off = (group_id + 1) * edge_block_size;
          our_end = parlay::binary_search(degrees, next_start_off, lt);
        } else {
          our_end = num_blocks;
        }

        // <= block_size edges in this range, sequentially process
        if (our_start != our_end && our_start != num_blocks) {
          for (size_t work_id = our_start; work_id < our_end; work_id++) {
            // 1. before starting next work block check whether we need to
            // reallocate
            // the output block. This guarantees that there is enough space in
            // the
            // output block even if all items in the work block are written out
            em_data_block* out_block =
                our_emhelper.get_block_and_offset_for_group(group_id);
            size_t offset = out_block->block_size;
            auto out_block_data = (S*)out_block->data;

            auto g = get_emblock_gen<data>(out_block_data);

            // 2. process the work block
            auto& block = blocks[work_id];
            uintE id = block.id;  // id in vset
            uintE block_num = block.block_num;
            uintE vtx_id =
                indices.vtx(id);  // actual vtx_id corresponding to id
            auto neighbors = (fl & in_edges)
                                 ? G.get_vertex(vtx_id).in_neighbors()
                                 : G.get_vertex(vtx_id).out_neighbors();
            size_t num_in = neighbors.decode_block(offset, block_num, f, g);
            out_block->block_size += num_in;
          }
        }
      },
      1);

  // scan the #output blocks/thread
  parlay::sequence<em_data_block*> all_blocks = our_emhelper.get_all_blocks();
  auto block_offsets = parlay::sequence<size_t>::from_function(
      all_blocks.size(), [&](size_t i) { return all_blocks[i]->block_size; });
  size_t output_size = parlay::scan_inplace(make_slice(block_offsets));
  vertexSubsetData<data> ret(n);
  if (output_size > 0) {
    auto out = parlay::sequence<S>(output_size);

    parallel_for(0, all_blocks.size(),
                 [&](size_t block_id) {
                   em_data_block* block = all_blocks[block_id];
                   size_t block_size = block->block_size;
                   auto block_data = (S*)block->data;
                   size_t block_offset = block_offsets[block_id];
                   for (size_t i = 0; i < block_size; i++) {
                     out[block_offset + i] = block_data[i];
                   }
                   // deallocate block to list_alloc
                   gbbs::free_array(block, 1);
                 },
                 1);
    ret = vertexSubsetData<data>(n, std::move(out));
  } else {
    parallel_for(0, all_blocks.size(), [&](size_t block_id) {
      em_data_block* block = all_blocks[block_id];
      gbbs::free_array(block, 1);
    });
  }
  block_offsets.clear();

  return ret;
}
}  // namespace gbbs
