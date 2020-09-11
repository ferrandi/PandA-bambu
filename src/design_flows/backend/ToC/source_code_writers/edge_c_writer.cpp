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
 * @file edge_c_writer.cpp
 * @brief This file contains the routines necessary to create a C executable program with instrumented edges
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "edge_c_writer.hpp"

/// algorithms/dominance include
#include "Dominance.hpp"

/// Backend include
#include "instruction_writer.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "function_behavior.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "op_graph.hpp"
#include "profiling_information.hpp"

/// Graph include
#include "basic_block.hpp"
#include "graph.hpp"

/// Parameter include
#include "Parameter.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

/// STL include
#include "custom_map.hpp"

/// utility include
#include "indented_output_stream.hpp"

EdgeCWriter::EdgeCWriter(const application_managerConstRef _AppM, const InstructionWriterRef _instruction_writer, const IndentedOutputStreamRef _indented_output_stream, const ParameterConstRef _Param, bool _verbose)
    : CWriter(_AppM, _instruction_writer, _indented_output_stream, _Param, _verbose), dumped_edges(ltedge<BBGraph>(nullptr)), counter(0)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this));
}

EdgeCWriter::~EdgeCWriter() = default;

void EdgeCWriter::Initialize()
{
   CWriter::Initialize();
   counter = 0;
   fun_loop_to_index.clear();
   // Iterating over all functions
   for(const auto f : AppM->get_functions_with_body())
   {
      const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(f);
      const std::list<LoopConstRef> loops = FB->CGetLoops()->GetList();
      std::list<LoopConstRef>::const_iterator l, l_end = loops.end();
      for(l = loops.begin(); l != l_end; ++l)
      {
         fun_loop_to_index[f][(*l)->GetId()] = counter;
         counter++;
      }
   }
}

void EdgeCWriter::print_edge(EdgeDescriptor e, unsigned int)
{
   dumped_edges.insert(e);
}

void EdgeCWriter::print_end_path(unsigned int, unsigned int)
{
}

void EdgeCWriter::print_loop_ending(EdgeDescriptor)
{
}

void EdgeCWriter::print_loop_escaping(EdgeDescriptor)
{
}

void EdgeCWriter::print_loop_starting(EdgeDescriptor)
{
}

void EdgeCWriter::print_loop_switching(EdgeDescriptor)
{
}

void EdgeCWriter::writeRoutineInstructions_rec(vertex current_vertex, bool bracket)
{
   const BBGraphConstRef bb_fcfgGraph = local_rec_function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   const BBGraphInfoConstRef bb_graph_info = bb_fcfgGraph->CGetBBGraphInfo();
   const OpGraphConstRef cfgGraph = local_rec_function_behavior->CGetOpGraph(FunctionBehavior::FCFG);
   const CustomUnorderedMap<unsigned int, vertex>& bb_index_map = bb_graph_info->bb_index_map;
   const BehavioralHelperConstRef behavioral_helper = local_rec_function_behavior->CGetBehavioralHelper();
   unsigned int funId = behavioral_helper->get_function_index();

   const BBNodeInfoConstRef bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(current_vertex);
   const unsigned int bb_number = bb_node_info->block->number;

   /// the entry vertex
   const vertex entry_vertex = bb_fcfgGraph->CGetBBGraphInfo()->entry_vertex;

   /// the exit vertex
   const vertex exit_vertex = bb_fcfgGraph->CGetBBGraphInfo()->exit_vertex;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting writing BB" + boost::lexical_cast<std::string>(bb_number));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   THROW_ASSERT(bb_frontier.find(current_vertex) == bb_frontier.end(), "current_vertex cannot be part of the basic block frontier");

   if(bb_analyzed.find(current_vertex) != bb_analyzed.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--BB" + boost::lexical_cast<std::string>(bb_number) + " already written");
      return;
   }

   bool add_phi_nodes_assignment = basic_block_tail.find(bb_number) != basic_block_tail.end();
   bool add_phi_nodes_assignment_prefix = basic_block_prefix.find(bb_number) != basic_block_prefix.end();
   bb_analyzed.insert(current_vertex);
   if(this->verbose)
      indented_output_stream->Append("//Basic block " + boost::lexical_cast<std::string>(bb_number) + " - loop " + boost::lexical_cast<std::string>(bb_node_info->loop_id) + "\n");

   /// get immediate post-dominator
   vertex bb_PD = post_dominators->get_immediate_dominator(current_vertex);
#ifndef NDEBUG
   const BBNodeInfoConstRef bb_node_info_pd = bb_fcfgGraph->CGetBBNodeInfo(bb_PD);
   const unsigned int bb_number_PD = bb_node_info_pd->block->number;
   std::string frontier_string;
   CustomOrderedSet<vertex>::iterator bb_frontier_begin, bb_frontier_end = bb_frontier.end();
   for(bb_frontier_begin = bb_frontier.begin(); bb_frontier_begin != bb_frontier_end; ++bb_frontier_begin)
   {
      frontier_string += "BB" + boost::lexical_cast<std::string>(bb_fcfgGraph->CGetBBNodeInfo(*bb_frontier_begin)->block->number) + " ";
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Frontier at the moment is: " + frontier_string);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Its post-dominator is BB" + boost::lexical_cast<std::string>(bb_number_PD));
#endif
   bool analyze_bb_PD = bb_frontier.find(bb_PD) == bb_frontier.end() && bb_analyzed.find(bb_PD) == bb_analyzed.end();
   if(analyze_bb_PD)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Post dominator will be examinated");
      bb_frontier.insert(bb_PD);
   }
   /// compute the last statement
   vertex last_stmt = NULL_VERTEX;
   std::list<vertex>::const_reverse_iterator vRIter, vRIterEnd;
   bool is_there = false;
