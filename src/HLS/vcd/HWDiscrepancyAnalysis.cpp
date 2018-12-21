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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

// includes from design_flows/backend/ToHDL
#include "language_writer.hpp"

// include from HLS/
#include "hls.hpp"
#include "hls_manager.hpp"

// include from HLS/stg/
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

// include from HLS/vcd/
#include "CallGraphUnfolder.hpp"
#include "Discrepancy.hpp"

// include from parser/discrepancy/
#include "parse_discrepancy.hpp"

// include from tree/
#include "behavioral_helper.hpp"
#include "tree_basic_block.hpp"

#include "string_manipulation.hpp" // for GET_CLASS

HWDiscrepancyAnalysis::HWDiscrepancyAnalysis(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::HW_DISCREPANCY_ANALYSIS),
      Discr(_HLSMgr->RDiscr),
      present_state_name(static_cast<HDLWriter_Language>(_parameters->getOption<unsigned int>(OPT_writer_language)) == HDLWriter_Language::VERILOG ? "_present_state" : "present_state")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

const std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> HWDiscrepancyAnalysis::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::C_TESTBENCH_EXECUTION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
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

#ifndef NDEBUG
static void print_c_control_flow_trace(std::unordered_map<unsigned int, std::unordered_map<uint64_t, std::list<unsigned int>>>& c_control_flow_trace, int debug_level)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Printing parsed C control flow trace");
   for(const auto& fid2ctxtrace : c_control_flow_trace)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->FUNCTION_ID: " + STR(fid2ctxtrace.first));
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

DesignFlowStep_Status HWDiscrepancyAnalysis::Exec()
{
   THROW_ASSERT(Discr, "Discr data structure is not correctly initialized");
   // unfold the call graph and compute data structures used for discrepancy analysis
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Unfolding call graph");
      std::unordered_map<unsigned int, std::unordered_set<unsigned int>> caller_to_call_id;
      std::unordered_map<unsigned int, std::unordered_set<unsigned int>> call_to_called_id;
      const CallGraphManagerConstRef CGMan = HLSMgr->CGetCallGraphManager();
      CallGraphUnfolder::Unfold(HLSMgr, parameters, caller_to_call_id, call_to_called_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Unfolded call graph");
   }
   // parse the file containing the C traces
   {
      const std::string& ctrace_filename = Discr->c_trace_filename;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Parsing C trace: " + ctrace_filename);
      parse_discrepancy(ctrace_filename, Discr);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Parsed C trace: " + ctrace_filename);
   }
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      print_c_control_flow_trace(Discr->c_control_flow_trace, debug_level);
   }
