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

#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <stdlib.h>
#include <string>

#include "bridge.h"
#include "edge_array.h"
#include "flags.h"
#include "macros.h"
#include "vertex.h"

namespace gbbs
{
   //  Compressed Sparse Row (CSR) based representation for symmetric graphs.
   //  Takes two template parameters:
   //  1) vertex_type: vertex template, parametrized by the weight type associated
   //  with each edge
   //  2) W: the edge weight template
   //  The graph is represented as an array of edges of type
   //  vertex_type::edge_type.
   //  For uncompressed vertices, this type is equal to tuple<uintE, W>.
   template <template <class W> class vertex_type, class W>
   struct symmetric_graph
   {
      using vertex = vertex_type<W>;
      using weight_type = W;
      using edge_type = typename vertex::edge_type;
      using graph = symmetric_graph<vertex_type, W>;
      using vertex_weight_type = double;

      size_t num_vertices()
      {
         return n;
      }
      size_t num_edges()
      {
         return m;
      }

      // ======== Graph operators that perform packing ========
      template <class P>
      uintE packNeighbors(uintE id, P& p, uint8_t* tmp)
      {
         uintE new_degree = get_vertex(id).out_neighbors().pack(p, (std::tuple<uintE, W>*)tmp);
         v_data[id].degree = new_degree; // updates the degree
         return new_degree;
      }

      // degree must be <= old_degree
      void decreaseVertexDegree(uintE id, uintE degree)
      {
         assert(degree <= v_data[id].degree);
         v_data[id].degree = degree;
      }

      void zeroVertexDegree(uintE id)
      {
         decreaseVertexDegree(id, 0);
      }

      sequence<std::tuple<uintE, uintE, W>> edges()
      {
         using g_edge = std::tuple<uintE, uintE, W>;
         auto degs = sequence<size_t>::from_function(n, [&](size_t i) { return get_vertex(i).out_degree(); });
         size_t sum_degs = parlay::scan_inplace(make_slice(degs));
         assert(sum_degs == m);
         auto edges = sequence<g_edge>(sum_degs);
         parallel_for(
             0, n,
             [&](size_t i) {
                size_t k = degs[i];
                auto map_f = [&](const uintE& u, const uintE& v, const W& wgh) {
                   edges[k++] = std::make_tuple(u, v, wgh);
                };
                get_vertex(i).out_neighbors().map(map_f, false);
             },
             1);
         return edges;
      }

      template <class F>
      void mapEdges(F f, bool parallel_inner_map = true, size_t granularity = 1)
      {
         parallel_for(
             0, n, [&](size_t i) { get_vertex(i).out_neighbors().map(f, parallel_inner_map); }, granularity);
      }

      template <class M, class R>
      typename R::T reduceEdges(M map_f, R reduce_f)
      {
         using T = typename R::T;
         auto D =
             parlay::delayed_seq<T>(n, [&](size_t i) { return get_vertex(i).out_neighbors().reduce(map_f, reduce_f); });
         return parlay::reduce(D, reduce_f);
      }

      // ======================= Constructors and fields  ========================
      symmetric_graph() : v_data(nullptr), e0(nullptr), vertex_weights(nullptr), n(0), m(0), deletion_fn([]() {})
      {
      }

      symmetric_graph(vertex_data* v_data, size_t n, size_t m, std::function<void()> _deletion_fn, edge_type* _e0,
                      vertex_weight_type* _vertex_weights = nullptr)
          : v_data(v_data), e0(_e0), vertex_weights(_vertex_weights), n(n), m(m), deletion_fn(_deletion_fn)
      {
      }

      // Move constructor
      symmetric_graph(symmetric_graph&& other) noexcept
      {
         n = other.n;
         m = other.m;
         v_data = other.v_data;
         e0 = other.e0;
         vertex_weights = other.vertex_weights;
         deletion_fn = std::move(other.deletion_fn);
         other.v_data = nullptr;
         other.e0 = nullptr;
         other.vertex_weights = nullptr;
         other.deletion_fn = []() {};
      }

