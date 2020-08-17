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
 * @file BB_based_stg.hpp
 * @brief
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Stefano Bodini
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "BB_based_stg.hpp"

///. include
#include "Parameter.hpp"

/// algorithms/loop_detection includes
#include "loop.hpp"
#include "loops.hpp"

/// behavior include
#include "basic_block.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// boost includes
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/incremental_components.hpp>

/// HLS includes
#include "hls.hpp"
#include "hls_constraints.hpp"

/// HLS/binding/module include
#include "fu_binding.hpp"

/// HLS/function_allocation include
#include "functions.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// HLS/scheduling include
#include "schedule.hpp"

/// HLS/stg includes
#include "StateTransitionGraph_constructor.hpp"
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

/// HLS/vcd includes
#include "Discrepancy.hpp"

/// STD include
#include <cmath>

/// technology includes
#include "technology_manager.hpp"
#include "technology_node.hpp"

/// technology/physical_library/models include
#include "time_model.hpp"

/// tree include
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "omp_functions.hpp"
#include "op_graph.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include <boost/foreach.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/incremental_components.hpp>
#include <cmath>

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// utility include
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

BB_based_stg::BB_based_stg(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : STG_creator(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::BB_STG_CREATOR)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

BB_based_stg::~BB_based_stg() = default;

void BB_based_stg::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->STG = StateTransitionGraphManagerRef(new StateTransitionGraphManager(HLSMgr, HLS, parameters));
}

