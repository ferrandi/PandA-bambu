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
 *              Copyright (C) 2014-2020 Politecnico di Milano
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
 * @brief Implementation of the sdc scheduling
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

/// Header include
#include "sdc_scheduling.hpp"

///. include
#include "Parameter.hpp"

/// algorithms/loops_detection includes
#include "loop.hpp"
#include "loops.hpp"

/// behavior include
#include "basic_block.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"

/// boost include
#include <boost/range/adaptor/reversed.hpp>

/// design_flow includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_analysis includes
#include "frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_frontend_flow_step.hpp"

/// frontend_analysis/IR_analysis include
#include "simple_code_motion.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_constraints.hpp"

/// HLS/binding/module_binding
#include "fu_binding.hpp"

/// HLS/memory include
#include "memory.hpp"

/// HLS/module_allocation includes
#include "allocation.hpp"
#include "allocation_information.hpp"

/// HLS/scheduling include
#include "ASLAP.hpp"
#include "schedule.hpp"

/// ilp include
#include "meilp_solver.hpp"

/// STD include
#include <list>

/// tree include
#include "behavioral_helper.hpp"
#include "tree_basic_block.hpp"

/// utility include
#include "cpu_time.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

CONSTREF_FORWARD_DECL(Schedule);

/**
 * Class used to sort operation using ALAP in ascending order as primary key and ASAP ascending order as secondary key
 */
class SDCSorter : std::binary_function<vertex, vertex, bool>
{
 private:
   /// ASAP
   const ScheduleConstRef asap;

   /// ALAP
   const ScheduleConstRef alap;

   /// The function behavior
   const FunctionBehaviorConstRef function_behavior;

   /// The basic block graph
   const BBGraphConstRef basic_block_graph;

   /// The operation graph
   const OpGraphConstRef op_graph;

   /// The reachability map built on the basis of dependencies, consolidated choices and current choice
   CustomMap<vertex, CustomSet<vertex>> reachability_map;

   /// The index basic block map
   const CustomUnorderedMap<unsigned int, vertex>& bb_index_map;

   /// For each operation its level
   CustomMap<vertex, size_t> op_levels;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   const int debug_level;

 public:
   /**
    * Constructor
    * @param _asap is the asap information
    * @param _alap is the alap information
    * @param _function_behavior is the function behavior
    * @param _op_graph is the operation graph
    * @param _statements_list is the list of the statements of the basic block
    * @param _parameters is the set of input parameters
    */
   SDCSorter(const ScheduleConstRef _asap, const ScheduleConstRef _alap, const FunctionBehaviorConstRef _function_behavior, const OpGraphConstRef _op_graph, std::set<vertex, bb_vertex_order_by_map> loop_bbs, const ParameterConstRef _parameters)
       : asap(_asap),
         alap(_alap),
         function_behavior(_function_behavior),
         basic_block_graph(_function_behavior->CGetBBGraph(FunctionBehavior::BB)),
         op_graph(_op_graph),
         bb_index_map(basic_block_graph->CGetBBGraphInfo()->bb_index_map),
         parameters(_parameters),
         debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->SDC sorter constructor");

      for(const auto loop_bb : loop_bbs)
      {
         const auto& statements_list = basic_block_graph->CGetBBNodeInfo(loop_bb)->statements_list;
         /// The position in the basic block
         for(const auto vertex_to_be_analyzed : boost::adaptors::reverse(statements_list))
         {
            OutEdgeIterator eo, eo_end;
            for(boost::tie(eo, eo_end) = boost::out_edges(vertex_to_be_analyzed, *op_graph); eo != eo_end; eo++)
            {
               vertex target = boost::target(*eo, *op_graph);
               reachability_map[vertex_to_be_analyzed].insert(target);
               reachability_map[vertex_to_be_analyzed].insert(reachability_map[target].begin(), reachability_map[target].end());
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Computed reachability");

         /// We cluster candidate operations according to alap and asap - must be maps since we exploit order
         std::map<ControlStep, std::map<ControlStep, CustomSet<vertex>>> alap_asap_cluster;

         VertexIterator op, op_end;
         for(boost::tie(op, op_end) = boost::vertices(*op_graph); op != op_end; op++)
         {
            alap_asap_cluster[alap->get_cstep(*op).second][asap->get_cstep(*op).second].insert(*op);
         }

         /// For each cluster
         for(const auto& alap_cluster : alap_asap_cluster)
         {
            for(const auto& asap_cluster : alap_cluster.second)
            {
               auto to_process = std::set<vertex, op_vertex_order_by_map>(op_vertex_order_by_map(function_behavior->get_map_levels(), op_graph.get()));
               for(const auto cluster_op : asap_cluster.second)
               {
                  to_process.insert(cluster_op);
               }
               for(const auto cluster_op : to_process)
               {
                  op_levels[cluster_op] = static_cast<size_t>(op_levels.size());
               }
            }
         }
      }
   }

   /**
    * Compare position of two vertices
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if x precedes y in topological sort, false otherwise
    */
   bool operator()(const vertex x, const vertex y) const
   {
      const unsigned int first_bb_index = op_graph->CGetOpNodeInfo(x)->bb_index;
      const unsigned int second_bb_index = op_graph->CGetOpNodeInfo(y)->bb_index;
      const vertex first_bb_vertex = bb_index_map.find(first_bb_index)->second;
      const vertex second_bb_vertex = bb_index_map.find(second_bb_index)->second;
      if(function_behavior->CheckBBReachability(first_bb_vertex, second_bb_vertex))
      {
         return true;
      }
      if(function_behavior->CheckBBReachability(second_bb_vertex, first_bb_vertex))
      {
         return false;
      }
      THROW_ASSERT(op_levels.find(x) != op_levels.end(), "");
      THROW_ASSERT(op_levels.find(y) != op_levels.end(), "");
      return op_levels.find(x)->second < op_levels.find(y)->second;
   }
};

SDCScheduling::SDCScheduling(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : Scheduling(_parameters, _HLSMgr, _function_id, _design_flow_manager, HLSFlowStep_Type::SDC_SCHEDULING, _hls_flow_step_specialization), clock_period(0.0), margin(0.0)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

SDCScheduling::~SDCScheduling() = default;

void SDCScheduling::AddDelayConstraints(const meilp_solverRef solver, const OpGraphConstRef filtered_op_graph,
#ifndef NDEBUG
                                        const OpGraphConstRef debug_filtered_op_graph,
#endif
                                        const std::set<vertex, bb_vertex_order_by_map>& loop_bbs)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding delay constraints");
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);

   /// The asap starting and ending time
   CustomMap<vertex, double> starting_times, ending_times;

   /// Reverse reachability
   OpVertexMap<OpVertexSet> reverse_reachability(filtered_op_graph);

   /// For each vertex the set of "live" operations; an operation is live if its distance is less than the clock period
   OpVertexMap<OpVertexSet> live_operations(filtered_op_graph);