      // Move assignment
      symmetric_graph& operator=(symmetric_graph&& other) noexcept
      {
         n = other.n;
         m = other.m;
         v_data = other.v_data;
         e0 = other.e0;
         vertex_weights = other.vertex_weights;
         deletion_fn();
         deletion_fn = std::move(other.deletion_fn);
         other.v_data = nullptr;
         other.e0 = nullptr;
         other.vertex_weights = nullptr;
         other.deletion_fn = []() {};
      }

      // Copy constructor
      symmetric_graph(const symmetric_graph& other)
      {
         debug(std::cout << "Copying symmetric graph." << std::endl;);
         n = other.n;
         m = other.m;
         v_data = gbbs::new_array_no_init<vertex_data>(n);
         e0 = gbbs::new_array_no_init<edge_type>(m);
         parallel_for(0, n, [&](size_t i) { v_data[i] = other.v_data[i]; });
         parallel_for(0, m, [&](size_t i) { e0[i] = other.e0[i]; });
         deletion_fn = [=]() {
            gbbs::free_array(v_data, n);
            gbbs::free_array(e0, m);
            if(vertex_weights != nullptr)
            {
               gbbs::free_array(vertex_weights, n);
            }
         };
         vertex_weights = nullptr;
         if(other.vertex_weights != nullptr)
         {
            vertex_weights = gbbs::new_array_no_init<vertex_weight_type>(n);
            parallel_for(0, n, [&](size_t i) { vertex_weights[i] = other.vertex_weights[i]; });
         }
      }

      ~symmetric_graph()
      {
         deletion_fn();
      }

      vertex get_vertex(uintE i)
      {
         return vertex(e0, v_data[i], i);
      }

      // Graph Data
      vertex_data* v_data;
      // Pointer to edges
      edge_type* e0;
      // Pointer to vertex weights
      vertex_weight_type* vertex_weights;

      // number of vertices in G
      size_t n;
      // number of edges in G
      size_t m;

      // called to delete the graph
      std::function<void()> deletion_fn;
   };

   // Similar to symmetric_graph, but edges are not necessarily allocated
   // consecutively. The structure simply stores an array of vertex
   // objects (which store an 8-byte pointer, and a uintE degree each).
   template <template <class W> class vertex_type, class W>
   struct symmetric_ptr_graph
   {
      using vertex = vertex_type<W>;
      using weight_type = W;
      using edge_type = typename vertex::edge_type;
      using graph = symmetric_ptr_graph<vertex_type, W>;
      using vertex_weight_type = double;

      size_t num_vertices()
      {
         return n;
      }
      size_t num_edges()
      {
         return m;
      }

      // ======== Graph operators that perform packing ========
      template <class P>
      uintE packNeighbors(uintE id, P& p, uint8_t* tmp)
      {
         uintE new_degree = get_vertex(id).out_neighbors().pack(p, (std::tuple<uintE, W>*)tmp);
         vertices[id].degree = new_degree; // updates the degree
         return new_degree;
      }

      // degree must be <= old_degree
      void decreaseVertexDegree(uintE id, uintE degree)
      {
         assert(degree <= vertices[id].degree);
         vertices[id].degree = degree;
      }

      void zeroVertexDegree(uintE id)
      {
         decreaseVertexDegree(id, 0);
      }

      sequence<std::tuple<uintE, uintE, W>> edges()
      {
         using g_edge = std::tuple<uintE, uintE, W>;
         auto degs = sequence<size_t>::from_function(n, [&](size_t i) { return get_vertex(i).out_degree(); });
         size_t sum_degs = parlay::scan_inplace(make_slice(degs));
         assert(sum_degs == m);
         auto edges = sequence<g_edge>(sum_degs);
         parallel_for(
             0, n,
             [&](size_t i) {
                size_t k = degs[i];
                auto map_f = [&](const uintE& u, const uintE& v, const W& wgh) {
                   edges[k++] = std::make_tuple(u, v, wgh);
                };
                get_vertex(i).out_neighbors().map(map_f, false);
             },
             1);
         return edges;
      }

      template <class F>
      void mapEdges(F f, bool parallel_inner_map = true)
      {
         parallel_for(
             0, n, [&](size_t i) { get_vertex(i).out_neighbors().map(f, parallel_inner_map); }, 1);
      }

