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
 *              Copyright (C) 2025-2026 Politecnico di Milano
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
 * @file buildFSM.cpp
 * @brief create the FSM. Loop and functional pipelining supported.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "buildFSM.hpp"

#include "Discrepancy.hpp"
#include "FSMInfo.hpp"
#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "cpu_time.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"

#include <boost/foreach.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/incremental_components.hpp>

#include <cmath>

#define PP_MAX_CYCLES_BOUNDED "max-cycles-bounded"
#define DEFAULT_MAX_CYCLES_BOUNDED (8)

class OpVertexSchedSorter
{
   /// reference to the scheduling
   const Schedule& sch;

 public:
   /**
    * Constructor
    * @param _sch is the schedule used to compare the vertices
    */
   explicit OpVertexSchedSorter(const Schedule& _sch) : sch(_sch)
   {
   }

   /**
    * Compare scheduling of two vertices
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if x has been scheduled before than y
    */
   bool operator()(OpGraph::vertex_descriptor x, OpGraph::vertex_descriptor y) const
   {
      return sch.get_cstep(x) < sch.get_cstep(y);
   }
};

static FSMInfo::edgeData build_edge_condition(const CustomOrderedSet<unsigned int>& cfg_edge_ids,
                                              OpGraph::vertex_descriptor last_operation, stateTransitionType type)
{
   FSMInfo::edgeData ed; // default noEdgeCondition if empty
   ed.edgeSelector = type;
   if(cfg_edge_ids.size())
   {
      CustomOrderedSet<unsigned> labels;
      bool isElseEdge = false;
      for(auto label : cfg_edge_ids)
      {
         if(label == default_COND)
         {
            isElseEdge = true;
         }
         else
         {
            labels.insert(label);
         }
      }
      ed = FSMInfo::buildElseIfCondition(last_operation, labels, isElseEdge, type);
   }

   return ed;
}

buildFSM::buildFSM(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned _funId,
                   const DesignFlowManager& _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::BUILD_FSM)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
buildFSM::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::LIST_BASED_SCHEDULING, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm),
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

void buildFSM::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->fsm_info = FSMInfoRef(new FSMInfo());
}

