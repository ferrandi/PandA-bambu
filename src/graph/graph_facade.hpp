/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2026 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file graph_facade.hpp
 * @brief PandA-owned facade helpers for graph queries, views and backend capabilities.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef GRAPH_FACADE_HPP
#define GRAPH_FACADE_HPP

#include "graph.hpp"

#include <boost/graph/incremental_components.hpp>
#include <boost/graph/sequential_vertex_coloring.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

template <typename Iterator>
class graph_iterator_range
{
 private:
   Iterator begin_iterator;
   Iterator end_iterator;

 public:
   graph_iterator_range(Iterator _begin_iterator, Iterator _end_iterator)
       : begin_iterator(_begin_iterator), end_iterator(_end_iterator)
   {
   }

   auto begin() const
   {
      return begin_iterator;
   }

   auto end() const
   {
      return end_iterator;
   }

   decltype(auto) front() const
   {
      return *begin_iterator;
   }

   bool empty() const
   {
      return begin_iterator == end_iterator;
   }
};

template <typename Iterator>
auto graph_make_range(Iterator begin_iterator, Iterator end_iterator)
{
   return graph_iterator_range<Iterator>(begin_iterator, end_iterator);
}

template <typename Iterator>
auto graph_make_range(const std::pair<Iterator, Iterator>& iterators)
{
   return graph_make_range(iterators.first, iterators.second);
}

template <typename T>
struct is_std_pair : std::false_type
{
};

template <typename T1, typename T2>
struct is_std_pair<std::pair<T1, T2>> : std::true_type
{
};

template <typename GraphLike, typename = void>
struct graph_has_storage_traits : std::false_type
{
};

template <typename GraphLike>
struct graph_has_storage_traits<GraphLike, std::void_t<typename GraphLike::storage_traits>> : std::true_type
{
};

template <typename Range,
          typename = std::enable_if_t<!is_std_pair<std::remove_cv_t<std::remove_reference_t<Range>>>::value>>
auto graph_make_range(Range&& range)
{
   using std::begin;
   using std::end;
   return graph_make_range(begin(range), end(range));
}

template <typename StoragePolicy>
struct graph_backend_capabilities
{
   static constexpr bool supports_vertex_removal = false;
   static constexpr bool supports_edge_removal = false;
   static constexpr bool supports_freeze = false;
   static constexpr bool supports_parallel_readers = false;
};

template <>
struct graph_backend_capabilities<mutable_list_graph_storage>
{
   static constexpr bool supports_vertex_removal = true;
   static constexpr bool supports_edge_removal = true;
   static constexpr bool supports_freeze = false;
   static constexpr bool supports_parallel_readers = false;
};

template <>
struct graph_backend_capabilities<append_only_vec_graph_storage>
{
   static constexpr bool supports_vertex_removal = false;
   static constexpr bool supports_edge_removal = false;
   static constexpr bool supports_freeze = false;
   static constexpr bool supports_parallel_readers = false;
};

template <typename GraphLike>
struct graph_facade_traits
{
   using vertex_descriptor = typename GraphLike::vertex_descriptor;
   using edge_descriptor = typename GraphLike::edge_descriptor;
   using storage_policy = typename GraphLike::storage_policy;
   using storage_traits = typename GraphLike::storage_traits;
};

using graph_no_property = boost::no_property;
using graph_vec_storage = boost::vecS;
using graph_list_storage = boost::listS;
using graph_directed = boost::directedS;
using graph_undirected = boost::undirectedS;
using graph_bidirectional = boost::bidirectionalS;
using graph_vertex_index_tag = boost::vertex_index_t;

template <typename... Args>
using graph_adjacency_list = boost::adjacency_list<Args...>;

template <typename... Args>
using graph_filtered_graph = boost::filtered_graph<Args...>;

template <typename... Args>
using graph_property = boost::property<Args...>;