      template <class M, class R>
      typename R::T reduceEdges(M map_f, R reduce_f)
      {
         using T = typename R::T;
         auto D =
             parlay::delayed_seq<T>(n, [&](size_t i) { return get_vertex(i).out_neighbors().reduce(map_f, reduce_f); });
         return parlay::reduce(D, reduce_f);
      }

      // ======================= Constructors and fields  ========================
      symmetric_ptr_graph()
          : n(0), m(0), vertices(nullptr), edge_list_sizes(nullptr), vertex_weights(nullptr), deletion_fn([]() {})
      {
      }

      symmetric_ptr_graph(size_t n, size_t m, vertex* _vertices, std::function<void()> _deletion_fn,
                          vertex_weight_type* _vertex_weights = nullptr, uintE* _edge_list_sizes = nullptr)
          : n(n),
            m(m),
            vertices(_vertices),
            edge_list_sizes(_edge_list_sizes),
            vertex_weights(_vertex_weights),
            deletion_fn(_deletion_fn)
      {
      }

      // Move constructor
      symmetric_ptr_graph(symmetric_ptr_graph&& other) noexcept
      {
         n = other.n;
         m = other.m;
         vertices = other.vertices;
         edge_list_sizes = other.edge_list_sizes;
         vertex_weights = other.vertex_weights;
         deletion_fn = std::move(other.deletion_fn);
         other.vertices = nullptr;
         other.edge_list_sizes = nullptr;
         other.vertex_weights = nullptr;
         other.deletion_fn = []() {};
      }

      // Move assignment
      symmetric_ptr_graph& operator=(symmetric_ptr_graph&& other) noexcept
      {
         n = other.n;
         m = other.m;
         vertices = other.vertices;
         edge_list_sizes = other.edge_list_sizes;
         vertex_weights = other.vertex_weights;
         deletion_fn();
         deletion_fn = std::move(other.deletion_fn);
         other.vertices = nullptr;
         other.edge_list_sizes = nullptr;
         other.vertex_weights = nullptr;
         other.deletion_fn = []() {};
      }

      // Copy constructor
      symmetric_ptr_graph(const symmetric_ptr_graph& other)
      {
         n = other.n;
         m = other.m;
         vertices = gbbs::new_array_no_init<vertex>(n);
         auto offsets = sequence<size_t>(n + 1);
         parallel_for(0, n, [&](size_t i) {
            vertices[i] = other.vertices[i];
            offsets[i] = vertices[i].out_degree();
         });
         offsets[n] = 0;
         size_t total_space = parlay::scan_inplace(make_slice(offsets));
         edge_type* E = gbbs::new_array_no_init<edge_type>(total_space);

         parallel_for(0, n, [&](size_t i) {
            size_t offset = offsets[i];
            auto map_f = [&](const uintE& u, const uintE& v, const W& wgh, size_t ind) {
               E[offset + ind] = std::make_tuple(v, wgh);
            };
            // Copy neighbor data into E.
            vertices[i].out_neighbors().map_with_index(map_f);
            // Update this vertex's pointer to point to E.
            vertices[i].neighbors = E + offset;
         });

         deletion_fn = [=]() {
            gbbs::free_array(vertices, n);
            gbbs::free_array(E, total_space);
            if(vertex_weights != nullptr)
            {
               gbbs::free_array(vertex_weights, n);
            }
         };
         vertex_weights = nullptr;
         if(other.vertex_weights != nullptr)
         {
            vertex_weights = gbbs::new_array_no_init<vertex_weight_type>(n);
            parallel_for(0, n, [&](size_t i) { vertex_weights[i] = other.vertex_weights[i]; });
         }
      }

      ~symmetric_ptr_graph()
      {
         deletion_fn();
      }

      // Note that observers recieve a handle to a vertex object which is only valid
      // so long as this graph's memory is valid.
      vertex get_vertex(uintE i)
      {
         return vertices[i];
      }

      // number of vertices in G
      size_t n;
      // number of edges in G
      size_t m;
      // pointer to array of vertex objects
      vertex* vertices;
      // pointer to array of vertex edge-list sizes---necessary if copying a
      // compressed graph in this representation.
      uintE* edge_list_sizes;
      // pointer to array of vertex weights
      vertex_weight_type* vertex_weights;

