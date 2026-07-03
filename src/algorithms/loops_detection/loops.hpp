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
 * @file loops.hpp
 * @brief Data structure describing and computing loops.
 * Use Havlak’s *Loop-Nesting Forest* to represent both reducible and irreducible cycles.
 * See:
 *     Havlak, P. "Nesting of reducible and irreducible loops".
 *     ACM Trans. Program. Lang. Syst. 19, 4 (July 1997), pp. 557–567
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#ifndef LOOPS_HPP
#define LOOPS_HPP

#include "SemiNCADominance.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "loop.hpp"
#include "loops_fwd.hpp"
#include "refcount.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

class ProfilingInformation;

/**
 * @brief Builds the loop forest for a control-flow graph.
 *
 * The resulting loop hierarchy follows the terminology defined in `LoopTerminology.rst`:
 * each `LoopTemplate` captures a strongly-connected region with a single dominating
 * header (for reducible loops) and records entering backedges, exiting blocks, and
 * parent/child relationships. The algorithm is derived from Havlak’s loop-nesting
 * forest construction and works for reducible and irreducible cycles. Clients
 * can inspect the forest, query loops by identifier.
 *
 * @tparam Graph        CFG type that exposes Boost Graph-style traversal primitives.
 * @tparam GraphTraits  Adapter that maps `Graph` vertices to identifiers/labels.
 * @tparam LoopT        Loop container, defaults to `LoopTemplate<GraphTraits>`.
 */
template <typename Graph, typename GraphTraits, typename LoopT = LoopTemplate<GraphTraits>>
class LoopsTemplate
{
 public:
   using Vertex = typename Graph::vertex_descriptor;
   using LoopImpl = LoopT;
   using LoopRefType = refcount<LoopImpl>;
   using LoopConstRefType = refcount<const LoopImpl>;

 private:
   Graph fbbGraph;
   std::vector<LoopRefType> loopsList;

   LoopsTemplate() = delete;

   /**
    * @brief Create the synthetic root loop containing every basic block.
    *
    * The extra root (depth == 0) mirrors the "loop forest" definition: every
    * real loop becomes a child of this zero loop, ensuring algorithms that
    * assume a single entry (SEME region) can start from a common ancestor.
    */
   void buildZeroLoop(Vertex entryVertex);

   /**
    * @brief Propagate nesting depth to every loop in the subtree.
    *
    * Depth starts from the zero loop and increases by one for each child,
    * matching the definitions of loop depth and loop guard hierarchy in
    * `LoopTerminology.rst`.
    */
   void computeDepth(const LoopConstRefType loop);

   /**
    * @brief Analyse the input CFG and immediately build the loop forest.
    */
   void analyze(dominance<Graph>& domTree, Vertex entryVertex);

 public:
   /**
    * @brief Analyse the input CFG and immediately build the loop forest.
    *
    * @param fbbGraph      Graph augmented with feedback edges (FixIrreducible output).
    * @param entryVertex   Graph entry vertex.
    * @param domTree       Immediate dominator tree over `fbbGraph`.
    */
   LoopsTemplate(Graph fbbGraph, Vertex entryVertex, dominance<Graph>& domTree);

   /**
    * @brief Return the number of loops currently tracked (including the zero loop).
    */
   size_t numLoops() const;

   /**
    * @brief Access the loop list in preorder (zero loop first, then nested loops).
    */
   const std::vector<LoopRefType>& getList() const;

   /**
    * @brief Return a const reference-counted handle given the numeric loop id.
    *
    * Loop identifiers come from `GraphTraits::loopId` and correspond to basic-block
    * numbers for reducible loops or synthetic ids for irreducible regions.
    */
   LoopConstRefType getConstLoop(unsigned int id) const;

   /**
    * @brief Return a mutable reference-counted handle given the numeric loop id.
    */
   LoopRefType getLoop(unsigned int id);

   /**
    * @brief Emit a DOT representation of the loop forest.
    *
    * The output includes loop ids, nesting depth, block membership, optional
    * profiling counters, and edges representing parent/child relationships.
    */
   void
   writeDot(const std::filesystem::path& file_name
#if HAVE_HOST_PROFILING_BUILT
            ,
            const refcount<const ProfilingInformation>& profiling_information = refcount<const ProfilingInformation>()
#endif
   ) const;
};

