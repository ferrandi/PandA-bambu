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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

// include class header
#include "HWDiscrepancyAnalysis.hpp"

#include "config_HAVE_ASSERTS.hpp"

// include from ./
#include "Parameter.hpp"

// include from behavior/
#include "basic_block.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

// includes from design_flows/backend/ToHDL
#include "language_writer.hpp"

// include from HLS/
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

// include from HLS/stg/
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

// include from HLS/vcd/
#include "Discrepancy.hpp"

// include from parser/discrepancy/
#include "parse_discrepancy.hpp"

/// STD include
#include <algorithm>
#include <limits>
#include <list>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "custom_map.hpp"
#include "custom_set.hpp"

// include from tree/
#include "behavioral_helper.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "structural_manager.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/tokenizer.hpp>

/// MAX metadata bit size
#define MAX_METADATA_BITSIZE 10

/// FIXED metadata bit size
#define FIXED_METADATA_SIZE 6

HWDiscrepancyAnalysis::HWDiscrepancyAnalysis(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::HW_DISCREPANCY_ANALYSIS),
      Discr(_HLSMgr->RDiscr),
      present_state_name(static_cast<HDLWriter_Language>(_parameters->getOption<unsigned int>(OPT_writer_language)) == HDLWriter_Language::VERILOG ? "_present_state" : "present_state")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> HWDiscrepancyAnalysis::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::C_TESTBENCH_EXECUTION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::CALL_GRAPH_UNFOLDING, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::HW_PATH_COMPUTATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
         break;
      }
   }
   return ret;
}