      // called to delete the graph
      std::function<void()> deletion_fn;
   };

   /* Compressed Sparse Row (CSR) based representation for asymmetric
    * graphs.  Note that the symmetric/asymmetric structures are pretty
    * similar, but defined separately. The purpose is to try and avoid
    * errors where an algorithm intended for symmetric graphs (e.g.,
    * biconnectivity) is not mistakenly called on a directed graph.
    *
    * Takes two template parameters:
    * 1) vertex_type: vertex template, parametrized by the weight type
    *    associated with each edge
    * 2) W: the edge weight template
    *
    * The graph is represented as an array of edges of type
    * vertex_type::edge_type, which is just a pair<uintE, W>.
    * */
   template <template <class W> class vertex_type, class W>
   struct asymmetric_graph
   {
      using vertex = vertex_type<W>;
      using weight_type = W;
      using edge_type = typename vertex::edge_type;
      using vertex_weight_type = double;

      // number of vertices in G
      size_t n;
      // number of edges in G
      size_t m;
      // called to delete the graph
      std::function<void()> deletion_fn;

      vertex_data* v_out_data;
      vertex_data* v_in_data;

      // Pointer to out-edges
      edge_type* out_edges;

      // Pointer to in-edges
      edge_type* in_edges;

      // Pointer to vertex weights
      vertex_weight_type* vertex_weights;

      vertex get_vertex(size_t i)
      {
         return vertex(out_edges, v_out_data[i], in_edges, v_in_data[i], i);
      }

      asymmetric_graph()
          : n(0),
            m(0),
            deletion_fn([]() {}),
            v_out_data(nullptr),
            v_in_data(nullptr),
            out_edges(nullptr),
            in_edges(nullptr),
            vertex_weights(nullptr)
      {
      }

      asymmetric_graph(vertex_data* v_out_data, vertex_data* v_in_data, size_t n, size_t m,
                       std::function<void()> _deletion_fn, edge_type* _out_edges, edge_type* _in_edges,
                       vertex_weight_type* _vertex_weights = nullptr)
          : n(n),
            m(m),
            deletion_fn(_deletion_fn),
            v_out_data(v_out_data),
            v_in_data(v_in_data),
            out_edges(_out_edges),
            in_edges(_in_edges),
            vertex_weights(_vertex_weights)
      {
      }

      // Move constructor
      asymmetric_graph(asymmetric_graph&& other) noexcept
      {
         n = other.n;
         m = other.m;
         v_out_data = other.v_out_data;
         v_in_data = other.v_in_data;
         out_edges = other.out_edges;
         in_edges = other.in_edges;
         vertex_weights = other.vertex_weights;
         deletion_fn = std::move(other.deletion_fn);
         other.v_out_data = nullptr;
         other.v_in_data = nullptr;
         other.out_edges = nullptr;
         other.in_edges = nullptr;
         other.vertex_weights = nullptr;
         other.deletion_fn = []() {};
      }

      // Move assignment
      asymmetric_graph& operator=(asymmetric_graph&& other) noexcept
      {
         n = other.n;
         m = other.m;
         v_out_data = other.v_out_data;
         v_in_data = other.v_in_data;
         out_edges = other.out_edges;
         in_edges = other.in_edges;
         vertex_weights = other.vertex_weights;
         deletion_fn();
         deletion_fn = std::move(other.deletion_fn);
         other.v_out_data = nullptr;
         other.v_in_data = nullptr;
         other.out_edges = nullptr;
         other.in_edges = nullptr;
         other.vertex_weights = nullptr;
         other.deletion_fn = []() {};
         return *this;
      }