namespace detail
{
   template <typename Graph, typename LoopRef>
   inline void appendProfilingInfo(std::ostream&, const refcount<const ProfilingInformation>&, const LoopRef&)
   {
   }
} // namespace detail

template <typename Graph, typename GraphTraits, typename LoopT>
LoopsTemplate<Graph, GraphTraits, LoopT>::LoopsTemplate(Graph inputFbbGraph, Vertex inputEntryVertex,
                                                        dominance<Graph>& inputDomTree)
    : fbbGraph(std::move(inputFbbGraph))
{
   analyze(inputDomTree, inputEntryVertex);
   buildZeroLoop(inputEntryVertex);
   if(!loopsList.empty())
   {
      computeDepth(loopsList.front());
   }
}

template <typename Graph, typename GraphTraits, typename LoopT>
void LoopsTemplate<Graph, GraphTraits, LoopT>::analyze(dominance<Graph>& domTree, Vertex entryVertex)
{
   loopsList.clear();
   CustomUnorderedMap<Vertex, LoopRefType> blockToLoop;
   blockToLoop.reserve(fbbGraph.num_vertices());

   // Implementation outline (Havlak loop forest):
   // 1. DFS-number the CFG to classify backedges versus forward edges.
   // 2. Build union-find nodes for each vertex and gather reducible/irreducible predecessors.
   // 3. Traverse vertices in reverse DFS order to materialize LoopTemplate instances,
   //    reconnecting child loops and recording header/latch information.

   const Graph& cfg = fbbGraph;

   static constexpr int UNVISITED = -1;

   enum class LoopType : std::uint8_t
   {
      NonHeader,
      Self,
      Reducible,
      Irreducible,
      Unreachable
   };

   // Disjoint-set node (union-find). Iterative find (path-halving) and a union
   // that can force a preferred root (the loop header) as the set representative.
   struct UnionFindNode
   {
      Vertex block;
      UnionFindNode* parent;
      LoopRefType loop;
      int dfs_number;

      UnionFindNode() : block(Graph::null_vertex()), parent(this), loop(), dfs_number(0)
      {
      }

      void Init(Vertex bb, int number)
      {
         block = bb;
         parent = this;
         loop = LoopRefType();
         dfs_number = number;
      }

      UnionFindNode* FindSet()
      {
         UnionFindNode* node = this;
         UnionFindNode* parent_node = node->parent;
         while(parent_node != node)
         {
            UnionFindNode* grandparent = parent_node->parent;
            node->parent = grandparent;
            node = parent_node;
            parent_node = node->parent;
         }
         return node;
      }

      void Union(UnionFindNode* representative)
      {
         UnionFindNode* root = FindSet();
         UnionFindNode* target = representative->FindSet();
         root->parent = target;
      }

      void UnionPref(UnionFindNode* preferred_root)
      {
         UnionFindNode* preferred_representative = preferred_root->FindSet();
         UnionFindNode* current_representative = FindSet();

         if(preferred_representative != preferred_root)
         {
            preferred_root->parent = preferred_root;
            preferred_representative->parent = preferred_root;
         }

         current_representative->parent = preferred_root;
      }

      void set_loop(const LoopRefType& new_loop)
      {
         loop = new_loop;
      }

      const LoopRefType& get_loop() const
      {
         return loop;
      }
   };

   CustomUnorderedMap<Vertex, int> number;
   number.reserve(cfg.num_vertices());
   for(const auto& vertex : cfg.vertices())
   {
      number.emplace(vertex, UNVISITED);
   }

   std::vector<UnionFindNode> nodes;
   nodes.reserve(number.size());
   std::vector<int> last;
   last.reserve(number.size());
   auto dfs = [&](Vertex root) -> int {
      struct Frame
      {
         Vertex vertex;
         std::vector<Vertex> successors;
         std::size_t next_successor;
         int index;
         int last_id;
      };

      std::vector<Frame> stack;
      stack.reserve(number.size());

      auto push_frame = [&](Vertex v) {
         Frame frame;
         frame.vertex = v;
         frame.next_successor = 0U;
         frame.index = -1;
         frame.last_id = -1;
         for(const auto& edge : cfg.out_edges(v))
         {
            frame.successors.push_back(cfg.target(edge));
         }
         stack.push_back(std::move(frame));
      };

      push_frame(root);
      int root_last = -1;

      while(!stack.empty())
      {
         Frame& frame = stack.back();

         if(frame.index == -1)
         {
            const int index = static_cast<int>(nodes.size());
            nodes.emplace_back();
            nodes.back().Init(frame.vertex, index);
            number[frame.vertex] = index;
            last.push_back(index);
            frame.index = index;
            frame.last_id = index;
         }

         bool pushed_child = false;
         while(frame.next_successor < frame.successors.size())
         {
            const Vertex succ = frame.successors[frame.next_successor++];
            auto it = number.find(succ);
            if(it == number.end())
            {
               continue;
            }
            if(it->second == UNVISITED)
            {
               push_frame(succ);
               pushed_child = true;
               break;
            }
         }

         if(pushed_child)
         {
            continue;
         }

         last[static_cast<std::size_t>(frame.index)] = frame.last_id;
         const int completed_last = frame.last_id;
         stack.pop_back();

         if(!stack.empty())
         {
            Frame& parent = stack.back();
            parent.last_id = completed_last;
         }
         else
         {
            root_last = completed_last;
         }
      }

      return root_last;
   };

   auto entry_it = number.find(entryVertex);
   if(entry_it == number.end())
   {
      return;
   }

   if(entry_it->second == UNVISITED)
   {
      dfs(entryVertex);
   }

   const std::size_t size = nodes.size();
   if(size == 0)
   {
      return;
   }

   auto dominatesByDFS = [&](int ancestor, int node_index) {
      if(ancestor < 0 || node_index < 0)
      {
         return false;
      }
      return ancestor <= node_index && node_index <= last[static_cast<std::size_t>(ancestor)];
   };

   const auto headerVertexForIndex = [&](int index) -> Vertex {
      if(index < 0 || static_cast<std::size_t>(index) >= nodes.size())
      {
         return boost::graph_traits<Graph>::null_vertex();
      }
      return nodes[static_cast<std::size_t>(index)].block;
   };

   auto dominatesHeader = [&](int headerIndex, Vertex candidate) {
      const Vertex headerVertex = headerVertexForIndex(headerIndex);
      if(headerVertex == boost::graph_traits<Graph>::null_vertex())
      {
         const auto numIt = number.find(candidate);
         if(numIt == number.end())
         {
            return false;
         }
         return dominatesByDFS(headerIndex, numIt->second);
      }

      if(candidate == headerVertex)
      {
         return true;
      }

      Vertex current = candidate;
      std::size_t guard = 0;
      while(true)
      {
         Vertex idomVertex = domTree.getImmediateDominator(current);
         if(idomVertex == current)
         {
            break;
         }
         if(idomVertex == headerVertex)
         {
            return true;
         }
         current = idomVertex;
         if(++guard > size)
         {
            break;
         }
      }

      const auto numIt = number.find(candidate);
      if(numIt == number.end())
      {
         return false;
      }
      const int candidateIndex = numIt->second;
      if(candidateIndex == UNVISITED)
      {
         return false;
      }

      return dominatesByDFS(headerIndex, candidateIndex);
   };

   std::vector<CustomUnorderedSet<int>> non_back_preds(size);
   std::vector<std::vector<int>> back_preds(size);
   std::vector<LoopType> block_state(size, LoopType::NonHeader);

   for(std::size_t w = 0; w < size; ++w)
   {
      UnionFindNode& node_w = nodes[w];
      const Vertex block = node_w.block;
      if(block == Graph::null_vertex())
      {
         block_state[w] = LoopType::Unreachable;
         continue;
      }

      for(const auto& edge : cfg.in_edges(block))
      {
         const Vertex pred = cfg.source(edge);
         const auto number_it = number.find(pred);
         if(number_it == number.end())
         {
            continue;
         }
         const int dfsIndex = number_it->second;
         if(dfsIndex == UNVISITED)
         {
            continue;
         }

         const Vertex predecessorVertex = nodes[static_cast<std::size_t>(dfsIndex)].block;
         if(dominatesHeader(static_cast<int>(w), predecessorVertex))
         {
            back_preds[w].push_back(dfsIndex);
         }
         else
         {
            non_back_preds[w].insert(dfsIndex);
         }
      }
   }

   auto attachBlockToLoop = [&](const LoopRefType& loop, Vertex block) {
      if(block == Graph::null_vertex())
      {
         return;
      }

      auto [map_it, inserted] = blockToLoop.emplace(block, loop);
      if(inserted)
      {
         loop->addBasicBlock(block);
         return;
      }

      const LoopRefType& existing = map_it->second;
      if(existing == loop)
      {
         return;
      }

      existing->setParentLoop(loop);
      loop->addChildLoop(existing);
   };

   for(int w = static_cast<int>(size) - 1; w >= 0; --w)
   {
      UnionFindNode& node_w = nodes[static_cast<std::size_t>(w)];
      const Vertex block = node_w.block;
      if(block == Graph::null_vertex())
      {
         continue;
      }

      std::vector<UnionFindNode*> node_pool;
      node_pool.reserve(back_preds[static_cast<std::size_t>(w)].size());
      for(int v : back_preds[static_cast<std::size_t>(w)])
      {
         if(v != w)
         {
            node_pool.push_back(nodes[static_cast<std::size_t>(v)].FindSet());
         }
         else
         {
            block_state[static_cast<std::size_t>(w)] = LoopType::Self;
         }
      }

      std::deque<UnionFindNode*> worklist(node_pool.begin(), node_pool.end());
      if(!node_pool.empty())
      {
         block_state[static_cast<std::size_t>(w)] = LoopType::Reducible;
      }

      std::vector<LoopRefType> discoveredSubLoops;
      discoveredSubLoops.reserve(node_pool.size());

      while(!worklist.empty())
      {
         UnionFindNode* x = worklist.front();
         worklist.pop_front();

         const int x_index = x->dfs_number;
         const auto& preds = non_back_preds[static_cast<std::size_t>(x_index)];
         for(int pred_index : preds)
         {
            UnionFindNode* ydash = nodes[static_cast<std::size_t>(pred_index)].FindSet();
            const Vertex yBlock = ydash->block;

            const auto mappedIt = blockToLoop.find(yBlock);
            if(mappedIt != blockToLoop.end())
            {
               const LoopRefType& mappedLoop = mappedIt->second;
               if(mappedLoop && std::find(discoveredSubLoops.begin(), discoveredSubLoops.end(), mappedLoop) ==
                                    discoveredSubLoops.end())
               {
                  discoveredSubLoops.push_back(mappedLoop);
               }
               if(std::find(node_pool.begin(), node_pool.end(), ydash) == node_pool.end())
               {
                  node_pool.push_back(ydash);
                  worklist.push_back(ydash);
               }
               continue;
            }

            if(!dominatesHeader(w, yBlock))
            {
               block_state[static_cast<std::size_t>(w)] = LoopType::Irreducible;
               non_back_preds[static_cast<std::size_t>(w)].insert(ydash->dfs_number);
            }
            else if(ydash->dfs_number != w)
            {
               if(std::find(node_pool.begin(), node_pool.end(), ydash) == node_pool.end())
               {
                  worklist.push_back(ydash);
                  node_pool.push_back(ydash);
               }
            }
         }
      }

      if(node_pool.empty() && block_state[static_cast<std::size_t>(w)] != LoopType::Self)
      {
         continue;
      }

      const bool reducible = block_state[static_cast<std::size_t>(w)] != LoopType::Irreducible;
      LoopRefType loop = reducible ? LoopRefType(new LoopImpl(cfg, block)) : LoopRefType(new LoopImpl());
      loop->backEdges.clear();

      for(const auto& subLoop : discoveredSubLoops)
      {
         subLoop->setParentLoop(loop);
         loop->addChildLoop(subLoop);
      }

      for(int latch_index : back_preds[static_cast<std::size_t>(w)])
      {
         const Vertex latch = nodes[static_cast<std::size_t>(latch_index)].block;
         if(latch != Graph::null_vertex())
         {
            loop->backEdges.emplace(latch, block);
         }
      }

      node_w.set_loop(loop);
      attachBlockToLoop(loop, block);

      for(UnionFindNode* node : node_pool)
      {
         node->UnionPref(&nodes[static_cast<std::size_t>(w)]);

         const LoopRefType& child_loop = node->get_loop();
         if(child_loop)
         {
            child_loop->setParentLoop(loop);
            loop->addChildLoop(child_loop);
         }
         else
         {
            attachBlockToLoop(loop, node->block);
         }
      }

      loopsList.push_back(loop);
   }
}

