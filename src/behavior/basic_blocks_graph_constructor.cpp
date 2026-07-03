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
 * @file basic_blocks_graph_constructor.cpp
 * @brief This class provides methods to build a basic blocks graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "basic_blocks_graph_constructor.hpp"

#include "cdfg_edge_info.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "string_manipulation.hpp"

#include <boost/tuple/tuple.hpp>

#include <cstddef>
#include <string>
#include <utility>

BasicBlocksGraphConstructor::BasicBlocksGraphConstructor(BBGraphsCollection& _bg)
    : bg(_bg), bb_index_map(_bg.GetGraphInfo().bb_index_map)
{
}

BBGraph::vertex_descriptor BasicBlocksGraphConstructor::add_vertex(blocRef info)
{
   auto index = bg.num_vertices();
   auto v = bg.AddVertex();
   if(index == 0)
   {
      THROW_ASSERT(index == BB_ENTRY, "wrong value of the BB_ENTRY constant");
      bg.GetGraphInfo().entry_vertex = v;
   }
   else if(index == 1)
   {
      THROW_ASSERT(index == BB_EXIT, "wrong value of the BB_EXIT constant");
      bg.GetGraphInfo().exit_vertex = v;
   }
   bg.GetNodeInfo(v).block = info;
   bb_index_map[info->number] = v;
   return v;
}

BBGraph::edge_descriptor BasicBlocksGraphConstructor::AddEdge(BBGraph::vertex_descriptor source,
                                                              BBGraph::vertex_descriptor target, int selector)
{
   return bg.AddEdge(source, target, selector);
}

void BasicBlocksGraphConstructor::Clear()
{
   bg.clear();
   bb_index_map.clear();
}

void BasicBlocksGraphConstructor::RemoveEdge(BBGraph::vertex_descriptor source, BBGraph::vertex_descriptor target,
                                             const int selector)
{
   bg.RemoveSelector(source, target, selector);
}

void BasicBlocksGraphConstructor::RemoveEdge(const BBGraph::edge_descriptor& edge, int selector)
{
   bg.RemoveSelector(edge, selector);
}

void BasicBlocksGraphConstructor::add_bb_edge_info(BBGraph::vertex_descriptor source, BBGraph::vertex_descriptor target,
                                                   int type, const unsigned label)
{
   const auto [e, found] = boost::edge(source, target, bg);
   THROW_ASSERT(found, "Edge BB" + STR(bg.CGetNodeInfo(source).block->number) + "-->BB" +
                           STR(bg.CGetNodeInfo(target).block->number) + " doesn't exists");
   THROW_ASSERT(type & (CFG_SELECTOR | FB_CFG_SELECTOR | CDG_SELECTOR | FB_CDG_SELECTOR), "Not supported label type");
   bg.GetEdgeInfo(e).labels[type].insert(label);
}

BBGraph::edge_descriptor BasicBlocksGraphConstructor::connect_to_exit(BBGraph::vertex_descriptor source)
{
   return AddEdge(source, bg.GetGraphInfo().exit_vertex, CFG_SELECTOR);
}

BBGraph::edge_descriptor BasicBlocksGraphConstructor::connect_to_entry(BBGraph::vertex_descriptor target)
{
   return AddEdge(bg.GetGraphInfo().entry_vertex, target, CFG_SELECTOR);
}

bool BasicBlocksGraphConstructor::check_vertex(unsigned int block_index) const
{
   return bb_index_map.find(block_index) != bb_index_map.end();
}

BBGraph::vertex_descriptor BasicBlocksGraphConstructor::Cget_vertex(unsigned int block_index) const
{
   THROW_ASSERT(bb_index_map.find(block_index) != bb_index_map.end(),
                "this vertex does not exist " + std::to_string(block_index));
   return bb_index_map.find(block_index)->second;
}

void BasicBlocksGraphConstructor::add_operation_to_bb(BBGraph::vertex_descriptor op, unsigned int index)
{
   bg.GetNodeInfo(Cget_vertex(index)).statements_list.push_back(op);
}
