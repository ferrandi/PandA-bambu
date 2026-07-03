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
 * @file edge_c_writer.cpp
 * @brief This file contains the routines necessary to create a C executable program with instrumented edges
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "edge_c_writer.hpp"

#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "indented_output_stream.hpp"
#include "instruction_writer.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "op_graph.hpp"
#include "profiling_information.hpp"
#include "schedule.hpp"
#include "var_pp_functor.hpp"
#include <boost/range/adaptor/reversed.hpp>

EdgeCWriter::EdgeCWriter(const HLS_managerConstRef _HLSMgr, const InstructionWriterRef _instruction_writer,
                         const IndentedOutputStreamRef _indented_output_stream)
    : CWriter(_HLSMgr, _instruction_writer, _indented_output_stream), dumped_edges(), counter(0)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this));
}

void EdgeCWriter::Initialize()
{
   CWriter::Initialize();
   counter = 0;
   fun_loop_to_index.clear();
   // Iterating over all functions
   for(const auto f : HLSMgr->get_functions_with_body())
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(f);
      const auto& loops = FB->getConstLoops()->getList();
      for(const auto& loop : loops)
      {
         fun_loop_to_index[f][loop->getLoopId()] = counter;
         counter++;
      }
   }
}

void EdgeCWriter::print_edge(unsigned, BBGraph::edge_descriptor e, unsigned int)
{
   dumped_edges.insert(e);
}

void EdgeCWriter::print_end_path(unsigned int, unsigned int)
{
}

void EdgeCWriter::print_loop_ending(unsigned, BBGraph::edge_descriptor)
{
}

void EdgeCWriter::print_loop_escaping(unsigned, BBGraph::edge_descriptor)
{
}

void EdgeCWriter::print_loop_starting(unsigned, BBGraph::edge_descriptor)
{
}

void EdgeCWriter::print_loop_switching(unsigned, BBGraph::edge_descriptor)
{
}

void EdgeCWriter::writeRoutineInstructions_rec(unsigned fid, BBGraph::vertex_descriptor current_vertex, bool bracket,
                                               const std::unique_ptr<var_pp_functor>& variableFunctor)
{
   const auto function_behavior = HLSMgr->CGetFunctionBehavior(fid);
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   const auto cfgGraph = function_behavior->GetOpGraph(FunctionBehavior::FCFG);
   const auto bb_fcfgGraph = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   const auto& bb_graph_info = bb_fcfgGraph.CGetGraphInfo();
   const auto& bb_index_map = bb_graph_info.bb_index_map;

   const auto& bb_node_info = bb_fcfgGraph.CGetNodeInfo(current_vertex);
   const auto bb_number = bb_node_info.block->number;

   /// the entry vertex
   const auto entry_vertex = bb_graph_info.entry_vertex;

   /// the exit vertex
   const auto exit_vertex = bb_graph_info.exit_vertex;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting writing BB" + std::to_string(bb_number));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   THROW_ASSERT(bb_frontier.find(current_vertex) == bb_frontier.end(),
                "current_vertex cannot be part of the basic block frontier");

   if(bb_analyzed.find(current_vertex) != bb_analyzed.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--BB" + std::to_string(bb_number) + " already written");
      return;
   }

   bool add_phi_nodes_assignment = basic_block_tail.find(bb_number) != basic_block_tail.end();
   bool add_phi_nodes_assignment_prefix = basic_block_prefix.find(bb_number) != basic_block_prefix.end();
   bb_analyzed.insert(current_vertex);
   if(verbose)
   {
      indented_output_stream->Append("//Basic block " + std::to_string(bb_number) + " - loop " +
                                     std::to_string(bb_node_info.loop_id) + "\n");
   }

   /// get immediate post-dominator
   auto bb_PD = function_behavior->post_dominators->getImmediateDominator(current_vertex);
