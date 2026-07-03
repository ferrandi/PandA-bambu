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
 *              Copyright (C) 2022-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file sdc_solver.cpp
 * @brief Solver of system of difference constraints based on Bellman-Ford.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "sdc_solver.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <unordered_map>
#include <vector>

namespace
{
   struct Edge
   {
      unsigned source;
      unsigned target;
      int weight;
   };

   struct AdjEdge
   {
      unsigned target;
      int weight;
   };

   struct SolverGraph
   {
      unsigned vertex_count = 0;
      unsigned active_vertex_count = 0;
      unsigned max_original_id = 0;
      bool compact_ids = false;
      std::vector<unsigned> original_ids;
      std::vector<unsigned char> active;
      std::vector<std::size_t> offsets;
      std::vector<AdjEdge> adjacency;
      std::vector<unsigned char> has_negative_outgoing_edge;
   };

   using Distance = long long;
   constexpr unsigned invalid_vertex = std::numeric_limits<unsigned>::max();

   void build_adjacency(const unsigned vertex_count, const std::vector<Edge>& edges, SolverGraph& graph)
   {
      graph.offsets.assign(static_cast<std::size_t>(vertex_count) + 1, 0);
      for(const auto& edge : edges)
      {
         ++graph.offsets[edge.source + 1];
      }
      for(unsigned vertex = 1; vertex <= vertex_count; ++vertex)
      {
         graph.offsets[vertex] += graph.offsets[vertex - 1];
      }

      auto cursor = graph.offsets;
      graph.adjacency.resize(edges.size());
      for(const auto& edge : edges)
      {
         graph.adjacency[cursor[edge.source]++] = AdjEdge{edge.target, edge.weight};
      }
   }

   void build_direct_graph(const std::map<std::pair<unsigned, unsigned>, int>& constraints,
                           const unsigned max_original_id, SolverGraph& graph)
   {
      graph = SolverGraph{};
      graph.vertex_count = max_original_id + 1;
      graph.max_original_id = max_original_id;
      graph.active.assign(graph.vertex_count, 0);
      graph.has_negative_outgoing_edge.assign(graph.vertex_count, 0);

      for(const auto& constraint : constraints)
      {
         const auto source = constraint.first.first;
         const auto target = constraint.first.second;
         if(!graph.active[source])
         {
            graph.active[source] = 1;
            ++graph.active_vertex_count;
         }
         if(!graph.active[target])
         {
            graph.active[target] = 1;
            ++graph.active_vertex_count;
         }
      }

      std::vector<Edge> edges;
      edges.reserve(constraints.size());
      for(const auto& constraint : constraints)
      {
         const auto source = constraint.first.first;
         const auto target = constraint.first.second;
         const auto weight = constraint.second;
         if(source == target)
         {
            continue;
         }
         edges.push_back(Edge{source, target, weight});
         if(weight < 0)
         {
            graph.has_negative_outgoing_edge[source] = 1;
         }
      }

      build_adjacency(graph.vertex_count, edges, graph);
   }

   void build_compact_graph(const std::map<std::pair<unsigned, unsigned>, int>& constraints,
                            const unsigned max_original_id, SolverGraph& graph)
   {
      graph = SolverGraph{};
      graph.max_original_id = max_original_id;
      graph.compact_ids = true;
      graph.original_ids.reserve(constraints.size() * 2);

      for(const auto& constraint : constraints)
      {
         graph.original_ids.push_back(constraint.first.first);
         graph.original_ids.push_back(constraint.first.second);
      }
      std::sort(graph.original_ids.begin(), graph.original_ids.end());
      graph.original_ids.erase(std::unique(graph.original_ids.begin(), graph.original_ids.end()),
                               graph.original_ids.end());

      graph.vertex_count = static_cast<unsigned>(graph.original_ids.size());
      graph.active_vertex_count = graph.vertex_count;
      graph.has_negative_outgoing_edge.assign(graph.vertex_count, 0);

      std::unordered_map<unsigned, unsigned> compact_id;
      compact_id.reserve(graph.original_ids.size() * 2);
      for(unsigned compact = 0; compact < graph.original_ids.size(); ++compact)
      {
         compact_id.emplace(graph.original_ids[compact], compact);
      }

      std::vector<Edge> edges;
      edges.reserve(constraints.size());
      for(const auto& constraint : constraints)
      {
         const auto source = compact_id.at(constraint.first.first);
         const auto target = compact_id.at(constraint.first.second);
         const auto weight = constraint.second;
         if(source == target)
         {
            continue;
         }
         edges.push_back(Edge{source, target, weight});
         if(weight < 0)
         {
            graph.has_negative_outgoing_edge[source] = 1;
         }
      }

      build_adjacency(graph.vertex_count, edges, graph);
   }

