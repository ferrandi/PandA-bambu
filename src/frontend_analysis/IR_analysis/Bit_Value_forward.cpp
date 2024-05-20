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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiortio <michele.fiorito@polimi.it>
 */
#include "Bit_Value.hpp"

#include "Dominance.hpp"
#include "Parameter.hpp"
#include "Range.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include <boost/range/adaptors.hpp>

std::deque<bit_lattice> Bit_Value::get_current(const tree_nodeConstRef& tn) const
{
   if(tn->get_kind() == ssa_name_K || tn->get_kind() == parm_decl_K)
   {
      THROW_ASSERT(current.count(tn->index), "");
      return current.at(tn->index);
   }
   else if(tree_helper::IsConstant(tn))
   {
      THROW_ASSERT(best.count(tn->index), "");
      return best.at(tn->index);
   }
   THROW_UNREACHABLE("Unexpected node kind: " + tn->get_kind_text());
   return std::deque<bit_lattice>();
}

void Bit_Value::forward()
{
   std::deque<tree_nodeConstRef> working_list, return_list;
   CustomUnorderedSet<unsigned int> working_list_idx;
   const auto push_back = [&](const tree_nodeConstRef& stmt) {
      const auto stmt_kind = stmt->get_kind();
      if(!working_list_idx.count(stmt->index) && (stmt_kind == gimple_assign_K || stmt_kind == gimple_phi_K ||
                                                  stmt_kind == gimple_return_K || stmt_kind == gimple_asm_K))
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
   for(const auto& bb : bb_topological)
   {
      for(const auto& phi : bb->CGetPhiList())
      {
         const auto phi_node = phi;
         const auto gp = GetPointerS<const gimple_phi>(phi_node);
         if(!gp->virtual_flag)
         {
            if(IsHandledByBitvalue(gp->res))
            {
               push_back(phi_node);
            }
         }
      }
      for(const auto& stmt : bb->CGetStmtList())
      {
         const auto stmt_node = stmt;
         if(stmt_node->get_kind() == gimple_assign_K)
         {
            const auto ga = GetPointerS<const gimple_assign>(stmt_node);
            if(IsHandledByBitvalue(ga->op0))
            {
               push_back(stmt_node);
            }
         }
         else if(stmt_node->get_kind() == gimple_return_K)
         {
            return_list.push_back(stmt_node);
         }
      }
   }

   const auto is_root_function = AppM->CGetCallGraphManager()->GetRootFunctions().count(function_id);
   while(!working_list.empty())
   {
      const auto stmt_node = pop_front();
      const auto stmt_kind = stmt_node->get_kind();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + STR(stmt_node));
      if(stmt_kind == gimple_assign_K)
      {
         const auto ga = GetPointerS<const gimple_assign>(stmt_node);
         const auto output_nid = ga->op0->index;
         if(ga->op0->get_kind() == ssa_name_K)
         {
            const auto ssa = GetPointerS<const ssa_name>(ga->op0);
            if(!IsHandledByBitvalue(ga->op0) || ssa->CGetUseStmts().empty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "<--variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(ga->op0)) +
                                  " not considered");
               continue;
            }
            bool hasRequiredValues = true;
            std::vector<std::tuple<unsigned int, unsigned int>> vars_read;
            tree_helper::get_required_values(vars_read, stmt_node);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Requires " + STR(vars_read.size()) + " values");
            for(const auto& var_pair : vars_read)
            {
               const auto use_node_id = std::get<0>(var_pair);
               if(use_node_id == 0)
               {
                  continue;
               }
               const auto use_node = TM->GetTreeNode(use_node_id);
               if(!IsHandledByBitvalue(use_node))
               {
                  continue;
               }

               if(use_node->get_kind() == ssa_name_K && !current.count(use_node_id))
               {
                  const auto ssa_use = GetPointerS<const ssa_name>(use_node);
                  const auto def_stmt = ssa_use->CGetDefStmt();
                  if(def_stmt->get_kind() == gimple_nop_K)
                  {
                     THROW_ASSERT(!ssa_use->var || ssa_use->var->get_kind() != parm_decl_K,
                                  "Function parameter bitvalue must be defined before");
                     current.insert(
                         std::make_pair(use_node_id, create_bitstring_from_constant(0, tree_helper::TypeSize(use_node),
                                                                                    IsSignedIntegerType(use_node))));
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---Input " + ssa_use->ToString() + " definition has not been analyzed yet");
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Definition statement added to queue: " + def_stmt->ToString());
                     push_back(def_stmt);
                     hasRequiredValues = false;
                     continue;
                  }
               }
            }
            if(hasRequiredValues)
            {
               auto res = forward_transfer(ga);
               current.insert(std::make_pair(output_nid, best.at(output_nid)));
               if(update_current(res, ga->op0))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---current updated: " + bitstring_to_string(current.at(output_nid)));
                  for(const auto& next_node : ssa->CGetUseStmts())
                  {
                     push_back(next_node.first);
                  }
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "---Inputs not fully analyzed by the forward Bit Value Analysis. Operation  " +
                                  ga->op0->ToString() + " postponed");
               push_back(stmt_node);
            }
         }
      }
      else if(stmt_kind == gimple_phi_K)
      {
         const auto gp = GetPointerS<const gimple_phi>(stmt_node);
         THROW_ASSERT(!gp->virtual_flag, "unexpected case");

         const auto output_nid = gp->res->index;
         const auto ssa = GetPointerS<const ssa_name>(gp->res);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "phi: " + STR(stmt_node->index));
         if(!IsHandledByBitvalue(gp->res) || ssa->CGetUseStmts().empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "<--variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(gp->res)) +
                               " not considered");
            continue;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res id: " + STR(output_nid));
         auto res = create_x_bitstring(1);
         bool atLeastOne = false;
         bool allInputs = true;
#if HAVE_ASSERTS
         const auto is_signed1 = tree_helper::IsSignedIntegerType(gp->res);
#endif
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            const auto def_id = def_edge.first->index;
            if(def_id == output_nid)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Skipping " + STR(def_edge.first) + " coming from BB" + STR(def_edge.second) +
                                  " because of ssa cycle");
               continue;
            }
            if(current.find(def_id) == current.end())
            {
               const auto def_node = def_edge.first;
               if(def_node->get_kind() == ssa_name_K)
               {
                  const auto def_ssa = GetPointerS<const ssa_name>(def_node);
                  const auto def_stmt = def_ssa->CGetDefStmt();
                  if(def_stmt->get_kind() == gimple_nop_K)
                  {
                     THROW_ASSERT(!def_ssa->var || def_ssa->var->get_kind() != parm_decl_K,
                                  "Function parameter bitvalue must be defined before");
                     current.insert(
                         std::make_pair(def_id, create_bitstring_from_constant(0, tree_helper::TypeSize(def_node),
                                                                               IsSignedIntegerType(def_edge.first))));
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Skipping " + STR(def_node) +
                                        " no current has been yet computed for this ssa var");
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Definition statement added to queue: " + def_stmt->ToString());
                     push_back(def_stmt);
                     allInputs = false;
                     continue;
                  }
               }
               else
               {
                  THROW_ASSERT(best.find(def_id) != best.end(), "unexpected condition");
                  current.insert(std::make_pair(def_id, best.at(def_id)));
               }
            }
            atLeastOne = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---Edge " + STR(def_edge.second) + ": " + bitstring_to_string(current.at(def_id)));