   /// For each vertex the operation for which a constraint has to be added
   OpVertexMap<OpVertexSet> constraints(filtered_op_graph);

   std::list<vertex> basic_blocks;
   basic_block_graph->TopologicalSort(basic_blocks);

   for(const auto basic_block : basic_blocks)
   {
      if(loop_bbs.find(basic_block) != loop_bbs.end())
      {
         const auto bb_node_info = basic_block_graph->CGetBBNodeInfo(basic_block);
         const auto& statements_list = bb_node_info->statements_list;
         for(const auto current : statements_list)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Processing " + GET_NAME(filtered_op_graph, current));
            /// The execution time of the current operation - for the current operation we must consider last stage
            /// NOTE:: Chaining after unbound operation is not allowed
            const double current_op_execution_time = [&]() -> double {
               if(not allocation_information->is_operation_bounded(filtered_op_graph, current, allocation_information->GetFuType(current)))
                  return clock_period;
               if(GET_TYPE(filtered_op_graph, current) & TYPE_PHI)
                  return allocation_information->GetCondExprTimeLatency(filtered_op_graph->CGetOpNodeInfo(current)->GetNodeId());
               auto timeLatency = allocation_information->GetTimeLatency(current, fu_binding::UNKNOWN, allocation_information->GetCycleLatency(current) - 1);
               /// Stage period of first cycle of operations with registered inputs is 0
               return allocation_information->get_initiation_time(allocation_information->GetFuType(current), current) > 0 ?
                          (allocation_information->is_operation_PI_registered(filtered_op_graph, current, allocation_information->GetFuType(current)) ? 0.0 : timeLatency.second) :
                          timeLatency.first;
            }();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Its execution time is " + STR(current_op_execution_time));
            /// Computing starting time
            starting_times[current] = 0.0;
            InEdgeIterator ie, ie_end;
            reverse_reachability.insert(std::pair<vertex, OpVertexSet>(current, OpVertexSet(filtered_op_graph)));
            live_operations.insert(std::pair<vertex, OpVertexSet>(current, OpVertexSet(filtered_op_graph)));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing starting time");
            for(boost::tie(ie, ie_end) = boost::in_edges(current, *filtered_op_graph); ie != ie_end; ie++)
            {
               if(((filtered_op_graph->GetSelector(*ie) & ~CDG_SELECTOR) == 0) and behavioral_helper->CanBeSpeculated(filtered_op_graph->CGetOpNodeInfo(boost::target(*ie, *filtered_op_graph))->GetNodeId()))
                  continue;
               const vertex source = boost::source(*ie, *filtered_op_graph);
               const auto connection_time = allocation_information->GetConnectionTime(source, current, AbsControlStep(bb_node_info->block->number, AbsControlStep::UNKNOWN));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering " + GET_NAME(filtered_op_graph, source));
               if(ending_times.find(source) != ending_times.end() and ending_times.find(source)->second + connection_time > starting_times.find(current)->second)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New starting time is " + STR(ending_times.find(source)->second + connection_time));
                  starting_times[current] = ending_times.find(source)->second + connection_time;
               }
               reverse_reachability.find(current)->second.insert(reverse_reachability.find(source)->second.begin(), reverse_reachability.find(source)->second.end());
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Starting time is " + STR(starting_times.find(current)->second));
            ending_times[current] = starting_times.find(current)->second + current_op_execution_time;

            OpVertexSet dead_operations(filtered_op_graph);

            for(boost::tie(ie, ie_end) = boost::in_edges(current, *filtered_op_graph); ie != ie_end; ie++)
            {
               if(((filtered_op_graph->GetSelector(*ie) & ~CDG_SELECTOR) == 0) and behavioral_helper->CanBeSpeculated(filtered_op_graph->CGetOpNodeInfo(boost::target(*ie, *filtered_op_graph))->GetNodeId()))
                  continue;
               const vertex source = boost::source(*ie, *filtered_op_graph);
               if(dead_operations.find(source) != dead_operations.end())
               {
                  continue;
               }
               auto CheckChaining = [&](const vertex other) -> bool {
                  bool constraint_to_be_added = false;
                  /// Operations cannot be chained if the chain is longer than clock period
                  if(ending_times.find(current)->second - starting_times.find(other)->second > clock_period - margin)
                  {
                     constraint_to_be_added = true;
                  }
                  else if(not HLS->allocation_information->CanBeChained(other, current))
                  {
                     constraint_to_be_added = true;
                  }
                  if(constraint_to_be_added)
                  {
                     bool skip = false;
                     InEdgeIterator ie2, ie2_end;
                     for(boost::tie(ie2, ie2_end) = boost::in_edges(current, *filtered_op_graph); ie2 != ie2_end and not skip; ie2++)
                     {
                        const vertex other_source = boost::source(*ie2, *filtered_op_graph);
                        if(other_source == other)
                        {
                           continue;
                        }
                        if(reverse_reachability.find(other_source)->second.find(other) == reverse_reachability.find(other)->second.end())
                        {
                           continue;
                        }
                        if(((GET_TYPE(op_graph, other) & TYPE_STORE) != 0) or (not allocation_information->is_operation_bounded(filtered_op_graph, other, allocation_information->GetFuType(other))))
                        {
                           skip = true;
                           continue;
                        }
                        if(ending_times.find(other_source)->second - starting_times.find(other)->second < clock_period)
                        {
                           continue;
                        }
                        /// Another predecessor (source2) has already a constraint with (other), so we can skip this constraint
                        skip = true;
                     }
                     if(not skip)
                     {
                        if(constraints.find(other) == constraints.end())
                        {
                           constraints.insert(std::pair<vertex, OpVertexSet>(other, OpVertexSet(filtered_op_graph)));
                        }
                        constraints.find(other)->second.insert(current);
                     }
                     dead_operations.insert(other);
                  }
                  else
                  {
                     live_operations.find(current)->second.insert(other);
                  }
                  return constraint_to_be_added;
               };
               const bool added_constraint = CheckChaining(source);
               if(not added_constraint)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering operations coming from " + GET_NAME(filtered_op_graph, source));
                  for(const auto pred_live_operation : live_operations.find(source)->second)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering operation " + GET_NAME(filtered_op_graph, pred_live_operation));
                     CheckChaining(pred_live_operation);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered operation " + GET_NAME(filtered_op_graph, pred_live_operation));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered operations coming from " + GET_NAME(filtered_op_graph, source));
               }
            }
            for(const auto dead_operation : dead_operations)
            {
               if(live_operations.find(current)->second.find(dead_operation) != live_operations.find(current)->second.end())
               {
                  live_operations.find(current)->second.erase(live_operations.find(current)->second.find(dead_operation));
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Processed " + GET_NAME(filtered_op_graph, current));
         }
      }
   }
   for(const auto& constraint : constraints)
   {
      for(const auto second : constraint.second)
      {
         const unsigned int source_cycle_latency = allocation_information->GetCycleLatency(constraint.first);
         const std::string name = "Path_" + GET_NAME(op_graph, constraint.first) + "-" + GET_NAME(op_graph, second);
         std::map<int, double> coeffs;
         coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(constraint.first, source_cycle_latency - 1))->second)] = 1.0;
         coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(second, 0))->second)] = -1.0;
         solver->add_row(coeffs, -1.0, meilp_solver::L, name);