static void add_in_sched_order(std::list<vertex>& statement_list, vertex stmt, const ScheduleConstRef sch, const OpGraphConstRef ASSERT_PARAMETER(dfg))
{
   THROW_ASSERT(sch->is_scheduled(stmt), "First vertex is not scheduled");
   std::list<vertex>::const_iterator it_end = statement_list.end();
   std::list<vertex>::iterator it;
   THROW_ASSERT(std::find(statement_list.begin(), statement_list.end(), stmt) == statement_list.end(), "Statement already ordered: " + GET_NAME(dfg, stmt));

   for(it = statement_list.begin(); it != it_end; ++it)
   {
      THROW_ASSERT(sch->is_scheduled(*it), "Second vertex is not scheduled");
      if(sch->get_cstep(*it) > sch->get_cstep(stmt))
         break;
   }
   statement_list.insert(it, stmt);
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> BB_based_stg::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
#if HAVE_ILP_BUILT
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::LIST_BASED_SCHEDULING, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
#endif
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status BB_based_stg::InternalExec()
{
   long int step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      START_TIME(step_time);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting creation of STG...");

   StateTransitionGraph_constructorRef STG_builder = HLS->STG->STG_builder;
   THROW_ASSERT(STG_builder, "STG constructor not properly initialized");

   /// first state of a basic-block
   CustomUnorderedMap<vertex, vertex> first_state;
   /// last state of a basic-block
   CustomUnorderedMap<vertex, vertex> last_state;

   const OpGraphConstRef dfgRef = HLSMgr->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::DFG);

   const ScheduleConstRef sch = HLS->Rsch;

   const BBGraphConstRef fbb = HLSMgr->CGetFunctionBehavior(funId)->CGetBBGraph(FunctionBehavior::FBB);

   /// get entry and exit basic block
   const vertex bb_entry = fbb->CGetBBGraphInfo()->entry_vertex;
   const vertex bb_exit = fbb->CGetBBGraphInfo()->exit_vertex;

   bool first_state_p;
   bool have_previous;
   vertex previous;
   std::map<vertex, std::list<vertex>> call_states;
   std::map<vertex, std::list<vertex>> call_operations;
   CustomOrderedSet<vertex> already_analyzed;
   VertexIterator vit, vend;

   /// contains the list of operations which are executing, starting, ending and "on-fly" in every state of the STG
   std::map<vertex, std::list<vertex>> global_executing_ops, global_starting_ops, global_ending_ops, global_onfly_ops;

   const CallGraphManagerConstRef CGM = HLSMgr->CGetCallGraphManager();
   const auto top_functions = CGM->GetRootFunctions();
   bool needMemoryMappedRegisters = (top_functions.find(funId) != top_functions.end() && parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION) ||
                                    (HLSMgr->hasToBeInterfaced(funId) and top_functions.find(funId) == top_functions.end()) || parameters->getOption<bool>(OPT_memory_mapped_top);
   bool has_registered_inputs = HLS->registered_inputs && !needMemoryMappedRegisters;
   std::string function_name = functions::get_function_name_cleaned(funId, HLSMgr);
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   if(top_function_ids.find(funId) != top_function_ids.end() and parameters->getOption<std::string>(OPT_registered_inputs) == "top")
   {
      has_registered_inputs = true;
   }
   if(HLSMgr->Rfuns->is_a_proxied_function(function_name) && !needMemoryMappedRegisters)
   {
      if(parameters->getOption<std::string>(OPT_registered_inputs) != "no")
         has_registered_inputs = true;
   }
   else if(parameters->getOption<std::string>(OPT_registered_inputs) == "yes" && !needMemoryMappedRegisters)
      has_registered_inputs = true;
   if(parameters->getOption<std::string>(OPT_registered_inputs) != "no" && !has_registered_inputs && !needMemoryMappedRegisters)
   {
      /// the analysis has to be performed only on the reachable functions
      /// functions to be analyzed
      CustomOrderedSet<unsigned int> sort_list = CGM->GetReachedBodyFunctions();
      CustomUnorderedSet<vertex> vertex_subset;
      for(auto cvertex : sort_list)
         vertex_subset.insert(CGM->GetVertex(cvertex));
      const CallGraphConstRef subgraph = CGM->CGetCallSubGraph(vertex_subset);
      InEdgeIterator ie_it, ie_it_end;
      vertex current_vertex = CGM->GetVertex(funId);
      size_t n_call_sites = 0;
      for(boost::tie(ie_it, ie_it_end) = boost::in_edges(current_vertex, *subgraph); ie_it != ie_it_end; ++ie_it)
      {
         const auto* info = Cget_edge_info<FunctionEdgeInfo, const CallGraph>(*ie_it, *subgraph);
         n_call_sites += static_cast<size_t>(info->direct_call_points.size()) + static_cast<size_t>(info->indirect_call_points.size());
      }
      HLS->call_sites_number = n_call_sites;
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Number of function call sites = " + STR(n_call_sites));
      unsigned int n_levels = 0;
      const auto controller_delay = HLS->allocation_information->EstimateControllerDelay();
      for(; n_call_sites > (1u << n_levels); ++n_levels)
         ;
      double mux_time_estimation = (n_levels * HLS->allocation_information->mux_time_unit(32)) + (n_levels > 0 ? controller_delay : 0);
      if(mux_time_estimation > HLS->allocation_information->getMinimumSlack())
      {
         has_registered_inputs = true;
      }
   }

   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   if(HLS->Param->isOption(OPT_context_switch))
   {
      if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
         has_registered_inputs = true;
   }
   HLS->registered_inputs = has_registered_inputs;

   for(boost::tie(vit, vend) = boost::vertices(*fbb); vit != vend; ++vit)
   {
      if(*vit == bb_entry)
      {
         last_state[*vit] = first_state[*vit] = HLS->STG->get_entry_state();
         continue;
      }
      else if(*vit == bb_exit)
      {
         last_state[*vit] = first_state[*vit] = HLS->STG->get_exit_state();
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Building STG of BB" + STR(fbb->CGetBBNodeInfo(*vit)->block->number));
      const BBNodeInfoConstRef operations = fbb->CGetBBNodeInfo(*vit);
      std::list<vertex> ordered_operations;
      auto ops_it_end = operations->statements_list.end();
      for(auto ops_it = operations->statements_list.begin(); ops_it_end != ops_it; ++ops_it)
      {
         add_in_sched_order(ordered_operations, *ops_it, sch, dfgRef);
      }
      if(boost::in_degree(*vit, *fbb) == 1)
      {
         /// for basic block connected only to entry bb
         InEdgeIterator ie, iend;
         boost::tie(ie, iend) = boost::in_edges(*vit, *fbb);
         vertex bb_src = boost::source(*ie, *fbb);
         if(bb_src == bb_entry)
         {
            std::list<vertex>::const_iterator stmt_it_end = ordered_operations.end();
            for(std::list<vertex>::const_iterator stmt_it = ordered_operations.begin(); stmt_it_end != stmt_it; ++stmt_it)
            {
               if(has_registered_inputs || (GET_TYPE(dfgRef, *stmt_it) & TYPE_PHI))
               {
                  /// add an empty state before the current basic block
                  std::list<vertex> exec_ops, start_ops, end_ops;
                  const BBNodeInfoConstRef entry_operations = fbb->CGetBBNodeInfo(bb_src);
                  auto entry_ops_it_end = entry_operations->statements_list.end();
                  for(auto entry_ops_it = entry_operations->statements_list.begin(); entry_ops_it_end != entry_ops_it; ++entry_ops_it)
                  {
                     exec_ops.push_back(*entry_ops_it);
                     start_ops.push_back(*entry_ops_it);
                     end_ops.push_back(*entry_ops_it);
                  }
                  CustomOrderedSet<unsigned int> BB_ids;
                  BB_ids.insert(entry_operations->get_bb_index());
                  vertex s_cur = STG_builder->create_state(exec_ops, start_ops, end_ops, BB_ids);
                  STG_builder->connect_state(last_state[bb_src], s_cur, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL);
                  last_state[bb_src] = s_cur;
                  break;
               }
            }
         }
      }

      first_state_p = true;
      have_previous = false;
      std::map<ControlStep, std::list<vertex>> executing_ops, starting_ops, ending_ops, onfly_ops;
      ControlStep max_cstep = ControlStep(0u);
      ControlStep min_cstep = ControlStep(std::numeric_limits<unsigned int>::max());

      std::list<vertex>::const_iterator stmt_it_end = ordered_operations.end();
      for(std::list<vertex>::const_iterator stmt_it = ordered_operations.begin(); stmt_it_end != stmt_it; ++stmt_it)
      {
         vertex op = *stmt_it;
         if(GET_TYPE(dfgRef, op) & (TYPE_GOTO))
            continue;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing operation " + GET_NAME(dfgRef, op));
         if(GET_TYPE(dfgRef, op) & (TYPE_VPHI))
         {
            /// check if virtual phi can be removed and so its basic block
            bool can_be_removed = true;
            OutEdgeIterator oe, oend;
            for(boost::tie(oe, oend) = boost::out_edges(*vit, *fbb); oe != oend && can_be_removed; ++oe)
            {
               vertex tgt = boost::target(*oe, *fbb);
               if(tgt == *vit)
                  continue;
               const BBNodeInfoConstRef out_bb_operations = fbb->CGetBBNodeInfo(tgt);
               auto obo_it_end = out_bb_operations->statements_list.end();
               for(auto obo_it = out_bb_operations->statements_list.begin(); obo_it_end != obo_it && can_be_removed; ++obo_it)
               {
                  if((GET_TYPE(dfgRef, *obo_it) & TYPE_PHI) != 0)
                  {
                     for(const auto& def_edge : GetPointer<const gimple_phi>(HLSMgr->get_tree_manager()->get_tree_node_const(dfgRef->CGetOpNodeInfo(*obo_it)->GetNodeId()))->CGetDefEdgesList())
                     {
                        if(not def_edge.first)
                           continue;
                        if(def_edge.second == operations->get_bb_index())
                           can_be_removed = false;
                     }
                  }
               }
            }
            if(can_be_removed)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed operation " + GET_NAME(dfgRef, op));
               continue;
            }
         }
         if(already_analyzed.find(op) != already_analyzed.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Already Analyzed operation " + GET_NAME(dfgRef, op));
            continue;
         }
         const auto cstep = sch->get_cstep(op).second;
         unsigned int fu_name = HLS->Rfu->get_assign(op);
         THROW_ASSERT(sch->get_cstep_end(op) >= sch->get_cstep(op), "unexpected condition");
         const auto delay = sch->get_cstep_end(op).second - sch->get_cstep(op).second;
         const auto end_step = cstep + delay;
         max_cstep = std::max(max_cstep, end_step);
         min_cstep = std::min(min_cstep, cstep);
         executing_ops[cstep].push_back(op);
         starting_ops[cstep].push_back(op);
         const auto initiation_time = HLS->allocation_information->get_initiation_time(fu_name, op);
         if(initiation_time == 0 || initiation_time > delay)
         {
            for(auto c = cstep + 1u; c <= end_step; c++)
            {
               executing_ops[c].push_back(op);
            }
         }
         for(auto c = cstep + 1u; c <= end_step; c++)
         {
            onfly_ops[c].push_back(op);
         }
         ending_ops[end_step].push_back(op);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed operation " + GET_NAME(dfgRef, op));
      }
      vertex s_cur;
      for(auto l = min_cstep; l <= max_cstep; l++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering control step " + STR(l));
         std::list<vertex> exec_ops, start_ops, end_ops, onf_ops;
         if(executing_ops.find(l) != executing_ops.end())
            exec_ops = executing_ops[l];
         if(starting_ops.find(l) != starting_ops.end())
            start_ops = starting_ops[l];
         if(ending_ops.find(l) != ending_ops.end())
            end_ops = ending_ops[l];
         if(onfly_ops.find(l) != onfly_ops.end())
            onf_ops = onfly_ops[l];
         CustomOrderedSet<unsigned int> BB_ids;
         BB_ids.insert(operations->get_bb_index());
         s_cur = STG_builder->create_state(exec_ops, start_ops, end_ops, BB_ids);

         global_executing_ops[s_cur] = exec_ops;
         global_starting_ops[s_cur] = start_ops;
         global_ending_ops[s_cur] = end_ops;
         global_onfly_ops.insert({s_cur, onf_ops});

         for(auto& exec_op : exec_ops)
         {
            technology_nodeRef tn = HLS->allocation_information->get_fu(HLS->Rfu->get_assign(exec_op));
            technology_nodeRef op_tn = GetPointer<functional_unit>(tn)->get_operation(tree_helper::normalized_ID(dfgRef->CGetOpNodeInfo(exec_op)->GetOperation()));
            THROW_ASSERT(GetPointer<operation>(op_tn)->time_m, "Time model not available for operation: " + GET_NAME(dfgRef, exec_op));
            if(!GetPointer<operation>(op_tn)->is_bounded())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(dfgRef, exec_op) + " is unbounded");
               call_operations[s_cur].push_back(exec_op);
            }
         }
         // THROW_ASSERT(call_operations.find(s_cur) == call_operations.end() || call_operations.find(s_cur)->second.size() <= 1, "currently only one unbounded operation per state is admitted");
         if(call_operations.find(s_cur) != call_operations.end() && call_operations.find(s_cur)->second.size())
         {
            THROW_ASSERT(call_operations.find(s_cur) != call_operations.end() && call_operations.find(s_cur)->second.begin() != call_operations.find(s_cur)->second.end(), "unexpected condition");
            std::list<vertex> call_ops(call_operations.find(s_cur)->second.begin(), call_operations.find(s_cur)->second.end()), empty_ops;
            CustomOrderedSet<unsigned int> call_BB_ids;
            call_BB_ids.insert(operations->get_bb_index());
            vertex s_call = STG_builder->create_state(call_ops, empty_ops, call_ops, call_BB_ids);
            HLS->STG->GetStg()->GetStateInfo(s_call)->is_dummy = true;
            call_states[s_cur].push_back(s_call);
            CustomOrderedSet<vertex> ops;
            ops.insert(call_operations.find(s_cur)->second.begin(), call_operations.find(s_cur)->second.end());
            if(ops.size() > 1)
               HLS->STG->add_multi_unbounded_obj(s_cur, ops);
         }

         if(have_previous)
         {
            if(call_states.find(previous) == call_states.end())
            {
               STG_builder->connect_state(previous, s_cur, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL);
            }
            else
            {
               THROW_ASSERT(call_operations.find(previous) != call_operations.end() && call_operations.find(previous)->second.begin() != call_operations.find(previous)->second.end(), "unexpected condition");
               THROW_ASSERT(call_states.find(previous) != call_states.end(), "unexpected condition");
               CustomOrderedSet<vertex> ops;
               ops.insert(call_operations.find(previous)->second.begin(), call_operations.find(previous)->second.end());
               auto call_sets = call_states.find(previous)->second;
               for(auto& call_set : call_sets)
               {
                  EdgeDescriptor s_e = STG_builder->connect_state(call_set, s_cur, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL);
                  STG_builder->set_unbounded_condition(s_e, ALL_FINISHED, ops, previous);
               }
               EdgeDescriptor s_e = STG_builder->connect_state(previous, s_cur, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL);
               STG_builder->set_unbounded_condition(s_e, ALL_FINISHED, ops, previous);
            }
         }
         else
            have_previous = true;
         previous = s_cur;
         if(first_state_p)
         {
            first_state[*vit] = s_cur;
            first_state_p = false;
         }
         if(call_states.find(s_cur) != call_states.end())
         {
            THROW_ASSERT(call_operations.find(s_cur) != call_operations.end() && call_operations.find(s_cur)->second.begin() != call_operations.find(s_cur)->second.end(), "unexpected condition");
            THROW_ASSERT(call_states.find(s_cur) != call_states.end() && call_states.find(s_cur)->second.begin() != call_states.find(s_cur)->second.end(), "unexpected condition");
            vertex waiting_state = call_states.find(s_cur)->second.front();
            EdgeDescriptor s_e = STG_builder->connect_state(s_cur, waiting_state, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL);

            CustomOrderedSet<vertex> ops;
            ops.insert(call_operations.find(s_cur)->second.begin(), call_operations.find(s_cur)->second.end());
            STG_builder->set_unbounded_condition(s_e, NOT_ALL_FINISHED, ops, s_cur);

            s_e = STG_builder->connect_state(waiting_state, waiting_state, TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK);
            STG_builder->set_unbounded_condition(s_e, NOT_ALL_FINISHED, ops, s_cur);
         }
         last_state[*vit] = s_cur;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered control step " + STR(l));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Built STG of BB" + STR(fbb->CGetBBNodeInfo(*vit)->block->number));
   }

   /// connect two states belonging to different basic blocks
   /// concurrently manage entry and exit state and completely merged basic blocks
   EdgeIterator e, e_end;
   for(boost::tie(e, e_end) = boost::edges(*fbb); e != e_end; e++)
   {
      vertex bb_src = boost::source(*e, *fbb);
      if(last_state.find(bb_src) == last_state.end())
         continue;
      vertex bb_tgt = boost::target(*e, *fbb);
      /// removed the edge from entry to exit
      if(bb_src == bb_entry && bb_tgt == bb_exit)
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(fbb->CGetBBNodeInfo(bb_src)->block->number) + "-->" + STR(fbb->CGetBBNodeInfo(bb_tgt)->block->number));
      vertex s_src, s_tgt;
      if(bb_src == bb_exit)
         s_src = HLS->STG->get_exit_state();
      else
      {
         THROW_ASSERT(last_state.find(bb_src) != last_state.end(), "missing a state vertex");
         s_src = last_state.find(bb_src)->second;
      }
      if(bb_tgt == bb_entry)
         s_tgt = HLS->STG->get_entry_state();
      else if(bb_tgt == bb_exit)
         s_tgt = HLS->STG->get_exit_state();
      else
      {
         while(first_state.find(bb_tgt) == first_state.end())
         {
            THROW_ASSERT(boost::out_degree(bb_tgt, *fbb) == 1, "unexpected pattern");
            OutEdgeIterator oe, oend;
            boost::tie(oe, oend) = boost::out_edges(bb_tgt, *fbb);
            bb_tgt = boost::target(*oe, *fbb);
         }
         s_tgt = first_state.find(bb_tgt)->second;
      }
      // THROW_ASSERT(s_src != s_tgt, "chaining between basic block is not expected");

      EdgeDescriptor s_e;
      if(FB_CFG_SELECTOR & fbb->GetSelector(*e))
      {
         s_e = STG_builder->connect_state(s_src, s_tgt, TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK);
         if(call_states.find(s_src) != call_states.end())
         {
            auto call_sets = call_states.find(s_src)->second;
            CustomOrderedSet<vertex> ops;
            ops.insert(call_operations[s_src].begin(), call_operations[s_src].end());
            if(call_sets.begin() != call_sets.end())
            {
               STG_builder->set_unbounded_condition(s_e, ALL_FINISHED, ops, s_src);
            }
            for(auto& call_set : call_sets)
            {
               EdgeDescriptor s_e1 = STG_builder->connect_state(call_set, s_tgt, TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK);
               STG_builder->set_unbounded_condition(s_e1, ALL_FINISHED, ops, s_src);
            }
         }
      }
      else
      {
         if(call_states.find(s_src) == call_states.end())
            s_e = STG_builder->connect_state(s_src, s_tgt, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL);
         else
         {
            THROW_ASSERT(call_operations.find(s_src) != call_operations.end() && call_operations.find(s_src)->second.size() != 0, "State " + HLS->STG->get_state_name(s_src) + " does not contain any call expression");
            auto call_sets = call_states.find(s_src)->second;
            CustomOrderedSet<vertex> ops;
            ops.insert(call_operations.find(s_src)->second.begin(), call_operations.find(s_src)->second.end());
            for(auto& call_set : call_sets)
            {
               EdgeDescriptor s_edge = STG_builder->connect_state(call_set, s_tgt, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL);
               STG_builder->set_unbounded_condition(s_edge, ALL_FINISHED, ops, s_src);
            }

            s_e = STG_builder->connect_state(s_src, s_tgt, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL);
            STG_builder->set_unbounded_condition(s_e, ALL_FINISHED, ops, s_src);
         }
      }
      CustomOrderedSet<std::pair<vertex, unsigned int>> out_conditions;
      /// compute the controlling vertex
      const BBNodeInfoConstRef bb_node_info = fbb->CGetBBNodeInfo(bb_src);
      THROW_ASSERT(bb_node_info->statements_list.size(), "at least one operation should belong to this basic block");
      vertex last_operation = *(bb_node_info->statements_list.rbegin());
      const CustomOrderedSet<unsigned int>& cfg_edge_ids = fbb->CGetBBEdgeInfo(*e)->get_labels(CFG_SELECTOR);

      if(cfg_edge_ids.size())
      {
         auto edgeType = *cfg_edge_ids.begin();
         transition_type t = TRUE_COND;
         if(edgeType == T_COND || edgeType == F_COND)
         {
            if(edgeType == T_COND)
               t = TRUE_COND;
            else if(edgeType == F_COND)
               t = FALSE_COND;
            STG_builder->set_condition(s_e, t, last_operation);
         }
         else
         {
            CustomOrderedSet<unsigned> labels;
            bool has_default = false;
            for(auto label : cfg_edge_ids)
            {
               if(label == default_COND)
                  has_default = true;
               else
                  labels.insert(label);
            }
            STG_builder->set_switch_condition(s_e, last_operation, labels, has_default);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(fbb->CGetBBNodeInfo(bb_src)->block->number) + "-->" + STR(fbb->CGetBBNodeInfo(bb_tgt)->block->number));
   }
   ///*****************************************************
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      HLS->STG->CGetStg()->WriteDot("HLS_STGraph-pre-opt.dot");
   }
   /// Call optimize_cycles for every cycle in the stg
   if(not parameters->IsParameter("no-fsm-duplication") or not parameters->GetParameter<bool>("no-fsm-duplication"))
   {
      unsigned int instance = 0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing cycles");
      BOOST_FOREACH(EdgeDescriptor fbbei, boost::edges(*fbb))
      {
         if(FB_CFG_SELECTOR & fbb->GetSelector(fbbei))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing cycle starting from " + STR(fbb->CGetBBNodeInfo(boost::source(fbbei, *fbb))->block->number) + "-->" + STR(fbb->CGetBBNodeInfo(boost::target(fbbei, *fbb))->block->number));
            vertex bbEndingCycle = boost::source(fbbei, *fbb);
            // std::cerr << "begin cycles optimization" << std::endl;
            optimize_cycles(bbEndingCycle, first_state, last_state, global_starting_ops, global_ending_ops, global_executing_ops, global_onfly_ops);
            // std::cerr << "end cycles optimization " << STR(instance) << std::endl;
            if(parameters->getOption<bool>(OPT_print_dot))
            {
               HLS->STG->CGetStg()->WriteDot("HLS_STGraph-post" + STR(instance) + ".dot");
            }
            ++instance;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed cycle starting from " + STR(fbb->CGetBBNodeInfo(boost::source(fbbei, *fbb))->block->number) + "-->" + STR(fbb->CGetBBNodeInfo(boost::target(fbbei, *fbb))->block->number));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed cycles");
   }

   ///*****************************************************

   HLS->STG->compute_min_max();
   bool is_pipelined = HLSMgr->CGetFunctionBehavior(funId)->build_simple_pipeline();
   if(HLS->STG->CGetStg()->CGetStateTransitionGraphInfo()->min_cycles != 1 && !is_pipelined)
   {
      HLS->registered_done_port = true;
      /// check for unbounded op executed in the last step
      /// this ops creates problems with done port registering
      vertex exit_state = HLS->STG->get_exit_state();
      InEdgeIterator ie_it, ie_it_end;
      for(boost::tie(ie_it, ie_it_end) = boost::in_edges(exit_state, *HLS->STG->CGetStg()); ie_it != ie_it_end; ++ie_it)
      {
         vertex src_state = boost::source(*ie_it, *HLS->STG->CGetStg());
         if(HLS->STG->CGetStg()->CGetStateInfo(src_state)->is_dummy)
         {
            HLS->registered_done_port = false;
            break;
         }
      }
   }
   else
      HLS->registered_done_port = false;
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->State Transition Graph Information of function " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Number of operations: " + STR(boost::num_vertices(*(HLSMgr->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::CFG)))));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Number of basic blocks: " + STR(boost::num_vertices(*(HLSMgr->CGetFunctionBehavior(funId)->CGetBBGraph(FunctionBehavior::BB)))));
   HLS->STG->print_statistics();
   if(has_registered_inputs)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Parameters are registered");
   if(HLS->registered_done_port || is_pipelined)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Done port is registered");

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      HLS->STG->CGetStg()->WriteDot("HLS_STGraph.dot");
      HLS->STG->CGetStg()->WriteDot("fsm.dot", 1);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "STG created!");
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      STOP_TIME(step_time);
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Time to perform creation of STG: " + print_cpu_time(step_time) + " seconds");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking BB graph - ST graph consistency");
      CustomMap<unsigned int, ControlStep> bb_length, stg_length;
      const auto st_graph = HLS->STG->CGetStg();
      VertexIterator state, state_end;
      for(boost::tie(state, state_end) = boost::vertices(*st_graph); state != state_end; state++)
      {
         const auto state_info = st_graph->CGetStateInfo(*state);
         for(const auto BB_id : state_info->BB_ids)
         {
            if(stg_length.find(BB_id) != stg_length.end())
            {
               stg_length.at(BB_id)++;
            }
            else
            {
               stg_length.insert(CustomMap<unsigned int, ControlStep>::value_type(BB_id, ControlStep(1)));
            }
         }
      }
      const auto bb_cfg = HLSMgr->CGetFunctionBehavior(funId)->CGetBBGraph();
      VertexIterator bb, bb_end;
      for(boost::tie(bb, bb_end) = boost::vertices(*bb_cfg); bb != bb_end; bb++)
      {
         const auto bb_node_info = bb_cfg->CGetBBNodeInfo(*bb);
         ControlStep bb_begin = ControlStep(std::numeric_limits<unsigned int>::max());
         ControlStep bb_ending = ControlStep(0u);
         for(const auto op : bb_node_info->statements_list)
         {
            if(HLS->Rsch->get_cstep(op).second < bb_begin)
               bb_begin = HLS->Rsch->get_cstep(op).second;
            if(HLS->Rsch->get_cstep_end(op).second > bb_ending)
               bb_ending = HLS->Rsch->get_cstep_end(op).second;
         }
         bb_length.insert(CustomMap<unsigned int, ControlStep>::value_type(bb_node_info->block->number, bb_ending - bb_begin + 1));
      }