template <typename... Args>
using graph_adjacency_list_traits = boost::adjacency_list_traits<Args...>;

template <typename GraphLike>
using graph_vertex_descriptor_t = typename boost::graph_traits<GraphLike>::vertex_descriptor;

template <typename GraphLike>
using graph_edge_descriptor_t = typename boost::graph_traits<GraphLike>::edge_descriptor;

template <typename GraphLike>
using graph_vertex_iterator_t = typename boost::graph_traits<GraphLike>::vertex_iterator;

template <typename GraphLike>
using graph_adjacency_iterator_t = typename boost::graph_traits<GraphLike>::adjacency_iterator;

template <typename GraphLike>
using graph_out_edge_iterator_t = typename boost::graph_traits<GraphLike>::out_edge_iterator;

template <typename GraphLike>
using graph_vertices_size_type_t = typename boost::graph_traits<GraphLike>::vertices_size_type;

template <typename GraphLike>
using graph_storage_policy_t = typename graph_facade_traits<GraphLike>::storage_policy;

template <typename GraphLike>
inline constexpr bool graph_supports_vertex_removal_v =
    graph_backend_capabilities<graph_storage_policy_t<GraphLike>>::supports_vertex_removal;

template <typename GraphLike>
inline constexpr bool graph_supports_edge_removal_v =
    graph_backend_capabilities<graph_storage_policy_t<GraphLike>>::supports_edge_removal;

template <typename GraphLike>
inline constexpr bool graph_supports_freeze_v =
    graph_backend_capabilities<graph_storage_policy_t<GraphLike>>::supports_freeze;

template <typename GraphLike>
inline constexpr bool graph_supports_parallel_readers_v =
    graph_backend_capabilities<graph_storage_policy_t<GraphLike>>::supports_parallel_readers;

template <typename GraphLike>
constexpr auto graph_null_vertex() -> graph_vertex_descriptor_t<GraphLike>
{
   return graph_facade_traits<GraphLike>::storage_traits::null_vertex();
}

template <typename GraphLike>
constexpr auto graph_null_vertex(const GraphLike&) -> graph_vertex_descriptor_t<GraphLike>
{
   if constexpr(graph_has_storage_traits<GraphLike>::value)
   {
      return graph_facade_traits<GraphLike>::storage_traits::null_vertex();
   }
   else
   {
      return boost::graph_traits<GraphLike>::null_vertex();
   }
}

template <typename GraphLike>
auto graph_vertices(const GraphLike& graph)
{
   if constexpr(requires { graph.vertices(); })
   {
      return graph_make_range(graph.vertices());
   }
   else
   {
      return graph_make_range(boost::vertices(graph));
   }
}

template <typename GraphLike>
auto graph_edges(const GraphLike& graph)
{
   if constexpr(requires { graph.edges(); })
   {
      return graph_make_range(graph.edges());
   }
   else
   {
      return graph_make_range(boost::edges(graph));
   }
}

template <typename GraphLike>
auto graph_num_vertices(const GraphLike& graph)
{
   if constexpr(requires { graph.num_vertices(); })
   {
      return graph.num_vertices();
   }
   else
   {
      return boost::num_vertices(graph);
   }
}

template <typename GraphLike>
auto graph_num_edges(const GraphLike& graph)
{
   if constexpr(requires { graph.num_edges(); })
   {
      return graph.num_edges();
   }
   else
   {
      return boost::num_edges(graph);
   }
}

template <typename GraphLike>
auto graph_in_edges(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor vertex)
{
   if constexpr(requires { graph.in_edges(vertex); })
   {
      return graph_make_range(graph.in_edges(vertex));
   }
   else
   {
      return graph_make_range(boost::in_edges(vertex, graph));
   }
}

template <typename GraphLike>
auto graph_in_degree(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor vertex)
{
   if constexpr(requires { graph.in_degree(vertex); })
   {
      return graph.in_degree(vertex);
   }
   else
   {
      return boost::in_degree(vertex, graph);
   }
}