template <typename Graph, typename GraphTraits, typename LoopT>
size_t LoopsTemplate<Graph, GraphTraits, LoopT>::numLoops() const
{
   return loopsList.size();
}

template <typename Graph, typename GraphTraits, typename LoopT>
const std::vector<typename LoopsTemplate<Graph, GraphTraits, LoopT>::LoopRefType>&
LoopsTemplate<Graph, GraphTraits, LoopT>::getList() const
{
   return loopsList;
}

template <typename Graph, typename GraphTraits, typename LoopT>
typename LoopsTemplate<Graph, GraphTraits, LoopT>::LoopConstRefType
LoopsTemplate<Graph, GraphTraits, LoopT>::getConstLoop(unsigned int id) const
{
   for(const auto& loop : loopsList)
   {
      if(loop->getLoopId() == id)
      {
         return loop;
      }
   }
   THROW_UNREACHABLE(std::string("Loop with id ") + std::to_string(id) + " doesn't exist");
   return LoopConstRefType();
}

template <typename Graph, typename GraphTraits, typename LoopT>
typename LoopsTemplate<Graph, GraphTraits, LoopT>::LoopRefType
LoopsTemplate<Graph, GraphTraits, LoopT>::getLoop(unsigned int id)
{
   for(const auto& loop : loopsList)
   {
      if(loop->getLoopId() == id)
      {
         return loop;
      }
   }
   THROW_UNREACHABLE(std::string("Loop with id ") + std::to_string(id) + " doesn't exist");
   return LoopRefType();
}