      // Copy constructor
      asymmetric_graph(const asymmetric_graph& other)
      {
         debug(std::cout << "Copying asymmetric graph." << std::endl;);
         n = other.n;
         m = other.m;
         v_out_data = gbbs::new_array_no_init<vertex_data>(n);
         v_in_data = gbbs::new_array_no_init<vertex_data>(n);
         out_edges = gbbs::new_array_no_init<edge_type>(m);
         in_edges = gbbs::new_array_no_init<edge_type>(m);
         parallel_for(0, n, [&](size_t i) { v_out_data[i] = other.v_out_data[i]; });
         parallel_for(0, n, [&](size_t i) { v_in_data[i] = other.v_in_data[i]; });
         parallel_for(0, m, [&](size_t i) { out_edges[i] = other.out_edges[i]; });
         parallel_for(0, m, [&](size_t i) { in_edges[i] = other.in_edges[i]; });
         deletion_fn = [=]() {
            gbbs::free_array(v_out_data, n);
            gbbs::free_array(v_in_data, n);
            gbbs::free_array(out_edges, m);
            gbbs::free_array(in_edges, m);
            if(vertex_weights != nullptr)
            {
               gbbs::free_array(vertex_weights, n);
            }
         };
         vertex_weights = nullptr;
         if(other.vertex_weights != nullptr)
         {
            vertex_weights = gbbs::new_array_no_init<vertex_weight_type>(n);
            parallel_for(0, n, [&](size_t i) { vertex_weights[i] = other.vertex_weights[i]; });
         }
      }

      ~asymmetric_graph()
      {
         deletion_fn();
      }

      template <class F>
      void mapEdges(F f, bool parallel_inner_map = true)
      {
         parallel_for(
             0, n, [&](size_t i) { get_vertex(i).out_neighbors().map(f, parallel_inner_map); }, 1);
      }
   };

   // Similar to asymmetric_graph, but edges are not necessarily allocated
   // consecutively.
   template <template <class W> class vertex_type, class W>
   struct asymmetric_ptr_graph
   {
      using vertex = vertex_type<W>;
      using weight_type = W;
      using edge_type = typename vertex::edge_type;
      using vertex_weight_type = double;

      // number of vertices in G
      size_t n;
      // number of edges in G
      size_t m;
      // pointer to array of vertex object
      vertex* vertices;
      // pointer to array of vertex weights
      vertex_weight_type* vertex_weights;

      // called to delete the graph
      std::function<void()> deletion_fn;

      vertex get_vertex(size_t i)
      {
         return vertices[i];
      }

      asymmetric_ptr_graph() : n(0), m(0), vertices(nullptr), deletion_fn([]() {}), vertex_weights(nullptr)
      {
      }

      asymmetric_ptr_graph(size_t n, size_t m, vertex* _vertices, std::function<void()> _deletion_fn,
                           vertex_weight_type* _vertex_weights = nullptr)
          : n(n), m(m), vertices(_vertices), deletion_fn(_deletion_fn), vertex_weights(_vertex_weights)
      {
      }

      // Move constructor
      asymmetric_ptr_graph(asymmetric_ptr_graph&& other) noexcept
      {
         n = other.n;
         m = other.m;
         vertices = other.vertices;
         vertex_weights = other.vertex_weights;
         deletion_fn = std::move(other.deletion_fn);
         other.vertices = nullptr;
         other.vertex_weights = nullptr;
         other.deletion_fn = []() {};
      }

      // Move assignment
      asymmetric_ptr_graph& operator=(asymmetric_ptr_graph&& other) noexcept
      {
         n = other.n;
         m = other.m;
         vertices = other.vertices;
         vertex_weights = other.vertex_weights;
         deletion_fn();
         deletion_fn = std::move(other.deletion_fn);
         other.vertices = nullptr;
         other.vertex_weights = nullptr;
         other.deletion_fn = []() {};
      }

