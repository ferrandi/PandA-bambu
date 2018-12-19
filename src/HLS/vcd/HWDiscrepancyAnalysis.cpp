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

static inline std::pair<bool, vertex> find_next_state(const unsigned int bb_id, const StateTransitionGraphConstRef& stg, const vertex current_state)
{
   const auto& stg_info = stg->CGetStateTransitionGraphInfo();
   vertex next_state = nullptr;
   bool found = false;
   OutEdgeIterator out_ei, end_ei;
   boost::tie(out_ei, end_ei) = boost::out_edges(current_state, *stg);
   for(; out_ei != end_ei; out_ei++)
   {
      vertex neighbor_state = boost::target(*out_ei, *stg);
      const auto neighbor_info = stg->CGetStateInfo(neighbor_state);
      if(bb_id == *neighbor_info->BB_ids.begin())
      {
         // this state is a good candidate
         if(not neighbor_info->is_dummy)
         {
            THROW_ASSERT(not found, "two neighbors of state S_" + STR(stg_info->vertex_to_state_id.at(current_state)) + ": S_" + STR(stg_info->vertex_to_state_id.at(next_state)) + " and S_" + STR(stg_info->vertex_to_state_id.at(neighbor_state)) +
                                        " have the same BB id = " + STR(bb_id));
            found = true;
            next_state = neighbor_state;
         }
      }
   }
   return std::make_pair(found, next_state);
}

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
   std::unordered_map<std::string, std::list<unsigned int>> scope_to_fsm_trace;
   for(auto& f : Discr->c_control_flow_trace)
   {
      const auto f_id = f.first;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Untangling software control flow trace of function: " + STR(f_id));
      const auto stg = HLSMgr->get_HLS(f_id)->STG->CGetStg();
      const auto& stg_info = stg->CGetStateTransitionGraphInfo();
      const vertex exit_state = stg_info->exit_node;
      std::map<unsigned int, std::list<unsigned int>> bb_id_to_state_list;
      std::set<unsigned int> visited_states;
      for(auto& context2trace : f.second)
      {
         const auto context_id = context2trace.first;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing context: " + STR(context_id));
         const auto& scope = Discr->context_to_scope.at(context_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---scope: " + scope);
         const std::list<unsigned int>& bb_trace = context2trace.second;

         vertex current_state = stg_info->entry_node;
         THROW_ASSERT(boost::out_degree(current_state, *stg) == 1, "entry vertex have out degree = " + STR(boost::out_degree(current_state, *stg)));
         if(HLSMgr->get_HLS(f_id)->registered_inputs)
         {
            OutEdgeIterator out_edge, out_end;
            boost::tie(out_edge, out_end) = boost::out_edges(current_state, *stg);
            current_state = boost::target(*out_edge, *stg);
            scope_to_fsm_trace[scope].push_back(stg_info->vertex_to_state_id.at(current_state));
         }
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
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---find next state with bb_id: " + STR(bb_id));
            std::pair<bool, vertex> found_next_state = find_next_state(bb_id, stg, current_state);
            THROW_ASSERT(std::get<0>(found_next_state), "");
            const auto first_state_for_bb = std::get<1>(found_next_state);
            const auto first_state_info = stg->CGetStateInfo(first_state_for_bb);
            THROW_ASSERT(not first_state_info->is_dummy, "");
            const unsigned int first_state_id = stg_info->vertex_to_state_id.at(first_state_for_bb);
            if(visited_states.find(first_state_id) != visited_states.end() or (first_state_info->is_duplicated and not first_state_info->isOriginalState))
            {
               /*
                * this path was already visited, so we have the list of fsm
                * corresponding to the bb_id, computed in the previous
                * traversal.
                * this must be unique (i.e. it cannot happen that there are
                * multiple disjoint chains of states that represent the same bb
                * in hardware), so we can directly check the previous state list
                * to see if it matches
                */
               const auto& bb_state_list = bb_id_to_state_list[bb_id];
               auto state_it = bb_state_list.cbegin();
               const auto state_end = bb_state_list.cend();
               do
               {
                  const auto next_state = std::get<1>(found_next_state);
                  const auto next_state_info = stg->CGetStateInfo(next_state);
                  auto next_state_id = stg_info->vertex_to_state_id.at(next_state);
                  if(next_state_info->is_dummy)
                  {
                     continue;
                  }
                  if(next_state_info->is_duplicated and not next_state_info->isOriginalState)
                  {
                     visited_states.insert(next_state_id);
                     next_state_id = stg_info->vertex_to_state_id.at(next_state_info->clonedState);
                  }
                  THROW_ASSERT(*state_it == next_state_id, "");
                  scope_to_fsm_trace[scope].push_back(next_state_id);
                  current_state = next_state;
                  found_next_state = find_next_state(bb_id, stg, current_state);
                  state_it++;
               } while(std::get<0>(found_next_state) and state_it != state_end); // there is another state with the same bb_id
            }
            else
            {
               // this is the first time we traverse this path in the fsm
               auto& bb_state_list = bb_id_to_state_list[bb_id];
               do
               {
                  const auto next_state = std::get<1>(found_next_state);
                  const auto next_state_info = stg->CGetStateInfo(next_state);
                  const auto next_state_id = stg_info->vertex_to_state_id.at(next_state);
                  if(visited_states.find(next_state_id) != visited_states.end())
                  {
                     // if we already visited it we're done
                     break;
                  }
                  if(not next_state_info->is_dummy)
                  {
                     scope_to_fsm_trace[scope].push_back(next_state_id);
                     bb_state_list.push_back(next_state_id);
                  }
                  visited_states.insert(next_state_id);
                  current_state = next_state;
                  found_next_state = find_next_state(bb_id, stg, current_state);
               } while(std::get<0>(found_next_state)); // there is another state with the same bb_id
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         }
         // check that the current state is a valid end state for the FSM
         {
            bool valid_end_state = false;
            OutEdgeIterator out_ei, end_ei;
            boost::tie(out_ei, end_ei) = boost::out_edges(current_state, *stg);
            for(; out_ei != end_ei; out_ei++)
            {
               if(boost::target(*out_ei, *stg) == exit_state)
               {
                  valid_end_state = true;
                  break;
               }
            }
            if(not valid_end_state)
            {
               THROW_ERROR("FSM of function " + STR(f_id) + " in scope " + scope + " terminates execution in unexpected state: " + STR(stg_info->vertex_to_state_id.at(current_state)));
            }
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed context: " + STR(context_id));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Untangled software control flow trace of function: " + STR(f_id));
   }
#ifndef NDEBUG
   for(const auto& i : scope_to_fsm_trace)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->scope " + i.first);
      for(const auto id : i.second)
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---state: " + STR(id));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}

bool HWDiscrepancyAnalysis::HasToBeExecuted() const
{
   return true;
}
