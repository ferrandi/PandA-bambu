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
 * @author Giulio Stramondo
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 */
#include "Bit_Value.hpp"

#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"

#include <boost/range/adaptors.hpp>

std::deque<bit_lattice> Bit_Value::get_current_or_best(const ir_nodeRef& tn) const
{
   const auto nid = tn->index;
   const auto node = tn;
   if(node->get_kind() == ssa_node_K && current.find(nid) != current.end())
   {
      return current.at(nid);
   }
   THROW_ASSERT(best.count(nid), "");
   return best.at(nid);
}

std::deque<bit_lattice> Bit_Value::backward_chain(const ir_nodeRef& ssa_ref) const
{
   const auto ssa = GetPointerS<const ssa_node>(ssa_ref);
   const auto ssa_nid = ssa->index;
   std::deque<bit_lattice> res = create_x_bitstring(1);
   for(const auto& stmt_use : ssa->CGetUseStmts())
   {
      const auto user_stmt = stmt_use.first;
      const auto user_kind = user_stmt->get_kind();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing use - " + STR(user_stmt));
      std::deque<bit_lattice> user_res;
      if(user_kind == assign_stmt_K)
      {
         const auto ga = GetPointerS<const assign_stmt>(user_stmt);
         if(!IsHandledByBitvalue(ga->op0))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---variable " + STR(ga->op0) + " of type " + STR(ir_helper::CGetType(ga->op0)) +
                               " not considered");
            user_res = create_u_bitstring(ir_helper::TypeSize(ssa_ref));
         }
         else if(ga->predicate && ga->predicate->index == ssa_nid)
         {
            user_res = create_u_bitstring(ir_helper::TypeSize(ssa_ref));
         }
         else
         {
            user_res = backward_transfer(ga, ssa_ref);
         }
      }
      else if(user_kind == phi_stmt_K)
      {
         const auto gp = GetPointerS<const phi_stmt>(user_stmt);
         if(!gp->virtual_flag)
         {
            user_res = get_current_or_best(gp->res);
         }
      }
      else if(user_kind == return_stmt_K)
      {
         THROW_ASSERT(GetPointerS<const return_stmt>(user_stmt)->op,
                      "ssa id " + STR(ssa_nid) +
                          "used in empty return statement: " + STR(GetPointerS<const return_stmt>(user_stmt)));
         const auto res_it = current.find(function_id);
         if(res_it != current.end())
         {
            user_res = res_it->second;
         }
      }
      else if(user_kind == call_stmt_K)
      {
         const auto gc = GetPointerS<const call_stmt>(user_stmt);
         if(gc->predicate && gc->predicate->index == ssa_nid)
         {
            user_res = create_u_bitstring(ir_helper::TypeSize(ssa_ref));
         }
         else
         {
            const auto call_it = direct_call_id_to_called_id.find(gc->index);
            if(call_it != direct_call_id_to_called_id.end())
            {
               const auto called_id = call_it->second;
               const auto called_tn = TM->GetIRNode(called_id);
               const auto called_fd = GetPointerS<const function_val_node>(called_tn);

               const auto& actual_parms = gc->args;
               const auto& formal_parms = called_fd->list_of_args;
               THROW_ASSERT(actual_parms.size() == formal_parms.size(), "");
               auto a_it = actual_parms.cbegin();
               const auto a_end = actual_parms.cend();
               auto f_it = formal_parms.cbegin();
               const auto f_end = formal_parms.cend();
               auto found = false;
               for(; a_it != a_end && f_it != f_end; a_it++, f_it++)
               {
                  if((*a_it)->index == ssa_nid)
                  {
                     const auto p_decl_id = AppM->getSSAFromParm(called_id, (*f_it)->index);
                     const auto parmssa = TM->GetIRNode(p_decl_id);
                     const auto pd = GetPointerS<const ssa_node>(parmssa);
                     std::deque<bit_lattice> tmp;
                     if(pd->bit_values.empty())
                     {
                        tmp = create_u_bitstring(ir_helper::TypeSize(parmssa));
                     }
                     else
                     {
                        tmp = string_to_bitstring(pd->bit_values);
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---param: " + bitstring_to_string(tmp));
                     user_res = found ? inf(user_res, tmp, ssa_ref) : tmp;
                     found = true;
                  }
               }
               THROW_ASSERT(found, STR(ssa) + " is not an actual parameter of function " + STR(called_id));
            }
         }
      }
      else if(user_kind == multi_way_if_stmt_K)
      {
         user_res = create_u_bitstring(ir_helper::TypeSize(ssa_ref));
      }
      else
      {
         THROW_UNREACHABLE("Unhandled statement: " + STR(user_stmt) + "(" + user_stmt->get_kind_text() + ")");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---res : " + bitstring_to_string(user_res));
      if(user_res.size())
      {
         res = inf(res, user_res, ssa_ref);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---inf : " + bitstring_to_string(res));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed use - " + STR(user_stmt));
      }
      else
      {
         THROW_ASSERT(best.count(ssa_nid), "");
         res = best.at(ssa_nid);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---best: " + bitstring_to_string(res));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed use - " + STR(user_stmt));
         break;
      }
   }
   return res;
}