DesignFlowStep_Status HWDiscrepancyAnalysis::Exec()
{
   THROW_ASSERT(Discr, "Discr data structure is not correctly initialized");
   // parse the file containing the C traces
   {
      const std::string& ctrace_filename = Discr->c_trace_filename;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Parsing C trace: " + ctrace_filename);
      parse_discrepancy(ctrace_filename, Discr);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Parsed C trace: " + ctrace_filename);
   }
#ifndef NDEBUG
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Printing parsed C control flow trace");
      for(const auto& fid2ctxtrace : Discr->c_control_flow_trace)
      {
         const auto f_id = fid2ctxtrace.first;
         const std::string f_name = tree_helper::name_function(HLSMgr->get_tree_manager(), f_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->FUNCTION_ID " + STR(f_id) + ": " + f_name);
         for(const auto& ctx2trace : fid2ctxtrace.second)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->CONTEXT: " + STR(ctx2trace.first));
            for(const auto& bb_id : ctx2trace.second)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---BB: " + STR(bb_id));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--End of parsed C control flow trace");
   }
#endif
   // untangle control flow traces
   CustomUnorderedMap<std::string, std::list<size_t>> scope_to_epp_trace;
   CustomUnorderedMap<std::string, std::list<std::pair<size_t, size_t>>> scope_to_best_epp_trace_with_metadata;
   CustomUnorderedMap<std::string, std::list<unsigned int>> scope_to_state_trace;
   CustomUnorderedMapUnstable<std::string, unsigned int> scope_to_function_id;
   CustomUnorderedMap<std::string, size_t> scope_to_best_metadata_bits;
   std::vector<size_t> tot_memory_usage_per_bits = std::vector<size_t>(MAX_METADATA_BITSIZE, 0);
   size_t min_memory_usage = 0;
   size_t total_state_of_the_art_memory_usage = 0;
   size_t total_state_of_the_art_fixed_memory_usage = 0;
   size_t total_state_of_the_art_opt_memory_usage = 0;
   for(const auto& f : Discr->c_control_flow_trace)
   {
      const auto f_id = f.first;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Untangling software control flow trace of function: " + STR(f_id) + " " + HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->get_function_name());
      const auto& stg = HLSMgr->get_HLS(f_id)->STG->CGetStg();
      const auto& epp_stg = HLSMgr->get_HLS(f_id)->STG->CGetEPPStg();
      const auto& stg_info = stg->CGetStateTransitionGraphInfo();
      const auto fsm_entry_node = stg_info->entry_node;
      const auto fsm_exit_node = stg_info->exit_node;
      const auto& state_id_to_check = Discr->hw_discrepancy_info->fu_id_to_states_to_check.at(f_id);
#if HAVE_ASSERTS
      const auto& state_id_to_check_on_feedback = Discr->hw_discrepancy_info->fu_id_to_feedback_states_to_check.at(f_id);
      const auto& reset_transitions = Discr->hw_discrepancy_info->fu_id_to_reset_edges.at(f_id);
#endif

      for(auto& context2trace : f.second)
      {
         const auto context_id = context2trace.first;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing context: " + STR(context_id));
         const auto& scope = Discr->context_to_scope.at(context_id);
         scope_to_function_id[scope] = f_id;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope: " + scope);

         size_t epp_counter = 0;
         vertex current_state = fsm_entry_node;
         bool takes_feedback_edge = false;
         {
            /*
             * the entry state is not a real FSM state, so it is never to check.
             * but it may have a non zero increment on its single outgoing edge,
             * so we must check it and potentially add it to the epp_counter
             */
            THROW_ASSERT(boost::out_degree(current_state, *stg) == 1, "entry state has out degree = " + STR(boost::out_degree(current_state, *stg)));
            // select the only outgoing edge (must not be a feedback edge)
            OutEdgeIterator out_edge, out_end;
            boost::tie(out_edge, out_end) = boost::out_edges(current_state, *stg);
            const EdgeDescriptor taken_edge = *out_edge;
            THROW_ASSERT(reset_transitions.find(taken_edge) == reset_transitions.cend(), "");
            // update epp counter
            const auto taken_edge_info = stg->CGetTransitionInfo(taken_edge);
            epp_counter += taken_edge_info->get_epp_increment();
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "epp_counter " + STR(epp_counter));
            // update the current_state with the only successor of entry
            current_state = boost::target(taken_edge, *stg);
            // check that the first state is not a loop header
            THROW_ASSERT(state_id_to_check_on_feedback.find(stg_info->vertex_to_state_id.at(current_state)) == state_id_to_check_on_feedback.cend(), "");
         }
         if(HLSMgr->get_HLS(f_id)->registered_inputs)
         {
            /*
             * if the FSM has registered inputs, the first real state of the FSM
             * is spent registering the inputs and for this reason is not mapped
             * on any real BB identifier.
             * like the entry state, this is not a real FSM state, so it is never to check.
             * but it may have a non zero increment on its single outgoing edge,
             * so we must check it and potentially add it to the epp_counter
             */
            THROW_ASSERT(boost::out_degree(current_state, *stg) == 1, "registered input state has out degree = " + STR(boost::out_degree(current_state, *stg)));
            THROW_ASSERT(state_id_to_check.find(stg_info->vertex_to_state_id.at(current_state)) == state_id_to_check.cend(), "");
            // select the only outgoing edge (must not be a feedback edge)
            OutEdgeIterator out_edge, out_end;
            boost::tie(out_edge, out_end) = boost::out_edges(current_state, *stg);
            const EdgeDescriptor taken_edge = *out_edge;
            THROW_ASSERT(reset_transitions.find(taken_edge) == reset_transitions.cend(), "");
            // update epp counter
            const auto taken_edge_info = stg->CGetTransitionInfo(taken_edge);
            epp_counter += taken_edge_info->get_epp_increment();
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "epp_counter " + STR(epp_counter));
            // update the current_state with the only successor of entry
            current_state = boost::target(taken_edge, *stg);
            // check that the first state is not a loop header
            const auto next_state_id = stg_info->vertex_to_state_id.at(current_state);
            scope_to_state_trace[scope].push_back(next_state_id);
            THROW_ASSERT(state_id_to_check_on_feedback.find(next_state_id) == state_id_to_check_on_feedback.cend(), "");
         }
         const std::list<unsigned int>& bb_trace = context2trace.second;
         auto bb_id_it = bb_trace.cbegin();
         const auto bb_id_end = bb_trace.cend();
         for(; bb_id_it != bb_id_end; bb_id_it++)
         {
            const auto bb_id = *bb_id_it;
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->BB: " + STR(bb_id));
            if(bb_id == BB_ENTRY)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
               continue;
            }
            bool goes_to_next_bb_execution = false;
            do
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---find next state with bb_id: " + STR(bb_id));
               const auto cur_state_id = stg_info->vertex_to_state_id.at(current_state);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---current_state is: " + STR(cur_state_id));
               // if the current state is to check, write the trace
               if(not takes_feedback_edge and state_id_to_check.find(cur_state_id) != state_id_to_check.cend())
               {
                  scope_to_epp_trace[scope].push_back(epp_counter);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "epp_trace " + STR(epp_counter));
               }
               /* detect the taken edge and the next state  */
               bool found_valid_next_state = false;
               bool end_of_bb_execution = false;
               takes_feedback_edge = false;
               vertex next_state = nullptr;
               EdgeDescriptor taken_edge(NULL_VERTEX, NULL_VERTEX, nullptr);
               // look for the next state belonging to the same bb, without following a feedback edge
               BOOST_FOREACH(const EdgeDescriptor e, boost::out_edges(current_state, *stg))
               {
                  const auto dst = boost::target(e, *stg);
                  const auto dst_info = stg->CGetStateInfo(dst);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---analyzing next state: " + ((dst == fsm_exit_node) ? STR("EXIT") : STR(stg_info->vertex_to_state_id.at(dst))) + " in the same BB " + STR(bb_id));
                  if((not dst_info->is_dummy) and (*dst_info->BB_ids.begin() == bb_id))
                  {
                     if(not(stg->GetSelector(e) & TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK))
                     {
                        THROW_ASSERT(not found_valid_next_state, "two neighbors of state S_" + STR(stg_info->vertex_to_state_id.at(current_state)) + " have the same BB id = " + STR(bb_id));
                        found_valid_next_state = true;
                        taken_edge = e;
                        next_state = dst;
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---FOUND next state: " + STR(stg_info->vertex_to_state_id.at(next_state)) + " in the same BB " + STR(bb_id));
                     }
                  }
                  else if(dst_info->is_dummy)
                  {
                     scope_to_state_trace[scope].push_back(stg_info->vertex_to_state_id.at(dst));
                  }
               }
               // if the next state was not found in the same BB without feedback edges there are two cases
               if(not found_valid_next_state)
               {
                  if(std::next(bb_id_it) != bb_id_end)
                  {
                     // the execution branches to another BB (or to the same with a feedback edge)
                     const auto next_bb_id = *std::next(bb_id_it);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---look for next BB " + STR(next_bb_id));
                     BOOST_FOREACH(const EdgeDescriptor e, boost::out_edges(current_state, *stg))
                     {
                        const auto dst = boost::target(e, *stg);
                        const auto dst_info = stg->CGetStateInfo(dst);
                        if((not dst_info->is_dummy) and (*dst_info->BB_ids.begin() == next_bb_id))
                        {
                           THROW_ASSERT(not found_valid_next_state, "two neighbors of state S_" + STR(stg_info->vertex_to_state_id.at(current_state)) + " have the same BB id = " + STR(next_bb_id));
                           found_valid_next_state = true;
                           end_of_bb_execution = true;
                           taken_edge = e;
                           next_state = dst;
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---FOUND next state: " + STR(stg_info->vertex_to_state_id.at(next_state)) + " in new BB " + STR(next_bb_id));
                           if(stg->GetSelector(taken_edge) & TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK)
                           {
                              takes_feedback_edge = true;
                           }
                        }
                        else if(dst_info->is_dummy)
                        {
                           scope_to_state_trace[scope].push_back(stg_info->vertex_to_state_id.at(dst));
                        }
                     }
                  }
                  else
                  {
                     // the execution ends
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---look for EXIT");
                     BOOST_FOREACH(const EdgeDescriptor e, boost::out_edges(current_state, *stg))
                     {
                        const auto dst = boost::target(e, *stg);
                        if(dst == fsm_exit_node)
                        {
                           found_valid_next_state = true;
                           end_of_bb_execution = true;
                           taken_edge = e;
                           next_state = dst;
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---found next state: EXIT");
                        }
                     }
                  }
                  THROW_ASSERT(end_of_bb_execution, "");
               }
               THROW_ASSERT(found_valid_next_state, "next valid state not found");
               THROW_ASSERT(takes_feedback_edge == (reset_transitions.find(taken_edge) != reset_transitions.cend()), "");
               if(next_state != fsm_exit_node)
               {
                  const auto next_state_id = stg_info->vertex_to_state_id.at(next_state);
                  scope_to_state_trace[scope].push_back(next_state_id);
                  THROW_ASSERT((not takes_feedback_edge) or (state_id_to_check_on_feedback.find(next_state_id) != state_id_to_check_on_feedback.cend()), "");
               }
               const auto taken_edge_info = epp_stg->CGetTransitionInfo(taken_edge);
               epp_counter += taken_edge_info->get_epp_increment();
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "epp_counter " + STR(epp_counter));

               if(takes_feedback_edge)
               {
                  /*
                   * the execution takes a feedback edge. feedback edges have no
                   * associated epp_increment. to compute increments and reset
                   * values we have to look onto the EPP edges.
                   * the reset value is equal to the epp_increment on the EPP
                   * edge going from the entry node of the FSM to the target of
                   * the feedback edge.
                   * the increment value before reset is the epp_increment of
                   * the EPP edge going from the source of the feedback edge to
                   * the exit node of the STG.
                   * note that these entry and exit nodes are not real states of
                   * the FSM but just placeholders.
                   */
                  const auto src = boost::source(taken_edge, *stg);
                  const auto dst = boost::target(taken_edge, *stg);
                  const auto epp_edge_from_entry = epp_stg->CGetEdge(fsm_entry_node, dst);
                  const auto epp_edge_to_exit = epp_stg->CGetEdge(src, fsm_exit_node);
                  const auto from_entry_edge_info = epp_stg->CGetTransitionInfo(epp_edge_from_entry);
                  const auto to_exit_edge_info = epp_stg->CGetTransitionInfo(epp_edge_to_exit);
                  scope_to_epp_trace[scope].push_back(epp_counter + to_exit_edge_info->get_epp_increment());
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "epp_trace " + STR(epp_counter + to_exit_edge_info->get_epp_increment()));
                  epp_counter = from_entry_edge_info->get_epp_increment();
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "reset epp_counter " + STR(epp_counter));
               }
               current_state = next_state;
               goes_to_next_bb_execution = end_of_bb_execution;
            } while(not goes_to_next_bb_execution);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed context: " + STR(context_id));
      }

      // state_of_the_art memory usage
      {
         std::string vendor;
         const auto tgt_device = HLSMgr->get_HLS_target()->get_target_device();
         if(tgt_device->has_parameter("vendor"))
         {
            vendor = tgt_device->get_parameter<std::string>("vendor");
            boost::algorithm::to_lower(vendor);
         }
         unsigned int n_states = HLSMgr->get_HLS(f_id)->STG->get_number_of_states();
         bool one_hot_encoding = false;
         if(parameters->getOption<std::string>(OPT_fsm_encoding) == "one-hot")
            one_hot_encoding = true;
         else if(parameters->getOption<std::string>(OPT_fsm_encoding) == "auto" && vendor == "xilinx" && n_states < 256)
            one_hot_encoding = true;

         unsigned int bitsnumber = language_writer::bitnumber(n_states - 1);
         /// adjust in case states are not consecutive
         unsigned max_value = 0;
         for(const auto& s : stg_info->state_id_to_vertex)
            max_value = std::max(max_value, s.first);
         if(max_value != n_states - 1)
            bitsnumber = language_writer::bitnumber(max_value);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " ONE HOT" + STR(one_hot_encoding ? "  true" : " false"));

         const unsigned int state_bitsize = one_hot_encoding ? (max_value + 1) : bitsnumber;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " STATE BITSIZE " + STR(state_bitsize));

         size_t f_state_of_the_art_usage = 0;
         size_t f_state_of_the_art_usage_fixed = 0;
         size_t f_state_of_the_art_usage_opt = 0;
         for(const auto& scope : Discr->f_id_to_scope.at(f_id))
         {
            if(scope_to_state_trace.find(scope) == scope_to_state_trace.end())
               continue;
            const auto& state_trace = scope_to_state_trace.at(scope);
            size_t scope_memory_usage = state_bitsize * state_trace.size();
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope " + scope + " state_of_the_art memory usage (BITS): " + STR(scope_memory_usage));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope " + scope + " state_of_the_art memory usage (BYTES): " + STR((scope_memory_usage / 8) + (((scope_memory_usage % 8) == 0) ? 0 : 1)));
            f_state_of_the_art_usage += scope_memory_usage;

            size_t prev_state = std::numeric_limits<std::size_t>::max() - 1;
            size_t incremental_counter_fixed = 0;
            CustomMap<unsigned int, size_t> incremental_counter_opt;
            size_t scope_memory_usage_fixed = 0;
            CustomMap<unsigned int, size_t> scope_memory_usage_opt;
            for(const auto state_id : state_trace)
            {
               if(state_id != (prev_state + 1))
               {
                  scope_memory_usage_fixed += state_bitsize + FIXED_METADATA_SIZE;
                  for(unsigned int metadata_bitsize = 0; metadata_bitsize < MAX_METADATA_BITSIZE; metadata_bitsize++)
                  {
                     scope_memory_usage_opt[metadata_bitsize] += state_bitsize + metadata_bitsize;
                  }
                  incremental_counter_fixed = 0;
                  for(unsigned int metadata_bitsize = 0; metadata_bitsize < MAX_METADATA_BITSIZE; metadata_bitsize++)
                  {
                     incremental_counter_opt[metadata_bitsize] = 0;
                  }
               }
               else
               {
                  incremental_counter_fixed++;
                  for(unsigned int metadata_bitsize = 0; metadata_bitsize < MAX_METADATA_BITSIZE; metadata_bitsize++)
                  {
                     incremental_counter_opt[metadata_bitsize]++;
                  }
                  if(incremental_counter_fixed >= (1ULL << FIXED_METADATA_SIZE))
                  {
                     scope_memory_usage_fixed += state_bitsize + FIXED_METADATA_SIZE;
                     incremental_counter_fixed = 0;
                  }
                  for(unsigned int metadata_bitsize = 0; metadata_bitsize < MAX_METADATA_BITSIZE; metadata_bitsize++)
                  {
                     if(incremental_counter_opt[metadata_bitsize] >= (1ULL << metadata_bitsize))
                     {
                        scope_memory_usage_opt[metadata_bitsize] += state_bitsize + metadata_bitsize;
                        incremental_counter_opt[metadata_bitsize] = 0;
                     }
                  }
               }
               prev_state = state_id;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope " + scope + " state_of_the_art fixed memory usage (BITS): " + STR(scope_memory_usage_fixed));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope " + scope + " state_of_the_art fixed memory usage (BYTES): " + STR((scope_memory_usage_fixed / 8) + (((scope_memory_usage_fixed % 8) == 0) ? 0 : 1)));
            f_state_of_the_art_usage_fixed += scope_memory_usage_fixed;
            size_t min_scope_memory_usage_opt = std::numeric_limits<size_t>::max();
            for(unsigned int metadata_bitsize = 0; metadata_bitsize < MAX_METADATA_BITSIZE; metadata_bitsize++)
            {
               min_scope_memory_usage_opt = std::min(min_scope_memory_usage_opt, scope_memory_usage_opt[metadata_bitsize]);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope " + scope + " state_of_the_art opt memory usage (BITS): " + STR(min_scope_memory_usage_opt));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope " + scope + " state_of_the_art opt memory usage (BYTES): " + STR((min_scope_memory_usage_opt / 8) + (((min_scope_memory_usage_opt % 8) == 0) ? 0 : 1)));
            f_state_of_the_art_usage_opt += min_scope_memory_usage_opt;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " state_of_the_art memory usage (BITS): " + STR(f_state_of_the_art_usage));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " state_of_the_art memory usage (BYTES): " + STR((f_state_of_the_art_usage / 8) + (((f_state_of_the_art_usage % 8) == 0) ? 0 : 1)));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " state_of_the_art fixed memory usage (BITS): " + STR(f_state_of_the_art_usage_fixed));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " state_of_the_art fixed memory usage (BYTES): " + STR((f_state_of_the_art_usage_fixed / 8) + (((f_state_of_the_art_usage_fixed % 8) == 0) ? 0 : 1)));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " state_of_the_art opt memory usage (BITS): " + STR(f_state_of_the_art_usage_opt));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " state_of_the_art opt memory usage (BYTES): " + STR((f_state_of_the_art_usage_opt / 8) + (((f_state_of_the_art_usage_opt % 8) == 0) ? 0 : 1)));
         total_state_of_the_art_memory_usage += f_state_of_the_art_usage;
         total_state_of_the_art_fixed_memory_usage += f_state_of_the_art_usage_fixed;
         total_state_of_the_art_opt_memory_usage += f_state_of_the_art_usage_opt;
      }
      // our memory usage
      {
         std::vector<size_t> f_memory_usage = std::vector<size_t>(MAX_METADATA_BITSIZE, 0);
         size_t f_min_memory_usage = 0;
         std::map<std::string, std::map<size_t, size_t>> scope_to_bits_to_usage;
         if(Discr->hw_discrepancy_info->fu_id_control_flow_skip.find(f_id) == Discr->hw_discrepancy_info->fu_id_control_flow_skip.end())
         {
            size_t f_id_epp_trace_memory_word_bitsize = Discr->hw_discrepancy_info->fu_id_to_epp_trace_bitsize.at(f_id);
            if((Discr->hw_discrepancy_info->fu_id_to_max_epp_path_val.at(f_id) + 1) == (1ULL << f_id_epp_trace_memory_word_bitsize))
            {
               f_id_epp_trace_memory_word_bitsize++;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " EPP TRACE BITSIZE " + STR(f_id_epp_trace_memory_word_bitsize));
            for(const auto& scope : Discr->f_id_to_scope.at(f_id))
            {
               if(scope_to_epp_trace.find(scope) == scope_to_epp_trace.end())
                  continue;
               const auto epp_trace = scope_to_epp_trace.at(scope);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " scope " + scope + " EPP TRACE LENGTH " + STR(epp_trace.size()));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " scope " + scope + " expected baseline EPP TRACE LENGTH " + STR(f_id_epp_trace_memory_word_bitsize * epp_trace.size()));
               size_t best_metadata_bitsize = 0;
               size_t min_scope_memory_usage = std::numeric_limits<size_t>::max();
               for(size_t i = 0; i < MAX_METADATA_BITSIZE; i++)
               {
                  std::list<std::pair<size_t, size_t>> compressed_epp_trace_with_metadata;
                  if(epp_trace.size())
                  {
                     size_t prev_epp_counter = epp_trace.front();
                     // this represents the number of subsequent entries that
                     // are repeated in the trace
                     size_t metadata_counter = 0;
                     for(const auto epp_counter : epp_trace)
                     {
                        if(epp_counter != prev_epp_counter)
                        {
                           compressed_epp_trace_with_metadata.emplace_back(metadata_counter - 1, prev_epp_counter);
                           metadata_counter = 0;
                        }
                        else
                        {
                           if(metadata_counter > ((1ULL << i) - 1))
                           {
                              compressed_epp_trace_with_metadata.emplace_back(metadata_counter - 1, prev_epp_counter);
                              metadata_counter = 0;
                           }
                        }
                        metadata_counter++;
                        prev_epp_counter = epp_counter;
                     }
                     if(metadata_counter)
                        compressed_epp_trace_with_metadata.emplace_back(metadata_counter - 1, prev_epp_counter);
                  }
                  size_t scope_memory_usage = (f_id_epp_trace_memory_word_bitsize + i) * compressed_epp_trace_with_metadata.size();
                  scope_to_bits_to_usage[scope][i] = scope_memory_usage;
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope " + scope + " memory usage (METADATA) " + STR(i) + " (BITS): " + STR(scope_memory_usage));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope " + scope + " memory usage (METADATA) " + STR(i) + " (BYTES): " + STR((scope_memory_usage / 8) + (((scope_memory_usage % 8) == 0) ? 0 : 1)));
                  if(scope_memory_usage < min_scope_memory_usage)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---new best metadata bitsize = " + STR(i));
                     best_metadata_bitsize = i;
                     min_scope_memory_usage = scope_memory_usage;
                     scope_to_best_epp_trace_with_metadata[scope].swap(compressed_epp_trace_with_metadata);
                  }
               }
               scope_to_best_metadata_bits[scope] = best_metadata_bitsize;
            }
            for(const auto& s2b2u : scope_to_bits_to_usage)
            {
               size_t min_scope_usage = std::numeric_limits<size_t>::max();
               for(size_t i = 0; i < MAX_METADATA_BITSIZE; i++)
               {
                  f_memory_usage.at(i) += s2b2u.second.at(i);
                  min_scope_usage = std::min(min_scope_usage, s2b2u.second.at(i));
               }
               f_min_memory_usage += min_scope_usage;
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " NO CONTROL FLOW CHECKS NEEDED");
         }

         for(size_t i = 0; i < MAX_METADATA_BITSIZE; i++)
         {
            tot_memory_usage_per_bits.at(i) += f_memory_usage.at(i);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " memory usage (METADATA) " + STR(i) + " (BITS): " + STR(f_memory_usage.at(i)));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " memory usage (METADATA) " + STR(i) + " (BYTES): " + STR((f_memory_usage.at(i) / 8) + (((f_memory_usage.at(i) % 8) == 0) ? 0 : 1)));
         }
         min_memory_usage += f_min_memory_usage;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " memory usage (BITS): " + STR(f_min_memory_usage));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---f_id " + STR(f_id) + " memory usage (BYTES): " + STR((f_min_memory_usage / 8) + (((f_min_memory_usage % 8) == 0) ? 0 : 1)));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Untangled software control flow trace of function: " + STR(f_id));
   }
