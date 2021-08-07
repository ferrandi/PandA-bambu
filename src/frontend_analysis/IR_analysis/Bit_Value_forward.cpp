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
 *              Copyright (C) 2004-2021 Politecnico di Milano
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
 */

// include class header
#include "Bit_Value.hpp"

#include "Dominance.hpp"
#include "Parameter.hpp"
#include "basic_block.hpp"
#include "function_behavior.hpp"

// include from tree/
#include "Range.hpp"
#include "application_manager.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "string_manipulation.hpp"
#include <boost/range/adaptors.hpp>
#include <unordered_set>

std::deque<bit_lattice> Bit_Value::get_current(const tree_nodeConstRef& tn) const
{
   const auto nid = GET_INDEX_CONST_NODE(tn);
   const auto node = GET_CONST_NODE(tn);
   if(node->get_kind() == ssa_name_K)
   {
      THROW_ASSERT(current.count(nid), "");
      return current.at(nid);
   }
   else if(GetPointer<const cst_node>(node))
   {
      THROW_ASSERT(best.count(nid), "");
      return best.at(nid);
   }
   THROW_UNREACHABLE("Unexpected node kind: " + node->get_kind_text());
   return std::deque<bit_lattice>();
}

std::deque<bit_lattice> Bit_Value::forward_transfer(const gimple_assign* ga) const
{
   std::deque<bit_lattice> res;
   const auto& lhs = ga->op0;
   const auto& rhs = ga->op1;
   const auto lhs_nid = GET_INDEX_CONST_NODE(lhs);
   const auto lhs_size = BitLatticeManipulator::Size(GET_CONST_NODE(lhs));
   const auto rhs_kind = GET_CONST_NODE(rhs)->get_kind();
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
         auto ae0 = [&] {
            const auto ae = GetPointerS<const addr_expr>(GET_CONST_NODE(rhs));
            const auto address_size = AppM->get_address_bitsize();
            const auto is_pretty_print_used = parameters->isOption(OPT_pretty_print) || (parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy));
            const auto lt0 = lsb_to_zero(ae, is_pretty_print_used);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "address_size: " + STR(address_size) + " lt0: " + STR(lt0));
            if(lt0 && address_size > lt0)
            {
               res = create_u_bitstring(address_size - lt0);
               for(auto index = 0u; index < lt0; ++index)
               {
                  res.push_back(bit_lattice::ZERO);
               }
            }
         };
         ae0();
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
         const auto operation = GetPointer<const unary_expr>(GET_CONST_NODE(rhs));

         const auto op_nid = GET_INDEX_NODE(operation->op);
         if(!is_handled_by_bitvalue(op_nid))
         {
            res = create_u_bitstring(lhs_size);
            break;
         }
         auto op_bitstring = get_current(operation->op);

         if(rhs_kind == abs_expr_K)
         {
            auto ae0 = [&] {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = abs " + STR(op_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " abs");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op_bitstring) + " =");

               const auto op_size = BitLatticeManipulator::Size(GET_CONST_NODE(operation->op));
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
                        negated_bitstring.push_front(minus_expr_map.at(bit_lattice::ZERO).at(op_bitstring.front()).at(borrow).back());
                     }
                     res = inf(op_bitstring, negated_bitstring, lhs_nid);
                     break;
                  }
                  default:
                  {
                     THROW_UNREACHABLE("unexpected bit lattice for sign bit " + bitstring_to_string(op_bitstring));
                     break;
                  }
               }
            };
            THROW_ASSERT(tree_helper::is_int(TM, lhs_nid) and tree_helper::is_int(TM, op_nid), "lhs and rhs of an abs_expr must be signed");
            ae0();
         }
         else if(rhs_kind == bit_not_expr_K)
         {
            auto bne0 = [&] {
               if(op_bitstring.size() == 1 && op_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, op_nid) && !tree_helper::is_int(TM, op_nid))
               {
                  op_bitstring.push_front(bit_lattice::ZERO);
               }
               if(op_bitstring.size() < lhs_size)
               {
                  op_bitstring = sign_extend_bitstring(op_bitstring, tree_helper::is_int(TM, op_nid), lhs_size);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = ~" + STR(op_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " ~" + bitstring_to_string(op_bitstring) + " =");

               auto op_it = op_bitstring.rbegin();
               for(unsigned bit_index = 0; bit_index < lhs_size && op_it != op_bitstring.rend(); ++op_it, ++bit_index)
               {
                  res.push_front(bit_xor_expr_map.at(*op_it).at(bit_lattice::ONE));
               }
            };
            bne0();
         }
         else if(rhs_kind == convert_expr_K || rhs_kind == nop_expr_K || rhs_kind == view_convert_expr_K)
         {
            res = op_bitstring;
            const auto left_signed = tree_helper::is_int(TM, lhs_nid);
            const auto right_signed = tree_helper::is_int(TM, op_nid);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + (left_signed ? "S" : "U") + " = " + (rhs_kind == nop_expr_K ? "cast" : "convert") + " " + STR(op_nid) + (right_signed ? "S" : "U"));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " = op:  " + bitstring_to_string(res) + "(" + STR(BitLatticeManipulator::Size(GET_CONST_NODE(operation->op))) + "->" + STR(lhs_size) + ")");
            bool do_not_extend = false;
            if(left_signed && lhs_size == 1 && tree_helper::is_bool(TM, op_nid))
            {
               do_not_extend = true;
            }
            if(tree_helper::is_real(TM, lhs_nid) and res.size() < lhs_size)
            {
               res = sign_extend_bitstring(res, res.front() == bit_lattice::U, lhs_size);
            }
            if((left_signed != right_signed and !do_not_extend) and res.size() < lhs_size)
            {
               res = sign_extend_bitstring(res, right_signed, lhs_size);
            }
            while(res.size() > lhs_size)
            {
               res.pop_front();
            }
            THROW_ASSERT(!tree_helper::is_real(TM, lhs_nid) || res.size() == lhs_size, "Real type bit value should be exact");
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(res));
         }
         else if(rhs_kind == negate_expr_K)
         {
            auto ne0 = [&] {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = - " + STR(op_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " -");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op_bitstring) + " =");

               bit_lattice borrow = bit_lattice::ZERO;
               if(!tree_helper::is_int(TM, lhs_nid) && lhs_size > op_bitstring.size())
               {
                  op_bitstring = sign_extend_bitstring(op_bitstring, tree_helper::is_int(TM, lhs_nid), lhs_size);
               }
               auto op_it = op_bitstring.rbegin();
               for(unsigned bit_index = 0; bit_index < lhs_size && op_it != op_bitstring.rend(); ++op_it, ++bit_index)
               {
                  res.push_front(minus_expr_map.at(bit_lattice::ZERO).at(*op_it).at(borrow).back());
                  borrow = minus_expr_map.at(bit_lattice::ZERO).at(*op_it).at(borrow).front();
               }
               if(tree_helper::is_int(TM, lhs_nid) && res.size() < lhs_size)
               {
                  res.push_front(minus_expr_map.at(bit_lattice::ZERO).at(op_bitstring.front()).at(borrow).back());
               }
            };
            ne0();
         }
         else if(rhs_kind == truth_not_expr_K)
         {
            THROW_ASSERT(tree_helper::is_bool(TM, lhs_nid) || lhs_size == 1, "");
            auto tne0 = [&] {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = !" + STR(op_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " !" + bitstring_to_string(op_bitstring) + " =");

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
            };
            tne0();
         }
         else
         {
            THROW_UNREACHABLE("Unhadled unary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) + ")");
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
         const auto operation = GetPointer<const binary_expr>(GET_CONST_NODE(rhs));

         const auto op0_nid = GET_INDEX_NODE(operation->op0);
         auto op0_bitstring = get_current(operation->op0);

         const auto op1_nid = GET_INDEX_NODE(operation->op1);
         auto op1_bitstring = get_current(operation->op1);

         if(rhs_kind == bit_and_expr_K)
         {
            auto bae0 = [&] {
               if(lhs_size > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), lhs_size);
               }
               if(lhs_size > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), lhs_size);
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " & " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " &");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               auto op0_it = op0_bitstring.rbegin();
               auto op1_it = op1_bitstring.rbegin();

               for(unsigned bit_index = 0; bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend(); ++op0_it, ++op1_it, ++bit_index)
               {
                  res.push_front(bit_and_expr_map.at(*op0_it).at(*op1_it));
               }
            };
            bae0();
         }
         else if(rhs_kind == bit_ior_expr_K)
         {
            auto bie0 = [&] {
               if(op0_bitstring.size() == 1 && op0_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, op0_nid) && !tree_helper::is_int(TM, op0_nid))
               {
                  op0_bitstring.push_front(bit_lattice::ZERO);
               }
               if(op1_bitstring.size() == 1 && op1_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, op1_nid) && !tree_helper::is_int(TM, op1_nid))
               {
                  op1_bitstring.push_front(bit_lattice::ZERO);
               }
               if(lhs_size > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), lhs_size);
               }
               if(lhs_size > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), lhs_size);
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " | " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " |");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               auto op0_it = op0_bitstring.rbegin();
               auto op1_it = op1_bitstring.rbegin();

               for(unsigned bit_index = 0; bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend(); ++op0_it, ++op1_it, ++bit_index)
               {
                  res.push_front(bit_ior_expr_map.at(*op0_it).at(*op1_it));
               }
            };
            bie0();
         }
         else if(rhs_kind == bit_xor_expr_K)
         {
            auto bxe0 = [&] {
               if(op0_bitstring.size() == 1 && op0_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, op0_nid) && !tree_helper::is_int(TM, op0_nid))
               {
                  op0_bitstring.push_front(bit_lattice::ZERO);
               }
               if(op1_bitstring.size() == 1 && op1_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, op1_nid) && !tree_helper::is_int(TM, op1_nid))
               {
                  op1_bitstring.push_front(bit_lattice::ZERO);
               }

               if(lhs_size > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), lhs_size);
               }
               if(lhs_size > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), lhs_size);
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " ^ " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " ^");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               auto op0_it = op0_bitstring.rbegin();
               auto op1_it = op1_bitstring.rbegin();

               for(unsigned bit_index = 0; bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend(); ++op0_it, ++op1_it, ++bit_index)
               {
                  res.push_front(bit_xor_expr_map.at(*op0_it).at(*op1_it));
               }
            };
            bxe0();
         }
         else if(rhs_kind == eq_expr_K)
         {
            auto ee0 = [&] {
               if(op0_bitstring.size() > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
               }
               if(op1_bitstring.size() > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " == " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " ==");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " ?");

               auto op0_bitstring_it = op0_bitstring.begin();
               auto op1_bitstring_it = op1_bitstring.begin();
               bool computed_result = false;
               for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end(); ++op0_bitstring_it, ++op1_bitstring_it)
               {
                  if(*op0_bitstring_it == bit_lattice::U || *op1_bitstring_it == bit_lattice::U)
                  {
                     // <U> is UNKNOWN
                     res.push_front(bit_lattice::U);
                     computed_result = true;
                     break;
                  }
                  else if((*op0_bitstring_it == bit_lattice::ZERO && *op1_bitstring_it == bit_lattice::ONE) || (*op0_bitstring_it == bit_lattice::ONE && *op1_bitstring_it == bit_lattice::ZERO))
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
            };
            if(tree_helper::is_real(TM, op0_nid) && tree_helper::is_real(TM, op1_nid))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer Error: operation unhandled yet with real type operands -> " + tree_node::GetString(rhs_kind));
               return res;
            }
            else
            {
               ee0();
            }
         }
         else if(rhs_kind == exact_div_expr_K || rhs_kind == trunc_div_expr_K)
         {
            auto tde0 = [&] {
               if(tree_helper::is_int(TM, op0_nid))
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
            };
            tde0();
         }
         else if(rhs_kind == ge_expr_K)
         {
            auto ge0 = [&] {
               if(op0_bitstring.size() > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
               }
               if(op1_bitstring.size() > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " >= " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " >=");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " ?");

               bool is_signed_var = tree_helper::is_int(TM, op0_nid) || tree_helper::is_int(TM, op1_nid);
               auto op0_bitstring_it = op0_bitstring.begin();
               auto op1_bitstring_it = op1_bitstring.begin();
               bool computed_result = false;
               for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end(); ++op0_bitstring_it, ++op1_bitstring_it)
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
            };
            ge0();
         }
         else if(rhs_kind == gt_expr_K)
         {
            auto gt0 = [&] {
               bool is_signed_var = tree_helper::is_int(TM, op0_nid) || tree_helper::is_int(TM, op1_nid);

               if(op0_bitstring.size() > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
               }
               if(op1_bitstring.size() > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " > " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " >");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " ?");

               auto op0_bitstring_it = op0_bitstring.begin();
               auto op1_bitstring_it = op1_bitstring.begin();
               bool computed_result = false;
               for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end(); ++op0_bitstring_it, ++op1_bitstring_it)
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
            };
            gt0();
         }
         else if(rhs_kind == le_expr_K)
         {
            auto le0 = [&] {
               if(op0_bitstring.size() > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
               }
               if(op1_bitstring.size() > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " <= " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " <=");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " ?");

               bool is_signed_var = tree_helper::is_int(TM, op0_nid) || tree_helper::is_int(TM, op1_nid);
               auto op0_bitstring_it = op0_bitstring.begin();
               auto op1_bitstring_it = op1_bitstring.begin();
               bool computed_result = false;
               for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end(); ++op0_bitstring_it, ++op1_bitstring_it)
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
            };
            le0();
         }
         else if(rhs_kind == lrotate_expr_K)
         {
            if(GET_CONST_NODE(operation->op1)->get_kind() == ssa_name_K)
            {
               res = create_u_bitstring(lhs_size);
            }
            else if(GET_CONST_NODE(operation->op1)->get_kind() == integer_cst_K)
            {
               const auto const2 = GetPointerS<const integer_cst>(GET_CONST_NODE(operation->op1));
               const auto arg2_value = static_cast<unsigned int>(const2->value);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(arg2_value));

               if(lhs_size > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), lhs_size);
               }
               res = op0_bitstring;
               for(unsigned int index = 0; index < arg2_value; ++index)
               {
                  bit_lattice cur_bit = res.front();
                  res.pop_front();
                  res.push_back(cur_bit);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " lrotate " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " lrotate");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, STR(arg2_value) + " =");
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else if(rhs_kind == lshift_expr_K)
         {
            if(GET_CONST_NODE(operation->op1)->get_kind() == ssa_name_K)
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
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " << " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(op0_bitstring) + "<<" + bitstring_to_string(op1_bitstring) + " => " + bitstring_to_string(res));
            }
            else if(GET_CONST_NODE(operation->op1)->get_kind() == integer_cst_K)
            {
               auto* const2 = GetPointerS<const integer_cst>(GET_CONST_NODE(operation->op1));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(const2->value));
               if(const2->value < 0)
               {
                  res.push_back(bit_lattice::X);
                  return res;
               }

               const auto op0_bitsize = BitLatticeManipulator::Size(GET_CONST_NODE(operation->op0));
               if(lhs_size <= static_cast<long long unsigned int>(const2->value))
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
                  for(int i = 0; i < const2->value; i++)
                  {
                     res.push_back(bit_lattice::ZERO);
                     if(res.size() > lhs_size)
                     {
                        res.pop_front();
                     }
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " << " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(op0_bitstring) + "<<" + STR(const2->value) + " => " + bitstring_to_string(res));
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else if(rhs_kind == lt_expr_K)
         {
            auto lt0 = [&] {
               if(op0_bitstring.size() > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
               }
               if(op1_bitstring.size() > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " < " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " <");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " ?");

               bool is_signed_var = tree_helper::is_int(TM, op0_nid) || tree_helper::is_int(TM, op1_nid);
               auto op0_bitstring_it = op0_bitstring.begin();
               auto op1_bitstring_it = op1_bitstring.begin();
               bool computed_result = false;
               for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end(); ++op0_bitstring_it, ++op1_bitstring_it)
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
            };
            lt0();
         }
         else if(rhs_kind == max_expr_K || rhs_kind == min_expr_K)
         {
            auto me0 = [&] {
               if(op0_bitstring.size() > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
               }
               if(op1_bitstring.size() > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " " + (rhs_kind == min_expr_K ? std::string("min") : std::string("max")) + " " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " " + (rhs_kind == min_expr_K ? std::string("min") : std::string("max")) + "");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               THROW_ASSERT(tree_helper::is_int(TM, op0_nid) == tree_helper::is_int(TM, op1_nid), "");
               res = inf(op0_bitstring, op1_bitstring, op0_nid);
            };
            me0();
         }
         else if(rhs_kind == minus_expr_K)
         {
            auto me0 = [&] {
               size_t arg_size_max = std::max(op0_bitstring.size(), op1_bitstring.size());
               if(arg_size_max > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), arg_size_max);
               }
               if(arg_size_max > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), arg_size_max);
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " - " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " -");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               auto op0_it = op0_bitstring.rbegin();
               auto op1_it = op1_bitstring.rbegin();
               bit_lattice borrow = bit_lattice::ZERO;

               for(unsigned bit_index = 0; bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend(); ++op0_it, ++op1_it, ++bit_index)
               {
                  res.push_front(minus_expr_map.at(*op0_it).at(*op1_it).at(borrow).back());
                  borrow = minus_expr_map.at(*op0_it).at(*op1_it).at(borrow).front();
               }
               if(tree_helper::is_int(TM, lhs_nid) && res.size() < lhs_size)
               {
                  res.push_front(minus_expr_map.at(op0_bitstring.front()).at(op1_bitstring.front()).at(borrow).back());
               }
               else if(!tree_helper::is_int(TM, lhs_nid))
               {
                  while(res.size() < lhs_size)
                  {
                     res.push_front(borrow);
                  }
               }
            };
            me0();
         }
         else if(rhs_kind == mult_expr_K || rhs_kind == widen_mult_expr_K)
         {
            //    auto mult0 = [&] {
            //       if(op0_bitstring.size() > op1_bitstring.size())
            //       {
            //          op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
            //       }
            //       if(op1_bitstring.size() > op0_bitstring.size())
            //       {
            //          op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
            //       }
            //
            //       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " * " + STR(op1_nid));
            //       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " *");
            //       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");
            //
            //       std::deque<bit_lattice>::const_reverse_iterator op0_it = op0_bitstring.rbegin();
            //       std::deque<bit_lattice>::const_reverse_iterator op1_it = op1_bitstring.rbegin();
            //       std::deque<bit_lattice>::const_iterator op0_it_fw = op0_bitstring.begin();
            //       std::deque<bit_lattice>::const_iterator op1_it_fw = op1_bitstring.begin();
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
            //          THROW_ASSERT(static_cast<int>(lenght_op0 + lenght_op1 - ta - tb - la - lb) > 0, "unexpected condition");
            //          res = create_u_bitstring(static_cast<unsigned int>(lenght_op0 + lenght_op1 - ta - tb - la - lb));
            //          for(unsigned int i = 0; i < ta + tb; i++)
            //             res.push_back(bit_lattice::ZERO);
            //          if(tree_helper::is_int(TM, lhs_nid) && la > 0 && lb > 0)
            //             res.push_front(bit_lattice::ZERO);
            //          while(res.size() > res_bitsize)
            //             res.pop_front();
            //       }
            //    };
            //    if(0)
            //       mult0();
            auto mult1 = [&] {
               size_t lenght_op0 = op0_bitstring.size();
               size_t lenght_op1 = op1_bitstring.size();
               const auto res_bitsize = std::min(lenght_op0 + lenght_op1, static_cast<size_t>(lhs_size));
               if(res_bitsize > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), res_bitsize);
               }
               if(res_bitsize > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), res_bitsize);
               }
               while(op0_bitstring.size() > res_bitsize)
               {
                  op0_bitstring.pop_front();
               }
               while(op1_bitstring.size() > res_bitsize)
               {
                  op1_bitstring.pop_front();
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " * " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " *");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               while(res.size() < res_bitsize)
               {
                  res.push_front(bit_lattice::ZERO);
               }
               auto op1_it = op1_bitstring.crbegin();
               for(size_t pos = 0; op1_it != op1_bitstring.crend() && pos < res_bitsize; ++op1_it, ++pos)
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
                  for(unsigned bit_index = 0; bit_index < res_bitsize and temp_op1_it != temp_op1_end and res_it != res_end; temp_op1_it++, res_it++, bit_index++)
                  {
                     temp_res.push_front(plus_expr_map.at(*temp_op1_it).at(*res_it).at(carry1).back());
                     carry1 = plus_expr_map.at(*temp_op1_it).at(*res_it).at(carry1).front();
                  }
                  res = temp_res;
               }
            };
            mult1();
         }
         else if(rhs_kind == ne_expr_K)
         {
            auto ne0 = [&] {
               if(op0_bitstring.size() > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
               }
               if(op1_bitstring.size() > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " != " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " !=");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " ?");

               auto op0_bitstring_it = op0_bitstring.begin();
               auto op1_bitstring_it = op1_bitstring.begin();
               bool computed_result = false;
               for(; op0_bitstring_it != op0_bitstring.end() && op1_bitstring_it != op1_bitstring.end(); ++op0_bitstring_it, ++op1_bitstring_it)
               {
                  if(*op0_bitstring_it == bit_lattice::U || *op1_bitstring_it == bit_lattice::U)
                  {
                     // <U> is UNKNOWN
                     res.push_front(bit_lattice::U);
                     computed_result = true;
                     break;
                  }
                  else if((*op0_bitstring_it == bit_lattice::ZERO && *op1_bitstring_it == bit_lattice::ONE) || (*op0_bitstring_it == bit_lattice::ONE && *op1_bitstring_it == bit_lattice::ZERO))
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
            };
            if(tree_helper::is_real(TM, op0_nid) && tree_helper::is_real(TM, op1_nid))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer Error: operation unhandled yet with real type operands -> " + tree_node::GetString(rhs_kind));
            }
            else
            {
               ne0();
            }
         }
         else if(rhs_kind == plus_expr_K || rhs_kind == pointer_plus_expr_K)
         {
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " + " + STR(op1_nid));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " +");
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

            auto op0_it = op0_bitstring.rbegin();
            auto op1_it = op1_bitstring.rbegin();
            auto carry1 = bit_lattice::ZERO;

            for(unsigned bit_index = 0; bit_index < lhs_size && op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend(); ++op0_it, ++op1_it, ++bit_index)
            {
               // INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "op0: "+STR(*op0_it) + "op1: "STR(*op1_it) + "carry: " + STR(carry1));
               res.push_front(plus_expr_map.at(*op0_it).at(*op1_it).at(carry1).back());
               carry1 = plus_expr_map.at(*op0_it).at(*op1_it).at(carry1).front();
            }

            if(tree_helper::is_int(TM, lhs_nid) && res.size() < lhs_size)
            {
               res.push_front(plus_expr_map.at(op0_bitstring.front()).at(op1_bitstring.front()).at(carry1).back());
            }
            else if(!tree_helper::is_int(TM, lhs_nid))
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
            if(GET_CONST_NODE(operation->op1)->get_kind() == ssa_name_K)
            {
               res = create_u_bitstring(lhs_size);
            }
            else if(GET_CONST_NODE(operation->op1)->get_kind() == integer_cst_K)
            {
               auto* const2 = GetPointerS<const integer_cst>(GET_CONST_NODE(operation->op1));
               auto arg2_value = static_cast<unsigned int>(const2->value);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(arg2_value));

               if(lhs_size > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), lhs_size);
               }
               res = op0_bitstring;
               for(unsigned int index = 0; index < arg2_value; ++index)
               {
                  const auto cur_bit = res.back();
                  res.pop_back();
                  res.push_front(cur_bit);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " rrotate " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " rrotate");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, STR(arg2_value) + " =");
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else if(rhs_kind == rshift_expr_K)
         {
            if(GET_CONST_NODE(operation->op1)->get_kind() == ssa_name_K)
            {
               res = create_u_bitstring(static_cast<unsigned int>(op0_bitstring.size()));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " >> " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(res));
            }
            else if(GET_CONST_NODE(operation->op1)->get_kind() == integer_cst_K)
            {
               const auto const2 = GetPointerS<const integer_cst>(GET_CONST_NODE(operation->op1));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, second argument is constant -> " + STR(const2->value));
               if(const2->value < 0)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, negative right shift is undefined behavior");
                  res.push_back(bit_lattice::X);
                  return res;
               }

               if(op0_bitstring.size() <= static_cast<size_t>(const2->value))
               {
                  if(tree_helper::is_int(TM, op0_nid))
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
                  size_t new_lenght = op0_bitstring.size() - static_cast<size_t>(const2->value);
                  auto op0_it = op0_bitstring.begin();
                  while(res.size() < new_lenght)
                  {
                     res.push_back(*op0_it);
                     ++op0_it;
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " >> " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(op0_bitstring) + ">>" + STR(const2->value) + " => " + bitstring_to_string(res));
            }
            else
            {
               THROW_ERROR("unexpected case");
            }
         }
         else if(rhs_kind == trunc_mod_expr_K)
         {
            auto tme0 = [&] {
               if(tree_helper::is_int(TM, op0_nid))
               {
                  res = create_u_bitstring(1 + static_cast<unsigned int>(std::min(op0_bitstring.size(), op1_bitstring.size())));
               }
               else
               {
                  res = create_u_bitstring(static_cast<unsigned int>(std::min(op0_bitstring.size(), op1_bitstring.size())));
               }
               while(res.size() > lhs_size)
               {
                  res.pop_front();
               }
            };
            tme0();
         }
         else if(rhs_kind == truth_and_expr_K || rhs_kind == truth_andif_expr_K)
         {
            auto tae0 = [&] {
               if(op0_bitstring.size() > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
               }
               if(op1_bitstring.size() > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " && " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " &&");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               bit_lattice arg_left = bit_lattice::ZERO;
               for(auto current_bit : op0_bitstring)
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
               bit_lattice arg_right = bit_lattice::ZERO;
               for(auto current_bit : op1_bitstring)
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
               res.push_front(bit_and_expr_map.at(arg_left).at(arg_right));
            };
            tae0();
         }
         else if(rhs_kind == truth_or_expr_K || rhs_kind == truth_orif_expr_K)
         {
            auto toe0 = [&] {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " || " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " ||");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               bit_lattice arg_left = bit_lattice::ZERO;
               for(auto current_bit : op0_bitstring)
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
               bit_lattice arg_right = bit_lattice::ZERO;
               for(auto current_bit : op1_bitstring)
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
               res.push_front(bit_ior_expr_map.at(arg_left).at(arg_right));
            };
            toe0();
         }
         else if(rhs_kind == truth_xor_expr_K)
         {
            auto txe0 = [&] {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " ^ " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " ^");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               bit_lattice arg_left = bit_lattice::ZERO;
               for(auto current_bit : op0_bitstring)
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
               bit_lattice arg_right = bit_lattice::ZERO;
               for(auto current_bit : op1_bitstring)
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

               res.push_front(bit_xor_expr_map.at(arg_left).at(arg_right));
            };
            txe0();
         }
         else if(rhs_kind == extract_bit_expr_K)
         {
            auto ebe0 = [&] {
               THROW_ASSERT(GET_CONST_NODE(operation->op1)->get_kind() == integer_cst_K, "unexpected condition");
               const auto const2 = GetPointerS<const integer_cst>(GET_CONST_NODE(operation->op1));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, second argument is constant -> " + STR(const2->value));
               THROW_ASSERT(const2->value >= 0, "unexpected condition");

               if(op0_bitstring.size() <= static_cast<size_t>(const2->value))
               {
                  if(tree_helper::is_int(TM, op0_nid))
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
                  size_t new_lenght = op0_bitstring.size() - static_cast<size_t>(const2->value);
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
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " extract bit " + STR(op1_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(op0_bitstring) + ">>" + STR(const2->value) + " => " + bitstring_to_string(res));
            };
            ebe0();
         }
         else
         {
            THROW_UNREACHABLE("Unhadled binary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) + ")");
         }
         break;
      }
      case bit_ior_concat_expr_K:
      case cond_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      {
         const auto operation = GetPointer<const ternary_expr>(GET_CONST_NODE(rhs));

         const auto op0_nid = GET_INDEX_NODE(operation->op0);
         auto op0_bitstring = get_current(operation->op0);

         const auto op1_nid = GET_INDEX_NODE(operation->op1);
         auto op1_bitstring = get_current(operation->op1);

         const auto op2_nid = GET_INDEX_NODE(operation->op2);
         auto op2_bitstring = get_current(operation->op2);

         if(rhs_kind == bit_ior_concat_expr_K)
         {
            auto bice0 = [&] {
               long long int offset = GetPointerS<integer_cst>(GET_NODE(operation->op2))->value;

               if(op0_bitstring.size() > op1_bitstring.size())
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
               }
               if(op1_bitstring.size() > op0_bitstring.size())
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + " |concat " + STR(op1_nid) + "(" + STR(offset) + ")");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " |concat");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + " =");

               auto op0_it = op0_bitstring.rbegin();
               auto op1_it = op1_bitstring.rbegin();
               long long index = 0;

               for(; op0_it != op0_bitstring.rend() && op1_it != op1_bitstring.rend(); ++op0_it, ++op1_it, ++index)
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
            };
            bice0();
         }
         else if(rhs_kind == cond_expr_K)
         {
            auto ce0 = [&] {
               auto arg_cond = bit_lattice::ZERO;
               for(auto current_bit : op0_bitstring)
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
                     op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), inf_size);
                  }
                  if(op2_bitstring.size() < inf_size)
                  {
                     op2_bitstring = sign_extend_bitstring(op2_bitstring, tree_helper::is_int(TM, op2_nid), inf_size);
                  }
                  res = inf(op1_bitstring, op2_bitstring, lhs_nid);
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + " ? " + bitstring_to_string(op1_bitstring) + " : " + bitstring_to_string(op2_bitstring));
            };
            ce0();
         }
         else if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K || rhs_kind == ternary_mp_expr_K || rhs_kind == ternary_mm_expr_K)
         {
            auto ternary_adders = [&] {
               size_t arg_size_max = std::max({op0_bitstring.size(), op1_bitstring.size(), op2_bitstring.size()});
               if(op0_bitstring.size() < arg_size_max)
               {
                  op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), arg_size_max);
               }
               if(op1_bitstring.size() < arg_size_max)
               {
                  op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), arg_size_max);
               }
               if(op2_bitstring.size() < arg_size_max)
               {
                  op2_bitstring = sign_extend_bitstring(op2_bitstring, tree_helper::is_int(TM, op2_nid), arg_size_max);
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "forward_transfer, operation: " + STR(lhs_nid) + " = " + STR(op0_nid) + (rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K ? " + " : " - ") + STR(op1_nid) +
                                  (rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_mp_expr_K ? " +" : " - ") + STR(op2_nid));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op0_bitstring) + (rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K ? " + " : " - "));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op1_bitstring) + (rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_mp_expr_K ? " +" : " - "));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op2_bitstring) + " =");

               auto op0_it = op0_bitstring.crbegin();
               auto op1_it = op1_bitstring.crbegin();
               const auto op0_end = op0_bitstring.crend();
               const auto op1_end = op1_bitstring.crend();
               bit_lattice carry1 = bit_lattice::ZERO;
               std::deque<bit_lattice> res_int;

               for(unsigned bit_index = 0; bit_index < lhs_size and op0_it != op0_end and op1_it != op1_end; op0_it++, op1_it++, bit_index++)
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

               if(tree_helper::is_int(TM, lhs_nid) and res_int.size() < lhs_size)
               {
                  if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K)
                  {
                     res_int.push_front(plus_expr_map.at(op0_bitstring.front()).at(op1_bitstring.front()).at(carry1).back());
                  }
                  else
                  {
                     res_int.push_front(minus_expr_map.at(op0_bitstring.front()).at(op1_bitstring.front()).at(carry1).back());
                  }
               }
               else if(not tree_helper::is_int(TM, lhs_nid))
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
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(res_int) + ":res_int:");

               if(res_int.size() > op2_bitstring.size())
               {
                  op2_bitstring = sign_extend_bitstring(op2_bitstring, tree_helper::is_int(TM, op2_nid), res_int.size());
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(op2_bitstring) + ":op2_bitstring:");
               auto op2_it = op2_bitstring.crbegin();
               auto res_int_it = res_int.crbegin();
               const auto op2_end = op2_bitstring.crend();
               const auto res_int_end = res_int.crend();
               carry1 = bit_lattice::ZERO;
               for(unsigned bit_index = 0; bit_index < lhs_size and op2_it != op2_end and res_int_it != res_int_end; op2_it++, res_int_it++, bit_index++)
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

               if(tree_helper::is_int(TM, lhs_nid) and res.size() < lhs_size)
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
               else if(not tree_helper::is_int(TM, lhs_nid))
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
            };
            ternary_adders();
         }
         else
         {
            THROW_UNREACHABLE("Unhadled ternary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) + ")");
         }
         break;
      }
      // Unary expressions
      case imagpart_expr_K:
      case realpart_expr_K:
      // Binary expressions
      case mem_ref_K:
      // Ternary expressions
      case bit_field_ref_K:
      case component_ref_K:
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
            const auto tn = TM->CGetTreeNode(called_id);
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
         res = create_u_bitstring(1);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation lut_expr: " + STR(lhs_nid) + " = LUT VALUE >> ins");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(res));
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
      case ordered_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case set_le_expr_K:
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
      case sat_plus_expr_K:
      case sat_minus_expr_K:
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
   return res;
}

