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
 * @file CSE.cpp
 * @brief common subexpression elimination step
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "CSE.hpp"

#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "application_frontend_flow_step.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "op_graph.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#define FMT_HEADER_ONLY 1
#define DISABLE_NAUTY
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <kitty/constructors.hpp>
#include <kitty/operations.hpp>
#include <limits>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/klut.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace
{
   class predicate_klut_builder
   {
    public:
      using network_t = mockturtle::klut_network;
      using signal_t = network_t::signal;

      signal_t build(const ir_nodeRef& node)
      {
         if(!node || failed)
         {
            failed = true;
            return ntk.get_constant(false);
         }
         if(cache.find(node->index) != cache.end())
         {
            return cache.at(node->index);
         }
         const auto signal = build_uncached(node);
         cache[node->index] = signal;
         return signal;
      }

      bool ok() const
      {
         return !failed;
      }

      bool mutually_exclusive(const ir_nodeRef& first_predicate, const ir_nodeRef& second_predicate)
      {
         const auto first = build(first_predicate);
         const auto second = build(second_predicate);
         if(!ok())
         {
            return false;
         }
         ntk.create_po(ntk.create_and(first, second));
         const auto sim = mockturtle::simulate<kitty::dynamic_truth_table>(
             ntk, mockturtle::default_simulator<kitty::dynamic_truth_table>(ntk.num_pis()));
         return !sim.empty() && kitty::is_const0(sim.front());
      }

    private:
      static constexpr unsigned int max_pis = 16;
      static constexpr unsigned int max_recursion_depth = 128;

      network_t ntk;
      std::map<unsigned int, signal_t> cache;
      unsigned int recursion_depth = 0;
      bool failed = false;

      signal_t create_pi()
      {
         if(ntk.num_pis() >= max_pis)
         {
            failed = true;
            return ntk.get_constant(false);
         }
         return ntk.create_pi();
      }

      signal_t build_ssa(const ir_nodeRef& node)
      {
         const auto ssa = GetPointerS<const ssa_node>(node);
         const auto def_stmt = ssa->GetDefStmt();
         const auto assign = GetPointer<const assign_stmt>(def_stmt);
         if(!assign || assign->op0->index != node->index)
         {
            return create_pi();
         }
         return build(assign->op1);
      }

      signal_t build_lut(const lut_node* lut)
      {
         if(!lut->op0 || lut->op0->get_kind() != constant_int_val_node_K)
         {
            failed = true;
            return ntk.get_constant(false);
         }
         std::vector<signal_t> inputs;
         for(const auto& op : {lut->op1, lut->op2, lut->op3, lut->op4, lut->op5, lut->op6, lut->op7, lut->op8})
         {
            if(op)
            {
               inputs.push_back(build(op));
            }
         }
         if(!ok())
         {
            return ntk.get_constant(false);
         }

         const auto cst_val = ir_helper::GetConstValue(lut->op0);
         if(cst_val == -1LL)
         {
            return ntk.create_not(ntk.get_constant(false));
         }
         if(ir_helper::GetConstValue(lut->op0, false) > std::numeric_limits<unsigned long long>::max())
         {
            failed = true;
            return ntk.get_constant(false);
         }
         kitty::dynamic_truth_table tt(static_cast<unsigned>(inputs.size()));
         std::stringstream res_hex;
         res_hex << std::hex << static_cast<unsigned long long>(cst_val);
         auto res = res_hex.str();
         if(tt.num_vars() > 1)
         {
            while((res.size() << 2) < tt.num_bits())
            {
               res = "0" + res;
            }
         }
         while((res.size() << 2) > tt.num_bits() && tt.num_vars() > 1)
         {
            res = res.substr(1);
         }
         kitty::create_from_hex_string(tt, res);
         return ntk.create_node(inputs, tt);
      }

      signal_t build_uncached(const ir_nodeRef& node)
      {
         if(++recursion_depth > max_recursion_depth)
         {
            failed = true;
            --recursion_depth;
            return ntk.get_constant(false);
         }

         signal_t result = ntk.get_constant(false);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
         switch(node->get_kind())
         {
            case constant_int_val_node_K:
            {
               result = ntk.get_constant(ir_helper::GetConstValue(node) != 0);
               break;
            }
            case ssa_node_K:
            {
               result = build_ssa(node);
               break;
            }
            case not_node_K:
            {
               result = ntk.create_not(build(GetPointerS<const unary_node>(node)->op));
               break;
            }
            case nop_node_K:
            case bitcast_node_K:
            {
               if(ir_helper::Size(node) != 1)
               {
                  result = create_pi();
                  break;
               }
               result = build(GetPointerS<const unary_node>(node)->op);
               break;
            }
            case and_node_K:
            {
               const auto binary = GetPointerS<const binary_node>(node);
               result = ntk.create_and(build(binary->op0), build(binary->op1));
               break;
            }
            case or_node_K:
            {
               const auto binary = GetPointerS<const binary_node>(node);
               result = ntk.create_or(build(binary->op0), build(binary->op1));
               break;
            }
            case xor_node_K:
            {
               const auto binary = GetPointerS<const binary_node>(node);
               result = ntk.create_xor(build(binary->op0), build(binary->op1));
               break;
            }
            case eq_node_K:
            case ne_node_K:
            {
               const auto binary = GetPointerS<const binary_node>(node);
               if(ir_helper::Size(binary->op0) != 1 || ir_helper::Size(binary->op1) != 1)
               {
                  result = create_pi();
                  break;
               }
               const auto xor_signal = ntk.create_xor(build(binary->op0), build(binary->op1));
               result = node->get_kind() == eq_node_K ? ntk.create_not(xor_signal) : xor_signal;
               break;
            }
            case lut_node_K:
            {
               result = build_lut(GetPointerS<const lut_node>(node));
               break;
            }
            case extract_bit_node_K:
            {
               result = create_pi();
               break;
            }
            default:
            {
               result = create_pi();
               break;
            }
         }
#pragma GCC diagnostic pop
         --recursion_depth;
         return result;
      }
   };

   bool predicates_are_mutually_exclusive(const ir_nodeRef& first_predicate, const ir_nodeRef& second_predicate)
   {
      predicate_klut_builder builder;
      return builder.mutually_exclusive(first_predicate, second_predicate);
   }

   bool is_predicated_memory_access(const assign_stmt* stmt)
   {
      return stmt && stmt->predicate && stmt->predicate->get_kind() != constant_int_val_node_K;
   }

   bool is_predicated_load(const assign_stmt* stmt)
   {
      return stmt && stmt->op0->get_kind() == ssa_node_K && stmt->op1->get_kind() == mem_access_node_K &&
             is_predicated_memory_access(stmt);
   }

   bool is_predicated_store(const assign_stmt* stmt)
   {
      return stmt && stmt->op0->get_kind() == mem_access_node_K && is_predicated_memory_access(stmt);
   }
} // namespace