template <typename GraphLike>
auto graph_out_edges(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor vertex)
{
   if constexpr(requires { graph.out_edges(vertex); })
   {
      return graph_make_range(graph.out_edges(vertex));
   }
   else
   {
      return graph_make_range(boost::out_edges(vertex, graph));
   }
}

template <typename GraphLike>
auto graph_adjacent_vertices(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor vertex)
{
   return graph_make_range(boost::adjacent_vertices(vertex, graph));
}

template <typename GraphLike>
auto graph_out_degree(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor vertex)
{
   if constexpr(requires { graph.out_degree(vertex); })
   {
      return graph.out_degree(vertex);
   }
   else
   {
      return boost::out_degree(vertex, graph);
   }
}

template <typename GraphLike>
auto graph_source(const GraphLike& graph, typename boost::graph_traits<GraphLike>::edge_descriptor edge)
{
   if constexpr(requires { graph.source(edge); })
   {
      return graph.source(edge);
   }
   else
   {
      return boost::source(edge, graph);
   }
}

template <typename GraphLike>
auto graph_target(const GraphLike& graph, typename boost::graph_traits<GraphLike>::edge_descriptor edge)
{
   if constexpr(requires { graph.target(edge); })
   {
      return graph.target(edge);
   }
   else
   {
      return boost::target(edge, graph);
   }
}

template <typename GraphLike>
auto graph_find_edge(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor source,
                     typename boost::graph_traits<GraphLike>::vertex_descriptor target)
{
   return boost::edge(source, target, graph);
}

template <typename GraphLike>
auto graph_vertex(const GraphLike& graph, graph_vertices_size_type_t<GraphLike> index)
{
   return boost::vertex(index, graph);
}

template <typename GraphLike>
auto graph_degree(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor vertex)
{
   return boost::degree(vertex, graph);
}

template <typename GraphLike>
auto graph_add_edge(typename boost::graph_traits<GraphLike>::vertex_descriptor source,
                    typename boost::graph_traits<GraphLike>::vertex_descriptor target, GraphLike& graph)
{
   return boost::add_edge(source, target, graph);
}

template <typename GraphLike, typename EdgeProperty>
auto graph_add_edge(typename boost::graph_traits<GraphLike>::vertex_descriptor source,
                    typename boost::graph_traits<GraphLike>::vertex_descriptor target, const EdgeProperty& property,
                    GraphLike& graph)
{
   return boost::add_edge(source, target, property, graph);
}

template <typename GraphLike>
void graph_remove_edge(typename boost::graph_traits<GraphLike>::edge_descriptor edge, GraphLike& graph)
{
   boost::remove_edge(edge, graph);
}

template <typename GraphLike>
void graph_remove_edge(typename boost::graph_traits<GraphLike>::vertex_descriptor source,
                       typename boost::graph_traits<GraphLike>::vertex_descriptor target, GraphLike& graph)
{
   boost::remove_edge(source, target, graph);
}

template <typename GraphLike>
bool graph_exists_edge(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor source,
                       typename boost::graph_traits<GraphLike>::vertex_descriptor target)
{
   return graph_find_edge(graph, source, target).second;
}

template <typename GraphLike>
auto graph_get_edge(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor source,
                    typename boost::graph_traits<GraphLike>::vertex_descriptor target) ->
    typename boost::graph_traits<GraphLike>::edge_descriptor
{
   const auto [edge, found] = graph_find_edge(graph, source, target);
   THROW_ASSERT(found, "Edge does not exist in this graph");
   return edge;
}

template <typename GraphLike>
decltype(auto) graph_node_info(GraphLike& graph, graph_vertex_descriptor_t<GraphLike> vertex)
{
   return graph.GetNodeInfo(vertex);
}

template <typename GraphLike>
decltype(auto) graph_node_info(const GraphLike& graph, graph_vertex_descriptor_t<GraphLike> vertex)
{
   return graph.CGetNodeInfo(vertex);
}