#ifndef NDEBUG
   for(const auto& i : scope_to_state_trace)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---state trace length for function scope: " + i.first + ": " + STR(i.second.size()));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->scope " + i.first);
      for(const auto id : i.second)
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---S_" + STR(id));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   }
   for(const auto& i : scope_to_epp_trace)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---EPP trace length for function scope: " + i.first + ": " + STR(i.second.size()));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->scope " + i.first);
      for(const auto id : i.second)
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---EPP_" + STR(id));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   }
#endif

   const CustomOrderedSet<unsigned int> root_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(root_functions.size() == 1, "more than one root function is not supported");
   unsigned int top_function = *(root_functions.begin());
   structural_objectRef top_module = HLSMgr->get_HLS(top_function)->top->get_circ();

   // scope_id starts from 1 because 0 are the non-initialized ones
   size_t scope_id = 1;
   for(const auto& i : scope_to_best_epp_trace_with_metadata)
   {
      const std::string& scope = i.first;
      const auto f_id = scope_to_function_id.at(i.first);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Initializing checker for scope " + i.first);
      {
         if(i.second.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Skipping scope " + i.first + " since it is not executed");
            continue;
         }
         if(Discr->hw_discrepancy_info->fu_id_control_flow_skip.find(f_id) != Discr->hw_discrepancy_info->fu_id_control_flow_skip.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Skipping scope " + i.first + " since it is belongs to a function without control flow instructions");
            continue;
         }
      }
      typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
      boost::char_separator<char> sep("/", nullptr);
      tokenizer module_path_tokens(scope, sep);
      tokenizer::iterator tok_iter;
      tokenizer::iterator tok_iter_end = module_path_tokens.end();
      for(tok_iter = module_path_tokens.begin(); tok_iter != tok_iter_end; ++tok_iter)
      {
         if(*tok_iter == top_module->get_id())
            break;
      }
      THROW_ASSERT(tok_iter != tok_iter_end, "unexpected condition");
      ++tok_iter;
      THROW_ASSERT(tok_iter != tok_iter_end, "unexpected condition");
      structural_objectRef curr_module = top_module;
      for(; tok_iter != tok_iter_end; ++tok_iter)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Looking for " + *tok_iter + " in " + curr_module->get_path());
         curr_module = curr_module->find_member(*tok_iter, component_o_K, curr_module);
         THROW_ASSERT(curr_module, "unexpected condition");
      }
      const std::string datapath_id = "Datapath_i";
      curr_module = curr_module->find_member(datapath_id, component_o_K, curr_module);
      THROW_ASSERT(curr_module, "unexpected condition");
      const std::string cfc_id = "ControlFlowChecker_i";
      curr_module = curr_module->find_member(cfc_id, component_o_K, curr_module);
      THROW_ASSERT(curr_module, "unexpected condition");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Found " + curr_module->get_path() + " of type " + GET_TYPE_NAME(curr_module));
      GetPointer<module>(curr_module)->SetParameter("EPP_MISMATCH_ID", STR(scope_id));

      const size_t trace_len = i.second.size() + 1; // use one entry more in the trace to avoid ever accessing out of bounds
      const size_t metadata_word_size = scope_to_best_metadata_bits.at(scope);
      size_t data_word_size = boost::lexical_cast<size_t>(GetPointer<module>(curr_module)->GetParameter("EPP_TRACE_BITSIZE"));
      if((Discr->hw_discrepancy_info->fu_id_to_max_epp_path_val.at(f_id) + 1) == (1ULL << data_word_size))
      {
         data_word_size++;
         GetPointer<module>(curr_module)->SetParameter("EPP_TRACE_BITSIZE", STR(data_word_size));
      }
      const size_t invalid_epp_id = (1ULL << data_word_size) - 1;
      GetPointer<module>(curr_module)->SetParameter("EPP_TRACE_LENGTH", STR(trace_len));
      GetPointer<module>(curr_module)->SetParameter("EPP_TRACE_METADATA_BITSIZE", STR(metadata_word_size));
      GetPointer<module>(curr_module)->SetParameter("EPP_TRACE_INITIAL_METADATA", STR(i.second.begin()->first));