   bool build_solver_graph(const std::map<std::pair<unsigned, unsigned>, int>& constraints, SolverGraph& graph)
   {
      graph = SolverGraph{};
      if(constraints.empty())
      {
         return true;
      }

      unsigned max_original_id = 0;
      for(const auto& constraint : constraints)
      {
         const auto source = constraint.first.first;
         const auto target = constraint.first.second;
         const auto weight = constraint.second;
         max_original_id = std::max(max_original_id, source);
         max_original_id = std::max(max_original_id, target);
         if(source == target && weight < 0)
         {
            return false;
         }
      }

      const auto dense_limit = static_cast<unsigned long long>(constraints.size()) * 4ULL + 1024ULL;
      if(static_cast<unsigned long long>(max_original_id) <= dense_limit && max_original_id != invalid_vertex)
      {
         build_direct_graph(constraints, max_original_id, graph);
      }
      else
      {
         build_compact_graph(constraints, max_original_id, graph);
      }
      return true;
   }

   class CircularQueue
   {
      std::vector<unsigned> data;
      unsigned head = 0;
      unsigned tail = 0;
      unsigned count = 0;

    public:
      explicit CircularQueue(const unsigned capacity) : data(capacity == 0 ? 1 : capacity)
      {
      }

      bool empty() const
      {
         return count == 0;
      }

      unsigned front() const
      {
         return data[head];
      }

      void push_back(const unsigned vertex)
      {
         data[tail] = vertex;
         tail = (tail + 1) % static_cast<unsigned>(data.size());
         ++count;
      }

      void push_front(const unsigned vertex)
      {
         head = (head + static_cast<unsigned>(data.size()) - 1) % static_cast<unsigned>(data.size());
         data[head] = vertex;
         ++count;
      }

      unsigned pop_front()
      {
         const auto vertex = data[head];
         head = (head + 1) % static_cast<unsigned>(data.size());
         --count;
         return vertex;
      }
   };

   bool closes_negative_predecessor_cycle(const unsigned target, const unsigned source, const int edge_weight,
                                          const std::vector<unsigned>& predecessor,
                                          const std::vector<int>& predecessor_weight, std::vector<unsigned>& seen,
                                          unsigned& epoch)
   {
      ++epoch;
      if(epoch == 0)
      {
         std::fill(seen.begin(), seen.end(), 0);
         epoch = 1;
      }

      Distance cycle_weight = edge_weight;
      auto vertex = source;
      while(vertex != invalid_vertex)
      {
         if(vertex == target)
         {
            return cycle_weight < 0;
         }
         if(seen[vertex] == epoch)
         {
            return false;
         }
         seen[vertex] = epoch;

         const auto parent = predecessor[vertex];
         if(parent == invalid_vertex)
         {
            return false;
         }
         cycle_weight += static_cast<Distance>(predecessor_weight[vertex]);
         vertex = parent;
      }
      return false;
   }

