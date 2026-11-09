/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2022-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file AXIBurstAnalysis.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "AXIBurstAnalysis.hpp"

#include "call_graph.hpp"
#include "dbgPrintHelper.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"

#include <algorithm>
#include <deque>
#include <iterator>
#include <map>
#include <set>
#include <utility>

using InterfaceInferInternal::OriginalMemoryAccess;

namespace
{
   bool isPlainSsaExpression(const ir_nodeRef& expression)
   {
      if(!expression)
      {
         return false;
      }
      switch(static_cast<unsigned int>(expression->get_kind()))
      {
         case ssa_node_K:
         case argument_val_node_K:
         case constant_int_val_node_K:
         case constant_fp_val_node_K:
         case constant_vector_val_node_K:
            return true;
         case CASE_UNARY_NODES:
         {
            if(expression->get_kind() == mem_access_node_K || expression->get_kind() == unaligned_mem_access_node_K)
            {
               return false;
            }
            if(expression->get_kind() == addr_node_K && GetPointerS<const unary_node>(expression)->op &&
               GetPointerS<const unary_node>(expression)->op->get_kind() == variable_val_node_K)
            {
               return true;
            }
            return isPlainSsaExpression(GetPointerS<const unary_node>(expression)->op);
         }
         case CASE_BINARY_NODES:
         {
            const auto binary = GetPointerS<const binary_node>(expression);
            // GEP is pure pointer arithmetic. Keep it in the same recursive
            // whitelist as integer arithmetic so ordinary address derivations
            // for read-only loads do not look like opaque effects.
            return isPlainSsaExpression(binary->op0) && isPlainSsaExpression(binary->op1);
         }
         case CASE_TERNARY_NODES:
         {
            const auto ternary = GetPointerS<const ternary_node>(expression);
            return isPlainSsaExpression(ternary->op0) && isPlainSsaExpression(ternary->op1) &&
                   isPlainSsaExpression(ternary->op2);
         }
         default:
            return false;
      }
   }

   bool isPreanalyzedCrossBundleWrite(const ir_nodeRef& stmt,
                                      const std::vector<OriginalMemoryAccess>& original_memory_accesses,
                                      const ir_nodeRef& expected_base, const std::string& expected_bundle)
   {
      if(!stmt || !expected_base)
      {
         return false;
      }
      const auto is_distinct_ordinary_write = [&](const OriginalMemoryAccess& access) {
         return access.is_write && access.classified && access.ordinary && access.base &&
                access.base->index != expected_base->index && access.bundle != expected_bundle;
      };
      if(stmt->get_kind() == assign_stmt_K &&
         std::any_of(original_memory_accesses.begin(), original_memory_accesses.end(), [&](const auto& access) {
            return is_distinct_ordinary_write(access) && access.statement && access.statement->index == stmt->index;
         }))
      {
         return true;
      }
      if(stmt->get_kind() != call_stmt_K)
      {
         return false;
      }
      auto callee = GetPointerS<const call_stmt>(stmt)->fn;
      if(callee && callee->get_kind() == addr_node_K)
      {
         callee = GetPointerS<const addr_node>(callee)->op;
      }
      if(!callee || ir_helper::GetFunctionName(callee).empty() || GetPointerS<const call_stmt>(stmt)->args.size() != 5)
      {
         return false;
      }
      const auto call_name = ir_helper::GetFunctionName(callee);
      return std::any_of(original_memory_accesses.begin(), original_memory_accesses.end(), [&](const auto& access) {
         return is_distinct_ordinary_write(access) && call_name == ENCODE_FDNAME(access.bundle, "_store_write", "");
      });
   }
} // namespace

namespace InterfaceInferInternal
{
   constexpr double EPSILON = 0.000000001;

   bool hasOnlyOrdinaryReadEffects(const statement_list_node* sl, const CustomOrderedSet<unsigned int>& function_memory)
   {
      if(!sl)
      {
         return false;
      }
      for(const auto& block : sl->list_of_bloc)
      {
         for(const auto& stmt : block.second->CGetStmtList())
         {
            const auto node = GetPointerS<const node_stmt>(stmt);
            if(node->memdef || node->vdef || !node->vovers.empty() || ir_helper::IsStore(stmt, function_memory))
            {
               return false;
            }
            const bool is_load = ir_helper::IsLoad(stmt, function_memory);
            if((node->memuse || !node->vuses.empty()) && !is_load)
            {
               return false;
            }
            switch(static_cast<unsigned int>(stmt->get_kind()))
            {
               case assign_stmt_K:
               {
                  const auto assign = GetPointerS<const assign_stmt>(stmt);
                  if(!assign->op0 || assign->op0->get_kind() != ssa_node_K)
                  {
                     return false;
                  }
                  if(is_load && assign->op1->get_kind() != mem_access_node_K)
                  {
                     return false;
                  }
                  if(is_load && !isPlainSsaExpression(GetPointerS<const mem_access_node>(assign->op1)->op))
                  {
                     return false;
                  }
                  if(!is_load && (assign->op1->get_kind() == mem_access_node_K ||
                                  assign->op1->get_kind() == unaligned_mem_access_node_K))
                  {
                     return false;
                  }
                  if(!is_load && !isPlainSsaExpression(assign->op1))
                  {
                     return false;
                  }
                  break;
               }
               case phi_stmt_K:
               {
                  const auto phi = GetPointerS<const phi_stmt>(stmt);
                  if(is_load || std::any_of(phi->CGetDefEdgesList().begin(), phi->CGetDefEdgesList().end(),
                                            [](const auto& edge) { return !isPlainSsaExpression(edge.first); }))
                  {
                     return false;
                  }
                  break;
               }
               case multi_way_if_stmt_K:
               {
                  const auto branch = GetPointerS<const multi_way_if_stmt>(stmt);
                  if(is_load ||
                     std::any_of(branch->list_of_cond.begin(), branch->list_of_cond.end(), [](const auto& condition) {
                        return condition.first && !isPlainSsaExpression(condition.first);
                     }))
                  {
                     return false;
                  }
                  break;
               }
               case nop_stmt_K:
                  if(is_load)
                  {
                     return false;
                  }
                  break;
               case return_stmt_K:
               {
                  const auto ret = GetPointerS<const return_stmt>(stmt);
                  if(is_load || (ret->op && !isPlainSsaExpression(ret->op)))
                  {
                     return false;
                  }
                  break;
               }
               default:
                  // Calls, inline assembly, and any statement kind not explicitly
                  // classified here may have effects that invalidate prefetching.
                  return false;
            }
         }
      }
      return true;
   }

   bool hasOnlyOrdinaryMemoryEffects(const statement_list_node* sl)
   {
      if(!sl)
      {
         return false;
      }
      for(const auto& block : sl->list_of_bloc)
      {
         for(const auto& stmt : block.second->CGetStmtList())
         {
            const auto node = GetPointerS<const node_stmt>(stmt);
            const bool is_store = stmt->get_kind() == assign_stmt_K && GetPointerS<const assign_stmt>(stmt)->op0 &&
                                  GetPointerS<const assign_stmt>(stmt)->op0->get_kind() == mem_access_node_K;
            if(!node->vovers.empty() || ((node->memdef || node->vdef) && !is_store))
            {
               return false;
            }
            if(stmt->get_kind() != assign_stmt_K && stmt->get_kind() != phi_stmt_K &&
               stmt->get_kind() != multi_way_if_stmt_K && stmt->get_kind() != nop_stmt_K &&
               stmt->get_kind() != return_stmt_K)
            {
               return false;
            }
            if(stmt->get_kind() == assign_stmt_K)
            {
               const auto assignment = GetPointerS<const assign_stmt>(stmt);
               const bool lhs_memory = assignment->op0 && assignment->op0->get_kind() == mem_access_node_K;
               const bool rhs_memory = assignment->op1 && assignment->op1->get_kind() == mem_access_node_K;
               if(lhs_memory)
               {
                  const auto access = GetPointerS<const mem_access_node>(assignment->op0);
                  if(!access->ordinary_memory_access || !isPlainSsaExpression(access->op) ||
                     !isPlainSsaExpression(assignment->op1))
                  {
                     return false;
                  }
               }
               else if(rhs_memory)
               {
                  const auto access = GetPointerS<const mem_access_node>(assignment->op1);
                  if(!access->ordinary_memory_access || !isPlainSsaExpression(access->op) ||
                     assignment->op0->get_kind() != ssa_node_K)
                  {
                     return false;
                  }
               }
               else if(!isPlainSsaExpression(assignment->op1))
               {
                  return false;
               }
            }
         }
      }
      return true;
   }

   bool isConstantValue(const ir_nodeRef& value, const long long expected)
   {
      return value && value->get_kind() == constant_int_val_node_K && ir_helper::GetConstValue(value) == expected;
   }

   bool isUnsigned32Type(const ir_nodeConstRef& type)
   {
      return type && type->get_kind() == integer_ty_node_K && ir_helper::Size(type) == 32 &&
             GetPointerS<const integer_ty_node>(type)->unsigned_flag;
   }

   bool hasBlockPath(const statement_list_node* sl, const unsigned int source, const unsigned int target)
   {
      std::vector<unsigned int> pending{source};
      std::set<unsigned int> visited{source};
      while(!pending.empty())
      {
         const auto current = pending.back();
         pending.pop_back();
         if(current == target)
         {
            return true;
         }
         const auto current_it = sl->list_of_bloc.find(current);
         if(current_it == sl->list_of_bloc.end())
         {
            continue;
         }
         for(const auto successor : current_it->second->list_of_succ)
         {
            if(visited.insert(successor).second)
            {
               pending.push_back(successor);
            }
         }
      }
      return false;
   }

   std::string get_decl_name(ir_nodeRef tn)
   {
      const auto dn = GetPointer<const decl_node>(tn);
      THROW_ASSERT(dn, "Expected decl_node.");
      THROW_ASSERT(dn->name, "Expected declaration name.");
      return GetPointer<const identifier_node>(dn->name)->strg;
   }

   namespace
   {
      bool isSigned32Type(const ir_nodeConstRef& type)
      {
         return type && type->get_kind() == integer_ty_node_K && ir_helper::Size(type) == 32 &&
                !GetPointerS<const integer_ty_node>(type)->unsigned_flag;
      }

      template <typename PredList, typename SuccList>
      bool findSingleBlockLoopEdges(const PredList& predecessors, const SuccList& successors,
                                    const unsigned int loop_id, unsigned int& preheader_id, unsigned int& exit_id)
      {
         if(successors.size() != 2 || predecessors.size() != 2 ||
            std::count(successors.begin(), successors.end(), loop_id) != 1 ||
            std::count(predecessors.begin(), predecessors.end(), loop_id) != 1)
         {
            return false;
         }
         const auto preheader = std::find_if(predecessors.begin(), predecessors.end(),
                                             [&](const auto predecessor) { return predecessor != loop_id; });
         const auto exit = std::find_if(successors.begin(), successors.end(),
                                        [&](const auto successor) { return successor != loop_id; });
         if(preheader == predecessors.end() || exit == successors.end())
         {
            return false;
         }
         preheader_id = *preheader;
         exit_id = *exit;
         return true;
      }

      bool recognizeZeroToPlusOneInduction(const ir_nodeRef& phi_node, const unsigned int preheader_id,
                                           const unsigned int backedge_id, ir_nodeRef& induction, ir_nodeRef& increment)
      {
         if(!phi_node || phi_node->get_kind() != phi_stmt_K)
         {
            return false;
         }
         const auto phi = GetPointerS<const phi_stmt>(phi_node);
         ir_nodeRef initial;
         ir_nodeRef backedge;
         for(const auto& edge : phi->CGetDefEdgesList())
         {
            if(edge.second == preheader_id)
            {
               initial = edge.first;
            }
            else if(edge.second == backedge_id)
            {
               backedge = edge.first;
            }
         }
         if(!isConstantValue(initial, 0) || !backedge || backedge->get_kind() != ssa_node_K)
         {
            return false;
         }
         const auto update_stmt = GetPointerS<const ssa_node>(backedge)->GetDefStmt();
         if(!update_stmt || update_stmt->get_kind() != assign_stmt_K ||
            GetPointerS<const assign_stmt>(update_stmt)->op1->get_kind() != add_node_K)
         {
            return false;
         }
         const auto add = GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(update_stmt)->op1);
         if((add->op0->index != phi->res->index || !isConstantValue(add->op1, 1)) &&
            (add->op1->index != phi->res->index || !isConstantValue(add->op0, 1)))
         {
            return false;
         }
         induction = phi->res;
         increment = backedge;
         return true;
      }

      bool findUnsigned32Bound(const binary_node* compare, const ir_nodeRef& increment, ir_nodeRef& bound)
      {
         if(compare->op0->index == increment->index)
         {
            bound = compare->op1;
         }
         else if(compare->op1->index == increment->index)
         {
            bound = compare->op0;
         }
         else
         {
            return false;
         }
         return bound && bound->get_kind() == ssa_node_K && isUnsigned32Type(ir_helper::CGetType(bound));
      }