      // Copy constructor
      asymmetric_ptr_graph(const asymmetric_ptr_graph& other)
      {
         n = other.n;
         m = other.m;
         vertices = gbbs::new_array_no_init<vertex>(n);
         auto in_offsets = sequence<size_t>(n + 1);
         auto out_offsets = sequence<size_t>(n + 1);
         parallel_for(0, n, [&](size_t i) {
            vertices[i] = other.vertices[i];
            out_offsets[i] = vertices[i].out_degree();
            in_offsets[i] = vertices[i].in_degree();
         });
         in_offsets[n] = 0;
         out_offsets[n] = 0;

         size_t in_space = parlay::scan_inplace(make_slice(in_offsets));
         size_t out_space = parlay::scan_inplace(make_slice(out_offsets));
         edge_type* inE = gbbs::new_array_no_init<edge_type>(in_space);
         edge_type* outE = gbbs::new_array_no_init<edge_type>(out_space);

         parallel_for(0, n, [&](size_t i) {
            size_t out_offset = out_offsets[i];
            if(out_offsets[i + 1] != out_offset)
            {
               auto map_f = [&](const uintE& u, const uintE& v, const W& wgh, size_t ind) {
                  outE[out_offset + ind] = std::make_tuple(v, wgh);
               };
               // Copy neighbor data into E.
               vertices[i].out_neighbors().map_with_index(map_f);
               // Update this vertex's pointer to point to E.
               vertices[i].out_nghs = outE + out_offset;
            }

            size_t in_offset = in_offsets[i];
            if(in_offsets[i + 1] != in_offset)
            {
               auto map_f = [&](const uintE& u, const uintE& v, const W& wgh, size_t ind) {
                  inE[in_offset + ind] = std::make_tuple(v, wgh);
               };
               // Copy neighbor data into E.
               vertices[i].in_neighbors().map_with_index(map_f);
               // Update this vertex's pointer to point to E.
               vertices[i].in_nghs = inE + in_offset;
            }
         });

         deletion_fn = [=]() {
            gbbs::free_array(vertices, n);
            gbbs::free_array(inE, in_space);
            gbbs::free_array(outE, out_space);
            if(vertex_weights != nullptr)
            {
               gbbs::free_array(vertex_weights, n);
            }
         };
         vertex_weights = nullptr;
         if(other.vertex_weights != nullptr)
         {
            vertex_weights = gbbs::new_array_no_init<vertex_weight_type>(n);
            parallel_for(0, n, [&](size_t i) { vertex_weights[i] = other.vertex_weights[i]; });
         }
      }

      ~asymmetric_ptr_graph()
      {
         deletion_fn();
      }

      template <class F>
      void mapEdges(F f, bool parallel_inner_map = true)
      {
         parallel_for(
             0, n, [&](size_t i) { get_vertex(i).out_neighbors().map(f, parallel_inner_map); }, 1);
      }
   };

   // Mutates (sorts) the underlying array A containing a black-box description of
   // an edge of typename A::value_type. The caller provides functions GetU, GetV,
   // and GetW which extract the u, v, and weight of a (u,v,w) edge respective (if
   // the edge is a std::tuple<uinte, uintE, W> this is just get<0>, ..<1>, ..<2>
   // respectively.
   // e.g.:
   //   using edge = std::tuple<uintE, uintE, W>;
   //   auto get_u = [&] (const edge& e) { return std::get<0>(e); };
   //   auto get_v = [&] (const edge& e) { return std::get<1>(e); };
   //   auto get_w = [&] (const edge& e) { return std::get<2>(e); };
   //   auto G = sym_graph_from_edges<W>(coo1, get_u, get_v, get_w, 10, false);
   template <class Wgh, class EdgeSeq, class GetU, class GetV, class GetW>
   static inline symmetric_graph<symmetric_vertex, Wgh>
   sym_graph_from_edges(EdgeSeq& A, size_t n, GetU&& get_u, GetV&& get_v, GetW&& get_w, bool is_sorted = false)
   {
      using vertex = symmetric_vertex<Wgh>;
      using edge_type = typename vertex::edge_type;
      size_t m = A.size();

      if(m == 0)
      {
         if(n == 0)
         {
            std::function<void()> del = []() {};
            return symmetric_graph<symmetric_vertex, Wgh>(nullptr, 0, 0, del, nullptr);
         }
         else
         {
            auto v_data = gbbs::new_array_no_init<vertex_data>(n);
            parallel_for(0, n, [&](size_t i) {
               v_data[i].offset = 0;
               v_data[i].degree = 0;
            });
            return symmetric_graph<symmetric_vertex, Wgh>(
                v_data, n, 0, [=]() { gbbs::free_array(v_data, n); }, nullptr);
         }
      }

      if(!is_sorted)
      {
         parlay::integer_sort_inplace(make_slice(A), get_u);
      }

      auto starts = sequence<uintT>(n + 1, (uintT)0);

      using neighbor = std::tuple<uintE, Wgh>;
      auto edges = gbbs::new_array_no_init<neighbor>(m);
      parallel_for(0, m, [&](size_t i) {
         if(i == 0 || (get_u(A[i]) != get_u(A[i - 1])))
         {
            starts[get_u(A[i])] = i;
         }
         if(i != (m - 1))
         {
            uintE our_vtx = get_u(A[i]);
            uintE next_vtx = get_u(A[i + 1]);
            if(our_vtx != next_vtx && (our_vtx + 1 != next_vtx))
            {
               parallel_for(our_vtx + 1, next_vtx, [&](size_t k) { starts[k] = i + 1; });
            }
         }
         if(i == (m - 1))
         { /* last edge */
            parallel_for(get_u(A[i]) + 1, starts.size(), [&](size_t j) { starts[j] = m; });
         }
         edges[i] = std::make_tuple(get_v(A[i]), get_w(A[i]));
      });

      auto v_data = gbbs::new_array_no_init<vertex_data>(n);
      parallel_for(0, n, [&](size_t i) {
         uintT o = starts[i];
         v_data[i].offset = o;
         v_data[i].degree = (uintE)(((i == (n - 1)) ? m : starts[i + 1]) - o);
      });
      return symmetric_graph<symmetric_vertex, Wgh>(
          v_data, n, m,
          [=]() {
             gbbs::free_array(v_data, n);
             gbbs::free_array(edges, m);
          },
          (edge_type*)edges);
   }