#ifndef NDEBUG
      for(const auto& basic_block : bb_length)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---BB" + STR(basic_block.first) + ": " + STR(from_strongtype_cast<unsigned int>(basic_block.second)) + " vs. " + STR(from_strongtype_cast<unsigned int>(stg_length.find(basic_block.first)->second)));
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked BB graph - ST graph consistency");
   }
   if(parameters->isOption(OPT_discrepancy_hw) and parameters->getOption<bool>(OPT_discrepancy_hw))
   {
      compute_EPP_edge_increments(global_starting_ops);
   }
   return DesignFlowStep_Status::SUCCESS;
}

static void add_EPP_edges(const StateTransitionGraphManagerRef& STG)
{
   const auto stg = STG->GetStg();
   const auto entry_state = STG->get_entry_state();
   THROW_ASSERT(boost::out_degree(entry_state, *stg) == 1, "entry state has not 1 successor");
   const auto exit_state = STG->get_exit_state();
   THROW_ASSERT(boost::in_degree(exit_state, *stg), "exit state has no predecessors");
   /*
    * Select pairs of vertices for which we have to add artificial edges for
    * Efficient Path Profiling, to handle feedback edges.
    * Dummy states are skipped, because they can be ignored.
    */
   CustomOrderedSet<EdgeDescriptor> self_edges;
   CustomUnorderedSet<std::pair<vertex, vertex>> epp_edges_to_add;
   BOOST_FOREACH(const vertex v, boost::vertices(*stg))
   {
      if(stg->CGetStateInfo(v)->is_dummy)
      {
         // skip dummies
         continue;
      }
      BOOST_FOREACH(EdgeDescriptor oe, boost::out_edges(v, *stg))
      {
         if(stg->GetSelector(oe) & TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK)
         {
            epp_edges_to_add.insert(std::make_pair(v, exit_state));
         }
      }
      BOOST_FOREACH(EdgeDescriptor ie, boost::in_edges(v, *stg))
         if(stg->GetSelector(ie) & TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK)
            epp_edges_to_add.insert(std::make_pair(entry_state, v));
   }
   for(const auto& e : epp_edges_to_add)
   {
      STG->STG_builder->connect_state(e.first, e.second, TransitionInfo::StateTransitionType::ST_EDGE_EPP);
   }
}