DesignFlowStep_Status buildFSM::InternalExec()
{
   long int step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      START_TIME(step_time);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting creation of the FSM...");
   auto FB = HLSMgr->CGetFunctionBehavior(funId);

   auto fsm_info = HLS->fsm_info;

   /// first state of a basic-block
   CustomUnorderedMap<operation_descriptor, FSMInfo::state_descriptor> first_state;
   /// last state of a basic-block
   CustomUnorderedMap<operation_descriptor, FSMInfo::state_descriptor> last_state;

   const auto dfg = FB->GetOpGraph(FunctionBehavior::DFG);

   const auto sch = HLS->Rsch;

   const auto fbb = FB->GetBBGraph(FunctionBehavior::FBB);

   /// get entry and exit basic block
   const auto bb_entry = fbb.CGetGraphInfo().entry_vertex;
   const auto bb_exit = fbb.CGetGraphInfo().exit_vertex;

   bool first_state_p;
   bool have_previous = false;
   CustomOrderedMap<unsigned int, FSMInfo::state_descriptor> previous;
   std::map<FSMInfo::state_descriptor, std::list<FSMInfo::state_descriptor>> call_states;
   std::map<FSMInfo::state_descriptor, std::list<operation_descriptor>> call_operations;
   std::map<unsigned, std::map<unsigned, std::list<FSMInfo::state_descriptor>>> last_LP_states;

   const auto is_function_pipelined = FB->is_function_pipelined();
   const auto is_stallable_pipelined = FB->is_stp();
   auto pipelined_function_latency = 0u;

   /// initialize the state entry and exit
   {
      const auto& ogc_info = dfg.CGetGraphInfo();
      std::list<operation_descriptor> entry_ops{ogc_info.entry_vertex};
      unsigned int entry_bb_id = dfg.CGetNodeInfo(ogc_info.entry_vertex).bb_index;
      std::map<operation_descriptor, unsigned> empty_steps_in;
      std::map<operation_descriptor, unsigned> empty_steps_out;

      fsm_info->entryNode = fsm_info->createState(entry_ops, entry_ops, entry_ops, entry_bb_id, empty_steps_in,
                                                  empty_steps_out, 0, 0, false, false, dfg, "ENTRY");
      if(!is_function_pipelined)
      {
         std::list<operation_descriptor> exit_ops{ogc_info.exit_vertex};
         unsigned int exit_bb_id = dfg.CGetNodeInfo(ogc_info.exit_vertex).bb_index;
         fsm_info->exitNode = fsm_info->createState(exit_ops, exit_ops, exit_ops, exit_bb_id, empty_steps_in,
                                                    empty_steps_out, 0, 0, false, false, dfg, "EXIT");
      }
   }

   /// contains the list of operations which are executing, starting, ending and "on-fly" in every state of the FSM
   std::map<FSMInfo::state_descriptor, std::list<operation_descriptor>> global_executing_ops, global_starting_ops,
       global_ending_ops, global_onfly_ops;

   const auto& CGM = HLSMgr->CGetCallGraphManager();
   const bool is_top = CGM.GetRootFunctions().count(funId);
   const auto needMemoryMappedRegisters =
       is_top ? parameters->getOption<bool>(OPT_memory_mapped_top) : HLSMgr->hasToBeInterfaced(funId);
   auto has_registered_inputs = HLS->registered_inputs && !needMemoryMappedRegisters;
   if(is_top && parameters->getOption<std::string>(OPT_registered_inputs) == "top")
   {
      has_registered_inputs = true;
   }
   const auto fu_name = functions::GetFUName(funId, HLSMgr);
   if(HLSMgr->Rfuns->is_a_proxied_function(functions::GetFUName(funId, HLSMgr)) && !needMemoryMappedRegisters)
   {
      if(parameters->getOption<std::string>(OPT_registered_inputs) != "no")
      {
         has_registered_inputs = true;
      }
   }
   else if(parameters->getOption<std::string>(OPT_registered_inputs) == "yes" && !needMemoryMappedRegisters)
   {
      has_registered_inputs = true;
   }
   else if(HLSMgr->isOmpLambdaFunction(funId))
   {
      if(parameters->getOption<std::string>(OPT_registered_inputs) != "no")
      {
         has_registered_inputs = true;
      }
   }
   if(parameters->getOption<std::string>(OPT_registered_inputs) != "no" && !has_registered_inputs &&
      !needMemoryMappedRegisters)
   {
      /// the analysis has to be performed only on the reachable functions
      /// functions to be analyzed
      const auto sort_list = CGM.GetReachedBodyFunctions();
      CustomUnorderedSet<operation_descriptor> vertex_subset;
      for(auto cvertex : sort_list)
      {
         vertex_subset.insert(CGM.GetVertex(cvertex));
      }
      const auto subgraph = CGM.CGetCallSubGraph(vertex_subset);
      const auto current_vertex = CGM.GetVertex(funId);
      size_t n_call_sites = 0;
      for(const auto& ie : subgraph.in_edges(current_vertex))
      {
         const auto& info = subgraph.CGetEdgeInfo(ie);
         n_call_sites += info.direct_call_points.size() + info.indirect_call_points.size();
      }
      HLS->call_sites_number = n_call_sites;
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Number of function call sites = " + STR(n_call_sites));
      unsigned int n_levels = 0;
      const auto controller_delay = HLS->allocation_information->EstimateControllerDelay();
      for(; n_call_sites > (1u << n_levels); ++n_levels)
      {
         ;
      }
      auto dfp_P =
          parameters->isOption(OPT_disable_function_proxy) && parameters->getOption<bool>(OPT_disable_function_proxy);
      auto key_c = std::make_pair(fu_name, WORK_LIBRARY);
      bool has_constraint = HLSMgr->global_resource_constraints.count(key_c);
      double mux_time_estimation =
          (n_levels * HLS->allocation_information->mux_time_unit(32)) + (n_levels > 0 ? controller_delay : 0);
      if(mux_time_estimation > HLS->allocation_information->getMinimumSlack() && (!is_function_pipelined || !dfp_P) &&
         has_constraint)
      {
         has_registered_inputs = true;
      }
   }
   /// in case the function is pipelined the inputs are register in a different way
   HLS->registered_inputs = has_registered_inputs && !is_function_pipelined;

   /// build portion of the FSM associated with each BBs
   for(const auto v : fbb.vertices())
   {
      if(v == bb_entry)
      {
         last_state[v] = first_state[v] = fsm_info->entryNode;
         continue;
      }
      else if(v == bb_exit)
      {
         last_state[v] = first_state[v] = fsm_info->exitNode;
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Building FSM of BB" + STR(fbb.CGetNodeInfo(v).block->number));
      const auto& operations = fbb.CGetNodeInfo(v);
      OpVertexSchedSorter schSorter(*sch);
      std::multiset<operation_descriptor, OpVertexSchedSorter> ordered_operations(schSorter);
      for(auto ops : operations.statements_list)
      {
         ordered_operations.insert(ops);
      }
      if(fbb.in_degree(v) == 1)
      {
         /// for basic block connected only to entry bb
         const auto bb_src = fbb.source(fbb.in_edges(v).front());
         if(bb_src == bb_entry)
         {
            for(auto stmt : ordered_operations)
            {
               if((has_registered_inputs && !is_function_pipelined) || (dfg.CGetNodeInfo(stmt).node_type & TYPE_PHI))
               {
                  /// add an empty state before the current basic block
                  std::list<operation_descriptor> exec_ops, start_ops, end_ops;
                  std::map<operation_descriptor, unsigned> vertex_step_in, vertex_step_out;
                  const auto& entry_operations = fbb.CGetNodeInfo(bb_src);
                  auto entry_ops_it_end = entry_operations.statements_list.end();
                  for(auto entry_ops_it = entry_operations.statements_list.begin(); entry_ops_it_end != entry_ops_it;
                      ++entry_ops_it)
                  {
                     exec_ops.push_back(*entry_ops_it);
                     start_ops.push_back(*entry_ops_it);
                     end_ops.push_back(*entry_ops_it);
                  }
                  unsigned int BB_id = entry_operations.get_bb_index();

                  const auto s_cur = fsm_info->createState(exec_ops, start_ops, end_ops, BB_id, vertex_step_in,
                                                           vertex_step_out, 0, 0, false, false, dfg);
                  FSMInfo::edgeData ed;
                  fsm_info->createEdge(last_state[bb_src], s_cur, ed);
                  last_state[bb_src] = s_cur;
                  break;
               }
            }
         }
      }

      first_state_p = true;
      std::map<unsigned int, std::list<operation_descriptor>> executing_ops, starting_ops, ending_ops, onfly_ops;
      auto max_cstep = 0u;
      auto min_cstep = std::numeric_limits<unsigned int>::max();
      std::map<operation_descriptor, unsigned int> phi_cstep;

      for(auto op : ordered_operations)
      {
         const auto& op_info = dfg.CGetNodeInfo(op);
         if(op_info.node_type & (TYPE_GOTO))
         {
            continue;
         }
         if(op_info.node_type & (TYPE_VPHI))
         {
            continue;
         }
         const auto cstep = sch->get_cstep(op).second;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Analyzing operation " + op_info.vertex_name + " " + STR(cstep));
         if(op_info.node_type & (TYPE_PHI))
         {
            phi_cstep.insert(std::make_pair(op, cstep));
         }
         unsigned int fu_assigned = HLS->Rfu->get_assign(op);
         THROW_ASSERT(sch->get_cstep_end(op) >= sch->get_cstep(op), "unexpected condition");
         const auto delay = sch->get_cstep_end(op).second - cstep;
         const auto end_step = cstep + delay;
         max_cstep = std::max(max_cstep, end_step);
         min_cstep = std::min(min_cstep, cstep);
         executing_ops[cstep].push_back(op);
         starting_ops[cstep].push_back(op);
         const auto initiation_time = HLS->allocation_information->get_initiation_time(fu_assigned, op);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---initiation_time " + STR(initiation_time));
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed operation " + op_info.vertex_name);
      }
      /// in case we reserve a stage where the inputs are registered
      if(is_function_pipelined && has_registered_inputs)
      {
         --min_cstep;
      }

      CustomOrderedMap<unsigned int, FSMInfo::state_descriptor> s_cur;
      auto BBIndex = fbb.CGetNodeInfo(v).block->number;
      auto isLP = sch->IsLoopPipelined(BBIndex);
      auto LPII = sch->GetLoopPipeliningII(BBIndex);
      auto n_iter_LP_FSM_building =
          isLP ? (max_cstep - min_cstep + 1) / LPII + ((max_cstep - min_cstep + 1) % LPII ? 1 : 0) : 1;
#if HAVE_ASSERTS
      bool is_feedback_assigned = false;
      CustomOrderedSet<unsigned int> is_nofeedback_assigned;
#endif
      CustomOrderedSet<unsigned int> is_exit_edge_FB;
      CustomOrderedSet<unsigned int> backedge_cfg_edge_ids;
      CustomOrderedMap<unsigned int, CustomOrderedSet<unsigned int>> no_backedge_cfg_edge_ids;
      if(isLP)
      {
         for(const auto& oe : fbb.out_edges(v))
         {
            auto bb_src = fbb.source(oe);
            auto bb_tgt = fbb.target(oe);
            const auto cfg_edge_ids = fbb.CGetEdgeInfo(oe).get_labels(CFG_SELECTOR);
            if(is_function_pipelined && bb_tgt == fbb.CGetGraphInfo().exit_vertex)
            {
               backedge_cfg_edge_ids = cfg_edge_ids;
            }
            else
            {
               auto& bb_tgt_node_info = fbb.CGetNodeInfo(bb_tgt);
               auto bb_tgt_index = bb_tgt_node_info.get_bb_index();
               if(bb_src == bb_tgt)
               {
                  if(FB_CFG_SELECTOR & fbb.GetSelector(oe))
                  {
                     is_exit_edge_FB.insert(bb_tgt_index);
                  }
                  backedge_cfg_edge_ids = cfg_edge_ids;
#if HAVE_ASSERTS
                  is_feedback_assigned = true;
#endif
               }
               else
               {
                  THROW_ASSERT(is_nofeedback_assigned.find(bb_tgt_index) == is_nofeedback_assigned.end(),
                               "unexpected case");
                  if(FB_CFG_SELECTOR & fbb.GetSelector(oe))
                  {
                     is_exit_edge_FB.insert(bb_tgt_index);
                  }
                  no_backedge_cfg_edge_ids[bb_tgt_index] = cfg_edge_ids;
#if HAVE_ASSERTS
                  is_nofeedback_assigned.insert(bb_tgt_index);
#endif
               }
            }
         }
         THROW_ASSERT((!is_nofeedback_assigned.empty() && is_feedback_assigned) || is_function_pipelined,
                      "unexpected case");
      }
      THROW_ASSERT(!isLP || n_iter_LP_FSM_building > 1, "unexpected condition");
      // std::cerr << "BBIndex " << BBIndex << " isLP " << (isLP ? "T" : "F") << " LPII " << LPII
      // << " n_iter_LP_FSM_building " << n_iter_LP_FSM_building << "\n";
      bool has_previous_LP_first_state = false;
      std::set<FSMInfo::state_descriptor> previous_LP_first_state;
      auto current_LP_first_state = FSMInfo::invalidState;
      CustomOrderedMap<unsigned int, FSMInfo::state_descriptor> next_LP_first_state;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---n_iter_LP_FSM_building=" + STR(n_iter_LP_FSM_building) + " LPII=" + STR(LPII));
      for(unsigned LP_Index = (is_function_pipelined && !is_stallable_pipelined) ? (n_iter_LP_FSM_building - 1) : 0;
          LP_Index < n_iter_LP_FSM_building; ++LP_Index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LP_Index " + STR(LP_Index));
         if(!is_function_pipelined)
         {
            have_previous = false;
         }
         for(auto l = min_cstep; l <= max_cstep; l++)
         {
            if(is_function_pipelined && l >= min_cstep + LPII)
            {
               continue;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering control step " + STR(l));
            std::list<operation_descriptor> exec_ops, start_ops, end_ops, onf_ops;

            std::map<operation_descriptor, unsigned> vertex_step_in, vertex_step_out;
            std::set<operation_descriptor> is_prologue;
            bool has_last_step_op = (l + LP_Index * LPII) >= max_cstep;
            unsigned LP_Index_inner = 0;
            /// build is_prologue
            //  std::cerr << "l + LP_Index * LPII=" << l + LP_Index * LPII << "\n";

            for(const auto& [phi, step] : phi_cstep)
            {
               if(step > (l + LP_Index * LPII))
               {
                  // std::cerr << "is prologue for " << GET_NAME(dfg, phi) << "\n";
                  is_prologue.insert(phi);
               }
            }

            do
            {
               auto c_offset = l + LP_Index_inner * LPII;
               if(executing_ops.find(c_offset) != executing_ops.end())
               {
                  exec_ops.insert(exec_ops.end(), executing_ops.at(c_offset).begin(), executing_ops.at(c_offset).end());
                  for(auto vop : executing_ops.at(c_offset))
                  {
                     vertex_step_in[vop] = c_offset - min_cstep;
                  }
               }

               if(starting_ops.find(c_offset) != starting_ops.end())
               {
                  start_ops.insert(start_ops.end(), starting_ops.at(c_offset).begin(), starting_ops.at(c_offset).end());
               }
               if(ending_ops.find(c_offset) != ending_ops.end())
               {
                  end_ops.insert(end_ops.end(), ending_ops.at(c_offset).begin(), ending_ops.at(c_offset).end());
                  for(auto vop : ending_ops.at(c_offset))
                  {
                     vertex_step_out[vop] = c_offset - min_cstep;
                  }
               }
               if(onfly_ops.find(c_offset) != onfly_ops.end())
               {
                  onf_ops.insert(onf_ops.end(), onfly_ops.at(c_offset).begin(), onfly_ops.at(c_offset).end());
               }
               ++LP_Index_inner;
            } while(LP_Index_inner <= LP_Index);

            unsigned int BB_id = operations.get_bb_index();
            CustomOrderedSet<unsigned int> tgt_ids;
            if(have_previous && isLP && (l == min_cstep + LPII))
            {
               for(const auto& [bb, cfg_edge_ids] : no_backedge_cfg_edge_ids)
               {
                  tgt_ids.insert(bb);
               }
            }
            else if(have_previous)
            {
               for(const auto& [bb, prevState] : previous)
               {
                  tgt_ids.insert(bb);
               }
            }
            else
            {
               tgt_ids.insert(0);
            }

            s_cur.clear();
            for(auto ids : tgt_ids)
            {
               s_cur[ids] =
                   fsm_info->createState(exec_ops, start_ops, end_ops, BB_id, vertex_step_in, vertex_step_out, LPII,
                                         isLP ? (max_cstep - min_cstep) : 0, isLP && has_last_step_op, isLP, dfg);
               if(isLP)
               {
                  auto& state_data = fsm_info->getState(s_cur.at(ids));
                  state_data.isPrologue = is_prologue;
               }
            }

            for(const auto& [bb_tgt, s_curState] : s_cur)
            {
               global_executing_ops[s_curState] = exec_ops;
               global_starting_ops[s_curState] = start_ops;
               global_ending_ops[s_curState] = end_ops;
               global_onfly_ops.insert({s_curState, onf_ops});
            }

            for(const auto& exec_op : exec_ops)
            {
               const auto tn = HLS->allocation_information->get_fu(HLS->Rfu->get_assign(exec_op));
               const auto& op_info = dfg.CGetNodeInfo(exec_op);
               const auto op_tn =
                   GetPointer<functional_unit>(tn)->get_operation(ir_helper::NormalizeTypename(op_info.GetOperation()));
               THROW_ASSERT(GetPointer<operation>(op_tn)->time_m,
                            "Time model not available for operation: " + op_info.vertex_name);
               if(!GetPointer<operation>(op_tn)->is_bounded())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + op_info.vertex_name + " is unbounded");
                  for(const auto& [bb_tgt, s_curState] : s_cur)
                  {
                     call_operations[s_curState].push_back(exec_op);
                  }
               }
            }
            for(const auto& [bb_tgt, s_curState] : s_cur)
            {
               if(call_operations.find(s_curState) != call_operations.end() &&
                  call_operations.find(s_curState)->second.size())
               {
                  THROW_ASSERT(call_operations.find(s_curState) != call_operations.end() &&
                                   call_operations.find(s_curState)->second.begin() !=
                                       call_operations.find(s_curState)->second.end(),
                               "unexpected condition");
                  std::list<operation_descriptor> call_ops(call_operations.find(s_curState)->second.begin(),
                                                           call_operations.find(s_curState)->second.end()),
                      empty_ops;

                  for(const auto opv : global_ending_ops.at(s_curState))
                  {
                     if((dfg.CGetNodeInfo(opv).node_type & TYPE_MULTIIF) != 0 &&
                        std::find(call_ops.begin(), call_ops.end(), opv) == call_ops.end())
                     {
                        call_ops.push_back(opv);
                     }
                  }
                  unsigned int call_BB_id = operations.get_bb_index();
                  auto s_call = fsm_info->createState(call_ops, empty_ops, call_ops, call_BB_id, vertex_step_in,
                                                      vertex_step_out, LPII, isLP ? (max_cstep - min_cstep) : 0,
                                                      isLP && has_last_step_op, isLP, dfg);
                  if(isLP)
                  {
                     auto& state_data = fsm_info->getState(s_call);
                     state_data.isPrologue = is_prologue;
                  }
                  fsm_info->getState(s_call).isDummy = true;
                  call_states[s_curState].push_back(s_call);
                  if(call_operations.find(s_curState)->second.size() > 1)
                  {
                     std::vector<operation_descriptor> ops(call_operations.find(s_curState)->second.begin(),
                                                           call_operations.find(s_curState)->second.end());
                     // TODO: ordering and uniqueness check might not be necessary
                     std::sort(ops.begin(), ops.end());
                     ops.erase(std::unique(ops.begin(), ops.end()), ops.end());
                     fsm_info->addMultiUnboundedObj(s_curState, ops);
                  }
               }
            }
            if(have_previous)
            {
               if(call_states.find(previous.begin()->second) == call_states.end())
               {
                  if(isLP && (l == min_cstep + LPII))
                  {
                     THROW_ASSERT(previous.size() == 1, "unexpected condition");
                     previous_LP_first_state.clear();
                     for(const auto& [bb_tgt, s_curState] : s_cur)
                     {
                        const auto& bb_node_info = fbb.CGetNodeInfo(v);
                        THROW_ASSERT(bb_node_info.statements_list.size(),
                                     "at least one operation should belong to this basic block");
                        auto last_operation = *(bb_node_info.statements_list.rbegin());
                        THROW_ASSERT(no_backedge_cfg_edge_ids.find(bb_tgt) != no_backedge_cfg_edge_ids.end(),
                                     "unexpected condition");
                        next_LP_first_state[bb_tgt] = s_curState;
                        auto typeSelector = is_exit_edge_FB.contains(bb_tgt) ? stEdgeFeedback : stEdgeNormal;
                        auto edgeInfoObj =
                            build_edge_condition(no_backedge_cfg_edge_ids.at(bb_tgt), last_operation, typeSelector);
                        fsm_info->createEdge(previous.begin()->second, s_curState, edgeInfoObj);
                     }
                     has_previous_LP_first_state = true;
                     previous_LP_first_state.insert(previous.begin()->second);
                  }
                  else
                  {
                     THROW_ASSERT(previous.size() == s_cur.size(), "unexpected condition");
                     for(const auto& [previousBB, previousState] : previous)
                     {
                        THROW_ASSERT(s_cur.find(previousBB) != s_cur.end(), "unexpected condition");
                        FSMInfo::edgeData ed;
                        fsm_info->createEdge(previousState, s_cur.at(previousBB), ed);
                     }
                  }
               }
               else
               {
                  if(isLP && (l == min_cstep + LPII))
                  {
                     previous_LP_first_state.clear();
                     THROW_ASSERT(previous.size() == 1, "unexpected condition");
                     for(const auto& [bb_tgt, s_curState] : s_cur)
                     {
                        const auto& previousState = previous.begin()->second;
                        THROW_ASSERT(call_states.count(previousState), "unexpected condition");
                        auto call_sets = call_states.find(previousState)->second;

                        const auto& bb_node_info = fbb.CGetNodeInfo(v);
                        THROW_ASSERT(bb_node_info.statements_list.size(),
                                     "at least one operation should belong to this basic block");
                        auto last_operation = *(bb_node_info.statements_list.rbegin());
                        THROW_ASSERT(no_backedge_cfg_edge_ids.find(bb_tgt) != no_backedge_cfg_edge_ids.end(),
                                     "unexpected condition");

                        auto typeSelector = is_exit_edge_FB.contains(bb_tgt) ? stEdgeFeedback : stEdgeNormal;
                        auto edgeInfoObj =
                            build_edge_condition(no_backedge_cfg_edge_ids.at(bb_tgt), last_operation, typeSelector);
                        fsm_info->createEdge(previousState, s_curState, edgeInfoObj);

                        for(auto& call_set : call_sets)
                        {
                           edgeInfoObj =
                               build_edge_condition(no_backedge_cfg_edge_ids.at(bb_tgt), last_operation, typeSelector);
                           fsm_info->createEdge(call_set, s_curState, edgeInfoObj);
                           previous_LP_first_state.insert(call_set);
                        }
                        next_LP_first_state[bb_tgt] = s_curState;
                     }
                     has_previous_LP_first_state = true;
                     previous_LP_first_state.insert(previous.begin()->second);
                  }
                  else
                  {
                     for(const auto& [previousBB, previousState] : previous)
                     {
                        THROW_ASSERT(call_operations.find(previousState) != call_operations.end() &&
                                         call_operations.find(previousState)->second.begin() !=
                                             call_operations.find(previousState)->second.end(),
                                     "unexpected condition");
                        THROW_ASSERT(call_states.count(previousState), "unexpected condition");
                        std::vector<operation_descriptor> ops(call_operations.find(previousState)->second.begin(),
                                                              call_operations.find(previousState)->second.end());
                        // TODO: ordering and uniqueness check might not be necessary
                        std::sort(ops.begin(), ops.end());
                        ops.erase(std::unique(ops.begin(), ops.end()), ops.end());
                        auto call_sets = call_states.find(previousState)->second;
                        THROW_ASSERT(s_cur.find(previousBB) != s_cur.end(), "unexpected condition");
                        auto edgeInfoObj = FSMInfo::buildUnboundedCondition(doneVariableLatencyOpEdgeCondition, ops,
                                                                            previousState, stEdgeNormal);
                        for(auto& call_set : call_sets)
                        {
                           fsm_info->createEdge(call_set, s_cur.at(previousBB), edgeInfoObj);
                        }
                        fsm_info->createEdge(previousState, s_cur.at(previousBB), edgeInfoObj);
                     }
                  }
               }
            }
            else
            {
               have_previous = true;
            }
            if(isLP && l == min_cstep && LP_Index && has_previous_LP_first_state)
            {
               THROW_ASSERT(s_cur.size() == 1, "unexpected condition");
               const auto& bb_node_info = fbb.CGetNodeInfo(v);
               THROW_ASSERT(bb_node_info.statements_list.size(),
                            "at least one operation should belong to this basic block");
               auto last_operation = *(bb_node_info.statements_list.rbegin());
               auto edgeInfoObj = build_edge_condition(backedge_cfg_edge_ids, last_operation, stEdgeNormal);
               for(auto pv_state : previous_LP_first_state)
               {
                  fsm_info->createEdge(pv_state, s_cur.begin()->second, edgeInfoObj);
               }
               current_LP_first_state = s_cur.begin()->second;
            }
            else if(is_function_pipelined && isLP && l == min_cstep && LP_Index)
            {
               THROW_ASSERT(s_cur.size() == 1, "unexpected condition");
               current_LP_first_state = s_cur.begin()->second;
            }

            previous = s_cur;
            if(first_state_p)
            {
               THROW_ASSERT(s_cur.size() == 1, "unexpected condition");
               first_state[v] = s_cur.begin()->second;
               first_state_p = false;
            }
            for(const auto& [bb_tgt, s_curState] : s_cur)
            {
               if(call_states.count(s_curState))
               {
                  THROW_ASSERT(call_operations.find(s_curState) != call_operations.end() &&
                                   call_operations.find(s_curState)->second.begin() !=
                                       call_operations.find(s_curState)->second.end(),
                               "unexpected condition");
                  THROW_ASSERT(call_states.find(s_curState) != call_states.end() &&
                                   call_states.find(s_curState)->second.begin() !=
                                       call_states.find(s_curState)->second.end(),
                               "unexpected condition");
                  const auto waiting_state = call_states.at(s_curState).front();

                  std::vector<operation_descriptor> ops(call_operations.at(s_curState).begin(),
                                                        call_operations.at(s_curState).end());
                  // TODO: ordering and uniqueness check might not be necessary
                  std::sort(ops.begin(), ops.end());
                  ops.erase(std::unique(ops.begin(), ops.end()), ops.end());

                  auto edgeInfoObj = FSMInfo::buildUnboundedCondition(runningVariableLatencyOpEdgeCondition, ops,
                                                                      s_curState, stEdgeNormal);
                  fsm_info->createEdge(s_curState, waiting_state, edgeInfoObj);

                  edgeInfoObj = FSMInfo::buildUnboundedCondition(runningVariableLatencyOpEdgeCondition, ops, s_curState,
                                                                 stEdgeFeedback);
                  fsm_info->createEdge(waiting_state, waiting_state, edgeInfoObj);

                  if(is_function_pipelined && isLP && LP_Index == n_iter_LP_FSM_building - 1 &&
                     l == min_cstep + (LPII - 1))
                  {
                     edgeInfoObj = FSMInfo::buildUnboundedCondition(doneVariableLatencyOpEdgeCondition, ops,
                                                                    current_LP_first_state, stEdgeFeedback);
                     fsm_info->createEdge(waiting_state, current_LP_first_state, edgeInfoObj);
                  }
               }
            }
            if(l == max_cstep)
            {
               THROW_ASSERT(s_cur.size() == 1 || isLP, "unexpected condition");
               for(const auto& [bb_tgt, s_curState] : s_cur)
               {
                  if(isLP)
                  {
                     last_LP_states[BBIndex][bb_tgt].push_back(s_curState);
                  }
                  else
                  {
                     last_state[v] = s_curState;
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered control step " + STR(l));
            if(is_function_pipelined && isLP && LP_Index == n_iter_LP_FSM_building - 1 && l == min_cstep + (LPII - 1))
            {
               THROW_ASSERT(s_cur.size() == 1, "unexpected condition");
               const auto& s_curState = s_cur.begin()->second;
               fsm_info->exitNode = s_curState;

               FSMInfo::edgeData ed;
               ed.edgeSelector = stEdgeFeedback;
               if(call_states.count(s_curState))
               {
                  std::vector<operation_descriptor> ops(call_operations.at(s_curState).begin(),
                                                        call_operations.at(s_curState).end());
                  // TODO: ordering and uniqueness check might not be necessary
                  std::sort(ops.begin(), ops.end());
                  ops.erase(std::unique(ops.begin(), ops.end()), ops.end());
                  ed = FSMInfo::buildUnboundedCondition(doneVariableLatencyOpEdgeCondition, ops, current_LP_first_state,
                                                        stEdgeFeedback);
               }
               fsm_info->createEdge(s_curState, current_LP_first_state, ed);
            }
            else if(isLP && LP_Index == n_iter_LP_FSM_building - 1 && l == min_cstep + (LPII - 1))
            {
               THROW_ASSERT(s_cur.size() == 1, "unexpected condition");
               const auto& s_curState = s_cur.begin()->second;
               const auto& bb_node_info = fbb.CGetNodeInfo(v);
               THROW_ASSERT(bb_node_info.statements_list.size(),
                            "at least one operation should belong to this basic block");
               auto last_operation = *(bb_node_info.statements_list.rbegin());

               auto edgeInfoObj = build_edge_condition(backedge_cfg_edge_ids, last_operation, stEdgeFeedback);
               fsm_info->createEdge(s_curState, current_LP_first_state, edgeInfoObj);

               if(call_states.count(s_curState))
               {
                  auto call_sets = call_states.find(s_curState)->second;
                  for(auto& call_set : call_sets)
                  {
                     fsm_info->createEdge(call_set, current_LP_first_state, edgeInfoObj);
                  }
               }
               for(auto [tgt_bb, nLPfirst_state] : next_LP_first_state)
               {
                  auto edgeSelector = is_exit_edge_FB.contains(tgt_bb) ? stEdgeFeedback : stEdgeNormal;
                  edgeInfoObj = build_edge_condition(no_backedge_cfg_edge_ids.at(tgt_bb), last_operation, edgeSelector);
                  fsm_info->createEdge(s_curState, nLPfirst_state, edgeInfoObj);

                  if(call_states.count(s_curState))
                  {
                     auto call_sets = call_states.find(s_curState)->second;
                     for(auto& call_set : call_sets)
                     {
                        fsm_info->createEdge(call_set, nLPfirst_state, edgeInfoObj);
                     }
                  }
               }
               break;
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Built FSM of BB" + STR(fbb.CGetNodeInfo(v).block->number));
      if(is_function_pipelined)
      {
         fsm_info->nStages = 1 + max_cstep - min_cstep;
      }
      if(isLP)
      {
         pipelined_function_latency = 1 + max_cstep - min_cstep;
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Iteration latency for function " + FB->CGetBehavioralHelper()->GetFunctionName() + ":BB" +
                            std::to_string(BBIndex) + " = " + std::to_string(pipelined_function_latency));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Initiation Interval (II) for function " + FB->CGetBehavioralHelper()->GetFunctionName() +
                            ":BB" + std::to_string(BBIndex) + " = " + std::to_string(LPII));
      }
   }

   /// connect two states belonging to different basic blocks
   /// concurrently manage entry and exit state and completely merged basic blocks
   for(const auto& e : fbb.edges())
   {
      const auto bb_src = fbb.source(e);
      auto srcBBIndex = fbb.CGetNodeInfo(bb_src).block->number;
      auto isLP = sch->IsLoopPipelined(srcBBIndex);
      if(last_state.find(bb_src) == last_state.end() && !isLP)
      {
         continue;
      }
      auto bb_tgt = fbb.target(e);
      /// removed the edge from entry to exit
      if(bb_src == bb_entry && bb_tgt == bb_exit)
      {
         continue;
      }
      FSMInfo::state_descriptor s_tgt;
      if(bb_tgt == bb_entry)
      {
         s_tgt = fsm_info->entryNode;
      }
      else if(bb_tgt == bb_exit)
      {
         s_tgt = fsm_info->exitNode;
      }
      else
      {
         while(first_state.find(bb_tgt) == first_state.end())
         {
            THROW_ASSERT(fbb.out_degree(bb_tgt) == 1, "unexpected pattern");
            bb_tgt = fbb.target(fbb.out_edges(bb_tgt).front());
         }
         s_tgt = first_state.at(bb_tgt);
      }

      auto edge_type = (FB_CFG_SELECTOR & fbb.GetSelector(e)) ? stEdgeFeedback : stEdgeNormal;
      auto manage_edges_and_calls = [&](FSMInfo::state_descriptor ls) {
         /// compute the controlling vertex
         const auto& bb_node_info = fbb.CGetNodeInfo(bb_src);
         THROW_ASSERT(bb_node_info.statements_list.size(), "at least one operation should belong to this basic block");
         const auto last_operation = *(bb_node_info.statements_list.rbegin());
         const auto& cfg_edge_ids = fbb.CGetEdgeInfo(e).get_labels(CFG_SELECTOR);
         FSMInfo::edgeData edgeInfoObj;
         edgeInfoObj.edgeSelector = edge_type;

         if(!isLP)
         {
            if(!cfg_edge_ids.empty())
            {
               edgeInfoObj = build_edge_condition(cfg_edge_ids, last_operation, edge_type);
            }
            else
            {
               if(call_operations.find(ls) != call_operations.end())
               {
                  std::vector<operation_descriptor> ops(call_operations.at(ls).begin(), call_operations.at(ls).end());
                  // TODO: ordering and uniqueness check might not be necessary
                  std::sort(ops.begin(), ops.end());
                  ops.erase(std::unique(ops.begin(), ops.end()), ops.end());
                  edgeInfoObj =
                      FSMInfo::buildUnboundedCondition(doneVariableLatencyOpEdgeCondition, ops, ls, edge_type);
               }
            }
         }
         fsm_info->createEdge(ls, s_tgt, edgeInfoObj);
         if(call_states.find(ls) != call_states.end())
         {
            const auto& call_sets = call_states.at(ls);
            THROW_ASSERT(call_operations.find(ls) != call_operations.end() &&
                             call_operations.find(ls)->second.size() != 0,
                         "State " + fsm_info->getState(ls).name + " does not contain any call expression");
            for(const auto& call_set : call_sets)
            {
               if(!isLP)
               {
                  if(!cfg_edge_ids.empty())
                  {
                     auto edgeInfoObj0 = build_edge_condition(cfg_edge_ids, last_operation, edge_type);
                     fsm_info->createEdge(call_set, s_tgt, edgeInfoObj0);
                  }
                  else
                  {
                     std::vector<operation_descriptor> ops(call_operations.at(ls).begin(),
                                                           call_operations.at(ls).end());
                     // TODO: ordering and uniqueness check might not be necessary
                     std::sort(ops.begin(), ops.end());
                     ops.erase(std::unique(ops.begin(), ops.end()), ops.end());

                     auto edgeInfoObj1 =
                         FSMInfo::buildUnboundedCondition(doneVariableLatencyOpEdgeCondition, ops, ls, edge_type);
                     fsm_info->createEdge(call_set, s_tgt, edgeInfoObj1);
                  }
               }
               else
               {
                  FSMInfo::edgeData ed;
                  fsm_info->createEdge(call_set, s_tgt, ed);
               }
            }
         }
      };
      if(isLP)
      {
         if(bb_tgt != bb_src && !is_function_pipelined)
         {
            auto tgtBBIndex = fbb.CGetNodeInfo(bb_tgt).block->number;
            THROW_ASSERT(last_LP_states.find(srcBBIndex) != last_LP_states.end(), "expected end states for a LPBB");
            THROW_ASSERT(last_LP_states.at(srcBBIndex).find(tgtBBIndex) != last_LP_states.at(srcBBIndex).end(),
                         "expected end states for a LPBB");
            for(const auto& ls : last_LP_states.at(srcBBIndex).at(tgtBBIndex))
            {
               manage_edges_and_calls(ls);
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Analyzing BB" + STR(fbb.CGetNodeInfo(bb_src).block->number) + "-->BB" +
                            STR(fbb.CGetNodeInfo(bb_tgt).block->number));
         FSMInfo::state_descriptor s_src;
         if(bb_src == bb_exit)
         {
            s_src = fsm_info->exitNode;
         }
         else
         {
            THROW_ASSERT(last_state.find(bb_src) != last_state.end(), "missing a state vertex");
            s_src = last_state.find(bb_src)->second;
         }
         manage_edges_and_calls(s_src);

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Analyzed BB" + STR(fbb.CGetNodeInfo(bb_src).block->number) + "-->BB" +
                            STR(fbb.CGetNodeInfo(bb_tgt).block->number));
      }
   }

   fsm_info->finalizeFSMInfo(dfg, HLSMgr);

   ///*****************************************************
   ComputeCyclesCount(pipelined_function_latency);
   HLS->registered_done_port = [&]() {
      if(fsm_info->minCycles != 1 && !is_function_pipelined)
      {
         /// check for unbounded op executed in the last step
         /// this ops creates problems with done port registering
         const auto exit_state = fsm_info->exitNode;
         for(const auto& src_state : fsm_info->predecessors(exit_state))
         {
            if(fsm_info->getState(src_state).isDummy)
            {
               return false;
            }
         }
         return true;
      }
      return false;
   }();
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "-->State Transition Graph Information of function " + FB->CGetBehavioralHelper()->GetFunctionName() +
                      ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                  "---Number of operations: " + STR(FB->GetOpGraph(FunctionBehavior::CFG).num_vertices() - 2));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                  "---Number of basic blocks: " + STR(FB->GetBBGraph(FunctionBehavior::BB).num_vertices() - 2));
   print_statistics();
   if(has_registered_inputs)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Parameters are registered");
   }
   if(HLS->registered_done_port)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Done port is registered");
   }

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      fsm_info->writeDot(FB->GetDotPath() / "HLS_FSM.dot", FB, HLS);
      fsm_info->writeDot(FB->GetDotPath() / "fsm-mini.dot", FB, HLS, 1);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "FSM created!");
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
   }
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---Time to perform creation of the FSM: " + print_cpu_time(step_time) + " seconds");
   }
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   return DesignFlowStep_Status::SUCCESS;
}

