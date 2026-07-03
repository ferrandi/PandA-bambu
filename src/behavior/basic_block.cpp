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
 * @file basic_block.cpp
 * @brief Class implementation of the basic_block structure.
 *
 * This file implements some of the basic_block member functions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "basic_block.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "ir_basic_block.hpp"
#include "var_pp_functor.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/detail/adjacency_list.hpp>
#include <boost/graph/detail/edge.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <filesystem>
#include <utility>

#if HAVE_HLS_BUILT
#include "allocation_information.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "schedule.hpp"
#endif
#if HAVE_HOST_PROFILING_BUILT
#include "profiling_information.hpp"
#endif

void BBNodeInfo::add_operation_node(const gc_vertex_descriptor op)
{
   statements_list.push_back(op);
}

size_t BBNodeInfo::size() const
{
   return statements_list.size();
}

gc_vertex_descriptor BBNodeInfo::get_first_operation() const
{
   return statements_list.front();
}

gc_vertex_descriptor BBNodeInfo::get_last_operation() const
{
   return statements_list.back();
}

bool BBNodeInfo::empty() const
{
   return statements_list.empty();
}

unsigned int BBNodeInfo::get_bb_index() const
{
   return block->number;
}

const CustomOrderedSet<unsigned int>& BBNodeInfo::getLiveInBbVariables() const
{
   return block->live_in;
}

const CustomOrderedSet<unsigned int>& BBNodeInfo::getLiveOutBbVariables() const
{
   return block->live_out;
}

BBGraphInfo::BBGraphInfo(const application_managerConstRef _AppM, const unsigned int _function_index)
    : AppM(_AppM),
      function_index(_function_index),
      entry_vertex(BBGraph::null_vertex()),
      exit_vertex(BBGraph::null_vertex())
{
}

BBGraph::BBGraph(const BBGraphsCollection& g, int _selector) : graph(g, _selector)
{
}

BBGraph::BBGraph(const BBGraphsCollection& g, int _selector, const CustomUnorderedSet<vertex_descriptor>& sub)
    : graph(g, _selector, sub)
{
}

void BBGraph::writeDot(const std::filesystem::path& file_name, const int detail_level) const
{
   CustomUnorderedSet<vertex_descriptor> annotated;
   writeDot(file_name, annotated, detail_level);
}

void BBGraph::writeDot(const std::filesystem::path& file_name, const CustomUnorderedSet<vertex_descriptor>& annotated,
                       const int) const
{
   BBVertexWriter bb_writer(*this, annotated);
   BBEdgeWriter bb_edge_writer(*this);
   graph::writeDot(file_name, bb_writer, bb_edge_writer);
}

size_t BBGraph::num_bblocks() const
{
   return num_vertices() - 2U;
}

CustomOrderedSet<unsigned int> BBEdgeInfo::get_labels(const int selector) const
{
   if(labels.find(selector) != labels.end())
   {
      return labels.find(selector)->second;
   }
   else
   {
      return CustomOrderedSet<unsigned int>();
   }
}

#if !HAVE_UNORDERED
BBVertexSorter::BBVertexSorter(const BBGraphsCollection* _bb_graph) : bb_graph(_bb_graph)
{
}

bool BBVertexSorter::operator()(BBGraph::vertex_descriptor x, BBGraph::vertex_descriptor y) const
{
   return bb_graph->CGetNodeInfo(x).block->number < bb_graph->CGetNodeInfo(y).block->number;
}

BBEdgeSorter::BBEdgeSorter(const BBGraphsCollection* _bb_graph) : bb_graph(_bb_graph), bb_sorter(_bb_graph)
{
}

bool BBEdgeSorter::operator()(const BBGraph::edge_descriptor& x, const BBGraph::edge_descriptor& y) const
{
   const auto source_x = bb_graph->source(x);
   const auto source_y = bb_graph->source(y);
   if(source_x == source_y)
   {
      return bb_sorter(bb_graph->target(x), bb_graph->target(y));
   }
   else
   {
      return bb_sorter(source_x, source_y);
   }
}
#endif

BBVertexWriter::BBVertexWriter(const BBGraph& _g, CustomUnorderedSet<BBGraph::vertex_descriptor> _annotated)
    : VertexWriter(_g, 0),
      function_behavior(_g.CGetGraphInfo().AppM->CGetFunctionBehavior(_g.CGetGraphInfo().function_index)),
      helper(function_behavior->CGetBehavioralHelper()),
      annotated(std::move(_annotated))
#if HAVE_HLS_BUILT
      ,
      schedule((GetPointer<const HLS_manager>(_g.CGetGraphInfo().AppM) &&
                GetPointer<const HLS_manager>(_g.CGetGraphInfo().AppM)->get_HLS(helper->get_function_index())) ?
                   GetPointer<const HLS_manager>(_g.CGetGraphInfo().AppM)->get_HLS(helper->get_function_index())->Rsch :
                   ScheduleConstRef())
#endif
{
}