   template <class Wgh>
   static inline symmetric_graph<symmetric_vertex, Wgh> sym_graph_from_edges(sequence<std::tuple<uintE, uintE, Wgh>>& A,
                                                                             size_t n, bool is_sorted = false)
   {
      using edge = std::tuple<uintE, uintE, Wgh>;
      auto get_u = [&](const edge& e) { return std::get<0>(e); };
      auto get_v = [&](const edge& e) { return std::get<1>(e); };
      auto get_w = [&](const edge& e) { return std::get<2>(e); };
      return sym_graph_from_edges<Wgh>(A, n, get_u, get_v, get_w, is_sorted);
   }

   template <class Wgh, class EdgeSeq, class GetU, class GetV, class GetW>
   std::tuple<uintE, Wgh>* get_edges(EdgeSeq& A, sequence<uintT>& starts, size_t m, const GetU& get_u,
                                     const GetV& get_v, const GetW& get_w)
   {
      using neighbor = std::tuple<uintE, Wgh>;
      auto edges = gbbs::new_array_no_init<neighbor>(m);
      parallel_for(0, m, [&](size_t i) {
         if(i == 0 || (get_u(A[i]) != get_u(A[i - 1])))
         {
            starts[get_u(A[i])] = i;
         }
         if(i != (m - 1))
         {
            uintE our_vtx = get_u(A[i]);
            uintE next_vtx = get_u(A[i + 1]);
            if(our_vtx != next_vtx && (our_vtx + 1 != next_vtx))
            {
               parallel_for(our_vtx + 1, next_vtx, [&](size_t k) { starts[k] = i + 1; });
            }
         }
         if(i == (m - 1))
         { /* last edge */
            parallel_for(get_u(A[i]) + 1, starts.size(), [&](size_t j) { starts[j] = m; });
         }
         edges[i] = std::make_tuple(get_v(A[i]), get_w(A[i]));
      });
      return edges;
   }