static size_t compute_edge_increments(const StateTransitionGraphManagerRef& STG)
{
   /*
    * get the EPP stg, to avoid feedback edges
    */
   size_t res = 0;
   const auto epp_stg = STG->GetEPPStg();
   /*
    * Execute the algorithm described in Figure 5 of the paper, to compute the
    * edge increments. These increments are not definitive, because we have to
    * propagate them on the feedback edges that are temporarily removed so that
    * the algorithm can work properly.
    */
   CustomUnorderedMap<vertex, size_t> NumPaths;
   std::deque<vertex> reverse_v_list;
   epp_stg->ReverseTopologicalSort(reverse_v_list);
   for(const auto v : reverse_v_list)
   {
      if(epp_stg->CGetStateInfo(v)->is_dummy)
      {
         /* dummy states are skipped */
         continue;
      }
      if(boost::out_degree(v, *epp_stg) == 0) // is a leaf vertex
      {
         NumPaths[v] = 1;
      }
      else
      {
         size_t n = 0;
         BOOST_FOREACH(EdgeDescriptor e, boost::out_edges(v, *epp_stg))
         {
            const auto selector = epp_stg->GetSelector(e);
            const auto dst = boost::target(e, *epp_stg);
            if(epp_stg->CGetStateInfo(dst)->is_dummy)
               continue;
            switch(selector)
            {
               case TransitionInfo::StateTransitionType::ST_EDGE_EPP:
               case TransitionInfo::StateTransitionType::ST_EDGE_NORMAL:
                  epp_stg->GetTransitionInfo(e)->set_epp_increment(n);
                  n += NumPaths.at(dst);
                  break;
               default:
                  THROW_UNREACHABLE("");
            }
         }
         NumPaths[v] = n;
      }
      res = std::max(res, NumPaths.at(v));
   }
   return res;
}