void Bit_Value::forward()
{
   const auto tn = TM->CGetTreeNode(function_id);
   THROW_ASSERT(tn->get_kind() == function_decl_K, "Node is not a function");
   const auto fd = GetPointerS<const function_decl>(tn);
   std::deque<tree_nodeConstRef> working_list, return_list;
   CustomUnorderedSet<unsigned int> working_list_idx;
   auto push_back = [&](const tree_nodeConstRef& stmt) {
      if(!working_list_idx.count(stmt->index) && (stmt->get_kind() == gimple_assign_K || stmt->get_kind() == gimple_phi_K))
      {
         working_list.push_back(stmt);
         working_list_idx.insert(stmt->index);
      }
   };
   auto pop_front = [&]() -> tree_nodeConstRef {
      const tree_nodeConstRef stmt = working_list.front();
      working_list.pop_front();
      working_list_idx.erase(stmt->index);
      return stmt;
   };
   for(const auto& bb : bb_topological)
   {
      for(const auto& phi : bb->CGetPhiList())
      {
         const auto phi_node = GET_CONST_NODE(phi);
         const auto pn = GetPointerS<const gimple_phi>(phi_node);
         if(!pn->virtual_flag)
         {
            const auto output_uid = GET_INDEX_CONST_NODE(pn->res);
            if(is_handled_by_bitvalue(output_uid))
            {
               push_back(phi_node);
            }
         }
      }
      for(const auto& stmt : bb->CGetStmtList())
      {
         const auto stmt_node = GET_CONST_NODE(stmt);
         if(stmt_node->get_kind() == gimple_assign_K)
         {
            const auto ga = GetPointerS<const gimple_assign>(stmt_node);
            const auto output_uid = GET_INDEX_CONST_NODE(ga->op0);
            if(is_handled_by_bitvalue(output_uid))
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
   while(!working_list.empty())
   {
      const auto stmt_node = pop_front();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + STR(stmt_node));
      if(stmt_node->get_kind() == gimple_assign_K)
      {
         const auto ga = GetPointerS<const gimple_assign>(stmt_node);
         const auto output_uid = GET_INDEX_CONST_NODE(ga->op0);
         const auto ssa = GetPointer<const ssa_name>(GET_CONST_NODE(ga->op0));

         if(ssa)
         {
            if(!is_handled_by_bitvalue(output_uid) || ssa->CGetUseStmts().empty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(GET_CONST_NODE(ga->op0))) + " not considered id: " + STR(output_uid));
               continue;
            }
            bool hasRequiredValues = true;
            std::vector<std::tuple<unsigned int, unsigned int>> vars_read;
            tree_helper::get_required_values(TM, vars_read, TM->GetTreeNode(stmt_node->index), stmt_node->index);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Requires " + STR(vars_read.size()) + " values");
            for(const auto& var_pair : vars_read)
            {
               const auto use_node_id = std::get<0>(var_pair);
               if(use_node_id == 0)
               {
                  continue;
               }
               if(!is_handled_by_bitvalue(use_node_id))
               {
                  continue;
               }
               const auto use_node = TM->CGetTreeNode(use_node_id);
               const auto ssa_use = GetPointer<const ssa_name>(use_node);

               if(ssa_use && current.find(use_node_id) == current.end())
               {
                  const auto def_stmt = ssa_use->CGetDefStmt();
                  if(GET_CONST_NODE(def_stmt)->get_kind() == gimple_nop_K)
                  {
                     THROW_ASSERT(!ssa_use->var || GET_CONST_NODE(ssa_use->var)->get_kind() != parm_decl_K, "Function parameter bitvalue must be defined before");
                     current.insert(std::make_pair(use_node_id, create_bitstring_from_constant(0, BitLatticeManipulator::Size(use_node), tree_helper::is_int(TM, use_node_id))));
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Input " + ssa_use->ToString() + " definition has not been analyzed yet");
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition statement added to queue: " + GET_CONST_NODE(def_stmt)->ToString());
                     push_back(GET_CONST_NODE(def_stmt));
                     hasRequiredValues = false;
                     continue;
                  }
               }
            }
            if(!hasRequiredValues)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--inputs are not all fully analyzed by the forward Bit Value Analysis. Operation  " + GET_NODE(ga->op0)->ToString() + " postponed");
               push_back(stmt_node);
               continue;
            }

            auto res = forward_transfer(ga);
            if(res.empty())
            {
               THROW_ASSERT(best.find(output_uid) != best.end(), "unexpected condition");
               res = best.at(output_uid);
            }
            THROW_ASSERT(res.size(), "");
            auto& output_current = current[output_uid];
            if(output_current != res)
            {
               output_current = res;
               for(const auto& next_node : ssa->CGetUseStmts())
               {
                  push_back(GET_CONST_NODE(next_node.first));
               }
            }
         }
      }
      else
      {
         const auto pn = GetPointerS<const gimple_phi>(stmt_node);
         THROW_ASSERT(!pn->virtual_flag, "unexpected case");

         const auto output_uid = GET_INDEX_CONST_NODE(pn->res);
         const auto ssa = GetPointerS<const ssa_name>(GET_CONST_NODE(pn->res));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "phi: " + STR(stmt_node->index));
         if(!is_handled_by_bitvalue(output_uid) || ssa->CGetUseStmts().empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(GET_CONST_NODE(pn->res))) + " not considered id: " + STR(output_uid));
            continue;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res id: " + STR(output_uid));
         auto res = create_x_bitstring(1);
         bool atLeastOne = false;
         bool allInputs = true;
         for(const auto& def_edge : pn->CGetDefEdgesList())
         {
            const auto def_id = GET_INDEX_CONST_NODE(def_edge.first);
            if(def_id == output_uid)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping " + STR(def_edge.first) + " coming from BB" + STR(def_edge.second) + " because of ssa cycle");
               continue;
            }
            if(current.find(def_id) == current.end())
            {
               const auto def_node = GET_CONST_NODE(def_edge.first);
               const auto def_ssa = GetPointer<const ssa_name>(def_node);
               if(def_ssa)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping " + STR(def_node) + " no current has been yet computed for this ssa var");
                  const auto def_stmt = def_ssa->CGetDefStmt();
                  if(GET_CONST_NODE(def_stmt)->get_kind() == gimple_nop_K)
                  {
                     THROW_ASSERT(!def_ssa->var || GET_CONST_NODE(def_ssa->var)->get_kind() != parm_decl_K, "Function parameter bitvalue must be defined before");
                     current.insert(std::make_pair(def_id, create_bitstring_from_constant(0, BitLatticeManipulator::Size(def_node), tree_helper::is_int(TM, def_id))));
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition statement added to queue: " + GET_CONST_NODE(def_stmt)->ToString());
                     push_back(GET_CONST_NODE(def_stmt));
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
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Edge " + STR(def_edge.second) + ": " + bitstring_to_string(current.at(def_id)));

#if HAVE_ASSERTS
            const auto is_signed1 = tree_helper::is_int(TM, output_uid);
            const auto is_signed2 = tree_helper::is_int(TM, def_id);
#endif
            THROW_ASSERT(is_signed2 == is_signed1, STR(stmt_node));
            res = inf(res, current.at(def_id), output_uid);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---current res: " + bitstring_to_string(res));
         }
         if(atLeastOne)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---res: " + bitstring_to_string(res));
            auto& output_current = current[output_uid];
            THROW_ASSERT(res.size(), "");
            if(output_current != res)
            {
               output_current = res;
               for(const auto& next_node : ssa->CGetUseStmts())
               {
                  push_back(GET_NODE(next_node.first));
               }
            }
         }
         if(!allInputs)
         {
            push_back(stmt_node);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + STR(stmt_node));
   }
   /// returned value management
   auto fu_type = GET_NODE(fd->type);
   THROW_ASSERT(fu_type->get_kind() == function_type_K || fu_type->get_kind() == method_type_K, "node " + STR(function_id) + " is " + fu_type->get_kind_text());
   tree_nodeRef fret_type_node;
   unsigned int ret_type_id;
   if(fu_type->get_kind() == function_type_K)
   {
      const auto* ft = GetPointer<const function_type>(fu_type);
      fret_type_node = GET_NODE(ft->retn);
      ret_type_id = GET_INDEX_NODE(ft->retn);
   }
   else
   {
      const auto* mt = GetPointer<const method_type>(fu_type);
      fret_type_node = GET_NODE(mt->retn);
      ret_type_id = GET_INDEX_NODE(mt->retn);
   }
   if(tree_helper::is_scalar(TM, ret_type_id))
   {
      current.insert(std::make_pair(function_id, best.at(function_id)));
      auto res = create_x_bitstring(1);
      bool undefined_behavior = false;
      for(const auto& stmt_node : return_list)
      {
         const auto gr = GetPointerS<const gimple_return>(stmt_node);
         if(!gr->op)
         {
            /// do nothing
            /// undefined behavior
            undefined_behavior = true;
         }
         else
         {
            if(current.find(GET_INDEX_NODE(gr->op)) == current.end())
            {
               if(best.find(GET_INDEX_NODE(gr->op)) == best.end())
               {
                  current[GET_INDEX_NODE(gr->op)] = fd->range ? fd->range->getBitValues(tree_helper::is_int(TM, ret_type_id)) : create_u_bitstring(BitLatticeManipulator::Size(fret_type_node));
               }
               else
               {
                  current[GET_INDEX_NODE(gr->op)] = best.at(GET_INDEX_NODE(gr->op));
               }
            }
            res = inf(res, current.at(GET_INDEX_NODE(gr->op)), function_id);
         }
      }
      if(undefined_behavior)
      {
         res = create_x_bitstring(1);
      }
      update_current(res, function_id);
   }
}