template <typename GraphLike>
decltype(auto) graph_edge_info(GraphLike& graph, graph_edge_descriptor_t<GraphLike> edge)
{
   return graph.GetEdgeInfo(edge);
}

template <typename GraphLike>
decltype(auto) graph_edge_info(const GraphLike& graph, graph_edge_descriptor_t<GraphLike> edge)
{
   return graph.CGetEdgeInfo(edge);
}

template <typename GraphLike>
decltype(auto) graph_graph_info(GraphLike& graph)
{
   return graph.GetGraphInfo();
}

template <typename GraphLike>
decltype(auto) graph_graph_info(const GraphLike& graph)
{
   return graph.CGetGraphInfo();
}

template <typename GraphLike>
auto graph_edge_selector(const GraphLike& graph, graph_edge_descriptor_t<GraphLike> edge)
{
   return graph.GetSelector(edge);
}

template <typename GraphLike>
auto graph_vertex_index(const GraphLike& graph, typename boost::graph_traits<GraphLike>::vertex_descriptor vertex)
{
   return boost::get(boost::vertex_index, graph, vertex);
}

template <typename GraphLike>
auto graph_vertex_index_map(const GraphLike& graph)
{
   return boost::get(boost::vertex_index_t(), graph);
}

template <typename GraphLike>
using graph_vertex_index_map_t = decltype(graph_vertex_index_map(std::declval<const GraphLike&>()));

template <typename GraphLike>
auto graph_identity_vertex_map(const GraphLike&)
{
   using vertex_descriptor = graph_vertex_descriptor_t<GraphLike>;
   return boost::typed_identity_property_map<vertex_descriptor>();
}

template <typename Iterator, typename IndexMap, typename Value>
auto graph_make_indexed_iterator_property_map(Iterator iterator, IndexMap index_map, Value default_value)
{
   return boost::make_iterator_property_map(iterator, index_map, default_value);
}

template <typename GraphLike, typename Iterator, typename Value,
          typename = decltype(boost::get(boost::vertex_index_t(), std::declval<const GraphLike&>()))>
auto graph_make_iterator_property_map(const GraphLike& graph, Iterator iterator, Value default_value)
{
   return graph_make_indexed_iterator_property_map(iterator, graph_vertex_index_map(graph), default_value);
}

using graph_color_value_type = boost::default_color_type;
using graph_default_dfs_visitor = boost::default_dfs_visitor;
using graph_dfs_visitor = boost::dfs_visitor<>;

[[noreturn]] inline void graph_throw_not_a_dag()
{
   BOOST_THROW_EXCEPTION(boost::not_a_dag());
}

constexpr auto graph_color_white() -> graph_color_value_type
{
   return boost::white_color;
}

template <typename IndexMap>
class graph_indexed_color_map
{
 public:
   using color_value_type = graph_color_value_type;
   using storage_type = std::vector<color_value_type>;
   using map_type = decltype(graph_make_indexed_iterator_property_map(
       std::declval<typename storage_type::iterator>(), std::declval<IndexMap>(), std::declval<color_value_type>()));

   graph_indexed_color_map(std::size_t size, IndexMap index_map, color_value_type default_value = graph_color_white())
       : color_storage(size, default_value),
         color_map(graph_make_indexed_iterator_property_map(color_storage.begin(), index_map, default_value))
   {
   }

   auto get() -> map_type&
   {
      return color_map;
   }

   auto get() const -> const map_type&
   {
      return color_map;
   }

 private:
   storage_type color_storage;
   map_type color_map;
};

template <typename GraphLike>
auto graph_make_color_map(const GraphLike& graph, graph_color_value_type default_value = graph_color_white())
{
   return graph_indexed_color_map<decltype(graph_vertex_index_map(graph))>(
       graph_num_vertices(graph), graph_vertex_index_map(graph), default_value);
}

