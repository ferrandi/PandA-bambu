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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file loop.cpp
 * @brief implementation of the loop representation
 * @author Marco Garatti <m.garatti@gmail.com>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "loop.hpp"

#include "basic_block.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp" // for BBGraphRef
#include "op_graph.hpp"
#include "tree_basic_block.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/tuple/tuple.hpp> // for tie
#include <iosfwd>
#include <limits>

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Weffc++"

unsigned int Loop::curr_unused_irreducible_id = std::numeric_limits<unsigned int>::max();

Loop::Loop(const BBGraphRef _g)
    : g(_g),
      is_innermost_loop(false),
      parent_loop(),
      children(),
      blocks(),
      exits(),
      landing_pads(),
      primary_landing_pad_block(NULL_VERTEX),
      header_block(NULL_VERTEX),
      alternative_entries(),
      loop_id(curr_unused_irreducible_id--),
      exit_landing_association(),
      sp_back_edges(),
      depth(0),
      loop_type(UNKNOWN_LOOP),
      footprint_size(0),
      instruction_size(0),
      body_start(NULL_VERTEX),
      main_iv(0),
      initialization_tree_node_id(0),
      init_gimple_id(0),
      inc_id(0),
      increment(0),
      increment_tn(),
      lower_bound(0),
      upper_bound(0),
      upper_bound_tn(),
      close_interval(false)
{
}

Loop::Loop(const BBGraphRef _bb_graph, vertex _header_block)
    : g(_bb_graph),
      is_innermost_loop(false),
      parent_loop(),
      children(),
      blocks(),
      exits(),
      landing_pads(),
      primary_landing_pad_block(NULL_VERTEX),
      header_block(_header_block),
      alternative_entries(),
      loop_id(0),
      exit_landing_association(),
      sp_back_edges(),
      depth(0),
      loop_type(UNKNOWN_LOOP),
      footprint_size(0),
      instruction_size(0),
      body_start(NULL_VERTEX),
      main_iv(0),
      initialization_tree_node_id(0),
      init_gimple_id(0),
      inc_id(0),
      increment(0),
      increment_tn(),
      lower_bound(0),
      upper_bound(0),
      upper_bound_tn(),
      close_interval(false)
{
   const BBNodeInfoConstRef bb_node_info = g->CGetBBNodeInfo(_header_block);
   loop_id = bb_node_info->block->number;
   add_block(_header_block);
   alternative_entries.insert(_header_block);
}

unsigned int Loop::GetId() const
{
   return loop_id;
}

vertex Loop::GetHeader() const
{
   return header_block;
}

bool Loop::is_innermost() const
{
   return (children.size() == 0);
}

bool Loop::IsReducible() const
{
   return header_block != NULL_VERTEX;
}

size_t Loop::num_blocks() const
{
   return blocks.size();
}

const CustomUnorderedSet<vertex>& Loop::get_blocks() const
{
   return blocks;
}

size_t Loop::num_exits() const
{
   return exits.size();
}

std::list<vertex>::const_iterator Loop::exit_block_iter_begin() const
{
   return exits.begin();
}

std::list<vertex>::const_iterator Loop::exit_block_iter_end() const
{
   return exits.end();
}

size_t Loop::num_landing_pads() const
{
   return landing_pads.size();
}

const CustomUnorderedSet<vertex> Loop::GetLandingPadBlocks() const
{
   return landing_pads;
}

vertex Loop::primary_landing_pad() const
{
   CustomUnorderedSet<vertex>::const_iterator lp_iter, lp_end = landing_pads.end();
   vertex candidate = NULL_VERTEX;
   bool first = true;
   for(lp_iter = landing_pads.begin(); lp_iter != lp_end; ++lp_iter)
   {
      vertex lp = *lp_iter;

      if(candidate == NULL_VERTEX)
      {
         candidate = lp;
         continue;
      }

      graph::out_edge_iterator e_out_iter, e_out_iter_end;
      vertex test_candidate = NULL_VERTEX;
      for(boost::tie(e_out_iter, e_out_iter_end) = boost::out_edges(lp, *g); e_out_iter != e_out_iter_end; ++e_out_iter)
      {
         if(test_candidate == NULL_VERTEX)
            test_candidate = boost::target(*e_out_iter, *g);
         else
            test_candidate = NULL_VERTEX;
      }
      if(test_candidate != candidate)
      {
         if(first)
         {
            // If candidate has a single successor which is test_candidate, then test_candidate is our
            // new candidate
            graph::out_edge_iterator e_iter, e_iter_end;
            for(boost::tie(e_iter, e_iter_end) = boost::out_edges(candidate, *g); e_iter != e_iter_end; ++e_iter)
            {
               if(boost::target(*e_out_iter, *g) != test_candidate)
                  return NULL_VERTEX;
            }
         }
         else
            return NULL_VERTEX;
      }
   }
   return candidate;
}

const LoopRef Loop::Parent() const
{
   return parent_loop;
}

void Loop::SetParent(LoopRef _parent)
{
   parent_loop = LoopRef(_parent.get(), null_deleter());
}

void Loop::ComputeLandingPadExits()
{
   exits.clear();
   landing_pads.clear();

   CustomUnorderedSet<vertex> belonging;
   get_recursively_bb(belonging);

   CustomUnorderedSet<vertex>::iterator source, source_end;
   source_end = belonging.end();
   for(source = belonging.begin(); source != source_end; ++source)
   {
      vertex block = *source;
      graph::out_edge_iterator e_out_iter, e_out_iter_end;
      for(boost::tie(e_out_iter, e_out_iter_end) = boost::out_edges(block, *g); e_out_iter != e_out_iter_end; ++e_out_iter)
      {
         vertex target = boost::target(*e_out_iter, *g);
         // If target does not belong to this loop, then this is an exit block
         if(belonging.find(target) == belonging.end())
         {
            // This is an exit block.
            exits.push_back(block);
            // Update the landing pad info
            landing_pads.insert(target);

            // setting the map exit_landing_association
            exit_landing_association[block].insert(target);
         }
      }
   }
}

void Loop::add_block(vertex block)
{
   blocks.insert(block);
}

void Loop::AddChild(LoopRef child)
{
   children.insert(child);
}

void Loop::get_recursively_bb(CustomUnorderedSet<vertex>& ret) const
{
   ret.insert(blocks.begin(), blocks.end());
   CustomOrderedSet<LoopConstRef>::const_iterator child, child_end = children.end();
   for(child = children.begin(); child != child_end; ++child)
      (*child)->get_recursively_bb(ret);
}

OpVertexSet Loop::GetRecursivelyOps(const OpGraphConstRef op_graph) const
{
   THROW_ASSERT(boost::num_vertices(*op_graph), "Operation graph not yet built");
   OpVertexSet ret(op_graph);
   CustomUnorderedSet<vertex> bb_vertices;
   get_recursively_bb(bb_vertices);
   CustomUnorderedSet<vertex>::const_iterator it, it_end = bb_vertices.end();
   for(it = bb_vertices.begin(); it != it_end; ++it)
   {
      const BBNodeInfoConstRef bb_node_info = g->CGetBBNodeInfo(*it);
      ret.insert(bb_node_info->statements_list.begin(), bb_node_info->statements_list.end());
   }
   return ret;
}

const std::map<vertex, CustomUnorderedSet<vertex>>& Loop::get_exit_landing_association() const
{
   return exit_landing_association;
}

const CustomOrderedSet<LoopConstRef>& Loop::GetChildren() const
{
   return children;
}
#pragma GCC diagnostic pop