void BB_based_stg::compute_EPP_edge_increments(const std::map<vertex, std::list<vertex>>& starting_ops) const
{
   const HWDiscrepancyInfoRef discr_info = HLSMgr->RDiscr->hw_discrepancy_info;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Computing Efficient Path Profiling edge increments for HW discrepancy analysis");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding EPP edges");
   add_EPP_edges(HLS->STG);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Added EPP edges");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Computing EPP edge increments");
   size_t max_path_val = compute_edge_increments(HLS->STG);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Computed EPP edge increments: max_path_val = " + STR(max_path_val));
   if(max_path_val)
   {
      size_t epp_trace_bitsize = 0;
      do
      {
         epp_trace_bitsize++;
         max_path_val >>= 1;
      } while(max_path_val);
      discr_info->fu_id_control_flow_skip.erase(funId);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---fun id " + STR(funId) + "EPP path bits " + STR(epp_trace_bitsize));
      discr_info->fu_id_to_epp_trace_bitsize[funId] = epp_trace_bitsize;
      discr_info->fu_id_to_max_epp_path_val[funId] = max_path_val;
   }
   else
   {
      discr_info->fu_id_control_flow_skip.insert(funId);
      discr_info->fu_id_to_epp_trace_bitsize[funId] = 0;
      discr_info->fu_id_to_max_epp_path_val[funId] = 0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---no control flow discrepancy is necessary");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Computing states where EPP trace must be checked");
   auto& state_id_to_check = discr_info->fu_id_to_states_to_check[funId];
   state_id_to_check.clear();
   auto& state_id_to_check_on_feedback = discr_info->fu_id_to_feedback_states_to_check[funId];
   state_id_to_check_on_feedback.clear();
   auto& reset_edges = discr_info->fu_id_to_reset_edges[funId];
   reset_edges.clear();
   const auto& stg = HLS->STG->CGetStg();
   const auto& stg_info = stg->CGetStateTransitionGraphInfo();
   const OpGraphConstRef dfgRef = HLSMgr->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::DFG);
   for(const auto& state_to_op : starting_ops)
   {
      for(const auto& op : state_to_op.second)
      {
         const technology_nodeConstRef tn = HLS->allocation_information->get_fu(HLS->Rfu->get_assign(op));
         const auto op_id = tree_helper::normalized_ID(dfgRef->CGetOpNodeInfo(op)->GetOperation());
         const technology_nodeConstRef op_tn = GetPointer<const functional_unit>(tn)->get_operation(op_id);
         if(not GetPointer<const operation>(op_tn)->is_bounded()) // TODO actual call, not unbounded
         {
            const auto state_id = stg_info->vertex_to_state_id.at(state_to_op.first);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "state: S_" + STR(state_id) + " is always to check because it contains an unbounded operation");
            state_id_to_check.insert(state_id);
         }
      }
   }
   BOOST_FOREACH(const EdgeDescriptor e, boost::edges(*stg))
   {
      if(stg->GetSelector(e) & TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK)
      {
#if HAVE_ASSERTS || !defined(NDEBUG)
         const auto src = boost::source(e, *stg);
#endif
         const auto dst = boost::target(e, *stg);
         const bool dst_dummy = stg->CGetStateInfo(dst)->is_dummy;
         if(not dst_dummy)
         {
            const auto dst_state_id = stg_info->vertex_to_state_id.at(dst);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "state: S_" + STR(dst_state_id) + " is to check on feedback because it is the destination of a feedback edge");
            state_id_to_check_on_feedback.insert(dst_state_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "edge from state: S_" + STR(stg_info->vertex_to_state_id.at(src)) + " to state: S_" + STR(dst_state_id) + " is to reset");
            reset_edges.insert(e);
         }
         else
         {
            THROW_ASSERT(stg->CGetStateInfo(src)->is_dummy and (src == dst), "");
         }
      }
   }
   const auto exit_state = stg_info->exit_node;
   BOOST_FOREACH(const EdgeDescriptor in_e, boost::in_edges(exit_state, *stg))
   {
      const auto src = boost::source(in_e, *stg);
      const bool src_dummy = stg->CGetStateInfo(src)->is_dummy;
      if(not src_dummy)
      {
         const auto state_id = stg_info->vertex_to_state_id.at(src);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "state: S_" + STR(state_id) + " is always to check because it is before end state");
         state_id_to_check.insert(state_id);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Computed states where EPP trace must be checked");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Computed Efficient Path Profiling edge increments for HW discrepancy analysis");
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      stg->WriteDot("STG_EPP.dot");
      HLS->STG->CGetEPPStg()->WriteDot("EPP.dot");
   }
   return;
}

/**
 * Given two bb linked by a forwarding edge, this method tries to move
 * overlap the execution of the last state of the bb ending the cycle
 * with the execution of the first state of the bb that begins the cycle.
 */