template <typename Graph, typename GraphTraits, typename LoopT>
void LoopsTemplate<Graph, GraphTraits, LoopT>::writeDot(
    const std::filesystem::path& file_name
#if HAVE_HOST_PROFILING_BUILT
    ,
    const refcount<const ProfilingInformation>& profiling_information
#endif
) const
{
   std::filesystem::create_directories(file_name.parent_path());
   std::ofstream dot(file_name);
   THROW_ASSERT(dot.is_open(), "Unable to open DOT file " + file_name.string());
   dot << "digraph LoopForest {" << std::endl;
   for(const auto& loop : loopsList)
   {
      dot << loop->getLoopId() << " [label=\"LoopId=" << loop->getLoopId() << " - Depth: " << loop->getLoopDepth();
      detail::appendProfilingInfo<Graph>(dot, profiling_information, loop);
      dot << "\nType:";
      if(loop->isPipelinable())
      {
         dot << " Pipelinable";
      }
      dot << "\nBlocks:";
      const auto& blocks = loop->getBlocks();
      for(const auto bb : blocks)
      {
         dot << " " << GraphTraits::nodeLabel(fbbGraph, bb);
      }
      dot << "\n\"];" << std::endl;
   }
   for(const auto& loop : loopsList)
   {
      for(const auto& child : loop->getSubLoops())
      {
         dot << loop->getLoopId() << "->" << child->getLoopId() << ";" << std::endl;
      }
   }
   dot << "}" << std::endl;
}