#if HAVE_ASSERTS
            const auto is_signed2 = IsSignedIntegerType(def_edge.first);
#endif
            THROW_ASSERT(is_signed2 == is_signed1, STR(stmt_node));
            res = inf(res, current.at(def_id), gp->res);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---current res: " + bitstring_to_string(res));
         }
         if(atLeastOne)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---res: " + bitstring_to_string(res));
            const auto ins = current.insert(std::make_pair(output_nid, res));
            auto current_updated = ins.second;
            if(!current_updated)
            {
               current_updated = update_current(res, gp->res);
            }
            if(current_updated)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---current updated: " + bitstring_to_string(current.at(output_nid)));
               for(const auto& next_node : ssa->CGetUseStmts())
               {
                  push_back(next_node.first);
               }
            }
         }
         if(!allInputs)
         {
            push_back(stmt_node);
         }
      }
      else if(stmt_kind == gimple_return_K)
      {
         if(!is_root_function)
         {
            const auto gr = GetPointerS<const gimple_return>(stmt_node);
            THROW_ASSERT(gr->op, "Empty return should not be a use of any ssa");
            if(gr->op->get_kind() == ssa_name_K && IsHandledByBitvalue(gr->op))
            {
               const auto res = get_current(gr->op);
               THROW_ASSERT(res.size(), "");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---res: " + bitstring_to_string(res));
               auto& output_current = current[function_id];
               output_current = output_current.size() ? inf(res, output_current, function_id) : res;
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---inf: " + bitstring_to_string(output_current));
            }
         }
      }
      else if(stmt_kind == gimple_asm_K)
      {
         const auto ga = GetPointerS<const gimple_asm>(stmt_node);
         if(ga->out)
         {
            auto tl = GetPointerS<const tree_list>(ga->out);
            THROW_ASSERT(tl->valu, "only the first output and so only single output gimple_asm are supported");
            if(tl->valu->get_kind() == ssa_name_K)
            {
               const auto ssa = GetPointerS<const ssa_name>(tl->valu);
               if(!ssa->CGetUseStmts().empty() && IsHandledByBitvalue(tl->valu))
               {
                  const auto output_nid = tl->valu->index;
                  THROW_ASSERT(best.count(output_nid), "");
                  current[output_nid] = best.at(output_nid);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---current updated: " + bitstring_to_string(current.at(output_nid)));
               }
            }
         }
      }
      else
      {
         THROW_UNREACHABLE("Unhandled statement: " + STR(stmt_node) + "(" + tree_node::GetString(stmt_kind) + ")");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + STR(stmt_node));
   }
   // Update current to perform sup with best after all return statements bitvalues have been propagated
   if(current.count(function_id))
   {
      update_current(current.at(function_id), TM->GetTreeNode(function_id));
   }
}