      template <typename PhiList>
      bool recognizeCanonicalTripBound(const PhiList& phi_nodes, const unsigned int loop_id,
                                       const unsigned int preheader_id, const ir_nodeRef& loop_condition,
                                       ir_nodeRef& induction, ir_nodeRef& increment, ir_nodeRef& bound)
      {
         if(!loop_condition || loop_condition->get_kind() != ssa_node_K)
         {
            return false;
         }
         const auto condition_def = GetPointerS<const ssa_node>(loop_condition)->GetDefStmt();
         if(!condition_def || condition_def->get_kind() != assign_stmt_K ||
            GetPointerS<const assign_stmt>(condition_def)->op1->get_kind() != eq_node_K)
         {
            return false;
         }
         const auto compare = GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(condition_def)->op1);
         for(const auto& phi_node : phi_nodes)
         {
            if(phi_node->get_kind() == phi_stmt_K &&
               GetPointerS<const phi_stmt>(phi_node)->CGetDefEdgesList().size() == 2 &&
               recognizeZeroToPlusOneInduction(phi_node, preheader_id, loop_id, induction, increment))
            {
               break;
            }
         }
         if(!induction || !increment)
         {
            return false;
         }
         if(!findUnsigned32Bound(compare, increment, bound))
         {
            return false;
         }
         const auto bound_def = GetPointerS<const ssa_node>(bound)->GetDefStmt();
         return !bound_def || GetPointerS<const node_stmt>(bound_def)->bb_index != loop_id;
      }

      bool isPositiveBoundGuard(const ir_nodeRef& guard_condition, const ir_nodeRef& bound)
      {
         if(!guard_condition || guard_condition->get_kind() != ssa_node_K)
         {
            return false;
         }
         const auto guard_def = GetPointerS<const ssa_node>(guard_condition)->GetDefStmt();
         if(!guard_def || guard_def->get_kind() != assign_stmt_K ||
            GetPointerS<const assign_stmt>(guard_def)->op1->get_kind() != gt_node_K)
         {
            return false;
         }
         const auto guard_compare = GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(guard_def)->op1);
         if(!isConstantValue(guard_compare->op1, 0) || guard_compare->op0->get_kind() != ssa_node_K ||
            !ir_helper::IsSignedIntegerType(guard_compare->op0) || !ir_helper::IsSignedIntegerType(guard_compare->op1))
         {
            return false;
         }
         const auto signed_bound = GetPointerS<const ssa_node>(guard_compare->op0);
         const auto signed_bound_def = signed_bound->GetDefStmt();
         if(!signed_bound_def || signed_bound_def->get_kind() != assign_stmt_K ||
            GetPointerS<const assign_stmt>(signed_bound_def)->op1->get_kind() != nop_node_K)
         {
            return false;
         }
         const auto unsigned_bound =
             GetPointerS<const unary_node>(GetPointerS<const assign_stmt>(signed_bound_def)->op1)->op;
         return unsigned_bound->index == bound->index && isUnsigned32Type(ir_helper::CGetType(unsigned_bound)) &&
                isSigned32Type(ir_helper::CGetType(guard_compare->op0));
      }

      bool isFourByteIndexAddress(const ir_nodeRef& address, const ir_nodeRef& expected_base,
                                  const ir_nodeRef& induction)
      {
         const auto address_def = address && address->get_kind() == ssa_node_K ?
                                      GetPointerS<const ssa_node>(address)->GetDefStmt() :
                                      ir_nodeRef();
         if(!address_def || address_def->get_kind() != assign_stmt_K ||
            GetPointerS<const assign_stmt>(address_def)->op1->get_kind() != gep_node_K)
         {
            return false;
         }
         const auto gep = GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(address_def)->op1);
         if(gep->op0->index != expected_base->index || gep->op1->get_kind() != ssa_node_K)
         {
            return false;
         }
         const auto index_def = GetPointerS<const ssa_node>(gep->op1)->GetDefStmt();
         if(!index_def || index_def->get_kind() != assign_stmt_K)
         {
            return false;
         }
         const auto index_expr = GetPointerS<const assign_stmt>(index_def)->op1;
         if(index_expr->get_kind() != shl_node_K && index_expr->get_kind() != mul_node_K)
         {
            return false;
         }
         const auto scale = GetPointerS<const binary_node>(index_expr);
         return (scale->op0->index == induction->index &&
                 ((index_expr->get_kind() == shl_node_K && isConstantValue(scale->op1, 2)) ||
                  (index_expr->get_kind() != shl_node_K && isConstantValue(scale->op1, 4)))) ||
                (scale->op1->index == induction->index && index_expr->get_kind() != shl_node_K &&
                 isConstantValue(scale->op0, 4));
      }

      bool recognizePrefixDuplexRegions(const statement_list_node* sl, const ir_nodeRef& read_stmt,
                                        const ir_nodeRef& write_stmt, const ir_nodeRef& read_base,
                                        const ir_nodeRef& write_base, ReadBurstRegion& read_region,
                                        WriteBurstRegion& write_region, std::string& reason)
      {
         const auto read_node = GetPointerS<const node_stmt>(read_stmt);
         const auto write_node = GetPointerS<const node_stmt>(write_stmt);
         if(!read_node || !write_node || read_node->bb_index == 0 || write_node->bb_index == 0)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         const auto read_block = sl->list_of_bloc.find(read_node->bb_index);
         const auto write_block = sl->list_of_bloc.find(write_node->bb_index);
         if(read_block == sl->list_of_bloc.end() || write_block == sl->list_of_bloc.end() ||
            read_block->second->loop_id == 0 || read_block->second->loop_id != write_block->second->loop_id)
         {
            reason = "read/write accesses do not share one canonical loop";
            return false;
         }
         const auto loop_id = read_block->second->loop_id;
         std::set<unsigned int> loop_blocks;
         for(const auto& block : sl->list_of_bloc)
         {
            if(block.second->loop_id == loop_id)
            {
               loop_blocks.insert(block.first);
            }
         }
         if(loop_blocks.empty())
         {
            reason = "unsupported direction guard CFG";
            return false;
         }

         std::vector<unsigned int> headers;
         for(const auto block_id : loop_blocks)
         {
            const auto& block = sl->list_of_bloc.at(block_id);
            if(std::any_of(block->list_of_pred.begin(), block->list_of_pred.end(),
                           [&](const auto pred) { return !loop_blocks.count(pred); }))
            {
               headers.push_back(block_id);
            }
         }
         if(headers.size() != 1)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         const auto header = headers.front();
         const auto& header_block = sl->list_of_bloc.at(header);
         std::vector<unsigned int> preheaders;
         for(const auto pred : header_block->list_of_pred)
         {
            if(!loop_blocks.count(pred))
            {
               preheaders.push_back(pred);
            }
         }
         if(preheaders.size() != 1)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         const auto preheader = preheaders.front();
         std::vector<unsigned int> latches;
         std::set<unsigned int> exits;
         for(const auto block_id : loop_blocks)
         {
            const auto& block = sl->list_of_bloc.at(block_id);
            if(std::count(block->list_of_succ.begin(), block->list_of_succ.end(), header) != 0)
            {
               latches.push_back(block_id);
            }
            for(const auto succ : block->list_of_succ)
            {
               if(!loop_blocks.count(succ))
               {
                  exits.insert(succ);
               }
            }
         }
         if(latches.size() != 1 || exits.size() != 1)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         const auto latch = latches.front();
         const auto exit = *exits.begin();
         for(const auto block_id : loop_blocks)
         {
            const auto& block = sl->list_of_bloc.at(block_id);
            if(block_id != latch && std::any_of(block->list_of_succ.begin(), block->list_of_succ.end(),
                                                [&](const auto succ) { return !loop_blocks.count(succ); }))
            {
               reason = "unsupported direction guard CFG";
               return false;
            }
         }

         std::map<unsigned int, unsigned int> indegree;
         std::deque<unsigned int> ready;
         for(const auto block_id : loop_blocks)
         {
            unsigned int degree = 0;
            for(const auto pred : sl->list_of_bloc.at(block_id)->list_of_pred)
            {
               if(loop_blocks.count(pred) && !(pred == latch && block_id == header))
               {
                  ++degree;
               }
            }
            indegree[block_id] = degree;
            if(degree == 0)
            {
               ready.push_back(block_id);
            }
         }
         std::vector<unsigned int> topological_order;
         while(!ready.empty())
         {
            const auto current = ready.front();
            ready.pop_front();
            topological_order.push_back(current);
            for(const auto succ : sl->list_of_bloc.at(current)->list_of_succ)
            {
               if(!loop_blocks.count(succ) || (current == latch && succ == header))
               {
                  continue;
               }
               auto& degree = indegree[succ];
               if(--degree == 0)
               {
                  ready.push_back(succ);
               }
            }
         }
         if(topological_order.size() != loop_blocks.size())
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         unsigned int body_branch_count = 0;
         for(const auto block_id : loop_blocks)
         {
            if(block_id == latch)
            {
               continue;
            }
            const auto& statements = sl->list_of_bloc.at(block_id)->CGetStmtList();
            if(!statements.empty() && statements.back()->get_kind() == multi_way_if_stmt_K)
            {
               ++body_branch_count;
            }
         }
         if(body_branch_count > 2)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }

         const auto latch_statements = sl->list_of_bloc.at(latch)->CGetStmtList();
         if(latch_statements.empty() || latch_statements.back()->get_kind() != multi_way_if_stmt_K)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         const auto latch_branch = GetPointerS<const multi_way_if_stmt>(latch_statements.back());
         if(latch_branch->list_of_cond.size() != 2)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         const auto latch_exit_cond = std::find_if(latch_branch->list_of_cond.begin(), latch_branch->list_of_cond.end(),
                                                   [&](const auto& item) { return item.first && item.second == exit; });
         const auto latch_backedge =
             std::find_if(latch_branch->list_of_cond.begin(), latch_branch->list_of_cond.end(),
                          [&](const auto& item) { return !item.first && item.second == header; });
         if(latch_exit_cond == latch_branch->list_of_cond.end() || latch_backedge == latch_branch->list_of_cond.end() ||
            !latch_exit_cond->first || latch_exit_cond->first->get_kind() != ssa_node_K)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         const auto loop_compare_def = GetPointerS<const ssa_node>(latch_exit_cond->first)->GetDefStmt();
         if(!loop_compare_def || loop_compare_def->get_kind() != assign_stmt_K ||
            GetPointerS<const assign_stmt>(loop_compare_def)->op1->get_kind() != eq_node_K)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         const auto loop_compare =
             GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(loop_compare_def)->op1);
         ir_nodeRef induction;
         ir_nodeRef increment;
         for(const auto& phi_node : header_block->CGetPhiList())
         {
            if(recognizeZeroToPlusOneInduction(phi_node, preheader, latch, induction, increment))
            {
               break;
            }
         }
         if(!increment)
         {
            reason = "canonical induction must start at zero and increment by one";
            return false;
         }
         ir_nodeRef trip_bound;
         if(!findUnsigned32Bound(loop_compare, increment, trip_bound))
         {
            reason = "loop trip bound is not unsigned 32-bit SSA";
            return false;
         }
         const auto trip_def = GetPointerS<const ssa_node>(trip_bound)->GetDefStmt();
         if(trip_def && loop_blocks.count(GetPointerS<const node_stmt>(trip_def)->bb_index))
         {
            reason = "loop trip bound is defined inside loop";
            return false;
         }

         ir_nodeRef entry_guard;
         bool entry_guard_recognized = false;
         const auto preheader_block = sl->list_of_bloc.find(preheader);
         if(preheader_block == sl->list_of_bloc.end())
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         if(!preheader_block->second->CGetStmtList().empty() &&
            preheader_block->second->CGetStmtList().back()->get_kind() == multi_way_if_stmt_K)
         {
            const auto preheader_branch =
                GetPointerS<const multi_way_if_stmt>(preheader_block->second->CGetStmtList().back());
            if(preheader_branch->list_of_cond.size() == 2)
            {
               const auto enters =
                   std::find_if(preheader_branch->list_of_cond.begin(), preheader_branch->list_of_cond.end(),
                                [&](const auto& item) { return item.first && item.second == header; });
               if(enters != preheader_branch->list_of_cond.end() && enters->first &&
                  enters->first->get_kind() == ssa_node_K)
               {
                  const auto skips =
                      std::find_if(preheader_branch->list_of_cond.begin(), preheader_branch->list_of_cond.end(),
                                   [&](const auto& item) { return !item.first && item.second == exit; });
                  const auto guard_def = GetPointerS<const ssa_node>(enters->first)->GetDefStmt();
                  if(skips == preheader_branch->list_of_cond.end() || !guard_def ||
                     guard_def->get_kind() != assign_stmt_K ||
                     GetPointerS<const assign_stmt>(guard_def)->op1->get_kind() != gt_node_K)
                  {
                     reason = "unsupported direction guard CFG";
                     return false;
                  }
                  const auto guard_compare =
                      GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(guard_def)->op1);
                  if(!isConstantValue(guard_compare->op1, 0) || guard_compare->op0->get_kind() != ssa_node_K ||
                     !isSigned32Type(ir_helper::CGetType(guard_compare->op0)) ||
                     !isSigned32Type(ir_helper::CGetType(guard_compare->op1)))
                  {
                     reason = "unsupported direction guard CFG";
                     return false;
                  }
                  const auto signed_bound = GetPointerS<const ssa_node>(guard_compare->op0);
                  const auto signed_bound_def = signed_bound->GetDefStmt();
                  if(!signed_bound_def || signed_bound_def->get_kind() != assign_stmt_K ||
                     GetPointerS<const assign_stmt>(signed_bound_def)->op1->get_kind() != nop_node_K)
                  {
                     reason = "unsupported direction guard CFG";
                     return false;
                  }
                  const auto unsigned_bound =
                      GetPointerS<const unary_node>(GetPointerS<const assign_stmt>(signed_bound_def)->op1)->op;
                  if(unsigned_bound->index != trip_bound->index ||
                     !isUnsigned32Type(ir_helper::CGetType(unsigned_bound)) ||
                     !isSigned32Type(ir_helper::CGetType(guard_compare->op0)))
                  {
                     reason = "unsupported direction guard CFG";
                     return false;
                  }
                  entry_guard = enters->first;
                  entry_guard_recognized = true;
               }
            }
            if(!entry_guard_recognized && preheader_branch->list_of_cond.size() == 2)
            {
               const auto skips =
                   std::find_if(preheader_branch->list_of_cond.begin(), preheader_branch->list_of_cond.end(),
                                [&](const auto& item) { return item.first && item.second == exit; });
               const auto enters =
                   std::find_if(preheader_branch->list_of_cond.begin(), preheader_branch->list_of_cond.end(),
                                [&](const auto& item) { return !item.first && item.second == header; });
               if(skips != preheader_branch->list_of_cond.end() && enters != preheader_branch->list_of_cond.end() &&
                  skips->first && skips->first->get_kind() == ssa_node_K &&
                  isUnsigned32Type(ir_helper::CGetType(trip_bound)))
               {
                  const auto skip_def = GetPointerS<const ssa_node>(skips->first)->GetDefStmt();
                  if(skip_def && skip_def->get_kind() == assign_stmt_K &&
                     GetPointerS<const assign_stmt>(skip_def)->op1->get_kind() == eq_node_K)
                  {
                     const auto zero_test =
                         GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(skip_def)->op1);
                     const bool compares_trip_bound_to_zero =
                         (zero_test->op0->get_kind() == ssa_node_K && zero_test->op0->index == trip_bound->index &&
                          isConstantValue(zero_test->op1, 0)) ||
                         (zero_test->op1->get_kind() == ssa_node_K && zero_test->op1->index == trip_bound->index &&
                          isConstantValue(zero_test->op0, 0));
                     if(compares_trip_bound_to_zero)
                     {
                        entry_guard = skips->first;
                     }
                     entry_guard_recognized = compares_trip_bound_to_zero;
                  }
               }
            }
         }
         if(!entry_guard_recognized)
         {
            reason = "unsupported direction guard CFG";
            return false;
         }

         const auto is_reachable_avoiding = [&](unsigned int source, unsigned int target, unsigned int avoid) {
            if(source == avoid || target == avoid)
            {
               return false;
            }
            std::vector<unsigned int> pending{source};
            std::set<unsigned int> visited{source};
            while(!pending.empty())
            {
               const auto current = pending.back();
               pending.pop_back();
               if(current == target)
               {
                  return true;
               }
               for(const auto succ : sl->list_of_bloc.at(current)->list_of_succ)
               {
                  if(!loop_blocks.count(succ) || (current == latch && succ == header) || succ == avoid)
                  {
                     continue;
                  }
                  if(visited.insert(succ).second)
                  {
                     pending.push_back(succ);
                  }
               }
            }
            return false;
         };
         const auto path_reaches = [&](unsigned int source, unsigned int target) {
            std::vector<unsigned int> pending{source};
            std::set<unsigned int> visited{source};
            while(!pending.empty())
            {
               const auto current = pending.back();
               pending.pop_back();
               if(current == target)
               {
                  return true;
               }
               for(const auto succ : sl->list_of_bloc.at(current)->list_of_succ)
               {
                  if(!loop_blocks.count(succ) || (current == latch && succ == header))
                  {
                     continue;
                  }
                  if(visited.insert(succ).second)
                  {
                     pending.push_back(succ);
                  }
               }
            }
            return false;
         };
         const auto recognize_direction = [&](const unsigned int access_bb, ir_nodeRef& effective_bound,
                                              ir_nodeRef& effective_compare, bool& guarded) {
            std::vector<std::tuple<unsigned int, ir_nodeRef, ir_nodeRef>> matching_guards;
            std::vector<unsigned int> induction_prefix_guard_blocks;
            bool mentions_induction = false;
            bool has_controlling_branch = false;
            unsigned int controlling_branch_count = 0;
            for(const auto block_id : loop_blocks)
            {
               const auto& statements = sl->list_of_bloc.at(block_id)->CGetStmtList();
               if(statements.empty() || statements.back()->get_kind() != multi_way_if_stmt_K || block_id == latch)
               {
                  continue;
               }
               const auto branch = GetPointerS<const multi_way_if_stmt>(statements.back());
               if(branch->list_of_cond.size() != 2 || !branch->list_of_cond.front().first ||
                  branch->list_of_cond.front().first->get_kind() != ssa_node_K)
               {
                  continue;
               }
               const auto true_edge = branch->list_of_cond.front();
               const auto false_edge = branch->list_of_cond.back();
               const bool true_reaches_access = path_reaches(true_edge.second, access_bb);
               const bool false_reaches_access = path_reaches(false_edge.second, access_bb);
               if(true_reaches_access || false_reaches_access)
               {
                  const auto condition_def =
                      GetPointerS<const ssa_node>(branch->list_of_cond.front().first)->GetDefStmt();
                  if(condition_def && condition_def->get_kind() == assign_stmt_K)
                  {
                     const auto condition = GetPointerS<const assign_stmt>(condition_def)->op1;
                     if(condition->get_kind() == lt_node_K)
                     {
                        const auto compare = GetPointerS<const binary_node>(condition);
                        if(compare->op0->index == induction->index && compare->op1->get_kind() == ssa_node_K &&
                           isUnsigned32Type(ir_helper::CGetType(compare->op0)) &&
                           isUnsigned32Type(ir_helper::CGetType(compare->op1)))
                        {
                           induction_prefix_guard_blocks.push_back(block_id);
                        }
                     }
                  }
               }
               if(true_reaches_access == false_reaches_access)
               {
                  continue;
               }
               has_controlling_branch = true;
               ++controlling_branch_count;
               const auto compare_def = GetPointerS<const ssa_node>(branch->list_of_cond.front().first)->GetDefStmt();
               if(!compare_def || compare_def->get_kind() != assign_stmt_K)
               {
                  continue;
               }
               const auto condition = GetPointerS<const assign_stmt>(compare_def)->op1;
               if(condition->get_kind() != lt_node_K)
               {
                  if(condition->get_kind() == eq_node_K || condition->get_kind() == ne_node_K ||
                     condition->get_kind() == gt_node_K || condition->get_kind() == ge_node_K ||
                     condition->get_kind() == le_node_K)
                  {
                     const auto other_compare = GetPointerS<const binary_node>(condition);
                     mentions_induction |=
                         other_compare->op0->index == induction->index || other_compare->op1->index == induction->index;
                  }
                  continue;
               }
               const auto compare = GetPointerS<const binary_node>(condition);
               if(compare->op0->index != induction->index && compare->op1->index != induction->index)
               {
                  continue;
               }
               mentions_induction = true;
               if(true_reaches_access && !false_reaches_access &&
                  !is_reachable_avoiding(true_edge.second, latch, access_bb))
               {
                  matching_guards.emplace_back(block_id, branch->list_of_cond.front().first, condition);
               }
            }
            if(matching_guards.empty())
            {
               if(std::any_of(induction_prefix_guard_blocks.begin(), induction_prefix_guard_blocks.end(),
                              [&](const auto guard_bb) { return is_reachable_avoiding(header, access_bb, guard_bb); }))
               {
                  reason = "direction guard does not dominate memory access";
                  return false;
               }
               if(is_reachable_avoiding(header, latch, access_bb))
               {
                  reason = mentions_induction ? "direction guard is not an unsigned induction prefix" :
                                                "unsupported direction guard CFG";
                  return false;
               }
               if(mentions_induction)
               {
                  reason = "direction guard is not an unsigned induction prefix";
                  return false;
               }
               if(has_controlling_branch)
               {
                  reason = "unsupported direction guard CFG";
                  return false;
               }
               effective_bound = trip_bound;
               effective_compare = ir_nodeRef();
               guarded = false;
               return true;
            }
            if(matching_guards.size() != 1)
            {
               reason = "unsupported direction guard CFG";
               return false;
            }
            if(controlling_branch_count != 1)
            {
               reason = "unsupported direction guard CFG";
               return false;
            }
            const auto guard_bb = std::get<0>(matching_guards.front());
            effective_compare = std::get<1>(matching_guards.front());
            const auto compare = GetPointerS<const binary_node>(std::get<2>(matching_guards.front()));
            if(compare->op0->index != induction->index || compare->op1->get_kind() != ssa_node_K ||
               !isUnsigned32Type(ir_helper::CGetType(compare->op0)) ||
               !isUnsigned32Type(ir_helper::CGetType(compare->op1)))
            {
               reason = "direction guard is not an unsigned induction prefix";
               return false;
            }
            effective_bound = compare->op1;
            const auto effective_def = GetPointerS<const ssa_node>(effective_bound)->GetDefStmt();
            if(effective_def && loop_blocks.count(GetPointerS<const node_stmt>(effective_def)->bb_index))
            {
               reason = "effective bound is defined inside loop";
               return false;
            }
            if(is_reachable_avoiding(header, access_bb, guard_bb))
            {
               reason = "direction guard does not dominate memory access";
               return false;
            }
            guarded = true;
            return true;
         };

         ir_nodeRef read_effective_bound;
         ir_nodeRef read_effective_compare;
         bool read_guarded = false;
         if(!recognize_direction(read_node->bb_index, read_effective_bound, read_effective_compare, read_guarded))
         {
            return false;
         }
         ir_nodeRef write_effective_bound;
         ir_nodeRef write_effective_compare;
         bool write_guarded = false;
         if(!recognize_direction(write_node->bb_index, write_effective_bound, write_effective_compare, write_guarded))
         {
            return false;
         }
         if(!path_reaches(read_node->bb_index, write_node->bb_index))
         {
            reason = "read-before-write is not proven on paired prefix";
            return false;
         }
         if(read_node->bb_index == write_node->bb_index)
         {
            const auto& statements = sl->list_of_bloc.at(read_node->bb_index)->CGetStmtList();
            const auto read_position = std::find(statements.begin(), statements.end(), read_stmt);
            const auto write_position = std::find(statements.begin(), statements.end(), write_stmt);
            if(read_position == statements.end() || write_position == statements.end() ||
               std::distance(statements.begin(), read_position) >= std::distance(statements.begin(), write_position))
            {
               reason = "read-before-write is not proven on paired prefix";
               return false;
            }
         }
         if(!read_base || !write_base || read_base->get_kind() != ssa_node_K || write_base->get_kind() != ssa_node_K)
         {
            reason = "canonical contiguous loop recognition failed";
            return false;
         }
         if(read_stmt->get_kind() != assign_stmt_K || write_stmt->get_kind() != assign_stmt_K)
         {
            reason = "canonical contiguous loop recognition failed";
            return false;
         }
         if((read_node->predicate && !isConstantValue(read_node->predicate, 1)) ||
            (write_node->predicate && !isConstantValue(write_node->predicate, 1)))
         {
            reason = "unsupported direction guard CFG";
            return false;
         }
         const auto read_assign = GetPointerS<const assign_stmt>(read_stmt);
         const auto write_assign = GetPointerS<const assign_stmt>(write_stmt);
         if(read_assign->op1->get_kind() != mem_access_node_K || write_assign->op0->get_kind() != mem_access_node_K)
         {
            reason = "canonical contiguous loop recognition failed";
            return false;
         }
         const auto read_address = GetPointerS<const mem_access_node>(read_assign->op1)->op;
         const auto write_address = GetPointerS<const mem_access_node>(write_assign->op0)->op;
         if(!read_address || read_address->get_kind() != ssa_node_K || !write_address ||
            write_address->get_kind() != ssa_node_K)
         {
            reason = "canonical contiguous loop recognition failed";
            return false;
         }
         const auto read_gep_def = GetPointerS<const ssa_node>(read_address)->GetDefStmt();
         const auto write_gep_def = GetPointerS<const ssa_node>(write_address)->GetDefStmt();
         if(!read_gep_def || !write_gep_def || read_gep_def->get_kind() != assign_stmt_K ||
            write_gep_def->get_kind() != assign_stmt_K ||
            GetPointerS<const assign_stmt>(read_gep_def)->op1->get_kind() != gep_node_K ||
            GetPointerS<const assign_stmt>(write_gep_def)->op1->get_kind() != gep_node_K)
         {
            reason = "canonical contiguous loop recognition failed";
            return false;
         }
         const auto read_gep = GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(read_gep_def)->op1);
         const auto write_gep = GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(write_gep_def)->op1);
         const auto recognize_scaled_index = [&](const ir_nodeRef& index) {
            if(!index || index->get_kind() != ssa_node_K)
            {
               return false;
            }
            const auto index_def = GetPointerS<const ssa_node>(index)->GetDefStmt();
            if(!index_def || index_def->get_kind() != assign_stmt_K)
            {
               return false;
            }
            const auto expression = GetPointerS<const assign_stmt>(index_def)->op1;
            if(expression->get_kind() != mul_node_K && expression->get_kind() != shl_node_K)
            {
               return false;
            }
            const auto scaled = GetPointerS<const binary_node>(expression);
            return (scaled->op0->index == induction->index &&
                    ((expression->get_kind() == shl_node_K && isConstantValue(scaled->op1, 2)) ||
                     (expression->get_kind() != shl_node_K && isConstantValue(scaled->op1, 4)))) ||
                   (scaled->op1->index == induction->index && expression->get_kind() != shl_node_K &&
                    isConstantValue(scaled->op0, 4));
         };
         if(read_gep->op0->index != read_base->index || write_gep->op0->index != write_base->index ||
            !recognize_scaled_index(read_gep->op1) || !recognize_scaled_index(write_gep->op1) ||
            ir_helper::Size(read_assign->op0) != 32 || ir_helper::Size(write_assign->op1) != 32 ||
            !GetPointerS<const mem_access_node>(read_assign->op1)->ordinary_memory_access ||
            !GetPointerS<const mem_access_node>(write_assign->op0)->ordinary_memory_access)
         {
            reason = "canonical contiguous loop recognition failed";
            return false;
         }

         read_region.preheader = preheader;
         read_region.header = header;
         read_region.branch = preheader_block->second->CGetStmtList().back();
         read_region.guard = entry_guard;
         read_region.bound = trip_bound;
         read_region.loop_trip_bound = trip_bound;
         read_region.loop_trip_compare = latch_exit_cond->first;
         read_region.effective_bound = read_effective_bound;
         read_region.effective_compare = read_effective_compare;
         read_region.access_bb = read_node->bb_index;
         read_region.has_direction_guard = read_guarded;
         read_region.base = read_base;
         read_region.induction = induction;
         read_region.reads = {read_stmt};
         write_region.preheader = preheader;
         write_region.header = header;
         write_region.exit = exit;
         write_region.guard = entry_guard;
         write_region.bound = trip_bound;
         write_region.loop_trip_bound = trip_bound;
         write_region.loop_trip_compare = latch_exit_cond->first;
         write_region.effective_bound = write_effective_bound;
         write_region.effective_compare = write_effective_compare;
         write_region.access_bb = write_node->bb_index;
         write_region.has_direction_guard = write_guarded;
         write_region.base = write_base;
         write_region.induction = induction;
         write_region.store = write_stmt;
         reason.clear();
         return true;
      }
   } // namespace

   bool recognizeReadBurstRegion(const statement_list_node* sl, const std::vector<ir_nodeRef>& reads,
                                 const ir_nodeRef& expected_base,
                                 const std::vector<OriginalMemoryAccess>& original_memory_accesses,
                                 const std::string& expected_bundle, ReadBurstRegion& result,
                                 const ir_nodeRef& paired_store)
   {
      if(reads.empty() || !expected_base || expected_base->get_kind() != ssa_node_K)
      {
         return false;
      }
      const auto first_stmt = reads.front();
      const auto loop_id = GetPointerS<const node_stmt>(first_stmt)->bb_index;
      const auto bb_it = sl->list_of_bloc.find(loop_id);
      if(bb_it == sl->list_of_bloc.end())
      {
         return false;
      }
      const auto loop_bb = bb_it->second;
      unsigned int preheader_id;
      unsigned int exit_id;
      if(!findSingleBlockLoopEdges(loop_bb->list_of_pred, loop_bb->list_of_succ, loop_id, preheader_id, exit_id))
      {
         return false;
      }
      const auto preheader_bb_it = sl->list_of_bloc.find(preheader_id);
      if(preheader_bb_it == sl->list_of_bloc.end() || preheader_bb_it->second->list_of_pred.size() != 1)
      {
         return false;
      }
      const auto& loop_stmts = loop_bb->CGetStmtList();
      if(loop_stmts.empty() || loop_stmts.back()->get_kind() != multi_way_if_stmt_K)
      {
         return false;
      }
      for(auto stmt_it = loop_stmts.begin(); stmt_it != std::prev(loop_stmts.end()); ++stmt_it)
      {
         const auto stmt = *stmt_it;
         if(paired_store && stmt->index == paired_store->index)
         {
            continue;
         }
         if(isPreanalyzedCrossBundleWrite(stmt, original_memory_accesses, expected_base, expected_bundle))
         {
            continue;
         }
         if(stmt->get_kind() != assign_stmt_K)
         {
            return false;
         }
         const auto assignment = GetPointerS<const assign_stmt>(stmt);
         if(assignment->op0->get_kind() == mem_access_node_K || assignment->op1->get_kind() == call_node_K)
         {
            return false;
         }
      }
      const auto loop_branch = loop_stmts.back();
      const auto loop_branch_node = GetPointerS<const multi_way_if_stmt>(loop_branch);
      if(loop_branch_node->list_of_cond.empty())
      {
         return false;
      }
      const auto branch_condition = loop_branch_node->list_of_cond.front().first;
      if(!branch_condition || branch_condition->get_kind() != ssa_node_K)
      {
         return false;
      }
      ir_nodeRef induction;
      ir_nodeRef increment;
      ir_nodeRef bound;
      if(!recognizeCanonicalTripBound(loop_bb->CGetPhiList(), loop_id, preheader_id, branch_condition, induction,
                                      increment, bound))
      {
         return false;
      }

      // The IR's conditional edge is the first successor for a true comparison.
      if(loop_bb->list_of_succ.size() != 2 || loop_branch_node->list_of_cond.front().second != exit_id)
      {
         return false;
      }

      const auto preheader_bb = preheader_bb_it->second;
      if(preheader_bb->CGetStmtList().empty() || preheader_bb->CGetStmtList().back()->get_kind() != multi_way_if_stmt_K)
      {
         return false;
      }
      const auto guard_branch = preheader_bb->CGetStmtList().back();
      const auto guard_node = GetPointerS<const multi_way_if_stmt>(guard_branch);
      if(guard_node->list_of_cond.empty())
      {
         return false;
      }
      const auto guard_condition = guard_node->list_of_cond.front().first;
      if(!guard_condition || guard_condition->get_kind() != ssa_node_K)
      {
         return false;
      }
      if(!isPositiveBoundGuard(guard_condition, bound))
      {
         return false;
      }
      if(guard_node->list_of_cond.front().second != loop_id)
      {
         return false;
      }

      std::vector<ir_nodeRef> loop_reads;
      if(reads.size() != 1)
      {
         return false;
      }
      for(const auto& read_stmt : reads)
      {
         if(GetPointerS<const node_stmt>(read_stmt)->bb_index != loop_id)
         {
            return false;
         }
         loop_reads.push_back(read_stmt);
      }
      for(const auto& read_stmt : loop_reads)
      {
         if(read_stmt->get_kind() != assign_stmt_K)
         {
            return false;
         }
         const auto read = GetPointerS<const assign_stmt>(read_stmt);
         const auto read_node = GetPointerS<const node_stmt>(read_stmt);
         if(read->op1->get_kind() != mem_access_node_K ||
            (read_node->predicate && !isConstantValue(read_node->predicate, 1)) || ir_helper::Size(read->op0) != 32)
         {
            return false;
         }
         const auto address = GetPointerS<const mem_access_node>(read->op1)->op;
         if(!isFourByteIndexAddress(address, expected_base, induction))
         {
            return false;
         }
      }
      result.preheader = preheader_id;
      result.header = loop_id;
      result.branch = guard_branch;
      result.guard = guard_condition;
      result.bound = bound;
      result.loop_trip_bound = bound;
      result.loop_trip_compare = branch_condition;
      result.effective_bound = bound;
      result.effective_compare = ir_nodeRef();
      result.access_bb = GetPointerS<const node_stmt>(reads.front())->bb_index;
      result.has_direction_guard = false;
      result.base = expected_base;
      result.induction = induction;
      result.reads = std::move(loop_reads);
      return true;
   }

   bool recognizeWriteBurstRegion(const statement_list_node* sl, const std::vector<ir_nodeRef>& stores,
                                  const ir_nodeRef& expected_base,
                                  const std::vector<OriginalMemoryAccess>& original_memory_accesses,
                                  const std::string& expected_bundle, WriteBurstRegion& result,
                                  const ir_nodeRef& paired_read)
   {
      if(stores.size() != 1 || !expected_base || expected_base->get_kind() != ssa_node_K)
      {
         return false;
      }
      const auto store_stmt = stores.front();
      const auto loop_id = GetPointerS<const node_stmt>(store_stmt)->bb_index;
      const auto loop_it = sl->list_of_bloc.find(loop_id);
      if(loop_it == sl->list_of_bloc.end())
      {
         return false;
      }
      const auto loop_bb = loop_it->second;
      unsigned int preheader_id;
      unsigned int exit_id;
      if(!findSingleBlockLoopEdges(loop_bb->list_of_pred, loop_bb->list_of_succ, loop_id, preheader_id, exit_id))
      {
         return false;
      }
      const auto preheader = sl->list_of_bloc.find(preheader_id);
      const auto exit = sl->list_of_bloc.find(exit_id);
      if(preheader == sl->list_of_bloc.end() || preheader->second->list_of_pred.size() != 1 ||
         exit == sl->list_of_bloc.end() ||
         (exit->second->list_of_pred.size() != 1 &&
          !(exit->second->list_of_pred.size() == 2 &&
            std::count(exit->second->list_of_pred.begin(), exit->second->list_of_pred.end(), preheader_id) == 1)))
      {
         return false;
      }
      const auto& statements = loop_bb->CGetStmtList();
      if(statements.empty() || statements.back()->get_kind() != multi_way_if_stmt_K)
      {
         return false;
      }
      for(auto stmt_it = statements.begin(); stmt_it != std::prev(statements.end()); ++stmt_it)
      {
         if(paired_read && (*stmt_it)->index == paired_read->index)
         {
            continue;
         }
         if((*stmt_it)->get_kind() != assign_stmt_K)
         {
            return false;
         }
         const auto assignment = GetPointerS<const assign_stmt>(*stmt_it);
         if((*stmt_it)->index != store_stmt->index && assignment->op0 &&
            assignment->op0->get_kind() == mem_access_node_K)
         {
            return false;
         }
         if(assignment->op1 && assignment->op1->get_kind() == call_node_K)
         {
            const auto call = GetPointerS<const call_node>(assignment->op1);
            const auto is_preanalyzed_source_read =
                std::any_of(original_memory_accesses.begin(), original_memory_accesses.end(), [&](const auto& access) {
                   return access.statement->index == (*stmt_it)->index && !access.is_write && access.classified &&
                          access.ordinary && access.base && access.bundle != expected_bundle && call->fn &&
                          ir_helper::GetFunctionName(call->fn) == ENCODE_FDNAME(access.bundle, "_burst_read", "");
                });
            if(!is_preanalyzed_source_read)
            {
               return false;
            }
         }
      }
      const auto branch = GetPointerS<const multi_way_if_stmt>(statements.back());
      if(branch->list_of_cond.size() != 2)
      {
         return false;
      }
      const auto loop_exit_condition =
          std::find_if(branch->list_of_cond.begin(), branch->list_of_cond.end(),
                       [&](const auto& condition) { return condition.first && condition.second == exit_id; });
      const auto loop_backedge =
          std::find_if(branch->list_of_cond.begin(), branch->list_of_cond.end(),
                       [&](const auto& condition) { return !condition.first && condition.second == loop_id; });
      if(loop_exit_condition == branch->list_of_cond.end() || loop_backedge == branch->list_of_cond.end() ||
         loop_exit_condition->first->get_kind() != ssa_node_K)
      {
         return false;
      }
      ir_nodeRef induction;
      ir_nodeRef increment;
      ir_nodeRef bound;
      if(!recognizeCanonicalTripBound(loop_bb->CGetPhiList(), loop_id, preheader_id, loop_exit_condition->first,
                                      induction, increment, bound))
      {
         return false;
      }
      const auto preheader_stmts = preheader->second->CGetStmtList();
      if(preheader_stmts.empty() || preheader_stmts.back()->get_kind() != multi_way_if_stmt_K)
      {
         return false;
      }
      const auto guard = GetPointerS<const multi_way_if_stmt>(preheader_stmts.back());
      if(guard->list_of_cond.size() != 2)
      {
         return false;
      }
      const auto enters_loop =
          std::find_if(guard->list_of_cond.begin(), guard->list_of_cond.end(),
                       [&](const auto& condition) { return condition.first && condition.second == loop_id; });
      const auto skips_loop =
          std::find_if(guard->list_of_cond.begin(), guard->list_of_cond.end(),
                       [&](const auto& condition) { return !condition.first && condition.second == exit_id; });
      if(enters_loop == guard->list_of_cond.end() || skips_loop == guard->list_of_cond.end() ||
         enters_loop->first->get_kind() != ssa_node_K)
      {
         return false;
      }
      if(!isPositiveBoundGuard(enters_loop->first, bound))
      {
         return false;
      }
      if(store_stmt->get_kind() != assign_stmt_K)
      {
         return false;
      }
      const auto store = GetPointerS<const assign_stmt>(store_stmt);
      const auto store_node = GetPointerS<const node_stmt>(store_stmt);
      if(store->op0->get_kind() != mem_access_node_K ||
         (store_node->predicate && !isConstantValue(store_node->predicate, 1)) || ir_helper::Size(store->op0) != 32 ||
         !GetPointerS<const mem_access_node>(store->op0)->ordinary_memory_access)
      {
         return false;
      }
      const auto address = GetPointerS<const mem_access_node>(store->op0)->op;
      if(!isFourByteIndexAddress(address, expected_base, induction) || ir_helper::Size(store->op1) != 32)
      {
         return false;
      }
      for(const auto& block : sl->list_of_bloc)
      {
         for(const auto& stmt : block.second->CGetStmtList())
         {
            if(stmt->get_kind() == return_stmt_K && block.first != exit_id)
            {
               return false;
            }
         }
      }
      result.preheader = preheader_id;
      result.header = loop_id;
      result.exit = exit_id;
      result.guard = enters_loop->first;
      result.bound = bound;
      result.loop_trip_bound = bound;
      result.loop_trip_compare = loop_exit_condition->first;
      result.effective_bound = bound;
      result.effective_compare = ir_nodeRef();
      result.access_bb = GetPointerS<const node_stmt>(store_stmt)->bb_index;
      result.has_direction_guard = false;
      result.base = expected_base;
      result.induction = induction;
      result.store = store_stmt;
      return true;
   }

   std::vector<OriginalMemoryAccess> CollectOriginalMemoryAccesses(const function_val_node* fd,
                                                                   const unsigned int root_id,
                                                                   const FunctionArchitectureRef& func_arch,
                                                                   const application_managerRef& AppM,
                                                                   const ir_managerRef& TM, bool& all_classified)
   {
      std::vector<OriginalMemoryAccess> accesses;
      all_classified = true;
      const auto sl = GetPointerS<const statement_list_node>(fd->body);
      for(const auto& block : sl->list_of_bloc)
      {
         for(const auto& stmt : block.second->CGetStmtList())
         {
            if(!stmt)
            {
               THROW_ERROR("Null statement in original memory access collection");
            }
            if(stmt->get_kind() != assign_stmt_K)
            {
               continue;
            }
            const auto assignment = GetPointerS<const assign_stmt>(stmt);
            const auto record_access = [&](const ir_nodeRef& operand, const bool is_write) {
               if(!operand || operand->get_kind() != mem_access_node_K)
               {
                  return;
               }
               const auto access = GetPointerS<const mem_access_node>(operand);
               const auto base = ir_helper::GetBaseVariable(access->op);
               unsigned int owner_argument_index = 0;
               std::string bundle;
               bool classified = false;
               for(const auto& candidate_arg : fd->list_of_args)
               {
                  if(!ir_helper::IsPointerType(ir_helper::CGetType(candidate_arg)))
                  {
                     continue;
                  }
                  const auto candidate_ssa = TM->GetIRNode(AppM->getSSAFromParm(root_id, candidate_arg->index));
                  if(!base ||
                     (candidate_arg->index != base->index && (!candidate_ssa || candidate_ssa->index != base->index)))
                  {
                     continue;
                  }
                  const auto parm = func_arch->parms.find(get_decl_name(candidate_arg));
                  if(parm == func_arch->parms.end())
                  {
                     continue;
                  }
                  const auto candidate_bundle = parm->second.at(FunctionArchitecture::parm_bundle);
                  if(!classified)
                  {
                     owner_argument_index = candidate_arg->index;
                     bundle = candidate_bundle;
                     classified = true;
                  }
                  else if(bundle != candidate_bundle)
                  {
                     classified = false;
                     break;
                  }
               }
               const bool ordinary = access->ordinary_memory_access;
               all_classified &= classified && ordinary;
               accesses.push_back({stmt, base, owner_argument_index, bundle, is_write, ordinary, classified});
            };
            record_access(assignment->op0, true);
            record_access(assignment->op1, false);
         }
      }
      return accesses;
   }

   bool RecognizeSameBaseDuplex(const statement_list_node* sl, const function_val_node* fd, const unsigned int root_id,
                                const std::string& bundle_name,
                                const std::vector<OriginalMemoryAccess>& original_accesses,
                                const std::vector<OriginalMemoryAccess>& burst_accesses,
                                const bool original_effects_safe, const application_managerRef& AppM,
                                const ir_managerRef& TM, const FunctionArchitectureRef& func_arch,
                                SameBaseDuplexProof& proof, std::string& reason)
   {
      reason = "original effects are not fully classified ordinary accesses";
      if(!original_effects_safe || original_accesses.size() != 2)
      {
         return false;
      }
      const auto read_access =
          std::find_if(original_accesses.begin(), original_accesses.end(),
                       [&](const auto& access) { return !access.is_write && access.bundle == bundle_name; });
      const auto write_access =
          std::find_if(original_accesses.begin(), original_accesses.end(),
                       [&](const auto& access) { return access.is_write && access.bundle == bundle_name; });
      if(read_access == original_accesses.end() || write_access == original_accesses.end() ||
         !read_access->classified || !write_access->classified || !read_access->ordinary || !write_access->ordinary ||
         !read_access->base || !write_access->base)
      {
         reason = "not exactly one classified same-base read/store pair";
         return false;
      }
      const bool same_base = read_access->base->index == write_access->base->index &&
                             read_access->owner_argument_index == write_access->owner_argument_index;
      const bool distinct_base = read_access->base->index != write_access->base->index &&
                                 read_access->owner_argument_index != write_access->owner_argument_index;
      const auto read_owner_arg =
          std::find_if(fd->list_of_args.begin(), fd->list_of_args.end(),
                       [&](const auto& candidate) { return candidate->index == read_access->owner_argument_index; });
      const auto write_owner_arg =
          std::find_if(fd->list_of_args.begin(), fd->list_of_args.end(),
                       [&](const auto& candidate) { return candidate->index == write_access->owner_argument_index; });
      if(read_owner_arg == fd->list_of_args.end() || write_owner_arg == fd->list_of_args.end())
      {
         reason = "read/write owner argument is unavailable";
         return false;
      }
      bool runtime_range_guard = false;
      if(!same_base && distinct_base)
      {
         const auto source_attrs = func_arch->parms.find(get_decl_name(*read_owner_arg));
         const auto destination_attrs = func_arch->parms.find(get_decl_name(*write_owner_arg));
         std::string source_restrict;
         std::string destination_restrict;
         if(source_attrs != func_arch->parms.end())
         {
            const auto attr = source_attrs->second.find(FunctionArchitecture::parm_source_restrict);
            if(attr != source_attrs->second.end())
            {
               source_restrict = attr->second;
            }
         }
         if(destination_attrs != func_arch->parms.end())
         {
            const auto attr = destination_attrs->second.find(FunctionArchitecture::parm_source_restrict);
            if(attr != destination_attrs->second.end())
            {
               destination_restrict = attr->second;
            }
         }
         const bool both_unrestricted = source_restrict == "0" && destination_restrict == "0";
         const bool both_restricted = source_restrict == "1" && destination_restrict == "1";
         if(both_unrestricted)
         {
            runtime_range_guard = true;
         }
         else if(!both_restricted)
         {
            reason = "distinct-base duplex requires source restrict on both owners";
            return false;
         }
      }
      else if(!same_base)
      {
         reason = "not exactly one classified same-base read/store pair";
         return false;
      }
      const auto expected_read_base = TM->GetIRNode(AppM->getSSAFromParm(root_id, (*read_owner_arg)->index));
      const auto expected_write_base = TM->GetIRNode(AppM->getSSAFromParm(root_id, (*write_owner_arg)->index));
      ReadBurstRegion read_region{};
      WriteBurstRegion write_region{};
      const std::vector<ir_nodeRef> reads{read_access->statement};
      const std::vector<ir_nodeRef> writes{write_access->statement};
      const bool recognized_by_legacy_shape =
          recognizeReadBurstRegion(sl, reads, expected_read_base, burst_accesses, bundle_name, read_region,
                                   write_access->statement) &&
          recognizeWriteBurstRegion(sl, writes, expected_write_base, burst_accesses, bundle_name, write_region,
                                    read_access->statement);
      if(!recognized_by_legacy_shape &&
         !recognizePrefixDuplexRegions(sl, read_access->statement, write_access->statement, expected_read_base,
                                       expected_write_base, read_region, write_region, reason))
      {
         if(reason.empty())
         {
            reason = "canonical contiguous loop recognition failed";
         }
         return false;
      }
      bool runtime_shape_valid = true;
      if(runtime_range_guard)
      {
         runtime_shape_valid = recognized_by_legacy_shape && read_region.access_bb == write_region.access_bb &&
                               read_region.access_bb == read_region.header && !read_region.has_direction_guard &&
                               !write_region.has_direction_guard && read_region.preheader == write_region.preheader &&
                               read_region.effective_bound->index == read_region.loop_trip_bound->index &&
                               write_region.effective_bound->index == write_region.loop_trip_bound->index &&
                               read_region.loop_trip_bound->index == write_region.loop_trip_bound->index &&
                               read_region.loop_trip_compare->index == write_region.loop_trip_compare->index;
         if(runtime_shape_valid)
         {
            const auto header_it = sl->list_of_bloc.find(read_region.header);
            const auto preheader_it = sl->list_of_bloc.find(read_region.preheader);
            const auto exit_it = sl->list_of_bloc.find(write_region.exit);
            runtime_shape_valid = header_it != sl->list_of_bloc.end() && preheader_it != sl->list_of_bloc.end() &&
                                  exit_it != sl->list_of_bloc.end() && header_it->second->loop_id != 0 &&
                                  header_it->second->loop_id != preheader_it->second->loop_id &&
                                  preheader_it->second->loop_id == exit_it->second->loop_id;
            if(runtime_shape_valid)
            {
               const auto& header = header_it->second;
               const std::set<unsigned int> expected_predecessors{read_region.preheader, read_region.header};
               const std::set<unsigned int> expected_successors{read_region.header, write_region.exit};
               runtime_shape_valid = std::set<unsigned int>(header->list_of_pred.begin(), header->list_of_pred.end()) ==
                                         expected_predecessors &&
                                     std::set<unsigned int>(header->list_of_succ.begin(), header->list_of_succ.end()) ==
                                         expected_successors &&
                                     header->CGetStmtList().size() >= 3 &&
                                     header->CGetStmtList().back()->get_kind() == multi_way_if_stmt_K;
               for(const auto& block : sl->list_of_bloc)
               {
                  if(block.second->loop_id == header->loop_id && block.first != read_region.header)
                  {
                     runtime_shape_valid = false;
                     break;
                  }
               }
               if(runtime_shape_valid)
               {
                  const auto branch = GetPointerS<const multi_way_if_stmt>(header->CGetStmtList().back());
                  runtime_shape_valid =
                      branch->list_of_cond.size() == 2 &&
                      std::any_of(branch->list_of_cond.begin(), branch->list_of_cond.end(),
                                  [&](const auto& edge) { return edge.first && edge.second == write_region.exit; }) &&
                      std::any_of(branch->list_of_cond.begin(), branch->list_of_cond.end(),
                                  [&](const auto& edge) { return !edge.first && edge.second == read_region.header; });
               }
               for(auto stmt = header->CGetStmtList().begin();
                   runtime_shape_valid && stmt != std::prev(header->CGetStmtList().end()); ++stmt)
               {
                  runtime_shape_valid = (*stmt)->get_kind() == assign_stmt_K;
               }
            }
         }
      }
      if(runtime_range_guard && !runtime_shape_valid)
      {
         reason = "runtime alias fallback requires one straight-line shared access block";
         return false;
      }
      if(read_region.preheader != write_region.preheader || read_region.header != write_region.header ||
         read_region.induction->index != write_region.induction->index ||
         read_region.loop_trip_bound->index != write_region.loop_trip_bound->index ||
         read_region.loop_trip_compare->index != write_region.loop_trip_compare->index)
      {
         reason = "read/write accesses do not share one canonical loop";
         return false;
      }
      THROW_ASSERT((same_base && read_region.base->index == write_region.base->index) ||
                       (distinct_base && read_region.base->index != write_region.base->index),
                   "Duplex owner/base classification must imply the accepted base relation");
      if(!recognized_by_legacy_shape)
      {
         if(read_region.access_bb == write_region.access_bb)
         {
            const auto& loop_stmts = sl->list_of_bloc.at(read_region.access_bb)->CGetStmtList();
            const auto read_position = std::find_if(loop_stmts.begin(), loop_stmts.end(), [&](const auto& stmt) {
               return stmt->index == read_access->statement->index;
            });
            const auto write_position = std::find_if(loop_stmts.begin(), loop_stmts.end(), [&](const auto& stmt) {
               return stmt->index == write_access->statement->index;
            });
            if(read_position == loop_stmts.end() || write_position == loop_stmts.end() ||
               std::distance(loop_stmts.begin(), read_position) >= std::distance(loop_stmts.begin(), write_position))
            {
               reason = "read-before-write is not proven on paired prefix";
               return false;
            }
         }
      }
      else
      {
         const auto& loop_stmts = sl->list_of_bloc.at(read_region.access_bb)->CGetStmtList();
         const auto read_position = std::find_if(loop_stmts.begin(), loop_stmts.end(), [&](const auto& stmt) {
            return stmt->index == read_access->statement->index;
         });
         const auto write_position = std::find_if(loop_stmts.begin(), loop_stmts.end(), [&](const auto& stmt) {
            return stmt->index == write_access->statement->index;
         });
         if(read_position == loop_stmts.end() || write_position == loop_stmts.end() ||
            std::distance(loop_stmts.begin(), read_position) >= std::distance(loop_stmts.begin(), write_position))
         {
            reason = "LOAD does not precede STORE in the original iteration";
            return false;
         }
      }
      proof.owner_argument_index = read_access->owner_argument_index;
      proof.read_owner_argument_index = read_access->owner_argument_index;
      proof.write_owner_argument_index = write_access->owner_argument_index;
      proof.distinct_bases = distinct_base;
      proof.runtime_range_guard = runtime_range_guard;
      proof.bundle = bundle_name;
      proof.base = read_region.base;
      proof.read_base = read_region.base;
      proof.write_base = write_region.base;
      proof.load_statement = read_access->statement;
      proof.store_statement = write_access->statement;
      proof.read_region = std::move(read_region);
      proof.write_region = std::move(write_region);
      reason = runtime_range_guard ? "distinct-base runtime-range-guarded pointwise read-before-write candidate" :
               distinct_base       ? "distinct-base restricted pointwise read-before-write candidate" :
                                     "same-base pointwise read-before-write candidate";
      return true;
   }
   void PreanalyzeBursts(const bool burst_feature_enabled, const unsigned int effective_burst_type,
                         const unsigned int root_id, const function_val_node* fd,
                         const FunctionArchitectureRef& func_arch,
                         const std::vector<OriginalMemoryAccess>& original_memory_accesses,
                         const std::vector<OriginalMemoryAccess>& burst_memory_accesses,
                         const bool function_effects_safe, const bool original_memory_effects_safe,
                         const std::map<unsigned int, std::map<std::string, FrozenDuplexContribution>>& frozen_by_root,
                         const std::map<std::string, std::string>& bundle_vetoes, const application_managerRef& AppM,
                         const ir_managerRef& TM, std::map<std::string, BundleBurstDecision>& burst_decisions,
                         int DEBUG_PARAMETER(debug_level))
   {
      if(!burst_feature_enabled)
      {
         return;
      }
      const auto frozen_proof_for_bundle = [&](const std::string& bundle_name) -> const SameBaseDuplexProof* {
         const auto root_plan = frozen_by_root.find(root_id);
         if(root_plan == frozen_by_root.end())
         {
            return nullptr;
         }
         const auto bundle_plan = root_plan->second.find(bundle_name);
         return bundle_plan == root_plan->second.end() ? nullptr : &bundle_plan->second.proof;
      };
      // Two m_axi bundles with different names and different SSA bases do not prove disjoint
      // address ranges: with non-restrict pointers two interfaces can address overlapping memory,
      // and the two burst engines order their channels independently. Coalescing a bundle therefore
      // requires the caller's restrict contract on both the owner of this bundle and every owner
      // that performs the opposite direction in another bundle; without that contract the scalar
      // interface is kept.
      const auto parm_has_restrict_contract = [&](unsigned int argument_index) {
         const auto arg = std::find_if(fd->list_of_args.begin(), fd->list_of_args.end(),
                                       [&](const auto& candidate) { return candidate->index == argument_index; });
         if(arg == fd->list_of_args.end())
         {
            return false;
         }
         const auto attrs = func_arch->parms.find(get_decl_name(*arg));
         if(attrs == func_arch->parms.end())
         {
            return false;
         }
         const auto attr = attrs->second.find(FunctionArchitecture::parm_source_restrict);
         return attr != attrs->second.end() && attr->second == "1";
      };
      const auto cross_bundle_burst_is_disjoint = [&](const std::string& bundle_name, bool coalescing_reads,
                                                      unsigned int owner_argument_index) {
         std::vector<unsigned int> opposite_owners;
         for(const auto& access : burst_memory_accesses)
         {
            if(!access.classified || !access.ordinary || !access.statement || access.bundle == bundle_name)
            {
               continue;
            }
            const bool opposite = coalescing_reads ? access.is_write : !access.is_write;
            if(opposite)
            {
               opposite_owners.push_back(access.owner_argument_index);
            }
         }
         if(opposite_owners.empty())
         {
            // No access of the opposite direction lives in another bundle, so the ordering
            // between two independent m_axi interfaces is not what has to be proven here.
            return true;
         }
         if(!parm_has_restrict_contract(owner_argument_index))
         {
            return false;
         }
         for(const auto opposite_owner : opposite_owners)
         {
            if(!parm_has_restrict_contract(opposite_owner))
            {
               return false;
            }
         }
         return true;
      };
      const auto sl = GetPointerS<const statement_list_node>(fd->body);
      for(const auto& iface : func_arch->ifaces)
      {
         const auto& bundle_name = iface.first;
         const auto& iface_attrs = iface.second;
         const auto mode = iface_attrs.find(FunctionArchitecture::iface_mode);
         if(mode == iface_attrs.end() || mode->second != "m_axi")
         {
            continue;
         }
         const auto bundle_has_bound_parameter =
             std::any_of(func_arch->parms.begin(), func_arch->parms.end(), [&](const auto& parameter) {
                const auto bundle = parameter.second.find(FunctionArchitecture::parm_bundle);
                return bundle != parameter.second.end() && bundle->second == bundle_name;
             });
         if(!bundle_has_bound_parameter)
         {
            const auto max_read = iface_attrs.find(FunctionArchitecture::iface_max_read_burst_length);
            const auto read_outstanding = iface_attrs.find(FunctionArchitecture::iface_num_read_outstanding);
            const auto read_fifo = iface_attrs.find(FunctionArchitecture::iface_read_fifo_depth);
            if(max_read != iface_attrs.end() || read_outstanding != iface_attrs.end() || read_fifo != iface_attrs.end())
            {
               // Unbound XML profiles are metadata only. Parse with the normal defaults so
               // O-only and D-only requests remain valid while malformed explicit values
               // retain the standard attribute- and bundle-specific diagnostic.
               (void)ParseAXIReadBurstProfile(iface_attrs, bundle_name);
            }
            const auto max_write = iface_attrs.find(FunctionArchitecture::iface_max_write_burst_length);
            const auto write_outstanding = iface_attrs.find(FunctionArchitecture::iface_write_burst_outstanding);
            const auto write_fifo = iface_attrs.find(FunctionArchitecture::iface_write_fifo_depth);
            if(max_write != iface_attrs.end() || write_outstanding != iface_attrs.end() ||
               write_fifo != iface_attrs.end())
            {
               (void)ParseAXIWriteBurstProfile(iface_attrs, bundle_name);
            }
            continue;
         }
         auto& decision = burst_decisions[bundle_name];
         const auto max_read_burst = iface_attrs.find(FunctionArchitecture::iface_max_read_burst_length);
         const auto max_write_burst = iface_attrs.find(FunctionArchitecture::iface_max_write_burst_length);
         const auto write_outstanding = iface_attrs.find(FunctionArchitecture::iface_write_burst_outstanding);
         const auto write_fifo_depth = iface_attrs.find(FunctionArchitecture::iface_write_fifo_depth);
         const auto read_outstanding = iface_attrs.find(FunctionArchitecture::iface_num_read_outstanding);
         const auto read_fifo_depth = iface_attrs.find(FunctionArchitecture::iface_read_fifo_depth);
         decision.read_requested = max_read_burst != iface_attrs.end();
         decision.write_requested = max_write_burst != iface_attrs.end();
         if(decision.read_requested)
         {
            decision.read_profile = ParseAXIReadBurstProfile(iface_attrs, bundle_name);
            decision.read_max_burst_length = static_cast<unsigned int>(std::stoul(max_read_burst->second));
         }
         if(max_write_burst != iface_attrs.end() || write_outstanding != iface_attrs.end() ||
            write_fifo_depth != iface_attrs.end())
         {
            decision.write_profile = ParseAXIWriteBurstProfile(iface_attrs, bundle_name);
         }
         if(max_read_burst == iface_attrs.end() &&
            (read_outstanding != iface_attrs.end() || read_fifo_depth != iface_attrs.end()))
         {
            THROW_ERROR_USAGE(
                "AXI read profile attributes num_read_outstanding/read_fifo_depth for bundle '" + bundle_name +
                "' require max_read_burst_length; the profile cannot be applied to the legacy scalar interface.");
         }
         for(const auto& access : burst_memory_accesses)
         {
            if(access.bundle == bundle_name && access.classified)
            {
               decision.bundle_has_memory_write |= access.is_write;
               decision.bundle_has_memory_read |= !access.is_write;
               auto& direction = decision.accesses_by_argument[access.owner_argument_index];
               (access.is_write ? direction.writes : direction.reads).push_back(access);
            }
         }
         // With neither coalescing nor concurrent transactions, keep the scalar
         // adapter and its start/done ABI. A configured engine would add region
         // tracking and payload buffering without enabling either AXI feature.
         const bool scalar_read = !decision.bundle_has_memory_read || (decision.read_profile.max_burst_length == 1 &&
                                                                       decision.read_profile.max_outstanding == 1);
         const bool scalar_write = !decision.bundle_has_memory_write || (decision.write_profile.max_burst_length == 1 &&
                                                                         decision.write_profile.max_outstanding == 1);
         if(scalar_read && scalar_write)
         {
            decision.read_fallback_reason = "single-beat profile with one transaction; using scalar adapter";
            decision.write_fallback_reason = decision.read_fallback_reason;
            decision.same_base_duplex_reason = decision.read_fallback_reason;
            continue;
         }
         const bool cache_enabled = iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end();
         if(decision.read_requested && decision.bundle_has_memory_read)
         {
            if(effective_burst_type != 1U)
            {
               decision.read_fallback_reason = effective_burst_type == 0U ?
                                                   "AXI burst type is FIXED; incremental prefetch is unsupported" :
                                                   "AXI burst type is unsupported; incremental prefetch is unavailable";
            }
            else if(!function_effects_safe)
            {
               decision.read_fallback_reason = "memory writes or opaque effects; prefetch ordering is not proven";
            }
            else if(cache_enabled)
            {
               decision.read_fallback_reason = "cache-enabled bundle";
            }
            else if(decision.bundle_has_memory_write && !frozen_proof_for_bundle(bundle_name))
            {
               decision.read_fallback_reason = "bundle contains stores";
            }
            if(decision.read_fallback_reason.empty())
            {
               std::vector<ir_nodeRef> bundle_pointer_args;
               for(const auto& candidate_arg : fd->list_of_args)
               {
                  const auto candidate_name = get_decl_name(candidate_arg);
                  if(func_arch->parms.count(candidate_name) &&
                     func_arch->parms.at(candidate_name).at(FunctionArchitecture::parm_bundle) == bundle_name &&
                     ir_helper::IsPointerType(ir_helper::CGetType(candidate_arg)))
                  {
                     bundle_pointer_args.push_back(candidate_arg);
                  }
               }
               std::set<unsigned int> seen_read_statements;
               std::map<unsigned int, std::vector<ir_nodeRef>> loop_reads;
               const auto frozen_proof = frozen_proof_for_bundle(bundle_name);
               const auto read_owner = frozen_proof ?
                                           std::find_if(bundle_pointer_args.begin(), bundle_pointer_args.end(),
                                                        [&](const auto& arg) {
                                                           return arg->index == frozen_proof->read_owner_argument_index;
                                                        }) :
                                           bundle_pointer_args.begin();
               bool cross_bundle_read_disjoint = true;
               if(read_owner != bundle_pointer_args.end() &&
                  !cross_bundle_burst_is_disjoint(bundle_name, true, (*read_owner)->index))
               {
                  cross_bundle_read_disjoint = false;
                  decision.read_fallback_reason =
                      "writes through another m_axi bundle without a restrict contract on both "
                      "owners; cross-bundle prefetch ordering is not proven";
               }
               if(cross_bundle_read_disjoint && (bundle_pointer_args.size() == 1 || frozen_proof) &&
                  read_owner != bundle_pointer_args.end())
               {
                  const auto owner_arg = *read_owner;
                  const auto original_reads = decision.accesses_by_argument.find(owner_arg->index);
                  if(original_reads != decision.accesses_by_argument.end())
                  {
                     for(const auto& access : original_reads->second.reads)
                     {
                        if(access.ordinary && seen_read_statements.insert(access.statement->index).second)
                        {
                           loop_reads[GetPointerS<const node_stmt>(access.statement)->bb_index].push_back(
                               access.statement);
                        }
                     }
                  }
                  const auto expected_base_id = AppM->getSSAFromParm(root_id, owner_arg->index);
                  const auto expected_base = TM->GetIRNode(expected_base_id);
                  std::vector<ReadBurstRegion> regions;
                  bool owner_recognized = !loop_reads.empty();
                  for(const auto& read_group : loop_reads)
                  {
                     ReadBurstRegion region{};
                     if(!recognizeReadBurstRegion(sl, read_group.second, expected_base, burst_memory_accesses,
                                                  bundle_name, region,
                                                  frozen_proof ? frozen_proof->store_statement : ir_nodeRef()))
                     {
                        owner_recognized = false;
                        break;
                     }
                     regions.push_back(std::move(region));
                  }
                  if(owner_recognized && !regions.empty())
                  {
                     for(size_t first = 0; first < regions.size() && owner_recognized; ++first)
                     {
                        for(size_t second = first + 1; second < regions.size(); ++second)
                        {
                           const auto first_to_second =
                               hasBlockPath(sl, regions[first].preheader, regions[second].preheader);
                           const auto second_to_first =
                               hasBlockPath(sl, regions[second].preheader, regions[first].preheader);
                           if(first_to_second == second_to_first)
                           {
                              owner_recognized = false;
                              break;
                           }
                        }
                     }
                  }
                  if(owner_recognized && !regions.empty())
                  {
                     std::sort(regions.begin(), regions.end(), [&](const auto& lhs, const auto& rhs) {
                        return lhs.preheader != rhs.preheader && hasBlockPath(sl, lhs.preheader, rhs.preheader);
                     });
                     decision.read_regions[owner_arg->index] = std::move(regions);
                  }
               }
               decision.read_recognized = !decision.read_regions.empty();
            }
         }
         if(decision.write_requested && decision.bundle_has_memory_write)
         {
            if(effective_burst_type != 1U)
            {
               decision.write_fallback_reason = effective_burst_type == 0U ?
                                                    "AXI burst type is FIXED; write coalescing is unsupported" :
                                                    "AXI burst type is unsupported; write coalescing is unavailable";
            }
            else if(!original_memory_effects_safe)
            {
               decision.write_fallback_reason = "opaque, volatile, atomic, or unclassified memory effects";
            }
            else if(cache_enabled)
            {
               decision.write_fallback_reason = "cache-enabled bundle";
            }
            else if(decision.bundle_has_memory_read && !frozen_proof_for_bundle(bundle_name))
            {
               decision.write_fallback_reason = "bundle contains reads; same-bundle read/write requires duplex support";
            }
            if(decision.write_fallback_reason.empty())
            {
               bool any_owner_recognized = false;
               for(const auto& owner : func_arch->parms)
               {
                  if(owner.second.at(FunctionArchitecture::parm_bundle) != bundle_name)
                  {
                     continue;
                  }
                  const auto owner_arg =
                      std::find_if(fd->list_of_args.begin(), fd->list_of_args.end(),
                                   [&](const auto& candidate) { return get_decl_name(candidate) == owner.first; });
                  if(owner_arg == fd->list_of_args.end() || !ir_helper::IsPointerType(ir_helper::CGetType(*owner_arg)))
                  {
                     continue;
                  }
                  std::vector<ir_nodeRef> stores;
                  const auto original_writes = decision.accesses_by_argument.find((*owner_arg)->index);
                  if(original_writes != decision.accesses_by_argument.end())
                  {
                     for(const auto& access : original_writes->second.writes)
                     {
                        if(access.ordinary)
                        {
                           stores.push_back(access.statement);
                        }
                     }
                  }
                  std::sort(stores.begin(), stores.end(),
                            [](const auto& lhs, const auto& rhs) { return lhs->index < rhs->index; });
                  stores.erase(std::unique(stores.begin(), stores.end(),
                                           [](const auto& lhs, const auto& rhs) { return lhs->index == rhs->index; }),
                               stores.end());
                  if(stores.empty())
                  {
                     continue;
                  }
                  if(!cross_bundle_burst_is_disjoint(bundle_name, false, (*owner_arg)->index))
                  {
                     decision.write_fallback_reason =
                         "reads through another m_axi bundle without a restrict contract on both "
                         "owners; cross-bundle write ordering is not proven";
                     continue;
                  }
                  const auto expected_base_id = AppM->getSSAFromParm(root_id, (*owner_arg)->index);
                  const auto expected_base = TM->GetIRNode(expected_base_id);
                  WriteBurstRegion region{};
                  const auto frozen_proof = frozen_proof_for_bundle(bundle_name);
                  bool recognized =
                      recognizeWriteBurstRegion(sl, stores, expected_base, burst_memory_accesses, bundle_name, region,
                                                frozen_proof ? frozen_proof->load_statement : ir_nodeRef());
                  if(recognized)
                  {
                     for(const auto& access : burst_memory_accesses)
                     {
                        if(access.is_write && access.statement->index != region.store->index)
                        {
                           recognized = false;
                           break;
                        }
                        const bool paired_duplex_read =
                            frozen_proof && access.statement->index == frozen_proof->load_statement->index &&
                            access.bundle == bundle_name && access.classified && access.ordinary && access.base &&
                            expected_base && access.base->index == frozen_proof->read_base->index;
                        if(!access.is_write && !paired_duplex_read &&
                           (!access.classified || !access.ordinary || !access.base || !expected_base ||
                            access.base->index == expected_base->index || access.bundle == bundle_name))
                        {
                           recognized = false;
                           break;
                        }
                     }
                  }
                  if(recognized)
                  {
                     decision.write_regions[(*owner_arg)->index] = std::move(region);
                     any_owner_recognized = true;
                  }
               }
               decision.write_recognized = any_owner_recognized && !decision.write_regions.empty();
            }
         }
         if(decision.read_requested && decision.write_requested && decision.bundle_has_memory_read &&
            decision.bundle_has_memory_write)
         {
            decision.same_base_duplex_profile_reason =
                cache_enabled ? "cache-enabled bundle" : "read/write profile is eligible";
            decision.same_base_duplex_profile_eligible = !cache_enabled;
            if(effective_burst_type != 1U)
            {
               decision.same_base_duplex_profile_eligible = false;
               decision.same_base_duplex_profile_reason =
                   effective_burst_type == 0U ?
                       "AXI burst type is FIXED; duplex burst engines require INCREMENTAL" :
                       "AXI burst type is unsupported; duplex burst engines require INCREMENTAL";
               decision.same_base_duplex_reason = decision.same_base_duplex_profile_reason;
               continue;
            }
            decision.same_base_duplex_reason = "original effects are not fully classified ordinary accesses";
            SameBaseDuplexProof proof{};
            if(RecognizeSameBaseDuplex(sl, fd, root_id, bundle_name, original_memory_accesses, burst_memory_accesses,
                                       original_memory_effects_safe, AppM, TM, func_arch, proof,
                                       decision.same_base_duplex_reason))
            {
               decision.same_base_duplex_proof = std::make_unique<SameBaseDuplexProof>(std::move(proof));
               decision.same_base_duplex_candidate = true;
            }
         }
         else if(decision.read_requested && decision.write_requested)
         {
            decision.same_base_duplex_reason = !decision.bundle_has_memory_read ? "no read access" : "no write access";
         }
      }
      for(auto& bundle_decision : burst_decisions)
      {
         const auto frozen_root = frozen_by_root.find(root_id);
         if(frozen_root != frozen_by_root.end())
         {
            const auto frozen = frozen_root->second.find(bundle_decision.first);
            if(frozen != frozen_root->second.end())
            {
               auto& decision = bundle_decision.second;
               const auto& proof = frozen->second.proof;
               THROW_ASSERT(decision.same_base_duplex_candidate && decision.same_base_duplex_proof,
                            "Frozen duplex proof disappeared before IR transformation for bundle " +
                                bundle_decision.first);
#if HAVE_ASSERTS
               const auto& current = *decision.same_base_duplex_proof;
#endif
               THROW_ASSERT(
                   current.owner_argument_index == proof.owner_argument_index &&
                       current.read_owner_argument_index == proof.read_owner_argument_index &&
                       current.write_owner_argument_index == proof.write_owner_argument_index &&
                       current.distinct_bases == proof.distinct_bases &&
                       current.runtime_range_guard == proof.runtime_range_guard &&
                       current.base->index == proof.base->index && current.read_base->index == proof.read_base->index &&
                       current.write_base->index == proof.write_base->index &&
                       current.load_statement->index == proof.load_statement->index &&
                       current.store_statement->index == proof.store_statement->index &&
                       current.read_region.preheader == proof.read_region.preheader &&
                       current.read_region.header == proof.read_region.header &&
                       ((!current.read_region.branch && !proof.read_region.branch) ||
                        (current.read_region.branch && proof.read_region.branch &&
                         current.read_region.branch->index == proof.read_region.branch->index)) &&
                       current.read_region.guard->index == proof.read_region.guard->index &&
                       current.read_region.bound->index == proof.read_region.bound->index &&
                       current.read_region.loop_trip_bound->index == proof.read_region.loop_trip_bound->index &&
                       current.read_region.loop_trip_compare->index == proof.read_region.loop_trip_compare->index &&
                       current.read_region.effective_bound->index == proof.read_region.effective_bound->index &&
                       ((!current.read_region.effective_compare && !proof.read_region.effective_compare) ||
                        (current.read_region.effective_compare && proof.read_region.effective_compare &&
                         current.read_region.effective_compare->index == proof.read_region.effective_compare->index)) &&
                       current.read_region.access_bb == proof.read_region.access_bb &&
                       current.read_region.has_direction_guard == proof.read_region.has_direction_guard &&
                       current.read_region.induction->index == proof.read_region.induction->index &&
                       current.read_region.base->index == proof.read_region.base->index &&
                       current.write_region.preheader == proof.write_region.preheader &&
                       current.write_region.header == proof.write_region.header &&
                       current.write_region.exit == proof.write_region.exit &&
                       current.write_region.guard->index == proof.write_region.guard->index &&
                       current.write_region.bound->index == proof.write_region.bound->index &&
                       current.write_region.loop_trip_bound->index == proof.write_region.loop_trip_bound->index &&
                       current.write_region.loop_trip_compare->index == proof.write_region.loop_trip_compare->index &&
                       current.write_region.effective_bound->index == proof.write_region.effective_bound->index &&
                       ((!current.write_region.effective_compare && !proof.write_region.effective_compare) ||
                        (current.write_region.effective_compare && proof.write_region.effective_compare &&
                         current.write_region.effective_compare->index ==
                             proof.write_region.effective_compare->index)) &&
                       current.write_region.access_bb == proof.write_region.access_bb &&
                       current.write_region.has_direction_guard == proof.write_region.has_direction_guard &&
                       current.write_region.induction->index == proof.write_region.induction->index &&
                       current.write_region.base->index == proof.write_region.base->index &&
                       current.write_region.store->index == proof.write_region.store->index,
                   "Original duplex proof snapshot changed before IR transformation for bundle " +
                       bundle_decision.first);
               decision.same_base_duplex_proof = std::make_unique<SameBaseDuplexProof>(proof);
               decision.read_regions[proof.read_owner_argument_index] = {proof.read_region};
               decision.write_regions[proof.write_owner_argument_index] = proof.write_region;
               decision.read_recognized = true;
               decision.write_recognized = true;
               decision.same_base_duplex_activated = true;
               decision.same_base_duplex_activation_reason = "frozen application-wide proof and profile accepted";
            }
         }
         if(!bundle_decision.second.same_base_duplex_activated)
         {
            const auto veto = bundle_vetoes.find(bundle_decision.first);
            if(veto != bundle_vetoes.end())
            {
               bundle_decision.second.same_base_duplex_activation_reason = veto->second;
            }
            else if(bundle_decision.second.same_base_duplex_candidate)
            {
               bundle_decision.second.same_base_duplex_activation_reason =
                   bundle_decision.first.rfind("DF_bambu_", 0) == 0 ?
                       "DATAFLOW/forwarded bundle ownership is unsupported" :
                   bundle_decision.second.same_base_duplex_profile_eligible ?
                       "bundle is not a globally eligible activation candidate" :
                       bundle_decision.second.same_base_duplex_profile_reason;
            }
            else if(bundle_decision.second.read_requested && bundle_decision.second.write_requested)
            {
               bundle_decision.second.same_base_duplex_activation_reason =
                   bundle_decision.second.same_base_duplex_reason;
            }
         }
      }
#ifndef NDEBUG
      for(const auto& bundle_decision : burst_decisions)
      {
         const auto& decision = bundle_decision.second;
         for(const auto& owner_accesses : decision.accesses_by_argument)
         {
            const auto owner_argument_index = owner_accesses.first;
            const auto read_region = decision.read_regions.find(owner_argument_index);
            const auto write_region = decision.write_regions.find(owner_argument_index);
            INDENT_DBG_MEX(
                DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                "AXI_BURST_PREANALYSIS bundle=" + bundle_decision.first +
                    " owner_argument_index=" + std::to_string(owner_argument_index) +
                    " read_accesses=" + std::to_string(owner_accesses.second.reads.size()) +
                    " write_accesses=" + std::to_string(owner_accesses.second.writes.size()) + " read_requested=" +
                    std::to_string(static_cast<unsigned int>(decision.read_requested)) + " read_recognized=" +
                    std::to_string(static_cast<unsigned int>(read_region != decision.read_regions.end())) +
                    " read_fallback=\"" +
                    (owner_accesses.second.reads.empty() || decision.read_fallback_reason.empty() ?
                         "-" :
                         decision.read_fallback_reason) +
                    "\" write_requested=" + std::to_string(static_cast<unsigned int>(decision.write_requested)) +
                    " write_recognized=" +
                    std::to_string(static_cast<unsigned int>(write_region != decision.write_regions.end())) +
                    " write_fallback=\"" +
                    (owner_accesses.second.writes.empty() || decision.write_fallback_reason.empty() ?
                         "-" :
                         decision.write_fallback_reason) +
                    "\" same_base_duplex_candidate=" +
                    std::to_string(static_cast<unsigned int>(decision.same_base_duplex_candidate)) +
                    " same_base_duplex_reason=\"" + decision.same_base_duplex_reason + "\" production_activated=" +
                    std::to_string(static_cast<unsigned int>(decision.same_base_duplex_activated)) +
                    " activation_reason=\"" + decision.same_base_duplex_activation_reason + "\"");
         }
         if(decision.read_requested && decision.write_requested)
         {
            const auto json_string = [](const std::string& value) {
               std::string escaped;
               escaped.reserve(value.size() + 2U);
               for(const auto character : value)
               {
                  if(character == '\\' || character == '"')
                  {
                     escaped.push_back('\\');
                  }
                  if(character == '\n')
                  {
                     escaped += "\\n";
                  }
                  else if(character == '\r')
                  {
                     escaped += "\\r";
                  }
                  else
                  {
                     escaped.push_back(character);
                  }
               }
               return escaped;
            };
            const auto node_id = [](const ir_nodeRef& node) {
               return node ? std::to_string(node->index) : std::string("null");
            };
            const auto node_id_or_zero = [](const ir_nodeRef& node) {
               return node ? std::to_string(node->index) : std::string("0");
            };
            std::ostringstream proof_record;
            proof_record << "AXI_BURST_DUPLEX_PROOF {\"bundle\":\"" << json_string(bundle_decision.first)
                         << "\",\"semantic_proof\":" << (decision.same_base_duplex_proof ? "true" : "false")
                         << ",\"semantic_reason\":\"" << json_string(decision.same_base_duplex_reason)
                         << "\",\"runtime_range_guard\":"
                         << (decision.same_base_duplex_proof && decision.same_base_duplex_proof->runtime_range_guard ?
                                 "true" :
                                 "false")
                         << ",\"profile_eligible\":" << (decision.same_base_duplex_profile_eligible ? "true" : "false")
                         << ",\"profile_reason\":\"" << json_string(decision.same_base_duplex_profile_reason) << "\"";
            if(decision.same_base_duplex_proof)
            {
               const auto& proof = *decision.same_base_duplex_proof;
               proof_record << ",\"proof_kind\":\""
                            << (proof.runtime_range_guard ? "distinct_base_runtime_guarded_pointwise" :
                                proof.distinct_bases      ? "distinct_base_restricted_pointwise" :
                                                            "same_base_pointwise")
                            << "\",\"owner_argument_index\":" << proof.owner_argument_index
                            << ",\"read_owner_argument_index\":" << proof.read_owner_argument_index
                            << ",\"write_owner_argument_index\":" << proof.write_owner_argument_index
                            << ",\"base_ssa_index\":" << node_id(proof.base)
                            << ",\"read_base_ssa_index\":" << node_id(proof.read_base)
                            << ",\"write_base_ssa_index\":" << node_id(proof.write_base)
                            << ",\"load_statement_index\":" << node_id(proof.load_statement)
                            << ",\"store_statement_index\":" << node_id(proof.store_statement)
                            << ",\"read_preheader_bb\":" << proof.read_region.preheader
                            << ",\"read_header_bb\":" << proof.read_region.header
                            << ",\"read_branch_index\":" << node_id(proof.read_region.branch)
                            << ",\"read_guard_index\":" << node_id(proof.read_region.guard)
                            << ",\"read_bound_index\":" << node_id(proof.read_region.bound)
                            << ",\"loop_trip_bound_ssa_index\":" << node_id(proof.read_region.loop_trip_bound)
                            << ",\"loop_trip_compare_index\":" << node_id(proof.read_region.loop_trip_compare)
                            << ",\"read_effective_bound_ssa_index\":" << node_id(proof.read_region.effective_bound)
                            << ",\"read_effective_compare_index\":"
                            << node_id_or_zero(proof.read_region.effective_compare)
                            << ",\"read_access_bb\":" << proof.read_region.access_bb << ",\"read_count_recipe\":\""
                            << (proof.read_region.has_direction_guard ? "min_trip_and_effective_bound" : "trip_count")
                            << "\""
                            << ",\"read_induction_index\":" << node_id(proof.read_region.induction)
                            << ",\"write_preheader_bb\":" << proof.write_region.preheader
                            << ",\"write_header_bb\":" << proof.write_region.header
                            << ",\"write_exit_bb\":" << proof.write_region.exit
                            << ",\"write_guard_index\":" << node_id(proof.write_region.guard)
                            << ",\"write_bound_index\":" << node_id(proof.write_region.bound)
                            << ",\"write_effective_bound_ssa_index\":" << node_id(proof.write_region.effective_bound)
                            << ",\"write_effective_compare_index\":"
                            << node_id_or_zero(proof.write_region.effective_compare)
                            << ",\"write_access_bb\":" << proof.write_region.access_bb << ",\"write_count_recipe\":\""
                            << (proof.write_region.has_direction_guard ? "min_trip_and_effective_bound" : "trip_count")
                            << "\",\"pairing_domain_recipe\":\"min_read_write_counts_prefix\""
                            << ",\"write_induction_index\":" << node_id(proof.write_region.induction);
            }
            proof_record << "}";
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, proof_record.str());
         }
      }
