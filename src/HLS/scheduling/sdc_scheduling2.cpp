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
 *              Copyright (C) 2014-2024 Politecnico di Milano
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
 * @file sdc_scheduling.cpp
 * @brief New implementation of the SDC scheduling
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "sdc_scheduling2.hpp"

#include "ASLAP.hpp"
#include "DAG_SSSP.hpp"
#include "Parameter.hpp"
#include "allocation.hpp"
#include "allocation_information.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "cpu_time.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"
#include "fu_binding.hpp"
#include "function_frontend_flow_step.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"
#include "schedule.hpp"
#include "sdc_solver.hpp"
#include "simple_code_motion.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"

#include <boost/range/adaptor/reversed.hpp>
#include <list>

CONSTREF_FORWARD_DECL(Schedule);

SDCScheduling2::SDCScheduling2(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                               unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager,
                               const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : SDCScheduling_base(_parameters, _HLSMgr, _function_id, _design_flow_manager, HLSFlowStep_Type::SDC_SCHEDULING,
                         _hls_flow_step_specialization)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

SDCScheduling2::~SDCScheduling2() = default;

void SDCScheduling2::ComputeRelationships(DesignFlowStepSet& relationship,
                                          const DesignFlowStep::RelationshipType relationship_type)
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   if(GetStatus() == DesignFlowStep_Status::SUCCESS)
   {
      if(relationship_type == INVALIDATION_RELATIONSHIP)
      {
         {
            vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(
                FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::SDC_CODE_MOTION, funId));
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step =
                frontend_step != NULL_VERTEX ?
                    design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                    GetPointer<const FrontendFlowStepFactory>(
                        design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))
                        ->CreateFunctionFrontendFlowStep(FrontendFlowStepType::SDC_CODE_MOTION, funId);
            relationship.insert(design_flow_step);
         }
      }
   }
   schedulingBaseStep::ComputeRelationships(relationship, relationship_type);
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
SDCScheduling2::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret =
       schedulingBaseStep::ComputeHLSRelationships(relationship_type);
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::INITIALIZE_HLS, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
#if HAVE_FROM_PRAGMA_BUILT
         if(parameters->getOption<bool>(OPT_parse_pragma))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::OMP_ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::CALLED_FUNCTIONS));
         }
         else
#endif
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

bool SDCScheduling2::HasToBeExecuted() const
{
   if(bb_version == 0)
   {
      return schedulingBaseStep::HasToBeExecuted();
   }
   else
   {
      return false;
   }
}