#endif
   // untangle control flow traces
   std::unordered_map<std::string, std::list<unsigned int>> scope_to_epp_trace;
   for(auto& f : Discr->c_control_flow_trace)
   {
      const auto f_id = f.first;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Untangling software control flow trace of function: " + STR(f_id));
      const auto& stg = HLSMgr->get_HLS(f_id)->STG->CGetStg();
      const auto& epp_stg = HLSMgr->get_HLS(f_id)->STG->CGetEPPStg();
      const auto& stg_info = stg->CGetStateTransitionGraphInfo();
      const auto fsm_entry_node = stg_info->entry_node;
      const auto fsm_exit_node = stg_info->exit_node;
      const auto& state_id_to_check = Discr->fu_id_to_states_to_check.at(f_id);
      const auto& state_id_to_check_on_feedback = Discr->fu_id_to_feedback_states_to_check.at(f_id);
      const auto& reset_transitions = Discr->fu_id_to_reset_edges.at(f_id);
      for(auto& context2trace : f.second)
      {
         const auto context_id = context2trace.first;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing context: " + STR(context_id));
         const auto& scope = Discr->context_to_scope.at(context_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope: " + scope);

         unsigned int epp_counter = 0;
         vertex current_state = fsm_entry_node;
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
            // update the current_state with the only successor of entry
            current_state = boost::target(taken_edge, *stg);
            // check that the first state is not a loop header
            const auto next_state_id = stg_info->vertex_to_state_id.at(current_state);
            THROW_ASSERT(state_id_to_check_on_feedback.find(next_state_id) == state_id_to_check_on_feedback.cend(), "");
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
            const auto cur_state_id = stg_info->vertex_to_state_id.at(current_state);
            THROW_ASSERT(state_id_to_check.find(cur_state_id) == state_id_to_check.cend(), "");
            // select the only outgoing edge (must not be a feedback edge)
            OutEdgeIterator out_edge, out_end;
            boost::tie(out_edge, out_end) = boost::out_edges(current_state, *stg);
            const EdgeDescriptor taken_edge = *out_edge;
            THROW_ASSERT(reset_transitions.find(taken_edge) == reset_transitions.cend(), "");
            // update epp counter
            const auto taken_edge_info = stg->CGetTransitionInfo(taken_edge);
            epp_counter += taken_edge_info->get_epp_increment();
            // update the current_state with the only successor of entry
            current_state = boost::target(taken_edge, *stg);
            // check that the first state is not a loop header
            const auto next_state_id = stg_info->vertex_to_state_id.at(current_state);
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
               // if the current state is to check, write the trace
               const auto cur_state_id = stg_info->vertex_to_state_id.at(current_state);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---current_state is: " + STR(cur_state_id));
               if(state_id_to_check.find(cur_state_id) != state_id_to_check.cend())
               {
                  scope_to_epp_trace[scope].push_back(epp_counter);
               }
               /* detect the taken edge and the next state  */
               bool found_valid_next_state = false;
               bool end_of_bb_execution = false;
               bool takes_feedback_edge = false;
               vertex next_state = nullptr;
               EdgeDescriptor taken_edge;
               BOOST_FOREACH(const EdgeDescriptor e, boost::out_edges(current_state, *stg))
               {
                  const auto dst = boost::target(e, *stg);
                  const auto dst_info = stg->CGetStateInfo(dst);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---analyzing next state: " + ((dst == fsm_exit_node) ? STR("EXIT") : STR(stg_info->vertex_to_state_id.at(dst))) + " in the same BB " + STR(bb_id));
                  if((not dst_info->is_dummy) and (*dst_info->BB_ids.begin() == bb_id))
                  {
                     THROW_ASSERT(not found_valid_next_state, "two neighbors of state S_" + STR(stg_info->vertex_to_state_id.at(current_state)) + " have the same BB id = " + STR(bb_id));
                     found_valid_next_state = true;
                     taken_edge = e;
                     next_state = dst;
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---found next state: " + STR(stg_info->vertex_to_state_id.at(next_state)) + " in the same BB " + STR(bb_id));

                     if(stg->GetSelector(taken_edge) & TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK)
                     {
                        takes_feedback_edge = true;
                     }
                  }
               }
               if(takes_feedback_edge)
               {
                  if(std::next(bb_id_it) == bb_id_end)
                  {
                     THROW_ERROR("the FSM expects a feedback edge, but the software execution");
                  }
               }
               // if the next state was not found there are two cases
               if(takes_feedback_edge or not found_valid_next_state)
               {
                  if(std::next(bb_id_it) != bb_id_end)
                  {
                     // the execution branches to another BB
                     const auto next_bb_id = *std::next(bb_id_it);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---look for next BB " + STR(next_bb_id));
                     found_valid_next_state = false;
                     takes_feedback_edge = false;
                     BOOST_FOREACH(const EdgeDescriptor e, boost::out_edges(current_state, *stg))
                     {
                        const auto dst = boost::target(e, *stg);
                        const auto dst_info = stg->CGetStateInfo(dst);
                        if((not dst_info->is_dummy) and (*dst_info->BB_ids.begin() == next_bb_id))
                        {
                           THROW_ASSERT(not found_valid_next_state, "two neighbors of state S_" + STR(stg_info->vertex_to_state_id.at(current_state)) + " have the same BB id = " + STR(next_bb_id));
                           taken_edge = e;
                           next_state = dst;
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---found next state: " + STR(stg_info->vertex_to_state_id.at(next_state)) + " in new BB " + STR(next_bb_id));
                           if(stg->GetSelector(taken_edge) & TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK)
                           {
                              takes_feedback_edge = true;
                           }
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
#if HAVE_ASSERTS
               THROW_ASSERT(takes_feedback_edge == (reset_transitions.find(taken_edge) != reset_transitions.cend()), "");
               if(next_state != fsm_exit_node)
               {
                  const auto next_state_id = stg_info->vertex_to_state_id.at(next_state);
                  THROW_ASSERT((not takes_feedback_edge) or (state_id_to_check_on_feedback.find(next_state_id) != state_id_to_check_on_feedback.cend()), "");
               }
#endif
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
                  epp_counter = from_entry_edge_info->get_epp_increment();
               }
               current_state = next_state;
               goes_to_next_bb_execution = end_of_bb_execution;
            } while(not goes_to_next_bb_execution);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Untangled software control flow trace of function: " + STR(f_id));
   }
#ifndef NDEBUG
   for(const auto& i : scope_to_epp_trace)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->scope " + i.first);
      for(const auto id : i.second)
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---" + STR(id));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}

bool HWDiscrepancyAnalysis::HasToBeExecuted() const
{
   return true;
}