CSE::CSE(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id,
         const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, CSE_STEP, _design_flow_manager, _parameters),
      TM(_AppM->get_ir_manager()),
      restart_phi_opt(false),
      restart_lut_opt(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
CSE::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
         relationships.insert(std::make_pair(DCE_PASS, SAME_FUNCTION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         if(!(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy)))
         {
            relationships.insert(std::make_pair(SIMPLE_CODE_MOTION, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::make_pair(DCE_PASS, SAME_FUNCTION));
            if(restart_phi_opt)
            {
               relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
            }
            if(restart_lut_opt)
            {
               relationships.insert(std::make_pair(BITVALUE_RANGE, SAME_FUNCTION));
            }
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

void CSE::Initialize()
{
   if(GetPointer<const HLS_manager>(AppM) && GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id) &&
      GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      schedule = GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
   }
}

DesignFlowStep_Status CSE::InternalExec()
{
   if(parameters->IsParameter("disable-cse") && parameters->GetParameter<unsigned int>("disable-cse") == 1)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   bool IR_changed = false;
   restart_phi_opt = false;
   restart_lut_opt = false;
#ifndef NDEBUG
   size_t n_equiv_stmt = 0;
#endif
   const auto IRman = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));
   /// define a map relating variables and columns
   std::map<BBGraph::vertex_descriptor, CustomUnorderedMapStable<CSE_tuple_key_type, ir_nodeRef>> unique_table;

   const auto temp = TM->GetIRNode(function_id);
   const auto fd = GetPointerS<const function_val_node>(temp);
   const auto sl = GetPointerS<const statement_list_node>(fd->body);

   /// store the IR based BB graph ala boost::graph
   BBGraphsCollection bb_graphs_collection(BBGraphInfo(AppM, function_id));
   BBGraph cfg(bb_graphs_collection, CFG_SELECTOR);
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor> inverse_vertex_map;
   /// add vertices
   for(const auto& block : sl->list_of_bloc)
   {
      inverse_vertex_map[block.first] = bb_graphs_collection.AddVertex(BBNodeInfo(block.second));
   }
   /// add edges
   for(const auto& idx_bb : sl->list_of_bloc)
   {
      for(const auto& pred : idx_bb.second->list_of_pred)
      {
         THROW_ASSERT(inverse_vertex_map.find(pred) != inverse_vertex_map.end(),
                      "BB" + STR(pred) + " (successor of BB" + STR(idx_bb.first) + ") does not exist");
         bb_graphs_collection.AddEdge(inverse_vertex_map[pred], inverse_vertex_map[idx_bb.first], CFG_SELECTOR);
      }
      for(const auto& succ : idx_bb.second->list_of_succ)
      {
         if(succ == bloc::EXIT_BLOCK_ID)
         {
            bb_graphs_collection.AddEdge(inverse_vertex_map[idx_bb.first], inverse_vertex_map[succ], CFG_SELECTOR);
         }
      }
      if(idx_bb.second->list_of_succ.empty())
      {
         bb_graphs_collection.AddEdge(inverse_vertex_map[idx_bb.first], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                      CFG_SELECTOR);
      }
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   bb_graphs_collection.AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                CFG_SELECTOR);

   dominance<BBGraph> bb_dominators(cfg, inverse_vertex_map[bloc::ENTRY_BLOCK_ID],
                                    inverse_vertex_map[bloc::EXIT_BLOCK_ID]);
   BBGraph dt(bb_graphs_collection, D_SELECTOR);
   bb_dominators.forEachDominanceRelation(
       [&](const BBGraph::vertex_descriptor child, const BBGraph::vertex_descriptor dom_vertex) {
          if(child != inverse_vertex_map[bloc::ENTRY_BLOCK_ID])
          {
             bb_graphs_collection.AddEdge(dom_vertex, child, D_SELECTOR);
          }
       });

   const auto dependence_graph = OpGraph(function_behavior->GetOpGraphsCollection(), SAODG_SELECTOR);
   const auto& stmt_to_op = dependence_graph.CGetGraphInfo().ir_node_to_operation;

   std::deque<BBGraph::vertex_descriptor> sort_list;
   dt.TopologicalSort(sort_list);

   for(const auto& bb : sort_list)
   {
      const auto B = dt.CGetNodeInfo(bb).block;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering BB " + STR(B->number));
      /// CSE on basic blocks
      unique_table[bb].clear();
      const auto dom_vertex = bb_dominators.getImmediateDominator(bb);
      if(dom_vertex != bb)
      {
         THROW_ASSERT(unique_table.find(dom_vertex) != unique_table.end(), "unexpected condition");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Adding dominator equiv: " + STR(dt.CGetNodeInfo(dom_vertex).block->number));

         for(const auto& key_value_pair : unique_table.at(dom_vertex))
         {
            unique_table.at(bb)[key_value_pair.first] = key_value_pair.second;
         }
      }
      IRNodeSet to_be_removed;
      bool reorder_luts = false;
      const auto is_available_before = [&](const ir_nodeRef& value, const ir_nodeRef& use_stmt) -> bool {
         if(!value || value->get_kind() != ssa_node_K)
         {
            return true;
         }

         const auto value_ssa = GetPointerS<const ssa_node>(value);
         if(value_ssa->virtual_flag)
         {
            return true;
         }

         const auto def_stmt = value_ssa->GetDefStmt();
         if(!def_stmt)
         {
            return true;
         }
         if(def_stmt->index == use_stmt->index)
         {
            return false;
         }

         if(def_stmt->get_kind() == phi_stmt_K)
         {
            return true;
         }

         const auto def_stmt_node = GetPointerS<const node_stmt>(def_stmt);
         if(def_stmt_node->bb_index != B->number)
         {
            return true;
         }

         for(const auto& current_stmt : B->CGetStmtList())
         {
            if(current_stmt->index == def_stmt->index)
            {
               return true;
            }
            if(current_stmt->index == use_stmt->index)
            {
               return false;
            }
         }
         return false;
      };
      const auto has_dependency_path = [&](const ir_nodeRef& src_stmt, const ir_nodeRef& dst_stmt) -> bool {
         const auto src_it = stmt_to_op.find(src_stmt->index);
         const auto dst_it = stmt_to_op.find(dst_stmt->index);
         if(src_it == stmt_to_op.end() || dst_it == stmt_to_op.end())
         {
            return true;
         }
         return dependence_graph.IsReachable(src_it->second, dst_it->second);
      };
      const auto replace_definition_uses = [&](const ir_nodeRef& old_def, const ir_nodeRef& new_def) {
         if(!old_def || !new_def || old_def->index == new_def->index)
         {
            return;
         }
         const auto old_ssa = GetPointerS<const ssa_node>(old_def);
         const auto stmt_uses = old_ssa->CGetUseStmts();
         for(const auto& use : stmt_uses)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---replace equivalent memory definition before: " + use.first->ToString());
            TM->ReplaceIRNode(use.first, old_def, new_def);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---replace equivalent memory definition after: " + use.first->ToString());
         }
      };
      std::function<bool(const ir_nodeRef&, const ir_nodeRef&, IRNodeSet&)> ensure_available_before =
          [&](const ir_nodeRef& value, const ir_nodeRef& use_stmt, IRNodeSet& visiting) -> bool {
         if(is_available_before(value, use_stmt))
         {
            return true;
         }
         if(!value || value->get_kind() != ssa_node_K || visiting.find(value) != visiting.end())
         {
            return false;
         }
         visiting.insert(value);

         const auto value_ssa = GetPointerS<const ssa_node>(value);
         const auto def_stmt = value_ssa->GetDefStmt();
         const auto def_ga = GetPointer<const assign_stmt>(def_stmt);
         if(!def_ga || GetPointerS<const node_stmt>(def_stmt)->bb_index != B->number || def_ga->vdef ||
            def_ga->memdef || def_ga->memuse || !def_ga->vuses.empty() || !def_ga->vovers.empty() ||
            def_ga->op1->get_kind() == call_node_K || def_ga->op1->get_kind() == mem_access_node_K)
         {
            visiting.erase(value);
            return false;
         }

         const auto uses = ir_helper::ComputeSsaUses(def_stmt);
         for(const auto& use : uses)
         {
            if(use.first->index == value->index)
            {
               continue;
            }
            if(!ensure_available_before(use.first, use_stmt, visiting))
            {
               visiting.erase(value);
               return false;
            }
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Hoisting predicate definition before merge point: " + def_stmt->ToString());
         B->RemoveStmt(def_stmt, AppM);
         B->PushBefore(def_stmt, use_stmt, AppM);
         if(schedule)
         {
            schedule->UpdateTime(def_stmt->index, true);
         }
         visiting.erase(value);
         return true;
      };
#if HAVE_ASSERTS
      const auto is_after_control_stmt = [&](const ir_nodeRef& target_stmt) -> bool {
         for(const auto& current_stmt : B->CGetStmtList())
         {
            if(current_stmt->index == target_stmt->index)
            {
               return false;
            }
            if(current_stmt->get_kind() == multi_way_if_stmt_K || current_stmt->get_kind() == return_stmt_K)
            {
               return true;
            }
         }
         return true;
      };
#endif
      for(const auto& stmt : B->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing " + stmt->ToString());
         if(!AppM->ApplyNewTransformation())
         {
            break;
         }
         const auto eq_tn = hash_check(stmt, bb, sl, unique_table);
         if(eq_tn)
         {
            const auto ref_ga = GetPointerS<assign_stmt>(eq_tn);
            const auto dead_ga = GetPointerS<const assign_stmt>(stmt);
            const auto ref_predicate = ref_ga->predicate;
            const auto dead_predicate = dead_ga->predicate;

            const bool is_predicated_load_merge =
                is_predicated_load(ref_ga) && is_predicated_load(dead_ga) && ref_ga->bb_index == dead_ga->bb_index;
            const bool is_predicated_store_merge =
                is_predicated_store(ref_ga) && is_predicated_store(dead_ga) && ref_ga->bb_index == dead_ga->bb_index;
            const auto ref_store_value = is_predicated_store_merge ? ref_ga->op1 : ir_nodeRef();
            const auto dead_store_value = is_predicated_store_merge ? dead_ga->op1 : ir_nodeRef();
            const bool same_store_value =
                is_predicated_store_merge && ref_store_value->index == dead_store_value->index;
            if(is_predicated_load_merge || is_predicated_store_merge)
            {
               THROW_ASSERT(!is_after_control_stmt(eq_tn),
                            "Unexpected predicated memory access after a control statement: " + eq_tn->ToString());
               THROW_ASSERT(!is_after_control_stmt(stmt),
                            "Unexpected predicated memory access after a control statement: " + stmt->ToString());
               const auto has_dependence = has_dependency_path(eq_tn, stmt) || has_dependency_path(stmt, eq_tn);
               const bool requires_mutual_exclusion =
                   is_predicated_load_merge ? true : (has_dependence || !same_store_value);
               if(requires_mutual_exclusion && !predicates_are_mutually_exclusive(ref_predicate, dead_predicate))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Predicated memory accesses not merged because the required predicate mutual "
                                 "exclusion was not proven");
                  continue;
               }
               IRNodeSet visiting;
               if(ref_predicate->index != dead_predicate->index &&
                  (!ensure_available_before(ref_predicate, eq_tn, visiting) ||
                   !ensure_available_before(dead_predicate, eq_tn, visiting)))
               {
                  INDENT_DBG_MEX(
                      DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                      "---Predicated memory accesses not merged because merged predicate would use a future SSA");
                  continue;
               }
               if(is_predicated_store_merge)
               {
                  if(!same_store_value && !ensure_available_before(dead_store_value, eq_tn, visiting))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Predicated stores not merged because selected value would use a future SSA");
                     continue;
                  }
                  if(!same_store_value)
                  {
                     const auto store_type = ir_helper::CGetType(ref_store_value);
                     const auto select_expr = IRman->create_ternary_operation(
                         store_type, ref_predicate, ref_store_value, dead_store_value, BUILTIN_LOCINFO, select_node_K);
                     const auto select_stmt = IRman->CreateAssignStmt(store_type, ir_nodeConstRef(), ir_nodeConstRef(),
                                                                      select_expr, function_id, BUILTIN_LOCINFO);
                     B->PushBefore(select_stmt, eq_tn, AppM);
                     const auto selected_store_value = GetPointerS<const assign_stmt>(select_stmt)->op0;
                     TM->RecursiveReplaceIRNode(ref_ga->op1, ref_store_value, selected_store_value, eq_tn, false, true);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Created selected store value: (" + STR(ref_predicate) + " ? " +
                                        STR(ref_store_value) + " : " + STR(dead_store_value) + ")");
                     if(schedule)
                     {
                        schedule->UpdateTime(select_stmt->index, true);
                        schedule->UpdateTime(eq_tn->index);
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Merging predicated stores: OR(" + STR(ref_predicate) + ", " + STR(dead_predicate) +
                                     ")");
                  if(ref_predicate->index != dead_predicate->index)
                  {
                     const auto or_pred = IRman->CreateOrExpr(ref_predicate, dead_predicate, nullptr, function_id);
                     const auto or_stmt = GetPointerS<const ssa_node>(or_pred)->GetDefStmt();
                     B->PushBefore(or_stmt, eq_tn, AppM);
                     TM->ReplaceIRNode(eq_tn, ref_predicate, or_pred);
                     reorder_luts = true;
                     restart_lut_opt = true;
                     if(schedule)
                     {
                        schedule->UpdateTime(or_stmt->index, true);
                        schedule->UpdateTime(eq_tn->index);
                     }
                  }
                  ref_ga->temporary_address = ref_ga->temporary_address && dead_ga->temporary_address;
                  replace_definition_uses(dead_ga->memdef, ref_ga->memdef);
                  replace_definition_uses(dead_ga->vdef, ref_ga->vdef);
                  to_be_removed.insert(stmt);

                  AppM->RegisterTransformation(GetName(), stmt);
                  IR_changed = true;
#ifndef NDEBUG
                  ++n_equiv_stmt;
#endif
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Updated/Removed duplicated predicated store");
                  continue;
               }
            }

            const auto ref_ssa = GetPointerS<ssa_node>(ref_ga->op0);
            const auto dead_ssa = GetPointerS<const ssa_node>(dead_ga->op0);

            if(ref_ssa->min)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---ref_min=" + STR(ref_ssa->min));
            }
            if(ref_ssa->max)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---ref_max=" + STR(ref_ssa->max));
            }
            if(dead_ssa->min)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dead_min=" + STR(dead_ssa->min));
            }
            if(dead_ssa->max)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dead_max=" + STR(dead_ssa->max));
            }

            if(!ref_ssa->bit_values.empty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---ref_bit_values=" + ref_ssa->bit_values);
            }
            if(!dead_ssa->bit_values.empty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---dead_bit_values=" + dead_ssa->bit_values);
            }

            bool same_range = dead_ssa->bit_values.empty() || ref_ssa->bit_values == dead_ssa->bit_values;

            if(same_range)
            {
               if(is_predicated_load_merge && ref_ga->predicate->index != dead_ga->predicate->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Merging predicated loads: OR(" + STR(ref_ga->predicate) + ", " +
                                     STR(dead_ga->predicate) + ")");
                  const auto or_pred = IRman->CreateOrExpr(ref_ga->predicate, dead_ga->predicate, nullptr, function_id);
                  const auto or_stmt = GetPointerS<const ssa_node>(or_pred)->GetDefStmt();
                  B->PushBefore(or_stmt, eq_tn, AppM);
                  TM->ReplaceIRNode(eq_tn, ref_ga->predicate, or_pred);
                  reorder_luts = true;
                  restart_lut_opt = true;
                  if(schedule)
                  {
                     schedule->UpdateTime(or_stmt->index, true);
                     schedule->UpdateTime(eq_tn->index);
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updating/Removing " + STR(dead_ga->op0));
               ref_ga->temporary_address = ref_ga->temporary_address && dead_ga->temporary_address;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---ref_ga->temporary_address" +
                                  (ref_ga->temporary_address ? std::string("T") : std::string("F")));
               ref_ssa->use_set = dead_ssa->use_set;

               const auto StmtUses = dead_ssa->CGetUseStmts();
               for(const auto& use : StmtUses)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---replace equivalent statement before: " + use.first->ToString());
                  TM->ReplaceIRNode(use.first, dead_ga->op0, ref_ga->op0);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---replace equivalent statement after: " + use.first->ToString());
               }
               to_be_removed.insert(stmt);

               AppM->RegisterTransformation(GetName(), stmt);
               IR_changed = true;
#ifndef NDEBUG
               ++n_equiv_stmt;
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Updated/Removed duplicated statement " + STR(dead_ga->op0));
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---not the same range");
            }
         }
      }

      // ==============================================================
      // Second pass: merge pairs of predicated loads/stores that have
      // mutually exclusive predicates and the same base variable but
      // different address pointers.
      //   if(pred1) a1 = *b1;  if(pred2) a2 = *b2;   (same base(b1)==base(b2))
      //   →  tmp = pred1 ? b1 : b2;  if(pred1|pred2) a1 = *tmp;
      // ==============================================================
      {
         struct AccessInfo
         {
            ir_nodeRef stmt;
            ir_nodeRef ptr; /// pointer operand of the mem_access node
         };
         using AccessKey = std::pair<unsigned int /*type_idx*/, unsigned int /*base_var_idx*/>;
         std::map<AccessKey, std::vector<AccessInfo>> load_groups;
         std::map<AccessKey, std::vector<AccessInfo>> store_groups;

         for(const auto& stmt : B->CGetStmtList())
         {
            const auto* ga = GetPointer<const assign_stmt>(stmt);
            if(!ga)
            {
               continue;
            }
            if(is_predicated_load(ga))
            {
               THROW_ASSERT(!is_after_control_stmt(stmt),
                            "Unexpected predicated load after a control statement: " + stmt->ToString());
               const auto ptr = GetPointerS<const mem_access_node>(ga->op1)->op;
               const auto base = ir_helper::GetBaseVariable(ptr);
               if(!base)
               {
                  continue;
               }
               load_groups[{ir_helper::CGetType(ga->op0)->index, base->index}].push_back({stmt, ptr});
            }
            else if(is_predicated_store(ga))
            {
               THROW_ASSERT(!is_after_control_stmt(stmt),
                            "Unexpected predicated store after a control statement: " + stmt->ToString());
               const auto ptr = GetPointerS<const mem_access_node>(ga->op0)->op;
               const auto base = ir_helper::GetBaseVariable(ptr);
               if(!base)
               {
                  continue;
               }
               store_groups[{ir_helper::CGetType(ga->op1)->index, base->index}].push_back({stmt, ptr});
            }
         }

         const auto try_merge_same_base_loads = [&](const AccessInfo& ref_access,
                                                    const AccessInfo& dead_access) -> bool {
            if(to_be_removed.count(ref_access.stmt) || to_be_removed.count(dead_access.stmt))
            {
               return false;
            }
            const auto ref_ptr = ref_access.ptr;
            const auto dead_ptr = dead_access.ptr;
            if(ref_ptr->index == dead_ptr->index)
            {
               return false; // same address — already handled by existing hash-based CSE
            }
            const auto ref_ga = GetPointerS<assign_stmt>(ref_access.stmt);
            const auto* dead_ga = GetPointerS<const assign_stmt>(dead_access.stmt);
            const auto ref_pred = ref_ga->predicate;
            const auto dead_pred = dead_ga->predicate;
            if(!predicates_are_mutually_exclusive(ref_pred, dead_pred))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Same-base loads not merged: predicates not mutually exclusive");
               return false;
            }
            // Ensure range information is compatible
            const auto* ref_ssa = GetPointerS<const ssa_node>(ref_ga->op0);
            const auto* dead_ssa = GetPointerS<const ssa_node>(dead_ga->op0);
            if(!dead_ssa->bit_values.empty() && ref_ssa->bit_values != dead_ssa->bit_values)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Same-base loads not merged: incompatible ranges");
               return false;
            }
            // Hoist required SSAs to before the reference load
            IRNodeSet visiting;
            if(!ensure_available_before(ref_pred, ref_access.stmt, visiting) ||
               !ensure_available_before(dead_pred, ref_access.stmt, visiting) ||
               !ensure_available_before(dead_ptr, ref_access.stmt, visiting))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Same-base loads not merged: cannot hoist required SSAs");
               return false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Merging same-base predicated loads: select(" + STR(ref_ptr) + ", " + STR(dead_ptr) +
                               ")");
            // Create address select: merged_ptr = ref_pred ? ref_ptr : dead_ptr
            const auto ptr_type = ir_helper::CGetType(ref_ptr);
            const auto select_ptr_expr =
                IRman->create_ternary_operation(ptr_type, ref_pred, ref_ptr, dead_ptr, BUILTIN_LOCINFO, select_node_K);
            const auto select_ptr_stmt = IRman->CreateAssignStmt(ptr_type, ir_nodeConstRef(), ir_nodeConstRef(),
                                                                 select_ptr_expr, function_id, BUILTIN_LOCINFO);
            B->PushBefore(select_ptr_stmt, ref_access.stmt, AppM);
            const auto merged_ptr = GetPointerS<const assign_stmt>(select_ptr_stmt)->op0;
            if(schedule)
            {
               schedule->UpdateTime(select_ptr_stmt->index, true);
            }
            // Replace the original pointer in the ref load with the merged (selected) pointer
            TM->ReplaceIRNode(ref_access.stmt, ref_ptr, merged_ptr);
            // OR the predicates
            if(ref_pred->index != dead_pred->index)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---OR-ing predicates: OR(" + STR(ref_pred) + ", " + STR(dead_pred) + ")");
               const auto or_pred = IRman->CreateOrExpr(ref_pred, dead_pred, nullptr, function_id);
               const auto or_stmt = GetPointerS<const ssa_node>(or_pred)->GetDefStmt();
               B->PushBefore(or_stmt, ref_access.stmt, AppM);
               TM->ReplaceIRNode(ref_access.stmt, ref_pred, or_pred);
               reorder_luts = true;
               restart_lut_opt = true;
               if(schedule)
               {
                  schedule->UpdateTime(or_stmt->index, true);
               }
            }
            if(schedule)
            {
               schedule->UpdateTime(ref_access.stmt->index);
            }
            ref_ga->temporary_address = ref_ga->temporary_address && dead_ga->temporary_address;
            // Replace all uses of the dead load result with the ref load result
            const auto StmtUses = dead_ssa->CGetUseStmts();
            for(const auto& use : StmtUses)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---replace same-base load result before: " + use.first->ToString());
               TM->ReplaceIRNode(use.first, dead_ga->op0, ref_ga->op0);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---replace same-base load result after: " + use.first->ToString());
            }
            to_be_removed.insert(dead_access.stmt);
            AppM->RegisterTransformation(GetName(), dead_access.stmt);
            IR_changed = true;