void BBVertexWriter::operator()(std::ostream& out, BBGraph::vertex_descriptor v) const
{
   const auto& info = printing_graph.CGetGraphInfo();
   const auto& bb_node_info = printing_graph.CGetNodeInfo(v);
   if(v == info.entry_vertex)
   {
      out << "[color=blue,shape=Msquare, ";
      if(annotated.find(v) != annotated.end())
      {
         out << " style=filled, fillcolor=black, fontcolor=white,";
      }
      out << "label=\"ENTRY";
   }
   else if(v == info.exit_vertex)
   {
      out << "[color=blue,shape=Msquare, ";
      if(annotated.find(v) != annotated.end())
      {
         out << " style=filled, fillcolor=black, fontcolor=white,";
      }
      out << "label=\"EXIT";
   }
   else
   {
      out << "[shape=box";
      if(annotated.find(v) != annotated.end())
      {
         out << ", style=filled, fillcolor=black, fontcolor=white";
      }
      if(bb_node_info.block)
      {
         out << ", label=\"BB" << bb_node_info.block->number << " - CLANGLI: " << bb_node_info.block->loop_id
             << " - Cer: " << bb_node_info.cer;
         out << " - Loop " << bb_node_info.loop_id;
#if HAVE_HOST_PROFILING_BUILT
         out << " - Executions: " << function_behavior->CGetProfilingInformation()->GetBBExecutions(v);
#endif
      }
      const std::unique_ptr<var_pp_functor> svpf = std::make_unique<std_var_pp_functor>(helper);
      if(bb_node_info.block->CGetPhiList().size())
      {
         out << "\\l";
         for(const auto& phi : bb_node_info.block->CGetPhiList())
         {
            auto res = STR(phi->index);
#if HAVE_HLS_BUILT
            if(schedule && schedule->is_scheduled(phi->index))
            {
               res += " " + schedule->PrintTimingInformation(phi->index) + " ";
            }
#endif
            res += " -> " + helper->PrintNode(phi, svpf);
            std::string temp;
            for(char re : res)
            {
               if(re == '\"')
               {
                  temp += "\\\"";
               }
               else if(re != '\n')
               {
                  temp += re;
               }
            }
            out << temp << "\\l";
         }
      }
      if(bb_node_info.block->CGetStmtList().size())
      {
         if(bb_node_info.block->CGetPhiList().empty())
         {
            out << "\\n";
         }
         for(const auto& statement : bb_node_info.block->CGetStmtList())
         {
            auto res = STR(statement->index);
#if HAVE_HLS_BUILT
            if(schedule && schedule->is_scheduled(statement->index))
            {
               res += " " + schedule->PrintTimingInformation(statement->index) + " ";
            }
#endif
            res += " -> " + helper->PrintNode(statement, svpf);
            switch(statement->get_kind())
            {
               case assign_stmt_K:
               case call_stmt_K:
                  res += ";";
                  break;
               case constructor_node_K:
               case call_node_K:
               case multi_way_if_stmt_K:
               case nop_stmt_K:
               case phi_stmt_K:
               case return_stmt_K:
               case identifier_node_K:
               case ssa_node_K:
               case statement_list_node_K:
               case lut_node_K:
               case CASE_BINARY_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_TERNARY_NODES:
               case CASE_TYPE_NODES:
               case CASE_UNARY_NODES:
               default:
                  break;
            }
            std::string temp;
            for(char re : res)
            {
               if(re == '\"')
               {
                  temp += "\\\"";
               }
               else if(re != '\n')
               {
                  temp += re;
               }
            }
            out << temp << "\\l";
         }
      }
   }
   out << "\"]";
}

BBEdgeWriter::BBEdgeWriter(const BBGraph& _g)
    : EdgeWriter(_g, 0),
      BH(_g.CGetGraphInfo().AppM->CGetFunctionBehavior(_g.CGetGraphInfo().function_index)->CGetBehavioralHelper())
{
}

void BBEdgeWriter::operator()(std::ostream& out, const BBGraph::edge_descriptor& e) const
{
   if(FB_CFG_SELECTOR & printing_graph.GetSelector(e))
   {
      out << "[fontcolor=blue, color=gold";
   }
   else if(CFG_SELECTOR & printing_graph.GetSelector(e))
   {
      out << "[fontcolor=blue, color=red3";
   }
   else if(CDG_SELECTOR & printing_graph.GetSelector(e))
   {
      out << "[fontcolor=blue, color=red3";
   }
   else if(D_SELECTOR & printing_graph.GetSelector(e))
   {
      out << "[fontcolor=blue";
   }
   else if(PD_SELECTOR & printing_graph.GetSelector(e))
   {
      out << "[fontcolor=blue";
   }
   else if(J_SELECTOR & printing_graph.GetSelector(e))
   {
      out << "[fontcolor=blue, color=gold";
   }
   else
   {
      THROW_UNREACHABLE("Not supported graph type in printing: " + STR(printing_graph.GetSelector(e)));
   }
   const auto& bb_edge_info = printing_graph.CGetEdgeInfo(e);
   if(selector & FCFG_SELECTOR)
   {
      if(bb_edge_info.IfElseIf())
      {
         out << ",label=\"";
         out << bb_edge_info.PrintLabels(CFG_SELECTOR, BH);
         out << "\"";
      }
   }
   else if(selector & FCDG_SELECTOR)
   {
      if(bb_edge_info.IfElseIf())
      {
         out << ",label=\"";
         out << bb_edge_info.PrintLabels(CDG_SELECTOR, BH);
         out << "\"";
      }
   }
   out << "]";
}
