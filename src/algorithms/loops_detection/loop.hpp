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
 * @file loop.hpp
 * @brief Loop data structure.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef LOOP_HPP
#define LOOP_HPP

#include "basic_block.hpp"
#include "ir_basic_block.hpp"
#include "refcount.hpp"
#include <cstddef>
#include <cstdio>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

template <typename Graph>
struct DefaultLoopTraits
{
   using GraphType = Graph;
   using Vertex = typename GraphType::vertex_descriptor;

   static unsigned loopId(const GraphType&, Vertex v)
   {
      return static_cast<unsigned>(v);
   }

   static std::string nodeLabel(const GraphType&, Vertex v)
   {
      return std::string("V") + std::to_string(static_cast<unsigned>(v));
   }
};

struct BBGraphTraits
{
   using GraphType = BBGraph;
   using Vertex = BBGraph::vertex_descriptor;

   static unsigned loopId(const GraphType& graph, Vertex v)
   {
      return graph.CGetNodeInfo(v).block->number;
   }

   static std::string nodeLabel(const GraphType& graph, Vertex v)
   {
      return std::string("BB") + std::to_string(loopId(graph, v));
   }
};

template <typename GraphTraits>
/**
 * @brief Graph-agnostic representation of a loop.
 *
 * The loop is identified by a single entry header that dominates every block in
 * the strongly connected region, while `blocks` collects those basic blocks and
 * `backEdges` captures the latch-to-header edges. Parent/child relationships
 * model the loop forest so callers can walk from outer to inner loops.
 *
 * When a loop does not satisfy the single-header property (irreducible control
 * flow), the default constructor records a synthetic identifier and leaves the
 * header null. This allows users to distinguish reducible from irreducible loops.
 */
class LoopTemplate
{
 private:
   using Graph = typename GraphTraits::GraphType;
   using Self = LoopTemplate<GraphTraits>;
   using Vertex = typename GraphTraits::Vertex;
   using LoopRef = refcount<Self>;
   using LoopConstRef = refcount<const Self>;

   template <typename, typename, typename>
   friend class LoopsTemplate;

   /// Parent loop
   LoopRef parentLoop;

   /// sub loops
   std::vector<LoopConstRef> subLoops;

   /// Blocks which belong to this loop
   std::set<Vertex> blocks;

   /// the header of the loop
   Vertex headerBlock;

   /// the id of the loop
   unsigned int loopId;

   /// used to label irreducible loops
   static unsigned int currUnusedIrreducibleId;

   /// set of vertex pairs describing back edge for the loop
   std::set<std::pair<Vertex, Vertex>> backEdges;

   /// Nesting depth of this loop
   unsigned int depth;

 public:
   using LoopRefType = LoopRef;
   using LoopConstRefType = LoopConstRef;

   /**
    * Constructor for empty loop (used for irreducible)
    */
   explicit LoopTemplate()
       : parentLoop(),
         subLoops(),
         blocks(),
         headerBlock(GraphTraits::GraphType::null_vertex()),
         loopId(currUnusedIrreducibleId--),
         backEdges(),
         depth(0)
   {
   }

   /**
    * Constructor for reducible loop
    * @param g is the control flow graph
    * @param _headerBlock is the header vertex
    */
   LoopTemplate(const Graph& g, Vertex _headerBlock)
       : parentLoop(),
         subLoops(),
         blocks(),
         headerBlock(_headerBlock),
         loopId(GraphTraits::loopId(g, _headerBlock)),
         backEdges(),
         depth(0)
   {
      addBasicBlock(_headerBlock);
   }

   /**
    * returns the loop id
    * @return the loop id
    */
   unsigned int getLoopId() const
   {
      return loopId;
   }

   /**
    * tells if the loop is innermost
    * @return true if the loop is innermost
    */
   bool isInnermost() const
   {
      return subLoops.empty();
   }

   /**
    * tells if the loop is reducible
    * @return true if the loop is reducible
    */
   bool isReducible() const
   {
      return headerBlock != GraphTraits::GraphType::null_vertex();
   }

   /**
    * tells if the loop is pipelinable
    * @return true if the loop is pipelinable
    */
   bool isPipelinable()
   {
      return isInnermost() && numBlocks() == 1;
   }

   /**
    * returns loop header
    * @return returns loop header
    */
   Vertex getHeader() const
   {
      return headerBlock;
   }

   /**
    * returns the parent loop
    * @return the parent loop if this loop is nested, NULL otherwise
    */
   LoopRef getParent() const
   {
      return parentLoop;
   }

   /**
    * Returns nesting depth in the loop forest.
    */
   unsigned int getLoopDepth() const
   {
      return depth;
   }

   /**
    * adds a basic block to this loop.
    * @param block is the vertex to be added
    */
   void addBasicBlock(Vertex block)
   {
      blocks.insert(block);
   }

   /**
    * Tells whether the block belongs to this loop (directly or through a child loop).
    */
   bool contains(Vertex block) const
   {
      if(blocks.find(block) != blocks.end())
      {
         return true;
      }

      for(const auto& child : subLoops)
      {
         if(child->contains(block))
         {
            return true;
         }
      }
      return false;
   }

   /**
    * Tells whether the given loop is nested inside this loop (including itself).
    */
   bool contains(const Self* loop) const
   {
      if(this == loop)
      {
         return true;
      }

      for(const auto& child : subLoops)
      {
         if(child.get() == loop || child->contains(loop))
         {
            return true;
         }
      }
      return false;
   }

   /**
    * returns the number of vertices belonging to this loop
    * @return the number of vertices
    */
   size_t numBlocks() const
   {
      return blocks.size();
   }

   /**
    * returns the vertices
    * @return the vertices
    */
   const std::set<Vertex>& getBlocks() const
   {
      return blocks;
   }

   /**
    * Sets parent for this loop
    */
   void setParentLoop(LoopRef parent)
   {
      parentLoop = LoopRef(parent.get(), null_deleter());
   }

   /**
    * Adds a child loop
    */
   void addChildLoop(LoopRef child)
   {
      for(auto l : subLoops)
      {
         if(l->getLoopId() == child->getLoopId())
         {
            return;
         }
      }
      subLoops.push_back(child);
   }

   /**
    * Returns the sub loops of this loop in the loop forest
    */
   const std::vector<LoopConstRef>& getSubLoops() const
   {
      return subLoops;
   }

   /**
    * Returns the vertices which belong to this loop and to loop nested in this loop
    */
   void collectBlocksRecursively(std::set<Vertex>& ret) const
   {
      ret.insert(blocks.begin(), blocks.end());
      for(const auto& child : subLoops)
      {
         child->collectBlocksRecursively(ret);
      }
   }

   /// return the list of tree back edges
   const std::set<std::pair<Vertex, Vertex>>& getBackEdges() const
   {
      return backEdges;
   }
};

template <typename Graph, typename GraphTraits = DefaultLoopTraits<Graph>>
using LoopT = LoopTemplate<GraphTraits>;

using Loop = LoopTemplate<BBGraphTraits>;
using LoopRef = refcount<Loop>;
using LoopConstRef = refcount<const Loop>;

template <typename GraphTraits>
unsigned int LoopTemplate<GraphTraits>::currUnusedIrreducibleId = std::numeric_limits<unsigned int>::max();

#endif // LOOP_HPP