void SDCScheduling2::sdc_schedule(std::map<vertex, int>& vals_vertex, const hlsRef HLS, const HLS_managerRef HLSMgr,
                                  unsigned function_id, const OpVertexSet& loop_operations,
                                  const std::set<vertex, bb_vertex_order_by_map>& loop_bbs,
                                  const BBGraphConstRef basic_block_graph, const OpGraphConstRef filtered_op_graph,
                                  const AllocationInformationConstRef allocation_information,
                                  const ParameterConstRef parameters, int debug_level)
{
   memoryConstRef Rmem = HLSMgr->Rmem;
   auto clock_period = HLS->HLS_C->get_clock_period() * HLS->HLS_C->get_clock_period_resource_fraction();
   auto FB = HLSMgr->CGetFunctionBehavior(function_id);
   auto behavioral_helper = FB->CGetBehavioralHelper();
   const OpGraphConstRef filtered_dfg_graph = FB->CGetOpGraph(FunctionBehavior::DFG, loop_operations);
   /// Create the solver for the scheduling problem.
   /// In particular, we use the reverse constraint graph to obtain the ALAP version of the SDC scheduling problem.
   sdc_solver solver;
   /// Map operation-stage to variable index
   CustomUnorderedMap<vertex, unsigned int> operation_to_varindex;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing variables");
   /// Compute variables
   /// Map real operation-stage to variable index
   unsigned int next_var_index = 0;
   for(const auto loop_operation : loop_operations)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Creating variables for " + GET_NAME(filtered_op_graph, loop_operation) + " (executed by " +
                         " " +
                         allocation_information->get_fu_name(allocation_information->GetFuType(loop_operation)).first +
                         ")  " + ": " + STR(next_var_index));
      operation_to_varindex[loop_operation] = next_var_index;
      next_var_index++;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed " + STR(next_var_index) + " vertices");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding dependencies constraints");
   /// Add dependence constraints: target can start in the same clock cycle in which source ends
   for(const auto operation : loop_operations)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Adding dependencies starting from " + GET_NAME(filtered_op_graph, operation));
      OutEdgeIterator oe, oe_end;
      for(boost::tie(oe, oe_end) = boost::out_edges(operation, *filtered_op_graph); oe != oe_end; oe++)
      {
         auto tgt = boost::target(*oe, *filtered_op_graph);
         if(filtered_op_graph->CGetOpNodeInfo(operation)->GetNodeId() == ENTRY_ID)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Added dependence constraint 0 " + GET_NAME(filtered_op_graph, operation) + "-" +
                               GET_NAME(filtered_op_graph, tgt));
            solver.add_constraint(operation_to_varindex.at(operation), operation_to_varindex.at(tgt), 0);
         }
         else
         {
            auto chainingP = allocation_information->CanBeChained(operation, tgt);
            /// check first chaining
            if(!chainingP)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Added no-chaining constraint -1 " + GET_NAME(filtered_op_graph, operation) + "-" +
                                  GET_NAME(filtered_op_graph, tgt));
               solver.add_constraint(operation_to_varindex.at(operation), operation_to_varindex.at(tgt), -1);
            }
            /// Not control dependence
            else if(filtered_op_graph->GetSelector(*oe) & ~CDG_SELECTOR)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Added dependence constraint 0 " + GET_NAME(filtered_op_graph, operation) + "-" +
                                  GET_NAME(filtered_op_graph, tgt));
               solver.add_constraint(operation_to_varindex.at(operation), operation_to_varindex.at(tgt), 0);
            }
            /// Non speculable operation
            else if(!behavioral_helper->CanBeSpeculated(filtered_op_graph->CGetOpNodeInfo(tgt)->GetNodeId()))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Added dependence constraint -1 " + GET_NAME(filtered_op_graph, operation) + "-" +
                                  GET_NAME(filtered_op_graph, tgt));
               solver.add_constraint(operation_to_varindex.at(operation), operation_to_varindex.at(tgt), -1);
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Skipped dependence -> " + GET_NAME(filtered_op_graph, tgt));
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Added dependencies starting from " + GET_NAME(filtered_op_graph, operation));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added dependencies constraints");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding timing constraints");
   DAG_SSSP ssspSolver(loop_operations.size());
   std::map<unsigned int, double> op_timing;
   std::set<unsigned int> op_multicycles;
   std::list<vertex> unbounded_operations;
   std::set<unsigned int> indirect_ld_st_operations;
   std::set<unsigned> indirect_rw_ext_operations;
   auto setupDelay = allocation_information->get_setup_hold_time();
   CustomUnorderedSet<vertex> RW_stmts;
   compute_RW_stmts(RW_stmts, filtered_op_graph, HLSMgr, function_id);
   for(const auto operation : loop_operations)
   {
      auto opFuType = allocation_information->GetFuType(operation);
      auto timeLatency = allocation_information->GetTimeLatency(operation, opFuType);
      auto isPipelined = allocation_information->get_initiation_time(opFuType, operation) > 0;
      auto op_varindex = operation_to_varindex.at(operation);
      auto is_cond_op = GET_TYPE(filtered_dfg_graph, operation) & (TYPE_IF | TYPE_MULTIIF | TYPE_SWITCH);
      auto is_ld_st = GET_TYPE(filtered_dfg_graph, operation) & (TYPE_LOAD | TYPE_STORE);
      auto is_rw_ext = RW_stmts.find(operation) == RW_stmts.end() &&
                       (GET_TYPE(filtered_dfg_graph, operation) & (TYPE_EXTERNAL | TYPE_RW));
      auto cycles = allocation_information->GetCycleLatency(operation);
      if(is_cond_op)
      {
         op_timing[op_varindex] = allocation_information->estimate_controller_delay_fb();
      }
      else
      {
         op_timing[op_varindex] = (isPipelined ? timeLatency.second : timeLatency.first);
         auto addCtrlDelay = parameters->getOption<double>(OPT_scheduling_mux_margins) != 0.0;
         if(addCtrlDelay)
         {
            op_timing[op_varindex] += allocation_information->EstimateControllerDelay();
         }
      }
      if(!allocation_information->is_operation_bounded(filtered_dfg_graph, operation, opFuType))
      {
         unbounded_operations.push_back(operation);
      }
      if(is_ld_st && !allocation_information->is_direct_access_memory_unit(opFuType))
      {
         indirect_ld_st_operations.insert(op_varindex);
      }
      if(is_rw_ext)
      {
         indirect_rw_ext_operations.insert(op_varindex);
      }
      if(cycles > 1)
      {
         op_multicycles.insert(op_varindex);
      }
      OutEdgeIterator oe, oe_end;
      for(boost::tie(oe, oe_end) = boost::out_edges(operation, *filtered_dfg_graph); oe != oe_end; oe++)
      {
         auto tgt = boost::target(*oe, *filtered_dfg_graph);
         auto chainingP = allocation_information->CanBeChained(operation, tgt);
         auto edge_type = filtered_dfg_graph->GetSelector(*oe);
         if((!(edge_type & FB_DFG_SELECTOR)) && chainingP)
         {
            const double edge_delay = [&]() -> double {
               const auto operation_bb = filtered_dfg_graph->CGetOpNodeInfo(operation)->bb_index;
               const auto tgt_bb = filtered_dfg_graph->CGetOpNodeInfo(operation)->bb_index;
               auto connection_contrib =
                   operation_bb == tgt_bb ? allocation_information->GetConnectionTime(
                                                operation, tgt, AbsControlStep(operation_bb, AbsControlStep::UNKNOWN)) :
                                            0.0;
               auto fsm_correction =
                   (allocation_information->is_one_cycle_direct_access_memory_unit(opFuType) &&
                    (!allocation_information->is_readonly_memory_unit(opFuType) ||
                     (!parameters->isOption(OPT_rom_duplication) ||
                      !parameters->getOption<bool>(OPT_rom_duplication))) &&
                    Rmem->get_maximum_references(allocation_information->is_memory_unit(opFuType) ?
                                                     allocation_information->get_memory_var(opFuType) :
                                                     allocation_information->get_proxy_memory_var(opFuType)) >
                        allocation_information->get_number_channels(opFuType)) ?
                       allocation_information->EstimateControllerDelay() :
                       0.0;
               return fsm_correction + connection_contrib +
                      (isPipelined ? (cycles - 1) * clock_period + timeLatency.second : timeLatency.first);
            }();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---DFG dependence delay " + GET_NAME(filtered_dfg_graph, operation) + "-" +
                               GET_NAME(filtered_dfg_graph, tgt) + " " + STR(edge_delay));
            ssspSolver.add_edge(op_varindex, operation_to_varindex.at(tgt), -edge_delay);
         }
      }
   }
   ssspSolver.init();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Built the delay graph");

   for(const auto operation : loop_operations)
   {
      std::vector<double> vals;
      auto op_varindex = operation_to_varindex.at(operation);
      ssspSolver.exec(op_varindex, vals);
      double max_delay = 0.0;
      unsigned int val_index = 0;
      for(auto v : vals)
      {
         auto del = -v;
         if(del > 0.0)
         {
            auto localDelay = del + op_timing.at(val_index) + setupDelay;
            int w = static_cast<int>(std::ceil((localDelay) / clock_period)) - 1;
            max_delay = std::max(max_delay, localDelay);
            solver.add_constraint(op_varindex, val_index, -w);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---timing constraint " + GET_NAME(filtered_op_graph, operation) + "-" + STR(val_index) +
                               " " + STR(w) + " " + STR(localDelay));
         }
         ++val_index;
      }
      const auto operation_bb = basic_block_graph->CGetBBGraphInfo()->bb_index_map.at(
          filtered_dfg_graph->CGetOpNodeInfo(operation)->bb_index);
      auto laststmt = *(basic_block_graph->CGetBBNodeInfo(operation_bb)->statements_list.rbegin());
      if(laststmt != operation)
      {
         auto laststmt_type = GET_TYPE(filtered_dfg_graph, laststmt);
         if((laststmt_type & (TYPE_IF | TYPE_MULTIIF)) != 0)
         {
            int w = 0;
            if(max_delay != 0.0)
            {
               w = static_cast<int>(std::ceil((max_delay) / clock_period)) - 1;
            }
            auto laststmt_index = operation_to_varindex.at(laststmt);
            solver.add_constraint(op_varindex, laststmt_index, -w);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---last statement constraint " + GET_NAME(filtered_op_graph, operation) + "-" +
                               GET_NAME(filtered_op_graph, laststmt) + " " + STR(w) + " " + STR(max_delay));
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added timing constraint");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding sorting constraint");
   SDCSorter sdc_sorter(FB, filtered_op_graph);

   /// For each basic block, for each functional unit fu, the list of the last n operations executed (n is the number
   /// of resource of type fu) - Value is a set since there can be different paths reaching current basic block
   CustomMap<vertex, CustomMap<unsigned int, CustomOrderedSet<std::list<vertex>>>> constrained_operations_sequences;
   for(const auto basic_block : loop_bbs)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Adding sorting constraints for BB" +
                         STR(basic_block_graph->CGetBBNodeInfo(basic_block)->block->number));

      InEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::in_edges(basic_block, *basic_block_graph); ie != ie_end; ie++)
      {
         const auto source = boost::source(*ie, *basic_block_graph);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Considering source BB" + STR(basic_block_graph->CGetBBNodeInfo(source)->block->number));
         if(loop_bbs.count(source))
         {
            for(const auto& fu_type : constrained_operations_sequences[source])
            {
               constrained_operations_sequences[basic_block][fu_type.first].insert(fu_type.second.begin(),
                                                                                   fu_type.second.end());
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      std::set<vertex, SDCSorter> basic_block_sorted_operations(sdc_sorter);
      for(const auto operation : basic_block_graph->CGetBBNodeInfo(basic_block)->statements_list)
      {
         const auto fu_type = allocation_information->GetFuType(operation);
         const unsigned int resources_number = allocation_information->get_number_fu(fu_type);
         if(resources_number < INFINITE_UINT && !allocation_information->is_vertex_bounded(fu_type))
         {
            basic_block_sorted_operations.insert(operation);
         }
      }
      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Ordered statements");
#ifndef NDEBUG
         for(const auto debug_operation : basic_block_sorted_operations)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---" + GET_NAME(filtered_op_graph, debug_operation));
         }
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      for(const auto operation : basic_block_sorted_operations)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Considering " + GET_NAME(filtered_op_graph, operation));
         /// Resource constraints
         const auto fu_type = allocation_information->GetFuType(operation);
         const unsigned int resources_number = allocation_information->get_number_fu(fu_type);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Mapped on a shared resource");
         const auto& old_sequences = constrained_operations_sequences[basic_block][fu_type];
         if(old_sequences.size())
         {
            CustomOrderedSet<std::list<vertex>> new_sequences;
            for(auto old_sequence : old_sequences)
            {
               if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
               {
                  std::string old_sequence_string;
                  for(const auto temp : old_sequence)
                  {
                     old_sequence_string += GET_NAME(filtered_op_graph, temp) + "-";
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "-->Considering sequence " + old_sequence_string);
               }
               old_sequence.push_back(operation);
               if(old_sequence.size() > resources_number)
               {
                  const auto front = old_sequence.front();
                  old_sequence.pop_front();
                  const std::string name = allocation_information->get_fu_name(fu_type).first + "_" +
                                           GET_NAME(filtered_op_graph, front) + "_" +
                                           GET_NAME(filtered_op_graph, operation);
                  auto frontII = allocation_information->get_initiation_time(fu_type, front);
                  auto cycles = allocation_information->GetCycleLatency(front);
                  auto w = (frontII > 0 ? -from_strongtype_cast<int>(frontII) : -static_cast<int>(cycles));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Resource constraint " + GET_NAME(filtered_op_graph, front) + "-" +
                                     GET_NAME(filtered_op_graph, operation) + " " + STR(w));
                  solver.add_constraint(operation_to_varindex.at(front), operation_to_varindex.at(operation), w);
               }
               if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
               {
                  std::string old_sequence_string;
                  for(const auto temp : old_sequence)
                  {
                     old_sequence_string += GET_NAME(filtered_op_graph, temp) + "-";
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--New sequence " + old_sequence_string);
               }
               new_sequences.insert(old_sequence);
            }
            constrained_operations_sequences[basic_block][fu_type] = new_sequences;
         }
         else
         {
            std::list<vertex> temp;
            temp.push_back(operation);
            constrained_operations_sequences[basic_block][fu_type].insert(temp);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Considered " + GET_NAME(filtered_op_graph, operation));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Added sorting constraints for BB" +
                         STR(basic_block_graph->CGetBBNodeInfo(basic_block)->block->number));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added sorting constraints");

   std::map<unsigned, int> vals;
   bool restart_sdc_solver;
   do
   {
      restart_sdc_solver = false;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Solving SDC ");
      auto ilp_result = solver.solve_SDCNeg(vals);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Solved SDC ");
      if(!ilp_result)
      {
         THROW_ERROR("Error in finding ilp solution");
      }

      /// refine the scheduling by adding some further constraints.
      /// Some constraints are not easy to be defined before the scheduling has been computed.
      std::map<int, std::set<unsigned>> reverse_vals;
      auto cond1 = !indirect_rw_ext_operations.empty();
      auto cond2 = !unbounded_operations.empty() && !op_multicycles.empty();
      if(cond1 || cond2)
      {
         for(auto val : vals)
         {
            reverse_vals[val.second].insert(val.first);
         }
      }
      if(cond1)
      {
         std::set<unsigned> visited;
         for(auto op_varindex : indirect_rw_ext_operations)
         {
            visited.insert(op_varindex);
            auto sched_step = vals.at(op_varindex);
            for(auto op : reverse_vals.at(sched_step))
            {
               if(visited.find(op) == visited.end() &&
                  (indirect_rw_ext_operations.find(op) != indirect_rw_ext_operations.end()))
               {
                  restart_sdc_solver = true;
                  solver.add_constraint(op_varindex, op, -1);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---added a precedence constraint between an RW operation and another RW operation " +
                                     STR(op_varindex) + "-" + STR(op));
               }
               else if(op != op_varindex && (indirect_ld_st_operations.find(op) != indirect_ld_st_operations.end()))
               {
                  restart_sdc_solver = true;
                  solver.add_constraint(op_varindex, op, -1);
                  INDENT_DBG_MEX(
                      DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                      "---added a precedence constraint between an RW operation and an indirect load/store operation " +
                          STR(op_varindex) + "-" + STR(op));
               }
            }
         }
      }
      if(cond2)
      {
         for(auto operation : unbounded_operations)
         {
            auto op_varindex = operation_to_varindex.at(operation);
            auto sched_step = vals.at(op_varindex);
            for(auto op : reverse_vals.at(sched_step))
            {
               if(op != op_varindex && (op_multicycles.find(op) != op_multicycles.end()))
               {
                  restart_sdc_solver = true;
                  solver.add_constraint(op_varindex, op, -1);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---added a precedence constraint between an unbounded operation and a multi-cycle "
                                 "operation " +
                                     GET_NAME(filtered_op_graph, operation) + "-" + STR(op));
               }
            }
         }
      }

   } while(restart_sdc_solver);
   for(const auto operation : loop_operations)
   {
      auto op_varindex = operation_to_varindex.at(operation);
      vals_vertex[operation] = vals.find(op_varindex) != vals.end() ? vals.at(op_varindex) : 0;
   }
}