#if 0
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operations of task ");
   if (debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      OpVertexSet::const_iterator it, it_end;
      it_end = local_rec_instructions.end();
      for(it = local_rec_instructions.begin(); it != it_end; ++it)
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_NAME(cfgGraph, *it));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Looking for last statement");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(vRIter = bb_node_info->statements_list.rbegin(), vRIterEnd = bb_node_info->statements_list.rend(); vRIter != vRIterEnd; ++vRIter)
   {
      if(local_rec_instructions.find(*vRIter) == local_rec_instructions.end())
         continue;
      if(GET_TYPE(cfgGraph, *vRIter) & TYPE_VPHI)
         continue;
      if((GET_TYPE(cfgGraph, *vRIter) & TYPE_INIT) != 0)
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering operation " + GET_NAME(cfgGraph, *vRIter));
      is_there = true;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "This is basic block is not empty in this task. Last operation to be printed id " + GET_NAME(cfgGraph, *vRIter));
      last_stmt = *vRIter;
      break;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   /// check the feasibility
   bool last_statement_is_a_cond_or_goto = is_there and behavioral_helper->end_with_a_cond_or_goto(bb_node_info->block) != 0 && last_stmt == bb_node_info->statements_list.back();
   THROW_ASSERT(!last_statement_is_a_cond_or_goto || !is_there || (last_statement_is_a_cond_or_goto && last_stmt == bb_node_info->statements_list.back()), "inconsistent recursion");
   /// check for basic block label
   bool start_with_a_label = behavioral_helper->start_with_a_label(bb_node_info->block);

   if(start_with_a_label)
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block starts with a label");

   bool add_bb_label = goto_list.find(current_vertex) != goto_list.end();

   if(add_bb_label)
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block should start with a label");

   if(!add_bb_label and !start_with_a_label and boost::in_degree(current_vertex, *bb_fcfgGraph) > 1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block has an indegree > 1 and not associated label");
      InEdgeIterator inE, inEEnd;
      for(boost::tie(inE, inEEnd) = boost::in_edges(current_vertex, *bb_fcfgGraph); inE != inEEnd; ++inE)
      {
         vertex source = boost::source(*inE, *bb_fcfgGraph);
         const BBNodeInfoConstRef pred_bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(source);

         // This condition match first basic block of case preceded by a case without break
         if(pred_bb_node_info->statements_list.size() and (GET_TYPE(cfgGraph, *(pred_bb_node_info->statements_list.rbegin())) & TYPE_SWITCH) and post_dominators->get_immediate_dominator(source) != current_vertex)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block is the first case of a case preceded by a case without break");
            add_bb_label = true;
            break;
         }
         // Basic block start the body of a short circuit
         else if(bb_analyzed.find(source) == bb_analyzed.end() and !((FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(*inE))))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block should start with a label since is the body of a short-circuit");
            add_bb_label = true;
            break;
         }
         // Basic block is a header loop, but it does not end with while or for
         else if((bb_analyzed.find(source) == bb_analyzed.end() or current_vertex == source) and (bb_node_info->statements_list.empty() or ((GET_TYPE(cfgGraph, *(bb_node_info->statements_list.rbegin())) & (TYPE_WHILE | TYPE_FOR)) == 0)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block is the header of a loop and it does not end with while or for");
            add_bb_label = true;
            break;
         }
      }
   }
   add_bb_label = add_bb_label && !start_with_a_label;
   bool add_semicolon = false;
   /// print each instruction
   if(bracket)
   {
      if(analyze_bb_PD || is_there || add_bb_label || add_phi_nodes_assignment || add_phi_nodes_assignment_prefix)
      {
         indented_output_stream->Append("{\n");
      }
      else
         add_semicolon = true;
   }

   if(local_inc.find(current_vertex) == local_inc.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "There is no local_inc");
      // Counting not feedback incoming edges
      unsigned int not_feedback_counter = 0;
      InEdgeIterator ei, ei_end;
      EdgeDescriptor only_edge;
      for(boost::tie(ei, ei_end) = boost::in_edges(current_vertex, *bb_fcfgGraph); ei != ei_end; ei++)
      {
         if(!(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(*ei)))
         {
            only_edge = *ei;
            not_feedback_counter++;
         }
      }

      // Entry block
      if(current_vertex == entry_vertex)
      {
         OutEdgeIterator eo, eo_end;
         for(boost::tie(eo, eo_end) = boost::out_edges(current_vertex, *bb_fcfgGraph); eo != eo_end; eo++)
         {
            if(exit_vertex != boost::target(*eo, *bb_fcfgGraph))
            {
               print_loop_starting(*eo);
            }
         }
      }
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---There is already an edge instrumentation associated with the current basic block");
      EdgeDescriptor e = local_inc[current_vertex];
      unsigned int first_loop_index = support_cfg->CGetBBNodeInfo(boost::source(e, *support_cfg))->loop_id;
      unsigned int second_loop_index = support_cfg->CGetBBNodeInfo(boost::target(e, *support_cfg))->loop_id;
      // Different loop
      if(first_loop_index != second_loop_index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Changing from loop " + boost::lexical_cast<std::string>(first_loop_index) + " to " + boost::lexical_cast<std::string>(second_loop_index));
         unsigned int first_depth = 0;
         unsigned int second_depth = 0;
         if(first_loop_index)
            first_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(first_loop_index)->depth;
         if(second_loop_index)
            second_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(second_loop_index)->depth;
         // Second vertex is an header
         if(first_depth < second_depth)
         {
            print_loop_starting(e);
         }
         // First vertex is an exit
         else if(second_depth < first_depth)
         {
            print_loop_escaping(e);
         }
         // First vertex is an exit, second vertex is an header
         else
         {
            print_loop_switching(e);
         }
      }
      else
      {
         print_edge(e, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
      }
   }

   // Header block
   if((bb_node_info->loop_id == bb_number) && bb_number)
   {
      if(verbose)
         indented_output_stream->Append("//Starting of a loop - average iteration number " + boost::lexical_cast<std::string>(AppM->CGetFunctionBehavior(funId)->CGetProfilingInformation()->GetLoopAvgIterations(bb_number)) + "\n");
   }

   if(add_bb_label)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "A label should be added at the beginning");
      THROW_ASSERT(basic_blocks_labels.find(bb_number) != basic_blocks_labels.end(), "I do not know the destination: " + STR(bb_number));
      indented_output_stream->Append(basic_blocks_labels.find(bb_number)->second + ":\n");
      add_semicolon = true;
   }
   else if(start_with_a_label)
   {
      add_semicolon = true;
   }
   std::list<vertex>::const_iterator vIter, vIterEnd, vIterBegin;
   vIter = bb_node_info->statements_list.begin();
   vIterBegin = vIter;
   vIterEnd = bb_node_info->statements_list.end();
   if(not is_there)
   {
      OutEdgeIterator oi, oend;
      bool notFeedBack = true;
      for(boost::tie(oi, oend) = boost::out_edges(current_vertex, *bb_fcfgGraph); oi != oend and notFeedBack; oi++)
      {
         if(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(*oi))
            notFeedBack = false;
      }
   }
   if(is_there)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "There are instructions to be printed for this pair task - basic block");

      /// fill the renamin table in case is needed
      if(renaming_table.find(current_vertex) != renaming_table.end())
      {
         const std::map<unsigned int, std::string>& rvt = renaming_table.find(current_vertex)->second;
         auto rvt_it_end = rvt.end();
         for(auto rvt_it = rvt.begin(); rvt_it != rvt_it_end; ++rvt_it)
            BehavioralHelper::rename_a_variable(rvt_it->first, rvt_it->second);
      }
      bool label_has_to_be_printed = start_with_a_label;
      bool prefix_has_to_be_printed = basic_block_prefix.find(bb_number) != basic_block_prefix.end();
      unsigned int analyzed_statement = 0;
      do
      {
         /// We can print results of split of phi nodes if they have not yet been printed and if label has already been printed (or there was not any label to be printed)
         if(prefix_has_to_be_printed and not label_has_to_be_printed)
         {
            prefix_has_to_be_printed = false;
            indented_output_stream->Append(basic_block_prefix.find(bb_number)->second);
         }
         if(local_rec_instructions.find(*vIter) == local_rec_instructions.end())
            continue;
         analyzed_statement++;
         /// If there is not any label to be printed, label_has_to_be_printed is already false, otherwise the label will be printed during this loop iteration
         label_has_to_be_printed = false;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Preparing printing of operation " + GET_NAME(cfgGraph, *vIter));
         // Now I print the instruction
         if(this->verbose)
            indented_output_stream->Append("//Instruction: " + GET_NAME(cfgGraph, *vIter) + "\n");

         if(start_with_a_label && vIter == vIterBegin)
         {
            InEdgeIterator inE, inEEnd;
            for(boost::tie(inE, inEEnd) = boost::in_edges(current_vertex, *bb_fcfgGraph); inE != inEEnd; inE++)
            {
               if(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(*inE))
                  indented_output_stream->Append("//start of a loop\n");
            }
         }
         bool isLastIntruction = last_stmt == *vIter;
         /// in case we have phi nodes we check if some assignments should be printed
         bool print_phi_now = ((GET_TYPE(cfgGraph, *vIter) & (TYPE_IF | TYPE_WHILE | TYPE_FOR | TYPE_SWITCH | TYPE_MULTIIF))) || behavioral_helper->end_with_a_cond_or_goto(bb_node_info->block);
         if(add_phi_nodes_assignment && isLastIntruction && print_phi_now)
            indented_output_stream->Append(basic_block_tail.find(bb_number)->second);
         if((GET_TYPE(cfgGraph, *vIter) & (TYPE_VPHI)) == 0)
         {
            if(GET_TYPE(cfgGraph, *vIter) & (TYPE_WHILE | TYPE_FOR) and this->verbose and local_rec_function_behavior->CGetLoops()->CGetLoop(bb_node_info->loop_id)->loop_type & DOALL_LOOP)
               indented_output_stream->Append("//#pragma omp parallel for\n");
            // End of a loop with goto
            if(isLastIntruction and behavioral_helper->end_with_a_cond_or_goto(bb_node_info->block) and boost::out_degree(current_vertex, *bb_fcfgGraph) == 1)
            {
               OutEdgeIterator eo1, eo_end1;
               boost::tie(eo1, eo_end1) = boost::out_edges(current_vertex, *bb_fcfgGraph);
               if(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(*eo1))
                  print_loop_ending(*eo1);
            }
            else if((GET_TYPE(cfgGraph, *vIter) & TYPE_RET) != 0)
            {
               print_end_path(fun_id, 0);
            }
            if((GET_TYPE(cfgGraph, *vIter) & TYPE_LAST_OP) != 0 and (GET_TYPE(cfgGraph, *vIter) & TYPE_RET) == 0)
            {
               print_end_path(fun_id, 0);
            }
            instrWriter->write(local_rec_function_behavior, *vIter, local_rec_variableFunctor);
            if((GET_TYPE(cfgGraph, *vIter) & TYPE_LABEL) == 0)
               add_semicolon = false;
         }
         else if(this->verbose)
         {
            indented_output_stream->Append("//(removed virtual phi instruction)\n");
         }
         if(!isLastIntruction)
            continue;
         BehavioralHelper::clear_renaming_table();
         if(add_phi_nodes_assignment && !print_phi_now)
            indented_output_stream->Append(basic_block_tail.find(bb_number)->second);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---This is not the last statement");
         if(boost::out_degree(current_vertex, *support_cfg) == 1 and boost::out_degree(current_vertex, *bb_fcfgGraph) == 1)
         {
            OutEdgeIterator eo1, eo_end1;
            boost::tie(eo1, eo_end1) = boost::out_edges(current_vertex, *support_cfg);
            vertex next = boost::target(*eo1, *support_cfg);
            if(bb_fcfgGraph->CGetBBNodeInfo(current_vertex)->loop_id == bb_fcfgGraph->CGetBBNodeInfo(next)->loop_id)
            {
               THROW_ASSERT(fun_loop_to_index.find(funId) != fun_loop_to_index.end(), "Function " + boost::lexical_cast<std::string>(funId) + " not found");
               THROW_ASSERT(fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id) != fun_loop_to_index.find(funId)->second.end(), "Loop " + boost::lexical_cast<std::string>(bb_node_info->loop_id) + " not found");
               print_edge(*eo1, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
            }
         }
         // Now I check if this is a control statement and I consequently print
         // the instructions contained in its branches
         if(GET_TYPE(cfgGraph, *vIter) & TYPE_IF)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operation is an if");
            const unsigned int bb_true_number = bb_node_info->block->true_edge;
            const vertex true_vertex = bb_index_map.find(bb_true_number)->second;
            const unsigned int bb_false_number = bb_node_info->block->false_edge;
            vertex false_vertex = bb_index_map.find(bb_false_number)->second;
            bool add_false_to_goto = false;
            if(bb_frontier.find(true_vertex) == bb_frontier.end())
            {
               if(bb_frontier.find(false_vertex) == bb_frontier.end() && goto_list.find(false_vertex) == goto_list.end())
               {
                  goto_list.insert(false_vertex);
                  add_false_to_goto = true;
               }
               if(bb_analyzed.find(true_vertex) == bb_analyzed.end())
               {
                  EdgeDescriptor e;
                  bool inserted;
                  boost::tie(e, inserted) = boost::edge(current_vertex, true_vertex, *support_cfg);
                  THROW_ASSERT(inserted, "Missing edge");
                  local_inc[true_vertex] = e;
                  writeRoutineInstructions_rec(true_vertex, true);
               }
               else
               {
                  THROW_ASSERT(basic_blocks_labels.find(bb_true_number) != basic_blocks_labels.end(), "I do not know the destination");
                  indented_output_stream->Append("{\n");
                  EdgeDescriptor e;
                  bool inserted;
                  boost::tie(e, inserted) = boost::edge(current_vertex, true_vertex, *bb_fcfgGraph);
                  THROW_ASSERT(inserted, "Edge missing");
                  if(not(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(e)))
                  {
                     unsigned int first_loop_index = support_cfg->CGetBBNodeInfo(boost::source(e, *support_cfg))->loop_id;
                     unsigned int second_loop_index = support_cfg->CGetBBNodeInfo(boost::target(e, *support_cfg))->loop_id;
                     // Different loop
                     if(first_loop_index != second_loop_index)
                     {
                        unsigned int first_depth = 0;
                        unsigned int second_depth = 0;
                        if(first_loop_index)
                           first_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(first_loop_index)->depth;
                        if(second_loop_index)
                           second_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(second_loop_index)->depth;
                        // Second vertex is an header
                        if(first_depth < second_depth)
                        {
                           print_loop_starting(e);
                        }
                        // First vertex is an exit
                        else if(second_depth < first_depth)
                        {
                           print_loop_escaping(e);
                        }
                        // First vertex is an exit, second vertex is an header
                        else
                        {
                           print_loop_switching(e);
                        }
                     }
                     else
                     {
                        print_edge(e, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
                     }
                  }
                  else
                  {
                     print_loop_ending(e);
                  }
                  indented_output_stream->Append("goto " + basic_blocks_labels.find(bb_true_number)->second + ";\n");
                  indented_output_stream->Append("}\n");
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---True edge was not in the frontier");
            }
            else
            {
               indented_output_stream->Append("{\n");
               EdgeDescriptor e;
               bool inserted;
               boost::tie(e, inserted) = boost::edge(current_vertex, true_vertex, *support_cfg);
               unsigned int first_loop_index = support_cfg->CGetBBNodeInfo(boost::source(e, *support_cfg))->loop_id;
               unsigned int second_loop_index = support_cfg->CGetBBNodeInfo(boost::target(e, *support_cfg))->loop_id;
               // Different loop
               if(first_loop_index != second_loop_index)
               {
                  unsigned int first_depth = 0;
                  unsigned int second_depth = 0;
                  if(first_loop_index)
                     first_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(first_loop_index)->depth;
                  if(second_loop_index)
                     second_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(second_loop_index)->depth;
                  // Second vertex is an header
                  if(first_depth < second_depth)
                  {
                     print_loop_starting(e);
                  }
                  // First vertex is an exit
                  else if(second_depth < first_depth)
                  {
                     print_loop_escaping(e);
                  }
                  // First vertex is an exit, second vertex is an header
                  else
                  {
                     print_loop_switching(e);
                  }
               }
               else
               {
                  print_edge(e, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
               }
               indented_output_stream->Append("}\n");
            }
            if(add_false_to_goto)
               goto_list.erase(false_vertex);
            if(bb_frontier.find(false_vertex) == bb_frontier.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "False BB is not on the frontier");
               EdgeDescriptor e;
               bool ins;
               boost::tie(e, ins) = boost::edge(current_vertex, false_vertex, *bb_fcfgGraph);
               if(not(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(e)))
               {
                  indented_output_stream->Append("else\n");
                  if(bb_analyzed.find(false_vertex) == bb_analyzed.end())
                  {
                     local_inc[false_vertex] = e;
                     writeRoutineInstructions_rec(false_vertex, true);
                  }
                  else
                  {
                     THROW_ASSERT(basic_blocks_labels.find(bb_false_number) != basic_blocks_labels.end(), "I do not know the destination");
                     indented_output_stream->Append("{\n");
                     boost::tie(e, ins) = boost::edge(current_vertex, false_vertex, *support_cfg);
                     THROW_ASSERT(ins, "Missing edge");
                     unsigned int first_loop_index = support_cfg->CGetBBNodeInfo(boost::source(e, *support_cfg))->loop_id;
                     unsigned int second_loop_index = support_cfg->CGetBBNodeInfo(boost::target(e, *support_cfg))->loop_id;
                     // Different loop
                     if(first_loop_index != second_loop_index)
                     {
                        unsigned int first_depth = 0;
                        unsigned int second_depth = 0;
                        if(first_loop_index)
                           first_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(first_loop_index)->depth;
                        if(second_loop_index)
                           second_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(second_loop_index)->depth;
                        // Second vertex is an header
                        if(first_depth < second_depth)
                        {
                           print_loop_starting(e);
                        }
                        // First vertex is an exit
                        else if(second_depth < first_depth)
                        {
                           print_loop_escaping(e);
                        }
                        // First vertex is an exit, second vertex is an header
                        else
                        {
                           print_loop_switching(e);
                        }
                     }
                     else
                     {
                        print_edge(e, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
                     }

                     indented_output_stream->Append("   goto " + basic_blocks_labels.find(bb_false_number)->second + ";/*goto6*/\n");
                     indented_output_stream->Append("}\n");
                  }
               }
               /// Feedback edge on the false path of an if
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Target is a header");
                  const vertex target = boost::target(e, *bb_fcfgGraph);
                  const BBNodeInfoConstRef target_bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(target);
                  /// Target is not a while or a for
                  if(target_bb_node_info->statements_list.empty() or ((GET_TYPE(cfgGraph, *(target_bb_node_info->statements_list.rbegin())) & (TYPE_WHILE | TYPE_FOR)) == 0))
                  {
                     indented_output_stream->Append("else\n");
                     indented_output_stream->Append("{\n");

                     print_loop_ending(e);
                     THROW_ASSERT(basic_blocks_labels.find(bb_false_number) != basic_blocks_labels.end(), "I do not know the destination");
                     indented_output_stream->Append("   goto " + basic_blocks_labels.find(bb_false_number)->second + ";/*goto7*/\n");
                     indented_output_stream->Append("}\n");
                  }
               }
            }
            else
            {
               indented_output_stream->Append("else\n");
               indented_output_stream->Append("{\n");
               EdgeDescriptor e;
               bool inserted;
               false_vertex = bb_fcfgGraph->CGetBBGraphInfo()->bb_index_map.find(bb_false_number)->second;
               boost::tie(e, inserted) = boost::edge(current_vertex, false_vertex, *bb_fcfgGraph);
               unsigned int first_loop_index = support_cfg->CGetBBNodeInfo(boost::source(e, *support_cfg))->loop_id;
               unsigned int second_loop_index = support_cfg->CGetBBNodeInfo(boost::target(e, *support_cfg))->loop_id;
               // Different loop
               if(first_loop_index != second_loop_index)
               {
                  unsigned int first_depth = 0;
                  unsigned int second_depth = 0;
                  if(first_loop_index)
                     first_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(first_loop_index)->depth;
                  if(second_loop_index)
                     second_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(second_loop_index)->depth;
                  // Second vertex is an header
                  if(first_depth < second_depth)
                  {
                     print_loop_starting(e);
                  }
                  // First vertex is an exit
                  else if(second_depth < first_depth)
                  {
                     print_loop_escaping(e);
                  }
                  // First vertex is an exit, second vertex is an header
                  else
                  {
                     print_loop_switching(e);
                  }
               }
               else
               {
                  print_edge(e, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
               }
               indented_output_stream->Append("}\n");
            }
         }
         else if(GET_TYPE(cfgGraph, *vIter) & (TYPE_WHILE | TYPE_FOR))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operation is a while or a for");
            const unsigned int bb_true_number = bb_node_info->block->true_edge;
            const vertex true_vertex = bb_index_map.find(bb_true_number)->second;
            if(bb_frontier.find(true_vertex) == bb_frontier.end())
            {
               if(bb_analyzed.find(true_vertex) == bb_analyzed.end())
               {
                  if(analyzed_statement == local_rec_instructions.size())
                     return;
                  EdgeDescriptor e;
                  bool inserted;
                  boost::tie(e, inserted) = boost::edge(current_vertex, true_vertex, *bb_fcfgGraph);
                  THROW_ASSERT(inserted, "Missing edge");
                  local_inc[true_vertex] = e;
                  writeRoutineInstructions_rec(true_vertex, true);
               }
               else
               {
                  THROW_ERROR("Body of a loop has yet been printed before the while statement");
               }
            }
            else
            {
               return;
            }

            const unsigned int bb_false_number = bb_node_info->block->false_edge;
            const vertex false_vertex = bb_index_map.find(bb_false_number)->second;
            EdgeDescriptor e;
            bool inserted;
            boost::tie(e, inserted) = boost::edge(current_vertex, false_vertex, *bb_fcfgGraph);
            unsigned int first_loop_index = support_cfg->CGetBBNodeInfo(boost::source(e, *support_cfg))->loop_id;
            unsigned int second_loop_index = support_cfg->CGetBBNodeInfo(boost::target(e, *support_cfg))->loop_id;
            // Different loop
            if(first_loop_index != second_loop_index)
            {
               unsigned int first_depth = 0;
               unsigned int second_depth = 0;
               if(first_loop_index)
                  first_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(first_loop_index)->depth;
               else
                  THROW_ERROR("Basic block of for or while it is not in a loop");
               if(second_loop_index)
                  second_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(second_loop_index)->depth;
               // Second vertex is an header
               if(first_depth < second_depth)
               {
                  THROW_ERROR("A loop is followed by a loop with higher depth");
               }
               // First vertex is an exit
               else if(second_depth < first_depth)
               {
                  print_loop_escaping(e);
               }
               // First vertex is an exit, second vertex is an header
               else
               {
                  print_loop_switching(e);
               }
            }
            else
            {
               print_edge(e, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
            }

            if(bb_frontier.find(false_vertex) == bb_frontier.end())
            {
               if(bb_analyzed.find(false_vertex) == bb_analyzed.end())
               {
                  writeRoutineInstructions_rec(false_vertex, false);
               }
               else
               {
                  indented_output_stream->Append("goto " + basic_blocks_labels.find(bb_false_number)->second + ";\n");
                  goto_list.insert(false_vertex);
               }
            }
         }
         else if(GET_TYPE(cfgGraph, *vIter) & TYPE_MULTIIF)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operation is a multiif");
            unsigned int node_id = cfgGraph->CGetOpNodeInfo(last_stmt)->GetNodeId();
            const tree_nodeRef node = TM->get_tree_node_const(node_id);
            THROW_ASSERT(node->get_kind() == gimple_multi_way_if_K, "unexpected node");
            auto* gmwi = GetPointer<gimple_multi_way_if>(node);
            std::map<unsigned int, bool> add_elseif_to_goto;
            for(const auto& cond : gmwi->list_of_cond)
            {
               unsigned int bb_index_num = cond.second;
               const vertex bb_vertex = bb_index_map.find(bb_index_num)->second;
               if(cond != gmwi->list_of_cond.front())
               {
                  bool to_be_added = bb_frontier.find(bb_vertex) == bb_frontier.end() && goto_list.find(bb_vertex) == goto_list.end();
                  add_elseif_to_goto[bb_index_num] = to_be_added;
                  if(to_be_added)
                     goto_list.insert(bb_vertex);
               }
               else
                  add_elseif_to_goto[bb_index_num] = false;
            }
            for(const auto& cond : gmwi->list_of_cond)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering successor BB" + STR(cond.second));
               unsigned int bb_index_num = cond.second;
               const vertex bb_vertex = bb_index_map.find(bb_index_num)->second;
               if(cond != gmwi->list_of_cond.front())
               {
                  if(cond.first)
                  {
                     indented_output_stream->Append("else if(");
                     indented_output_stream->Append(behavioral_helper->PrintVariable(GET_INDEX_NODE(cond.first)));
                     indented_output_stream->Append(")\n");
                  }
                  else
                     indented_output_stream->Append("else\n");
               }
               if(add_elseif_to_goto.find(bb_index_num) != add_elseif_to_goto.end() && add_elseif_to_goto.find(bb_index_num)->second)
                  goto_list.erase(bb_vertex);
               if(bb_frontier.find(bb_vertex) == bb_frontier.end())
               {
                  if(bb_analyzed.find(bb_vertex) == bb_analyzed.end())
                  {
                     EdgeDescriptor e;
                     bool inserted;
                     boost::tie(e, inserted) = boost::edge(current_vertex, bb_vertex, *support_cfg);
                     THROW_ASSERT(inserted, "Missing edge");
                     local_inc[bb_vertex] = e;
                     writeRoutineInstructions_rec(bb_vertex, true);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor has already been examined");

                     THROW_ASSERT(basic_blocks_labels.find(bb_index_num) != basic_blocks_labels.end(), "I do not know the destination " + boost::lexical_cast<std::string>(bb_index_num));
                     indented_output_stream->Append("{\n");
                     EdgeDescriptor e;
                     bool inserted;
                     boost::tie(e, inserted) = boost::edge(current_vertex, bb_vertex, *bb_fcfgGraph);
                     THROW_ASSERT(inserted, "Edge missing");
                     if(not(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(e)))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connected by feedback edge");
                        unsigned int first_loop_index = support_cfg->CGetBBNodeInfo(boost::source(e, *support_cfg))->loop_id;
                        unsigned int second_loop_index = support_cfg->CGetBBNodeInfo(boost::target(e, *support_cfg))->loop_id;
                        // Different loop
                        if(first_loop_index != second_loop_index)
                        {
                           unsigned int first_depth = 0;
                           unsigned int second_depth = 0;
                           if(first_loop_index)
                              first_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(first_loop_index)->depth;
                           if(second_loop_index)
                              second_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(second_loop_index)->depth;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Depth of first loop is " + STR(first_depth));
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Depth of second loop is " + STR(second_depth));
                           // Second vertex is an header
                           if(first_depth < second_depth)
                           {
                              print_loop_starting(e);
                           }
                           // First vertex is an exit
                           else if(second_depth < first_depth)
                           {
                              print_loop_escaping(e);
                           }
                           // First vertex is an exit, second vertex is an header
                           else
                           {
                              print_loop_switching(e);
                           }
                        }
                        else
                        {
                           print_edge(e, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
                        }
                     }
                     else
                     {
                        print_loop_ending(e);
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
                  EdgeDescriptor e;
                  bool inserted;
                  boost::tie(e, inserted) = boost::edge(current_vertex, bb_vertex, *support_cfg);
                  unsigned int first_loop_index = support_cfg->CGetBBNodeInfo(boost::source(e, *support_cfg))->loop_id;
                  unsigned int second_loop_index = support_cfg->CGetBBNodeInfo(boost::target(e, *support_cfg))->loop_id;
                  // Different loop
                  if(first_loop_index != second_loop_index)
                  {
                     unsigned int first_depth = 0;
                     unsigned int second_depth = 0;
                     if(first_loop_index)
                        first_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(first_loop_index)->depth;
                     if(second_loop_index)
                        second_depth = AppM->CGetFunctionBehavior(funId)->CGetLoops()->CGetLoop(second_loop_index)->depth;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Depth of first loop is " + STR(first_depth));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Depth of second loop is " + STR(second_depth));
                     // Second vertex is an header
                     if(first_depth < second_depth)
                     {
                        print_loop_starting(e);
                     }
                     // First vertex is an exit
                     else if(second_depth < first_depth)
                     {
                        print_loop_escaping(e);
                     }
                     // First vertex is an exit, second vertex is an header
                     else
                     {
                        print_loop_switching(e);
                     }
                  }
                  else
                  {
                     print_edge(e, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
                  }
                  indented_output_stream->Append("}\n");
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered successor BB" + STR(cond.second));
            }
         }
         else if(GET_TYPE(cfgGraph, *vIter) & TYPE_SWITCH)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operation is a switch");
            /// now we can analyze the following basic blocks
            indented_output_stream->Append("{\n");
            OutEdgeIterator oE, oEEnd;
            for(boost::tie(oE, oEEnd) = boost::out_edges(current_vertex, *bb_fcfgGraph); oE != oEEnd; oE++)
            {
               bool empty_block = false;
               vertex next_bb = boost::target(*oE, *bb_fcfgGraph);
               const BBNodeInfoConstRef next_bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(next_bb);
#ifndef NDEBUG
               const unsigned int bb_number_next_bb = next_bb_node_info->block->number;
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Examining successor " + boost::lexical_cast<std::string>(bb_number_next_bb));
               unsigned int in_edge_counter = 0;
               InEdgeIterator ei, ei_end;
               for(boost::tie(ei, ei_end) = boost::in_edges(next_bb, *bb_fcfgGraph); ei != ei_end; ei++)
               {
                  if(not(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(*ei)))
                     in_edge_counter++;
               }
               CustomOrderedSet<unsigned int>::const_iterator eIdBeg, eIdEnd;
               CustomOrderedSet<unsigned int> Set = bb_fcfgGraph->CGetBBEdgeInfo(*oE)->get_labels(CFG_SELECTOR);
               for(eIdBeg = Set.begin(), eIdEnd = Set.end(); eIdBeg != eIdEnd; ++eIdBeg)
               {
                  if(*eIdBeg == default_COND)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor is default case");
                     if(next_bb == post_dominators->get_immediate_dominator(current_vertex))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Default is empty");
                        empty_block = true;
                        indented_output_stream->Append("default:\n");
                        indented_output_stream->Append("{\n");
                        print_edge(*oE, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
                        indented_output_stream->Append("}\n");
                        if(current_vertex == dominators->get_immediate_dominator(next_bb))
                           analyze_bb_PD = true;
                        break;
                     }
                     local_inc[next_bb] = *oE;
                     if(in_edge_counter != 1)
                     {
                        THROW_ERROR_CODE(PROFILING_EC, "Not yet supported type of switch in profiling");
                        THROW_ERROR("Not yet supported type of switch in profiling");
                     }
                     indented_output_stream->Append("default");
                  }
                  else
                  {
                     local_inc[next_bb] = *oE;
                     if(in_edge_counter != 1)
                     {
                        THROW_ERROR_CODE(PROFILING_EC, "Not yet supported type of switch in profiling - Function is " + boost::lexical_cast<std::string>(funId) + " - current basic block is " + boost::lexical_cast<std::string>(bb_number));
                        THROW_ERROR("Not yet supported type of switch in profiling");
                     }
                     if(next_bb == bb_PD) /// then adjust post dominator
                     {
                        empty_block = true;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removed current basic block");
                     }

                     indented_output_stream->Append("case " + behavioral_helper->print_constant(*eIdBeg));
                  }
                  indented_output_stream->Append(":\n");
               }
               if(empty_block)
               {
                  indented_output_stream->Append("break;\n");
                  continue;
               }
               if(bb_analyzed.find(next_bb) == bb_analyzed.end())
               {
                  writeRoutineInstructions_rec(next_bb, true);
                  indented_output_stream->Append("break;\n");
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor has already been examined");
                  const unsigned int next_bb_number = next_bb_node_info->block->number;

                  THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(), "I do not know the destination " + boost::lexical_cast<std::string>(next_bb_number));
                  indented_output_stream->Append("   goto " + basic_blocks_labels.find(next_bb_number)->second + ";/*goto8*/\n");
               }
            }
            indented_output_stream->Append("}\n");
         }
         else if(behavioral_helper->end_with_a_cond_or_goto(bb_node_info->block))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block ends with a cond or a goto");
            if(last_statement_is_a_cond_or_goto)
            {
               /// now we can analyze the following basic blocks
               OutEdgeIterator oE, oEEnd;
               for(boost::tie(oE, oEEnd) = boost::out_edges(current_vertex, *bb_fcfgGraph); oE != oEEnd; oE++)
               {
                  vertex next_bb = boost::target(*oE, *bb_fcfgGraph);
                  if(boost::out_degree(current_vertex, *bb_fcfgGraph) > 1)
                  {
                     THROW_ERROR_CODE(PROFILING_EC, "Profiling does not support computed goto");
                     THROW_ERROR("Profiling does not support computed goto");
                  }
                  if(bb_frontier.find(next_bb) != bb_frontier.end())
                     continue;
                  goto_list.insert(next_bb);
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is not a special operation");
            const vertex bbentry = bb_fcfgGraph->CGetBBGraphInfo()->entry_vertex;
            if(current_vertex == bbentry)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended writing basic block " + boost::lexical_cast<std::string>(bb_number));
               return;
            }
            THROW_ASSERT(boost::out_degree(current_vertex, *bb_fcfgGraph) <= 1, "Only one edge expected as output of BB" + STR(bb_fcfgGraph->CGetBBNodeInfo(current_vertex)->block->number));
            OutEdgeIterator oE, oEEnd;
            for(boost::tie(oE, oEEnd) = boost::out_edges(current_vertex, *bb_fcfgGraph); oE != oEEnd; oE++)
            {
               vertex next_bb = boost::target(*oE, *bb_fcfgGraph);
               if(bb_frontier.find(next_bb) != bb_frontier.end())
               {
                  if(bb_fcfgGraph->CGetBBNodeInfo(current_vertex)->loop_id != bb_fcfgGraph->CGetBBNodeInfo(next_bb)->loop_id)
                     print_loop_starting(*oE);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not adding goto since target is in the frontier");
                  continue;
               }
               if(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(*oE))
               {
                  print_loop_ending(*oE);
                  const vertex target = boost::target(*oE, *bb_fcfgGraph);
                  const BBNodeInfoConstRef target_bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(target);
                  if(target_bb_node_info->statements_list.size() and (GET_TYPE(cfgGraph, *(target_bb_node_info->statements_list.rbegin())) & (TYPE_WHILE | TYPE_FOR)))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not adding a goto since target is a while/for");
                     continue;
                  }
               }
               if(bb_fcfgGraph->CGetBBNodeInfo(current_vertex)->loop_id != bb_fcfgGraph->CGetBBNodeInfo(next_bb)->loop_id)
                  print_loop_starting(*oE);
               if(boost::in_degree(next_bb, *bb_fcfgGraph) == 1)
               {
                  writeRoutineInstructions_rec(next_bb, false);
               }
               else
               {
                  const BBNodeInfoConstRef next_bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(next_bb);
                  const unsigned int next_bb_number = next_bb_node_info->block->number;
                  THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(), "I do not know the destination");
                  indented_output_stream->Append("   goto " + basic_blocks_labels.find(next_bb_number)->second + ";/*Goto4*/\n");
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
      if(!behavioral_helper->end_with_a_cond_or_goto(bb_node_info->block) && ((bb_node_info->statements_list.empty()) || ((GET_TYPE(cfgGraph, *bb_node_info->statements_list.rbegin()) & (TYPE_SWITCH | TYPE_WHILE | TYPE_FOR)) == 0)))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not end with a cond or goto nor switch");
         const vertex bbentry = bb_fcfgGraph->CGetBBGraphInfo()->entry_vertex;
         if(current_vertex == bbentry)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended writing basic block " + boost::lexical_cast<std::string>(bb_number));
            return;
         }
         THROW_ASSERT(boost::out_degree(current_vertex, *bb_fcfgGraph) <= 1, "only one edge expected BB(" + boost::lexical_cast<std::string>(bb_number) + ") Fun(" + boost::lexical_cast<std::string>(funId) + ")");
         OutEdgeIterator oE, oEEnd;
         for(boost::tie(oE, oEEnd) = boost::out_edges(current_vertex, *bb_fcfgGraph); oE != oEEnd; oE++)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Examining the only? successor");
            vertex next_bb = boost::target(*oE, *bb_fcfgGraph);
            if(boost::in_degree(next_bb, *bb_fcfgGraph) == 1)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor is the only");
               if(bb_fcfgGraph->CGetBBNodeInfo(current_vertex)->loop_id != bb_fcfgGraph->CGetBBNodeInfo(next_bb)->loop_id)
                  print_loop_starting(*oE);
               continue;
            }
            if(bb_frontier.find(next_bb) != bb_frontier.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor belongs to frontier");
               if(bb_fcfgGraph->CGetBBNodeInfo(current_vertex)->loop_id != bb_fcfgGraph->CGetBBNodeInfo(next_bb)->loop_id)
                  print_loop_starting(*oE);
               else
               {
                  print_edge(*oE, fun_loop_to_index.find(funId)->second.find(bb_node_info->loop_id)->second);
               }
               continue;
            }
            /// Last basic block of a while/for loop
            if(FB_CFG_SELECTOR & bb_fcfgGraph->GetSelector(*oE))
            {
               const vertex target = boost::target(*oE, *bb_fcfgGraph);
               const BBNodeInfoConstRef target_bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(target);
               if(target_bb_node_info->statements_list.size() and (GET_TYPE(cfgGraph, *(target_bb_node_info->statements_list.rbegin())) & (TYPE_WHILE | TYPE_FOR)))
                  continue;
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor does not belong to frontier");
            const BBNodeInfoConstRef next_bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(next_bb);
            const unsigned int next_bb_number = next_bb_node_info->block->number;
            THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(), "I do not know the destination");
            if(bb_fcfgGraph->CGetBBNodeInfo(current_vertex)->loop_id != bb_fcfgGraph->CGetBBNodeInfo(next_bb)->loop_id)
               print_loop_starting(*oE);
            if(bb_fcfgGraph->CGetBBNodeInfo(next_bb)->loop_id == bb_fcfgGraph->CGetBBNodeInfo(next_bb)->block->number)
            {
               print_loop_ending(*oE);
            }
            indented_output_stream->Append("   goto " + basic_blocks_labels.find(next_bb_number)->second + ";/*goto5*/\n");
            goto_list.insert(next_bb);
            add_semicolon = false;
         }
         if(add_semicolon)
         {
            indented_output_stream->Append(";\n"); /// added a fake indent
         }
      }
      else if(add_semicolon)
         indented_output_stream->Append("   ;\n"); /// added a fake indent
   }

   if(analyze_bb_PD)
   {
      // recurse on the post dominator
      bb_frontier.erase(bb_PD);
      THROW_ASSERT(bb_analyzed.find(bb_PD) == bb_analyzed.end(), "something of wrong happen " + boost::lexical_cast<std::string>(bb_fcfgGraph->CGetBBNodeInfo(bb_PD)->block->number) + " Fun(" + boost::lexical_cast<std::string>(funId) + ")");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Printing the post dominator");
      writeRoutineInstructions_rec(bb_PD, false);
   }
   if((analyze_bb_PD || is_there || add_bb_label || add_phi_nodes_assignment || add_phi_nodes_assignment_prefix) && bracket)
   {
      indented_output_stream->Append("}\n");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--BB" + boost::lexical_cast<std::string>(bb_number) + " written");
}