#ifndef NDEBUG
      for(const auto& id : i.second)
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---COMPRESSED_EPP " + STR(id.first) + ":" + STR(id.second));
#endif
      const std::string init_filename = "epp_control_flow_trace_scope__" + STR(scope_id) + ".mem";
      std::ofstream init_file(GetPath((init_filename).c_str()));

      for(const auto& id : i.second)
      {
         const auto metadata = id.first;
         const auto data = id.second;
         if(metadata >> metadata_word_size)
            THROW_ERROR("metadata " + STR(metadata) + " cannot be represented with " + STR(metadata_word_size) + " bits");
         if(data >> data_word_size)
            THROW_ERROR("data " + STR(data) + " cannot be represented with " + STR(data_word_size) + " bits");
         if(metadata_word_size)
            init_file << NumberToBinaryString(metadata, metadata_word_size);
         init_file << NumberToBinaryString(data, data_word_size) << std::endl;
      }
      if(metadata_word_size)
         init_file << NumberToBinaryString(0, metadata_word_size);
      init_file << NumberToBinaryString(invalid_epp_id, data_word_size) << std::endl;
      init_file.close();
      GetPointer<module>(curr_module)->SetParameter("MEMORY_INIT_file", "\"\"" + init_filename + "\"\"");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Initialized checker for scope " + i.first);
      scope_id++;
   }
   for(size_t i = 0; i < MAX_METADATA_BITSIZE; i++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---tot memory usage (METADATA) " + STR(i) + " (BITS): " + STR(tot_memory_usage_per_bits.at(i)));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---tot memory usage (METADATA) " + STR(i) + " (BYTES): " + STR((tot_memory_usage_per_bits.at(i) / 8) + (((tot_memory_usage_per_bits.at(i) % 8) == 0) ? 0 : 1)));
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---HW Discrepancy Analysis results:");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---tot memory usage BASE (BITS): " + STR(tot_memory_usage_per_bits.at(0)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---tot memory usage BASE (BYTES): " + STR((tot_memory_usage_per_bits.at(0) / 8) + (((tot_memory_usage_per_bits.at(0) % 8) == 0) ? 0 : 1)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---tot memory usage FIXED (BITS): " + STR(tot_memory_usage_per_bits.at(FIXED_METADATA_SIZE)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---tot memory usage FIXED (BYTES): " + STR((tot_memory_usage_per_bits.at(FIXED_METADATA_SIZE) / 8) + (((tot_memory_usage_per_bits.at(FIXED_METADATA_SIZE) % 8) == 0) ? 0 : 1)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---minimal tot memory usage (BITS): " + STR(min_memory_usage));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---minimal tot memory usage (BYTES): " + STR((min_memory_usage / 8) + (((min_memory_usage % 8) == 0) ? 0 : 1)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---total state_of_the_art memory usage (BITS): " + STR(total_state_of_the_art_memory_usage));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---total state_of_the_art memory usage (BYTES): " + STR((total_state_of_the_art_memory_usage / 8) + (((total_state_of_the_art_memory_usage % 8) == 0) ? 0 : 1)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---total state_of_the_art fixed memory usage (BITS): " + STR(total_state_of_the_art_fixed_memory_usage));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---total state_of_the_art fixed memory usage (BYTES): " + STR((total_state_of_the_art_fixed_memory_usage / 8) + (((total_state_of_the_art_fixed_memory_usage % 8) == 0) ? 0 : 1)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---total state_of_the_art opt memory usage (BITS): " + STR(total_state_of_the_art_opt_memory_usage));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---total state_of_the_art opt memory usage (BYTES): " + STR((total_state_of_the_art_opt_memory_usage / 8) + (((total_state_of_the_art_opt_memory_usage % 8) == 0) ? 0 : 1)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "\n");
   return DesignFlowStep_Status::SUCCESS;
}

bool HWDiscrepancyAnalysis::HasToBeExecuted() const
{
   return true;
}
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
