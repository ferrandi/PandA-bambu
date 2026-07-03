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
 *                Copyright (C) 2025-2026 Politecnico di Milano
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
 * @file SemiNCADominance.hpp
 * @brief Dominator and post-dominator computation
 *
 * This implementation closely follows the Semi-NCA algorithm described in the following paper:
 *   [1] Loukas Georgiadis, "Linear-Time Algorithms for Dominators
 *       and Related Problems", 2005.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SEMINCADOMINANCE_HPP
#define SEMINCADOMINANCE_HPP

#include "custom_map.hpp"
#include "exceptions.hpp"
#include <cstddef>
#include <map>
#include <utility>
#include <vector>

/**
 * Semi-NCA computer parameterised on the graph type and on whether
 * we are computing dominators or post-dominators.
 *
 */
template <typename Graph, bool IsPostDom>
class SemiNCAComputer
{
   using vertex_descriptor = typename Graph::vertex_descriptor;

   // Per-node metadata collected during DFS and reused by the Semi-NCA pass.
   struct InfoRec
   {
      unsigned DFSNum = 0;      // DFS discovery number (0 means unvisited).
      unsigned Parent = 0;      // DFS parent expressed as a DFS number.
      unsigned Semi = 0;        // Current semi-dominator candidate (DFS number).
      unsigned Best = 0;        // Best ancestor seen so far when evaluating predecessors.
      unsigned UnionParent = 0; // Disjoint-set parent used by the eval compression.
      unsigned IDomNum = 0;     // Immediate dominator expressed as DFS number.
      vertex_descriptor IDom = Graph::null_vertex();
      std::vector<unsigned> ReverseChildren; // Predecessors recorded as DFS numbers.
   };

   const Graph& g;
   CustomUnorderedMapStable<vertex_descriptor, vertex_descriptor>& dom_map;

   std::vector<vertex_descriptor> num_to_node;
   CustomUnorderedMapStable<vertex_descriptor, InfoRec> node_infos;
   std::map<vertex_descriptor, size_t> index_map;
   std::vector<vertex_descriptor> roots;

 public:
   SemiNCAComputer(const Graph& graph, vertex_descriptor entry, vertex_descriptor exit,
                   CustomUnorderedMapStable<vertex_descriptor, vertex_descriptor>& out_map)
       : g(graph), dom_map(out_map)
   {
      num_to_node.reserve(64);
      num_to_node.push_back(Graph::null_vertex()); // Slot 0 is dummy.

      size_t idx = 0;
      for(const auto& v : g.vertices())
      {
         index_map[v] = idx++;
      }

      if constexpr(IsPostDom)
      {
         if(exit != Graph::null_vertex())
         {
            roots.push_back(exit);
         }
      }
      else
      {
         roots.push_back(entry);
      }

      calculate();
   }

   SemiNCAComputer(const SemiNCAComputer&) = delete;
   SemiNCAComputer& operator=(const SemiNCAComputer&) = delete;

 private:
   void calculate()
   {
      doFullDFSWalk();
      buildReverseChildren();
      runSemiNCA();
      fillDomMap();
   }

   template <bool Inversed>
   std::vector<vertex_descriptor> getChildren(vertex_descriptor v) const
   {
      std::vector<vertex_descriptor> children;
      if constexpr(Inversed)
      {
         for(const auto& e : g.in_edges(v))
         {
            children.push_back(g.source(e));
         }
      }
      else
      {
         for(const auto& e : g.out_edges(v))
         {
            children.push_back(g.target(e));
         }
      }
      return children;
   }

   // Perform an iterative DFS starting at `start` and assign DFS numbers to reachable nodes.
   template <bool IsReverse>
   unsigned runDFS(vertex_descriptor start, unsigned last_num, unsigned attach_to_num)
   {
      if(start == Graph::null_vertex())
      {
         return last_num;
      }

      std::vector<std::pair<vertex_descriptor, unsigned>> worklist;
      worklist.emplace_back(start, attach_to_num);
      node_infos[start].Parent = attach_to_num;

      while(!worklist.empty())
      {
         const auto [node, parent_num] = worklist.back();
         worklist.pop_back();

         auto& node_info = node_infos[node];

         if(node_info.DFSNum != 0)
         {
            continue;
         }

         node_info.Parent = parent_num;
         node_info.DFSNum = ++last_num;
         node_info.Semi = node_info.DFSNum;
         node_info.Best = node_info.DFSNum;
         node_info.UnionParent = parent_num;
         node_info.IDomNum = parent_num;
         num_to_node.push_back(node);

         constexpr bool Direction = (IsReverse != IsPostDom);
         const auto children = getChildren<Direction>(node);
         for(const auto child : children)
         {
            worklist.emplace_back(child, last_num);
         }
      }

      return last_num;
   }

   void doFullDFSWalk()
   {
      unsigned num = 0;

      for(auto root : roots)
      {
         num = runDFS<false>(root, num, 0);
      }

      if constexpr(IsPostDom)
      {
         for(const auto& node : g.vertices())
         {
            if(node == Graph::null_vertex())
            {
               continue;
            }

            if(node_infos[node].DFSNum != 0)
            {
               continue;
            }

            const unsigned prev_num = num;
            const unsigned provisional_num = runDFS<true>(node, num, num);
            const vertex_descriptor furthest = num_to_node[provisional_num];

            for(unsigned i = provisional_num; i > prev_num; --i)
            {
               const vertex_descriptor temp = num_to_node.back();
               num_to_node.pop_back();
               node_infos[temp] = InfoRec{};
            }

            num = prev_num;
            roots.push_back(furthest);
            num = runDFS<false>(furthest, num, 1);
         }
      }

#ifndef NDEBUG
      for(const auto& node : g.vertices())
      {
         if(node == Graph::null_vertex())
         {
            continue;
         }
         THROW_ASSERT(node_infos[node].DFSNum != 0, "Node not visited during DFS walk");
      }
#endif
   }