#ifndef NDEBUG
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            if(not op_graph->ExistsEdge(constraint.first, second))
            {
               temp_edges.insert(FB->ogc->AddEdge(constraint.first, second, DEBUG_SELECTOR));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge " + GET_NAME(filtered_op_graph, constraint.first) + "-->" + GET_NAME(filtered_op_graph, second));
               try
               {
                  std::list<vertex> vertices;
                  debug_filtered_op_graph->TopologicalSort(vertices);
               }
               catch(const char* msg)
               {
                  debug_filtered_op_graph->WriteDot("Error.dot");
                  THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, constraint.first) + "-->" + GET_NAME(op_graph, second));
               }
               catch(const std::string& msg)
               {
                  debug_filtered_op_graph->WriteDot("Error.dot");
                  THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, constraint.first) + "-->" + GET_NAME(op_graph, second));
               }
               catch(const std::exception& ex)
               {
                  debug_filtered_op_graph->WriteDot("Error.dot");
                  THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, constraint.first) + "-->" + GET_NAME(op_graph, second));
               }
               catch(...)
               {
                  debug_filtered_op_graph->WriteDot("Error.dot");
                  THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, constraint.first) + "-->" + GET_NAME(op_graph, second));
               }
            }
         }
#endif
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added delay constraints");
}

void SDCScheduling::AddDependenceConstraint(const meilp_solverRef solver, const vertex source, const vertex target, bool simultaneous) const
{
   const unsigned int source_cycle_latency = allocation_information->GetCycleLatency(source);
   const std::string name = GET_NAME(op_graph, source) + "-" + GET_NAME(op_graph, target);
   std::map<int, double> coeffs;
   coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(source, source_cycle_latency - 1))->second)] = 1.0;
   coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(target, 0))->second)] = -1.0;
   solver->add_row(coeffs, simultaneous ? 0.0 : -1.0, meilp_solver::L, name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added dependence constraint " + name);
}

void SDCScheduling::AddStageConstraints(const meilp_solverRef solver, const vertex operation) const
{
   const unsigned int cycle_latency = allocation_information->GetCycleLatency(operation);
   for(unsigned int stage = 1; stage < cycle_latency; stage++)
   {
      const std::string name = "Stage_" + GET_NAME(op_graph, operation) + "_" + STR(stage - 1) + "-" + STR(stage);
      std::map<int, double> coeffs;
      coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, stage))->second)] = 1.0;
      coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, stage - 1))->second)] = -1.0;
      solver->add_row(coeffs, 1.0, meilp_solver::E, name);
   }
}

void SDCScheduling::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   if(GetStatus() == DesignFlowStep_Status::SUCCESS)
   {
      if(relationship_type == INVALIDATION_RELATIONSHIP)
      {
         {
            vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::SDC_CODE_MOTION, funId));
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step = frontend_step != NULL_VERTEX ?
                                                           design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                                                           GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))->CreateFunctionFrontendFlowStep(FrontendFlowStepType::SDC_CODE_MOTION, funId);
            relationship.insert(design_flow_step);
         }
         if(not parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE, funId));
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step = frontend_step != NULL_VERTEX ?
                                                           design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                                                           GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))->CreateFunctionFrontendFlowStep(FrontendFlowStepType::BIT_VALUE, funId);
            relationship.insert(design_flow_step);
         }
      }
   }
   Scheduling::ComputeRelationships(relationship, relationship_type);
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> SDCScheduling::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret = Scheduling::ComputeHLSRelationships(relationship_type);
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
         if(parameters->getOption<bool>(OPT_parse_pragma))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::OMP_ALLOCATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::CALLED_FUNCTIONS));
         }
         else
#endif
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::ALLOCATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

bool SDCScheduling::HasToBeExecuted() const
{
   if(bb_version == 0)
      return Scheduling::HasToBeExecuted();
   else
      return false;
}

DesignFlowStep_Status SDCScheduling::InternalExec()
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BBGraphConstRef dominators = FB->CGetBBGraph(FunctionBehavior::DOM_TREE);
   const LoopsConstRef loops = FB->CGetLoops();
   const std::map<vertex, unsigned int>& bb_map_levels = FB->get_bb_map_levels();
   ControlStep initial_ctrl_step = ControlStep(0u);
   for(const auto& loop : loops->GetList())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Scheduling loop " + STR(loop->GetId()));
      operation_to_varindex.clear();
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
            loop_operations.insert(basic_block_graph->CGetBBNodeInfo(*bb)->statements_list.begin(), basic_block_graph->CGetBBNodeInfo(*bb)->statements_list.end());
         }
      }
      /// FIXME: for the moment the considered graph contains control dependence edges
      const OpGraphConstRef filtered_op_graph = FB->CGetOpGraph(FunctionBehavior::FLSAODG, loop_operations);
#ifndef NDEBUG
      const OpGraphConstRef debug_filtered_op_graph = FB->CGetOpGraph(FunctionBehavior::FLSAODDG, loop_operations);
#endif
      /// Create the solver
      meilp_solverRef solver(meilp_solver::create_solver(static_cast<meilp_solver::supported_solvers>(parameters->getOption<int>(OPT_ilp_solver))));
      if(parameters->getOption<int>(OPT_ilp_max_time))
         solver->setMaximumSeconds(parameters->getOption<int>(OPT_ilp_max_time));

      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
      {
         filtered_op_graph->WriteDot("Loop_" + STR(loop_id) + "_to_be_scheduled.dot");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing variables");
      /// Compute variables
      /// Map real operation-stage to variable index
      unsigned int next_var_index = 0;
      for(const auto loop_operation : loop_operations)
      {
         const unsigned int cycle_latency = allocation_information->GetCycleLatency(loop_operation);
         for(unsigned int stage = 0; stage < cycle_latency; stage++)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Creating variables for " + GET_NAME(filtered_op_graph, loop_operation) + " (executed by " + " " + allocation_information->get_fu_name(allocation_information->GetFuType(loop_operation)).first + ")  stage " + STR(stage) +
                               ": " + STR(next_var_index + 1));
            operation_to_varindex[std::pair<vertex, unsigned int>(loop_operation, stage)] = next_var_index;
            next_var_index++;
         }
      }

      /// Create the variables representing the ending of a path
      /// Map tree leaf to variable index
      CustomMap<vertex, unsigned int> path_end_to_varindex;
      for(const auto basic_block : loop_bbs)
      {
         OutEdgeIterator oe, oe_end;
         for(boost::tie(oe, oe_end) = boost::out_edges(basic_block, *basic_block_graph); oe != oe_end; oe++)
         {
            const auto target = boost::target(*oe, *basic_block_graph);
            if(boost::in_degree(target, *basic_block_graph) < 2)
               continue;
            path_end_to_varindex[basic_block] = next_var_index;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Creating variable for path ending in BB" + STR(basic_block_graph->CGetBBNodeInfo(basic_block)->block->number));
            next_var_index++;
         }
      }
