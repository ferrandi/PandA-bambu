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
 * @file InterfaceInfer_burst_recognition.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "InterfaceInfer_internal.hpp"

#include "application_manager.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"

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
} // namespace InterfaceInferInternal