#ifndef NDEBUG
   const auto& bb_node_info_pd = bb_fcfgGraph.CGetNodeInfo(bb_PD);
   const auto bb_number_PD = bb_node_info_pd.block->number;
   std::string frontier_string;
   for(const auto bb : bb_frontier)
   {
      frontier_string += "BB" + std::to_string(bb_fcfgGraph.CGetNodeInfo(bb).block->number) + " ";
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Frontier at the moment is: " + frontier_string);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Its post-dominator is BB" + std::to_string(bb_number_PD));
#endif
   bool analyze_bb_PD = bb_frontier.find(bb_PD) == bb_frontier.end() && bb_analyzed.find(bb_PD) == bb_analyzed.end();
   if(analyze_bb_PD)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Post dominator will be examinated");
      bb_frontier.insert(bb_PD);
   }
   /// compute the last statement
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Looking for last statement");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   std::vector<OpGraph::vertex_descriptor> stmts_list(bb_node_info.statements_list.begin(),
                                                      bb_node_info.statements_list.end());
   const auto& sch = HLSMgr->get_HLS(fid)->Rsch;
   std::stable_sort(stmts_list.begin(), stmts_list.end(),
                    [&](const OpGraph::vertex_descriptor a, const OpGraph::vertex_descriptor b) {
                       const auto& a_info = cfgGraph.CGetNodeInfo(a);
                       const auto& b_info = cfgGraph.CGetNodeInfo(b);
                       const auto a_is_cond = (a_info.node_type & (TYPE_MULTIIF | TYPE_GOTO)) != 0;
                       const auto b_is_cond = (b_info.node_type & (TYPE_MULTIIF | TYPE_GOTO)) != 0;
                       if(a_is_cond || b_is_cond)
                       {
                          return !a_is_cond && b_is_cond;
                       }
                       return sch->get_cstep(a).second < sch->get_cstep(b).second;
                    });
   auto last_stmt = OpGraph::null_vertex();
   bool is_there = false;
   for(const auto st : boost::adaptors::reverse(stmts_list))
   {
      if(!local_rec_instructions.count(st))
      {
         continue;
      }
      const auto& op_info = cfgGraph.CGetNodeInfo(st);
      if(op_info.node_type & TYPE_VPHI)
      {
         continue;
      }
      if((op_info.node_type & TYPE_INIT) != 0)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering operation " + op_info.vertex_name);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "This is basic block is not empty in this task. Last operation to be printed id " +
                         op_info.vertex_name);
      last_stmt = st;
      is_there = true;
      break;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   /// check the feasibility
   bool last_statement_is_a_cond_or_goto = is_there and
                                           behavioral_helper->end_with_a_cond_or_goto(bb_node_info.block) != 0 &&
                                           last_stmt == stmts_list.back();
   THROW_ASSERT(!last_statement_is_a_cond_or_goto || !is_there ||
                    (last_statement_is_a_cond_or_goto && last_stmt == stmts_list.back()),
                "inconsistent recursion");

   bool add_bb_label = goto_list.find(current_vertex) != goto_list.end();

   if(add_bb_label)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block should start with a label");
   }

   if(!add_bb_label and bb_fcfgGraph.in_degree(current_vertex) > 1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Basic block has an indegree > 1 and not associated label");
      for(const auto& ie : bb_fcfgGraph.in_edges(current_vertex))
      {
         const auto source = bb_fcfgGraph.source(ie);
         // Basic block start the body of a short circuit
         if(!bb_analyzed.count(source) and !((FB_CFG_SELECTOR & bb_fcfgGraph.GetSelector(ie))))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Basic block should start with a label since is the body of a short-circuit");
            add_bb_label = true;
            break;
         }
         // Basic block is a header loop
         else if(!bb_analyzed.count(source) or current_vertex == source)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Basic block is the header of a loop and it does not end with while or for");
            add_bb_label = true;
            break;
         }
      }
   }
   bool add_semicolon = false;
   /// print each instruction
   if(bracket)
   {
      if(analyze_bb_PD || is_there || add_bb_label || add_phi_nodes_assignment || add_phi_nodes_assignment_prefix)
      {
         indented_output_stream->Append("{\n");
      }
      else
      {
         add_semicolon = true;
      }
   }

   if(local_inc.find(current_vertex) == local_inc.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "There is no local_inc");
      // Counting not feedback incoming edges
      gc_edge_descriptor only_edge;
      for(const auto& ei : bb_fcfgGraph.in_edges(current_vertex))
      {
         if(!(FB_CFG_SELECTOR & bb_fcfgGraph.GetSelector(ei)))
         {
            only_edge = ei;
         }
      }

      // Entry block
      if(current_vertex == entry_vertex)
      {
         for(const auto& eo : bb_fcfgGraph.out_edges(current_vertex))
         {
            if(exit_vertex != bb_fcfgGraph.target(eo))
            {
               print_loop_starting(fid, eo);
            }
         }
      }
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---There is already an edge instrumentation associated with the current basic block");
      const auto e = local_inc[current_vertex];
      const auto support_cfg = function_behavior->GetBBGraph(FunctionBehavior::BB);
      unsigned int first_loop_index = support_cfg.CGetNodeInfo(support_cfg.source(e)).loop_id;
      unsigned int second_loop_index = support_cfg.CGetNodeInfo(support_cfg.target(e)).loop_id;
      // Different loop
      if(first_loop_index != second_loop_index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Changing from loop " + std::to_string(first_loop_index) + " to " +
                            std::to_string(second_loop_index));
         unsigned int first_depth = 0;
         unsigned int second_depth = 0;
         if(first_loop_index)
         {
            first_depth = function_behavior->getConstLoops()->getConstLoop(first_loop_index)->getLoopDepth();
         }
         if(second_loop_index)
         {
            second_depth = function_behavior->getConstLoops()->getConstLoop(second_loop_index)->getLoopDepth();
         }
         // Second vertex is an header
         if(first_depth < second_depth)
         {
            print_loop_starting(fid, e);
         }
         // First vertex is an exit
         else if(second_depth < first_depth)
         {
            print_loop_escaping(fid, e);
         }
         // First vertex is an exit, second vertex is an header
         else
         {
            print_loop_switching(fid, e);
         }
      }
      else
      {
         print_edge(fid, e, fun_loop_to_index.find(fid)->second.find(bb_node_info.loop_id)->second);
      }
   }

   // Header block
   if((bb_node_info.loop_id == bb_number) && bb_number)
   {
      if(verbose)
      {
         indented_output_stream->Append(
             "//Starting of a loop - average iteration number " +
             std::to_string(function_behavior->CGetProfilingInformation()->GetLoopAvgIterations(bb_number)) + "\n");
      }
   }

   if(add_bb_label)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "A label should be added at the beginning");
      THROW_ASSERT(basic_blocks_labels.count(bb_number), "I do not know the destination: " + STR(bb_number));
      indented_output_stream->Append(basic_blocks_labels.at(bb_number) + ":\n");
      add_semicolon = true;
   }

   auto vIter = stmts_list.begin();
   if(!is_there)
   {
      for(const auto& oi : bb_fcfgGraph.out_edges(current_vertex))
      {
         if(FB_CFG_SELECTOR & bb_fcfgGraph.GetSelector(oi))
         {
            break;
         }
      }
   }
   if(is_there)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "There are instructions to be printed for this pair task - basic block");

      /// fill the renamin table in case is needed
      if(renaming_table.find(current_vertex) != renaming_table.end())
      {
         for(const auto& rvt : renaming_table.find(current_vertex)->second)
         {
            BehavioralHelper::rename_a_variable(rvt.first, rvt.second);
         }
      }
      const auto support_cfg = function_behavior->GetBBGraph(FunctionBehavior::BB);
      bool prefix_has_to_be_printed = basic_block_prefix.find(bb_number) != basic_block_prefix.end();
      do
      {
         /// We can print results of split of phi nodes if they have not yet been printed and if label has already been
         /// printed (or there was not any label to be printed)
         if(prefix_has_to_be_printed)
         {
            prefix_has_to_be_printed = false;
            indented_output_stream->Append(basic_block_prefix.find(bb_number)->second);
         }
         if(local_rec_instructions.find(*vIter) == local_rec_instructions.end())
         {
            continue;
         }
         const auto& v_info = cfgGraph.CGetNodeInfo(*vIter);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Preparing printing of operation " + v_info.vertex_name);
         // Now I print the instruction
         if(verbose)
         {
            indented_output_stream->Append("//Instruction: " + v_info.vertex_name + "\n");
         }

         bool isLastIntruction = last_stmt == *vIter;
         /// in case we have phi nodes we check if some assignments should be printed
         bool print_phi_now =
             ((v_info.node_type & TYPE_MULTIIF)) || behavioral_helper->end_with_a_cond_or_goto(bb_node_info.block);
         if(add_phi_nodes_assignment && isLastIntruction && print_phi_now)
         {
            indented_output_stream->Append(basic_block_tail.at(bb_number));
         }
         if((v_info.node_type & (TYPE_VPHI)) == 0)
         {
            // End of a loop with goto
            if(isLastIntruction and behavioral_helper->end_with_a_cond_or_goto(bb_node_info.block) and
               bb_fcfgGraph.out_degree(current_vertex) == 1)
            {
               const auto eo1 = bb_fcfgGraph.out_edges(current_vertex).front();
               if(FB_CFG_SELECTOR & bb_fcfgGraph.GetSelector(eo1))
               {
                  print_loop_ending(fid, eo1);
               }
            }
            else if((v_info.node_type & TYPE_RET) != 0)
            {
               print_end_path(fid, 0);
            }
            if((v_info.node_type & TYPE_LAST_OP) != 0 and (v_info.node_type & TYPE_RET) == 0)
            {
               print_end_path(fid, 0);
            }
            instrWriter->write(function_behavior, *vIter, variableFunctor);
            if((v_info.node_type & TYPE_LABEL) == 0)
            {
               add_semicolon = false;
            }
         }
         else if(verbose)
         {
            indented_output_stream->Append("//(removed virtual phi instruction)\n");
         }
         if(!isLastIntruction)
         {
            continue;
         }
         BehavioralHelper::clear_renaming_table();
         if(add_phi_nodes_assignment && !print_phi_now)
         {
            indented_output_stream->Append(basic_block_tail.find(bb_number)->second);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---This is not the last statement");
         if(support_cfg.out_degree(current_vertex) == 1 and bb_fcfgGraph.out_degree(current_vertex) == 1)
         {
            const auto eo1 = support_cfg.out_edges(current_vertex).front();
            const auto next = support_cfg.target(eo1);
            if(bb_fcfgGraph.CGetNodeInfo(current_vertex).loop_id == bb_fcfgGraph.CGetNodeInfo(next).loop_id)
            {
               THROW_ASSERT(fun_loop_to_index.find(fid) != fun_loop_to_index.end(),
                            "Function " + std::to_string(fid) + " not found");
               THROW_ASSERT(fun_loop_to_index.find(fid)->second.find(bb_node_info.loop_id) !=
                                fun_loop_to_index.find(fid)->second.end(),
                            "Loop " + std::to_string(bb_node_info.loop_id) + " not found");
               print_edge(fid, eo1, fun_loop_to_index.find(fid)->second.find(bb_node_info.loop_id)->second);
            }
         }
         // Now I check if this is a control statement and I consequently print
         // the instructions contained in its branches
         if(v_info.node_type & TYPE_MULTIIF)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operation is a multiif");
            unsigned int node_id = cfgGraph.CGetNodeInfo(last_stmt).GetNodeId();
            const auto node = TM->GetIRNode(node_id);
            THROW_ASSERT(node->get_kind() == multi_way_if_stmt_K, "unexpected node");
            auto* gmwi = GetPointer<multi_way_if_stmt>(node);
            std::map<unsigned int, bool> add_elseif_to_goto;
            for(const auto& cond : gmwi->list_of_cond)
            {
               unsigned int bb_index_num = cond.second;
               const auto bb_vertex = bb_index_map.find(bb_index_num)->second;
               if(cond != gmwi->list_of_cond.front())
               {
                  bool to_be_added =
                      bb_frontier.find(bb_vertex) == bb_frontier.end() && goto_list.find(bb_vertex) == goto_list.end();
                  add_elseif_to_goto[bb_index_num] = to_be_added;
                  if(to_be_added)
                  {
                     goto_list.insert(bb_vertex);
                  }
               }
               else
               {
                  add_elseif_to_goto[bb_index_num] = false;
               }
            }
            for(const auto& cond : gmwi->list_of_cond)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering successor BB" + STR(cond.second));
               unsigned int bb_index_num = cond.second;
               const auto bb_vertex = bb_index_map.find(bb_index_num)->second;
               if(cond != gmwi->list_of_cond.front())
               {
                  if(cond.first)
                  {
                     const auto cond_expr = behavioral_helper->PrintVariable(cond.first->index);
                     indented_output_stream->Append("else if(");
                     indented_output_stream->Append(ir_helper::IsBooleanType(cond.first) ? "((" + cond_expr + ") & 1)" :
                                                                                           cond_expr);
                     indented_output_stream->Append(")\n");
                  }
                  else
                  {
                     indented_output_stream->Append("else\n");
                  }
               }
               if(add_elseif_to_goto.find(bb_index_num) != add_elseif_to_goto.end() &&
                  add_elseif_to_goto.find(bb_index_num)->second)
               {
                  goto_list.erase(bb_vertex);
               }
               if(bb_frontier.find(bb_vertex) == bb_frontier.end())
               {
                  if(bb_analyzed.find(bb_vertex) == bb_analyzed.end())
                  {
                     const auto [e, found] = boost::edge(current_vertex, bb_vertex, support_cfg);
                     THROW_ASSERT(found, "Missing edge");
                     local_inc[bb_vertex] = e;
                     writeRoutineInstructions_rec(fid, bb_vertex, true, variableFunctor);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor has already been examined");

                     THROW_ASSERT(basic_blocks_labels.find(bb_index_num) != basic_blocks_labels.end(),
                                  "I do not know the destination " + std::to_string(bb_index_num));
                     indented_output_stream->Append("{\n");
                     const auto [e, found] = boost::edge(current_vertex, bb_vertex, bb_fcfgGraph);
                     THROW_ASSERT(found, "Edge missing");
                     if(not(FB_CFG_SELECTOR & bb_fcfgGraph.GetSelector(e)))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connected by feedback edge");
                        unsigned int first_loop_index = support_cfg.CGetNodeInfo(support_cfg.source(e)).loop_id;
                        unsigned int second_loop_index = support_cfg.CGetNodeInfo(support_cfg.target(e)).loop_id;
                        // Different loop
                        if(first_loop_index != second_loop_index)
                        {
                           unsigned int first_depth = 0;
                           unsigned int second_depth = 0;
                           if(first_loop_index)
                           {
                              first_depth =
                                  function_behavior->getConstLoops()->getConstLoop(first_loop_index)->getLoopDepth();
                           }
                           if(second_loop_index)
                           {
                              second_depth =
                                  function_behavior->getConstLoops()->getConstLoop(second_loop_index)->getLoopDepth();
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Depth of first loop is " + STR(first_depth));
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Depth of second loop is " + STR(second_depth));
                           // Second vertex is an header
                           if(first_depth < second_depth)
                           {
                              print_loop_starting(fid, e);
                           }
                           // First vertex is an exit
                           else if(second_depth < first_depth)
                           {
                              print_loop_escaping(fid, e);
                           }
                           // First vertex is an exit, second vertex is an header
                           else
                           {
                              print_loop_switching(fid, e);
                           }
                        }
                        else
                        {
                           print_edge(fid, e, fun_loop_to_index.find(fid)->second.find(bb_node_info.loop_id)->second);
                        }
                     }
                     else
                     {
                        print_loop_ending(fid, e);
                     }
                     indented_output_stream->Append("goto " + basic_blocks_labels.find(bb_index_num)->second + ";\n");
                     goto_list.insert(bb_vertex);
                     indented_output_stream->Append("}\n");
                  }
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connected by forward edge");
                  indented_output_stream->Append("{\n");
                  const auto [e, found] = boost::edge(current_vertex, bb_vertex, support_cfg);
                  unsigned int first_loop_index = support_cfg.CGetNodeInfo(support_cfg.source(e)).loop_id;
                  unsigned int second_loop_index = support_cfg.CGetNodeInfo(support_cfg.target(e)).loop_id;
                  // Different loop
                  if(first_loop_index != second_loop_index)
                  {
                     unsigned int first_depth = 0;
                     unsigned int second_depth = 0;
                     if(first_loop_index)
                     {
                        first_depth =
                            function_behavior->getConstLoops()->getConstLoop(first_loop_index)->getLoopDepth();
                     }
                     if(second_loop_index)
                     {
                        second_depth =
                            function_behavior->getConstLoops()->getConstLoop(second_loop_index)->getLoopDepth();
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Depth of first loop is " + STR(first_depth));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Depth of second loop is " + STR(second_depth));
                     // Second vertex is an header
                     if(first_depth < second_depth)
                     {
                        print_loop_starting(fid, e);
                     }
                     // First vertex is an exit
                     else if(second_depth < first_depth)
                     {
                        print_loop_escaping(fid, e);
                     }
                     // First vertex is an exit, second vertex is an header
                     else
                     {
                        print_loop_switching(fid, e);
                     }
                  }
                  else
                  {
                     print_edge(fid, e, fun_loop_to_index.find(fid)->second.find(bb_node_info.loop_id)->second);
                  }
                  indented_output_stream->Append("}\n");
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered successor BB" + STR(cond.second));
            }
         }
         else if(behavioral_helper->end_with_a_cond_or_goto(bb_node_info.block))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block ends with a cond or a goto");
            if(last_statement_is_a_cond_or_goto)
            {
               /// now we can analyze the following basic blocks
               for(const auto& oe : bb_fcfgGraph.out_edges(current_vertex))
               {
                  const auto next_bb = bb_fcfgGraph.target(oe);
                  if(bb_fcfgGraph.out_degree(current_vertex) > 1)
                  {
                     THROW_ERROR_CODE(PROFILING_EC, "Profiling does not support computed goto");
                     THROW_ERROR("Profiling does not support computed goto");
                  }
                  if(bb_frontier.find(next_bb) != bb_frontier.end())
                  {
                     continue;
                  }
                  goto_list.insert(next_bb);
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is not a special operation");
            const auto bbentry = bb_fcfgGraph.CGetGraphInfo().entry_vertex;
            if(current_vertex == bbentry)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Ended writing basic block " + std::to_string(bb_number));
               return;
            }
            THROW_ASSERT(bb_fcfgGraph.out_degree(current_vertex) <= 1,
                         "Only one edge expected as output of BB" +
                             STR(bb_fcfgGraph.CGetNodeInfo(current_vertex).block->number));
            for(const auto& oe : bb_fcfgGraph.out_edges(current_vertex))
            {
               const auto next_bb = bb_fcfgGraph.target(oe);
               if(bb_frontier.find(next_bb) != bb_frontier.end())
               {
                  if(bb_fcfgGraph.CGetNodeInfo(current_vertex).loop_id != bb_fcfgGraph.CGetNodeInfo(next_bb).loop_id)
                  {
                     print_loop_starting(fid, oe);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Not adding goto since target is in the frontier");
                  continue;
               }
               if(bb_fcfgGraph.CGetNodeInfo(current_vertex).loop_id != bb_fcfgGraph.CGetNodeInfo(next_bb).loop_id)
               {
                  print_loop_starting(fid, oe);
               }
               if(bb_fcfgGraph.in_degree(next_bb) == 1)
               {
                  writeRoutineInstructions_rec(fid, next_bb, false, variableFunctor);
               }
               else
               {
                  const auto& next_bb_node_info = bb_fcfgGraph.CGetNodeInfo(next_bb);
                  const unsigned int next_bb_number = next_bb_node_info.block->number;
                  THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(),
                               "I do not know the destination");
                  indented_output_stream->Append("   goto " + basic_blocks_labels.find(next_bb_number)->second +
                                                 ";/*Goto4*/\n");
                  goto_list.insert(next_bb);
               }
            }
         }
      } while(*vIter++ != last_stmt);
      if(add_semicolon)
      {
         indented_output_stream->Append(";\n"); /// added a fake indent
      }
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "is_there is false");
      /// in case we have phi nodes we check if some assignments should be printed
      if(add_phi_nodes_assignment)
      {
         indented_output_stream->Append(basic_block_tail.find(bb_number)->second);
         add_semicolon = false;
      }
      if(!behavioral_helper->end_with_a_cond_or_goto(bb_node_info.block))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not end with a cond or goto");
         const auto bbentry = bb_fcfgGraph.CGetGraphInfo().entry_vertex;
         if(current_vertex == bbentry)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Ended writing basic block " + std::to_string(bb_number));
            return;
         }
         THROW_ASSERT(bb_fcfgGraph.out_degree(current_vertex) <= 1,
                      "only one edge expected BB(" + std::to_string(bb_number) + ") Fun(" + std::to_string(fid) + ")");
         for(const auto& oe : bb_fcfgGraph.out_edges(current_vertex))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Examining the only? successor");
            const auto next_bb = bb_fcfgGraph.target(oe);
            if(bb_fcfgGraph.in_degree(next_bb) == 1)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor is the only");
               if(bb_fcfgGraph.CGetNodeInfo(current_vertex).loop_id != bb_fcfgGraph.CGetNodeInfo(next_bb).loop_id)
               {
                  print_loop_starting(fid, oe);
               }
               continue;
            }
            if(bb_frontier.find(next_bb) != bb_frontier.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor belongs to frontier");
               if(bb_fcfgGraph.CGetNodeInfo(current_vertex).loop_id != bb_fcfgGraph.CGetNodeInfo(next_bb).loop_id)
               {
                  print_loop_starting(fid, oe);
               }
               else
               {
                  print_edge(fid, oe, fun_loop_to_index.find(fid)->second.find(bb_node_info.loop_id)->second);
               }
               continue;
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor does not belong to frontier");
            const auto& next_bb_node_info = bb_fcfgGraph.CGetNodeInfo(next_bb);
            const unsigned int next_bb_number = next_bb_node_info.block->number;
            THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(),
                         "I do not know the destination");
            if(bb_fcfgGraph.CGetNodeInfo(current_vertex).loop_id != bb_fcfgGraph.CGetNodeInfo(next_bb).loop_id)
            {
               print_loop_starting(fid, oe);
            }
            if(bb_fcfgGraph.CGetNodeInfo(next_bb).loop_id == bb_fcfgGraph.CGetNodeInfo(next_bb).block->number)
            {
               print_loop_ending(fid, oe);
            }
            indented_output_stream->Append("   goto " + basic_blocks_labels.find(next_bb_number)->second +
                                           ";/*goto5*/\n");
            goto_list.insert(next_bb);
            add_semicolon = false;
         }
         if(add_semicolon)
         {
            indented_output_stream->Append(";\n"); /// added a fake indent
         }
      }
      else if(add_semicolon)
      {
         indented_output_stream->Append("   ;\n"); /// added a fake indent
      }
   }

   if(analyze_bb_PD)
   {
      // recurse on the post dominator
      bb_frontier.erase(bb_PD);
      THROW_ASSERT(bb_analyzed.find(bb_PD) == bb_analyzed.end(),
                   "something wrong happened " + std::to_string(bb_fcfgGraph.CGetNodeInfo(bb_PD).block->number) +
                       " Fun(" + std::to_string(fid) + ")");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Printing the post dominator");
      writeRoutineInstructions_rec(fid, bb_PD, false, variableFunctor);
   }
   if((analyze_bb_PD || is_there || add_bb_label || add_phi_nodes_assignment || add_phi_nodes_assignment_prefix) &&
      bracket)
   {
      indented_output_stream->Append("}\n");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--BB" + std::to_string(bb_number) + " written");
}