#if 0
      CustomMap<vertex, unsigned> bb_to_begin, bb_to_end;
      ///Create the variables representing the beginning and the ending of a bb
      if(not speculation)
      {
         for(const auto basic_block : loop_bbs)
         {
            bb_to_begin[basic_block] = next_var_index;
            next_var_index++;
            bb_to_end[basic_block] = next_var_index;
            next_var_index++;
         }
      }
#endif
      /// next_var_index has been incremented but the + 1 is the makespan
      const size_t num_variables = next_var_index;

      solver->make(static_cast<int>(num_variables));

      /// Set integer variables and lower bound
      for(size_t var_index = 0; var_index < num_variables; var_index++)
      {
         solver->set_int(static_cast<int>(var_index));
         solver->set_lowbo(static_cast<int>(var_index), 0);
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed " + STR(num_variables) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding consecutive stages constraints");

      /// Add consecutive stages constraints
      for(auto const operation : loop_operations)
      {
         AddStageConstraints(solver, operation);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added consecutive stages constraints");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding dependencies constraints");

      /// Add dependence constraints: target can start in the same clock cycle in which source ends
      for(const auto operation : loop_operations)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding dependencies starting from " + GET_NAME(filtered_op_graph, operation));
         OutEdgeIterator oe, oe_end;
         for(boost::tie(oe, oe_end) = boost::out_edges(operation, *filtered_op_graph); oe != oe_end; oe++)
         {
            /// Not control dependence
            if(filtered_op_graph->GetSelector(*oe) & ~CDG_SELECTOR)
            {
               AddDependenceConstraint(solver, boost::source(*oe, *filtered_op_graph), boost::target(*oe, *filtered_op_graph), true);
            }
            /// Non speculable operation
            else if(not behavioral_helper->CanBeSpeculated(filtered_op_graph->CGetOpNodeInfo(boost::target(*oe, *filtered_op_graph))->GetNodeId()))
            {
               AddDependenceConstraint(solver, boost::source(*oe, *filtered_op_graph), boost::target(*oe, *filtered_op_graph), false);
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped dependence -> " + GET_NAME(filtered_op_graph, boost::target(*oe, *filtered_op_graph)));
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added dependencies starting from " + GET_NAME(filtered_op_graph, operation));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added dependencies constraints");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding delay constraint");
      /// Add timing constraints: target can start in the same clock cycle in which source ends only with chaining
      AddDelayConstraints(solver, filtered_op_graph,
#ifndef NDEBUG
                          debug_filtered_op_graph,
#endif
                          loop_bbs);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added delay constraint");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding sorting constraint");
      const ASLAPRef aslap(new ASLAP(HLSMgr, HLS, true, loop_operations, parameters, 1000));
      aslap->compute_ASAP();
      aslap->compute_ALAP(ASLAP::ALAP_fast);
      SDCSorter sdc_sorter = SDCSorter(aslap->CGetASAP(), aslap->CGetALAP(), FB, filtered_op_graph, loop_bbs, parameters);

      /// For each basic block, the set of unbounded operations found on the paths to it
      CustomMap<vertex, OpVertexSet> loop_unbounded_operations;

      /// For each basic block, the set of pipelined operations found on the paths to it
      CustomMap<vertex, OpVertexSet> loop_pipelined_operations;

      /// For each basic block, for each functional unit fu, the list of the last n operations executed (n is the number of resource of type fu) - Value is a set since there can be different paths reaching current basic block
      CustomMap<vertex, CustomMap<unsigned int, CustomOrderedSet<std::list<vertex>>>> constrained_operations_sequences;
      for(const auto basic_block : loop_bbs)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding sorting constraints for BB" + STR(basic_block_graph->CGetBBNodeInfo(basic_block)->block->number));
         loop_unbounded_operations.insert(std::pair<vertex, OpVertexSet>(basic_block, OpVertexSet(filtered_op_graph)));
         loop_pipelined_operations.insert(std::pair<vertex, OpVertexSet>(basic_block, OpVertexSet(filtered_op_graph)));
         InEdgeIterator ie, ie_end;
         for(boost::tie(ie, ie_end) = boost::in_edges(basic_block, *basic_block_graph); ie != ie_end; ie++)
         {
            const auto source = boost::source(*ie, *basic_block_graph);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering source BB" + STR(basic_block_graph->CGetBBNodeInfo(source)->block->number));
            if(loop_bbs.find(source) != loop_bbs.end())
            {
               loop_unbounded_operations.at(basic_block).insert(loop_unbounded_operations.find(source)->second.begin(), loop_unbounded_operations.find(source)->second.end());
               loop_pipelined_operations.at(basic_block).insert(loop_pipelined_operations.find(source)->second.begin(), loop_pipelined_operations.find(source)->second.end());
               for(const auto& fu_type : constrained_operations_sequences[source])
               {
                  constrained_operations_sequences[basic_block][fu_type.first].insert(fu_type.second.begin(), fu_type.second.end());
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         std::set<vertex, SDCSorter> basic_block_sorted_operations(sdc_sorter);
         for(const auto operation : basic_block_graph->CGetBBNodeInfo(basic_block)->statements_list)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Inserting operation " + GET_NAME(filtered_op_graph, operation));
            basic_block_sorted_operations.insert(operation);
#ifndef NDEBUG
            for(const auto debug_operation : basic_block_sorted_operations)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(filtered_op_graph, debug_operation));
            }
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Inserted operation " + GET_NAME(filtered_op_graph, operation));
         }
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Ordered statements");
#ifndef NDEBUG
            for(const auto debug_operation : basic_block_sorted_operations)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(filtered_op_graph, debug_operation));
            }
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         for(const auto operation : basic_block_sorted_operations)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering " + GET_NAME(filtered_op_graph, operation));
            if(not allocation_information->is_operation_bounded(op_graph, operation, allocation_information->GetFuType(operation)))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding constraints for unbounded operations");
               for(const auto other_unbounded_operation : loop_unbounded_operations.at(basic_block))
               {
                  std::map<int, double> coeffs;
                  const bool other_before = sdc_sorter(other_unbounded_operation, operation);
                  if(other_before)
                  {
                     const std::string name = "UU1_" + GET_NAME(op_graph, other_unbounded_operation) + "_" + GET_NAME(op_graph, operation);
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, 0))->second)] = 1.0;
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(other_unbounded_operation, 0))->second)] = -1.0;
                     solver->add_row(coeffs, 1.0, meilp_solver::G, name);