#endif
   }

   bool IsDataflowArchitecture(const FunctionArchitectureRef& arch)
   {
      const auto it = arch->attrs.find(FunctionArchitecture::func_dataflow_module);
      return it != arch->attrs.end() && it->second == "1";
   }

   bool GetDuplexContribution(const unsigned int root_id, const FunctionArchitectureRef& arch,
                              const std::string& bundle, const bool burst_feature_enabled,
                              const unsigned int effective_burst_type, const unsigned int address_bitsize,
                              const application_managerRef& AppM, const ir_managerRef& TM, const CallGraphManager& CGM,
                              FrozenDuplexContribution& contribution, std::string& reason)
   {
      if(!burst_feature_enabled)
      {
         reason = "m-axi-burst=0 opt-out";
         return false;
      }
      if(bundle.rfind("DF_bambu_", 0) == 0 || IsDataflowArchitecture(arch))
      {
         reason = "DATAFLOW/forwarded bundle ownership is unsupported";
         return false;
      }
      const auto call_graph_vertex = CGM.GetVertex(root_id);
      if(CGM.GetCallGraph().in_degree(call_graph_vertex) != 0)
      {
         reason = "bundle contribution is not an independent entry root";
         return false;
      }
      if(effective_burst_type != 1U)
      {
         reason = "AXI burst type is not INCREMENTAL";
         return false;
      }
      const auto iface_it = arch->ifaces.find(bundle);
      if(iface_it == arch->ifaces.end())
      {
         reason = "bundle has no explicit interface architecture";
         return false;
      }
      const auto& attrs = iface_it->second;
      const auto mode = attrs.find(FunctionArchitecture::iface_mode);
      const auto read_max = attrs.find(FunctionArchitecture::iface_max_read_burst_length);
      const auto write_max = attrs.find(FunctionArchitecture::iface_max_write_burst_length);
      if(mode == attrs.end() || mode->second != "m_axi" || read_max == attrs.end() || write_max == attrs.end())
      {
         reason = "bundle does not explicitly request both read and write bursts";
         return false;
      }
      if(attrs.count(FunctionArchitecture::iface_cache_line_count))
      {
         reason = "cache-enabled bundle";
         return false;
      }
      const auto fd = GetPointer<const function_val_node>(TM->GetIRNode(root_id));
      const auto sl = GetPointerS<const statement_list_node>(fd->body);
      std::vector<ir_nodeRef> bundle_pointer_args;
      for(const auto& argument : fd->list_of_args)
      {
         if(!ir_helper::IsPointerType(ir_helper::CGetType(argument)))
         {
            continue;
         }
         const auto parm = arch->parms.find(get_decl_name(argument));
         if(parm == arch->parms.end() || parm->second.count(FunctionArchitecture::parm_bundle) == 0)
         {
            reason = "original pointer-to-bundle mapping is incomplete";
            return false;
         }
         if(parm->second.at(FunctionArchitecture::parm_bundle) == bundle)
         {
            bundle_pointer_args.push_back(argument);
         }
      }
      if(bundle_pointer_args.empty() || bundle_pointer_args.size() > 2)
      {
         reason = "bundle must have one or two pointer arguments";
         return false;
      }
      for(const auto& owner : bundle_pointer_args)
      {
         const auto owner_type = ir_helper::CGetPointedType(ir_helper::CGetType(owner));
         if(!owner_type || (owner_type->get_kind() != integer_ty_node_K && owner_type->get_kind() != real_ty_node_K) ||
            ir_helper::Size(owner_type) != 32 || ir_helper::SizeAlloc(owner_type) != 32)
         {
            reason = "bundle element must be a direct 32-bit integer or floating-point element";
            return false;
         }
      }
      const auto bitwidth = attrs.find(FunctionArchitecture::iface_bitwidth);
      const auto alignment = attrs.find(FunctionArchitecture::iface_alignment);
      if((bitwidth != attrs.end() && !bitwidth->second.empty() && std::stoul(bitwidth->second) != 32U) ||
         (alignment != attrs.end() && !alignment->second.empty() && std::stoul(alignment->second) < 4U))
      {
         reason = "unsupported interface width or alignment";
         return false;
      }
      if(address_bitsize != 32U && address_bitsize != 64U)
      {
         reason = "AXI address width is not 32 or 64 bits";
         return false;
      }
      bool classified = false;
      const auto original_accesses = CollectOriginalMemoryAccesses(fd, root_id, arch, AppM, TM, classified);
      const auto original_safe = hasOnlyOrdinaryMemoryEffects(sl) && classified;
      SameBaseDuplexProof proof{};
      if(!RecognizeSameBaseDuplex(sl, fd, root_id, bundle, original_accesses, original_accesses, original_safe, AppM,
                                  TM, arch, proof, reason))
      {
         return false;
      }
      if(proof.runtime_range_guard)
      {
         const auto pointer_bits =
             static_cast<unsigned int>(ir_helper::SizeAlloc(ir_helper::CGetType(proof.read_base)));
         if((pointer_bits != 32U && pointer_bits != 64U) || pointer_bits > address_bitsize)
         {
            reason = "runtime alias guard requires a 32/64-bit pointer no wider than the AXI address";
            return false;
         }
      }
      const auto owner_is_bundled = [&](const unsigned int owner_index) {
         return std::any_of(bundle_pointer_args.begin(), bundle_pointer_args.end(),
                            [&](const auto& owner) { return owner->index == owner_index; });
      };
      if(!owner_is_bundled(proof.read_owner_argument_index) || !owner_is_bundled(proof.write_owner_argument_index))
      {
         reason = "proof owners differ from the bundle pointer arguments";
         return false;
      }
      contribution.root_id = root_id;
      contribution.owner_argument_index = proof.read_owner_argument_index;
      contribution.bundle = bundle;
      contribution.proof = std::move(proof);
      contribution.read_max_burst_length = static_cast<unsigned int>(std::stoul(read_max->second));
      contribution.read_profile = ParseAXIReadBurstProfile(attrs, bundle);
      contribution.write_profile = ParseAXIWriteBurstProfile(attrs, bundle);
      if(contribution.read_profile.max_burst_length == 1 && contribution.read_profile.max_outstanding == 1 &&
         contribution.write_profile.max_burst_length == 1 && contribution.write_profile.max_outstanding == 1)
      {
         reason = "single-beat profile with one transaction; using scalar adapter";
         return false;
      }
      if(contribution.read_max_burst_length < 1U || contribution.read_max_burst_length > 256U)
      {
         reason = "unsupported duplex burst profile";
         return false;
      }
      contribution.address_bitsize = address_bitsize;
      contribution.latency = ParseAXIInterfaceLatency(attrs, bundle);
      return true;
   }
   AXIBurstApplicationPlan
   AnalyzeAXIBurstApplication(const std::vector<unsigned int>& sorted_roots, const bool burst_feature_enabled,
                              const unsigned int effective_burst_type, const unsigned int address_bitsize,
                              const ModuleArchitectureRef& module_arch, const application_managerRef& AppM,
                              const ir_managerRef& TM, const CallGraphManager& CGM)
   {
      AXIBurstApplicationPlan plan;
      std::map<unsigned int, std::map<std::string, FrozenDuplexContribution>> candidate_duplex_by_root;
      std::set<std::string> candidate_duplex_bundles;
      if(burst_feature_enabled)
      {
         for(const auto root_id : sorted_roots)
         {
            const auto fnode = TM->GetIRNode(root_id);
            const auto fsymbol = ir_helper::GetFunctionName(fnode);
            const auto arch = module_arch->GetArchitecture(fsymbol);
            if(!arch || IsDataflowArchitecture(arch))
            {
               continue;
            }
            for(const auto& iface : arch->ifaces)
            {
               const auto mode = iface.second.find(FunctionArchitecture::iface_mode);
               const auto read = iface.second.find(FunctionArchitecture::iface_max_read_burst_length);
               const auto write = iface.second.find(FunctionArchitecture::iface_max_write_burst_length);
               if(mode == iface.second.end() || mode->second != "m_axi" || read == iface.second.end() ||
                  write == iface.second.end())
               {
                  continue;
               }
               FrozenDuplexContribution contribution{};
               std::string reason;
               if(GetDuplexContribution(root_id, arch, iface.first, burst_feature_enabled, effective_burst_type,
                                        address_bitsize, AppM, TM, CGM, contribution, reason))
               {
                  candidate_duplex_bundles.insert(iface.first);
                  candidate_duplex_by_root[root_id].emplace(iface.first, std::move(contribution));
               }
            }
         }
      }

      for(const auto& candidate_bundle : candidate_duplex_bundles)
      {
         std::vector<FrozenDuplexContribution> contributions;
         std::string veto_reason;
         for(auto arch_it = module_arch->cbegin(); arch_it != module_arch->cend(); ++arch_it)
         {
            const auto arch = arch_it->second;
            const auto iface = arch->ifaces.find(candidate_bundle);
            if(iface == arch->ifaces.end())
            {
               continue;
            }
            const auto mode = iface->second.find(FunctionArchitecture::iface_mode);
            if(mode == iface->second.end() || mode->second != "m_axi")
            {
               continue;
            }
            const auto symbol = arch->attrs.find(FunctionArchitecture::func_symbol);
            if(symbol == arch->attrs.end())
            {
               veto_reason = "same-name architecture contribution has no resolved function owner";
               break;
            }
            const auto function = TM->GetFunction(symbol->second);
            if(!function)
            {
               veto_reason = "same-name architecture contribution has no resolved function owner";
               break;
            }
            const auto root_id = function->index;
            const auto existing_candidate = candidate_duplex_by_root.find(root_id);
            if(existing_candidate == candidate_duplex_by_root.end() ||
               existing_candidate->second.find(candidate_bundle) == existing_candidate->second.end())
            {
               FrozenDuplexContribution contribution{};
               std::string reason;
               if(!GetDuplexContribution(root_id, arch, candidate_bundle, burst_feature_enabled, effective_burst_type,
                                         address_bitsize, AppM, TM, CGM, contribution, reason))
               {
                  veto_reason = reason.empty() ? "same-name architecture contribution is not eligible" : reason;
                  break;
               }
               contributions.push_back(std::move(contribution));
            }
            else
            {
               contributions.push_back(existing_candidate->second.at(candidate_bundle));
            }
         }
         if(!veto_reason.empty() || contributions.empty())
         {
            plan.bundle_vetoes[candidate_bundle] =
                veto_reason.empty() ? "same-name architecture contribution is not eligible" : veto_reason;
            continue;
         }
         const auto& reference = contributions.front();
         const auto incompatible =
             std::find_if(std::next(contributions.begin()), contributions.end(), [&](const auto& contribution) {
                return contribution.read_max_burst_length != reference.read_max_burst_length ||
                       contribution.read_profile.max_outstanding != reference.read_profile.max_outstanding ||
                       contribution.read_profile.fifo_depth != reference.read_profile.fifo_depth ||
                       contribution.write_profile.max_burst_length != reference.write_profile.max_burst_length ||
                       contribution.write_profile.max_outstanding != reference.write_profile.max_outstanding ||
                       contribution.write_profile.fifo_depth != reference.write_profile.fifo_depth ||
                       contribution.latency != reference.latency ||
                       contribution.address_bitsize != reference.address_bitsize ||
                       contribution.data_bitsize != reference.data_bitsize;
             });
         if(incompatible != contributions.end())
         {
            THROW_ERROR_USAGE("Incompatible duplex m_axi profiles or latency for bundle '" + candidate_bundle +
                              "' across root functions; use distinct bundle names.");
         }
         plan.frozen_by_bundle.emplace(candidate_bundle, reference);
         for(auto& contribution : contributions)
         {
            plan.frozen_by_root[contribution.root_id].emplace(candidate_bundle, std::move(contribution));
         }
      }
      for(const auto& candidate_bundle : candidate_duplex_bundles)
      {
         if(plan.frozen_by_bundle.count(candidate_bundle) == 0)
         {
            plan.force_legacy_bundles.insert(candidate_bundle);
         }
      }
      return plan;
   }
} // namespace InterfaceInferInternal