void EdgeCWriter::writeRoutineInstructions(const unsigned int fid, const OpVertexSet& instructions,
                                           const std::unique_ptr<var_pp_functor>& variableFunctor,
                                           BBGraph::vertex_descriptor bb_start,
                                           CustomOrderedSet<BBGraph::vertex_descriptor> bb_end)
{
   const auto function_behavior = HLSMgr->CGetFunctionBehavior(fid);
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   const auto bb_fcfgGraph = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   const auto support_cfg = function_behavior->GetBBGraph(FunctionBehavior::BB);
   local_inc.clear();
   dumped_edges = std::set<BBGraph::edge_descriptor, ltedge<BBGraphsCollection>>(
       ltedge<BBGraphsCollection>(&function_behavior->GetBBGraphsCollection()));

   const auto cfgGraph = function_behavior->GetOpGraph(FunctionBehavior::FCFG);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "-->Edge profiling writer - start to fwrite body of function " +
                      behavioral_helper->GetFunctionName());
   if(instructions.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Edge profiling writer - Empty function");
      return;
   }
   else if(instructions.size() == 1)
   {
      if(cfgGraph.CGetNodeInfo((*instructions.begin())).node_type & TYPE_ENTRY)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Edge profiling writer - Empty function");
         return;
      }
   }
   /// Then I compute all the labels associated with a basic block with more than one entering edge.
   basic_blocks_labels.clear();
   OpGraph::vertex_descriptor bbentry;
   CustomOrderedSet<OpGraph::vertex_descriptor> bb_exit;
   if(!bb_start)
   {
      bbentry = bb_fcfgGraph.CGetGraphInfo().entry_vertex;
   }
   else
   {
      bbentry = bb_start;
   }
   if(bb_end.empty())
   {
      bb_exit.insert(bb_fcfgGraph.CGetGraphInfo().exit_vertex);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "There are " + std::to_string(bb_exit.size()) + " exit basic blocks");
   }
   else
   {
      bb_exit = bb_end;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing labels");
   for(const auto& v : bb_fcfgGraph.vertices())
   {
      size_t delta = bb_exit.find(v) != bb_exit.end() ? 1u : 0u;
      if(bb_fcfgGraph.in_degree(v) <= (1 + delta))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Skipped BB" + std::to_string(bb_fcfgGraph.CGetNodeInfo(v).block->number));
         continue;
      }
      const auto& bb_node_info = bb_fcfgGraph.CGetNodeInfo(v);
      basic_blocks_labels[bb_node_info.block->number] =
          ("BB_LABEL_" + std::to_string(bb_node_info.block->number)) +
          (bb_label_counter == 1 ? "" : "_" + std::to_string(bb_label_counter));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Label of BB" + std::to_string(bb_fcfgGraph.CGetNodeInfo(v).block->number) + " is " +
                         basic_blocks_labels[bb_node_info.block->number]);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed labels");
   /// set of basic block already analyzed
   bb_analyzed.clear();
   bb_analyzed.insert(bb_exit.begin(), bb_exit.end());
   /// store for which basic block the goto has been used
   goto_list.clear();
   /// basic block frontier over which writeRoutineInstructions_rec cannot go.
   bb_frontier.clear();
   bb_frontier.insert(bb_exit.begin(), bb_exit.end());
   local_rec_instructions.clear();
   local_rec_instructions.insert(instructions.begin(), instructions.end());

   /// some statements can be in entry
   writeRoutineInstructions_rec(fid, bbentry, false, variableFunctor);
   if(!bb_start && bb_end.size() == 0)
   {
      for(const auto& oe : bb_fcfgGraph.out_edges(bbentry))
      {
         if(bb_exit.find(bb_fcfgGraph.target(oe)) != bb_exit.end())
         {
            continue;
         }
         else
         {
            writeRoutineInstructions_rec(fid, bb_fcfgGraph.target(oe), false, variableFunctor);
         }
      }
   }
   CustomOrderedSet<BBGraph::vertex_descriptor> not_yet_considered;
   std::set_difference(goto_list.begin(), goto_list.end(),                           /*first set*/
                       bb_analyzed.begin(), bb_analyzed.end(),                       /*second set*/
                       std::inserter(not_yet_considered, not_yet_considered.begin()) /*result*/
   );
   while(!not_yet_considered.empty())
   {
      const auto next_bb = *not_yet_considered.begin();
      not_yet_considered.erase(next_bb);
      writeRoutineInstructions_rec(fid, next_bb, false, variableFunctor);
      not_yet_considered.clear();
      std::set_difference(goto_list.begin(), goto_list.end(),     /*first set*/
                          bb_analyzed.begin(), bb_analyzed.end(), /*second set*/
                          std::inserter(not_yet_considered, not_yet_considered.begin()) /*result*/);
   }
   const auto exit = bb_fcfgGraph.CGetGraphInfo().exit_vertex;
   if(goto_list.find(exit) != goto_list.end() &&
      basic_blocks_labels.find(bloc::EXIT_BLOCK_ID) != basic_blocks_labels.end())
   {
      indented_output_stream->Append(basic_blocks_labels.find(bloc::EXIT_BLOCK_ID)->second + ":\n");
   }
   for(const auto& e : support_cfg.edges())
   {
      if(dumped_edges.find(e) == dumped_edges.end() &&
         support_cfg.source(e) != support_cfg.CGetGraphInfo().entry_vertex &&
         support_cfg.target(e) != support_cfg.CGetGraphInfo().exit_vertex)
      {
         WriteFile("Error.c");
         THROW_ERROR_CODE(PROFILING_EC,
                          "Profiling Instrumentation of Edge of function " + behavioral_helper->GetFunctionName() +
                              " from vertex BB" +
                              std::to_string(support_cfg.CGetNodeInfo(support_cfg.source(e)).block->number) + " to BB" +
                              std::to_string(support_cfg.CGetNodeInfo(support_cfg.target(e)).block->number) +
                              " not printed");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Edge profiling writer - ended");
}

const std::map<unsigned int, std::map<unsigned int, unsigned int>>& EdgeCWriter::CGetFunctionLoopToId() const
{
   return fun_loop_to_index;
}

void EdgeCWriter::InternalWriteHeader()
{
   CWriter::InternalWriteHeader();
   indented_output_stream->Append("#define __builtin_bambu_time_start()\n");
   indented_output_stream->Append("#define __builtin_bambu_time_stop()\n\n");
   indented_output_stream->Append("#define __builtin___divsc3 __divsc3\n");
   indented_output_stream->Append("#define __builtin___divdc3 __divdc3\n\n");
}