#ifndef NDEBUG
                     if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                     {
                        if(not op_graph->ExistsEdge(other_unbounded_operation, operation))
                        {
                           temp_edges.insert(FB->ogc->AddEdge(other_unbounded_operation, operation, DEBUG_SELECTOR));
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge " + GET_NAME(filtered_op_graph, other_unbounded_operation) + "-->" + GET_NAME(filtered_op_graph, operation));
                           try
                           {
                              std::list<vertex> vertices;
                              debug_filtered_op_graph->TopologicalSort(vertices);
                           }
                           catch(const char* msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, other_unbounded_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                           catch(const std::string& msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, other_unbounded_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                           catch(const std::exception& ex)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, other_unbounded_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                           catch(...)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, other_unbounded_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                        }
                     }
#endif
                  }
                  else
                  {
                     /// NOTE: unbounded operation can start during last stage of multi cycle operation
                     const std::string name = "UU2_" + GET_NAME(op_graph, operation) + "_" + GET_NAME(op_graph, other_unbounded_operation);
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(other_unbounded_operation, 0))->second)] = 1.0;
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, 0))->second)] = -1.0;
                     solver->add_row(coeffs, 0.0, meilp_solver::G, name);
#ifndef NDEBUG
                     if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                     {
                        if(not op_graph->ExistsEdge(operation, other_unbounded_operation))
                        {
                           temp_edges.insert(FB->ogc->AddEdge(operation, other_unbounded_operation, DEBUG_SELECTOR));
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge " + GET_NAME(filtered_op_graph, operation) + "-->" + GET_NAME(filtered_op_graph, other_unbounded_operation));
                           try
                           {
                              std::list<vertex> vertices;
                              debug_filtered_op_graph->TopologicalSort(vertices);
                           }
                           catch(const char* msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, other_unbounded_operation));
                           }
                           catch(const std::string& msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, other_unbounded_operation));
                           }
                           catch(const std::exception& ex)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, other_unbounded_operation));
                           }
                           catch(...)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, other_unbounded_operation));
                           }
                        }
                     }
#endif
                  }
               }
               loop_unbounded_operations.at(basic_block).insert(operation);
               for(const auto loop_pipelined_operation : loop_pipelined_operations.at(basic_block))
               {
                  std::map<int, double> coeffs;
                  const bool pipelined_before = sdc_sorter(loop_pipelined_operation, operation);
                  if(pipelined_before)
                  {
                     const std::string name = "PU_" + GET_NAME(op_graph, loop_pipelined_operation) + "_" + GET_NAME(op_graph, operation);
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, 0))->second)] = 1.0;
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(loop_pipelined_operation, allocation_information->GetCycleLatency(loop_pipelined_operation) - 1))->second)] = -1.0;
                     solver->add_row(coeffs, 1.0, meilp_solver::G, name);
#ifndef NDEBUG
                     if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                     {
                        if(not op_graph->ExistsEdge(loop_pipelined_operation, operation))
                        {
                           temp_edges.insert(FB->ogc->AddEdge(loop_pipelined_operation, operation, DEBUG_SELECTOR));
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge " + GET_NAME(filtered_op_graph, loop_pipelined_operation) + "-->" + GET_NAME(filtered_op_graph, operation));
                           try
                           {
                              std::list<vertex> vertices;
                              debug_filtered_op_graph->TopologicalSort(vertices);
                           }
                           catch(const char* msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, loop_pipelined_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                           catch(const std::string& msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, loop_pipelined_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                           catch(const std::exception& ex)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, loop_pipelined_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                           catch(...)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, loop_pipelined_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                        }
                     }
#endif
                  }
                  else
                  {
                     /// NOTE: unbounded operation can start during last stage of multi cycle operation
                     const std::string name = "UP_" + GET_NAME(op_graph, operation) + "_" + GET_NAME(op_graph, loop_pipelined_operation);
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(loop_pipelined_operation, 0))->second)] = 1.0;
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, 0))->second)] = -1.0;
                     solver->add_row(coeffs, 0.0, meilp_solver::G, name);
#ifndef NDEBUG
                     if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                     {
                        if(not op_graph->ExistsEdge(operation, loop_pipelined_operation))
                        {
                           temp_edges.insert(FB->ogc->AddEdge(operation, loop_pipelined_operation, DEBUG_SELECTOR));
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge " + GET_NAME(filtered_op_graph, operation) + "-->" + GET_NAME(filtered_op_graph, loop_pipelined_operation));
                           try
                           {
                              std::list<vertex> vertices;
                              debug_filtered_op_graph->TopologicalSort(vertices);
                           }
                           catch(const char* msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, loop_pipelined_operation));
                           }
                           catch(const std::string& msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, loop_pipelined_operation));
                           }
                           catch(const std::exception& ex)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, loop_pipelined_operation));
                           }
                           catch(...)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, loop_pipelined_operation));
                           }
                        }
                     }
#endif
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added constraints for unbounded operations");
            }
            if(allocation_information->GetCycleLatency(operation) > 1)
            {
               loop_pipelined_operations.at(basic_block).insert(operation);
               for(const auto loop_unbounded_operation : loop_unbounded_operations.at(basic_block))
               {
                  std::map<int, double> coeffs;
                  const bool unbounded_before = sdc_sorter(loop_unbounded_operation, operation);
                  if(unbounded_before)
                  {
                     const std::string name = "UM_" + GET_NAME(op_graph, loop_unbounded_operation) + "_" + GET_NAME(op_graph, operation);
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, 0))->second)] = 1.0;
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(loop_unbounded_operation, 0))->second)] = -1.0;
                     solver->add_row(coeffs, 1.0, meilp_solver::G, name);
#ifndef NDEBUG
                     if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                     {
                        if(not op_graph->ExistsEdge(loop_unbounded_operation, operation))
                        {
                           temp_edges.insert(FB->ogc->AddEdge(loop_unbounded_operation, operation, DEBUG_SELECTOR));
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge " + GET_NAME(filtered_op_graph, loop_unbounded_operation) + "-->" + GET_NAME(filtered_op_graph, operation));
                           try
                           {
                              std::list<vertex> vertices;
                              debug_filtered_op_graph->TopologicalSort(vertices);
                           }
                           catch(const char* msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, loop_unbounded_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                           catch(const std::string& msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, loop_unbounded_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                           catch(const std::exception& ex)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, loop_unbounded_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                           catch(...)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, loop_unbounded_operation) + "-->" + GET_NAME(op_graph, operation));
                           }
                        }
                     }