   // Mutates (sorts) the underlying array A containing a black-box description of
   // an edge of typename A::value_type. The caller provides functions GetU, GetV,
   // and GetW which extract the u, v, and weight of a (u,v,w) edge respective (if
   // the edge is a std::tuple<uinte, uintE, W> this is just get<0>, ..<1>, ..<2>
   // respectively.
   // e.g.:
   //   using edge = std::tuple<uintE, uintE, W>;
   //   auto get_u = [&] (const edge& e) { return std::get<0>(e); };
   //   auto get_v = [&] (const edge& e) { return std::get<1>(e); };
   //   auto get_w = [&] (const edge& e) { return std::get<2>(e); };
   //   auto G = asym_graph_from_edges<W>(coo1, get_u, get_v, get_w, 10, false);
   template <class Wgh, class EdgeSeq, class GetU, class GetV, class GetW>
   static inline asymmetric_graph<asymmetric_vertex, Wgh>
   asym_graph_from_edges(EdgeSeq& A, size_t n, GetU&& get_u, GetV&& get_v, GetW&& get_w, bool is_sorted = false)
   {
      using vertex = asymmetric_vertex<Wgh>;
      using edge_type = typename vertex::edge_type;
      size_t m = A.size();

      if(m == 0)
      {
         if(n == 0)
         {
            std::function<void()> del = []() {};
            return asymmetric_graph<asymmetric_vertex, Wgh>(nullptr, nullptr, 0, 0, del, nullptr, nullptr);
         }
         else
         {
            auto v_in_data = gbbs::new_array_no_init<vertex_data>(n);
            auto v_out_data = gbbs::new_array_no_init<vertex_data>(n);
            parallel_for(0, n, [&](size_t i) {
               v_in_data[i].offset = 0;
               v_in_data[i].degree = 0;

               v_out_data[i].offset = 0;
               v_out_data[i].degree = 0;
            });
            return asymmetric_graph<asymmetric_vertex, Wgh>(
                v_out_data, v_in_data, n, 0,
                [=]() {
                   gbbs::free_array(v_out_data, n);
                   gbbs::free_array(v_in_data, n);
                },
                nullptr, nullptr);
         }
      }

      // flip to create the in-edges
      auto I = sequence<typename EdgeSeq::value_type>::from_function(A.size(), [&](size_t i) {
         using T = typename EdgeSeq::value_type;
         auto e = A[i];
         return T(get_v(e), get_u(e), get_w(e));
      });

      if(!is_sorted)
      {
         parlay::integer_sort_inplace(make_slice(A), get_u);
         parlay::integer_sort_inplace(make_slice(I), get_u);
      }

      auto in_starts = sequence<uintT>(n + 1, (uintT)0);
      auto out_starts = sequence<uintT>(n + 1, (uintT)0);

      auto in_edges = get_edges<Wgh>(I, in_starts, m, get_u, get_v, get_w);
      auto out_edges = get_edges<Wgh>(A, out_starts, m, get_u, get_v, get_w);

      auto in_v_data = gbbs::new_array_no_init<vertex_data>(n);
      auto out_v_data = gbbs::new_array_no_init<vertex_data>(n);
      parallel_for(
          0, n,
          [&](size_t i) {
             uintT in_o = in_starts[i];
             in_v_data[i].offset = in_o;
             in_v_data[i].degree = (uintE)(((i == (n - 1)) ? m : in_starts[i + 1]) - in_o);

             uintT out_o = out_starts[i];
             out_v_data[i].offset = out_o;
             out_v_data[i].degree = (uintE)(((i == (n - 1)) ? m : out_starts[i + 1]) - out_o);
          },
          1024);
      return asymmetric_graph<asymmetric_vertex, Wgh>(
          out_v_data, in_v_data, n, m,
          [=]() {
             gbbs::free_array(in_v_data, n);
             gbbs::free_array(out_v_data, n);
             gbbs::free_array(in_edges, m);
             gbbs::free_array(out_edges, m);
          },
          (edge_type*)out_edges, (edge_type*)in_edges);
   }

   template <class Wgh>
   static inline asymmetric_graph<asymmetric_vertex, Wgh>
   asym_graph_from_edges(sequence<std::tuple<uintE, uintE, Wgh>>& A, size_t n, bool is_sorted = false)
   {
      using edge = std::tuple<uintE, uintE, Wgh>;
      auto get_u = [&](const edge& e) { return std::get<0>(e); };
      auto get_v = [&](const edge& e) { return std::get<1>(e); };
      auto get_w = [&](const edge& e) { return std::get<2>(e); };
      return asym_graph_from_edges<Wgh>(A, n, get_u, get_v, get_w, is_sorted);
   }

} // namespace gbbs