#ifndef NDEBUG
            ++n_equiv_stmt;
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Merged same-base predicated loads: " + STR(dead_ga->op0) + " -> " + STR(ref_ga->op0));
            return true;
         };

         const auto try_merge_same_base_stores = [&](const AccessInfo& ref_access,
                                                     const AccessInfo& dead_access) -> bool {
            if(to_be_removed.count(ref_access.stmt) || to_be_removed.count(dead_access.stmt))
            {
               return false;
            }
            const auto ref_ptr = ref_access.ptr;
            const auto dead_ptr = dead_access.ptr;
            if(ref_ptr->index == dead_ptr->index)
            {
               return false; // same address — already handled by existing hash-based CSE
            }
            const auto ref_ga = GetPointerS<assign_stmt>(ref_access.stmt);
            const auto* dead_ga = GetPointerS<const assign_stmt>(dead_access.stmt);
            const auto ref_pred = ref_ga->predicate;
            const auto dead_pred = dead_ga->predicate;
            // Different-address store merge always requires mutual exclusion
            if(!predicates_are_mutually_exclusive(ref_pred, dead_pred))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Same-base stores not merged: predicates not mutually exclusive");
               return false;
            }
            const auto ref_val = ref_ga->op1;
            const auto dead_val = dead_ga->op1;
            const bool same_val = ref_val->index == dead_val->index;
            // Hoist required SSAs to before the reference store
            IRNodeSet visiting;
            if(!ensure_available_before(ref_pred, ref_access.stmt, visiting) ||
               !ensure_available_before(dead_pred, ref_access.stmt, visiting) ||
               !ensure_available_before(dead_ptr, ref_access.stmt, visiting))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Same-base stores not merged: cannot hoist address/predicate");
               return false;
            }
            if(!same_val && !ensure_available_before(dead_val, ref_access.stmt, visiting))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Same-base stores not merged: cannot hoist store value");
               return false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Merging same-base predicated stores: select(" + STR(ref_ptr) + ", " + STR(dead_ptr) +
                               ")");
            // Create address select: merged_ptr = ref_pred ? ref_ptr : dead_ptr
            const auto ptr_type = ir_helper::CGetType(ref_ptr);
            const auto select_ptr_expr =
                IRman->create_ternary_operation(ptr_type, ref_pred, ref_ptr, dead_ptr, BUILTIN_LOCINFO, select_node_K);
            const auto select_ptr_stmt = IRman->CreateAssignStmt(ptr_type, ir_nodeConstRef(), ir_nodeConstRef(),
                                                                 select_ptr_expr, function_id, BUILTIN_LOCINFO);
            B->PushBefore(select_ptr_stmt, ref_access.stmt, AppM);
            const auto merged_ptr = GetPointerS<const assign_stmt>(select_ptr_stmt)->op0;
            if(schedule)
            {
               schedule->UpdateTime(select_ptr_stmt->index, true);
            }
            // Replace the original pointer in the ref store's mem_access with the merged pointer
            TM->ReplaceIRNode(ref_access.stmt, ref_ptr, merged_ptr);
            // Create a value select when the two stores write different values
            if(!same_val)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Created selected store value: (" + STR(ref_pred) + " ? " + STR(ref_val) + " : " +
                                  STR(dead_val) + ")");
               const auto val_type = ir_helper::CGetType(ref_val);
               const auto select_val_expr = IRman->create_ternary_operation(val_type, ref_pred, ref_val, dead_val,
                                                                            BUILTIN_LOCINFO, select_node_K);
               const auto select_val_stmt = IRman->CreateAssignStmt(val_type, ir_nodeConstRef(), ir_nodeConstRef(),
                                                                    select_val_expr, function_id, BUILTIN_LOCINFO);
               B->PushBefore(select_val_stmt, ref_access.stmt, AppM);
               const auto merged_val = GetPointerS<const assign_stmt>(select_val_stmt)->op0;
               TM->RecursiveReplaceIRNode(ref_ga->op1, ref_val, merged_val, ref_access.stmt, false, true);
               if(schedule)
               {
                  schedule->UpdateTime(select_val_stmt->index, true);
               }
            }
            // OR the predicates
            if(ref_pred->index != dead_pred->index)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---OR-ing predicates: OR(" + STR(ref_pred) + ", " + STR(dead_pred) + ")");
               const auto or_pred = IRman->CreateOrExpr(ref_pred, dead_pred, nullptr, function_id);
               const auto or_stmt = GetPointerS<const ssa_node>(or_pred)->GetDefStmt();
               B->PushBefore(or_stmt, ref_access.stmt, AppM);
               TM->ReplaceIRNode(ref_access.stmt, ref_pred, or_pred);
               reorder_luts = true;
               restart_lut_opt = true;
               if(schedule)
               {
                  schedule->UpdateTime(or_stmt->index, true);
               }
            }
            if(schedule)
            {
               schedule->UpdateTime(ref_access.stmt->index);
            }
            ref_ga->temporary_address = ref_ga->temporary_address && dead_ga->temporary_address;
            replace_definition_uses(dead_ga->memdef, ref_ga->memdef);
            replace_definition_uses(dead_ga->vdef, ref_ga->vdef);
            to_be_removed.insert(dead_access.stmt);
            AppM->RegisterTransformation(GetName(), dead_access.stmt);
            IR_changed = true;