#endif
                  }
                  else
                  {
                     /// NOTE: unbounded operation can start during last stage of multi cycle operation
                     const std::string name = "MU_" + GET_NAME(op_graph, operation) + "_" + GET_NAME(op_graph, loop_unbounded_operation);
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(loop_unbounded_operation, 0))->second)] = 1.0;
                     coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, allocation_information->GetCycleLatency(operation) - 1))->second)] = -1.0;
                     solver->add_row(coeffs, 0.0, meilp_solver::G, name);
#ifndef NDEBUG
                     if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                     {
                        if(not op_graph->ExistsEdge(operation, loop_unbounded_operation))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge " + GET_NAME(filtered_op_graph, operation) + "-->" + GET_NAME(filtered_op_graph, loop_unbounded_operation));
                           temp_edges.insert(FB->ogc->AddEdge(operation, loop_unbounded_operation, DEBUG_SELECTOR));
                           try
                           {
                              std::list<vertex> vertices;
                              debug_filtered_op_graph->TopologicalSort(vertices);
                           }
                           catch(const char* msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, loop_unbounded_operation));
                           }
                           catch(const std::string& msg)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, loop_unbounded_operation));
                           }
                           catch(const std::exception& ex)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, loop_unbounded_operation));
                           }
                           catch(...)
                           {
                              debug_filtered_op_graph->WriteDot("Error.dot");
                              THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, operation) + "-->" + GET_NAME(op_graph, loop_unbounded_operation));
                           }
                        }
                     }
#endif
                  }
               }
            }
            /// Resource constraints
            const auto fu_type = allocation_information->GetFuType(operation);
            const unsigned int resources_number = allocation_information->get_number_fu(fu_type);
            if(limited_resources.find(fu_type) != limited_resources.end())
            {
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
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering sequence " + old_sequence_string);
                     }
                     old_sequence.push_back(operation);
                     if(old_sequence.size() > resources_number)
                     {
                        const auto front = old_sequence.front();
                        old_sequence.pop_front();
                        const std::string name = allocation_information->get_fu_name(fu_type).first + "_" + GET_NAME(op_graph, front) + "_" + GET_NAME(op_graph, operation);
                        std::map<int, double> coeffs;
                        coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, 0))->second)] = 1.0;
                        coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(front, 0))->second)] = -1.0;
                        solver->add_row(coeffs, 1.0, meilp_solver::G, name);
#ifndef NDEBUG
                        if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                        {
                           if(not op_graph->ExistsEdge(front, operation))
                           {
                              temp_edges.insert(FB->ogc->AddEdge(front, operation, DEBUG_SELECTOR));
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge " + GET_NAME(filtered_op_graph, front) + "-->" + GET_NAME(filtered_op_graph, operation));
                              try
                              {
                                 std::list<vertex> vertices;
                                 debug_filtered_op_graph->TopologicalSort(vertices);
                              }
                              catch(const char* msg)
                              {
                                 debug_filtered_op_graph->WriteDot("Error.dot");
                                 THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, front) + "-->" + GET_NAME(op_graph, operation));
                              }
                              catch(const std::string& msg)
                              {
                                 debug_filtered_op_graph->WriteDot("Error.dot");
                                 THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, front) + "-->" + GET_NAME(op_graph, operation));
                              }
                              catch(const std::exception& ex)
                              {
                                 debug_filtered_op_graph->WriteDot("Error.dot");
                                 THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, front) + "-->" + GET_NAME(op_graph, operation));
                              }
                              catch(...)
                              {
                                 debug_filtered_op_graph->WriteDot("Error.dot");
                                 THROW_UNREACHABLE("Edge " + GET_NAME(op_graph, front) + "-->" + GET_NAME(op_graph, operation));
                              }
                           }
                        }
#endif
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
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered " + GET_NAME(filtered_op_graph, operation));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added sorting constraints for BB" + STR(basic_block_graph->CGetBBNodeInfo(basic_block)->block->number));
      }

      /// Adding last operation constraint: return must be executed after all the other operations.
      for(const auto operation : loop_operations)
      {
         if((GET_TYPE(filtered_op_graph, operation) & (TYPE_RET)) != 0)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding constraint for return " + GET_NAME(filtered_op_graph, operation));
            for(const auto other_operation : loop_operations)
            {
               if(operation == other_operation or not FB->CheckReachability(other_operation, operation))
                  continue;
               const std::string name = "last_" + GET_NAME(op_graph, other_operation) + "_" + GET_NAME(op_graph, operation);
               std::map<int, double> coeffs;
               coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, 0))->second)] = 1.0;
               coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(other_operation, allocation_information->GetCycleLatency(other_operation) - 1))->second)] = -1.0;
               /// last cannot be scheduleded with unbounded operations
               if(not allocation_information->is_operation_bounded(filtered_op_graph, other_operation, allocation_information->GetFuType(other_operation)))
               {
                  solver->add_row(coeffs, 1.0, meilp_solver::G, name);
               }
               else
               {
                  solver->add_row(coeffs, 0.0, meilp_solver::G, name);
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added constraint for return " + GET_NAME(filtered_op_graph, operation));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added sorting constraints");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Forcing path end");
      /// Setting path end to be the real end
      for(auto const path_end : path_end_to_varindex)
      {
         for(auto const operation : basic_block_graph->CGetBBNodeInfo(path_end.first)->statements_list)
         {
            const unsigned int source_cycle_latency = allocation_information->GetCycleLatency(operation);
            const std::string name = GET_NAME(op_graph, operation) + "_to_BB" + STR(basic_block_graph->CGetBBNodeInfo(path_end.first)->block->number) + "_end";
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding constraint " + name);
            std::map<int, double> coeffs;
            coeffs[static_cast<int>(path_end.second)] = 1.0;
            coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, source_cycle_latency - 1))->second)] = -1.0;
            solver->add_row(coeffs, 0.0, meilp_solver::G, name);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Forced path end");