void BB_based_stg::optimize_cycles(vertex bbEndingCycle, CustomUnorderedMap<vertex, vertex>& first_state, CustomUnorderedMap<vertex, vertex>& last_state, std::map<vertex, std::list<vertex>>& global_starting_ops,
                                   std::map<vertex, std::list<vertex>>& global_ending_ops, std::map<vertex, std::list<vertex>>& global_executing_ops, std::map<vertex, std::list<vertex>>& global_onfly_ops)
{
   const BBGraphConstRef fbb = HLSMgr->CGetFunctionBehavior(funId)->CGetBBGraph(FunctionBehavior::FBB);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering bbEndingCycle:" + STR(fbb->CGetBBNodeInfo(bbEndingCycle)->block->number));
   std::list<vertex>::iterator it, findIter;
   // The last state of the cycle
   const StateTransitionGraphConstRef stg = HLS->STG->CGetStg();
   vertex lastst = last_state[bbEndingCycle];
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Last state is " + stg->CGetStateInfo(lastst)->name);
   if(stg->CGetStateInfo(lastst)->is_duplicated)
      return;
   if(boost::in_degree(lastst, *stg) != 1)
      return;

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Compute the second last state of the bb");
   /*
    Compute the second last state of the bb. After the move,
    this state will become the last state of the bb.
    */
   vertex secondLastState = boost::source(*(boost::in_edges(lastst, *stg).first), *stg);
   if(stg->CGetStateInfo(secondLastState)->is_dummy)
      return;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Second last state computed " + stg->CGetStateInfo(secondLastState)->name);

   BOOST_FOREACH(EdgeDescriptor oedge, boost::out_edges(lastst, *stg))
   {
      vertex cstate = boost::target(oedge, *stg);
      if(stg->CGetStateInfo(cstate)->is_duplicated)
         return;
   }
   // std::cerr << "considering this last state " << stg->CGetStateInfo(lastst)->name << std::endl;
   // All the functions called in the last cycle
   std::list<vertex> lastStateStartingOpList = global_starting_ops[lastst];
   // All the operations that need input in the last cycle
   std::list<vertex> lastStateExecutingOpList = global_executing_ops[lastst];
   // All the operations ending in the last cycle
   std::list<vertex> lastStateEndingOpList = global_ending_ops[lastst];
   // All the operations already running at the beginning of the last cycle
   std::list<vertex> lastStateOnFlyOpList = global_onfly_ops[lastst];
   // All the "conditional operations" in the last state
   std::list<vertex> lastStateConditionalOpList;

   if(lastStateEndingOpList.size() == 1)
      return;
   // If the bb is composed by just one state, it is not possible to optimize
   if(first_state[bbEndingCycle] == last_state[bbEndingCycle])
      return;

   /*
    If an operation starts and does not end in the final state,
    the optimization is complete. Any further optimization will
    require rescheduling.
    */
   for(it = lastStateStartingOpList.begin(); it != lastStateStartingOpList.end(); ++it)
   {
      findIter = std::find(lastStateEndingOpList.begin(), lastStateEndingOpList.end(), *it);
      if(findIter == lastStateEndingOpList.end())
         return;
   }
   for(it = lastStateExecutingOpList.begin(); it != lastStateExecutingOpList.end(); ++it)
   {
      findIter = std::find(lastStateEndingOpList.begin(), lastStateEndingOpList.end(), *it);
      if(findIter == lastStateEndingOpList.end())
         return;
   }
   for(it = lastStateOnFlyOpList.begin(); it != lastStateOnFlyOpList.end(); ++it)
   {
      findIter = std::find(lastStateEndingOpList.begin(), lastStateEndingOpList.end(), *it);
      if(findIter == lastStateEndingOpList.end())
         return;
   }
   /*************************************************************
    From now on, lastStateEndingOpList can be considered a list
    containing all the operations of the state.
    *************************************************************/
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic checks passed");
   /*
    Some operations cannot be moved in any case. If any of them
    is executed in the last state, I have to return without performing
    any optimization.
    In addition to this, operations chained to conditional operations
    cannot be moved.
    */
   bool has_STOREs = false;
   const OpGraphConstRef dfgRef = HLSMgr->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::DFG);
   for(it = lastStateEndingOpList.begin(); it != lastStateEndingOpList.end(); ++it)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Check phi");
      // phi operators cannot be moved
      if(GET_TYPE(dfgRef, *it) & (TYPE_PHI | TYPE_VPHI))
         return;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Check return");
      if(GET_TYPE(dfgRef, *it) & TYPE_RET)
         return;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Check switch and multi-if");
      // creates a list containing all the conditional operations
      if(GET_TYPE(dfgRef, *it) & (TYPE_IF))
         lastStateConditionalOpList.push_back(*it);
      else if(GET_TYPE(dfgRef, *it) & (TYPE_MULTIIF))
         return; // lastStateConditionalOpList.push_back(*it);
      else if(GET_TYPE(dfgRef, *it) & (TYPE_SWITCH))
         return; // lastStateConditionalOpList.push_back(*it);

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Check unbounded");
      technology_nodeRef tn = HLS->allocation_information->get_fu(HLS->Rfu->get_assign(*it));
      technology_nodeRef op_tn = GetPointer<functional_unit>(tn)->get_operation(tree_helper::normalized_ID(dfgRef->CGetOpNodeInfo(*it)->GetOperation()));
      if(!GetPointer<operation>(op_tn)->is_bounded())
         return;

      if(GET_TYPE(dfgRef, *it) & TYPE_STORE)
         has_STOREs = true;

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Check resource-constrained");
      // operations executed by resource-constrained fu should not be moved
      if(res_const_operation(*it, lastStateExecutingOpList, lastst))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Resource constraints limit on operation " + dfgRef->CGetOpNodeInfo(*it)->GetOperation());
         return;
      }

      /*
        check if the operation is chained to a conditional operation:
        in this case, I cannot optimize.
        */
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Check data dependency with control operation");
      BOOST_FOREACH(EdgeDescriptor ei, boost::out_edges(*it, *dfgRef))
      {
         vertex tgt_op = boost::target(ei, *dfgRef);
         if(std::find(lastStateExecutingOpList.begin(), lastStateExecutingOpList.end(), tgt_op) == lastStateExecutingOpList.end())
            continue;
         if(GET_TYPE(dfgRef, tgt_op) & (TYPE_IF))
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "depending if");
            return;
         }
         if(GET_TYPE(dfgRef, tgt_op) & (TYPE_MULTIIF))
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "depending multi if");
            return;
         }
         if(GET_TYPE(dfgRef, tgt_op) & (TYPE_SWITCH))
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "depending switch");
            return;
         }
      }
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Creating list of following bb");
   /*
    Create a list of all the bb following the last bb of the cycle
    that I am optimizing.
    */
   std::list<vertex> followingBb;
   vertex tempBb;

   BOOST_FOREACH(EdgeDescriptor ei, boost::out_edges(bbEndingCycle, *fbb))
   {
      tempBb = boost::target(ei, *fbb);
      /// If the exit vertex follows the ending bb,
      /// I cannot perform any optimization.
      /// The same happen for next BBs having a multi way if, load, store or return operations.

      if(tempBb == fbb->CGetBBGraphInfo()->exit_vertex)
         return;
      if(bbEndingCycle == tempBb)
         followingBb.push_back(tempBb);
      else
         followingBb.push_front(tempBb);
      if(has_STOREs)
      {
         for(auto next_ops : global_starting_ops[first_state[tempBb]])
         {
            const auto operation = dfgRef->CGetOpNodeInfo(next_ops);
            if((GET_TYPE(dfgRef, next_ops) & (TYPE_STORE | TYPE_LOAD | TYPE_EXTERNAL)) != 0)
               return;
         }
      }
      else
      {
         for(auto next_ops : global_starting_ops[first_state[tempBb]])
         {
            if((GET_TYPE(dfgRef, next_ops) & TYPE_MULTIIF))
               return;
         }
      }
      // std::cerr << fbb->CGetBBNodeInfo(tempBb)->get_bb_index() << std::endl;
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "check if it is possible to move the last state");
   /*
    check if it is possible to move the last state of the cycle at the beginning
    of every adjacent bb.
    */
   for(auto bbVertex : followingBb)
   {
      if(!can_be_moved(lastStateEndingOpList, lastStateConditionalOpList, first_state[bbVertex], global_starting_ops, global_executing_ops))
         return;
   }

   /********************************************************
    If I arrive here, it is possible to move the last state
    *********************************************************/

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "compute which variables are defined and used");
   /*
    compute which variables are defined and used by the operations that will be
    moved, and check that it is possible to store in registers all the used variables
    */
   CustomOrderedSet<unsigned int> useSet, defSet;
   compute_use_def(lastStateEndingOpList, lastStateExecutingOpList, lastStateConditionalOpList, useSet, defSet, dfgRef);

   /*
    * check if destination states define a variable in the useSet
    * This check is required by the current interconnection algorithm
    */
   bool is_dest_first_st_equal_to_second = false;
   for(auto bbVertex : followingBb)
   {
      vertex dest_first_state = first_state[bbVertex];
      if(dest_first_state == secondLastState)
      {
         is_dest_first_st_equal_to_second = true;
         break;
      }
   }
   if(is_dest_first_st_equal_to_second)
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "is_dest_first_st_equal_to_second is true!");
   for(auto bbVertex : followingBb)
   {
      vertex dest_first_state = first_state[bbVertex];
      bool skip_phi = true;
      if(boost::in_degree(dest_first_state, *stg) > 1)
      {
         if(!is_dest_first_st_equal_to_second)
         {
            BOOST_FOREACH(EdgeDescriptor prev_edge, boost::out_edges(dest_first_state, *stg))
            {
               vertex next_state = boost::target(prev_edge, *stg);
               if(next_state == dest_first_state)
               {
                  skip_phi = false;
                  break;
               }
            }
         }
         else
         {
            if(dest_first_state != secondLastState)
            {
               skip_phi = false;
            }
         }
      }
      if(skip_phi)
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Phi can be skipped!");
      for(auto stmt : global_ending_ops[dest_first_state])
      {
         if(skip_phi && (GET_TYPE(dfgRef, stmt) & TYPE_PHI))
            continue;
         if(GET_TYPE(dfgRef, stmt) & (TYPE_VPHI))
            continue;
         const CustomSet<unsigned int>& scalar_defs = dfgRef->CGetOpNodeInfo(stmt)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);
         if(not scalar_defs.empty())
            for(auto tree_var : scalar_defs)
               if(HLSMgr->is_register_compatible(tree_var) && useSet.find(tree_var) != useSet.end())
                  return;
      }
   }
   std::map<vertex, std::size_t> in_degree_following_Bb;
   for(auto bbVertex : followingBb)
   {
      in_degree_following_Bb[bbVertex] = boost::in_degree(first_state[bbVertex], *stg);
      if(in_degree_following_Bb[bbVertex] > 2)
         return;
   }

   for(auto bbVertex : followingBb)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "analyzing BB:" + STR(fbb->CGetBBNodeInfo(bbVertex)->block->number));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "first state:" + stg->CGetStateInfo(first_state[bbVertex])->name);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "last state:" + stg->CGetStateInfo(last_state[bbVertex])->name);
      if(last_state[bbVertex] == first_state[bbVertex])
      {
         BOOST_FOREACH(EdgeDescriptor ei, boost::out_edges(bbVertex, *fbb))
         {
            vertex tgtBB = boost::target(ei, *fbb);
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "target BB:" + STR(fbb->CGetBBNodeInfo(tgtBB)->block->number));
            if(boost::in_degree(first_state[tgtBB], *stg) > 1)
               return;
         }
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "One state can be moved!");

   /*
    Move all the conditional operations from the last state to
    the second last state, removing them from lastState...OpList.
    There is no need of removing these operations form the vertex
    representing the last state in the STG, since that state will
    be deleted later in this method.
    */
   StateTransitionGraphRef wstg = HLS->STG->GetStg();
   for(auto ops : lastStateConditionalOpList)
   {
      // update global_... maps
      global_starting_ops[lastst].remove(ops);
      global_executing_ops[lastst].remove(ops);
      global_ending_ops[lastst].remove(ops);
      global_starting_ops[secondLastState].push_back(ops);
      global_executing_ops[secondLastState].push_back(ops);
      global_ending_ops[secondLastState].push_back(ops);
      // update the second last state in the STG
      wstg->GetStateInfo(secondLastState)->starting_operations.push_back(ops);
      wstg->GetStateInfo(secondLastState)->executing_operations.push_back(ops);
      wstg->GetStateInfo(secondLastState)->ending_operations.push_back(ops);
   }

   /*
    Move all the non-conditional operations to the first state of the following bbs
    */
   for(auto bbVertex : followingBb)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "analyzing BB:" + STR(fbb->CGetBBNodeInfo(bbVertex)->block->number));
      if(in_degree_following_Bb[bbVertex] > 1)
         move_with_duplication(lastst, secondLastState, first_state[bbVertex], global_starting_ops, global_executing_ops, global_ending_ops, defSet, useSet);
      else
         move_without_duplication(lastst, secondLastState, first_state[bbVertex], global_starting_ops, global_executing_ops, global_ending_ops, defSet, useSet);
   }

   /***********************************************************
    Now I can delete the last state of the bb ending the cycle,
    and all the edges entering and exiting from it
    **********************************************************/
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Deleting the last state of the cycle..");
   /*
    update the last_state map
    */
   last_state[bbEndingCycle] = secondLastState;
   /*
    delete the edge entering the last state
    */
   HLS->STG->STG_builder->delete_edge(secondLastState, lastst);
   // std::cerr << "deleting egde " << stg->CGetStateInfo(secondLastState)->name << "->" << stg->CGetStateInfo(lastst)->name << std::endl;
   /*
    delete all the edges exiting the last state
    */
   std::list<vertex> linkedStates;
   BOOST_FOREACH(EdgeDescriptor oedge, boost::out_edges(lastst, *stg))
   {
      vertex cstate = boost::target(oedge, *stg);
      linkedStates.push_back(cstate);
   }
   for(auto cstate : linkedStates)
   {
      HLS->STG->STG_builder->delete_edge(lastst, cstate);
      // std::cerr << "deleting egde " << stg->CGetStateInfo(lastst)->name << "->" << stg->CGetStateInfo(*its)->name << std::endl;
   }

   // std::cerr << "deleting " << stg->CGetStateInfo(lastst)->name << std::endl;
   /*
    delete the last state
    */
   HLS->STG->STG_builder->delete_state(lastst);

   return;
}