#ifndef NDEBUG
            ++n_equiv_stmt;
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Merged same-base predicated stores");
            return true;
         };

         for(auto& [key, accesses] : load_groups)
         {
            if(accesses.size() < 2)
            {
               continue;
            }
            for(size_t i = 0; i + 1 < accesses.size() && AppM->ApplyNewTransformation(); ++i)
            {
               if(to_be_removed.count(accesses[i].stmt))
               {
                  continue;
               }
               for(size_t j = i + 1; j < accesses.size() && AppM->ApplyNewTransformation(); ++j)
               {
                  if(try_merge_same_base_loads(accesses[i], accesses[j]))
                  {
                     break; // ref merged once; next CSE pass handles remaining pairs
                  }
               }
            }
         }

         for(auto& [key, accesses] : store_groups)
         {
            if(accesses.size() < 2)
            {
               continue;
            }
            for(size_t i = 0; i + 1 < accesses.size() && AppM->ApplyNewTransformation(); ++i)
            {
               if(to_be_removed.count(accesses[i].stmt))
               {
                  continue;
               }
               for(size_t j = i + 1; j < accesses.size() && AppM->ApplyNewTransformation(); ++j)
               {
                  if(try_merge_same_base_stores(accesses[i], accesses[j]))
                  {
                     break;
                  }
               }
            }
         }
      }

      for(const auto& stmt : to_be_removed)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + stmt->ToString());
         B->RemoveStmt(stmt, AppM);
      }
      if(B->CGetStmtList().empty() && B->CGetPhiList().empty() && !to_be_removed.empty())
      {
         restart_phi_opt = true;
      }
      if(reorder_luts)
      {
         B->ReorderLUTs();
      }
      if((!to_be_removed.empty() || reorder_luts) && schedule)
      {
         for(const auto& stmt : B->CGetStmtList())
         {
            schedule->UpdateTime(stmt->index);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered BB" + STR(B->number));
   }

   // Cross-BB same-base predicated load merge.  This intentionally accepts only
   // cases where the selected address/predicate are already available in the
   // common dominator and where moving the surviving load does not carry virtual
   // memory dependencies across blocks.
   {
      struct CrossBBLoadInfo
      {
         blocRef block;
         ir_nodeRef stmt;
         ir_nodeRef ptr;
         ir_nodeRef predicate;
         bool inverted_predicate;
      };
      using AccessKey = std::pair<unsigned int /*type_idx*/, unsigned int /*base_var_idx*/>;
      std::map<AccessKey, std::vector<CrossBBLoadInfo>> load_groups;

      const auto get_edge_predicate = [&](const blocRef& block, ir_nodeRef& predicate, bool& inverted) -> bool {
         if(block->list_of_pred.size() != 1)
         {
            return false;
         }
         const auto pred_index = block->list_of_pred.front();
         if(pred_index == bloc::ENTRY_BLOCK_ID || !sl->list_of_bloc.count(pred_index))
         {
            return false;
         }
         const auto pred_block = sl->list_of_bloc.at(pred_index);
         if(pred_block->CGetStmtList().empty() || pred_block->CGetStmtList().back()->get_kind() != multi_way_if_stmt_K)
         {
            return false;
         }
         const auto* mwi = GetPointerS<const multi_way_if_stmt>(pred_block->CGetStmtList().back());
         size_t explicit_conditions = 0;
         ir_nodeRef default_cond;
         bool default_edge = false;
         for(const auto& [cond, succ] : mwi->list_of_cond)
         {
            if(cond)
            {
               ++explicit_conditions;
               default_cond = cond;
            }
            if(succ == block->number)
            {
               if(cond)
               {
                  predicate = cond;
                  inverted = false;
                  return true;
               }
               default_edge = true;
            }
         }
         if(default_edge && explicit_conditions == 1 && default_cond)
         {
            predicate = default_cond;
            inverted = true;
            return true;
         }
         return false;
      };

      for(const auto& [bb_index, block] : sl->list_of_bloc)
      {
         if(bb_index == bloc::ENTRY_BLOCK_ID || bb_index == bloc::EXIT_BLOCK_ID)
         {
            continue;
         }
         for(const auto& stmt : block->CGetStmtList())
         {
            const auto* ga = GetPointer<const assign_stmt>(stmt);
            if(!ga || ga->op0->get_kind() != ssa_node_K || ga->op1->get_kind() != mem_access_node_K)
            {
               continue;
            }
            auto predicate = ga->predicate;
            bool inverted_predicate = false;
            if(!predicate || predicate->get_kind() == constant_int_val_node_K)
            {
               if(!get_edge_predicate(block, predicate, inverted_predicate))
               {
                  continue;
               }
            }
            const auto ptr = GetPointerS<const mem_access_node>(ga->op1)->op;
            const auto base = ir_helper::GetBaseVariable(ptr);
            if(!base)
            {
               continue;
            }
            load_groups[{ir_helper::CGetType(ga->op0)->index, base->index}].push_back(
                {block, stmt, ptr, predicate, inverted_predicate});
         }
      }

      const auto common_dominator = [&](const unsigned int first_bb, const unsigned int second_bb) {
         std::vector<BBGraph::vertex_descriptor> first_doms;
         auto current = inverse_vertex_map.at(first_bb);
         while(true)
         {
            first_doms.push_back(current);
            const auto idom = bb_dominators.getImmediateDominator(current);
            if(idom == current)
            {
               break;
            }
            current = idom;
         }

         current = inverse_vertex_map.at(second_bb);
         while(true)
         {
            if(std::find(first_doms.begin(), first_doms.end(), current) != first_doms.end())
            {
               return current;
            }
            const auto idom = bb_dominators.getImmediateDominator(current);
            if(idom == current)
            {
               return current;
            }
            current = idom;
         }
      };

      const auto is_available_before_stmt = [&](const ir_nodeRef& value, const blocRef& block,
                                                const ir_nodeRef& anchor_stmt) -> bool {
         if(!value || value->get_kind() != ssa_node_K)
         {
            return true;
         }

         const auto value_ssa = GetPointerS<const ssa_node>(value);
         if(value_ssa->virtual_flag)
         {
            return true;
         }

         const auto def_stmt = value_ssa->GetDefStmt();
         if(!def_stmt || def_stmt->get_kind() == phi_stmt_K)
         {
            return true;
         }

         const auto def_bb_index = GetPointerS<const node_stmt>(def_stmt)->bb_index;
         if(def_bb_index != block->number)
         {
            return common_dominator(def_bb_index, block->number) == inverse_vertex_map.at(def_bb_index);
         }

         if(!anchor_stmt)
         {
            return true;
         }
         for(const auto& current_stmt : block->CGetStmtList())
         {
            if(current_stmt->index == def_stmt->index)
            {
               return true;
            }
            if(current_stmt->index == anchor_stmt->index)
            {
               return false;
            }
         }
         return false;
      };

      const auto has_memory_before = [](const CrossBBLoadInfo& access) -> bool {
         for(const auto& stmt : access.block->CGetStmtList())
         {
            if(stmt->index == access.stmt->index)
            {
               return false;
            }
            const auto* ga = GetPointer<const assign_stmt>(stmt);
            if(stmt->get_kind() == call_stmt_K ||
               (ga && (ga->op1->get_kind() == call_node_K || ga->op0->get_kind() == mem_access_node_K ||
                       ga->op1->get_kind() == mem_access_node_K)))
            {
               return true;
            }
         }
         return true;
      };
#if HAVE_ASSERTS
      const auto is_after_control_stmt_in_block = [](const CrossBBLoadInfo& access) -> bool {
         for(const auto& stmt : access.block->CGetStmtList())
         {
            if(stmt->index == access.stmt->index)
            {
               return false;
            }
            if(stmt->get_kind() == multi_way_if_stmt_K || stmt->get_kind() == return_stmt_K)
            {
               return true;
            }
         }
         return true;
      };
#endif
      const auto has_side_effecting_load_use = [](const ssa_node* loaded_value) -> bool {
         for(const auto& [use_stmt, uses] : loaded_value->CGetUseStmts())
         {
            if(use_stmt->get_kind() == call_stmt_K)
            {
               return true;
            }
            const auto* use_assign = GetPointer<const assign_stmt>(use_stmt);
            if(use_assign &&
               (use_assign->op0->get_kind() == mem_access_node_K || use_assign->op1->get_kind() == call_node_K))
            {
               return true;
            }
         }
         return false;
      };

      const auto incompatible_virtual_memory = [](const assign_stmt* first, const assign_stmt* second) {
         if(first->memuse || second->memuse || first->memdef || second->memdef || first->vdef || second->vdef ||
            !first->vovers.empty() || !second->vovers.empty() || first->vuses.size() != second->vuses.size())
         {
            return true;
         }
         auto first_it = first->vuses.begin();
         auto second_it = second->vuses.begin();
         for(; first_it != first->vuses.end(); ++first_it, ++second_it)
         {
            if((*first_it)->index != (*second_it)->index)
            {
               return true;
            }
         }
         return false;
      };

      const auto try_merge_cross_bb_loads = [&](const CrossBBLoadInfo& ref_access,
                                                const CrossBBLoadInfo& dead_access) -> bool {
         if(ref_access.block->number == dead_access.block->number || ref_access.ptr->index == dead_access.ptr->index)
         {
            return false;
         }

         const auto ref_ga = GetPointerS<assign_stmt>(ref_access.stmt);
         const auto dead_ga = GetPointerS<const assign_stmt>(dead_access.stmt);
         if(incompatible_virtual_memory(ref_ga, dead_ga))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Cross-BB same-base loads not merged: incompatible virtual memory dependencies");
            return false;
         }

         const auto ref_pred_source = ref_access.predicate;
         const auto dead_pred_source = dead_access.predicate;
         const bool opposite_predicates = ref_pred_source->index == dead_pred_source->index &&
                                          ref_access.inverted_predicate != dead_access.inverted_predicate;
         if(!opposite_predicates && (ref_access.inverted_predicate || dead_access.inverted_predicate ||
                                     !predicates_are_mutually_exclusive(ref_pred_source, dead_pred_source)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Cross-BB same-base loads not merged: predicates not mutually exclusive");
            return false;
         }

         const auto* ref_ssa = GetPointerS<const ssa_node>(ref_ga->op0);
         const auto* dead_ssa = GetPointerS<const ssa_node>(dead_ga->op0);
         if(!dead_ssa->bit_values.empty() && ref_ssa->bit_values != dead_ssa->bit_values)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Cross-BB same-base loads not merged: incompatible ranges");
            return false;
         }
         if(has_side_effecting_load_use(ref_ssa) || has_side_effecting_load_use(dead_ssa))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Cross-BB same-base loads not merged: side-effecting load use");
            return false;
         }

         if(has_memory_before(ref_access) || has_memory_before(dead_access))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Cross-BB same-base loads not merged: branch-local memory before load");
            return false;
         }
         THROW_ASSERT(!is_after_control_stmt_in_block(ref_access),
                      "Unexpected cross-BB load after a control statement: " + ref_access.stmt->ToString());
         THROW_ASSERT(!is_after_control_stmt_in_block(dead_access),
                      "Unexpected cross-BB load after a control statement: " + dead_access.stmt->ToString());

         const auto common_dom = common_dominator(ref_access.block->number, dead_access.block->number);
         const auto common_block = dt.CGetNodeInfo(common_dom).block;
         if(common_block->number == bloc::ENTRY_BLOCK_ID || common_block->number == bloc::EXIT_BLOCK_ID ||
            common_block->number == ref_access.block->number || common_block->number == dead_access.block->number)
         {
            return false;
         }

         const auto anchor_stmt =
             common_block->CGetStmtList().empty() ? ir_nodeRef() : common_block->CGetStmtList().back();
         if(!is_available_before_stmt(ref_pred_source, common_block, anchor_stmt) ||
            !is_available_before_stmt(dead_pred_source, common_block, anchor_stmt) ||
            !is_available_before_stmt(ref_access.ptr, common_block, anchor_stmt) ||
            !is_available_before_stmt(dead_access.ptr, common_block, anchor_stmt))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Cross-BB same-base loads not merged: operands unavailable in common dominator");
            return false;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Merging cross-BB same-base predicated loads in BB" + STR(common_block->number));
         const auto push_before_anchor = [&](const ir_nodeRef& stmt) {
            if(anchor_stmt)
            {
               common_block->PushBefore(stmt, anchor_stmt, AppM);
            }
            else
            {
               common_block->PushBack(stmt, AppM);
            }
         };
         const auto materialize_predicate = [&](const CrossBBLoadInfo& access) -> ir_nodeRef {
            if(!access.inverted_predicate)
            {
               return access.predicate;
            }
            const auto not_pred = IRman->CreateNotExpr(access.predicate, nullptr, function_id);
            push_before_anchor(GetPointerS<const ssa_node>(not_pred)->GetDefStmt());
            if(schedule)
            {
               schedule->UpdateTime(GetPointerS<const ssa_node>(not_pred)->GetDefStmt()->index, true);
            }
            return not_pred;
         };
         const auto ref_pred = materialize_predicate(ref_access);
         const auto dead_pred = materialize_predicate(dead_access);
         const auto ptr_type = ir_helper::CGetType(ref_access.ptr);
         const auto select_ptr_expr = IRman->create_ternary_operation(ptr_type, ref_pred, ref_access.ptr,
                                                                      dead_access.ptr, BUILTIN_LOCINFO, select_node_K);
         const auto select_ptr_stmt = IRman->CreateAssignStmt(ptr_type, ir_nodeConstRef(), ir_nodeConstRef(),
                                                              select_ptr_expr, function_id, BUILTIN_LOCINFO);
         push_before_anchor(select_ptr_stmt);
         const auto merged_ptr = GetPointerS<const assign_stmt>(select_ptr_stmt)->op0;

         const auto or_pred = IRman->CreateOrExpr(ref_pred, dead_pred, nullptr, function_id);
         const auto or_stmt = GetPointerS<const ssa_node>(or_pred)->GetDefStmt();
         push_before_anchor(or_stmt);

         TM->ReplaceIRNode(ref_access.stmt, ref_access.ptr, merged_ptr);
         if(ref_ga->predicate)
         {
            TM->ReplaceIRNode(ref_access.stmt, ref_ga->predicate, or_pred);
         }
         else
         {
            ref_ga->predicate = or_pred;
         }

         ref_access.block->RemoveStmt(ref_access.stmt, AppM);
         push_before_anchor(ref_access.stmt);

         ref_ga->temporary_address = ref_ga->temporary_address && dead_ga->temporary_address;
         const auto stmt_uses = dead_ssa->CGetUseStmts();
         for(const auto& use : stmt_uses)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---replace cross-BB same-base load result before: " + use.first->ToString());
            TM->ReplaceIRNode(use.first, dead_ga->op0, ref_ga->op0);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---replace cross-BB same-base load result after: " + use.first->ToString());
         }
         dead_access.block->RemoveStmt(dead_access.stmt, AppM);

         AppM->RegisterTransformation(GetName(), dead_access.stmt);
         restart_lut_opt = true;
         IR_changed = true;