#if 0
      if(not speculation)
      {
         ///Setting beginning of basic block
         for(const auto basic_block : loop_bbs)
         {
            for(const auto operation : basic_block_graph->CGetBBNodeInfo(basic_block)->statements_list)
            {
               const auto name = "BB" + STR(basic_block_graph->CGetBBNodeInfo(basic_block)->block->number) + "_Begin_" + GET_NAME(op_graph, operation);
               std::map<int, double> coeffs;
               coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, 0))->second)] = 1.0;
               coeffs[static_cast<int>(bb_to_begin.find(basic_block)->second)] = -1.0;
               solver->add_row(coeffs, 0.0, meilp_solver::G, name);
            }
            for(const auto operation : basic_block_graph->CGetBBNodeInfo(basic_block)->statements_list)
            {
               const unsigned int source_cycle_latency = allocation_information->GetCycleLatency(operation);
               const auto name = "BB" + STR(basic_block_graph->CGetBBNodeInfo(basic_block)->block->number) + "_End_" + GET_NAME(op_graph, operation);
               std::map<int, double> coeffs;
               coeffs[static_cast<int>(bb_to_end.find(basic_block)->second)] = 1.0;
               coeffs[static_cast<int>(operation_to_varindex.find(std::pair<vertex, unsigned int>(operation, source_cycle_latency-1))->second)] = -1.0;
               solver->add_row(coeffs, 0.0, meilp_solver::G, name);
            }
            OutEdgeIterator oe, oe_end;
            for(boost::tie(oe, oe_end) = boost::out_edges(basic_block, *basic_block_graph); oe != oe_end; oe++)
            {
               const auto target = boost::target(*oe, *basic_block_graph);
               const auto target_bb_node_info = basic_block_graph->CGetBBNodeInfo(target);
               if(target_bb_node_info->loop_id == loop_id)
               {
                  const auto name = "CFG_BB" + STR(basic_block_graph->CGetBBNodeInfo(basic_block)->block->number) + "_" + STR(target_bb_node_info->block->number);
                  std::map<int, double> coeffs;
                  coeffs[static_cast<int>(bb_to_begin.find(target)->second)] = 1.0;
                  coeffs[static_cast<int>(bb_to_end.find(basic_block)->second)] = -1.0;
                  solver->add_row(coeffs, 0.0, meilp_solver::G, name);
               }
            }
         }
      }
#endif
      /// Setting objective function
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Setting objective function");
      std::map<int, double> objective_coeffs;
      for(auto const& path_end : path_end_to_varindex)
      {
         objective_coeffs[static_cast<int>(path_end.second)] += 1.0;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      solver->objective_add(objective_coeffs, meilp_solver::min);
      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC and loop_operations.size() > 2)
         solver->print_to_file(parameters->getOption<std::string>(OPT_output_temporary_directory) + "/" + FB->CGetBehavioralHelper()->get_function_name() + "_SDC_formulation_Loop_" + STR(loop_id));
#ifndef NDEBUG
      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
      {
         filtered_op_graph->WriteDot("HLS_SDC_" + STR(loop_id) + ".dot");
      }
#endif
      int ilp_result = solver->solve_ilp();
      if(ilp_result != 0)
         THROW_ERROR("Error in finding ilp solution");

#ifndef NDEBUG
      for(auto const& temp_edge : temp_edges)
      {
         FB->ogc->RemoveSelector(temp_edge, DEBUG_SELECTOR);
      }
      temp_edges.clear();
#endif

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Solution:");
      std::map<int, double> vals;
      solver->get_vars_solution(vals);
      ControlStep last_relative_step = ControlStep(0u);
      for(const auto operation : loop_operations)
      {
         const unsigned int begin_variable = operation_to_varindex[std::pair<vertex, unsigned int>(operation, 0)];
         ControlStep current_control_step = ControlStep(static_cast<unsigned int>(vals[static_cast<int>(begin_variable)]));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(filtered_op_graph, operation) + " scheduled at relative step " + STR(current_control_step));
         HLS->Rsch->set_execution(operation, current_control_step + initial_ctrl_step);
         HLS->Rsch->set_execution_end(operation, current_control_step + initial_ctrl_step + allocation_information->GetCycleLatency(operation) - 1);
         const unsigned int cycle_latency = allocation_information->GetCycleLatency(operation);
         if(last_relative_step < current_control_step + cycle_latency)
            last_relative_step = current_control_step + cycle_latency;
         /// set the binding information
         if(HLS->HLS_C->has_binding_to_fu(GET_NAME(filtered_op_graph, operation)))
            res_binding->bind(operation, allocation_information->GetFuType(operation), 0);
         else
            res_binding->bind(operation, allocation_information->GetFuType(operation));
      }
      initial_ctrl_step = last_relative_step + 1u;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      /// Check which statements have to be moved
      /// For each statement the basic blocks above which it cannot be moved.
      /// Phi cannot be moved
      /// Operations which depend from the phi cannot be moved before the phi and so on
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking which operations have to be moved");
      CustomMap<vertex, CustomSet<vertex>> bb_barrier;
      for(const auto loop_bb : loop_bbs)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking if operations of BB" + STR(basic_block_graph->CGetBBNodeInfo(loop_bb)->block->number) + " can be moved");
         /// Set of operations which cannot be moved (at the moment) because of dependencies from gimple phi
         OpVertexSet blocked_ops = OpVertexSet(filtered_op_graph);

         for(const auto loop_operation : basic_block_graph->CGetBBNodeInfo(loop_bb)->statements_list)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking if " + GET_NAME(filtered_op_graph, loop_operation) + " has to be moved");
            if((GET_TYPE(filtered_op_graph, loop_operation) & (TYPE_IF | TYPE_MULTIIF)) != 0)
            {
               /// IF is not a barrier for controlled operations
               continue;
            }
            if((GET_TYPE(filtered_op_graph, loop_operation) & TYPE_PHI) != 0)
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if((GET_TYPE(filtered_op_graph, loop_operation) & (TYPE_SWITCH | TYPE_RET | TYPE_VPHI | TYPE_LAST_OP | TYPE_LABEL)) != 0)
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if((GET_TYPE(filtered_op_graph, loop_operation) & (TYPE_STORE)) != 0)
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if((GET_TYPE(filtered_op_graph, loop_operation) & (TYPE_EXTERNAL)) != 0)
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            /// Loads which do not have resource limitation can be moved but not speculated; speculation should be prevented by control edges
            if(((GET_TYPE(filtered_op_graph, loop_operation) & TYPE_LOAD) != 0) and limited_resources.find(allocation_information->GetFuType(loop_operation)) != limited_resources.end())
            {
               bb_barrier[loop_operation].insert(loop_bb);
               continue;
            }
            if(not allocation_information->is_operation_bounded(op_graph, loop_operation, allocation_information->GetFuType(loop_operation)))
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
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Type is ok");

            /// Computing bb barrier starting from bb barrier of predecesso
            InEdgeIterator ie, ie_end;
            for(boost::tie(ie, ie_end) = boost::in_edges(loop_operation, *filtered_op_graph); ie != ie_end; ie++)
            {
               const auto source = boost::source(*ie, *filtered_op_graph);
               if(bb_barrier.find(source) != bb_barrier.end())
               {
                  for(const auto pred_bb_barrier : bb_barrier.find(source)->second)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inserting BB" + STR(basic_block_graph->CGetBBNodeInfo(pred_bb_barrier)->block->number) + " because of " + GET_NAME(filtered_op_graph, source));
                     bb_barrier[loop_operation].insert(pred_bb_barrier);
                  }
               }
            }
            if(bb_barrier.find(loop_operation) != bb_barrier.end() and bb_barrier.find(loop_operation)->second.find(loop_bb) != bb_barrier.find(loop_operation)->second.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Cannot be moved becaused depends from a phi in the same bb");
               continue;
            }

            const auto operation_step = HLS->Rsch->get_cstep(loop_operation).second + (allocation_information->GetCycleLatency(loop_operation) - 1u);
            const vertex operation_bb = basic_block_graph->CGetBBGraphInfo()->bb_index_map.find(filtered_op_graph->CGetOpNodeInfo(loop_operation)->bb_index)->second;
            vertex current_bb_dominator = operation_bb;
            THROW_ASSERT(boost::in_degree(current_bb_dominator, *dominators) == 1, "Dominator is not a tree or entry was reached");
            boost::tie(ie, ie_end) = boost::in_edges(current_bb_dominator, *dominators);
            vertex candidate_bb = boost::source(*ie, *dominators);
            while(true)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking if it can be bemove in BB" + STR(basic_block_graph->CGetBBNodeInfo(candidate_bb)->block->number));
               if(candidate_bb == basic_block_graph->CGetBBGraphInfo()->entry_vertex or loop_bbs.find(candidate_bb) == loop_bbs.end())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "No because it is in other loop");
                  break;
               }
               bool overlapping = false;
               for(const auto dominator_op : basic_block_graph->CGetBBNodeInfo(candidate_bb)->statements_list)
               {
                  if((HLS->Rsch->get_cstep(dominator_op).second + (allocation_information->GetCycleLatency(dominator_op) - 1u)) >= operation_step)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---" + GET_NAME(filtered_op_graph, dominator_op) + " ends at " + STR(HLS->Rsch->get_cstep(dominator_op).second + (allocation_information->GetCycleLatency(dominator_op) - 1u)) + " - " +
                                        GET_NAME(filtered_op_graph, loop_operation) + " ends at " + STR(operation_step));
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
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Updating dominator to BB" + STR(basic_block_graph->CGetBBNodeInfo(current_bb_dominator)->block->number));
               THROW_ASSERT(boost::in_degree(current_bb_dominator, *dominators) == 1, "Dominator is not a tree or entry was reached");
               boost::tie(ie, ie_end) = boost::in_edges(current_bb_dominator, *dominators);
               candidate_bb = boost::source(*ie, *dominators);
               /// If the current is in the barrier, do not check for the candidate
               if(bb_barrier.find(loop_operation) != bb_barrier.end() and bb_barrier.find(loop_operation)->second.find(current_bb_dominator) != bb_barrier.find(loop_operation)->second.end())
               {
                  break;
               }
            }
            if(current_bb_dominator != operation_bb)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operation " + GET_NAME(filtered_op_graph, loop_operation) + " has to be moved in BB" + STR(basic_block_graph->CGetBBNodeInfo(current_bb_dominator)->block->number));
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