/**
 * Returns true if all the operations in the list can be moved to the state
 * specified.
 * Operations contained in the vertx list lastStateConditionalOpList
 * will be ignored in the analysis.
 * This method works fine only if the list of operation contains all the
 * operations executed in a state preceding the one passed as a parameter.
 */
bool BB_based_stg::can_be_moved(std::list<vertex>& lastStateEndingOp, std::list<vertex>& lastStateConditionalOpList, vertex firstStateNextBb, std::map<vertex, std::list<vertex>>& global_starting_ops,
                                std::map<vertex, std::list<vertex>>& global_executing_ops)
{
   vertex tempDependentOp;
   std::list<vertex>::iterator it, findIter;

   for(it = lastStateEndingOp.begin(); it != lastStateEndingOp.end(); ++it)
   {
      findIter = std::find(lastStateConditionalOpList.begin(), lastStateConditionalOpList.end(), *it);
      /*
       conditional operations should not be considered in this analysis,
       because they will be moved in a different way.
       */
      if(findIter == lastStateConditionalOpList.end())
      {
         if(is_instantaneous_operation(*it))
            continue; // instantaneous operations can always be moved
         tempDependentOp = check_data_dependency(*it, firstStateNextBb, global_starting_ops, global_executing_ops);
         if(tempDependentOp == nullptr)
            continue; // no data dependency, it is possible to move that operation
         if(is_instantaneous_operation(tempDependentOp))
         {
            continue; /* the operation will be chained to an instantaneous operation,
                         in the destination state, so it is possible to move it */
         }
         return false; // data dependencies prevent this operation from being moved
      }
   }
   return true;
}

/**
 * This method takes as parameters an operation and a state
 * of the STG graph, and returns the operation that needs
 * the output of the given operation in the given state, nullptr otherwise.
 * If the result of the given operation is read by a phi operation chained
 * with a second operation, a pointer to that second operation is returned.
 * In case the result of the given operation is read by a phi which is not
 * chained to any other operation, and no other operation needs the output
 * of the given operation, a pointer to the phi itself is returned.
 *
 * NOTICE that an instantaneous operation is treated as a weaker dependence,
 * because it can be solved via chaining. An instantaneous operation is
 * returned only if no stronger data dependency is found.
 */
vertex BB_based_stg::check_data_dependency(vertex operation, vertex state, std::map<vertex, std::list<vertex>>& global_starting_ops, std::map<vertex, std::list<vertex>>& global_executing_ops)
{
   std::list<vertex> StartingOpList = global_starting_ops[state];
   std::list<vertex> ExecutingOpList = global_executing_ops[state];
   std::list<vertex>::iterator it;
   vertex dependentOperation;

   for(it = StartingOpList.begin(); it != StartingOpList.end(); ++it)
   {
      if(HLSMgr->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::DFG)->ExistsEdge(operation, *it))
      {
         if(is_instantaneous_operation(*it))
         {
            dependentOperation = check_data_dependency(*it, state, global_starting_ops, global_executing_ops);
            if(dependentOperation != nullptr)
               return dependentOperation;
         }
         return *it;
      }
   }

   for(it = ExecutingOpList.begin(); it != ExecutingOpList.end(); ++it)
   {
      if(HLSMgr->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::DFG)->ExistsEdge(operation, *it))
      {
         if(is_instantaneous_operation(*it))
         {
            dependentOperation = check_data_dependency(*it, state, global_starting_ops, global_executing_ops);
            if(dependentOperation != nullptr)
               return dependentOperation;
         }
         return *it;
      }
   }

   return nullptr;
}

/**
 * returns true if the operation takes no time
 */
bool BB_based_stg::is_instantaneous_operation(vertex operation)
{
   const OpGraphConstRef dfgRef = HLSMgr->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::DFG);

   // The operation is a phi
   if(GET_TYPE(dfgRef, operation) & (TYPE_PHI))
      return true;

   if(dfgRef->CGetOpNodeInfo(operation)->GetOperation() == ASSIGN)
      return true;

   if(dfgRef->CGetOpNodeInfo(operation)->GetOperation() == NOP_EXPR)
      return true;

   if(dfgRef->CGetOpNodeInfo(operation)->GetOperation() == CONVERT_EXPR)
      return true;

   if(dfgRef->CGetOpNodeInfo(operation)->GetOperation() == VIEW_CONVERT_EXPR)
      return true;

   return false;
}

/**
 * returns true if the number of fu available
 * prevents us from moving that operation in the next state
 */
bool BB_based_stg::res_const_operation(vertex& operation, std::list<vertex>& lastStateExecutingOpList, vertex lastst)
{
   unsigned int currentUnitID = HLS->Rfu->get_assign(operation);
   if(HLS->allocation_information->get_number_fu(currentUnitID) == INFINITE_UINT)
      return false;

   /*
    * if the number of fu of that type is limited, we have to
    * make sure that it is feasible to move that
    * operation in the following states
    */
   unsigned int currentlyNeededUnit = 0;

   for(auto tempOp : lastStateExecutingOpList)
      if(currentUnitID == HLS->Rfu->get_assign(tempOp))
         currentlyNeededUnit++;
   const StateTransitionGraphConstRef stg = HLS->STG->CGetStg();
   BOOST_FOREACH(EdgeDescriptor oedge, boost::out_edges(lastst, *stg))
   {
      vertex tempSt = boost::target(oedge, *stg);
      unsigned int tempNeededUnit = 0;
      for(auto tempOp : HLS->STG->GetStg()->GetStateInfo(tempSt)->executing_operations)
      {
         if(currentUnitID == HLS->Rfu->get_assign(tempOp))
            tempNeededUnit++;
      }
      if(tempNeededUnit + currentlyNeededUnit > HLS->allocation_information->get_number_fu(currentUnitID))
      {
         return true;
      }
   }

   return false;
}

/**
 * computes the variables used and defined in the
 * by the given list of operations, and saves them in the two sets.
 *
 * operations included in the ignoreList will not be considered in
 * this analysis.
 */
void BB_based_stg::compute_use_def(const std::list<vertex>& opEndingList, const std::list<vertex>& opRuningList, const std::list<vertex>& ignoreList, CustomOrderedSet<unsigned int>& useSet, CustomOrderedSet<unsigned int>& defSet,
                                   const OpGraphConstRef data)
{
   /*
    Clear useSet and defSet
    */
   useSet.clear();
   defSet.clear();

   /*
    For each operation, I get the lista of SSA variables used and
    defined, and I update useSet and defSet
    */

   for(const auto cur_op : opEndingList)
   {
      if(std::find(ignoreList.begin(), ignoreList.end(), cur_op) != ignoreList.end())
         continue;

      const CustomSet<unsigned int>& scalar_defs = data->CGetOpNodeInfo(cur_op)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);

      for(auto tree_var : scalar_defs)
         if(HLSMgr->is_register_compatible(tree_var))
            defSet.insert(tree_var);
   }

   for(const auto cur_op : opRuningList)
   {
      if(std::find(ignoreList.begin(), ignoreList.end(), cur_op) != ignoreList.end())
         continue;

      const CustomSet<unsigned int>& scalar_uses = data->CGetOpNodeInfo(cur_op)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE);

      for(auto tree_var : scalar_uses)
         if(HLSMgr->is_register_compatible(tree_var) and defSet.find(tree_var) == defSet.end())
            useSet.insert(tree_var);
   }

   return;
}

/**
 * Copies all the operations of the state to move in the following.
 * An edge is created from the second last state to the destination state.
 * The state to move, and all the edges to/from it are not modified by this method.
 */
