/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file BasicBlockReachability.hpp
 * @brief helper function for BB reachability.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef BASICBLOCKREACHABILITY_HPP
#define BASICBLOCKREACHABILITY_HPP

#include "custom_set.hpp"

#include <deque>

namespace reachability
{
   /**
    * Simple breadth-first reachability over a generic PandA graph.
    * The traversal considers only edges that belong to the provided graph
    * (which may already be filtered by selector).
    *
    * @tparam GraphT a PandA graph type exposing vertex_descriptor, out_edges, target and IsReachable helpers.
    * @param graph the graph to explore (typically a BBGraph or one of its filtered variants).
    * @param source starting vertex.
    * @param target ending vertex.
    * @return true when there exists a non-empty path from @p source to @p target following the edges of @p graph.
    */
   template <typename GraphT>
   bool HasPath(const GraphT& graph, typename GraphT::vertex_descriptor source,
                typename GraphT::vertex_descriptor target)
   {
      if(source == target && graph.out_degree(source) == 0)
      {
         return false;
      }

      std::deque<typename GraphT::vertex_descriptor> queue;
      CustomUnorderedSet<typename GraphT::vertex_descriptor> visited;

      queue.push_back(source);
      visited.insert(source);

      while(!queue.empty())
      {
         const auto current = queue.front();
         queue.pop_front();

         for(const auto& edge : graph.out_edges(current))
         {
            const auto next = graph.target(edge);
            if(next == target)
            {
               return true;
            }
            if(visited.insert(next).second)
            {
               queue.push_back(next);
            }
         }
      }
      return false;
   }

   /**
    * Convenience helper checking for the existence of a cycle passing through @p vertex.
    */
   template <typename GraphT>
   bool HasCycleThrough(const GraphT& graph, typename GraphT::vertex_descriptor vertex)
   {
      return HasPath(graph, vertex, vertex);
   }

} // namespace reachability

#endif // BASICBLOCKREACHABILITY_HPP