void SDCScheduling::Initialize()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Initializing SDCScheduling");
   Scheduling::Initialize();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   op_graph = FB->CGetOpGraph(FunctionBehavior::FLSAODG);
   behavioral_helper = FB->CGetBehavioralHelper();
   feedback_op_graph = FB->CGetOpGraph(FunctionBehavior::FFLSAODG);
   allocation_information = HLS->allocation_information;
   res_binding = HLS->Rfu;
   clock_period = HLS->HLS_C->get_clock_period() * HLS->HLS_C->get_clock_period_resource_fraction();
   margin = allocation_information->GetClockPeriodMargin();
   /// Build the full reachability map
   full_reachability_map.clear();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating reachability map");
   std::deque<vertex> vertices_to_be_analyzed;
   op_graph->ReverseTopologicalSort(vertices_to_be_analyzed);
   for(const auto vertex_to_be_analyzed : vertices_to_be_analyzed)
   {
      OutEdgeIterator eo, eo_end;
      for(boost::tie(eo, eo_end) = boost::out_edges(vertex_to_be_analyzed, *op_graph); eo != eo_end; eo++)
      {
         vertex target = boost::target(*eo, *op_graph);
         full_reachability_map[vertex_to_be_analyzed].insert(target);
         full_reachability_map[vertex_to_be_analyzed].insert(full_reachability_map[target].begin(), full_reachability_map[target].end());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created reachability map");
   unbounded_operations.clear();
   basic_block_graph = FB->CGetBBGraph(FunctionBehavior::BB);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computed unbounded operations");
   VertexIterator basic_block, basic_block_end;
   for(boost::tie(basic_block, basic_block_end) = boost::vertices(*basic_block_graph); basic_block != basic_block_end; basic_block++)
   {
      OpVertexSet filtered_operations(op_graph);
      for(auto const operation : basic_block_graph->CGetBBNodeInfo(*basic_block)->statements_list)
      {
         if(HLS->operations.find(operation) != HLS->operations.end() and not allocation_information->is_operation_bounded(op_graph, operation, allocation_information->GetFuType(operation)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(op_graph, operation));
            unbounded_operations.insert(operation);
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed unbounded operations");
   limited_resources.clear();
   unsigned int resource_types_number = allocation_information->get_number_fu_types();
   for(unsigned int resource_type = 0; resource_type != resource_types_number; resource_type++)
   {
      unsigned int resources_number = allocation_information->get_number_fu(resource_type);
      if(resources_number < INFINITE_UINT and not allocation_information->is_vertex_bounded(resource_type))
      {
         limited_resources.insert(resource_type);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---There are only " + STR(resources_number) + " of type " + allocation_information->get_fu_name(resource_type).first);
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---There are " + STR(resources_number) + " of type " + allocation_information->get_fu_name(resource_type).first);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Computed limited resources");
   sharing_operations.clear();
   /// For the limited resource, the operations assigned to it
   for(boost::tie(basic_block, basic_block_end) = boost::vertices(*basic_block_graph); basic_block != basic_block_end; basic_block++)
   {
      OpVertexSet filtered_operations(op_graph);
      for(auto const operation : basic_block_graph->CGetBBNodeInfo(*basic_block)->statements_list)
      {
         if(HLS->operations.find(operation) != HLS->operations.end())
            filtered_operations.insert(operation);
      }
      for(const auto operation : filtered_operations)
      {
         const unsigned int fu_type = allocation_information->GetFuType(operation);
         if(limited_resources.find(fu_type) != limited_resources.end())
         {
            sharing_operations[fu_type].insert(operation);
         }
      }
   }

   /// If number of resources is limited but larger than number of operations
   CustomSet<unsigned int> not_limited_resources;

   for(auto limited_resource : limited_resources)
   {
      if(sharing_operations[limited_resource].size() <= allocation_information->get_number_fu(limited_resource))
      {
         not_limited_resources.insert(limited_resource);
      }
   }

   /// Removing not actually limited resources
   for(auto not_limited_resource : not_limited_resources)
   {
      sharing_operations.erase(not_limited_resource);
      limited_resources.erase(not_limited_resource);
   }
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Shared resources:");
      for(const auto& shared_resource : sharing_operations)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->" + STR(allocation_information->get_number_fu(shared_resource.first)) + " " + allocation_information->get_fu_name(shared_resource.first).first);
         for(const auto operation : shared_resource.second)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(op_graph, operation));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Initialized SDCScheduling");
}