template <typename Graph, typename GraphTraits, typename LoopT>
void LoopsTemplate<Graph, GraphTraits, LoopT>::computeDepth(const LoopConstRefType loop)
{
   for(const auto& child : loop->getSubLoops())
   {
      auto child_loop = std::const_pointer_cast<LoopImpl>(child);
      child_loop->depth = loop->getLoopDepth() + 1;
      computeDepth(child);
   }
}

template <typename Graph, typename GraphTraits, typename LoopT>
void LoopsTemplate<Graph, GraphTraits, LoopT>::buildZeroLoop(Vertex entryVertex)
{
   LoopRefType zero_loop(new LoopImpl(fbbGraph, entryVertex));
   const auto [vit, vit_end] = boost::vertices(fbbGraph);
   zero_loop->blocks.insert(vit, vit_end);

   for(const auto& loop : loopsList)
   {
      if(!loop->getParent())
      {
         loop->parentLoop = zero_loop;
         zero_loop->addChildLoop(loop);
         std::set<Vertex> children_blocks;
         loop->collectBlocksRecursively(children_blocks);
         for(const auto& childBB : children_blocks)
         {
            const auto it = zero_loop->blocks.find(childBB);
            if(it != zero_loop->blocks.end())
            {
               zero_loop->blocks.erase(it);
            }
         }
      }
   }

   loopsList.insert(loopsList.begin(), zero_loop);
}

#endif // LOOPS_HPP