std::deque<bit_lattice> Bit_Value::forward_transfer(const gimple_assign* ga) const
{
   std::deque<bit_lattice> res;
   const auto& lhs = ga->op0;
   const auto& rhs = ga->op1;
   const auto lhs_signed = IsSignedIntegerType(lhs);
   const auto lhs_size = tree_helper::TypeSize(lhs);
   const auto rhs_kind = rhs->get_kind();
   switch(rhs_kind)
   {
      case ssa_name_K:
      case integer_cst_K:
      {
         res = get_current(rhs);
         break;
      }
      case addr_expr_K:
      {
         const auto ae = GetPointerS<const addr_expr>(rhs);
         const auto address_size = AppM->get_address_bitsize();
         const auto is_pretty_print_used =
             parameters->isOption(OPT_pretty_print) ||
             (parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy));
         const auto lt0 = lsb_to_zero(ae, is_pretty_print_used);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---address_size: " + STR(address_size) + " lt0: " + STR(lt0));
         if(lt0 && address_size > lt0)
         {
            res = create_u_bitstring(address_size - lt0);
            for(auto index = 0u; index < lt0; ++index)
            {
               res.push_back(bit_lattice::ZERO);
            }
         }
         break;
      }
      case abs_expr_K:
      case bit_not_expr_K:
      case convert_expr_K:
      case negate_expr_K:
      case nop_expr_K:
      case truth_not_expr_K:
      case view_convert_expr_K:
      {
         const auto operation = GetPointerS<const unary_expr>(rhs);

         if(!IsHandledByBitvalue(operation->op))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---operand " + STR(operation->op) + " of type " +
                               STR(tree_helper::CGetType(operation->op)) + " not handled by bitvalue");
            res = create_u_bitstring(lhs_size);
            break;
         }
         const auto op_signed = IsSignedIntegerType(operation->op);
         auto op_bitstring = get_current(operation->op);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---forward_transfer, operand(" + STR(operation->op->index) +
                            "): " + bitstring_to_string(op_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---=> " + tree_node::GetString(rhs_kind));

         if(rhs_kind == abs_expr_K)
         {
            const auto op_size = tree_helper::TypeSize(operation->op);
            const auto sign_bit = op_bitstring.front();
            switch(sign_bit)
            {
               case bit_lattice::ZERO:
               {
                  res = op_bitstring;
                  break;
               }
               case bit_lattice::ONE:
               {
                  bit_lattice borrow = bit_lattice::ZERO;
                  for(const auto bit : boost::adaptors::reverse(op_bitstring))
                  {
                     const auto borrow_and_bit_pair = minus_expr_map.at(bit_lattice::ZERO).at(bit).at(borrow);
                     res.push_front(borrow_and_bit_pair.back());
                     borrow = borrow_and_bit_pair.front();
                  }
                  if(res.size() < op_size)
                  {
                     res.push_front(minus_expr_map.at(bit_lattice::ZERO).at(op_bitstring.front()).at(borrow).back());
                  }
                  break;
               }
               case bit_lattice::X:
               case bit_lattice::U:
               {
                  std::deque<bit_lattice> negated_bitstring;
                  bit_lattice borrow = bit_lattice::ZERO;
                  for(const auto& bit : boost::adaptors::reverse(op_bitstring))
                  {
                     const auto borrow_and_bit_pair = minus_expr_map.at(bit_lattice::ZERO).at(bit).at(borrow);
                     negated_bitstring.push_front(borrow_and_bit_pair.back());
                     borrow = borrow_and_bit_pair.front();
                  }
                  if(negated_bitstring.size() < op_size)
                  {
                     negated_bitstring.push_front(
                         minus_expr_map.at(bit_lattice::ZERO).at(op_bitstring.front()).at(borrow).back());
                  }
                  res = inf(op_bitstring, negated_bitstring, lhs);
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("unexpected bit lattice for sign bit " + bitstring_to_string(op_bitstring));
                  break;
               }
            }
            THROW_ASSERT(lhs_signed && op_signed, "lhs and rhs of an abs_expr must be signed");
         }
         else if(rhs_kind == bit_not_expr_K)
         {
            if(op_bitstring.size() == 1 && op_bitstring.at(0) == bit_lattice::X &&
               !tree_helper::IsBooleanType(operation->op) && !op_signed)
            {
               op_bitstring.push_front(bit_lattice::ZERO);
            }
            if(op_bitstring.size() < lhs_size)
            {
               op_bitstring = sign_extend_bitstring(op_bitstring, op_signed, lhs_size);
            }

            auto op_it = op_bitstring.rbegin();
            for(unsigned bit_index = 0; bit_index < lhs_size && op_it != op_bitstring.rend(); ++op_it, ++bit_index)
            {
               res.push_front(bit_xor_expr_map.at(*op_it).at(bit_lattice::ONE));
            }
         }
         else if(rhs_kind == convert_expr_K || rhs_kind == nop_expr_K || rhs_kind == view_convert_expr_K)
         {
            res = op_bitstring;
            const auto do_not_extend = lhs_signed && lhs_size == 1 && tree_helper::IsBooleanType(operation->op);
            if((lhs_signed != op_signed && !do_not_extend) && res.size() < lhs_size)
            {
               res = sign_extend_bitstring(res, op_signed, lhs_size);
            }
            while(res.size() > lhs_size)
            {
               res.pop_front();
            }
         }
         else if(rhs_kind == negate_expr_K)
         {
            bit_lattice borrow = bit_lattice::ZERO;
            if(!lhs_signed && lhs_size > op_bitstring.size())
            {
               op_bitstring = sign_extend_bitstring(op_bitstring, lhs_signed, lhs_size);
            }
            auto op_it = op_bitstring.rbegin();
            for(unsigned bit_index = 0; bit_index < lhs_size && op_it != op_bitstring.rend(); ++op_it, ++bit_index)
            {
               res.push_front(minus_expr_map.at(bit_lattice::ZERO).at(*op_it).at(borrow).back());
               borrow = minus_expr_map.at(bit_lattice::ZERO).at(*op_it).at(borrow).front();
            }
            if(lhs_signed && res.size() < lhs_size)
            {
               res.push_front(minus_expr_map.at(bit_lattice::ZERO).at(op_bitstring.front()).at(borrow).back());
            }
         }
         else if(rhs_kind == truth_not_expr_K)
         {
            THROW_ASSERT(tree_helper::IsBooleanType(lhs) || lhs_size == 1, "");
            bit_lattice arg_left = bit_lattice::ZERO;
            for(auto current_bit : op_bitstring)
            {
               if(current_bit == bit_lattice::ONE)
               {
                  arg_left = bit_lattice::ONE;
                  break;
               }
               else if(current_bit == bit_lattice::U)
               {
                  arg_left = bit_lattice::U;
               }
            }
            res.push_front(bit_xor_expr_map.at(arg_left).at(bit_lattice::ONE));
         }
         else
         {
            THROW_UNREACHABLE("Unhadled unary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) +
                              ")");
         }
         break;
      }
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case eq_expr_K:
      case exact_div_expr_K:
      case ge_expr_K:
      case gt_expr_K:
      case le_expr_K:
      case lrotate_expr_K:
      case lshift_expr_K:
      case lt_expr_K:
      case max_expr_K:
      case min_expr_K:
      case minus_expr_K:
      case mult_expr_K:
      case ne_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case widen_mult_expr_K:
      case extract_bit_expr_K:
      {
         const auto operation = GetPointerS<const binary_expr>(rhs);

         if(!IsHandledByBitvalue(operation->op0))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---operand " + STR(operation->op0) + " of type " +
                               STR(tree_helper::CGetType(operation->op0)) + " not handled by bitvalue");
            res = create_u_bitstring(lhs_size);
            break;
         }

         if(!IsHandledByBitvalue(operation->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---operand " + STR(operation->op1) + " of type " +
                               STR(tree_helper::CGetType(operation->op1)) + " not handled by bitvalue");
            res = create_u_bitstring(lhs_size);
            break;
         }

         const auto op0_signed = IsSignedIntegerType(operation->op0);
         auto op0_bitstring = get_current(operation->op0);

         const auto op1_signed = IsSignedIntegerType(operation->op1);
         auto op1_bitstring = get_current(operation->op1);

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---forward_transfer");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---   operand0(" + STR(operation->op0->index) + "): " + bitstring_to_string(op0_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---   operand1(" + STR(operation->op1->index) + "): " + bitstring_to_string(op1_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---=> " + tree_node::GetString(rhs_kind));

         if(rhs_kind == bit_and_expr_K)
         {
            if(lhs_size > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, lhs_size);
            }
            if(lhs_size > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, lhs_size);
            }

            auto op0_it = op0_bitstring.rbegin();
            auto op1_it = op1_bitstring.rbegin();
            for(auto bit_index = 0u;
                bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend();
                ++op0_it, ++op1_it, ++bit_index)
            {
               res.push_front(bit_and_expr_map.at(*op0_it).at(*op1_it));
            }
         }
         else if(rhs_kind == bit_ior_expr_K)
         {
            if(op0_bitstring.size() == 1 && op0_bitstring.at(0) == bit_lattice::X &&
               !tree_helper::IsBooleanType(operation->op0) && !op0_signed)
            {
               op0_bitstring.push_front(bit_lattice::ZERO);
            }
            if(op1_bitstring.size() == 1 && op1_bitstring.at(0) == bit_lattice::X &&
               !tree_helper::IsBooleanType(operation->op1) && !op1_signed)
            {
               op1_bitstring.push_front(bit_lattice::ZERO);
            }
            if(lhs_size > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, lhs_size);
            }
            if(lhs_size > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, lhs_size);
            }

            auto op0_it = op0_bitstring.rbegin();
            auto op1_it = op1_bitstring.rbegin();
            for(auto bit_index = 0u;
                bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend();
                ++op0_it, ++op1_it, ++bit_index)
            {
               res.push_front(bit_ior_expr_map.at(*op0_it).at(*op1_it));
            }
         }
         else if(rhs_kind == bit_xor_expr_K)
         {
            if(op0_bitstring.size() == 1 && op0_bitstring.at(0) == bit_lattice::X &&
               !tree_helper::IsBooleanType(operation->op0) && !op0_signed)
            {
               op0_bitstring.push_front(bit_lattice::ZERO);
            }
            if(op1_bitstring.size() == 1 && op1_bitstring.at(0) == bit_lattice::X &&
               !tree_helper::IsBooleanType(operation->op1) && !op1_signed)
            {
               op1_bitstring.push_front(bit_lattice::ZERO);
            }

            if(lhs_size > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, lhs_size);
            }
            if(lhs_size > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, lhs_size);
            }

            auto op0_it = op0_bitstring.rbegin();
            auto op1_it = op1_bitstring.rbegin();
            for(auto bit_index = 0u;
                bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend();
                ++op0_it, ++op1_it, ++bit_index)
            {
               res.push_front(bit_xor_expr_map.at(*op0_it).at(*op1_it));
            }
         }
         else if(rhs_kind == eq_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            auto op0_bitstring_it = op0_bitstring.begin();
            auto op1_bitstring_it = op1_bitstring.begin();
            auto computed_result = false;
            for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end();
                ++op0_bitstring_it, ++op1_bitstring_it)
            {
               if(*op0_bitstring_it == bit_lattice::U || *op1_bitstring_it == bit_lattice::U)
               {
                  // <U> is UNKNOWN
                  res.push_front(bit_lattice::U);
                  computed_result = true;
                  break;
               }
               else if((*op0_bitstring_it == bit_lattice::ZERO && *op1_bitstring_it == bit_lattice::ONE) ||
                       (*op0_bitstring_it == bit_lattice::ONE && *op1_bitstring_it == bit_lattice::ZERO))
               {
                  // <0> is FALSE
                  res.push_front(bit_lattice::ZERO);
                  computed_result = true;
                  break;
               }
            }
            if(!computed_result)
            {
               // If the result is not computed until now the bitstring are equal
               // <1> is TRUE
               res.push_front(bit_lattice::ONE);
            }
         }
         else if(rhs_kind == exact_div_expr_K || rhs_kind == trunc_div_expr_K)
         {
            if(op0_signed)
            {
               res = create_u_bitstring(1u + static_cast<unsigned int>(op0_bitstring.size()));
            }
            else
            {
               res = create_u_bitstring(static_cast<unsigned int>(op0_bitstring.size()));
            }
            while(res.size() > lhs_size)
            {
               res.pop_front();
            }
         }
         else if(rhs_kind == ge_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            const auto is_signed_var = op0_signed || op1_signed;
            auto op0_bitstring_it = op0_bitstring.begin();
            auto op1_bitstring_it = op1_bitstring.begin();
            auto computed_result = false;
            for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end();
                ++op0_bitstring_it, ++op1_bitstring_it)
            {
               if(*op0_bitstring_it == bit_lattice::U || *op1_bitstring_it == bit_lattice::U)
               {
                  // <U> is UNKNOWN
                  res.push_front(bit_lattice::U);
                  computed_result = true;
                  break;
               }
               else if(*op0_bitstring_it == bit_lattice::ONE && *op1_bitstring_it == bit_lattice::ZERO)
               {
                  if(op0_bitstring_it == op0_bitstring.begin() && is_signed_var)
                  {
                     // <0> is FALSE
                     res.push_front(bit_lattice::ZERO);
                  }
                  else
                  {
                     // <1> is TRUE
                     res.push_front(bit_lattice::ONE);
                  }
                  computed_result = true;
                  break;
               }
               else if(*op0_bitstring_it == bit_lattice::ZERO && *op1_bitstring_it == bit_lattice::ONE)
               {
                  if(op0_bitstring_it == op0_bitstring.begin() && is_signed_var)
                  {
                     // <1> is TRUE
                     res.push_front(bit_lattice::ONE);
                  }
                  else
                  {
                     // <0> is FALSE
                     res.push_front(bit_lattice::ZERO);
                  }
                  computed_result = true;
                  break;
               }
            }
            if(!computed_result)
            {
               // If the result is not computed until now the bitstring are equal
               // <1> is TRUE
               res.push_front(bit_lattice::ONE);
            }
         }
         else if(rhs_kind == gt_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            const auto is_signed_var = op0_signed || op1_signed;
            auto op0_bitstring_it = op0_bitstring.begin();
            auto op1_bitstring_it = op1_bitstring.begin();
            auto computed_result = false;
            for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end();
                ++op0_bitstring_it, ++op1_bitstring_it)
            {
               if(*op0_bitstring_it == bit_lattice::U || *op1_bitstring_it == bit_lattice::U)
               {
                  // <U> is UNKNOWN
                  res.push_front(bit_lattice::U);
                  computed_result = true;
                  break;
               }
               else if(*op0_bitstring_it == bit_lattice::ONE && *op1_bitstring_it == bit_lattice::ZERO)
               {
                  if(op0_bitstring_it == op0_bitstring.begin() && is_signed_var)
                  {
                     // <0> is FALSE
                     res.push_front(bit_lattice::ZERO);
                  }
                  else
                  {
                     // <1> is TRUE
                     res.push_front(bit_lattice::ONE);
                  }
                  computed_result = true;
                  break;
               }
               else if(*op0_bitstring_it == bit_lattice::ZERO && *op1_bitstring_it == bit_lattice::ONE)
               {
                  if(op0_bitstring_it == op0_bitstring.begin() && is_signed_var)
                  {
                     // <1> is TRUE
                     res.push_front(bit_lattice::ONE);
                  }
                  else
                  {
                     // <0> is FALSE
                     res.push_front(bit_lattice::ZERO);
                  }
                  computed_result = true;
                  break;
               }
            }
            if(!computed_result)
            {
               // If the result is not computed until now the bitstring are equal
               // <0> is FALSE
               res.push_front(bit_lattice::ZERO);
            }
         }
         else if(rhs_kind == le_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            const auto is_signed_var = op0_signed || op1_signed;
            auto op0_bitstring_it = op0_bitstring.begin();
            auto op1_bitstring_it = op1_bitstring.begin();
            auto computed_result = false;
            for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end();
                ++op0_bitstring_it, ++op1_bitstring_it)
            {
               if(*op0_bitstring_it == bit_lattice::U || *op1_bitstring_it == bit_lattice::U)
               {
                  // <U> is UNKNOWN
                  res.push_front(bit_lattice::U);
                  computed_result = true;
                  break;
               }
               else if(*op0_bitstring_it == bit_lattice::ZERO && *op1_bitstring_it == bit_lattice::ONE)
               {
                  if(op0_bitstring_it == op0_bitstring.begin() && is_signed_var)
                  {
                     // <0> is FALSE
                     res.push_front(bit_lattice::ZERO);
                  }
                  else
                  {
                     // <1> is TRUE
                     res.push_front(bit_lattice::ONE);
                  }
                  computed_result = true;
                  break;
               }
               else if(*op0_bitstring_it == bit_lattice::ONE && *op1_bitstring_it == bit_lattice::ZERO)
               {
                  if(op0_bitstring_it == op0_bitstring.begin() && is_signed_var)
                  {
                     // <1> is TRUE
                     res.push_front(bit_lattice::ONE);
                  }
                  else
                  {
                     // <0> is FALSE
                     res.push_front(bit_lattice::ZERO);
                  }
                  computed_result = true;
                  break;
               }
            }
            if(!computed_result)
            {
               // If the result is not computed until now the bitstring are equal
               // <1> is TRUE
               res.push_front(bit_lattice::ONE);
            }
         }
         else if(rhs_kind == lrotate_expr_K)
         {
            if(operation->op1->get_kind() == ssa_name_K)
            {
               res = create_u_bitstring(lhs_size);
            }
            else if(operation->op1->get_kind() == integer_cst_K)
            {
               const auto arg2_value = tree_helper::GetConstValue(operation->op1);

               if(lhs_size > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, lhs_size);
               }
               res = op0_bitstring;
               for(integer_cst_t index = 0; index < arg2_value; ++index)
               {
                  bit_lattice cur_bit = res.front();
                  res.pop_front();
                  res.push_back(cur_bit);
               }
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else if(rhs_kind == lshift_expr_K)
         {
            if(operation->op1->get_kind() == ssa_name_K)
            {
               const auto bsize_elev2 = 1ULL << op1_bitstring.size();
               if(lhs_size < bsize_elev2 || lhs_size < bsize_elev2 + op0_bitstring.size())
               {
                  res = create_u_bitstring(lhs_size);
               }
               else
               {
                  res = create_u_bitstring(static_cast<unsigned int>(op0_bitstring.size() + bsize_elev2));
               }
            }
            else if(operation->op1->get_kind() == integer_cst_K)
            {
               const auto cst_val = tree_helper::GetConstValue(operation->op1);
               if(cst_val < 0)
               {
                  res.push_back(bit_lattice::X);
                  break;
               }

               const auto op0_bitsize = tree_helper::TypeSize(operation->op0);
               if(lhs_size <= static_cast<size_t>(cst_val))
               {
                  res.push_front(bit_lattice::ZERO);
               }
               else
               {
                  res = op0_bitstring;
                  while(res.size() > op0_bitsize)
                  {
                     res.pop_front();
                  }
                  for(integer_cst_t i = 0; i < cst_val; i++)
                  {
                     res.push_back(bit_lattice::ZERO);
                     if(res.size() > lhs_size)
                     {
                        res.pop_front();
                     }
                  }
               }
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else if(rhs_kind == lt_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            const auto is_signed_var = op0_signed || op1_signed;
            auto op0_bitstring_it = op0_bitstring.begin();
            auto op1_bitstring_it = op1_bitstring.begin();
            auto computed_result = false;
            for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end();
                ++op0_bitstring_it, ++op1_bitstring_it)
            {
               if(*op0_bitstring_it == bit_lattice::U || *op1_bitstring_it == bit_lattice::U)
               {
                  // <U> is UNKNOWN
                  res.push_front(bit_lattice::U);
                  computed_result = true;
                  break;
               }
               else if(*op0_bitstring_it == bit_lattice::ZERO && *op1_bitstring_it == bit_lattice::ONE)
               {
                  if(op0_bitstring_it == op0_bitstring.begin() && is_signed_var)
                  {
                     // <0> is FALSE
                     res.push_front(bit_lattice::ZERO);
                  }
                  else
                  {
                     // <1> is TRUE
                     res.push_front(bit_lattice::ONE);
                  }
                  computed_result = true;
                  break;
               }
               else if(*op0_bitstring_it == bit_lattice::ONE && *op1_bitstring_it == bit_lattice::ZERO)
               {
                  if(op0_bitstring_it == op0_bitstring.begin() && is_signed_var)
                  {
                     // <1> is TRUE
                     res.push_front(bit_lattice::ONE);
                  }
                  else
                  {
                     // <0> is FALSE
                     res.push_front(bit_lattice::ZERO);
                  }
                  computed_result = true;
                  break;
               }
            }
            if(!computed_result)
            {
               // If the result is not computed until now the bitstring are equal
               // <0> is FALSE
               res.push_front(bit_lattice::ZERO);
            }
         }
         else if(rhs_kind == max_expr_K || rhs_kind == min_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            THROW_ASSERT(op0_signed == op1_signed, "");
            res = inf(op0_bitstring, op1_bitstring, operation->op0);
         }
         else if(rhs_kind == minus_expr_K)
         {
            const auto arg_size_max = std::max(op0_bitstring.size(), op1_bitstring.size());
            if(arg_size_max > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, arg_size_max);
            }
            if(arg_size_max > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, arg_size_max);
            }

            auto op0_it = op0_bitstring.rbegin();
            auto op1_it = op1_bitstring.rbegin();
            auto borrow = bit_lattice::ZERO;
            for(auto bit_index = 0u;
                bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend();
                ++op0_it, ++op1_it, ++bit_index)
            {
               res.push_front(minus_expr_map.at(*op0_it).at(*op1_it).at(borrow).back());
               borrow = minus_expr_map.at(*op0_it).at(*op1_it).at(borrow).front();
            }
            if(lhs_signed && res.size() < lhs_size)
            {
               res.push_front(minus_expr_map.at(op0_bitstring.front()).at(op1_bitstring.front()).at(borrow).back());
            }
            else if(!lhs_signed)
            {
               while(res.size() < lhs_size)
               {
                  res.push_front(borrow);
               }
            }
         }
         else if(rhs_kind == mult_expr_K || rhs_kind == widen_mult_expr_K)
         {
            //    auto mult0 = [&] {
            //       if(op0_bitstring.size() > op1_bitstring.size())
            //       {
            //          op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            //       }
            //       if(op1_bitstring.size() > op0_bitstring.size())
            //       {
            //          op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            //       }
            //
            //       auto op0_it = op0_bitstring.rbegin();
            //       auto op1_it = op1_bitstring.rbegin();
            //       auto op0_it_fw = op0_bitstring.begin();
            //       auto op1_it_fw = op1_bitstring.begin();
            //       // trailing zeros of a
            //       unsigned int ta = 0;
            //       // trailing zeros of b
            //       unsigned int tb = 0;
            //       // leading zeros of a
            //       unsigned int la = 0;
            //       // leading zeros of b
            //       unsigned int lb = 0;
            //       for(; op0_it != op0_bitstring.rend() && *op0_it == bit_lattice::ZERO; ++op0_it, ++ta)
            //          ;
            //
            //       for(; op1_it != op1_bitstring.rend() && *op1_it == bit_lattice::ZERO; ++op1_it, ++tb)
            //          ;
            //
            //       for(; op0_it_fw != op0_bitstring.end() && *op0_it_fw == bit_lattice::ZERO; ++op0_it_fw, ++la)
            //          ;
            //
            //       for(; op1_it_fw != op1_bitstring.end() && *op1_it_fw == bit_lattice::ZERO; ++op1_it_fw, ++lb)
            //          ;
            //
            //       // if one of the two arguments is all zeros returns zero
            //       if(la == op0_bitstring.size() || lb == op1_bitstring.size())
            //       {
            //          res.push_back(bit_lattice::ZERO);
            //       }
            //       else
            //       {
            //          size_t lenght_op0 = op0_bitstring.size();
            //          size_t lenght_op1 = op1_bitstring.size();
            //          THROW_ASSERT(static_cast<int>(lenght_op0 + lenght_op1 - ta - tb - la - lb) > 0, "unexpected
            //          condition"); res = create_u_bitstring(static_cast<unsigned int>(lenght_op0 + lenght_op1 - ta -
            //          tb - la - lb)); for(unsigned int i = 0; i < ta + tb; i++)
            //             res.push_back(bit_lattice::ZERO);
            //          if(lhs_signed && la > 0 && lb > 0)
            //             res.push_front(bit_lattice::ZERO);
            //          while(res.size() > res_bitsize)
            //             res.pop_front();
            //       }
            //    };
            //    if(0)
            //       mult0();
            const auto lenght_op0 = op0_bitstring.size();
            const auto lenght_op1 = op1_bitstring.size();
            const auto res_bitsize = std::min(lenght_op0 + lenght_op1, static_cast<size_t>(lhs_size));
            if(res_bitsize > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, res_bitsize);
            }
            if(res_bitsize > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, res_bitsize);
            }
            while(op0_bitstring.size() > res_bitsize)
            {
               op0_bitstring.pop_front();
            }
            while(op1_bitstring.size() > res_bitsize)
            {
               op1_bitstring.pop_front();
            }

            while(res.size() < res_bitsize)
            {
               res.push_front(bit_lattice::ZERO);
            }
            auto op1_it = op1_bitstring.crbegin();
            for(auto pos = 0u; op1_it != op1_bitstring.crend() && pos < res_bitsize; ++op1_it, ++pos)
            {
               std::deque<bit_lattice> temp_op1;
               while(temp_op1.size() < pos)
               {
                  temp_op1.push_front(bit_lattice::ZERO);
               }
               auto op0_it = op0_bitstring.crbegin();
               for(size_t idx = 0; (idx + pos) < res_bitsize; ++idx, ++op0_it)
               {
                  temp_op1.push_front(bit_and_expr_map.at(*op0_it).at(*op1_it));
               }
               bit_lattice carry1 = bit_lattice::ZERO;
               std::deque<bit_lattice> temp_res;
               auto temp_op1_it = temp_op1.crbegin();
               const auto temp_op1_end = temp_op1.crend();
               auto res_it = res.crbegin();
               const auto res_end = res.crend();
               for(auto bit_index = 0u; bit_index < res_bitsize && temp_op1_it != temp_op1_end && res_it != res_end;
                   temp_op1_it++, res_it++, bit_index++)
               {
                  temp_res.push_front(plus_expr_map.at(*temp_op1_it).at(*res_it).at(carry1).back());
                  carry1 = plus_expr_map.at(*temp_op1_it).at(*res_it).at(carry1).front();
               }
               res = temp_res;
            }
         }
         else if(rhs_kind == ne_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            auto op0_bitstring_it = op0_bitstring.begin();
            auto op1_bitstring_it = op1_bitstring.begin();
            auto computed_result = false;
            for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end();
                ++op0_bitstring_it, ++op1_bitstring_it)
            {
               if(*op0_bitstring_it == bit_lattice::U || *op1_bitstring_it == bit_lattice::U)
               {
                  // <U> is UNKNOWN
                  res.push_front(bit_lattice::U);
                  computed_result = true;
                  break;
               }
               else if((*op0_bitstring_it == bit_lattice::ZERO && *op1_bitstring_it == bit_lattice::ONE) ||
                       (*op0_bitstring_it == bit_lattice::ONE && *op1_bitstring_it == bit_lattice::ZERO))
               {
                  // <1> is TRUE
                  res.push_front(bit_lattice::ONE);
                  computed_result = true;
                  break;
               }
            }
            if(!computed_result)
            {
               // If the result is not computed until now the bitstring are equal
               // <0> is FALSE
               res.push_front(bit_lattice::ZERO);
            }
         }
         else if(rhs_kind == plus_expr_K || rhs_kind == pointer_plus_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            auto op0_it = op0_bitstring.rbegin();
            auto op1_it = op1_bitstring.rbegin();
            auto carry1 = bit_lattice::ZERO;
            for(auto bit_index = 0u;
                bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend();
                ++op0_it, ++op1_it, ++bit_index)
            {
               // INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "op0: "+STR(*op0_it) + "op1: "STR(*op1_it) + "carry:
               // " + STR(carry1));
               res.push_front(plus_expr_map.at(*op0_it).at(*op1_it).at(carry1).back());
               carry1 = plus_expr_map.at(*op0_it).at(*op1_it).at(carry1).front();
            }

            if(lhs_signed && res.size() < lhs_size)
            {
               res.push_front(plus_expr_map.at(op0_bitstring.front()).at(op1_bitstring.front()).at(carry1).back());
            }
            else if(!lhs_signed)
            {
               while(res.size() < lhs_size)
               {
                  res.push_front(plus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).back());
                  carry1 = plus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).front();
               }
            }
         }
         else if(rhs_kind == rrotate_expr_K)
         {
            if(operation->op1->get_kind() == ssa_name_K)
            {
               res = create_u_bitstring(lhs_size);
            }
            else if(operation->op1->get_kind() == integer_cst_K)
            {
               const auto op1_value = tree_helper::GetConstValue(operation->op1);

               if(lhs_size > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, lhs_size);
               }
               res = op0_bitstring;
               for(integer_cst_t index = 0; index < op1_value; ++index)
               {
                  const auto cur_bit = res.back();
                  res.pop_back();
                  res.push_front(cur_bit);
               }
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else if(rhs_kind == rshift_expr_K)
         {
            if(operation->op1->get_kind() == ssa_name_K)
            {
               res = create_u_bitstring(static_cast<unsigned int>(op0_bitstring.size()));
            }
            else if(operation->op1->get_kind() == integer_cst_K)
            {
               const auto cst_val = tree_helper::GetConstValue(operation->op1);
               if(cst_val < 0)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---forward_transfer, negative right shift is undefined behavior");
                  res.push_back(bit_lattice::X);
                  break;
               }

               if(op0_bitstring.size() <= static_cast<size_t>(cst_val))
               {
                  if(op0_signed)
                  {
                     res.push_front(op0_bitstring.front());
                  }
                  else
                  {
                     res.push_front(bit_lattice::ZERO);
                  }
               }
               else
               {
                  const auto new_lenght = op0_bitstring.size() - static_cast<size_t>(cst_val);
                  auto op0_it = op0_bitstring.begin();
                  while(res.size() < new_lenght)
                  {
                     res.push_back(*op0_it);
                     ++op0_it;
                  }
               }
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else if(rhs_kind == trunc_mod_expr_K)
         {
            if(op0_signed)
            {
               res = create_u_bitstring(
                   1 + static_cast<unsigned int>(std::min(op0_bitstring.size(), op1_bitstring.size())));
            }
            else
            {
               res =
                   create_u_bitstring(static_cast<unsigned int>(std::min(op0_bitstring.size(), op1_bitstring.size())));
            }
            while(res.size() > lhs_size)
            {
               res.pop_front();
            }
         }
         else if(rhs_kind == truth_and_expr_K || rhs_kind == truth_andif_expr_K || rhs_kind == truth_or_expr_K ||
                 rhs_kind == truth_orif_expr_K || rhs_kind == truth_xor_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            auto arg_left = bit_lattice::ZERO;
            for(const auto& current_bit : op0_bitstring)
            {
               if(current_bit == bit_lattice::ONE)
               {
                  arg_left = bit_lattice::ONE;
                  break;
               }
               else if(current_bit == bit_lattice::U)
               {
                  arg_left = bit_lattice::U;
               }
            }
            auto arg_right = bit_lattice::ZERO;
            for(const auto& current_bit : op1_bitstring)
            {
               if(current_bit == bit_lattice::ONE)
               {
                  arg_right = bit_lattice::ONE;
                  break;
               }
               else if(current_bit == bit_lattice::U)
               {
                  arg_right = bit_lattice::U;
               }
            }
            if(rhs_kind == truth_and_expr_K || rhs_kind == truth_andif_expr_K)
            {
               res.push_front(bit_and_expr_map.at(arg_left).at(arg_right));
            }
            else if(rhs_kind == truth_or_expr_K || rhs_kind == truth_orif_expr_K)
            {
               res.push_front(bit_ior_expr_map.at(arg_left).at(arg_right));
            }
            else if(rhs_kind == truth_xor_expr_K)
            {
               res.push_front(bit_xor_expr_map.at(arg_left).at(arg_right));
            }
            else
            {
               THROW_UNREACHABLE("unexpected condition");
            }
         }
         else if(rhs_kind == extract_bit_expr_K)
         {
            THROW_ASSERT(operation->op1->get_kind() == integer_cst_K, "unexpected condition");
            const auto cst_val = tree_helper::GetConstValue(operation->op1);
            THROW_ASSERT(cst_val >= 0, "unexpected condition");

            if(op0_bitstring.size() <= static_cast<size_t>(cst_val))
            {
               if(op0_signed)
               {
                  res.push_front(op0_bitstring.front());
               }
               else
               {
                  res.push_front(bit_lattice::ZERO);
               }
            }
            else
            {
               const auto new_lenght = op0_bitstring.size() - static_cast<size_t>(cst_val);
               auto op0_it = op0_bitstring.begin();
               while(res.size() < new_lenght)
               {
                  res.push_back(*op0_it);
                  ++op0_it;
               }
               while(res.size() > 1)
               {
                  res.pop_front();
               }
            }
         }
         else
         {
            THROW_UNREACHABLE("Unhadled binary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) +
                              ")");
         }
         break;
      }
      case bit_ior_concat_expr_K:
      case cond_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case fshl_expr_K:
      case fshr_expr_K:
      {
         const auto operation = GetPointerS<const ternary_expr>(rhs);

         if(!IsHandledByBitvalue(operation->op0))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---operand " + STR(operation->op0) + " of type " +
                               STR(tree_helper::CGetType(operation->op0)) + " not handled by bitvalue");
            res = create_u_bitstring(lhs_size);
            break;
         }

         if(!IsHandledByBitvalue(operation->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---operand " + STR(operation->op1) + " of type " +
                               STR(tree_helper::CGetType(operation->op1)) + " not handled by bitvalue");
            res = create_u_bitstring(lhs_size);
            break;
         }

         if(!IsHandledByBitvalue(operation->op2))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---operand " + STR(operation->op2) + " of type " +
                               STR(tree_helper::CGetType(operation->op2)) + " not handled by bitvalue");
            res = create_u_bitstring(lhs_size);
            break;
         }

         const auto op0_signed = IsSignedIntegerType(operation->op0);
         auto op0_bitstring = get_current(operation->op0);

         const auto op1_signed = IsSignedIntegerType(operation->op1);
         auto op1_bitstring = get_current(operation->op1);

         auto op2_bitstring = get_current(operation->op2);

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---forward_transfer");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---   operand0(" + STR(operation->op0->index) + "): " + bitstring_to_string(op0_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---   operand1(" + STR(operation->op1->index) + "): " + bitstring_to_string(op1_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---   operand2(" + STR(operation->op2->index) + "): " + bitstring_to_string(op2_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---=> " + tree_node::GetString(rhs_kind));

         if(rhs_kind == bit_ior_concat_expr_K)
         {
            const auto offset = tree_helper::GetConstValue(operation->op2);

            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, op1_bitstring.size());
            }

            auto op0_it = op0_bitstring.rbegin();
            auto op1_it = op1_bitstring.rbegin();
            for(integer_cst_t index = 0; op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend();
                ++op0_it, ++op1_it, ++index)
            {
               if(index < offset)
               {
                  res.push_front(*op1_it);
               }
               else
               {
                  res.push_front(*op0_it);
               }
            }
         }
         else if(rhs_kind == cond_expr_K)
         {
            auto arg_cond = bit_lattice::ZERO;
            for(const auto& current_bit : op0_bitstring)
            {
               if(current_bit == bit_lattice::ONE)
               {
                  arg_cond = bit_lattice::ONE;
                  break;
               }
               else if(current_bit == bit_lattice::U)
               {
                  arg_cond = bit_lattice::U;
               }
            }

            if(arg_cond == bit_lattice::ZERO)
            {
               // Condition is false return second arg (op2_bitstring)
               res = op2_bitstring;
            }
            else if(arg_cond == bit_lattice::ONE)
            {
               // Condition is false return second arg (op2_bitstring)
               res = op1_bitstring;
            }
            else
            {
               // CONDITION IS UNKNOWN ---> RETURN INF OF ARGS
               /*
                * Note: forward transfer for cond_expr is the only case where it may
                * be necessary to compute the inf of the bitstrings of two ssa with
                * different signedness. For this reason the bitstrings of both ssa
                * must be extended with their own signedness before computing the inf.
                * The reason is that otherwise the inf() will sign_extend them with
                * the signedness of the output of the cond_expr, which is not
                * necessarily the same of both the inputs.
                */
               const auto inf_size = std::max(op1_bitstring.size(), op2_bitstring.size());
               if(op1_bitstring.size() < inf_size)
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, inf_size);
               }
               if(op2_bitstring.size() < inf_size)
               {
                  op2_bitstring = sign_extend_bitstring(op2_bitstring, IsSignedIntegerType(operation->op2), inf_size);
               }
               res = inf(op1_bitstring, op2_bitstring, lhs);
            }
         }
         else if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K || rhs_kind == ternary_mp_expr_K ||
                 rhs_kind == ternary_mm_expr_K)
         {
            const auto arg_size_max = std::max({op0_bitstring.size(), op1_bitstring.size(), op2_bitstring.size()});
            if(op0_bitstring.size() < arg_size_max)
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, arg_size_max);
            }
            if(op1_bitstring.size() < arg_size_max)
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, arg_size_max);
            }
            if(op2_bitstring.size() < arg_size_max)
            {
               op2_bitstring = sign_extend_bitstring(op2_bitstring, IsSignedIntegerType(operation->op2), arg_size_max);
            }

            auto op0_it = op0_bitstring.crbegin();
            auto op1_it = op1_bitstring.crbegin();
            const auto op0_end = op0_bitstring.crend();
            const auto op1_end = op1_bitstring.crend();
            auto carry1 = bit_lattice::ZERO;
            std::deque<bit_lattice> res_int;
            for(auto bit_index = 0u; bit_index < lhs_size && op0_it != op0_end && op1_it != op1_end;
                op0_it++, op1_it++, bit_index++)
            {
               if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K)
               {
                  res_int.push_front(plus_expr_map.at(*op0_it).at(*op1_it).at(carry1).back());
                  carry1 = plus_expr_map.at(*op0_it).at(*op1_it).at(carry1).front();
               }
               else
               {
                  res_int.push_front(minus_expr_map.at(*op0_it).at(*op1_it).at(carry1).back());
                  carry1 = minus_expr_map.at(*op0_it).at(*op1_it).at(carry1).front();
               }
            }

            if(lhs_signed && res_int.size() < lhs_size)
            {
               if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K)
               {
                  res_int.push_front(
                      plus_expr_map.at(op0_bitstring.front()).at(op1_bitstring.front()).at(carry1).back());
               }
               else
               {
                  res_int.push_front(
                      minus_expr_map.at(op0_bitstring.front()).at(op1_bitstring.front()).at(carry1).back());
               }
            }
            else if(!lhs_signed)
            {
               while(res_int.size() < lhs_size)
               {
                  if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K)
                  {
                     res_int.push_front(plus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).back());
                     carry1 = plus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).front();
                  }
                  else
                  {
                     res_int.push_front(minus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).back());
                     carry1 = minus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).front();
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---   res_int: " + bitstring_to_string(res_int));

            if(res_int.size() > op2_bitstring.size())
            {
               op2_bitstring =
                   sign_extend_bitstring(op2_bitstring, IsSignedIntegerType(operation->op2), res_int.size());
            }
            auto op2_it = op2_bitstring.crbegin();
            auto res_int_it = res_int.crbegin();
            const auto op2_end = op2_bitstring.crend();
            const auto res_int_end = res_int.crend();
            carry1 = bit_lattice::ZERO;
            for(auto bit_index = 0u; bit_index < lhs_size && op2_it != op2_end && res_int_it != res_int_end;
                op2_it++, res_int_it++, bit_index++)
            {
               if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_mp_expr_K)
               {
                  res.push_front(plus_expr_map.at(*res_int_it).at(*op2_it).at(carry1).back());
                  carry1 = plus_expr_map.at(*res_int_it).at(*op2_it).at(carry1).front();
               }
               else
               {
                  res.push_front(minus_expr_map.at(*res_int_it).at(*op2_it).at(carry1).back());
                  carry1 = minus_expr_map.at(*res_int_it).at(*op2_it).at(carry1).front();
               }
            }

            if(lhs_signed && res.size() < lhs_size)
            {
               if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_mp_expr_K)
               {
                  res.push_front(plus_expr_map.at(res_int.front()).at(op2_bitstring.front()).at(carry1).back());
               }
               else
               {
                  res.push_front(minus_expr_map.at(res_int.front()).at(op2_bitstring.front()).at(carry1).back());
               }
            }
            else if(!lhs_signed)
            {
               while(res.size() < lhs_size)
               {
                  if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_mp_expr_K)
                  {
                     res.push_front(plus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).back());
                     carry1 = plus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).front();
                  }
                  else
                  {
                     res.push_front(minus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).back());
                     carry1 = minus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).front();
                  }
               }
            }
         }
         else if(rhs_kind == fshl_expr_K)
         {
            if(operation->op2->get_kind() == ssa_name_K)
            {
               res = create_u_bitstring(lhs_size);
            }
            else if(operation->op2->get_kind() == integer_cst_K)
            {
               THROW_ASSERT(tree_helper::GetConstValue(operation->op2) >= 0, "");
               const auto offset = static_cast<unsigned int>(tree_helper::GetConstValue(operation->op2) %
                                                             static_cast<unsigned int>(lhs_size));

               if(lhs_size > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, lhs_size);
               }
               if(lhs_size > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, lhs_size);
               }
               res.insert(res.end(), op0_bitstring.begin() + offset, op0_bitstring.end());
               res.insert(res.end(), op1_bitstring.begin(), op1_bitstring.begin() + offset);
               THROW_ASSERT(res.size() == lhs_size, "");
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else if(rhs_kind == fshr_expr_K)
         {
            if(operation->op2->get_kind() == ssa_name_K)
            {
               res = create_u_bitstring(lhs_size);
            }
            else if(operation->op2->get_kind() == integer_cst_K)
            {
               THROW_ASSERT(tree_helper::GetConstValue(operation->op2) >= 0, "");
               const auto offset = static_cast<unsigned int>(tree_helper::GetConstValue(operation->op2) %
                                                             static_cast<unsigned int>(lhs_size));

               if(lhs_size > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, op0_signed, lhs_size);
               }
               if(lhs_size > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, op1_signed, lhs_size);
               }
               res.insert(res.end(), op0_bitstring.begin() + offset, op0_bitstring.end());
               res.insert(res.end(), op1_bitstring.begin(), op1_bitstring.begin() + offset);
               THROW_ASSERT(res.size() == lhs_size, "");
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else
         {
            THROW_UNREACHABLE("Unhadled ternary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) +
                              ")");
         }
         break;
      }
      // Unary expressions
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fix_trunc_expr_K:
      case imagpart_expr_K:
      case realpart_expr_K:
      case alignof_expr_K:
      // Binary expressions
      case ltgt_expr_K:
      case mem_ref_K:
      case ordered_expr_K:
      case sat_minus_expr_K:
      case sat_plus_expr_K:
      case uneq_expr_K:
      case unge_expr_K:
      case ungt_expr_K:
      case unle_expr_K:
      case unlt_expr_K:
      case unordered_expr_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      // Ternary expressions
      case bit_field_ref_K:
      case component_ref_K:
      case insertvalue_expr_K:
      case insertelement_expr_K:
      // Quaternary expressions
      case array_ref_K:
      {
         // Do nothing
         break;
      }
      case aggr_init_expr_K:
      case call_expr_K:
      {
         const auto call_it = direct_call_id_to_called_id.find(ga->index);
         if(call_it != direct_call_id_to_called_id.end())
         {
            const auto called_id = call_it->second;
            const auto tn = TM->GetTreeNode(called_id);
            THROW_ASSERT(tn->get_kind() == function_decl_K, "node " + STR(called_id) + " is not a function_decl");
            const auto* fd = GetPointerS<const function_decl>(tn);
            if(fd->bit_values.empty())
            {
               res = create_u_bitstring(lhs_size);
            }
            else
            {
               res = string_to_bitstring(fd->bit_values);
            }
         }
         break;
      }
      case lut_expr_K:
      {
         // lut_transformation will take care of this
         res = create_u_bitstring(1);
         break;
      }
      // Unary expressions
      case arrow_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case exit_expr_K:
      case float_expr_K:
      case indirect_ref_K:
      case misaligned_indirect_ref_K:
      case loop_expr_K:
      case non_lvalue_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case paren_expr_K:
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
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      // Quaternary expressions
      case array_range_ref_K:
      // Const nodes
      case complex_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case CASE_DECL_NODES:
      case CASE_TYPE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_FAKE_NODES:
      case CASE_CPP_NODES:
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
         THROW_UNREACHABLE("Unhandled statement: " + ga->ToString() + " (" + tree_node::GetString(rhs_kind) + ")");
         break;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---res: " + bitstring_to_string(res));
   return res;
}