#ifndef NDEBUG
         ++n_equiv_stmt;
#endif
         if(schedule)
         {
            schedule->UpdateTime(select_ptr_stmt->index, true);
            schedule->UpdateTime(or_stmt->index, true);
            schedule->UpdateTime(ref_access.stmt->index);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Merged cross-BB same-base predicated loads");
         return true;
      };

      bool cross_bb_reorder_luts = false;
      for(auto& [key, accesses] : load_groups)
      {
         if(accesses.size() < 2)
         {
            continue;
         }
         for(size_t i = 0; i + 1 < accesses.size() && AppM->ApplyNewTransformation(); ++i)
         {
            for(size_t j = i + 1; j < accesses.size() && AppM->ApplyNewTransformation(); ++j)
            {
               if(try_merge_cross_bb_loads(accesses[i], accesses[j]))
               {
                  cross_bb_reorder_luts = true;
                  break;
               }
            }
         }
      }

      if(cross_bb_reorder_luts)
      {
         for(const auto& [bb_index, block] : sl->list_of_bloc)
         {
            if(bb_index != bloc::ENTRY_BLOCK_ID && bb_index != bloc::EXIT_BLOCK_ID)
            {
               block->ReorderLUTs();
               if(schedule)
               {
                  for(const auto& stmt : block->CGetStmtList())
                  {
                     schedule->UpdateTime(stmt->index);
                  }
               }
            }
         }
      }
   }
   if(!IR_changed)
   {
      restart_phi_opt = false;
      restart_lut_opt = false;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---CSE: number of equivalent statement = " + STR(n_equiv_stmt));
   IR_changed ? function_behavior->UpdateBBVersion() : 0;
   return IR_changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool CSE::has_memory_access(const assign_stmt* ga) const
{
   const auto& fun_mem_data = function_behavior->get_function_mem();
   const auto rhs_kind = ga->op1->get_kind();
   const auto op0_type = ir_helper::CGetType(ga->op0);
   const auto op1_type = ir_helper::CGetType(ga->op1);

   bool skip_check = rhs_kind == variable_val_node_K || rhs_kind == constructor_node_K ||
                     rhs_kind == unaligned_mem_access_node_K || rhs_kind == mem_access_node_K;
   if(rhs_kind == bitcast_node_K)
   {
      const auto bitcast_expr = GetPointerS<const bitcast_node>(ga->op1);
      const auto bitcast_op_type = ir_helper::CGetType(bitcast_expr->op);
      if(op0_type->get_kind() == struct_ty_node_K)
      {
         skip_check = true;
      }
      if(bitcast_op_type->get_kind() == struct_ty_node_K)
      {
         skip_check = true;
      }

      if(bitcast_op_type->get_kind() == array_ty_node_K && op0_type->get_kind() == vector_ty_node_K)
      {
         skip_check = true;
      }
      if(bitcast_op_type->get_kind() == vector_ty_node_K && op0_type->get_kind() == array_ty_node_K)
      {
         skip_check = true;
      }
   }
   if(!ir_helper::IsVectorType(ga->op0) && ir_helper::IsArrayEquivType(ga->op0) && !ir_helper::IsPointerType(ga->op0))
   {
      skip_check = true;
   }
   if(fun_mem_data.find(ga->op0->index) != fun_mem_data.end() ||
      fun_mem_data.find(ga->op1->index) != fun_mem_data.end())
   {
      skip_check = true;
   }
   if(op0_type && op1_type &&
      ((op0_type->get_kind() == struct_ty_node_K && op1_type->get_kind() == struct_ty_node_K &&
        rhs_kind != bitcast_node_K) ||
       (op0_type->get_kind() == array_ty_node_K)))
   {
      skip_check = true;
   }

   return skip_check;
}

ir_nodeRef CSE::hash_check(
    const ir_nodeRef& tn, BBGraph::vertex_descriptor bb_vertex, const statement_list_node* sl,
    std::map<BBGraph::vertex_descriptor, CustomUnorderedMapStable<CSE_tuple_key_type, ir_nodeRef>>& unique_table) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking: " + tn->ToString());
   if(GetPointer<const node_stmt>(tn)->keep)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null keep");
      return nullptr;
   }
   const auto ga = GetPointer<const assign_stmt>(tn);
   if(ga && ga->op0->get_kind() == ssa_node_K)
   {
      auto bitwidth_values = ir_helper::Size(ga->op0);
      const auto rhs = ga->op1;
      const auto rhs_kind = rhs->get_kind();
      if(GetPointer<const binary_node>(rhs))
      {
         bitwidth_values = std::max(bitwidth_values, ir_helper::Size(GetPointerS<const binary_node>(rhs)->op0));
      }
      if(rhs_kind != extract_bit_node_K && rhs_kind != lut_node_K && parameters->IsParameter("CSE_size") &&
         bitwidth_values < parameters->GetParameter<unsigned int>("CSE_size"))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: too small");
         return nullptr;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Right part type: " + rhs->get_kind_text());

      /// check for LOADs, STOREs, MEMSET, MEMCPY, etc. etc.
      if(has_memory_access(ga))
      {
         /// Allow predicated loads to be considered here; dependence safety is
         /// checked later before applying the merge.
         const bool is_predicated_load =
             rhs_kind == mem_access_node_K && ga->predicate && ga->predicate->get_kind() != constant_int_val_node_K;
         if(!is_predicated_load)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null memory");
            return nullptr;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Predicated load candidate for merging");
      }
      std::vector<unsigned int> ins;
      /// We add type of right part; load from same address with different types must be considered different
      ins.push_back(ir_helper::CGetType(ga->op1)->index);
      /// Predicated statements can be equivalent only within the same execution domain.
      /// Exception: predicated loads with identical operands can be merged by OR-ing predicates.
      const bool is_predicated_load =
          rhs_kind == mem_access_node_K && ga->predicate && ga->predicate->get_kind() != constant_int_val_node_K;
      if(is_predicated_load)
      {
         ins.push_back(ga->bb_index);
      }
      else if(ga->predicate)
      {
         ins.push_back(ga->predicate->index);
      }

      if(ga->vuses.size() && !is_predicated_load)
      {
         /// If there are virtual uses, not only they must be the same, but also the basic block must be the same
         ins.push_back(ga->bb_index);
         for(const auto& vuse : ga->vuses)
         {
            ins.push_back(vuse->index);

            /// Check if the virtual is defined in the same basic block
            const auto virtual_sn = GetPointerS<const ssa_node>(vuse);
            const auto virtual_sn_def = virtual_sn->GetDefStmt();
            const auto virtual_sn_gn = GetPointerS<const node_stmt>(virtual_sn_def);

            if(virtual_sn_gn->bb_index == ga->bb_index)
            {
               THROW_ASSERT(sl->list_of_bloc.count(ga->bb_index), "");
               const auto& bb = sl->list_of_bloc.at(ga->bb_index);
               const auto vdef_it =
                   virtual_sn_def->get_kind() == phi_stmt_K ?
                       bb->CGetStmtList().end() :
                       std::find_if(bb->CGetStmtList().begin(), bb->CGetStmtList().end(),
                                    [&](const ir_nodeRef& stmt) { return stmt->index == virtual_sn_gn->index; });
               const auto ga_it = std::find_if(vdef_it, bb->CGetStmtList().end(),
                                               [&](const ir_nodeRef& stmt) { return stmt->index == ga->index; });
               if(ga_it != bb->CGetStmtList().end())
               {
                  ins.push_back(virtual_sn_def->index);
               }
            }
         }
      }
      if(rhs_kind == ssa_node_K)
      {
         ins.push_back(rhs->index);
      }
      else if(GetPointer<const cst_node>(rhs))
      {
         ins.push_back(rhs->index);
      }
      else if(rhs_kind == nop_node_K || rhs_kind == bitcast_node_K || rhs_kind == itofp_node_K ||
              rhs_kind == fptoi_node_K)
      {
         const auto ue = GetPointerS<const unary_node>(rhs);
         ins.push_back(ue->op->index);
         const auto type_index = ir_helper::CGetType(ga->op0)->index;
         ins.push_back(type_index);
      }
      else if(GetPointer<const unary_node>(rhs))
      {
         const auto ue = GetPointerS<const unary_node>(rhs);
         ins.push_back(ue->op->index);
      }
      else if(GetPointer<const binary_node>(rhs))
      {
         const auto be = GetPointerS<const binary_node>(rhs);
         ins.push_back(be->op0->index);
         ins.push_back(be->op1->index);
      }
      else if(GetPointer<const ternary_node>(rhs))
      {
         const auto te = GetPointerS<const ternary_node>(rhs);
         ins.push_back(te->op0->index);
         ins.push_back(te->op1->index);
         if(te->op2)
         {
            ins.push_back(te->op2->index);
         }
      }
      else if(GetPointer<const call_node>(rhs))
      {
         const auto ce = GetPointerS<const call_node>(rhs);
         if(ce->fn->get_kind() == addr_node_K)
         {
            const auto addr_ref = ce->fn;
            const auto ae = GetPointerS<const addr_node>(addr_ref);
            ins.push_back(ae->op->index);
            const auto fd = GetPointerS<const function_val_node>(ae->op);
            // TODO: may be optimized
            if(!fd->body || fd->writing_memory || fd->reading_memory || ga->vuses.size())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null");
               return nullptr;
            }
            for(const auto& arg : ce->args)
            {
               ins.push_back(arg->index);
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null");
            return nullptr;
         }
      }
      else if(GetPointer<const lut_node>(rhs))
      {
         const auto le = GetPointerS<const lut_node>(rhs);
         ins.push_back(le->op0->index);
         ins.push_back(le->op1->index);
         if(le->op2)
         {
            ins.push_back(le->op2->index);
         }
         if(le->op3)
         {
            ins.push_back(le->op3->index);
         }
         if(le->op4)
         {
            ins.push_back(le->op4->index);
         }
         if(le->op5)
         {
            ins.push_back(le->op5->index);
         }
         if(le->op6)
         {
            ins.push_back(le->op6->index);
         }
         if(le->op7)
         {
            ins.push_back(le->op7->index);
         }
         if(le->op8)
         {
            ins.push_back(le->op8->index);
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null");
         return nullptr;
      }

#ifndef NDEBUG
      auto signature_message = "Signature of " + STR(tn->index) + " is ";
      for(const auto& temp_sign : ins)
      {
         signature_message += STR(temp_sign) + "-";
      }
      signature_message.pop_back();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, signature_message);
#endif
      CSE_tuple_key_type t(rhs_kind, ins);
      if(unique_table.at(bb_vertex).find(t) != unique_table.at(bb_vertex).end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- statement = " + tn->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "--- equivalent with = " + unique_table.at(bb_vertex).at(t)->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         THROW_ASSERT(!ga->memdef, "Unexpected memdef " + ga->memdef->ToString() + " in " + tn->ToString());
         THROW_ASSERT(!ga->vdef, "Unexpected vdef " + ga->vdef->ToString() + " in " + tn->ToString());
         THROW_ASSERT(ga->vovers.empty(), "Unexpected vovers in " + tn->ToString());
         return unique_table.at(bb_vertex).at(t);
      }
      else
      {
         unique_table.at(bb_vertex)[t] = tn;
      }
   }
   else if(ga && is_predicated_store(ga))
   {
      const auto store_addr = GetPointerS<const mem_access_node>(ga->op0)->op;
      std::vector<unsigned int> ins;
      /// Predicated stores are merge candidates when address and store types match in the same execution domain.
      ins.push_back(ir_helper::CGetType(ga->op0)->index);
      ins.push_back(ir_helper::CGetType(ga->op1)->index);
      ins.push_back(ga->bb_index);
      ins.push_back(store_addr->index);

#ifndef NDEBUG
      auto signature_message = "Signature of predicated store " + STR(tn->index) + " is ";
      for(const auto& temp_sign : ins)
      {
         signature_message += STR(temp_sign) + "-";
      }
      signature_message.pop_back();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, signature_message);
#endif
      CSE_tuple_key_type t(mem_access_node_K, ins);
      if(unique_table.at(bb_vertex).find(t) != unique_table.at(bb_vertex).end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- statement = " + tn->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "--- equivalent with = " + unique_table.at(bb_vertex).at(t)->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         return unique_table.at(bb_vertex).at(t);
      }
      unique_table.at(bb_vertex)[t] = tn;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked: null");
   return nullptr;
}