template <typename IndexMap>
auto graph_make_indexed_color_map(std::size_t size, IndexMap index_map,
                                  graph_color_value_type default_value = graph_color_white())
{
   return graph_indexed_color_map<IndexMap>(size, index_map, default_value);
}

template <typename GraphLike>
bool graph_is_reachable(const GraphLike& graph, graph_vertex_descriptor_t<GraphLike> source,
                        graph_vertex_descriptor_t<GraphLike> target)
{
   return graph.IsReachable(source, target);
}

template <typename GraphLike>
auto graph_strongly_connected_components(const GraphLike& graph)
{
   return graph.GetStronglyConnectedComponents();
}

template <typename GraphLike, typename Container>
void graph_topological_sort(const GraphLike& graph, Container& sorted_vertices)
{
   graph.TopologicalSort(sorted_vertices);
}

template <typename GraphLike, typename Container>
void graph_reverse_topological_sort(const GraphLike& graph, Container& sorted_vertices)
{
   graph.ReverseTopologicalSort(sorted_vertices);
}

template <typename GraphLike, typename DisjointSets>
void graph_initialize_incremental_components(const GraphLike& graph, DisjointSets& disjoint_sets)
{
   boost::initialize_incremental_components(graph, disjoint_sets);
}

template <typename GraphLike, typename Vertex, typename Visitor, typename ColorMap>
void graph_depth_first_visit(const GraphLike& graph, Vertex start_vertex, Visitor visitor, ColorMap color_map)
{
   boost::depth_first_visit(graph, start_vertex, visitor, color_map);
}

template <typename GraphLike, typename Vertex, typename Visitor, typename ColorMap, typename TerminatorFunc>
void graph_depth_first_visit(const GraphLike& graph, Vertex start_vertex, Visitor visitor, ColorMap color_map,
                             TerminatorFunc terminator)
{
   boost::depth_first_visit(graph, start_vertex, visitor, color_map, terminator);
}

template <typename GraphLike, typename Params>
void graph_depth_first_search(const GraphLike& graph, const Params& params)
{
   boost::depth_first_search(graph, params);
}

template <typename Visitor>
auto graph_visitor(Visitor&& visitor)
{
   return boost::visitor(std::forward<Visitor>(visitor));
}

template <typename GraphLike, typename OrderPropertyMap, typename ColorPropertyMap>
auto graph_sequential_vertex_coloring(const GraphLike& graph, OrderPropertyMap order, ColorPropertyMap color)
{
   return boost::sequential_vertex_coloring(graph, order, color);
}

template <typename Base, typename VertexPropertiesWriter, typename EdgePropertiesWriter, typename GraphPropertiesWriter>
void graph_write_dot(const graph_base<Base>& graph, const std::filesystem::path& filename, VertexPropertiesWriter vpw,
                     EdgePropertiesWriter epw, GraphPropertiesWriter gpw)
{
   graph.writeDot(filename, vpw, epw, gpw);
}

template <typename Base, typename VertexPropertiesWriter, typename EdgePropertiesWriter>
void graph_write_dot(const graph_base<Base>& graph, const std::filesystem::path& filename, VertexPropertiesWriter vpw,
                     EdgePropertiesWriter epw)
{
   graph.writeDot(filename, vpw, epw);
}

template <typename Base, typename VertexPropertiesWriter>
void graph_write_dot(const graph_base<Base>& graph, const std::filesystem::path& filename, VertexPropertiesWriter vpw)
{
   graph.writeDot(filename, vpw);
}

template <typename GraphView>
GraphView make_graph_view(const typename GraphView::collection_type& collection, int selector)
{
   return GraphView(collection, selector);
}

template <typename GraphView>
GraphView make_graph_view(const typename GraphView::collection_type& collection, int selector,
                          const typename GraphView::subset_type& subset)
{
   return GraphView(collection, selector, subset);
}

#endif