void Bit_Value::backward()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Performing backward transfer");
   std::deque<ir_nodeConstRef> working_list;
   CustomUnorderedSet<unsigned int> working_list_idx;
   const auto push_back = [&](const ir_nodeConstRef& stmt) {
      const auto stmt_kind = stmt->get_kind();
      if(!working_list_idx.count(stmt->index) && (stmt_kind == assign_stmt_K || stmt_kind == phi_stmt_K))
      {
         working_list.push_back(stmt);
         working_list_idx.insert(stmt->index);
      }
   };
   const auto pop_front = [&]() -> ir_nodeConstRef {
      const auto stmt = working_list.front();
      working_list.pop_front();
      working_list_idx.erase(stmt->index);
      return stmt;
   };
   for(const auto& bb : boost::adaptors::reverse(bb_topological))
   {
      for(const auto& stmt : boost::adaptors::reverse(bb->CGetStmtList()))
      {
         const auto s = stmt;
         push_back(s);
         THROW_ASSERT(GetPointerS<const node_stmt>(s)->bb_index == bb->number,
                      "BB" + STR(bb->number) + " contains statement from BB" +
                          STR(GetPointerS<const node_stmt>(s)->bb_index) + " - " + s->get_kind_text() + " - " + STR(s));
      }
      for(const auto& stmt : boost::adaptors::reverse(bb->CGetPhiList()))
      {
         const auto s = stmt;
         const auto gp = GetPointerS<const phi_stmt>(s);
         if(!gp->virtual_flag)
         {
            if(IsHandledByBitvalue(gp->res))
            {
               push_back(s);
               THROW_ASSERT(GetPointerS<const node_stmt>(s)->bb_index == bb->number,
                            "BB" + STR(bb->number) + " contains statement from BB" +
                                STR(GetPointerS<const node_stmt>(s)->bb_index) + " - " + s->get_kind_text() + " - " +
                                STR(s));
            }
         }
      }
   }
   while(!working_list.empty())
   {
      const auto stmt = pop_front();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing statement " + STR(stmt));
      const auto stmt_kind = stmt->get_kind();
      ir_nodeRef lhs;
      if(stmt_kind == assign_stmt_K)
      {
         lhs = GetPointerS<const assign_stmt>(stmt)->op0;
      }
      else if(stmt_kind == phi_stmt_K)
      {
         lhs = GetPointerS<const phi_stmt>(stmt)->res;
      }
      else
      {
         THROW_UNREACHABLE("Unexpected statement kind: " + stmt->get_kind_text() + " - " + STR(stmt));
      }
      if(lhs->get_kind() == ssa_node_K)
      {
         if(!IsHandledByBitvalue(lhs))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "<--variable " + STR(lhs) + " of type " + STR(ir_helper::CGetType(lhs)) + " not considered");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Propagation for " + STR(lhs));
         auto res = backward_chain(lhs);
         if(update_current(res, lhs))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---current updated: " + bitstring_to_string(current.at(lhs->index)));
            std::vector<std::tuple<unsigned int, unsigned int>> vars_read;
            ir_helper::get_required_values(vars_read, stmt);
            for(const auto& var_pair : vars_read)
            {
               const auto in_ssa_nid = std::get<0>(var_pair);
               if(in_ssa_nid == 0)
               {
                  continue;
               }
               const auto in_ssa = TM->GetIRNode(in_ssa_nid);
               if(!IsHandledByBitvalue(in_ssa))
               {
                  continue;
               }
               if(in_ssa->get_kind() == ssa_node_K)
               {
                  const auto ssa_var = GetPointerS<const ssa_node>(in_ssa);
                  const auto nextNode = ssa_var->GetDefStmt();
                  push_back(nextNode);
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + STR(stmt));
   }

   const auto tn = TM->GetIRNode(function_id);
   const auto fd = GetPointerS<const function_val_node>(tn);
   for(const auto& parm_decl_node : fd->list_of_args)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing argument " + STR(parm_decl_node));
      const auto parmssa_id = AppM->getSSAFromParm(function_id, parm_decl_node->index);
      const auto parmssa = TM->GetIRNode(parmssa_id);
      if(!IsHandledByBitvalue(parmssa))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "<--argument " + STR(parmssa) + " of type " + STR(ir_helper::CGetType(parmssa)) +
                            " not considered id: " + STR(parmssa_id));
         continue;
      }
      auto res = get_current_or_best(parmssa);
      if(bitstring_constant(res))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "<--argument has been proven to be constant: " + STR(parmssa));
         continue;
      }
      res = backward_chain(parmssa);
      THROW_ASSERT(res.size(), "");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---res: " + bitstring_to_string(res));
      update_current(res, parmssa);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed argument " + STR(parm_decl_node));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Performed backward transfer");
}