void BB_based_stg::move_without_duplication(const vertex stateToMove, const vertex secondLastState, const vertex destinationState, const std::map<vertex, std::list<vertex>>& global_starting_ops,
                                            const std::map<vertex, std::list<vertex>>& global_executing_ops, const std::map<vertex, std::list<vertex>>& global_ending_ops, const CustomOrderedSet<unsigned int>& defSet,
                                            const CustomOrderedSet<unsigned int>& useSet)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Moving without duplication");
   StateTransitionGraphManagerRef STGMan = HLS->STG;
   StateTransitionGraphRef stg = STGMan->GetStg();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "moving operations from state: " + STGMan->get_state_name(stateToMove));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "moving operations to state: " + STGMan->get_state_name(destinationState));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "second last state: " + STGMan->get_state_name(secondLastState));
   const auto dstStateInfo = stg->GetStateInfo(destinationState);
   dstStateInfo->moved_op_use_set = useSet;
   dstStateInfo->moved_op_def_set = defSet;
   /*
    * copy in the destination state all the operations that are
    * executed in the state to move.
    */
   {
      const auto op_it = global_executing_ops.find(stateToMove);
      if(op_it != global_executing_ops.end())
      {
         BOOST_REVERSE_FOREACH(vertex operation, op_it->second)
         {
            dstStateInfo->executing_operations.push_front(operation);
            dstStateInfo->moved_exec_op.push_front(operation);
         }
      }
   }
   {
      const auto op_it = global_starting_ops.find(stateToMove);
      if(op_it != global_starting_ops.end())
      {
         BOOST_REVERSE_FOREACH(vertex operation, op_it->second)
         {
            dstStateInfo->starting_operations.push_front(operation);
         }
      }
   }
   {
      const auto op_it = global_ending_ops.find(stateToMove);
      if(op_it != global_ending_ops.end())
      {
         BOOST_REVERSE_FOREACH(vertex operation, op_it->second)
         {
            dstStateInfo->ending_operations.push_front(operation);
            dstStateInfo->moved_ending_op.push_front(operation);
         }
      }
   }
   /*
    * create an edge from the second last state to the new state
    */
   EdgeDescriptor linkingEdge = stg->CGetEdge(stateToMove, destinationState);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "existing edge: " + STGMan->get_state_name(stateToMove) + "->" + STGMan->get_state_name(destinationState));
   EdgeDescriptor newEdge = HLS->STG->STG_builder->connect_state(secondLastState, destinationState, stg->GetSelector(linkingEdge));
   HLS->STG->STG_builder->copy_condition(newEdge, linkingEdge);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "new edge: " + STGMan->get_state_name(secondLastState) + "->" + STGMan->get_state_name(destinationState));

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Moved without duplication");
   return;
}

/**
 * Duplicates the first state of the destination bb and copies all the operations
 * of the state to move in the new state. An edge is created from the second last
 * state to the new state, and outgoing edges are created from the new state to all
 * the states that follows the first state of the destination bb.
 * The state to move, and all the edges to/from it are not modified by this method.
 */
void BB_based_stg::move_with_duplication(const vertex stateToMove, const vertex secondLastState, const vertex stateToClone, const std::map<vertex, std::list<vertex>>& global_starting_ops, const std::map<vertex, std::list<vertex>>& global_executing_ops,
                                         const std::map<vertex, std::list<vertex>>& global_ending_ops, const CustomOrderedSet<unsigned int>& defSet, const CustomOrderedSet<unsigned int>& useSet)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Moving with duplication");
   StateTransitionGraphRef stg = HLS->STG->GetStg();
   const auto toMoveStateInfo = stg->GetStateInfo(stateToMove);
   const auto toCloneStateInfo = stg->GetStateInfo(stateToClone);
   const auto secondLastStateInfo = stg->GetStateInfo(secondLastState);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "moving operations from state: " + toMoveStateInfo->name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "moving operations to state: " + toCloneStateInfo->name + " that will be cloned");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "second last state: " + secondLastStateInfo->name);
   unsigned int currentBbID = *toMoveStateInfo->BB_ids.begin();
   /*
    * clone the first state of the destination bb
    */
   vertex clonedState = HLS->STG->STG_builder->create_state(toCloneStateInfo->executing_operations, toCloneStateInfo->starting_operations, toCloneStateInfo->ending_operations, toCloneStateInfo->BB_ids);
   // set is_duplicated to true, and saves info about the source Bb of the data
   const auto clonedStateInfo = stg->GetStateInfo(clonedState);
   clonedStateInfo->is_duplicated = true;
   clonedStateInfo->sourceBb = currentBbID;
   clonedStateInfo->clonedState = stateToClone;
   clonedStateInfo->moved_op_def_set = defSet;
   clonedStateInfo->moved_op_use_set = useSet;
   toCloneStateInfo->is_duplicated = true;
   toCloneStateInfo->isOriginalState = true;
   toCloneStateInfo->sourceBb = currentBbID;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "cloned state: " + toCloneStateInfo->name + " new clone: " + clonedStateInfo->name);

   {
      const auto op_it = global_executing_ops.find(stateToMove);
      if(op_it != global_executing_ops.end())
      {
         BOOST_REVERSE_FOREACH(vertex operation, op_it->second)
         {
            clonedStateInfo->executing_operations.push_front(operation);
            clonedStateInfo->moved_exec_op.push_front(operation);
         }
      }
   }
   {
      const auto op_it = global_starting_ops.find(stateToMove);
      if(op_it != global_starting_ops.end())
      {
         BOOST_REVERSE_FOREACH(vertex operation, op_it->second)
         {
            clonedStateInfo->starting_operations.push_front(operation);
         }
      }
   }
   {
      const auto op_it = global_ending_ops.find(stateToMove);
      if(op_it != global_ending_ops.end())
      {
         BOOST_REVERSE_FOREACH(vertex operation, op_it->second)
         {
            clonedStateInfo->ending_operations.push_front(operation);
            clonedStateInfo->moved_ending_op.push_front(operation);
         }
      }
   }

   /*
    * create a NORMAL edge from the second last state to the new state, and set the conditions
    */
   EdgeDescriptor linkingEdge = stg->CGetEdge(stateToMove, stateToClone);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "existing edge: " + toMoveStateInfo->name + "->" + toCloneStateInfo->name);
   EdgeDescriptor newEdge = HLS->STG->STG_builder->connect_state(secondLastState, clonedState, stateToClone != secondLastState ? stg->GetSelector(linkingEdge) : TransitionInfo::StateTransitionType::ST_EDGE_NORMAL);
   HLS->STG->STG_builder->copy_condition(newEdge, linkingEdge);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "new edge: " + secondLastStateInfo->name + "->" + clonedStateInfo->name);
   /*
    * create an edge from the the new state to every state that
    * follows the first state of the destination bb
    */
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "state to clone: " + toCloneStateInfo->name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "second last state: " + secondLastStateInfo->name);
   if(stateToClone != secondLastState)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "state to clone != second last state");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering successors of state to clone");
      BOOST_FOREACH(EdgeDescriptor oe, boost::out_edges(stateToClone, *stg))
      {
         vertex successor = boost::target(oe, *stg);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->successor: " + stg->CGetStateInfo(successor)->name);
         THROW_ASSERT(successor != stateToMove, "unexpected case");
         vertex new_successor = ((successor == stateToMove) ? clonedState : successor);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "new successor: " + stg->CGetStateInfo(new_successor)->name);
         EdgeDescriptor new_edge = HLS->STG->STG_builder->connect_state(clonedState, new_successor, stg->GetSelector(oe));
         HLS->STG->STG_builder->copy_condition(new_edge, oe);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "new edge: " + clonedStateInfo->name + "->" + stg->CGetStateInfo(new_successor)->name);
         if(successor == stateToClone)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "successor: " + stg->CGetStateInfo(successor)->name);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "state to clone: " + toCloneStateInfo->name);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "all path for " + stg->CGetStateInfo(successor)->name + " set to true");
            stg->GetStateInfo(successor)->all_paths = true;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "state to clone == second last state");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering successors of state to clone");
      BOOST_FOREACH(EdgeDescriptor oe, boost::out_edges(stateToClone, *stg))
      {
         vertex successor = boost::target(oe, *stg);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->successor: " + stg->CGetStateInfo(successor)->name);
         if(successor == stateToMove)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--state to move : " + toMoveStateInfo->name);
            continue;
         }
         const auto sel = successor != clonedState ? stg->GetSelector(oe) : TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK;
         EdgeDescriptor new_edge = HLS->STG->STG_builder->connect_state(clonedState, successor, sel);
         HLS->STG->STG_builder->copy_condition(new_edge, oe);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "new edge: " + clonedStateInfo->name + "->" + stg->GetStateInfo(successor)->name);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      /*
      BOOST_FOREACH(EdgeDescriptor oe, boost::out_edges(stateToMove, *stg))
      {
         vertex next_state = boost::target(oe, *stg);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "(F)newEdge " + clonedStateInfo->name + "->" +
               stg->GetStateInfo(next_state == stateToClone ?  clonedState : next_state)->name);
         EdgeDescriptor new_edge = HLS->STG->STG_builder->connect_state(clonedState,
               next_state == stateToClone ?  clonedState : next_state, stg->GetSelector(oe));
         HLS->STG->STG_builder->copy_condition(new_edge, oe);
         if(next_state != stateToClone)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
            "all path for " + stg->GetStateInfo(next_state)->name + "set to true");
            stg->GetStateInfo(next_state)->all_paths = true;
         }
      }
      */
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Moved with duplication");
   return;
}