DesignFlowStep_Status SDCScheduling2::InternalExec()
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   auto basic_block_graph = FB->CGetBBGraph(FunctionBehavior::BB);
   auto allocation_information = HLS->allocation_information;
   auto res_binding = HLS->Rfu;
   auto op_graph = FB->CGetOpGraph(FunctionBehavior::FLSAODG);

   const BBGraphConstRef dominators = FB->CGetBBGraph(FunctionBehavior::DOM_TREE);
   const LoopsConstRef loops = FB->CGetLoops();
   const std::map<vertex, unsigned int>& bb_map_levels = FB->get_bb_map_levels();
   ControlStep initial_ctrl_step = ControlStep(0u);
   for(const auto& loop : loops->GetList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Scheduling loop " + STR(loop->GetId()));
      const unsigned int loop_id = loop->GetId();

      /// Vertices not yet added to any tree
      const bb_vertex_order_by_map comp_i(bb_map_levels);
      std::set<vertex, bb_vertex_order_by_map> loop_bbs(comp_i);
      OpVertexSet loop_operations(op_graph);
      VertexIterator bb, bb_end;
      for(boost::tie(bb, bb_end) = boost::vertices(*basic_block_graph); bb != bb_end; bb++)
      {
         if(basic_block_graph->CGetBBNodeInfo(*bb)->loop_id == loop_id)
         {
            loop_bbs.insert(*bb);
            for(const auto stmt : basic_block_graph->CGetBBNodeInfo(*bb)->statements_list)
            {
               loop_operations.insert(stmt);
            }
         }
      }
      const OpGraphConstRef filtered_op_graph = FB->CGetOpGraph(FunctionBehavior::FLSAODG, loop_operations);
      std::map<vertex, int> vals;
      sdc_schedule(vals, HLS, HLSMgr, funId, loop_operations, loop_bbs, basic_block_graph, filtered_op_graph,
                   allocation_information, parameters, debug_level);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Solution:");
      ControlStep last_relative_step = ControlStep(0u);
      for(const auto operation : loop_operations)
      {
         ControlStep current_control_step = ControlStep(static_cast<unsigned int>(vals.at(operation)));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---" + GET_NAME(filtered_op_graph, operation) + " scheduled at relative step " +
                            STR(current_control_step));
         HLS->Rsch->set_execution(operation, current_control_step + initial_ctrl_step);
         HLS->Rsch->set_execution_end(operation, current_control_step + initial_ctrl_step +
                                                     allocation_information->GetCycleLatency(operation) - 1);
         const unsigned int cycle_latency = allocation_information->GetCycleLatency(operation);
         if(last_relative_step < current_control_step + cycle_latency)
         {
            last_relative_step = current_control_step + cycle_latency;
         }
         /// set the binding information
         if(HLS->HLS_C->has_binding_to_fu(GET_NAME(filtered_op_graph, operation)))
         {
            res_binding->bind(operation, allocation_information->GetFuType(operation), 0);
         }
         else
         {
            res_binding->bind(operation, allocation_information->GetFuType(operation));
         }
      }
      initial_ctrl_step = last_relative_step + 1u;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

      /// Check which statements have to be moved
      /// For each statement the basic blocks above which it cannot be moved.
      /// Phi cannot be moved
      /// Operations which depend from the phi cannot be moved before the phi and so on
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking which operations have to be moved");
      CustomUnorderedSet<vertex> RW_stmts;
      compute_RW_stmts(RW_stmts, op_graph, HLSMgr, funId);
      CustomMap<vertex, CustomSet<vertex>> bb_barrier;
      for(const auto loop_bb : loop_bbs)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Checking if operations of BB" +
                            STR(basic_block_graph->CGetBBNodeInfo(loop_bb)->block->number) + " can be moved");
         /// Set of operations which cannot be moved (at the moment) because of dependencies from gimple phi
         OpVertexSet blocked_ops = OpVertexSet(filtered_op_graph);

         for(const auto loop_operation : basic_block_graph->CGetBBNodeInfo(loop_bb)->statements_list)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Checking if " + GET_NAME(filtered_op_graph, loop_operation) + " has to be moved");
            auto curr_vertex_type = GET_TYPE(filtered_op_graph, loop_operation);
            if((curr_vertex_type & (TYPE_IF | TYPE_MULTIIF)) != 0)
            {
               /// IF is not a barrier for controlled operations
               continue;
            }
            if((curr_vertex_type & TYPE_PHI) != 0)
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if((curr_vertex_type & (TYPE_SWITCH | TYPE_RET | TYPE_VPHI | TYPE_LAST_OP | TYPE_LABEL)) != 0)
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if((curr_vertex_type & (TYPE_STORE)) != 0)
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if((curr_vertex_type & TYPE_EXTERNAL) && (curr_vertex_type & TYPE_RW))
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if(((curr_vertex_type & TYPE_LOAD) != 0))
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if(!allocation_information->is_operation_bounded(op_graph, loop_operation,
                                                             allocation_information->GetFuType(loop_operation)))
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if(loop_operation == filtered_op_graph->CGetOpGraphInfo()->entry_vertex)
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if(loop_operation == filtered_op_graph->CGetOpGraphInfo()->exit_vertex)
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if(RW_stmts.count(loop_operation))
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Type is ok");

            /// Computing bb barrier starting from bb barrier of predecessor
            InEdgeIterator ie, ie_end;
            for(boost::tie(ie, ie_end) = boost::in_edges(loop_operation, *filtered_op_graph); ie != ie_end; ie++)
            {
               const auto source = boost::source(*ie, *filtered_op_graph);
               if(bb_barrier.count(source))
               {
                  for(const auto pred_bb_barrier : bb_barrier.at(source))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Inserting BB" +
                                        STR(basic_block_graph->CGetBBNodeInfo(pred_bb_barrier)->block->number) +
                                        " because of " + GET_NAME(filtered_op_graph, source));
                     bb_barrier[loop_operation].insert(pred_bb_barrier);
                  }
               }
            }
            if(bb_barrier.count(loop_operation) && bb_barrier.at(loop_operation).count(loop_bb))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Cannot be moved because depends from a phi in the same bb");
               continue;
            }

            const auto operation_step = HLS->Rsch->get_cstep(loop_operation).second +
                                        (allocation_information->GetCycleLatency(loop_operation) - 1u);
            const auto operation_bb = basic_block_graph->CGetBBGraphInfo()->bb_index_map.at(
                filtered_op_graph->CGetOpNodeInfo(loop_operation)->bb_index);
            auto current_bb_dominator = operation_bb;
            THROW_ASSERT(boost::in_degree(current_bb_dominator, *dominators) == 1,
                         "Dominator is not a tree or entry was reached");
            boost::tie(ie, ie_end) = boost::in_edges(current_bb_dominator, *dominators);
            auto candidate_bb = boost::source(*ie, *dominators);
            while(true)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Checking if it can be remove in BB" +
                                  STR(basic_block_graph->CGetBBNodeInfo(candidate_bb)->block->number));
               if(candidate_bb == basic_block_graph->CGetBBGraphInfo()->entry_vertex ||
                  loop_bbs.count(candidate_bb) == 0)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "No because it is in other loop");
                  break;
               }
               bool overlapping = false;
               for(const auto dominator_op : basic_block_graph->CGetBBNodeInfo(candidate_bb)->statements_list)
               {
                  if((HLS->Rsch->get_cstep(dominator_op).second +
                      (allocation_information->GetCycleLatency(dominator_op) - 1u)) >= operation_step)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---" + GET_NAME(filtered_op_graph, dominator_op) + " ends at " +
                                        STR(HLS->Rsch->get_cstep(dominator_op).second +
                                            (allocation_information->GetCycleLatency(dominator_op) - 1u)) +
                                        " - " + GET_NAME(filtered_op_graph, loop_operation) + " ends at " +
                                        STR(operation_step));
                     overlapping = true;
                     break;
                  }
               }
               if(not overlapping)
               {
                  break;
               }
               /// Update dominator
               current_bb_dominator = candidate_bb;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Updating dominator to BB" +
                                  STR(basic_block_graph->CGetBBNodeInfo(current_bb_dominator)->block->number));
               THROW_ASSERT(boost::in_degree(current_bb_dominator, *dominators) == 1,
                            "Dominator is not a tree or entry was reached");
               boost::tie(ie, ie_end) = boost::in_edges(current_bb_dominator, *dominators);
               candidate_bb = boost::source(*ie, *dominators);
               /// If the current is in the barrier, do not check for the candidate
               if(bb_barrier.count(loop_operation) && bb_barrier.at(loop_operation).count(current_bb_dominator))
               {
                  break;
               }
            }
            if(current_bb_dominator != operation_bb)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Operation " + GET_NAME(filtered_op_graph, loop_operation) + " has to be moved in BB" +
                                  STR(basic_block_graph->CGetBBNodeInfo(current_bb_dominator)->block->number));
               std::vector<unsigned int> movement;
               movement.push_back(op_graph->CGetOpNodeInfo(loop_operation)->GetNodeId());
               movement.push_back(op_graph->CGetOpNodeInfo(loop_operation)->bb_index);
               movement.push_back(basic_block_graph->CGetBBNodeInfo(current_bb_dominator)->block->number);
               movements_list.push_back(movement);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updating execution times");
      std::list<vertex> sorted_vertices;
      filtered_op_graph->TopologicalSort(sorted_vertices);
      for(const auto sorted_vertex : sorted_vertices)
      {
         HLS->Rsch->UpdateTime(filtered_op_graph->CGetOpNodeInfo(sorted_vertex)->GetNodeId(), false);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated execution time");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked which operations have to be moved");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Scheduled loop " + STR(loop->GetId()));
   }
   HLS->Rsch->set_csteps(initial_ctrl_step);
   return DesignFlowStep_Status::SUCCESS;
}

void SDCScheduling2::Initialize()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Initializing SDCScheduling2");
   schedulingBaseStep::Initialize();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Initialized SDCScheduling2");
}