   std::vector<Distance> solve_graph(const SolverGraph& graph, bool& feasible)
   {
      feasible = true;
      std::vector<Distance> distances(graph.vertex_count, 0);
      if(graph.vertex_count == 0 || graph.active_vertex_count == 0)
      {
         return distances;
      }

      std::vector<unsigned char> in_queue(graph.vertex_count, 0);
      CircularQueue queue(graph.active_vertex_count);

      for(unsigned vertex = 0; vertex < graph.vertex_count; ++vertex)
      {
         if(graph.has_negative_outgoing_edge[vertex])
         {
            queue.push_back(vertex);
            in_queue[vertex] = 1;
         }
      }

      if(queue.empty())
      {
         return distances;
      }

      std::vector<unsigned> predecessor(graph.vertex_count, invalid_vertex);
      std::vector<int> predecessor_weight(graph.vertex_count, 0);
      std::vector<unsigned> path_edge_count(graph.vertex_count, 0);
      std::vector<unsigned> seen(graph.vertex_count, 0);
      unsigned epoch = 0;

      auto push = [&](const unsigned vertex) {
         if(in_queue[vertex])
         {
            return;
         }
         if(!queue.empty() && distances[vertex] < distances[queue.front()])
         {
            queue.push_front(vertex);
         }
         else
         {
            queue.push_back(vertex);
         }
         in_queue[vertex] = 1;
      };

      while(!queue.empty())
      {
         const auto source = queue.pop_front();
         in_queue[source] = 0;
         const auto source_distance = distances[source];

         for(auto edge_index = graph.offsets[source]; edge_index < graph.offsets[source + 1]; ++edge_index)
         {
            const auto& edge = graph.adjacency[edge_index];
            const auto candidate = source_distance + edge.weight;
            if(candidate < distances[edge.target])
            {
               const auto candidate_edge_count = path_edge_count[source] + 1;
               if(candidate_edge_count >= graph.active_vertex_count ||
                  closes_negative_predecessor_cycle(edge.target, source, edge.weight, predecessor, predecessor_weight,
                                                    seen, epoch))
               {
                  feasible = false;
                  return distances;
               }

               distances[edge.target] = candidate;
               predecessor[edge.target] = source;
               predecessor_weight[edge.target] = edge.weight;
               path_edge_count[edge.target] = candidate_edge_count;
               push(edge.target);
            }
         }
      }

      return distances;
   }
} // namespace

bool sdc_solver::solve_SDC_internal(std::map<unsigned int, int>& vals, bool negate_solution)
{
   SolverGraph graph;
   if(!build_solver_graph(constraints, graph))
   {
      return false;
   }

   bool feasible = true;
   const auto distances = solve_graph(graph, feasible);
   if(!feasible)
   {
      return false;
   }

   vals.clear();
   if(graph.vertex_count == 0)
   {
      return true;
   }

   auto hint = vals.end();
   if(graph.compact_ids)
   {
      std::size_t compact = 0;
      for(unsigned original = 0; original <= graph.max_original_id; ++original)
      {
         int value = 0;
         if(compact < graph.original_ids.size() && graph.original_ids[compact] == original)
         {
            const auto assigned_distance = negate_solution ? -distances[compact] : distances[compact];
            value = static_cast<int>(assigned_distance);
            ++compact;
         }
         hint = vals.emplace_hint(hint, original, value);
         if(original == std::numeric_limits<unsigned>::max())
         {
            break;
         }
      }
   }
   else
   {
      for(unsigned original = 0; original <= graph.max_original_id; ++original)
      {
         int value = 0;
         if(graph.active[original])
         {
            const auto assigned_distance = negate_solution ? -distances[original] : distances[original];
            value = static_cast<int>(assigned_distance);
         }
         hint = vals.emplace_hint(hint, original, value);
         if(original == std::numeric_limits<unsigned>::max())
         {
            break;
         }
      }
   }
   return true;
}

bool sdc_solver::solve_SDCNeg(std::map<unsigned int, int>& vals)
{
   return solve_SDC_internal(vals, true);
}

bool sdc_solver::solve_SDC(std::map<unsigned int, int>& vals)
{
   return solve_SDC_internal(vals, false);
}
