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
 * @file AXIBurstRewrite.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "AXIBurstRewrite.hpp"
#include "application_manager.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "token_interface.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace InterfaceInferInternal
{
   /// Interfaces carry integer bits. Resize integers separately from reinterpreting
   /// floating-point or vector values: bitcasts require equal operand/result widths.
   /// `context` names the interface parameter, to make a rejected cast diagnosable.
   ir_nodeRef createInterfaceCast(ir_nodeRef value, const ir_nodeConstRef& target_type,
                                  const ir_manipulationRef& ir_man, unsigned scope,
                                  std::vector<ir_nodeRef>& intermediate_stmts, const std::string& context)
   {
      const auto source_type = ir_helper::CGetType(value);
      const auto source_is_bits = ir_helper::IsRealType(source_type) || ir_helper::IsVectorType(source_type);
      const auto target_is_bits = ir_helper::IsRealType(target_type) || ir_helper::IsVectorType(target_type);
      auto cast_kind = source_is_bits || target_is_bits ? bitcast_node_K : nop_node_K;
      if(cast_kind == bitcast_node_K && ir_helper::Size(source_type) != ir_helper::Size(target_type))
      {
         // The intermediate is unsigned: the padding bits of a reinterpreted value
         // carry no sign, so widening it must never fill the upper bits with ones.
         const auto integer_type =
             ir_man->GetCustomIntegerType(ir_helper::Size(source_is_bits ? source_type : target_type), true);
         const auto intermediate_expr = ir_man->create_unary_operation(integer_type, value, BUILTIN_LOCINFO,
                                                                       source_is_bits ? bitcast_node_K : nop_node_K);
         const auto intermediate_stmt =
             ir_man->CreateAssignStmt(integer_type, nullptr, nullptr, intermediate_expr, scope, BUILTIN_LOCINFO);
         intermediate_stmts.push_back(intermediate_stmt);
         value = GetPointerS<const assign_stmt>(intermediate_stmt)->op0;
         cast_kind = target_is_bits ? bitcast_node_K : nop_node_K;
      }
      if(cast_kind == bitcast_node_K && ir_helper::Size(value) != ir_helper::Size(target_type))
      {
         const auto value_type = ir_helper::CGetType(value);
         THROW_ERROR("Interface bitcast width mismatch on interface '" + context + "': " + STR(value_type) + " (" +
                     STR(ir_helper::Size(value_type)) + " bits) -> " + STR(target_type) + " (" +
                     STR(ir_helper::Size(target_type)) + " bits)");
      }
      return ir_man->create_unary_operation(target_type, value, BUILTIN_LOCINFO, cast_kind);
   }

   namespace
   {
      void addVirtualUseToStatement(const ir_nodeRef& stmt, const ir_nodeRef& vssa)
      {
         if(!vssa)
         {
            return;
         }
         const auto stmt_node = GetPointerS<node_stmt>(stmt);
         if(stmt_node->AddVuse(vssa))
         {
            GetPointerS<ssa_node>(vssa)->AddUseStmt(stmt);
         }
      }

      void addVirtualOverToStatement(const ir_nodeRef& stmt, const ir_nodeRef& vssa)
      {
         if(!vssa)
         {
            return;
         }
         const auto stmt_node = GetPointerS<node_stmt>(stmt);
         if(stmt_node->AddVover(vssa))
         {
            GetPointerS<ssa_node>(vssa)->AddUseStmt(stmt);
         }
      }

      blocRef splitBlockBeforeStatement(statement_list_node* sl, const blocRef& block,
                                        const ir_nodeRef& first_statement, const application_managerRef& AppM,
                                        const ir_managerRef& TM)
      {
         THROW_ASSERT(block && !block->schedule, "Runtime duplex CFG lowering requires an unscheduled basic block");
         const auto first = std::find_if(block->CGetStmtList().begin(), block->CGetStmtList().end(),
                                         [&](const auto& stmt) { return stmt->index == first_statement->index; });
         THROW_ASSERT(first != block->CGetStmtList().end(), "Split point is not in its expected basic block");
         const auto old_successors = block->list_of_succ;
         const auto new_index = sl->list_of_bloc.rbegin()->first + 1U;
         const auto tail = blocRef(new bloc(new_index));
         tail->loop_id = block->loop_id;
         tail->schedule = block->schedule;
         tail->SetSSAUsesComputed();
         sl->list_of_bloc.emplace(new_index, tail);
         std::vector<ir_nodeRef> moved(first, block->CGetStmtList().end());
         for(const auto& stmt : moved)
         {
            block->RemoveStmt(stmt, AppM);
            tail->PushBack(stmt, AppM);
         }
         tail->list_of_pred.push_back(block->number);
         tail->list_of_succ = old_successors;
         block->list_of_succ = {new_index};
         for(const auto successor_id : old_successors)
         {
            const auto successor = sl->list_of_bloc.at(successor_id);
            for(auto& predecessor : successor->list_of_pred)
            {
               if(predecessor == block->number)
               {
                  predecessor = new_index;
               }
            }
            for(const auto& phi_node : successor->CGetPhiList())
            {
               auto phi = GetPointerS<phi_stmt>(phi_node);
               std::vector<phi_stmt::DefEdge> old_edges;
               for(const auto& edge : phi->CGetDefEdgesList())
               {
                  if(edge.second == block->number)
                  {
                     old_edges.push_back(edge);
                  }
               }
               for(const auto& edge : old_edges)
               {
                  phi->ReplaceDefEdge(TM, edge, {edge.first, new_index});
               }
            }
         }
         return tail;
      }

      ir_nodeRef createBranch(const ir_managerRef& TM, const unsigned int function_id, const blocRef& block,
                              const ir_nodeRef& condition, const unsigned int true_target,
                              const unsigned int false_target, const application_managerRef& AppM)
      {
         if(true_target == false_target)
         {
            THROW_ASSERT(block->list_of_succ.size() == 1 && block->list_of_succ.front() == true_target,
                         "Unconditional branch block must already have its sole CFG successor");
            return ir_nodeRef();
         }
         THROW_ASSERT(condition, "Conditional branch requires a condition");
         ir_manager::IRSchema schema;
         schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
         schema[TOK(TOK_PARENT)] = STR(function_id);
         const auto branch = TM->create_ir_node(multi_way_if_stmt_K, schema);
         const auto branch_stmt = GetPointerS<multi_way_if_stmt>(branch);
         branch_stmt->add_cond(condition, true_target);
         branch_stmt->add_cond(ir_nodeRef(), false_target);
         block->PushBack(branch, AppM);
         return branch;
      }

      void insertConditionalCallDiamond(statement_list_node* sl, const blocRef& predecessor, const blocRef& join,
                                        const ir_nodeRef& fast_mode, const ir_nodeRef& conditional_call,
                                        const unsigned int function_id, const application_managerRef& AppM,
                                        const ir_managerRef& TM)
      {
         THROW_ASSERT(predecessor->list_of_succ.size() == 1 && predecessor->list_of_succ.front() == join->number,
                      "Conditional call insertion requires a single predecessor edge");
         const auto make_block = [&](const unsigned int loop_id) {
            const auto index = sl->list_of_bloc.rbegin()->first + 1U;
            const auto block = blocRef(new bloc(index));
            block->loop_id = loop_id;
            block->schedule = predecessor->schedule;
            block->SetSSAUsesComputed();
            sl->list_of_bloc.emplace(index, block);
            return block;
         };
         const auto dispatch = make_block(predecessor->loop_id);
         const auto call_block = make_block(predecessor->loop_id);
         predecessor->list_of_succ = {dispatch->number};
         dispatch->list_of_pred.push_back(predecessor->number);
         dispatch->list_of_succ = {join->number, call_block->number};
         call_block->list_of_pred.push_back(dispatch->number);
         call_block->list_of_succ.push_back(join->number);
         join->list_of_pred.erase(
             std::remove(join->list_of_pred.begin(), join->list_of_pred.end(), predecessor->number),
             join->list_of_pred.end());
         join->list_of_pred.push_back(dispatch->number);
         join->list_of_pred.push_back(call_block->number);
         for(const auto& phi_node : join->CGetPhiList())
         {
            auto phi = GetPointerS<phi_stmt>(phi_node);
            std::vector<phi_stmt::DefEdge> old_edges;
            for(const auto& edge : phi->CGetDefEdgesList())
            {
               if(edge.second == predecessor->number)
               {
                  old_edges.push_back(edge);
               }
            }
            for(const auto& edge : old_edges)
            {
               phi->ReplaceDefEdge(TM, edge, {edge.first, dispatch->number});
               phi->AddDefEdge(TM, {edge.first, call_block->number});
            }
         }
         createBranch(TM, function_id, predecessor, ir_nodeRef(), dispatch->number, dispatch->number, AppM);
         createBranch(TM, function_id, dispatch, fast_mode, join->number, call_block->number, AppM);
         call_block->PushBack(conditional_call, AppM);
         createBranch(TM, function_id, call_block, ir_nodeRef(), join->number, join->number, AppM);
      }

   } // namespace

   void serializeInterfaceAccess(const ir_nodeRef& stmt, const std::string& interface_fname,
                                 std::map<std::string, ir_nodeRef>& last_interface_vdefs,
                                 const ir_manipulationRef& ir_man)
   {
      const auto stmt_node = GetPointerS<node_stmt>(stmt);
      THROW_ASSERT(stmt_node, "Expected statement for interface serialization");
      const auto last_vdef = last_interface_vdefs.find(interface_fname);
      if(last_vdef != last_interface_vdefs.end())
      {
         addVirtualUseToStatement(stmt, last_vdef->second);
      }
      const auto new_vdef = ir_man->create_ssa_name(ir_nodeRef(), ir_man->GetPointerType(ir_man->GetVoidType()),
                                                    ir_nodeRef(), ir_nodeRef(), true);
      stmt_node->SetVdef(new_vdef);
      GetPointerS<ssa_node>(new_vdef)->SetDefStmt(stmt);
      last_interface_vdefs[interface_fname] = new_vdef;
   }

   namespace
   {
      ir_nodeRef CreateDuplexDirectionCount(const ir_manipulationRef& ir_man, const ir_managerRef& TM,
                                            const application_managerRef& AppM, const unsigned int function_id,
                                            const blocRef& preheader, const ir_nodeRef& insertion_point,
                                            const ir_nodeRef& entry_guard, const ir_nodeRef& trip_bound,
                                            const ir_nodeRef& effective_bound, const bool has_direction_guard,
                                            const ir_nodeConstRef& u32_type)
      {
         const auto append_value = [&](const ir_nodeRef& expression, const ir_nodeConstRef& value_type) {
            const auto statement =
                ir_man->CreateAssignStmt(value_type, nullptr, nullptr, expression, function_id, BUILTIN_LOCINFO);
            preheader->PushBefore(statement, insertion_point, AppM);
            return GetPointerS<const assign_stmt>(statement)->op0;
         };
         ir_nodeRef count = trip_bound;
         if(has_direction_guard)
         {
            const auto boolean_type = ir_man->GetBooleanType();
            const auto compare =
                ir_man->create_binary_operation(boolean_type, trip_bound, effective_bound, BUILTIN_LOCINFO, lt_node_K);
            const auto compare_value = append_value(compare, boolean_type);
            const auto minimum = ir_man->create_ternary_operation(u32_type, compare_value, trip_bound, effective_bound,
                                                                  BUILTIN_LOCINFO, select_node_K);
            count = append_value(minimum, u32_type);
         }
         if(entry_guard)
         {
            const auto zero = TM->CreateUniqueIntegerCst(0, u32_type);
            ir_nodeRef entry_condition = entry_guard;
            const auto entry_guard_def = GetPointerS<const ssa_node>(entry_guard)->GetDefStmt();
            if(entry_guard_def && entry_guard_def->get_kind() == assign_stmt_K &&
               GetPointerS<const assign_stmt>(entry_guard_def)->op1->get_kind() == eq_node_K)
            {
               const auto zero_test =
                   GetPointerS<const binary_node>(GetPointerS<const assign_stmt>(entry_guard_def)->op1);
               const bool is_trip_bound_zero_test =
                   (zero_test->op0->get_kind() == ssa_node_K && zero_test->op0->index == trip_bound->index &&
                    isConstantValue(zero_test->op1, 0)) ||
                   (zero_test->op1->get_kind() == ssa_node_K && zero_test->op1->index == trip_bound->index &&
                    isConstantValue(zero_test->op0, 0));
               if(is_trip_bound_zero_test)
               {
                  const auto boolean_type = ir_man->GetBooleanType();
                  const auto nonzero_test =
                      ir_man->create_binary_operation(boolean_type, trip_bound, zero, BUILTIN_LOCINFO, ne_node_K);
                  entry_condition = append_value(nonzero_test, boolean_type);
               }
            }
            count = append_value(ir_man->create_ternary_operation(u32_type, entry_condition, count, zero,
                                                                  BUILTIN_LOCINFO, select_node_K),
                                 u32_type);
         }
         return count;
      }
   } // namespace

   void preserveBlockingInterfaceAccess(const ir_nodeRef& stmt, const ir_nodeRef& original_stmt,
                                        const std::string& interface_fname,
                                        std::map<std::string, ir_nodeRef>& last_interface_vdefs,
                                        const ir_manipulationRef& ir_man)
   {
      const auto stmt_node = GetPointerS<node_stmt>(stmt);
      const auto original_stmt_node = GetPointerS<node_stmt>(original_stmt);
      THROW_ASSERT(stmt_node, "Expected statement for blocking interface serialization");
      THROW_ASSERT(original_stmt_node, "Expected original statement for blocking interface serialization");

      if(original_stmt_node->memdef && !stmt_node->memdef)
      {
         stmt_node->memdef = original_stmt_node->memdef;
         GetPointerS<ssa_node>(stmt_node->memdef)->SetDefStmt(stmt);
      }
      if(original_stmt_node->memuse && !stmt_node->memuse)
      {
         stmt_node->memuse = original_stmt_node->memuse;
         GetPointerS<ssa_node>(stmt_node->memuse)->AddUseStmt(stmt);
      }
      for(const auto& vuse : original_stmt_node->vuses)
      {
         addVirtualUseToStatement(stmt, vuse);
      }
      for(const auto& vover : original_stmt_node->vovers)
      {
         addVirtualOverToStatement(stmt, vover);
      }

      const auto last_vdef = last_interface_vdefs.find(interface_fname);
      if(last_vdef != last_interface_vdefs.end())
      {
         addVirtualUseToStatement(stmt, last_vdef->second);
      }

      ir_nodeRef stmt_vdef = stmt_node->vdef;
      if(!stmt_vdef)
      {
         stmt_vdef = ir_man->create_ssa_name(ir_nodeRef(), ir_man->GetPointerType(ir_man->GetVoidType()), ir_nodeRef(),
                                             ir_nodeRef(), true);
         stmt_node->SetVdef(stmt_vdef);
         GetPointerS<ssa_node>(stmt_vdef)->SetDefStmt(stmt);
      }
      last_interface_vdefs[interface_fname] = stmt_vdef;
   }

   std::vector<ir_nodeRef> lowerRuntimeGuardedDuplex(
       const statement_list_node* const_sl, const function_val_node* fd, const SameBaseDuplexProof& proof,
       const std::string& bundle_name, const std::string& read_argument_name, const std::string& write_argument_name,
       const ir_nodeConstRef& interface_datatype, const unsigned int address_bitsize,
       const application_managerRef& AppM, const ir_managerRef& TM, const ir_manipulationRef& ir_man,
       std::map<std::string, ir_nodeRef>& channel_read_vdefs, std::map<std::string, ir_nodeRef>& channel_write_vdefs,
       std::map<unsigned int, std::map<std::string, std::list<unsigned int>>>& interface_io)
   {
      auto sl = const_cast<statement_list_node*>(const_sl);
      const auto function_id = fd->index;
      const auto& read_region = proof.read_region;
      const auto& write_region = proof.write_region;
      const auto address_type = ir_helper::CGetType(proof.read_base);
      const auto pointer_bits = static_cast<unsigned int>(ir_helper::SizeAlloc(address_type));
      const auto write_address_type = ir_helper::CGetType(proof.write_base);
      const auto count_type = ir_helper::CGetType(read_region.loop_trip_bound);
      THROW_ASSERT(proof.runtime_range_guard && proof.distinct_bases && pointer_bits >= 32 && pointer_bits <= 64 &&
                       (pointer_bits == 32 || pointer_bits == 64) && (address_bitsize == 32 || address_bitsize == 64) &&
                       pointer_bits <= address_bitsize,
                   "Runtime duplex range guard received an unqualified address profile");
      THROW_ASSERT(read_region.header == read_region.access_bb && read_region.access_bb == write_region.access_bb &&
                       read_region.preheader == write_region.preheader && read_region.header == write_region.header &&
                       read_region.loop_trip_bound->index == write_region.loop_trip_bound->index &&
                       ir_helper::SizeAlloc(write_address_type) == pointer_bits && isUnsigned32Type(count_type) &&
                       ir_helper::Size(GetPointerS<const assign_stmt>(proof.load_statement)->op0) ==
                           ir_helper::Size(interface_datatype) &&
                       ir_helper::Size(GetPointerS<const assign_stmt>(proof.store_statement)->op1) == 32,
                   "Runtime duplex lowering requires the frozen single-block shared loop");
      const auto preheader = sl->list_of_bloc.at(read_region.preheader);
      const auto header = sl->list_of_bloc.at(read_region.header);
      const auto exit = sl->list_of_bloc.at(write_region.exit);
      const auto parent_loop_id = preheader->loop_id;
      THROW_ASSERT(!preheader->CGetStmtList().empty() && !header->CGetStmtList().empty() &&
                       preheader->CGetStmtList().back()->get_kind() == multi_way_if_stmt_K &&
                       header->CGetStmtList().back()->get_kind() == multi_way_if_stmt_K &&
                       header->loop_id != parent_loop_id && exit->loop_id == parent_loop_id &&
                       std::find(preheader->list_of_succ.begin(), preheader->list_of_succ.end(), header->number) !=
                           preheader->list_of_succ.end() &&
                       std::find(header->list_of_succ.begin(), header->list_of_succ.end(), exit->number) !=
                           header->list_of_succ.end(),
                   "Runtime duplex loop entry/exit CFG is not canonical");
      const auto preheader_branch = GetPointerS<multi_way_if_stmt>(preheader->CGetStmtList().back());
#if HAVE_ASSERTS
      const auto header_branch = GetPointerS<const multi_way_if_stmt>(header->CGetStmtList().back());
#endif
      THROW_ASSERT(std::any_of(preheader_branch->list_of_cond.begin(), preheader_branch->list_of_cond.end(),
                               [&](const auto& edge) { return edge.second == header->number; }),
                   "Runtime duplex preheader terminator does not target its loop header");
      THROW_ASSERT(std::any_of(header_branch->list_of_cond.begin(), header_branch->list_of_cond.end(),
                               [&](const auto& edge) { return edge.second == exit->number; }) &&
                       std::any_of(header_branch->list_of_cond.begin(), header_branch->list_of_cond.end(),
                                   [&](const auto& edge) { return edge.second == header->number; }),
                   "Runtime duplex loop terminator must provide backedge and exit edges");
      const auto load = proof.load_statement;
      const auto store = proof.store_statement;
      const auto load_assign = GetPointerS<const assign_stmt>(load);
      const auto store_assign = GetPointerS<const assign_stmt>(store);
#if HAVE_ASSERTS
      const auto load_position = std::find_if(header->CGetStmtList().begin(), header->CGetStmtList().end(),
                                              [&](const auto& stmt) { return stmt->index == load->index; });
      const auto header_store_position = std::find_if(header->CGetStmtList().begin(), header->CGetStmtList().end(),
                                                      [&](const auto& stmt) { return stmt->index == store->index; });
#endif
      THROW_ASSERT(GetPointerS<const node_stmt>(load)->bb_index == header->number &&
                       GetPointerS<const node_stmt>(store)->bb_index == header->number &&
                       load_position != header->CGetStmtList().end() &&
                       header_store_position != header->CGetStmtList().end() &&
                       std::distance(header->CGetStmtList().begin(), load_position) <
                           std::distance(header->CGetStmtList().begin(), header_store_position) &&
                       std::next(header_store_position) != header->CGetStmtList().end() && !header->schedule &&
                       !preheader->schedule,
                   "Runtime duplex accesses are not ordered in the validated shared header");
#if HAVE_ASSERTS
      for(const auto& phi_node : header->CGetPhiList())
      {
         const auto phi = GetPointerS<const phi_stmt>(phi_node);
         const auto& edges = phi->CGetDefEdgesList();
         THROW_ASSERT(std::any_of(edges.begin(), edges.end(),
                                  [&](const auto& edge) { return edge.second == preheader->number; }) &&
                          std::any_of(edges.begin(), edges.end(),
                                      [&](const auto& edge) { return edge.second == header->number; }),
                      "Runtime duplex header PHI must represent both entry and backedge");
      }
#endif

      const auto make_block = [&](const unsigned int loop_id) {
         const auto index = sl->list_of_bloc.rbegin()->first + 1U;
         const auto block = blocRef(new bloc(index));
         block->loop_id = loop_id;
         block->SetSSAUsesComputed();
         sl->list_of_bloc.emplace(index, block);
         return block;
      };
      const auto append_value = [&](const blocRef& block, const ir_nodeRef& expression,
                                    const ir_nodeConstRef& value_type) {
         const auto statement =
             ir_man->CreateAssignStmt(value_type, nullptr, nullptr, expression, function_id, BUILTIN_LOCINFO);
         block->PushBack(statement, AppM);
         return GetPointerS<const assign_stmt>(statement)->op0;
      };
      std::vector<ir_nodeRef> generated_calls;
      CustomUnorderedSet<unsigned int> call_points;
      const auto register_call = [&](const ir_nodeRef& call, const ir_nodeRef& declaration,
                                     const std::string& argument_name,
                                     std::map<std::string, ir_nodeRef>& channel_vdefs) {
         serializeInterfaceAccess(call, bundle_name, channel_vdefs, ir_man);
         const auto call_bb = GetPointerS<const node_stmt>(call)->bb_index;
         interface_io[call_bb][argument_name].push_back(call->index);
         CallGraphManager::addCallPointAndExpand(call_points, AppM, function_id, declaration->index, call->index,
                                                 FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
         generated_calls.push_back(call);
      };
      const auto append_call = [&](const blocRef& block, const ir_nodeRef& declaration,
                                   const std::vector<ir_nodeRef>& args, const std::string& argument_name,
                                   std::map<std::string, ir_nodeRef>& channel_vdefs) {
         const auto call = ir_man->create_call_stmt(declaration, args, function_id, BUILTIN_LOCINFO);
         block->PushBack(call, AppM);
         register_call(call, declaration, argument_name, channel_vdefs);
         return call;
      };
      const auto opcode_type = ir_man->GetCustomIntegerType(2, true);
      const auto u32_type = ir_man->GetUnsignedIntegerType();
      const auto boolean_type = ir_man->GetBooleanType();
      const auto extended_bits = std::max(address_bitsize + 1U, 35U);
      const auto extended_type = ir_man->GetCustomIntegerType(extended_bits, true);
      const auto pointer_type = ir_man->GetCustomIntegerType(pointer_bits, true);
      const auto address_integer_type = ir_man->GetCustomIntegerType(address_bitsize, true);
      const auto read_configure_name = ENCODE_FDNAME(bundle_name, "_configure_read", "");
      const auto burst_read_name = ENCODE_FDNAME(bundle_name, "_burst_read", "");
      const auto write_configure_name = ENCODE_FDNAME(bundle_name, "_configure_write", "");
      const auto store_name = ENCODE_FDNAME(bundle_name, "_store_write", "");
      const auto drain_name = ENCODE_FDNAME(bundle_name, "_drain_write", "");
      const std::vector<ir_nodeConstRef> command_types{opcode_type, u32_type, interface_datatype, address_type,
                                                       u32_type};
      const auto read_configure_decl = ir_man->create_function_decl(read_configure_name, fd->parent, command_types,
                                                                    ir_man->GetVoidType(), BUILTIN_LOCINFO, false);
      const auto read_decl = ir_man->create_function_decl(burst_read_name, fd->parent, command_types,
                                                          interface_datatype, BUILTIN_LOCINFO, false);
      const auto write_configure_decl = ir_man->create_function_decl(write_configure_name, fd->parent, command_types,
                                                                     ir_man->GetVoidType(), BUILTIN_LOCINFO, false);
      const auto store_decl = ir_man->create_function_decl(store_name, fd->parent, command_types, ir_man->GetVoidType(),
                                                           BUILTIN_LOCINFO, false);
      const auto drain_decl = ir_man->create_function_decl(drain_name, fd->parent, command_types, ir_man->GetVoidType(),
                                                           BUILTIN_LOCINFO, false);

      const auto old_predecessor_edges = [&]() {
         std::map<unsigned int, ir_nodeRef> incoming;
         for(const auto& phi_node : header->CGetPhiList())
         {
            const auto phi = GetPointerS<const phi_stmt>(phi_node);
            for(const auto& edge : phi->CGetDefEdgesList())
            {
               if(edge.second == preheader->number)
               {
                  incoming[phi->index] = edge.first;
                  break;
               }
            }
         }
         return incoming;
      }();
      const auto entry = make_block(parent_loop_id);
      const auto full_configure = make_block(parent_loop_id);
      bool entry_edge_retargeted = false;
      for(auto& edge : preheader_branch->list_of_cond)
      {
         if(edge.second == header->number)
         {
            edge.second = entry->number;
            entry_edge_retargeted = true;
         }
      }
      THROW_ASSERT(entry_edge_retargeted, "Runtime duplex preheader edge to the loop header was not retargeted");
      (void)entry_edge_retargeted;
      std::replace(preheader->list_of_succ.begin(), preheader->list_of_succ.end(), header->number, entry->number);
      header->list_of_pred.erase(
          std::remove(header->list_of_pred.begin(), header->list_of_pred.end(), preheader->number),
          header->list_of_pred.end());
      entry->list_of_pred.push_back(preheader->number);
      entry->list_of_succ = {header->number, full_configure->number};
      full_configure->list_of_pred.push_back(entry->number);
      full_configure->list_of_succ.push_back(header->number);
      header->list_of_pred.push_back(entry->number);
      header->list_of_pred.push_back(full_configure->number);
      for(const auto& phi_node : header->CGetPhiList())
      {
         const auto phi = GetPointerS<phi_stmt>(phi_node);
         const auto incoming = old_predecessor_edges.find(phi->index);
         if(incoming != old_predecessor_edges.end())
         {
            phi->ReplaceDefEdge(TM, {incoming->second, preheader->number}, {incoming->second, entry->number});
            phi->AddDefEdge(TM, {incoming->second, full_configure->number});
         }
      }

      const auto append_unsigned_cast = [&](const blocRef& block, const ir_nodeRef& value,
                                            const ir_nodeConstRef& target_type) {
         const auto cast = ir_man->CreateNopExpr(value, target_type, nullptr, nullptr, function_id);
         block->PushBack(cast, AppM);
         return GetPointerS<const assign_stmt>(cast)->op0;
      };
      const auto append_extended_pointer = [&](const ir_nodeRef& pointer) {
         auto value = append_unsigned_cast(entry, pointer, pointer_type);
         if(pointer_bits < address_bitsize)
         {
            value = append_unsigned_cast(entry, value, address_integer_type);
         }
         if(address_bitsize < extended_bits)
         {
            value = append_unsigned_cast(entry, value, extended_type);
         }
         return value;
      };
      const auto read_start = append_extended_pointer(proof.read_base);
      const auto write_start = append_extended_pointer(proof.write_base);
      const auto count_u32 = read_region.loop_trip_bound;
      const auto extended_count = append_unsigned_cast(entry, count_u32, extended_type);
      const auto shift = TM->CreateUniqueIntegerCst(2, extended_type);
      const auto byte_count = append_value(
          entry, ir_man->create_binary_operation(extended_type, extended_count, shift, BUILTIN_LOCINFO, shl_node_K),
          extended_type);
      const auto read_end = append_value(
          entry, ir_man->create_binary_operation(extended_type, read_start, byte_count, BUILTIN_LOCINFO, add_node_K),
          extended_type);
      const auto write_end = append_value(
          entry, ir_man->create_binary_operation(extended_type, write_start, byte_count, BUILTIN_LOCINFO, add_node_K),
          extended_type);
      const auto limit_bits = std::min(pointer_bits, address_bitsize);
      const auto limit = TM->CreateUniqueIntegerCst(integer_cst_t(1) << limit_bits, extended_type);
      const auto read_valid = append_value(
          entry, ir_man->create_binary_operation(boolean_type, read_end, limit, BUILTIN_LOCINFO, le_node_K),
          boolean_type);
      const auto write_valid = append_value(
          entry, ir_man->create_binary_operation(boolean_type, write_end, limit, BUILTIN_LOCINFO, le_node_K),
          boolean_type);
      const auto both_valid = append_value(
          entry, ir_man->create_binary_operation(boolean_type, read_valid, write_valid, BUILTIN_LOCINFO, and_node_K),
          boolean_type);
      const auto read_before_write = append_value(
          entry, ir_man->create_binary_operation(boolean_type, read_end, write_start, BUILTIN_LOCINFO, le_node_K),
          boolean_type);
      const auto write_before_read = append_value(
          entry, ir_man->create_binary_operation(boolean_type, write_end, read_start, BUILTIN_LOCINFO, le_node_K),
          boolean_type);
      const auto disjoint = append_value(entry,
                                         ir_man->create_binary_operation(boolean_type, read_before_write,
                                                                         write_before_read, BUILTIN_LOCINFO, or_node_K),
                                         boolean_type);
      const auto nonempty =
          append_value(entry,
                       ir_man->create_binary_operation(boolean_type, count_u32, TM->CreateUniqueIntegerCst(0, u32_type),
                                                       BUILTIN_LOCINFO, ne_node_K),
                       boolean_type);
      const auto valid_disjoint = append_value(
          entry, ir_man->create_binary_operation(boolean_type, both_valid, disjoint, BUILTIN_LOCINFO, and_node_K),
          boolean_type);
      const auto fast_mode = append_value(
          entry, ir_man->create_binary_operation(boolean_type, nonempty, valid_disjoint, BUILTIN_LOCINFO, and_node_K),
          boolean_type);

      append_call(full_configure, read_configure_decl,
                  {TM->CreateUniqueIntegerCst(2, opcode_type), TM->CreateUniqueIntegerCst(32, u32_type),
                   TM->CreateUniqueIntegerCst(0, interface_datatype), proof.read_base, read_region.loop_trip_bound},
                  read_argument_name, channel_read_vdefs);
      append_call(full_configure, write_configure_decl,
                  {TM->CreateUniqueIntegerCst(2, opcode_type), TM->CreateUniqueIntegerCst(32, u32_type),
                   TM->CreateUniqueIntegerCst(0, interface_datatype), proof.write_base, write_region.loop_trip_bound},
                  write_argument_name, channel_write_vdefs);
      createBranch(TM, function_id, entry, fast_mode, full_configure->number, header->number, AppM);
      createBranch(TM, function_id, full_configure, ir_nodeRef(), header->number, header->number, AppM);

      const auto read_address = GetPointerS<const mem_access_node>(load_assign->op1)->op;
      const auto write_address = GetPointerS<const mem_access_node>(store_assign->op0)->op;
      const auto scalar_count = TM->CreateUniqueIntegerCst(1, u32_type);
      const auto scalar_read_configure = ir_man->create_call_stmt(
          read_configure_decl,
          {TM->CreateUniqueIntegerCst(2, opcode_type), TM->CreateUniqueIntegerCst(32, u32_type),
           TM->CreateUniqueIntegerCst(0, interface_datatype), read_address, scalar_count},
          function_id, BUILTIN_LOCINFO);
      const auto load_block = header;
      const auto load_join = splitBlockBeforeStatement(sl, load_block, load, AppM, TM);
      insertConditionalCallDiamond(sl, load_block, load_join, fast_mode, scalar_read_configure, function_id, AppM, TM);
      register_call(scalar_read_configure, read_configure_decl, read_argument_name, channel_read_vdefs);

      const auto scalar_write_configure = ir_man->create_call_stmt(
          write_configure_decl,
          {TM->CreateUniqueIntegerCst(2, opcode_type), TM->CreateUniqueIntegerCst(32, u32_type),
           TM->CreateUniqueIntegerCst(0, interface_datatype), write_address, scalar_count},
          function_id, BUILTIN_LOCINFO);
      const auto store_block = sl->list_of_bloc.at(GetPointerS<const node_stmt>(store)->bb_index);
      const auto store_join = splitBlockBeforeStatement(sl, store_block, store, AppM, TM);
      insertConditionalCallDiamond(sl, store_block, store_join, fast_mode, scalar_write_configure, function_id, AppM,
                                   TM);
      register_call(scalar_write_configure, write_configure_decl, write_argument_name, channel_write_vdefs);

      ir_nodeRef read_expr = ir_man->CreateCallExpr(
          read_decl,
          {TM->CreateUniqueIntegerCst(0, opcode_type), TM->CreateUniqueIntegerCst(32, u32_type),
           TM->CreateUniqueIntegerCst(0, interface_datatype), read_address, TM->CreateUniqueIntegerCst(0, u32_type)},
          BUILTIN_LOCINFO);
      ir_nodeRef rewritten_read = load;
      if(ir_helper::IsSameType(interface_datatype, ir_helper::CGetType(load_assign->op0)))
      {
         TM->ReplaceIRNode(load, load_assign->op1, read_expr);
      }
      else
      {
         const auto actual_type = ir_helper::CGetType(load_assign->op0);
         const auto temporary = ir_man->create_ssa_name(nullptr, interface_datatype, nullptr, nullptr);
         rewritten_read = ir_man->create_assign_stmt(temporary, read_expr, function_id, BUILTIN_LOCINFO);
         const auto access_block = sl->list_of_bloc.at(GetPointerS<const node_stmt>(load)->bb_index);
         access_block->Replace(load, rewritten_read, true, AppM);
         std::vector<ir_nodeRef> intermediate_stmts;
         const auto cast_expression =
             createInterfaceCast(temporary, actual_type, ir_man, function_id, intermediate_stmts, read_argument_name);
         auto last_stmt = rewritten_read;
         for(const auto& intermediate_stmt : intermediate_stmts)
         {
            access_block->PushAfter(intermediate_stmt, last_stmt, AppM);
            last_stmt = intermediate_stmt;
         }
         const auto cast_statement =
             ir_man->create_assign_stmt(load_assign->op0, cast_expression, function_id, BUILTIN_LOCINFO);
         access_block->PushAfter(cast_statement, last_stmt, AppM);
         generated_calls.push_back(cast_statement);
      }
      serializeInterfaceAccess(rewritten_read, bundle_name, channel_read_vdefs, ir_man);
      CallGraphManager::addCallPointAndExpand(call_points, AppM, function_id, read_decl->index, rewritten_read->index,
                                              FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
      interface_io[GetPointerS<const node_stmt>(rewritten_read)->bb_index][read_argument_name].push_back(
          rewritten_read->index);
      generated_calls.push_back(rewritten_read);

      auto value = store_assign->op1;
      if(!ir_helper::IsSameType(interface_datatype, ir_helper::CGetType(value)))
      {
         std::vector<ir_nodeRef> intermediate_stmts;
         const auto cast = createInterfaceCast(value, interface_datatype, ir_man, function_id, intermediate_stmts,
                                               write_argument_name);
         const auto block = sl->list_of_bloc.at(GetPointerS<const node_stmt>(store)->bb_index);
         for(const auto& intermediate_stmt : intermediate_stmts)
         {
            block->PushBefore(intermediate_stmt, store, AppM);
         }
         const auto cast_stmt =
             ir_man->CreateAssignStmt(interface_datatype, nullptr, nullptr, cast, function_id, BUILTIN_LOCINFO);
         block->PushBefore(cast_stmt, store, AppM);
         value = GetPointerS<const assign_stmt>(cast_stmt)->op0;
      }
      const auto store_call = ir_man->create_call_stmt(store_decl,
                                                       {TM->CreateUniqueIntegerCst(1, opcode_type),
                                                        TM->CreateUniqueIntegerCst(32, u32_type), value, write_address,
                                                        TM->CreateUniqueIntegerCst(0, u32_type)},
                                                       function_id, BUILTIN_LOCINFO);
      const auto store_access_block = sl->list_of_bloc.at(GetPointerS<const node_stmt>(store)->bb_index);
      store_access_block->Replace(store, store_call, true, AppM);
      preserveBlockingInterfaceAccess(store_call, store, bundle_name, channel_write_vdefs, ir_man);
      CallGraphManager::addCallPointAndExpand(call_points, AppM, function_id, store_decl->index, store_call->index,
                                              FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
      interface_io[GetPointerS<const node_stmt>(store_call)->bb_index][write_argument_name].push_back(
          store_call->index);
      generated_calls.push_back(store_call);

      const auto scalar_drain = ir_man->create_call_stmt(drain_decl,
                                                         {TM->CreateUniqueIntegerCst(3, opcode_type),
                                                          TM->CreateUniqueIntegerCst(32, u32_type),
                                                          TM->CreateUniqueIntegerCst(0, interface_datatype),
                                                          proof.write_base, TM->CreateUniqueIntegerCst(0, u32_type)},
                                                         function_id, BUILTIN_LOCINFO);
      const auto current_store_block = sl->list_of_bloc.at(GetPointerS<const node_stmt>(store_call)->bb_index);
      const auto& store_statements = current_store_block->CGetStmtList();
      const auto after_store = std::find_if(store_statements.begin(), store_statements.end(),
                                            [&](const auto& stmt) { return stmt->index == store_call->index; });
      const auto latch = splitBlockBeforeStatement(sl, current_store_block, *std::next(after_store), AppM, TM);
      insertConditionalCallDiamond(sl, current_store_block, latch, fast_mode, scalar_drain, function_id, AppM, TM);
      register_call(scalar_drain, drain_decl, write_argument_name, channel_write_vdefs);
      // Preserve configure-to-drain ordering; BuildVirtualPhi adds the loop-carried virtual PHI separately.
      addVirtualUseToStatement(scalar_read_configure, GetPointerS<const node_stmt>(scalar_drain)->vdef);

      const auto full_drain_block = make_block(parent_loop_id);
      append_call(full_drain_block, drain_decl,
                  {TM->CreateUniqueIntegerCst(3, opcode_type), TM->CreateUniqueIntegerCst(32, u32_type),
                   TM->CreateUniqueIntegerCst(0, interface_datatype), proof.write_base,
                   TM->CreateUniqueIntegerCst(0, u32_type)},
                  write_argument_name, channel_write_vdefs);
      const auto full_drain_join = make_block(parent_loop_id);
      const auto loop_exit = sl->list_of_bloc.at(write_region.exit);
      const auto latch_exit_branch = GetPointerS<multi_way_if_stmt>(latch->CGetStmtList().back());
      bool latch_exit_retargeted = false;
      for(auto& edge : latch_exit_branch->list_of_cond)
      {
         if(edge.second == loop_exit->number)
         {
            edge.second = full_drain_join->number;
            latch_exit_retargeted = true;
         }
      }
      THROW_ASSERT(latch_exit_retargeted, "Runtime duplex latch edge to the loop exit was not retargeted");
      (void)latch_exit_retargeted;
      std::replace(latch->list_of_succ.begin(), latch->list_of_succ.end(), loop_exit->number, full_drain_join->number);
      loop_exit->list_of_pred.erase(
          std::remove(loop_exit->list_of_pred.begin(), loop_exit->list_of_pred.end(), latch->number),
          loop_exit->list_of_pred.end());
      full_drain_join->list_of_pred.push_back(latch->number);
      full_drain_join->list_of_succ = {loop_exit->number, full_drain_block->number};
      full_drain_block->list_of_pred.push_back(full_drain_join->number);
      full_drain_block->list_of_succ.push_back(loop_exit->number);
      loop_exit->list_of_pred.push_back(full_drain_join->number);
      loop_exit->list_of_pred.push_back(full_drain_block->number);
      for(const auto& phi_node : loop_exit->CGetPhiList())
      {
         const auto phi = GetPointerS<phi_stmt>(phi_node);
         std::vector<phi_stmt::DefEdge> latch_edges;
         for(const auto& edge : phi->CGetDefEdgesList())
         {
            if(edge.second == latch->number)
            {
               latch_edges.push_back(edge);
            }
         }
         for(const auto& edge : latch_edges)
         {
            phi->ReplaceDefEdge(TM, edge, {edge.first, full_drain_join->number});
            phi->AddDefEdge(TM, {edge.first, full_drain_block->number});
         }
      }
      createBranch(TM, function_id, full_drain_join, fast_mode, full_drain_block->number, loop_exit->number, AppM);
      createBranch(TM, function_id, full_drain_block, ir_nodeRef(), loop_exit->number, loop_exit->number, AppM);
      return generated_calls;
   }

   void LowerReadBurstRegions(const statement_list_node* sl, const function_val_node* fd,
                              const std::vector<ReadBurstRegion>& regions, const ir_nodeRef& expected_base,
                              const std::string& bundle_name, const std::string& argument_name,
                              const ir_nodeConstRef& interface_datatype, const application_managerRef& AppM,
                              const ir_managerRef& TM, const ir_manipulationRef& ir_man,
                              std::map<std::string, ir_nodeRef>& channel_read_vdefs,
                              std::map<unsigned int, std::map<std::string, std::list<unsigned int>>>& interface_io,
                              std::set<std::string>& operationsR, std::set<unsigned int>& modified)
   {
      const auto add_to_modified = [&](const ir_nodeRef& tn) {
         modified.insert(GetPointer<node_stmt>(tn)->parent->index);
      };
      const auto configure_name = ENCODE_FDNAME(bundle_name, "_configure_read", "");
      const auto burst_read_name = ENCODE_FDNAME(bundle_name, "_burst_read", "");
      operationsR.insert(configure_name);
      operationsR.insert(burst_read_name);
      const auto opcode_type = ir_man->GetCustomIntegerType(2, true);
      const auto u32_type = ir_man->GetUnsignedIntegerType();
      const auto address_type = ir_helper::CGetType(expected_base);
      std::vector<ir_nodeConstRef> configure_arg_types{opcode_type, u32_type, interface_datatype, address_type,
                                                       u32_type};
      const auto configure_decl = ir_man->create_function_decl(configure_name, fd->parent, configure_arg_types,
                                                               ir_man->GetVoidType(), BUILTIN_LOCINFO, false);
      std::vector<ir_nodeConstRef> burst_read_arg_types{opcode_type, u32_type, interface_datatype, address_type,
                                                        u32_type};
      const auto burst_read_decl = ir_man->create_function_decl(burst_read_name, fd->parent, burst_read_arg_types,
                                                                interface_datatype, BUILTIN_LOCINFO, false);

      for(const auto& region : regions)
      {
         const auto preheader = sl->list_of_bloc.at(region.preheader);
         const auto branch = preheader->CGetStmtList().back();
         const auto count_ssa = CreateDuplexDirectionCount(ir_man, TM, AppM, fd->index, preheader, branch, region.guard,
                                                           region.loop_trip_bound, region.effective_bound,
                                                           region.has_direction_guard, u32_type);
         std::vector<ir_nodeRef> configure_args{
             TM->CreateUniqueIntegerCst(2, opcode_type), TM->CreateUniqueIntegerCst(32, u32_type),
             TM->CreateUniqueIntegerCst(0, interface_datatype), region.base, count_ssa};
         const auto configure_call =
             ir_man->create_call_stmt(configure_decl, configure_args, fd->index, BUILTIN_LOCINFO);
         preheader->PushBefore(configure_call, branch, AppM);
         CustomUnorderedSet<unsigned int> call_points;
         CallGraphManager::addCallPointAndExpand(call_points, AppM, fd->index, configure_decl->index,
                                                 configure_call->index, FunctionEdgeInfo::CallType::direct_call,
                                                 DEBUG_LEVEL_NONE);
         serializeInterfaceAccess(configure_call, bundle_name, channel_read_vdefs, ir_man);
         interface_io[region.preheader][argument_name].push_back(configure_call->index);
         add_to_modified(configure_call);

         for(const auto& read_stmt : region.reads)
         {
            auto read = GetPointerS<assign_stmt>(read_stmt);
            const auto mem_access = GetPointerS<const mem_access_node>(read->op1);
            std::vector<ir_nodeRef> burst_read_args{TM->CreateUniqueIntegerCst(0, opcode_type),
                                                    TM->CreateUniqueIntegerCst(32, u32_type),
                                                    TM->CreateUniqueIntegerCst(0, interface_datatype), mem_access->op,
                                                    TM->CreateUniqueIntegerCst(0, u32_type)};
            const auto read_expr = ir_man->CreateCallExpr(burst_read_decl, burst_read_args, BUILTIN_LOCINFO);
            ir_nodeRef rewritten_read = read_stmt;
            if(ir_helper::IsSameType(interface_datatype, ir_helper::CGetType(read->op0)))
            {
               TM->ReplaceIRNode(read_stmt, read->op1, read_expr);
            }
            else
            {
               const auto actual_type = ir_helper::CGetType(read->op0);
               const auto tmp_ssa = ir_man->create_ssa_name(nullptr, interface_datatype, nullptr, nullptr);
               rewritten_read = ir_man->create_assign_stmt(tmp_ssa, read_expr, fd->index, BUILTIN_LOCINFO);
               const auto access_bb = sl->list_of_bloc.at(region.access_bb);
               access_bb->Replace(read_stmt, rewritten_read, true, AppM);
               std::vector<ir_nodeRef> intermediate_stmts;
               const auto cast_expr =
                   createInterfaceCast(tmp_ssa, actual_type, ir_man, fd->index, intermediate_stmts, argument_name);
               auto last_stmt = rewritten_read;
               for(const auto& intermediate_stmt : intermediate_stmts)
               {
                  access_bb->PushAfter(intermediate_stmt, last_stmt, AppM);
                  last_stmt = intermediate_stmt;
               }
               const auto cast_stmt = ir_man->create_assign_stmt(read->op0, cast_expr, fd->index, BUILTIN_LOCINFO);
               access_bb->PushAfter(cast_stmt, last_stmt, AppM);
            }
            CallGraphManager::addCallPointAndExpand(call_points, AppM, fd->index, burst_read_decl->index,
                                                    rewritten_read->index, FunctionEdgeInfo::CallType::direct_call,
                                                    DEBUG_LEVEL_NONE);
            serializeInterfaceAccess(rewritten_read, bundle_name, channel_read_vdefs, ir_man);
            interface_io[GetPointerS<const node_stmt>(rewritten_read)->bb_index][argument_name].push_back(
                rewritten_read->index);
            add_to_modified(rewritten_read);
         }
      }
   }

   void LowerWriteBurstRegion(const statement_list_node* sl, const function_val_node* fd,
                              const WriteBurstRegion& region, const ir_nodeRef& expected_base,
                              const std::string& bundle_name, const std::string& argument_name,
                              const ir_nodeConstRef& interface_datatype, const application_managerRef& AppM,
                              const ir_managerRef& TM, const ir_manipulationRef& ir_man,
                              std::map<std::string, ir_nodeRef>& channel_write_vdefs,
                              std::map<unsigned int, std::map<std::string, std::list<unsigned int>>>& interface_io,
                              std::set<std::string>& operationsW, std::set<unsigned int>& modified)
   {
      const auto add_to_modified = [&](const ir_nodeRef& tn) {
         modified.insert(GetPointer<node_stmt>(tn)->parent->index);
      };
      const auto configure_name = ENCODE_FDNAME(bundle_name, "_configure_write", "");
      const auto store_name = ENCODE_FDNAME(bundle_name, "_store_write", "");
      const auto drain_name = ENCODE_FDNAME(bundle_name, "_drain_write", "");
      operationsW.insert(configure_name);
      operationsW.insert(store_name);
      operationsW.insert(drain_name);
      const auto opcode_type = ir_man->GetCustomIntegerType(2, true);
      const auto u32_type = ir_man->GetUnsignedIntegerType();
      const auto address_type = ir_helper::CGetType(expected_base);
      const std::vector<ir_nodeConstRef> command_arg_types{opcode_type, u32_type, interface_datatype, address_type,
                                                           u32_type};
      const auto configure_decl = ir_man->create_function_decl(configure_name, fd->parent, command_arg_types,
                                                               ir_man->GetVoidType(), BUILTIN_LOCINFO, false);
      const auto store_decl = ir_man->create_function_decl(store_name, fd->parent, command_arg_types,
                                                           ir_man->GetVoidType(), BUILTIN_LOCINFO, false);
      const auto drain_decl = ir_man->create_function_decl(drain_name, fd->parent, command_arg_types,
                                                           ir_man->GetVoidType(), BUILTIN_LOCINFO, false);
      const auto preheader = sl->list_of_bloc.at(region.preheader);
      const auto preheader_branch = preheader->CGetStmtList().back();
      const auto count = CreateDuplexDirectionCount(ir_man, TM, AppM, fd->index, preheader, preheader_branch,
                                                    region.guard, region.loop_trip_bound, region.effective_bound,
                                                    region.has_direction_guard, u32_type);
      const std::vector<ir_nodeRef> configure_args{
          TM->CreateUniqueIntegerCst(2, opcode_type), TM->CreateUniqueIntegerCst(32, u32_type),
          TM->CreateUniqueIntegerCst(0, interface_datatype), region.base, count};
      const auto configure_call = ir_man->create_call_stmt(configure_decl, configure_args, fd->index, BUILTIN_LOCINFO);
      preheader->PushBefore(configure_call, preheader_branch, AppM);
      CustomUnorderedSet<unsigned int> call_points;
      CallGraphManager::addCallPointAndExpand(call_points, AppM, fd->index, configure_decl->index,
                                              configure_call->index, FunctionEdgeInfo::CallType::direct_call,
                                              DEBUG_LEVEL_NONE);
      serializeInterfaceAccess(configure_call, bundle_name, channel_write_vdefs, ir_man);
      interface_io[region.preheader][argument_name].push_back(configure_call->index);
      add_to_modified(configure_call);

      const auto store = GetPointerS<const assign_stmt>(region.store);
      auto value = store->op1;
      if(!ir_helper::IsSameType(interface_datatype, ir_helper::CGetType(value)))
      {
         std::vector<ir_nodeRef> intermediate_stmts;
         const auto cast =
             createInterfaceCast(value, interface_datatype, ir_man, fd->index, intermediate_stmts, argument_name);
         const auto access_bb = sl->list_of_bloc.at(region.access_bb);
         for(const auto& intermediate_stmt : intermediate_stmts)
         {
            access_bb->PushBefore(intermediate_stmt, region.store, AppM);
         }
         const auto cast_stmt =
             ir_man->CreateAssignStmt(interface_datatype, nullptr, nullptr, cast, fd->index, BUILTIN_LOCINFO);
         access_bb->PushBefore(cast_stmt, region.store, AppM);
         value = GetPointerS<const assign_stmt>(cast_stmt)->op0;
      }
      const auto address = GetPointerS<const mem_access_node>(store->op0)->op;
      const std::vector<ir_nodeRef> store_args{TM->CreateUniqueIntegerCst(1, opcode_type),
                                               TM->CreateUniqueIntegerCst(32, u32_type), value, address,
                                               TM->CreateUniqueIntegerCst(0, u32_type)};
      const auto store_call = ir_man->create_call_stmt(store_decl, store_args, fd->index, BUILTIN_LOCINFO);
      const auto access_bb = sl->list_of_bloc.at(region.access_bb);
      access_bb->Replace(region.store, store_call, true, AppM);
      preserveBlockingInterfaceAccess(store_call, region.store, bundle_name, channel_write_vdefs, ir_man);
      CallGraphManager::addCallPointAndExpand(call_points, AppM, fd->index, store_decl->index, store_call->index,
                                              FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
      interface_io[region.access_bb][argument_name].push_back(store_call->index);
      add_to_modified(store_call);

      const auto exit = sl->list_of_bloc.at(region.exit);
      const auto drain_call = ir_man->create_call_stmt(
          drain_decl,
          {TM->CreateUniqueIntegerCst(3, opcode_type), TM->CreateUniqueIntegerCst(32, u32_type),
           TM->CreateUniqueIntegerCst(0, interface_datatype), region.base, TM->CreateUniqueIntegerCst(0, u32_type)},
          fd->index, BUILTIN_LOCINFO);
      if(exit->CGetStmtList().empty())
      {
         exit->PushBack(drain_call, AppM);
      }
      else
      {
         exit->PushBefore(drain_call, exit->CGetStmtList().front(), AppM);
      }
      CallGraphManager::addCallPointAndExpand(call_points, AppM, fd->index, drain_decl->index, drain_call->index,
                                              FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
      serializeInterfaceAccess(drain_call, bundle_name, channel_write_vdefs, ir_man);
      interface_io[region.exit][argument_name].push_back(drain_call->index);
      add_to_modified(drain_call);
   }

   void ApplyAXIBurstDecision(const AXIBurstRewriteContext& context)
   {
      const auto* decision = context.decision;
      const auto& fd = context.fd;
      const auto root_id = context.root_id;
      const auto argument_id = context.argument_id;
      const auto address_bitsize = context.address_bitsize;
      const auto& argument_name = context.argument_name;
      const auto& bundle_name = context.bundle_name;
      const auto& function_name = context.function_name;
      const auto bitwidth = context.bitwidth;
      const auto alignment = context.alignment;
      const bool isRead = context.is_read;
      const bool isWrite = context.is_write;
      const bool burst_enabled = context.burst_feature_enabled;
      const bool same_base_duplex_active = context.same_base_duplex_active;
      const auto& iface_attrs = context.iface_attrs;
      const auto& interface_datatype = context.interface_datatype;
      const auto& AppM = context.AppM;
      const auto& TM = context.TM;
      const auto& ir_man = context.ir_man;
      auto& read_statements = context.read_statements;
      auto& write_statements = context.write_statements;
      auto& channel_read_vdefs = context.channel_read_vdefs;
      auto& channel_write_vdefs = context.channel_write_vdefs;
      auto& interface_io = context.interface_io;
      auto& operations_read = context.operations_read;
      auto& operations_write = context.operations_write;
      auto& modified = context.modified;
      const auto output_level = context.output_level;
      const bool write_burst_requested =
          burst_enabled && decision && decision->write_requested && !context.force_legacy;
      const bool burst_requested = burst_enabled && decision && decision->read_requested && !context.force_legacy;
      const bool runtime_guarded_duplex =
          same_base_duplex_active && decision->same_base_duplex_proof->runtime_range_guard;
      if(same_base_duplex_active)
      {
#if HAVE_ASSERTS
         const auto& proof = *decision->same_base_duplex_proof;
         const auto read_regions = decision->read_regions.find(proof.read_owner_argument_index);
         const auto write_region = decision->write_regions.find(proof.write_owner_argument_index);
         const auto same_optional_node = [](const ir_nodeRef& lhs, const ir_nodeRef& rhs) {
            return (!lhs && !rhs) || (lhs && rhs && lhs->index == rhs->index);
         };
#endif
         THROW_ASSERT(
             read_regions != decision->read_regions.end() && read_regions->second.size() == 1 &&
                 write_region != decision->write_regions.end() &&
                 read_regions->second.front().preheader == proof.read_region.preheader &&
                 read_regions->second.front().header == proof.read_region.header &&
                 read_regions->second.front().bound->index == proof.read_region.bound->index &&
                 read_regions->second.front().loop_trip_bound->index == proof.read_region.loop_trip_bound->index &&
                 same_optional_node(read_regions->second.front().loop_trip_compare,
                                    proof.read_region.loop_trip_compare) &&
                 read_regions->second.front().effective_bound->index == proof.read_region.effective_bound->index &&
                 same_optional_node(read_regions->second.front().effective_compare,
                                    proof.read_region.effective_compare) &&
                 read_regions->second.front().has_direction_guard == proof.read_region.has_direction_guard &&
                 read_regions->second.front().access_bb == proof.read_region.access_bb &&
                 write_region->second.preheader == proof.write_region.preheader &&
                 write_region->second.header == proof.write_region.header &&
                 write_region->second.exit == proof.write_region.exit &&
                 write_region->second.bound->index == proof.write_region.bound->index &&
                 write_region->second.loop_trip_bound->index == proof.write_region.loop_trip_bound->index &&
                 same_optional_node(write_region->second.loop_trip_compare, proof.write_region.loop_trip_compare) &&
                 write_region->second.effective_bound->index == proof.write_region.effective_bound->index &&
                 same_optional_node(write_region->second.effective_compare, proof.write_region.effective_compare) &&
                 write_region->second.has_direction_guard == proof.write_region.has_direction_guard &&
                 write_region->second.access_bb == proof.write_region.access_bb,
             "Both frozen duplex regions must be validated before either direction is rewritten");
      }
      bool burst_profile_eligible = true;
      std::string burst_fallback_reason = burst_requested ? decision->read_fallback_reason : "";
      if(burst_requested)
      {
         if(burst_fallback_reason.empty() && (bitwidth != 32 || alignment < 4))
         {
            burst_fallback_reason = "data width/alignment is not 32-bit/4-byte";
         }
         else if(burst_fallback_reason.empty() &&
                 iface_attrs.find(FunctionArchitecture::iface_bitwidth) != iface_attrs.end() &&
                 std::stoul(iface_attrs.at(FunctionArchitecture::iface_bitwidth)) != 32)
         {
            burst_fallback_reason = "m_axi interface data width is not 32-bit";
         }
         else if(burst_fallback_reason.empty() && address_bitsize != 32 && address_bitsize != 64)
         {
            burst_fallback_reason = "AXI address width is not 32 or 64 bit";
         }
         else if(burst_fallback_reason.empty() && !decision->read_recognized)
         {
            burst_fallback_reason = "read-burst CFG/SSA recognition failed or bundle is not exclusive";
         }
         burst_profile_eligible = burst_fallback_reason.empty();
         if(!burst_profile_eligible && isRead)
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---Keeping legacy m_axi accesses for bundle " + bundle_name + ": " + burst_fallback_reason);
         }
      }
      if(runtime_guarded_duplex && argument_id == decision->same_base_duplex_proof->read_owner_argument_index)
      {
         THROW_ASSERT(burst_profile_eligible && decision->write_recognized,
                      "Frozen runtime duplex profile failed direction eligibility");
         const auto lowered_calls = lowerRuntimeGuardedDuplex(
             GetPointerS<const statement_list_node>(fd->body), fd, *decision->same_base_duplex_proof, bundle_name,
             get_decl_name(TM->GetIRNode(decision->same_base_duplex_proof->read_owner_argument_index)),
             get_decl_name(TM->GetIRNode(decision->same_base_duplex_proof->write_owner_argument_index)),
             interface_datatype, address_bitsize, AppM, TM, ir_man, channel_read_vdefs, channel_write_vdefs,
             interface_io);
         for(const auto& call : lowered_calls)
         {
            modified.insert(GetPointer<node_stmt>(call)->parent->index);
         }
         read_statements.clear();
         operations_read.insert(ENCODE_FDNAME(bundle_name, "_configure_read", ""));
         operations_read.insert(ENCODE_FDNAME(bundle_name, "_burst_read", ""));
         operations_write.insert(ENCODE_FDNAME(bundle_name, "_configure_write", ""));
         operations_write.insert(ENCODE_FDNAME(bundle_name, "_store_write", ""));
         operations_write.insert(ENCODE_FDNAME(bundle_name, "_drain_write", ""));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Activated guarded distinct-base duplex for bundle " + bundle_name +
                            " (overlap uses serialized scalar transfers)");
      }
      else if(!runtime_guarded_duplex && burst_requested && burst_profile_eligible && isRead)
      {
         const auto sl = GetPointerS<const statement_list_node>(fd->body);
         const auto expected_base_id = AppM->getSSAFromParm(root_id, argument_id);
         const auto expected_base = TM->GetIRNode(expected_base_id);
         const auto recognized_regions = decision->read_regions.find(argument_id);
         const bool recognized = decision->read_recognized && recognized_regions != decision->read_regions.end();
         if(recognized)
         {
            const auto& regions = recognized_regions->second;
            const auto max_burst = decision->read_max_burst_length;
            THROW_ASSERT(max_burst >= 1 && max_burst <= 256, "Invalid parsed max_read_burst_length");
            LowerReadBurstRegions(sl, fd, regions, expected_base, bundle_name, argument_name, interface_datatype, AppM,
                                  TM, ir_man, channel_read_vdefs, interface_io, operations_read, modified);
            read_statements.clear();
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---Configured read burst regions for bundle " + bundle_name +
                               " (B=" + std::to_string(max_burst) + ")");
         }
         else
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---Keeping legacy m_axi reads for bundle " + bundle_name +
                               ": read-burst CFG/SSA recognition failed or bundle is not exclusive");
         }
      }
      if(runtime_guarded_duplex && argument_id == decision->same_base_duplex_proof->write_owner_argument_index)
      {
         write_statements.clear();
      }
      else if(!runtime_guarded_duplex && write_burst_requested && isWrite)
      {
         std::string write_fallback_reason = decision->write_fallback_reason;
         if(write_fallback_reason.empty() &&
            (bitwidth != 32 || alignment < 4 ||
             (iface_attrs.find(FunctionArchitecture::iface_bitwidth) != iface_attrs.end() &&
              std::stoul(iface_attrs.at(FunctionArchitecture::iface_bitwidth)) != 32)))
         {
            write_fallback_reason = "data width/alignment is not 32-bit/4-byte";
         }
         else if(write_fallback_reason.empty() && address_bitsize != 32 && address_bitsize != 64)
         {
            write_fallback_reason = "AXI address width is not 32 or 64 bit";
         }
         else if(write_fallback_reason.empty() && !decision->write_recognized)
         {
            write_fallback_reason = "write-burst CFG/SSA/effect recognition failed";
         }
         const auto sl = GetPointerS<const statement_list_node>(fd->body);
         const auto expected_base_id = AppM->getSSAFromParm(root_id, argument_id);
         const auto expected_base = TM->GetIRNode(expected_base_id);
         const auto recognized_region = decision->write_regions.find(argument_id);
         const bool recognized = write_fallback_reason.empty() && decision->write_recognized &&
                                 recognized_region != decision->write_regions.end();
         if(!recognized && write_fallback_reason.empty())
         {
            write_fallback_reason = "write-burst CFG/SSA/effect recognition failed";
         }
         if(recognized)
         {
            const auto& region = recognized_region->second;
            LowerWriteBurstRegion(sl, fd, region, expected_base, bundle_name, argument_name, interface_datatype, AppM,
                                  TM, ir_man, channel_write_vdefs, interface_io, operations_write, modified);
            write_statements.clear();
            if(same_base_duplex_active)
            {
               INDENT_OUT_MEX(
                   OUTPUT_LEVEL_MINIMUM, output_level,
                   std::string(decision->same_base_duplex_proof->runtime_range_guard ?
                                   "---Activated distinct-base runtime-range-guarded pointwise duplex m_axi bundle " :
                               decision->same_base_duplex_proof->distinct_bases ?
                                   "---Activated distinct-base restricted pointwise duplex m_axi bundle " :
                                   "---Activated same-base pointwise duplex m_axi bundle ") +
                       bundle_name + " for root " + function_name);
               if(decision->same_base_duplex_proof->distinct_bases &&
                  !decision->same_base_duplex_proof->runtime_range_guard)
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                                 "WARNING: distinct-base duplex relies on the caller's __restrict contract; "
                                 "the caller must ensure the accessed regions do not overlap (no runtime "
                                 "range validation is performed)");
               }
            }
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---Configured write burst region for bundle " + bundle_name +
                               " (B=" + std::to_string(decision->write_profile.max_burst_length) + ")");
         }
         else
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---Keeping legacy m_axi writes for bundle " + bundle_name + ": " + write_fallback_reason);
         }
      }
   }

} // namespace InterfaceInferInternal