void buildFSM::print_statistics() const
{
   auto fsm_info = HLS->fsm_info;
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---Number of states: " + STR(HLSMgr->CGetFunctionBehavior(funId)->is_function_pipelined() ?
                                                    fsm_info->getNumberOfStates(true) :
                                                    fsm_info->getNumberOfStates(false)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level,
                  "---" + std::string("Is a DAG: ") + (fsm_info->isADag ? "T" : "F"));
   if(fsm_info->minCycles != 0 && fsm_info->maxCycles != 0)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Minimum number of cycles: " + STR(fsm_info->minCycles));
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Maximum number of cycles " + STR(fsm_info->maxCycles));
   }
}

void buildFSM::ComputeCyclesCount(unsigned int latency)
{
   auto fsm_info = HLS->fsm_info;
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto is_function_pipelined = FB->is_function_pipelined();
   const unsigned int _maxCycles_bounded = parameters->IsParameter(PP_MAX_CYCLES_BOUNDED) ?
                                               parameters->GetParameter<unsigned int>(PP_MAX_CYCLES_BOUNDED) :
                                               DEFAULT_MAX_CYCLES_BOUNDED;
   if(is_function_pipelined || (fsm_info->isADag && !parameters->getOption<bool>(OPT_disable_bounded_function)))
   {
      std::list<FSMInfo::state_descriptor> sorted_vertices;
      fsm_info->topologicalOrder(sorted_vertices);
      CustomUnorderedMap<FSMInfo::state_descriptor, unsigned int> CSteps_min, CSteps_max;
      bool hasDummyState = false;
      for(const auto v : sorted_vertices)
      {
         CSteps_min[v] = 0;
         CSteps_max[v] = 0;
         hasDummyState |= fsm_info->getState(v).isDummy;
         auto first_edge = true;
         for(auto src : fsm_info->predecessors(v, true))
         {
            CSteps_max[v] = std::max(CSteps_max[v], 1 + CSteps_max[src]);
            if(first_edge)
            {
               first_edge = false;
               CSteps_min[v] = 1 + CSteps_min[src];
            }
            else
            {
               CSteps_min[v] = std::min(CSteps_min[v], 1 + CSteps_max[src]);
            }
         }
      }
      THROW_ASSERT(CSteps_min.find(fsm_info->exitNode) != CSteps_min.end(), "Exit node not reachable");
      THROW_ASSERT(CSteps_max.find(fsm_info->exitNode) != CSteps_max.end(), "Exit node not reachable");
      if(is_function_pipelined)
      {
         fsm_info->minCycles = latency;
         fsm_info->maxCycles = latency;
      }
      else
      {
         fsm_info->minCycles = CSteps_min.at(fsm_info->exitNode) - (is_function_pipelined ? 0 : 1);
         fsm_info->maxCycles = CSteps_max.at(fsm_info->exitNode) - (is_function_pipelined ? 0 : 1);
      }
      fsm_info->hasDummyState = hasDummyState || parameters->getOption<bool>(OPT_disable_bounded_function);
      fsm_info->bounded = is_function_pipelined ||
                          (fsm_info->minCycles == fsm_info->maxCycles && fsm_info->maxCycles <= _maxCycles_bounded &&
                           fsm_info->minCycles > 0 && !hasDummyState);
   }
}