void EdgeCWriter::writeRoutineInstructions(const unsigned int function_index, const OpVertexSet& instructions, const var_pp_functorConstRef variableFunctor, vertex bb_start, CustomOrderedSet<vertex> bb_end)
{
   const FunctionBehaviorConstRef function_behavior = AppM->CGetFunctionBehavior(function_index);
   const BehavioralHelperConstRef behavioral_helper = function_behavior->CGetBehavioralHelper();
   const BBGraphConstRef bb_fcfgGraph = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   fun_id = behavioral_helper->get_function_index();
   support_cfg = function_behavior->CGetBBGraph(FunctionBehavior::PPG);
   local_inc.clear();
   dumped_edges = std::set<EdgeDescriptor, ltedge<BBGraph>>(ltedge<BBGraph>(support_cfg.get()));

   const OpGraphConstRef cfgGraph = function_behavior->CGetOpGraph(FunctionBehavior::FCFG);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Edge profiling writer - start to fwrite body of function " + behavioral_helper->get_function_name());
   if(instructions.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Edge profiling writer - Empty function");
      return;
   }
   else if(instructions.size() == 1)
   {
      if(GET_TYPE(cfgGraph, (*instructions.begin())) & TYPE_ENTRY)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Edge profiling writer - Empty function");
         return;
      }
   }
   /// Then I compute all the labels associated with a basic block with more than one entering edge.
   basic_blocks_labels.clear();
   VertexIterator vi, vi_end;
   vertex bbentry;
   CustomOrderedSet<vertex> bb_exit;
   if(!bb_start)
      bbentry = bb_fcfgGraph->CGetBBGraphInfo()->entry_vertex;
   else
      bbentry = bb_start;
   if(bb_end.empty())
   {
      bb_exit.insert(bb_fcfgGraph->CGetBBGraphInfo()->exit_vertex);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "There are " + boost::lexical_cast<std::string>(bb_exit.size()) + " exit basic blocks");
   }
   else
   {
      bb_exit = bb_end;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing labels");
   for(boost::tie(vi, vi_end) = boost::vertices(*bb_fcfgGraph); vi != vi_end; vi++)
   {
      size_t delta = bb_exit.find(*vi) != bb_exit.end() ? 1u : 0u;
      if(boost::in_degree(*vi, *bb_fcfgGraph) <= (1 + delta))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped BB" + boost::lexical_cast<std::string>(bb_fcfgGraph->CGetBBNodeInfo(*vi)->block->number));
         continue;
      }
      const BBNodeInfoConstRef bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(*vi);
      const unsigned int le = behavioral_helper->start_with_a_label(bb_node_info->block);
      basic_blocks_labels[bb_node_info->block->number] =
          (le ? behavioral_helper->get_label_name(le) : ("BB_LABEL_" + boost::lexical_cast<std::string>(bb_node_info->block->number)) + (bb_label_counter == 1 ? "" : "_" + boost::lexical_cast<std::string>(bb_label_counter)));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Label of BB" + boost::lexical_cast<std::string>(bb_fcfgGraph->CGetBBNodeInfo(*vi)->block->number) + " is " + basic_blocks_labels[bb_node_info->block->number]);
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
   local_rec_variableFunctor = variableFunctor;
   local_rec_function_behavior = function_behavior;
   local_rec_instructions.clear();
   local_rec_instructions.insert(instructions.begin(), instructions.end());
   dominators = local_rec_function_behavior->dominators;
   post_dominators = local_rec_function_behavior->post_dominators;

   OutEdgeIterator oE, oEEnd;
   /// some statements can be in entry
   writeRoutineInstructions_rec(bbentry, false);
   if(!bb_start && bb_end.size() == 0)
   {
      for(boost::tie(oE, oEEnd) = boost::out_edges(bbentry, *bb_fcfgGraph); oE != oEEnd; oE++)
      {
         if(bb_exit.find(boost::target(*oE, *bb_fcfgGraph)) != bb_exit.end())
            continue;
         else
         {
            writeRoutineInstructions_rec(boost::target(*oE, *bb_fcfgGraph), false);
         }
      }
   }
   CustomOrderedSet<vertex> not_yet_considered;
   std::set_difference(goto_list.begin(), goto_list.end(),                           /*first set*/
                       bb_analyzed.begin(), bb_analyzed.end(),                       /*second set*/
                       std::inserter(not_yet_considered, not_yet_considered.begin()) /*result*/
   );
   while(!not_yet_considered.empty())
   {
      vertex next_bb = *not_yet_considered.begin();
      not_yet_considered.erase(next_bb);
      writeRoutineInstructions_rec(next_bb, false);
      not_yet_considered.clear();
      std::set_difference(goto_list.begin(), goto_list.end(),     /*first set*/
                          bb_analyzed.begin(), bb_analyzed.end(), /*second set*/
                          std::inserter(not_yet_considered, not_yet_considered.begin()) /*result*/);
   }
   const vertex exit = bb_fcfgGraph->CGetBBGraphInfo()->exit_vertex;
   if(goto_list.find(exit) != goto_list.end() && basic_blocks_labels.find(bloc::EXIT_BLOCK_ID) != basic_blocks_labels.end())
   {
      indented_output_stream->Append(basic_blocks_labels.find(bloc::EXIT_BLOCK_ID)->second + ":\n");
   }
   EdgeIterator e, e_end;
   for(boost::tie(e, e_end) = boost::edges(*support_cfg); e != e_end; e++)
   {
      if(dumped_edges.find(*e) == dumped_edges.end() && boost::source(*e, *support_cfg) != support_cfg->CGetBBGraphInfo()->entry_vertex && boost::target(*e, *support_cfg) != support_cfg->CGetBBGraphInfo()->exit_vertex)
      {
         WriteFile("Error.c");
         THROW_ERROR_CODE(PROFILING_EC, "Profiling Instrumentation of Edge of function " + behavioral_helper->get_function_name() + " from vertex BB" +
                                            boost::lexical_cast<std::string>(support_cfg->CGetBBNodeInfo(boost::source(*e, *support_cfg))->block->number) + " to BB" +
                                            boost::lexical_cast<std::string>(support_cfg->CGetBBNodeInfo(boost::target(*e, *support_cfg))->block->number) + " not printed");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Edge profiling writer - ended");
}

const std::map<unsigned int, std::map<unsigned int, unsigned int>>& EdgeCWriter::CGetFunctionLoopToId() const
{
   return fun_loop_to_index;
}

void EdgeCWriter::WriteHeader()
{
   indented_output_stream->Append("void __builtin_bambu_time_start(){}\n");
   indented_output_stream->Append("void __builtin_bambu_time_stop(){}\n\n");
   indented_output_stream->Append("#define __builtin_cond_expr32(cond, value1, value2) cond ? value1 : value2\n\n");
   indented_output_stream->Append("#define __builtin___divsc3 __divsc3\n");
   indented_output_stream->Append("#define __builtin___divdc3 __divdc3\n");
}