   // Build the predecessor lists (expressed as DFS numbers) required by Semi-NCA.
   void buildReverseChildren()
   {
      for(auto& entry : node_infos)
      {
         entry.second.ReverseChildren.clear();
      }

      for(const auto& node : num_to_node)
      {
         if(node == Graph::null_vertex())
         {
            continue;
         }

         auto& node_info = node_infos[node];
         if(node_info.DFSNum == 0)
         {
            continue;
         }

         const auto predecessors = getChildren<!IsPostDom>(node);
         for(const auto& pred : predecessors)
         {
            auto pred_it = node_infos.find(pred);
            if(pred_it == node_infos.end())
            {
               continue;
            }
            const auto& pred_info = pred_it->second;
            if(pred_info.DFSNum == 0)
            {
               continue;
            }
            node_info.ReverseChildren.push_back(pred_info.DFSNum);
         }
      }
   }

   // Core Semi-NCA algorithm.
   void runSemiNCA()
   {
      const auto next_dfs_num = static_cast<unsigned>(num_to_node.size());
      if(next_dfs_num <= 1)
      {
         return;
      }

      std::vector<InfoRec*> num_to_info(next_dfs_num, nullptr);
      // Initialize every visited node with conservative values based on DFS order.
      for(unsigned i = 1; i < next_dfs_num; ++i)
      {
         const auto node = num_to_node[i];
         auto& info = node_infos[node];
         if(info.Parent >= next_dfs_num)
         {
            info.Parent = 0;
         }
         info.Semi = i;
         info.Best = i;
         info.UnionParent = info.Parent;
         info.IDomNum = info.Parent;
         num_to_info[i] = &info;
      }

      // Process nodes in reverse DFS order computing their semi-dominators.
      for(unsigned i = next_dfs_num - 1; i >= 2; --i)
      {
         auto& w_info = *num_to_info[i];
         unsigned semi = i;
         for(const unsigned pred_num : w_info.ReverseChildren)
         {
            if(pred_num == 0 || pred_num >= next_dfs_num)
            {
               continue;
            }
            eval(pred_num, i, num_to_info);
            const unsigned candidate = num_to_info[pred_num]->Best;
            if(candidate < semi)
            {
               semi = candidate;
            }
         }
         w_info.Semi = semi;
         w_info.Best = semi;
         w_info.IDomNum = w_info.Parent;
      }

      // Finalize IDomNum by walking up the semi-dominator buckets.
      for(unsigned i = 2; i < next_dfs_num; ++i)
      {
         auto& w_info = *num_to_info[i];
         while(w_info.IDomNum != 0 && w_info.IDomNum > w_info.Semi)
         {
            w_info.IDomNum = num_to_info[w_info.IDomNum]->IDomNum;
         }
      }

      // Translate DFS-numbered dominators back to graph vertices.
      for(unsigned i = 1; i < next_dfs_num; ++i)
      {
         auto& info = *num_to_info[i];
         if(info.IDomNum == 0)
         {
            info.IDom = Graph::null_vertex();
         }
         else
         {
            info.IDom = num_to_node[info.IDomNum];
         }
      }
   }

   // Union-find `eval` with path compression, returning the canonical ancestor.
   unsigned eval(unsigned v_num, unsigned cur, std::vector<InfoRec*>& num_to_info)
   {
      if(v_num == 0 || v_num <= cur)
      {
         return v_num;
      }

      InfoRec* info = num_to_info[v_num];
      const unsigned parent_num = info->UnionParent;
      const unsigned representative = eval(parent_num, cur, num_to_info);
      if(parent_num != 0)
      {
         InfoRec* parent_info = num_to_info[parent_num];
         if(parent_info->Best < info->Best)
         {
            info->Best = parent_info->Best;
         }
      }
      info->UnionParent = representative;
      return representative;
   }

   // Export the immediate-dominator map using the previously computed InfoRec table.
   void fillDomMap()
   {
      dom_map.clear();

      for(const auto& node : num_to_node)
      {
         if(node == Graph::null_vertex())
         {
            continue;
         }

         const auto& info = node_infos.at(node);
         if(info.IDom != Graph::null_vertex())
         {
            dom_map[node] = info.IDom;
         }
      }

      if(!roots.empty() && roots.front() != Graph::null_vertex())
      {
         dom_map[roots.front()] = roots.front();
      }
   }
};

/**
 * Dominator / post-dominator front-end class.
 */
template <typename Graph, bool ComputePostDominators = false>
class dominance
{
 private:
   using vertex_descriptor = typename Graph::vertex_descriptor;

   CustomUnorderedMapStable<vertex_descriptor, vertex_descriptor> dom;

 public:
   // Construct a dominator tree for `graph` between entry and exit vertices.
   dominance(const Graph& graph, vertex_descriptor entry, vertex_descriptor exit)
   {
      THROW_ASSERT(entry != exit, "incorrect entry and exit basic blocks");
      SemiNCAComputer<Graph, ComputePostDominators> computer(graph, entry, exit, dom);
   }

   dominance(const dominance&) = delete;
   dominance& operator=(const dominance&) = delete;

   vertex_descriptor getImmediateDominator(vertex_descriptor v) const
   {
      auto it = dom.find(v);
      THROW_ASSERT(it != dom.end(), "dominance information not available for requested vertex");
      return it->second;
   }

   template <typename Visitor>
   void forEachDominanceRelation(Visitor&& visitor) const
   {
      for(const auto& [node, idom] : dom)
      {
         visitor(node, idom);
      }
   }
};

#endif // SEMINCADOMINANCE_HPP
