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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @author Giulio Stramondo
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Fiortio <michele.fiorito@polimi.it>
 */
#include "Bit_Value.hpp"

#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include <boost/range/adaptors.hpp>

std::deque<bit_lattice> Bit_Value::get_current_or_best(const tree_nodeConstRef& tn) const
{
   const auto nid = tn->index;
   const auto node = tn;
   if(node->get_kind() == ssa_name_K && current.find(nid) != current.end())
   {
      return current.at(nid);
   }
   THROW_ASSERT(best.count(nid), "");
   return best.at(nid);
}

std::deque<bit_lattice> Bit_Value::backward_chain(const tree_nodeConstRef& ssa_node) const
{
   const auto ssa = GetPointerS<const ssa_name>(ssa_node);
   const auto ssa_nid = ssa->index;
   std::deque<bit_lattice> res = create_x_bitstring(1);
   for(const auto& stmt_use : ssa->CGetUseStmts())
   {
      const auto user_stmt = stmt_use.first;
      const auto user_kind = user_stmt->get_kind();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing use - " + STR(user_stmt));
      std::deque<bit_lattice> user_res;
      if(user_kind == gimple_assign_K)
      {
         const auto ga = GetPointerS<const gimple_assign>(user_stmt);
         if(!IsHandledByBitvalue(ga->op0))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---variable " + STR(ga->op0) + " of type " + STR(tree_helper::CGetType(ga->op0)) +
                               " not considered");
            user_res = create_u_bitstring(tree_helper::TypeSize(ssa_node));
         }
         else if(ga->predicate && ga->predicate->index == ssa_nid)
         {
            user_res = create_u_bitstring(tree_helper::TypeSize(ssa_node));
         }
         else
         {
            user_res = backward_transfer(ga, ssa_nid);
         }
      }
      else if(user_kind == gimple_phi_K)
      {
         const auto gp = GetPointerS<const gimple_phi>(user_stmt);
         if(!gp->virtual_flag)
         {
            user_res = get_current_or_best(gp->res);
         }
      }
      else if(user_kind == gimple_return_K)
      {
         THROW_ASSERT(GetPointerS<const gimple_return>(user_stmt)->op,
                      "ssa id " + STR(ssa_nid) +
                          "used in empty return statement: " + STR(GetPointerS<const gimple_return>(user_stmt)));
         const auto res_it = current.find(function_id);
         if(res_it != current.end())
         {
            user_res = res_it->second;
         }
      }
      else if(user_kind == gimple_call_K)
      {
         const auto gc = GetPointerS<const gimple_call>(user_stmt);
         const auto call_it = direct_call_id_to_called_id.find(gc->index);
         if(call_it != direct_call_id_to_called_id.end())
         {
            const auto called_id = call_it->second;
            const auto called_tn = TM->GetTreeNode(called_id);
            const auto called_fd = GetPointerS<const function_decl>(called_tn);

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
                  const auto parmssa = TM->GetTreeNode(p_decl_id);
                  const auto pd = GetPointerS<const ssa_name>(parmssa);
                  std::deque<bit_lattice> tmp;
                  if(pd->bit_values.empty())
                  {
                     tmp = create_u_bitstring(tree_helper::TypeSize(parmssa));
                  }
                  else
                  {
                     tmp = string_to_bitstring(pd->bit_values);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---param: " + bitstring_to_string(tmp));
                  user_res = found ? inf(user_res, tmp, ssa_node) : tmp;
                  found = true;
               }
            }
            THROW_ASSERT(found, STR(ssa) + " is not an actual parameter of function " + STR(called_id));
         }
      }
      else if(user_kind == gimple_cond_K || user_kind == gimple_multi_way_if_K || user_kind == gimple_switch_K ||
              user_kind == gimple_asm_K)
      {
         user_res = create_u_bitstring(tree_helper::TypeSize(ssa_node));
      }
      else
      {
         THROW_UNREACHABLE("Unhandled statement: " + STR(user_stmt) + "(" + user_stmt->get_kind_text() + ")");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---res : " + bitstring_to_string(user_res));
      if(user_res.size())
      {
         res = inf(res, user_res, ssa_node);
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
   std::deque<tree_nodeConstRef> working_list;
   CustomUnorderedSet<unsigned int> working_list_idx;
   const auto push_back = [&](const tree_nodeConstRef& stmt) {
      const auto stmt_kind = stmt->get_kind();
      if(!working_list_idx.count(stmt->index) && (stmt_kind == gimple_assign_K || stmt_kind == gimple_phi_K))
      {
         working_list.push_back(stmt);
         working_list_idx.insert(stmt->index);
      }
   };
   const auto pop_front = [&]() -> tree_nodeConstRef {
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
         THROW_ASSERT(GetPointerS<const gimple_node>(s)->bb_index == bb->number,
                      "BB" + STR(bb->number) + " contains statement from BB" +
                          STR(GetPointerS<const gimple_node>(s)->bb_index) + " - " + s->get_kind_text() + " - " +
                          STR(s));
      }
      for(const auto& stmt : boost::adaptors::reverse(bb->CGetPhiList()))
      {
         const auto s = stmt;
         const auto gp = GetPointerS<const gimple_phi>(s);
         if(!gp->virtual_flag)
         {
            if(IsHandledByBitvalue(gp->res))
            {
               push_back(s);
               THROW_ASSERT(GetPointerS<const gimple_node>(s)->bb_index == bb->number,
                            "BB" + STR(bb->number) + " contains statement from BB" +
                                STR(GetPointerS<const gimple_node>(s)->bb_index) + " - " + s->get_kind_text() + " - " +
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
      tree_nodeConstRef lhs;
      if(stmt_kind == gimple_assign_K)
      {
         lhs = GetPointerS<const gimple_assign>(stmt)->op0;
      }
      else if(stmt_kind == gimple_phi_K)
      {
         lhs = GetPointerS<const gimple_phi>(stmt)->res;
      }
      else
      {
         THROW_UNREACHABLE("Unexpected statement kind: " + stmt->get_kind_text() + " - " + STR(stmt));
      }
      if(lhs->get_kind() == ssa_name_K)
      {
         if(!IsHandledByBitvalue(lhs))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "<--variable " + STR(lhs) + " of type " + STR(tree_helper::CGetType(lhs)) +
                               " not considered");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Propagation for " + STR(lhs));
         auto res = backward_chain(lhs);
         if(update_current(res, lhs))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---current updated: " + bitstring_to_string(current.at(lhs->index)));
            std::vector<std::tuple<unsigned int, unsigned int>> vars_read;
            tree_helper::get_required_values(vars_read, stmt);
            for(const auto& var_pair : vars_read)
            {
               const auto in_ssa_nid = std::get<0>(var_pair);
               if(in_ssa_nid == 0)
               {
                  continue;
               }
               const auto in_ssa = TM->GetTreeNode(in_ssa_nid);
               if(!IsHandledByBitvalue(in_ssa))
               {
                  continue;
               }
               if(in_ssa->get_kind() == ssa_name_K)
               {
                  const auto ssa_var = GetPointerS<const ssa_name>(in_ssa);
                  const auto nextNode = ssa_var->CGetDefStmt();
                  push_back(nextNode);
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + STR(stmt));
   }

   const auto tn = TM->GetTreeNode(function_id);
   const auto fd = GetPointerS<const function_decl>(tn);
   for(const auto& parm_decl_node : fd->list_of_args)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing argument " + STR(parm_decl_node));
      const auto parmssa_id = AppM->getSSAFromParm(function_id, parm_decl_node->index);
      const auto parmssa = TM->GetTreeNode(parmssa_id);
      if(!IsHandledByBitvalue(parmssa))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "<--argument " + STR(parmssa) + " of type " + STR(tree_helper::CGetType(parmssa)) +
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

std::deque<bit_lattice> Bit_Value::backward_transfer(const gimple_assign* ga, unsigned int res_nid) const
{
   std::deque<bit_lattice> res;
   if(tree_helper::IsConstant(TM->GetTreeNode(res_nid)))
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
   const auto lhs_nid = lhs->index;
   const auto lhs_size = tree_helper::TypeSize(lhs);
   const auto lhs_kind = lhs->get_kind();

   switch(lhs_kind)
   {
      case array_ref_K:
      {
         auto operation = GetPointerS<const array_ref>(ga->op0);
         do
         {
            if(operation->op1->index == res_nid && current.count(operation->op1->index))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS equals operand");
               return create_u_bitstring(pointer_resizing(operation->op0->index));
            }
            operation = GetPointerS<const array_ref>(operation->op0);
         } while(operation);
         return res;
      }
      case mem_ref_K:
      {
         const auto operation = GetPointerS<const mem_ref>(ga->op0);
         if(operation->op0->index == res_nid && current.count(operation->op0->index))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS equals operand");
            return create_u_bitstring(pointer_resizing(res_nid));
         }
         return res;
      }
      case ssa_name_K:
      {
         break;
      }
      case CASE_UNARY_EXPRESSION:
      case assert_expr_K:
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case catch_expr_K:
      case ceil_div_expr_K:
      case ceil_mod_expr_K:
      case complex_expr_K:
      case compound_expr_K:
      case eh_filter_expr_K:
      case eq_expr_K:
      case exact_div_expr_K:
      case fdesc_expr_K:
      case floor_div_expr_K:
      case floor_mod_expr_K:
      case ge_expr_K:
      case gt_expr_K:
      case goto_subroutine_K:
      case in_expr_K:
      case init_expr_K:
      case le_expr_K:
      case lrotate_expr_K:
      case lshift_expr_K:
      case lt_expr_K:
      case max_expr_K:
      case min_expr_K:
      case minus_expr_K:
      case modify_expr_K:
      case mult_expr_K:
      case mult_highpart_expr_K:
      case ne_expr_K:
      case ordered_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case frem_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      case set_le_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case uneq_expr_K:
      case ltgt_expr_K:
      case unge_expr_K:
      case ungt_expr_K:
      case unle_expr_K:
      case unlt_expr_K:
      case unordered_expr_K:
      case widen_sum_expr_K:
      case widen_mult_expr_K:
      case with_size_expr_K:
      case vec_lshift_expr_K:
      case vec_rshift_expr_K:
      case widen_mult_hi_expr_K:
      case widen_mult_lo_expr_K:
      case vec_pack_trunc_expr_K:
      case vec_pack_sat_expr_K:
      case vec_pack_fix_trunc_expr_K:
      case vec_extracteven_expr_K:
      case vec_extractodd_expr_K:
      case vec_interleavehigh_expr_K:
      case vec_interleavelow_expr_K:
      case extract_bit_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case CASE_TERNARY_EXPRESSION:
      case array_range_ref_K:
      case CASE_TYPE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_FAKE_NODES:
      case CASE_CPP_NODES:
      case CASE_DECL_NODES:
      case CASE_CST_NODES:
      case CASE_GIMPLE_NODES:
      case aggr_init_expr_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case lut_expr_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      default:
      {
         THROW_UNREACHABLE("Unhandled lhs expression: " + ga->ToString() + " (" + tree_node::GetString(lhs_kind) + ")");
         break;
      }
   }

   const auto lhs_bitstring = get_current_or_best(lhs);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---backward_transfer, lhs: " + bitstring_to_string(lhs_bitstring));
   const auto& rhs = ga->op1;
   const auto rhs_kind = rhs->get_kind();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- => " + tree_node::GetString(rhs_kind));
   switch(rhs_kind)
   {
      // Unary expressions
      case addr_expr_K:
      case bit_not_expr_K:
      case convert_expr_K:
      case negate_expr_K:
      case nop_expr_K:
      case truth_not_expr_K:
      case view_convert_expr_K:
      {
         const auto operation = GetPointerS<const unary_expr>(rhs);

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

         if(rhs_kind == addr_expr_K)
         {
            if(ga->temporary_address)
            {
               const auto op_kind = operation->op->get_kind();
               if(op_kind == mem_ref_K)
               {
                  const auto mr = GetPointerS<const mem_ref>(operation->op);
                  if(mr->op0->index == res_nid && current.count(mr->op0->index))
                  {
                     return create_u_bitstring(pointer_resizing(ga->op0->index));
                  }
               }
               else if(op_kind == target_mem_ref461_K)
               {
                  const auto tmr = GetPointerS<const target_mem_ref461>(operation->op);

                  if(tmr->base->index == res_nid && current.count(tmr->base->index))
                  {
                     return create_u_bitstring(pointer_resizing(lhs_nid));
                  }
                  else if(tmr->idx2 && tmr->idx2->index == res_nid && current.count(tmr->idx2->index))
                  {
                     return create_u_bitstring(pointer_resizing(lhs_nid));
                  }
               }
               else if(op_kind == array_ref_K)
               {
                  auto ar = GetPointerS<const array_ref>(operation->op);
                  do
                  {
                     if(ar->op1->index == res_nid && current.count(ar->op1->index))
                     {
                        return create_u_bitstring(pointer_resizing(ar->op0->index));
                     }
                     ar = GetPointerS<const array_ref>(ar->op0);
                  } while(operation);
               }
            }
         }
         else if(rhs_kind == bit_not_expr_K)
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
         else if(rhs_kind == convert_expr_K || rhs_kind == nop_expr_K || rhs_kind == view_convert_expr_K)
         {
            const bool lhs_signed = IsSignedIntegerType(lhs);
            const bool op_signed = IsSignedIntegerType(operation->op);
            const auto op_size = tree_helper::TypeSize(operation->op);
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
         else if(rhs_kind == negate_expr_K)
         {
            res = op_bitstring;
            const auto initial_size = op_bitstring.size();
            auto res_size = tree_helper::TypeSize(TM->GetTreeNode(res_nid));
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
         else if(rhs_kind == truth_not_expr_K)
         {
            res = op_bitstring;
            while(res.size() > 1)
            {
               res.pop_front();
            }
         }
         else
         {
            THROW_UNREACHABLE("Unhadled unary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) +
                              ")");
         }
         break;
      }
      // Binary expressions
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case extract_bit_expr_K:
      case lrotate_expr_K:
      case lshift_expr_K:
      case mem_ref_K:
      case minus_expr_K:
      case mult_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case widen_mult_expr_K:
      {
         const auto operation = GetPointerS<const binary_expr>(rhs);

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

         if(rhs_kind == bit_and_expr_K || rhs_kind == bit_ior_expr_K || rhs_kind == bit_xor_expr_K)
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
               else if(rhs_kind == bit_and_expr_K && *it_op0_bitstring != bit_lattice::ZERO &&
                       *it_op1_bitstring == bit_lattice::ZERO)
               {
                  res.push_front(bit_lattice::X);
               }
               else if(rhs_kind == bit_ior_expr_K && *it_op0_bitstring != bit_lattice::ONE &&
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
         else if(rhs_kind == lrotate_expr_K || rhs_kind == rrotate_expr_K)
         {
            if(op1_nid == res_nid)
            {
               const auto op_signed_p = tree_helper::is_int(TM, res_nid);
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
         }
         else if(rhs_kind == lshift_expr_K)
         {
            if(operation->op1->get_kind() != integer_cst_K)
            {
               if(op1_nid == res_nid)
               {
                  const auto op_signed_p = tree_helper::is_int(TM, res_nid);
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

            const auto cst_val = tree_helper::GetConstValue(operation->op1);
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
               res = sign_extend_bitstring(res, tree_helper::is_int(TM, res_nid), lhs_bitstring.size());
            }
         }
         else if(rhs_kind == mem_ref_K)
         {
            if(op0_nid == res_nid)
            {
               res = create_u_bitstring(pointer_resizing(res_nid));
            }
         }
         else if(rhs_kind == minus_expr_K || rhs_kind == mult_expr_K || rhs_kind == plus_expr_K ||
                 rhs_kind == pointer_plus_expr_K || rhs_kind == widen_mult_expr_K)
         {
            if(op0_nid != res_nid)
            {
               std::swap(op0_nid, op1_nid);
               std::swap(op0_bitstring, op1_bitstring);
            }

            res = op0_bitstring;
            const auto initial_size = op0_bitstring.size();
            auto res_size = tree_helper::TypeSize(TM->GetTreeNode(res_nid));
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
         else if(rhs_kind == rshift_expr_K)
         {
            if(operation->op1->get_kind() != integer_cst_K)
            {
               if(op1_nid == res_nid)
               {
                  const auto op_signed_p = tree_helper::is_int(TM, res_nid);
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

            const auto cst_val = tree_helper::GetConstValue(operation->op1);
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

            const auto shifted_type_size = tree_helper::TypeSize(operation->op0);
            while(res.size() > shifted_type_size)
            {
               res.pop_front();
            }
            if(tree_helper::IsSignedIntegerType(operation->op0) && (lhs_bitstring.size() + shift_value) > lhs_size)
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
         else if(rhs_kind == truth_and_expr_K || rhs_kind == truth_andif_expr_K || rhs_kind == truth_or_expr_K ||
                 rhs_kind == truth_orif_expr_K || rhs_kind == truth_xor_expr_K)
         {
            if(op0_nid != res_nid)
            {
               std::swap(op0_nid, op0_nid);
               std::swap(op0_bitstring, op1_bitstring);
            }
            res = op0_bitstring;
            while(res.size() > 1)
            {
               res.pop_front();
            }
         }
         else if(rhs_kind == extract_bit_expr_K)
         {
            if(op1_nid == res_nid)
            {
               THROW_ERROR("unexpected condition");
               break;
            }

            const auto cst_val = tree_helper::GetConstValue(operation->op1);
            THROW_ASSERT(cst_val >= 0, "unexpected condition");
            THROW_ASSERT(lhs_bitstring.size() == 1, "unexpected condition - " + bitstring_to_string(lhs_bitstring));

            res = lhs_bitstring;
            const auto shift_value = static_cast<unsigned long long>(cst_val);
            for(auto shift_value_it = 0u; shift_value_it < shift_value; shift_value_it++)
            {
               res.push_back(bit_lattice::X);
            }
            const auto shifted_type_size = tree_helper::TypeSize(operation->op0);
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
            THROW_UNREACHABLE("Unhadled binary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) +
                              ")");
         }
         break;
      }
      // Ternary expressions
      case bit_ior_concat_expr_K:
      case cond_expr_K:
      case fshl_expr_K:
      case fshr_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      {
         const auto operation = GetPointerS<const ternary_expr>(rhs);

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

         if(rhs_kind == bit_ior_concat_expr_K)
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

            const auto offset = tree_helper::GetConstValue(operation->op2);
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
         else if(rhs_kind == cond_expr_K)
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
         else if(rhs_kind == fshl_expr_K || rhs_kind == fshr_expr_K)
         {
            if(operation->op2->get_kind() != integer_cst_K)
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

            THROW_ASSERT(tree_helper::GetConstValue(operation->op2) >= 0, "");
            const auto offset = static_cast<size_t>(tree_helper::GetConstValue(operation->op2)) % lhs_size;
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
         else if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K || rhs_kind == ternary_mp_expr_K ||
                 rhs_kind == ternary_mm_expr_K)
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
            auto res_size = tree_helper::TypeSize(TM->GetTreeNode(res_nid));
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
            THROW_UNREACHABLE("Unhadled ternary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) +
                              ")");
         }
         break;
      }
      case array_ref_K:
      {
         auto operation = GetPointerS<const array_ref>(rhs);
         do
         {
            if(operation->op1->index == res_nid)
            {
               return create_u_bitstring(pointer_resizing(operation->op0->index));
            }
            operation = GetPointer<const array_ref>(operation->op0);
         } while(operation);
         break;
      }
      case aggr_init_expr_K:
      case call_expr_K:
      {
         const auto call = GetPointerS<const call_expr>(rhs);
         const auto call_it = direct_call_id_to_called_id.find(ga->index);
         if(call_it != direct_call_id_to_called_id.end())
         {
            const auto called_id = call_it->second;
            const auto tn = TM->GetTreeNode(called_id);
            const auto fd = GetPointerS<const function_decl>(tn);

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
                  const auto parmssa = TM->GetTreeNode(p_decl_id);
                  const auto pd = GetPointerS<const ssa_name>(parmssa);
                  std::deque<bit_lattice> tmp;
                  if(pd->bit_values.empty())
                  {
                     tmp = create_u_bitstring(tree_helper::TypeSize(parmssa));
                  }
                  else
                  {
                     tmp = string_to_bitstring(pd->bit_values);
                  }

                  res = found ? inf(res, tmp, res_nid) : tmp;
                  found = true;
               }
            }
            THROW_ASSERT(found, STR(res_nid) + " is not an actual parameter of function " + STR(called_id));
         }
         break;
      }
      case ssa_name_K:
      {
         THROW_ASSERT(best.count(rhs->index), "");
         res = best.at(rhs->index);
         break;
      }
      // Unary expressions
      case abs_expr_K:
      case alignof_expr_K:
      // Binary expressions
      case eq_expr_K:
      case exact_div_expr_K:
      case ge_expr_K:
      case gt_expr_K:
      case le_expr_K:
      case lt_expr_K:
      case ltgt_expr_K:
      case max_expr_K:
      case min_expr_K:
      case ne_expr_K:
      case ordered_expr_K:
      case sat_minus_expr_K:
      case sat_plus_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      case uneq_expr_K:
      case unge_expr_K:
      case ungt_expr_K:
      case unle_expr_K:
      case unlt_expr_K:
      case unordered_expr_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      // Ternary expressions
      case lut_expr_K:
      case insertvalue_expr_K:
      case insertelement_expr_K:
      {
         // Do nothing
         break;
      }
      // Unary expressions
      case arrow_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fix_trunc_expr_K:
      case float_expr_K:
      case imagpart_expr_K:
      case indirect_ref_K:
      case misaligned_indirect_ref_K:
      case loop_expr_K:
      case non_lvalue_expr_K:
      case paren_expr_K:
      case realpart_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      // Binary expressions
      case assert_expr_K:
      case catch_expr_K:
      case ceil_div_expr_K:
      case ceil_mod_expr_K:
      case complex_expr_K:
      case compound_expr_K:
      case eh_filter_expr_K:
      case fdesc_expr_K:
      case floor_div_expr_K:
      case floor_mod_expr_K:
      case goto_subroutine_K:
      case in_expr_K:
      case init_expr_K:
      case modify_expr_K:
      case mult_highpart_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case frem_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case set_le_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case widen_sum_expr_K:
      case with_size_expr_K:
      case vec_lshift_expr_K:
      case vec_rshift_expr_K:
      case widen_mult_hi_expr_K:
      case widen_mult_lo_expr_K:
      case vec_pack_trunc_expr_K:
      case vec_pack_sat_expr_K:
      case vec_pack_fix_trunc_expr_K:
      case vec_extracteven_expr_K:
      case vec_extractodd_expr_K:
      case vec_interleavehigh_expr_K:
      case vec_interleavelow_expr_K:
      // Ternary expressions
      case component_ref_K:
      case bit_field_ref_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      // Quaternary expressions
      case array_range_ref_K:
      case CASE_TYPE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_FAKE_NODES:
      case CASE_CPP_NODES:
      case CASE_DECL_NODES:
      case CASE_CST_NODES:
      case CASE_GIMPLE_NODES:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      default:
      {
         THROW_UNREACHABLE("Unhandled rhs expression: " + ga->ToString() + " (" + tree_node::GetString(rhs_kind) + ")");
         break;
      }
   }
   // TODO: this is because IrLowering is not doing its job, better fix it and remove this
   const auto res_size = tree_helper::TypeSize(TM->GetTreeNode(res_nid));
   while(res.size() > res_size)
   {
      res.pop_front();
   }
   return res;
}