std::deque<bit_lattice> Bit_Value::backward_transfer(const assign_stmt* ga, const ir_nodeRef& res_tn) const
{
   std::deque<bit_lattice> res;
   const auto res_nid = res_tn->index;
   if(ir_helper::IsConstant(res_tn))
   {
      return res;
   }
   THROW_ASSERT(best.count(res_nid), "");
   if(bitstring_constant(best.at(res_nid)))
   {
      THROW_ASSERT(best.count(res_nid), "");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---backward transfer, skipping constant target");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- res: " + bitstring_to_string(best.at(res_nid)));
      return best.at(res_nid);
   }
   const auto& lhs = ga->op0;
   const auto lhs_size = ir_helper::TypeSize(lhs);
   const auto lhs_kind = lhs->get_kind();

   switch(lhs_kind)
   {
      case mem_access_node_K:
      {
         const auto operation = GetPointerS<const mem_access_node>(ga->op0);
         if(operation->op->index == res_nid && current.count(operation->op->index))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS equals operand");
            return create_u_bitstring(pointer_resizing(res_tn));
         }
         return res;
      }
      case ssa_node_K:
      {
         break;
      }
      case abs_node_K:
      case addr_node_K:
      case not_node_K:
      case fptoi_node_K:
      case unaligned_mem_access_node_K:
      case neg_node_K:
      case nop_node_K:
      case bitcast_node_K:
      case itofp_node_K:
      case CASE_BINARY_NODES:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_FAKE_NODES:
      case CASE_DECL_NODES:
      case CASE_CST_NODES:
      case CASE_NODE_STMTS:
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case lut_node_K:
      case statement_list_node_K:
      default:
      {
         THROW_UNREACHABLE("Unhandled lhs expression: " + ga->ToString() + " (" + ir_node::GetString(lhs_kind) + ")");
         break;
      }
   }

   const auto lhs_bitstring = get_current_or_best(lhs);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---backward_transfer, lhs: " + bitstring_to_string(lhs_bitstring));
   const auto& rhs = ga->op1;
   const auto rhs_kind = rhs->get_kind();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- => " + ir_node::GetString(rhs_kind));
   switch(rhs_kind)
   {
      // Unary expressions
      case addr_node_K:
      case not_node_K:
      case mem_access_node_K:
      case neg_node_K:
      case nop_node_K:
      case bitcast_node_K:
      {
         const auto operation = GetPointerS<const unary_node>(rhs);

         const auto op_nid = operation->op->index;
         THROW_ASSERT(res_nid == op_nid, "Invalid operand: " + STR(res_nid) + " (" + ga->ToString() + ")");
         if(!IsHandledByBitvalue(operation->op))
         {
            break;
         }
         THROW_ASSERT(best.count(op_nid), "");
         auto op_bitstring = best.at(op_nid);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---   operand(" + STR(op_nid) + "): " + bitstring_to_string(op_bitstring));

         if(rhs_kind == addr_node_K)
         {
            if(ga->temporary_address)
            {
               const auto op_kind = operation->op->get_kind();
               if(op_kind == mem_access_node_K)
               {
                  const auto mr = GetPointerS<const mem_access_node>(operation->op);
                  if(mr->op->index == res_nid && current.count(mr->op->index))
                  {
                     return create_u_bitstring(pointer_resizing(ga->op0));
                  }
               }
            }
         }
         else if(rhs_kind == mem_access_node_K)
         {
            res = create_u_bitstring(pointer_resizing(res_tn));
         }
         else if(rhs_kind == not_node_K)
         {
            const auto lhs_bitsize = lhs_bitstring.size();

            auto se_lhs_bitstring = lhs_bitstring;
            const auto initial_size = op_bitstring.size();
            if(initial_size < lhs_bitsize)
            {
               op_bitstring = sign_extend_bitstring(op_bitstring, IsSignedIntegerType(operation->op), lhs_bitsize);
            }
            if(initial_size > lhs_bitsize)
            {
               se_lhs_bitstring = sign_extend_bitstring(lhs_bitstring, IsSignedIntegerType(lhs), initial_size);
            }

            auto it_lhs_bitstring = se_lhs_bitstring.rbegin();
            auto it_op_bitstring = op_bitstring.rbegin();
            for(; it_lhs_bitstring != se_lhs_bitstring.rend() && it_op_bitstring != op_bitstring.rend();
                ++it_lhs_bitstring, ++it_op_bitstring)
            {
               if(*it_lhs_bitstring == bit_lattice::X)
               {
                  res.push_front(bit_lattice::X);
               }
               else
               {
                  res.push_front(*it_op_bitstring);
               }
            }
         }
         else if(rhs_kind == nop_node_K || rhs_kind == bitcast_node_K)
         {
            const bool lhs_signed = IsSignedIntegerType(lhs);
            const bool op_signed = IsSignedIntegerType(operation->op);
            const auto op_size = ir_helper::TypeSize(operation->op);
            if(op_signed && !lhs_signed)
            {
               /*
                * ###################################################################
                * WARNING!! do not remove this condition!
                * the backward propagation of casts cannot be performed when the lhs
                * is unsigned and the rhs is signed.
                * the reason is that bitstrings attached to unsigned do not carry
                * around implicit information on sign bits. on the contrary,
                * bitstrings attached to signed carry this kind of implicit
                * information. if we propagate back from unsigned to signed there are
                * some corner cases when the sign bit information on the rhs would be
                * overwritten by dontcares in the lhs unsigned string, leading to
                * nasty propagations bugs, that occur rarely and only in very complex
                * tests, and are very hard to track down.
                * ###################################################################
                */
               break;
            }

            res = lhs_bitstring;
            if(res.size() < lhs_size)
            {
               res = sign_extend_bitstring(res, lhs_signed, lhs_size);
            }
            if(lhs_size > op_size)
            {
               if(op_size < 32 && op_size > 1)
               {
                  const auto sign_bit = res.front();
                  res.pop_front();
                  while(res.size() > op_size)
                  {
                     res.pop_front();
                  }
                  res.front() = bit_inf(sign_bit, res.front());
               }
               else
               {
                  while(res.size() > op_size)
                  {
                     res.pop_front();
                  }
               }
            }
         }
         else if(rhs_kind == neg_node_K)
         {
            res = op_bitstring;
            const auto initial_size = op_bitstring.size();
            auto res_size = ir_helper::TypeSize(res_tn);
            if(lhs_bitstring.front() == bit_lattice::U)
            {
               res_size = std::min(res_size, static_cast<unsigned long long>(lhs_bitstring.size()));
            }
            if(lhs_bitstring.front() == bit_lattice::X)
            {
               res_size = std::min(res_size, static_cast<unsigned long long>(lhs_bitstring.size() - 1));
            }
            while(res.size() > res_size)
            {
               res.pop_front();
            }
            if(res.size() != initial_size)
            {
               res.push_front(bit_lattice::X);
            }
         }
         else
         {
            THROW_UNREACHABLE("Unhadled unary expression: " + ga->ToString() + "(" + ir_node::GetString(rhs_kind) +
                              ")");
         }
         break;
      }
      // Binary expressions
      case and_node_K:
      case or_node_K:
      case xor_node_K:
      case extract_bit_node_K:
      case shl_node_K:
      case sub_node_K:
      case mul_node_K:
      case add_node_K:
      case gep_node_K:
      case shr_node_K:
      case widen_mul_node_K:
      {
         const auto operation = GetPointerS<const binary_node>(rhs);

         auto op0_nid = operation->op0->index;
         THROW_ASSERT(best.count(op0_nid), "");
         auto op0_bitstring = best.at(op0_nid);

         auto op1_nid = operation->op1->index;
         THROW_ASSERT(best.count(op1_nid), "");
         auto op1_bitstring = best.at(op1_nid);

         THROW_ASSERT(res_nid == op0_nid || res_nid == op1_nid,
                      "Invalid operand: " + STR(res_nid) + " (" + ga->ToString() + ")");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---   operand0(" + STR(op0_nid) + "): " + bitstring_to_string(op0_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---   operand1(" + STR(op1_nid) + "): " + bitstring_to_string(op1_bitstring));

         if(rhs_kind == and_node_K || rhs_kind == or_node_K || rhs_kind == xor_node_K)
         {
            const auto lhs_bitsize = lhs_bitstring.size();

            if(op0_nid != res_nid)
            {
               std::swap(op0_nid, op1_nid);
               std::swap(op0_bitstring, op1_bitstring);
            }

            auto se_lhs_bitstring = lhs_bitstring;
            const auto initial_size = op0_bitstring.size();
            if(initial_size < lhs_bitsize)
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, IsSignedIntegerType(operation->op0), lhs_bitsize);
            }
            if(initial_size > lhs_bitsize)
            {
               se_lhs_bitstring = sign_extend_bitstring(lhs_bitstring, IsSignedIntegerType(lhs), initial_size);
            }
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring =
                   sign_extend_bitstring(op1_bitstring, IsSignedIntegerType(operation->op1), op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring =
                   sign_extend_bitstring(op0_bitstring, IsSignedIntegerType(operation->op0), op1_bitstring.size());
            }

            auto it_lhs_bitstring = se_lhs_bitstring.rbegin();
            auto it_op0_bitstring = op0_bitstring.rbegin();
            auto it_op1_bitstring = op1_bitstring.rbegin();
            for(; it_lhs_bitstring != se_lhs_bitstring.rend() && it_op0_bitstring != op0_bitstring.rend();
                ++it_lhs_bitstring, ++it_op0_bitstring, ++it_op1_bitstring)
            {
               if(*it_lhs_bitstring == bit_lattice::X)
               {
                  res.push_front(bit_lattice::X);
               }
               else if(rhs_kind == and_node_K && *it_op0_bitstring != bit_lattice::ZERO &&
                       *it_op1_bitstring == bit_lattice::ZERO)
               {
                  res.push_front(bit_lattice::X);
               }
               else if(rhs_kind == or_node_K && *it_op0_bitstring != bit_lattice::ONE &&
                       *it_op1_bitstring == bit_lattice::ONE)
               {
                  res.push_front(bit_lattice::X);
               }
               else
               {
                  res.push_front(*it_op0_bitstring);
               }
            }
         }
         else if(rhs_kind == shl_node_K)
         {
            if(operation->op1->get_kind() != constant_int_val_node_K)
            {
               if(op1_nid == res_nid)
               {
                  const auto op_signed_p = ir_helper::IsSignedIntegerType(res_tn);
                  unsigned int log2;
                  for(log2 = 1; lhs_size > (1u << log2); ++log2)
                  {
                     ;
                  }
                  res = op1_bitstring;
                  for(auto index = 0u; res.size() > index + log2; ++index)
                  {
                     if(op_signed_p && (res.size() == index + log2 + 1))
                     {
                        res[index] = bit_lattice::ZERO;
                     }
                     else
                     {
                        res[index] = bit_lattice::X;
                     }
                  }
               }
               break;
            }

            const auto cst_val = ir_helper::GetConstValue(operation->op1);
            if(cst_val < 0)
            {
               res.push_back(bit_lattice::X);
               break;
            }

            res = lhs_bitstring;
            while(res.size() > (lhs_bitstring.size() - static_cast<size_t>(cst_val)))
            {
               res.pop_back();
            }
            if(res.size() < lhs_bitstring.size())
            {
               res = sign_extend_bitstring(res, ir_helper::IsSignedIntegerType(res_tn), lhs_bitstring.size());
            }
         }
         else if(rhs_kind == sub_node_K || rhs_kind == mul_node_K || rhs_kind == add_node_K || rhs_kind == gep_node_K ||
                 rhs_kind == widen_mul_node_K)
         {
            if(op0_nid != res_nid)
            {
               std::swap(op0_nid, op1_nid);
               std::swap(op0_bitstring, op1_bitstring);
            }

            res = op0_bitstring;
            const auto initial_size = op0_bitstring.size();
            auto res_size = ir_helper::TypeSize(res_tn);
            if(lhs_bitstring.front() == bit_lattice::U)
            {
               res_size = std::min(res_size, static_cast<unsigned long long>(lhs_bitstring.size()));
            }
            if(lhs_bitstring.front() == bit_lattice::X)
            {
               res_size = std::min(res_size, static_cast<unsigned long long>(lhs_bitstring.size() - 1));
            }
            while(res.size() > res_size)
            {
               res.pop_front();
            }
            if(res.size() != initial_size)
            {
               res.push_front(bit_lattice::X);
            }
         }
         else if(rhs_kind == shr_node_K)
         {
            if(operation->op1->get_kind() != constant_int_val_node_K)
            {
               if(op1_nid == res_nid)
               {
                  const auto op_signed_p = ir_helper::IsSignedIntegerType(res_tn);
                  unsigned int log2;
                  for(log2 = 1; lhs_size > (1u << log2); ++log2)
                  {
                     ;
                  }
                  res = op1_bitstring;
                  for(auto index = 0u; res.size() > index + log2; ++index)
                  {
                     if(op_signed_p && (res.size() == index + log2 + 1))
                     {
                        res[index] = bit_lattice::ZERO;
                     }
                     else
                     {
                        res[index] = bit_lattice::X;
                     }
                  }
               }
               break;
            }

            const auto cst_val = ir_helper::GetConstValue(operation->op1);
            THROW_ASSERT(cst_val <= std::numeric_limits<long long>::max(), "");
            if(cst_val < 0)
            {
               res.push_back(bit_lattice::X);
               break;
            }

            res = lhs_bitstring;
            const auto shift_value = static_cast<unsigned long long>(cst_val);
            for(auto shift_value_it = 0u; shift_value_it < shift_value; shift_value_it++)
            {
               res.push_back(bit_lattice::X);
            }

            const auto shifted_type_size = ir_helper::TypeSize(operation->op0);
            while(res.size() > shifted_type_size)
            {
               res.pop_front();
            }
            if(ir_helper::IsSignedIntegerType(operation->op0) && (lhs_bitstring.size() + shift_value) > lhs_size)
            {
               const auto lhs_sign_extend_end =
                   lhs_bitstring.begin() +
                   static_cast<decltype(lhs_bitstring)::difference_type>(lhs_bitstring.size() + shift_value - lhs_size);
               if(std::find(lhs_bitstring.begin(), lhs_sign_extend_end, bit_lattice::U) != lhs_sign_extend_end)
               {
                  res.front() = bit_lattice::U;
               }
            }
         }
         else if(rhs_kind == extract_bit_node_K)
         {
            if(op1_nid == res_nid)
            {
               THROW_ERROR("unexpected condition");
               break;
            }

            const auto cst_val = ir_helper::GetConstValue(operation->op1);
            THROW_ASSERT(cst_val >= 0, "unexpected condition");
            THROW_ASSERT(lhs_bitstring.size() == 1, "unexpected condition - " + bitstring_to_string(lhs_bitstring));

            res = lhs_bitstring;
            const auto shift_value = static_cast<unsigned long long>(cst_val);
            for(auto shift_value_it = 0u; shift_value_it < shift_value; shift_value_it++)
            {
               res.push_back(bit_lattice::X);
            }
            const auto shifted_type_size = ir_helper::TypeSize(operation->op0);
            while(res.size() < shifted_type_size)
            {
               res.push_front(bit_lattice::X);
            }
            while(res.size() > shifted_type_size)
            {
               res.pop_front();
            }
         }
         else
         {
            THROW_UNREACHABLE("Unhadled binary expression: " + ga->ToString() + "(" + ir_node::GetString(rhs_kind) +
                              ")");
         }
         break;
      }
      // Ternary expressions
      case concat_bit_node_K:
      case select_node_K:
      case fshl_node_K:
      case fshr_node_K:
      case ternary_add_node_K:
      case ternary_as_node_K:
      case ternary_sa_node_K:
      case ternary_ss_node_K:
      {
         const auto operation = GetPointerS<const ternary_node>(rhs);

         auto op0_nid = operation->op0->index;
         THROW_ASSERT(best.count(op0_nid), "");
         auto op0_bitstring = best.at(op0_nid);

         auto op1_nid = operation->op1->index;
         THROW_ASSERT(best.count(op1_nid), "");
         auto op1_bitstring = best.at(op1_nid);

         auto op2_nid = operation->op2->index;
         THROW_ASSERT(best.count(op2_nid), "");
         auto op2_bitstring = best.at(op2_nid);

         THROW_ASSERT(res_nid == op0_nid || res_nid == op1_nid || res_nid == op2_nid,
                      "Invalid operand: " + STR(res_nid) + " (" + ga->ToString() + ")");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---   operand0(" + STR(op0_nid) + "): " + bitstring_to_string(op0_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---   operand1(" + STR(op1_nid) + "): " + bitstring_to_string(op1_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---   operand2(" + STR(op2_nid) + "): " + bitstring_to_string(op2_bitstring));

         if(rhs_kind == concat_bit_node_K)
         {
            if(op2_nid == res_nid)
            {
               break;
            }
            const auto lhs_bitsize = lhs_bitstring.size();

            if(op0_nid == res_nid)
            {
               op1_nid = 0;
            }
            if(op1_nid == res_nid)
            {
               op0_nid = op1_nid;
               op0_bitstring = op1_bitstring;
            }

            const auto offset = ir_helper::GetConstValue(operation->op2);
            const auto initial_size = op0_bitstring.size();
            auto se_lhs_bitstring = lhs_bitstring;
            if(initial_size < lhs_bitsize)
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, IsSignedIntegerType(operation->op0), lhs_bitsize);
            }
            if(initial_size > lhs_bitsize)
            {
               se_lhs_bitstring = sign_extend_bitstring(lhs_bitstring, IsSignedIntegerType(lhs), initial_size);
            }

            auto it_lhs_bitstring = se_lhs_bitstring.rbegin();
            auto it_op0_bitstring = op0_bitstring.rbegin();
            integer_cst_t index = 0;
            if(op1_nid)
            {
               for(; it_lhs_bitstring != se_lhs_bitstring.rend() && it_op0_bitstring != op0_bitstring.rend() &&
                     index < offset;
                   ++it_lhs_bitstring, ++it_op0_bitstring, ++index)
               {
                  res.push_front(*it_lhs_bitstring);
               }
               if(IsSignedIntegerType(operation->op1))
               {
                  res.push_front(bit_lattice::X);
               }
            }
            else
            {
               for(; it_lhs_bitstring != se_lhs_bitstring.rend() && it_op0_bitstring != op0_bitstring.rend();
                   ++it_lhs_bitstring, ++it_op0_bitstring, ++index)
               {
                  if(index < offset)
                  {
                     res.push_front(bit_lattice::ZERO);
                  }
                  else
                  {
                     res.push_front(*it_lhs_bitstring);
                  }
               }
            }
         }
         else if(rhs_kind == select_node_K)
         {
            if(op0_nid == res_nid)
            {
               break;
            }

            if(op1_nid != res_nid)
            {
               std::swap(op1_nid, op2_nid);
               std::swap(op1_bitstring, op2_bitstring);
            }

            auto it_lhs_bitstring = lhs_bitstring.rbegin();
            auto it_op1_bitstring = op1_bitstring.rbegin();
            for(; it_lhs_bitstring != lhs_bitstring.rend() && it_op1_bitstring != op1_bitstring.rend();
                ++it_lhs_bitstring, ++it_op1_bitstring)
            {
               if(*it_lhs_bitstring == bit_lattice::X)
               {
                  res.push_front(bit_lattice::X);
               }
               else
               {
                  res.push_front(*it_op1_bitstring);
               }
            }
            if(res.front() == bit_lattice::X && op1_bitstring.size() < lhs_bitstring.size())
            {
               const auto arg1_sign = op1_bitstring.front();
               res.pop_front();
               res.push_front(arg1_sign);
            }
         }
         else if(rhs_kind == fshl_node_K || rhs_kind == fshr_node_K)
         {
            if(operation->op2->get_kind() != constant_int_val_node_K)
            {
               if(op2_nid == res_nid)
               {
                  res = create_u_bitstring(static_cast<size_t>(ceil_log2(lhs_size)));
               }
               break;
            }
            if(op0_nid == op1_nid)
            {
               res = create_u_bitstring(lhs_size);
               break;
            }

            THROW_ASSERT(ir_helper::GetConstValue(operation->op2) >= 0, "");
            const auto offset = static_cast<size_t>(ir_helper::GetConstValue(operation->op2)) % lhs_size;
            if(op0_nid == res_nid)
            {
               res = create_u_bitstring(static_cast<size_t>(lhs_size - offset));
            }
            else
            {
               THROW_ASSERT(op1_nid == res_nid, "");
               res = create_u_bitstring(offset);
               res.insert(res.end(), static_cast<size_t>(lhs_size - offset), bit_lattice::X);
            }
         }
         else if(rhs_kind == ternary_add_node_K || rhs_kind == ternary_as_node_K || rhs_kind == ternary_sa_node_K ||
                 rhs_kind == ternary_ss_node_K)
         {
            if(op0_nid == res_nid)
            {
               res = op0_bitstring;
            }
            if(op1_nid == res_nid)
            {
               res = op1_bitstring;
            }
            if(op2_nid == res_nid)
            {
               res = op2_bitstring;
            }
            const auto initial_size = res.size();
            auto res_size = ir_helper::TypeSize(res_tn);
            if(lhs_bitstring.front() == bit_lattice::U)
            {
               res_size = std::min(res_size, static_cast<unsigned long long>(lhs_bitstring.size()));
            }
            if(lhs_bitstring.front() == bit_lattice::X)
            {
               res_size = std::min(res_size, static_cast<unsigned long long>(lhs_bitstring.size() - 1));
            }
            while(res.size() > res_size)
            {
               res.pop_front();
            }
            if(res.size() != initial_size)
            {
               res.push_front(bit_lattice::X);
            }
         }
         else
         {
            THROW_UNREACHABLE("Unhadled ternary expression: " + ga->ToString() + "(" + ir_node::GetString(rhs_kind) +
                              ")");
         }
         break;
      }
      case call_node_K:
      {
         const auto call = GetPointerS<const call_node>(rhs);
         const auto call_it = direct_call_id_to_called_id.find(ga->index);
         if(call_it != direct_call_id_to_called_id.end())
         {
            const auto called_id = call_it->second;
            const auto tn = TM->GetIRNode(called_id);
            const auto fd = GetPointerS<const function_val_node>(tn);

            const auto actual_parms = call->args;
            const auto formal_parms = fd->list_of_args;
            THROW_ASSERT(actual_parms.size() == formal_parms.size(), "");
            auto a_it = actual_parms.cbegin();
            auto a_end = actual_parms.cend();
            auto f_it = formal_parms.cbegin();
            auto f_end = formal_parms.cend();
            bool found = actual_parms.empty();
            for(; a_it != a_end && f_it != f_end; a_it++, f_it++)
            {
               if((*a_it)->index == res_nid)
               {
                  const auto p_decl_id = AppM->getSSAFromParm(called_id, (*f_it)->index);
                  const auto parmssa = TM->GetIRNode(p_decl_id);
                  const auto pd = GetPointerS<const ssa_node>(parmssa);
                  std::deque<bit_lattice> tmp;
                  if(pd->bit_values.empty())
                  {
                     tmp = create_u_bitstring(ir_helper::TypeSize(parmssa));
                  }
                  else
                  {
                     tmp = string_to_bitstring(pd->bit_values);
                  }

                  res = found ? inf(res, tmp, res_tn) : tmp;
                  found = true;
               }
            }
            THROW_ASSERT(found, STR(res_nid) + " is not an actual parameter of function " + STR(called_id));
         }
         break;
      }
      case ssa_node_K:
      {
         THROW_ASSERT(best.count(rhs->index), "");
         res = best.at(rhs->index);
         break;
      }
      // Unary expressions
      case abs_node_K:
      // Binary expressions
      case eq_node_K:
      case ge_node_K:
      case gt_node_K:
      case le_node_K:
      case lt_node_K:
      case max_node_K:
      case min_node_K:
      case ne_node_K:
      case sub_sat_node_K:
      case add_sat_node_K:
      case idiv_node_K:
      case irem_node_K:
      case extractvalue_node_K:
      case extractelement_node_K:
      // Ternary expressions
      case lut_node_K:
      case insertvalue_node_K:
      case insertelement_node_K:
      {
         // Do nothing
         break;
      }
      // Unary expressions
      case fptoi_node_K:
      case itofp_node_K:
      case unaligned_mem_access_node_K:
      // Binary expressions
      case fdiv_node_K:
      case frem_node_K:
      // Ternary expressions
      case shufflevector_node_K:
      case CASE_TYPE_NODES:
      case CASE_FAKE_NODES:
      case CASE_DECL_NODES:
      case CASE_CST_NODES:
      case CASE_NODE_STMTS:
      case constructor_node_K:
      case identifier_node_K:
      case statement_list_node_K:
      default:
      {
         THROW_UNREACHABLE("Unhandled rhs expression: " + ga->ToString() + " (" + ir_node::GetString(rhs_kind) + ")");
         break;
      }
   }
   // TODO: this is because IrLowering is not doing its job, better fix it and remove this
   const auto res_size = ir_helper::TypeSize(res_tn);
   while(res.size() > res_size)
   {
      res.pop_front();
   }
   return res;
}
