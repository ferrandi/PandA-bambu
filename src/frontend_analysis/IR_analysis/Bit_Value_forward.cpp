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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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

#include "Parameter.hpp"

// include from tree/
#include "application_manager.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "string_manipulation.hpp"
#include <boost/range/adaptors.hpp>

bool Bit_Value::manage_forward_binary_operands(const binary_expr* operation, unsigned int& arg1_uid, unsigned int& arg2_uid, std::deque<bit_lattice>& arg1_bitstring, std::deque<bit_lattice>& arg2_bitstring) const
{
   if(GET_NODE(operation->op0)->get_kind() == ssa_name_K)
   {
      arg1_uid = GET_INDEX_NODE(operation->op0);

      if(current.find(arg1_uid) == current.end())
      {
         const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op0));
         if(not is_handled_by_bitvalue(op1_type->index))
            return false;
         else
            arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op0)));
      }
      else
         arg1_bitstring = current.at(arg1_uid);
   }
   else if(GET_NODE(operation->op0)->get_kind() == integer_cst_K)
   {
      arg1_uid = GET_INDEX_NODE(operation->op0);
      THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
      arg1_bitstring = best.at(arg1_uid);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op0))->value) + " : " + bitstring_to_string(arg1_bitstring));
   }
   else if(GET_NODE(operation->op0)->get_kind() == real_cst_K)
   {
      arg1_uid = GET_INDEX_NODE(operation->op0);
      THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
      arg1_bitstring = best.at(arg1_uid);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<real_cst>(GET_NODE(operation->op0))->valr) + " : " + bitstring_to_string(arg1_bitstring));
   }
   else
      return false;

   if(GET_NODE(operation->op1)->get_kind() == ssa_name_K)
   {
      arg2_uid = GET_INDEX_NODE(operation->op1);
      if(current.find(arg2_uid) == current.end())
      {
         const tree_nodeConstRef op2_type = tree_helper::CGetType(GET_NODE(operation->op1));
         if(not is_handled_by_bitvalue(op2_type->index))
            return false;
         else
            arg2_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op1)));
      }
      else
         arg2_bitstring = current.at(arg2_uid);
   }
   else if(GET_NODE(operation->op1)->get_kind() == integer_cst_K)
   {
      arg2_uid = GET_INDEX_NODE(operation->op1);
      THROW_ASSERT(best.find(arg2_uid) != best.end(), "unexpected condition");
      arg2_bitstring = best.at(arg2_uid);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op1))->value) + " : " + bitstring_to_string(arg2_bitstring));
   }
   else if(GET_NODE(operation->op1)->get_kind() == real_cst_K)
   {
      arg2_uid = GET_INDEX_NODE(operation->op1);
      THROW_ASSERT(best.find(arg2_uid) != best.end(), "unexpected condition");
      arg2_bitstring = best.at(arg2_uid);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<real_cst>(GET_NODE(operation->op1))->valr) + " : " + bitstring_to_string(arg2_bitstring));
   }
   else
      return false;
   return true;
}

std::deque<bit_lattice> Bit_Value::forward_transfer(const gimple_assign* ga) const
{
   std::deque<bit_lattice> res;
   const unsigned int output_uid = GET_INDEX_NODE(ga->op0);
   enum kind op_kind = GET_NODE(ga->op1)->get_kind();
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, GET_NODE(ga->op1)->get_kind_text());

   if(op_kind == ssa_name_K)
   {
      unsigned int arg2_uid;
      arg2_uid = GET_INDEX_NODE(ga->op1);
      if(current.find(arg2_uid) == current.end())
      {
         const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(ga->op1));
         if(not is_handled_by_bitvalue(op1_type->index))
            return res;
         else
            res = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->op1)));
      }
      else
      {
         res = current.at(arg2_uid);
      }
   }
   else if(op_kind == integer_cst_K or op_kind == real_cst_K)
   {
      unsigned int arg2_uid;
      arg2_uid = GET_INDEX_NODE(ga->op1);
      THROW_ASSERT(best.find(arg2_uid) != best.end(), "unexpected condition");
      res = best.at(arg2_uid);
   }
#if 1
   else if(op_kind == plus_expr_K || op_kind == pointer_plus_expr_K)
   {
      auto* operation = GetPointer<binary_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      if(arg1_bitstring.size() > arg2_bitstring.size())
      {
         arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
      }
      if(arg2_bitstring.size() > arg1_bitstring.size())
      {
         arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " + " + STR(arg2_uid));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " +");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

      std::deque<bit_lattice>::const_reverse_iterator arg1_it = arg1_bitstring.rbegin();
      std::deque<bit_lattice>::const_reverse_iterator arg2_it = arg2_bitstring.rbegin();
      bit_lattice carry1 = bit_lattice::ZERO;
      unsigned int max_bitsize = BitLatticeManipulator::Size(GET_NODE(ga->op0));

      for(unsigned bit_index = 0; bit_index < max_bitsize && arg1_it != arg1_bitstring.rend() && arg2_it != arg2_bitstring.rend(); ++arg1_it, ++arg2_it, ++bit_index)
      {
         // INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "arg1: "+STR(*arg1_it) + "arg2: "STR(*arg2_it) + "carry: " + STR(carry1));
         res.push_front(plus_expr_map.at(*arg1_it).at(*arg2_it).at(carry1).back());
         carry1 = plus_expr_map.at(*arg1_it).at(*arg2_it).at(carry1).front();
      }

      if(tree_helper::is_int(TM, output_uid) && res.size() < max_bitsize)
         res.push_front(plus_expr_map.at(arg1_bitstring.front()).at(arg2_bitstring.front()).at(carry1).back());
      else if(!tree_helper::is_int(TM, output_uid))
      {
         while(res.size() < max_bitsize)
         {
            res.push_front(plus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).back());
            carry1 = plus_expr_map.at(bit_lattice::ZERO).at(bit_lattice::ZERO).at(carry1).front();
         }
      }
   }
#endif
#if 1
   else if(op_kind == ternary_plus_expr_K || op_kind == ternary_pm_expr_K || op_kind == ternary_mp_expr_K || op_kind == ternary_mm_expr_K)
   {
      auto* operation = GetPointer<ternary_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      std::deque<bit_lattice> arg3_bitstring;
      unsigned int arg1_uid = 0;
      unsigned int arg2_uid = 0;
      unsigned int arg3_uid = 0;

      if(GET_NODE(operation->op0)->get_kind() == ssa_name_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         if(current.find(arg1_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op0));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op0)));
         }
         else
            arg1_bitstring = current.at(arg1_uid);
      }
      else if(GET_NODE(operation->op0)->get_kind() == integer_cst_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op0))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;

      if(GET_NODE(operation->op1)->get_kind() == ssa_name_K)
      {
         arg2_uid = GET_INDEX_NODE(operation->op1);
         if(current.find(arg2_uid) == current.end())
         {
            const tree_nodeConstRef op2_type = tree_helper::CGetType(GET_NODE(operation->op1));
            if(not is_handled_by_bitvalue(op2_type->index))
               return res;
            else
               arg2_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op1)));
         }
         else
            arg2_bitstring = current.at(arg2_uid);
      }
      else if(GET_NODE(operation->op1)->get_kind() == integer_cst_K)
      {
         arg2_uid = GET_INDEX_NODE(operation->op1);
         THROW_ASSERT(best.find(arg2_uid) != best.end(), "unexpected condition");
         arg2_bitstring = best.at(arg2_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op1))->value) + " : " + bitstring_to_string(arg2_bitstring));
      }
      else
         return res;

      if(GET_NODE(operation->op2)->get_kind() == ssa_name_K)
      {
         arg3_uid = GET_INDEX_NODE(operation->op2);
         if(current.find(arg3_uid) == current.end())
         {
            const tree_nodeConstRef op3_type = tree_helper::CGetType(GET_NODE(operation->op2));
            if(not is_handled_by_bitvalue(op3_type->index))
               return res;
            else
               arg3_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op2)));
         }
         else
            arg3_bitstring = current.at(arg3_uid);
      }
      else if(GET_NODE(operation->op2)->get_kind() == integer_cst_K)
      {
         arg3_uid = GET_INDEX_NODE(operation->op2);
         THROW_ASSERT(best.find(arg3_uid) != best.end(), "unexpected condition");
         arg3_bitstring = best.at(arg3_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op2))->value) + " : " + bitstring_to_string(arg3_bitstring));
      }
      else
         return res;

      auto ternary_adders = [&] {
         size_t arg_size_max = std::max({arg1_bitstring.size(), arg2_bitstring.size(), arg3_bitstring.size()});
         if(arg1_bitstring.size() < arg_size_max)
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg_size_max);
         if(arg2_bitstring.size() < arg_size_max)
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg_size_max);
         if(arg3_bitstring.size() < arg_size_max)
            arg3_bitstring = sign_extend_bitstring(arg3_bitstring, tree_helper::is_int(TM, arg3_uid), arg_size_max);

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + (op_kind == ternary_plus_expr_K || op_kind == ternary_pm_expr_K ? " + " : " - ") + STR(arg2_uid) +
                            (op_kind == ternary_plus_expr_K || op_kind == ternary_mp_expr_K ? " +" : " - ") + STR(arg3_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + (op_kind == ternary_plus_expr_K || op_kind == ternary_pm_expr_K ? " + " : " - "));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + (op_kind == ternary_plus_expr_K || op_kind == ternary_mp_expr_K ? " +" : " - "));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg3_bitstring) + " =");

         auto arg1_it = arg1_bitstring.crbegin();
         auto arg2_it = arg2_bitstring.crbegin();
         const auto arg1_end = arg1_bitstring.crend();
         const auto arg2_end = arg2_bitstring.crend();
         bit_lattice carry1 = bit_lattice::ZERO;
         std::deque<bit_lattice> res_int;
         unsigned int max_bitsize = BitLatticeManipulator::Size(GET_NODE(ga->op0));

         for(unsigned bit_index = 0; bit_index < max_bitsize and arg1_it != arg1_end and arg2_it != arg2_end; arg1_it++, arg2_it++, bit_index++)
         {
            if(op_kind == ternary_plus_expr_K or op_kind == ternary_pm_expr_K)
            {
               res_int.push_front(plus_expr_map.at(*arg1_it).at(*arg2_it).at(carry1).back());
               carry1 = plus_expr_map.at(*arg1_it).at(*arg2_it).at(carry1).front();
            }
            else
            {
               res_int.push_front(minus_expr_map.at(*arg1_it).at(*arg2_it).at(carry1).back());
               carry1 = minus_expr_map.at(*arg1_it).at(*arg2_it).at(carry1).front();
            }
         }

         if(tree_helper::is_int(TM, output_uid) and res_int.size() < max_bitsize)
         {
            if(op_kind == ternary_plus_expr_K or op_kind == ternary_pm_expr_K)
               res_int.push_front(plus_expr_map.at(arg1_bitstring.front()).at(arg2_bitstring.front()).at(carry1).back());
            else
               res_int.push_front(minus_expr_map.at(arg1_bitstring.front()).at(arg2_bitstring.front()).at(carry1).back());
         }
         else if(not tree_helper::is_int(TM, output_uid))
         {
            while(res_int.size() < max_bitsize)
            {
               if(op_kind == ternary_plus_expr_K or op_kind == ternary_pm_expr_K)
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

         if(res_int.size() > arg3_bitstring.size())
         {
            arg3_bitstring = sign_extend_bitstring(arg3_bitstring, tree_helper::is_int(TM, arg3_uid), res_int.size());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg3_bitstring) + ":arg3_bitstring:");
         auto arg3_it = arg3_bitstring.crbegin();
         auto res_int_it = res_int.crbegin();
         const auto arg3_end = arg3_bitstring.crend();
         const auto res_int_end = res_int.crend();
         carry1 = bit_lattice::ZERO;
         for(unsigned bit_index = 0; bit_index < max_bitsize and arg3_it != arg3_end and res_int_it != res_int_end; arg3_it++, res_int_it++, bit_index++)
         {
            if(op_kind == ternary_plus_expr_K or op_kind == ternary_mp_expr_K)
            {
               res.push_front(plus_expr_map.at(*res_int_it).at(*arg3_it).at(carry1).back());
               carry1 = plus_expr_map.at(*res_int_it).at(*arg3_it).at(carry1).front();
            }
            else
            {
               res.push_front(minus_expr_map.at(*res_int_it).at(*arg3_it).at(carry1).back());
               carry1 = minus_expr_map.at(*res_int_it).at(*arg3_it).at(carry1).front();
            }
         }

         if(tree_helper::is_int(TM, output_uid) and res.size() < max_bitsize)
         {
            if(op_kind == ternary_plus_expr_K or op_kind == ternary_mp_expr_K)
               res.push_front(plus_expr_map.at(res_int.front()).at(arg3_bitstring.front()).at(carry1).back());
            else
               res.push_front(minus_expr_map.at(res_int.front()).at(arg3_bitstring.front()).at(carry1).back());
         }
         else if(not tree_helper::is_int(TM, output_uid))
         {
            while(res.size() < max_bitsize)
            {
               if(op_kind == ternary_plus_expr_K or op_kind == ternary_mp_expr_K)
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
#endif
#if 1
   else if(op_kind == mult_expr_K || op_kind == widen_mult_expr_K)
   {
      auto* operation = GetPointer<binary_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      //    auto mult0 = [&] {
      //       if(arg1_bitstring.size() > arg2_bitstring.size())
      //       {
      //          arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
      //       }
      //       if(arg2_bitstring.size() > arg1_bitstring.size())
      //       {
      //          arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
      //       }
      //
      //       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " * " + STR(arg2_uid));
      //       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " *");
      //       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");
      //
      //       std::deque<bit_lattice>::const_reverse_iterator arg1_it = arg1_bitstring.rbegin();
      //       std::deque<bit_lattice>::const_reverse_iterator arg2_it = arg2_bitstring.rbegin();
      //       std::deque<bit_lattice>::const_iterator arg1_it_fw = arg1_bitstring.begin();
      //       std::deque<bit_lattice>::const_iterator arg2_it_fw = arg2_bitstring.begin();
      //       // trailing zeros of a
      //       unsigned int ta = 0;
      //       // trailing zeros of b
      //       unsigned int tb = 0;
      //       // leading zeros of a
      //       unsigned int la = 0;
      //       // leading zeros of b
      //       unsigned int lb = 0;
      //       for(; arg1_it != arg1_bitstring.rend() && *arg1_it == bit_lattice::ZERO; ++arg1_it, ++ta)
      //          ;
      //
      //       for(; arg2_it != arg2_bitstring.rend() && *arg2_it == bit_lattice::ZERO; ++arg2_it, ++tb)
      //          ;
      //
      //       for(; arg1_it_fw != arg1_bitstring.end() && *arg1_it_fw == bit_lattice::ZERO; ++arg1_it_fw, ++la)
      //          ;
      //
      //       for(; arg2_it_fw != arg2_bitstring.end() && *arg2_it_fw == bit_lattice::ZERO; ++arg2_it_fw, ++lb)
      //          ;
      //
      //       // if one of the two arguments is all zeros returns zero
      //       if(la == arg1_bitstring.size() || lb == arg2_bitstring.size())
      //       {
      //          res.push_back(bit_lattice::ZERO);
      //       }
      //       else
      //       {
      //          size_t lenght_arg1 = arg1_bitstring.size();
      //          size_t lenght_arg2 = arg2_bitstring.size();
      //          unsigned res_bitsize = BitLatticeManipulator::Size(GET_NODE(ga->op0));
      //          THROW_ASSERT(static_cast<int>(lenght_arg1 + lenght_arg2 - ta - tb - la - lb) > 0, "unexpected condition");
      //          res = create_u_bitstring(static_cast<unsigned int>(lenght_arg1 + lenght_arg2 - ta - tb - la - lb));
      //          for(unsigned int i = 0; i < ta + tb; i++)
      //             res.push_back(bit_lattice::ZERO);
      //          if(tree_helper::is_int(TM, output_uid) && la > 0 && lb > 0)
      //             res.push_front(bit_lattice::ZERO);
      //          while(res.size() > res_bitsize)
      //             res.pop_front();
      //       }
      //    };
      //    if(0)
      //       mult0();
      auto mult1 = [&] {
         size_t lenght_arg1 = arg1_bitstring.size();
         size_t lenght_arg2 = arg2_bitstring.size();
         auto ga0_bitsize = static_cast<size_t>(BitLatticeManipulator::Size(GET_NODE(ga->op0)));
         auto res_bitsize = std::min(lenght_arg1 + lenght_arg2, ga0_bitsize);
         if(res_bitsize > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), res_bitsize);
         }
         if(res_bitsize > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), res_bitsize);
         }
         while(arg1_bitstring.size() > res_bitsize)
            arg1_bitstring.pop_front();
         while(arg2_bitstring.size() > res_bitsize)
            arg2_bitstring.pop_front();
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " * " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " *");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

         while(res.size() < res_bitsize)
            res.push_front(bit_lattice::ZERO);
         auto arg2_it = arg2_bitstring.crbegin();
         for(size_t pos = 0; arg2_it != arg2_bitstring.crend() && pos < res_bitsize; ++arg2_it, ++pos)
         {
            std::deque<bit_lattice> temp_op1;
            while(temp_op1.size() < pos)
               temp_op1.push_front(bit_lattice::ZERO);
            auto arg1_it = arg1_bitstring.crbegin();
            for(size_t idx = 0; (idx + pos) < res_bitsize; ++idx, ++arg1_it)
               temp_op1.push_front(bit_and_expr_map.at(*arg1_it).at(*arg2_it));
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
      if(1)
         mult1();
   }
#endif
#if 1
   else if(op_kind == trunc_div_expr_K || op_kind == exact_div_expr_K)
   {
      auto* operation = GetPointer<binary_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;
      auto tde0 = [&] {
         unsigned res_bitsize = BitLatticeManipulator::Size(GET_NODE(ga->op0));

         if(tree_helper::is_int(TM, arg1_uid))
            res = create_u_bitstring(1u + static_cast<unsigned int>(arg1_bitstring.size()));
         else
            res = create_u_bitstring(static_cast<unsigned int>(arg1_bitstring.size()));
         while(res.size() > res_bitsize)
            res.pop_front();
      };
      tde0();
   }
   else if(op_kind == trunc_mod_expr_K)
   {
      auto* operation = GetPointer<binary_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;
      auto tme0 = [&] {
         unsigned res_bitsize = BitLatticeManipulator::Size(GET_NODE(ga->op0));

         if(tree_helper::is_int(TM, arg1_uid))
            res = create_u_bitstring(1 + static_cast<unsigned int>(std::min(arg1_bitstring.size(), arg2_bitstring.size())));
         else
            res = create_u_bitstring(static_cast<unsigned int>(std::min(arg1_bitstring.size(), arg2_bitstring.size())));
         while(res.size() > res_bitsize)
            res.pop_front();
      };
      tme0();
   }
#endif
#if 1
   else if(op_kind == gt_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid), "");
      auto* operation = GetPointer<gt_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto gt0 = [&] {
         bool is_signed_var = tree_helper::is_int(TM, arg1_uid) || tree_helper::is_int(TM, arg2_uid);

         if(arg1_bitstring.size() > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
         }
         if(arg2_bitstring.size() > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " > " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " >");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " ?");

         std::deque<bit_lattice>::const_iterator arg1_bitstring_it = arg1_bitstring.begin();
         std::deque<bit_lattice>::const_iterator arg2_bitstring_it = arg2_bitstring.begin();
         bool computed_result = false;
         for(; arg1_bitstring_it != arg1_bitstring.end() && arg2_bitstring_it != arg2_bitstring.end(); ++arg1_bitstring_it, ++arg2_bitstring_it)
         {
            if(*arg1_bitstring_it == bit_lattice::U || *arg2_bitstring_it == bit_lattice::U)
            {
               // <U> is UNKNOWN
               res.push_front(bit_lattice::U);
               computed_result = true;
               break;
            }
            else if(*arg1_bitstring_it == bit_lattice::ONE && *arg2_bitstring_it == bit_lattice::ZERO)
            {
               if(arg1_bitstring_it == arg1_bitstring.begin() && is_signed_var)
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
            else if(*arg1_bitstring_it == bit_lattice::ZERO && *arg2_bitstring_it == bit_lattice::ONE)
            {
               if(arg1_bitstring_it == arg1_bitstring.begin() && is_signed_var)
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
   else if(op_kind == ge_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid), "");
      auto* operation = GetPointer<ge_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto ge0 = [&] {
         if(arg1_bitstring.size() > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
         }
         if(arg2_bitstring.size() > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " >= " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " >=");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " ?");

         bool is_signed_var = tree_helper::is_int(TM, arg1_uid) || tree_helper::is_int(TM, arg2_uid);
         std::deque<bit_lattice>::const_iterator arg1_bitstring_it = arg1_bitstring.begin();
         std::deque<bit_lattice>::const_iterator arg2_bitstring_it = arg2_bitstring.begin();
         bool computed_result = false;
         for(; arg1_bitstring_it != arg1_bitstring.end() && arg2_bitstring_it != arg2_bitstring.end(); ++arg1_bitstring_it, ++arg2_bitstring_it)
         {
            if(*arg1_bitstring_it == bit_lattice::U || *arg2_bitstring_it == bit_lattice::U)
            {
               // <U> is UNKNOWN
               res.push_front(bit_lattice::U);
               computed_result = true;
               break;
            }
            else if(*arg1_bitstring_it == bit_lattice::ONE && *arg2_bitstring_it == bit_lattice::ZERO)
            {
               if(arg1_bitstring_it == arg1_bitstring.begin() && is_signed_var)
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
            else if(*arg1_bitstring_it == bit_lattice::ZERO && *arg2_bitstring_it == bit_lattice::ONE)
            {
               if(arg1_bitstring_it == arg1_bitstring.begin() && is_signed_var)
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
   else if(op_kind == lt_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid), "");
      auto* operation = GetPointer<lt_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto lt0 = [&] {
         if(arg1_bitstring.size() > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
         }
         if(arg2_bitstring.size() > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " < " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " <");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " ?");

         bool is_signed_var = tree_helper::is_int(TM, arg1_uid) || tree_helper::is_int(TM, arg2_uid);
         std::deque<bit_lattice>::const_iterator arg1_bitstring_it = arg1_bitstring.begin();
         std::deque<bit_lattice>::const_iterator arg2_bitstring_it = arg2_bitstring.begin();
         bool computed_result = false;
         for(; arg1_bitstring_it != arg1_bitstring.end() && arg2_bitstring_it != arg2_bitstring.end(); ++arg1_bitstring_it, ++arg2_bitstring_it)
         {
            if(*arg1_bitstring_it == bit_lattice::U || *arg2_bitstring_it == bit_lattice::U)
            {
               // <U> is UNKNOWN
               res.push_front(bit_lattice::U);
               computed_result = true;
               break;
            }
            else if(*arg1_bitstring_it == bit_lattice::ZERO && *arg2_bitstring_it == bit_lattice::ONE)
            {
               if(arg1_bitstring_it == arg1_bitstring.begin() && is_signed_var)
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
            else if(*arg1_bitstring_it == bit_lattice::ONE && *arg2_bitstring_it == bit_lattice::ZERO)
            {
               if(arg1_bitstring_it == arg1_bitstring.begin() && is_signed_var)
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
   else if(op_kind == le_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid), "");
      auto* operation = GetPointer<le_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto le0 = [&] {
         if(arg1_bitstring.size() > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
         }
         if(arg2_bitstring.size() > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " <= " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " <=");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " ?");

         bool is_signed_var = tree_helper::is_int(TM, arg1_uid) || tree_helper::is_int(TM, arg2_uid);
         std::deque<bit_lattice>::const_iterator arg1_bitstring_it = arg1_bitstring.begin();
         std::deque<bit_lattice>::const_iterator arg2_bitstring_it = arg2_bitstring.begin();
         bool computed_result = false;
         for(; arg1_bitstring_it != arg1_bitstring.end() && arg2_bitstring_it != arg2_bitstring.end(); ++arg1_bitstring_it, ++arg2_bitstring_it)
         {
            if(*arg1_bitstring_it == bit_lattice::U || *arg2_bitstring_it == bit_lattice::U)
            {
               // <U> is UNKNOWN
               res.push_front(bit_lattice::U);
               computed_result = true;
               break;
            }
            else if(*arg1_bitstring_it == bit_lattice::ZERO && *arg2_bitstring_it == bit_lattice::ONE)
            {
               if(arg1_bitstring_it == arg1_bitstring.begin() && is_signed_var)
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
            else if(*arg1_bitstring_it == bit_lattice::ONE && *arg2_bitstring_it == bit_lattice::ZERO)
            {
               if(arg1_bitstring_it == arg1_bitstring.begin() && is_signed_var)
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
   else if(op_kind == eq_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid), "");
      auto* operation = GetPointer<eq_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto ee0 = [&] {
         if(arg1_bitstring.size() > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
         }
         if(arg2_bitstring.size() > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " == " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " ==");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " ?");

         std::deque<bit_lattice>::const_iterator arg1_bitstring_it = arg1_bitstring.begin();
         std::deque<bit_lattice>::const_iterator arg2_bitstring_it = arg2_bitstring.begin();
         bool computed_result = false;
         for(; arg1_bitstring_it != arg1_bitstring.end() && arg2_bitstring_it != arg2_bitstring.end(); ++arg1_bitstring_it, ++arg2_bitstring_it)
         {
            if(*arg1_bitstring_it == bit_lattice::U || *arg2_bitstring_it == bit_lattice::U)
            {
               // <U> is UNKNOWN
               res.push_front(bit_lattice::U);
               computed_result = true;
               break;
            }
            else if((*arg1_bitstring_it == bit_lattice::ZERO && *arg2_bitstring_it == bit_lattice::ONE) || (*arg1_bitstring_it == bit_lattice::ONE && *arg2_bitstring_it == bit_lattice::ZERO))
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
      if(tree_helper::CGetType(GET_NODE(operation->op0))->get_kind() == real_type_K && tree_helper::CGetType(GET_NODE(operation->op1))->get_kind() == real_type_K)
      {
         // TODO: add check for real type equality (mind zero sign bug)
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer Error: operation unhandled yet with real type operands -> " + GET_NODE(ga->op1)->get_kind_text());
         return res;
      }
      else
      {
         ee0();
      }
   }
   else if(op_kind == ne_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid), "");
      auto* operation = GetPointer<ne_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto ne0 = [&] {
         if(arg1_bitstring.size() > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
         }
         if(arg2_bitstring.size() > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " != " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " !=");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " ?");

         std::deque<bit_lattice>::const_iterator arg1_bitstring_it = arg1_bitstring.begin();
         std::deque<bit_lattice>::const_iterator arg2_bitstring_it = arg2_bitstring.begin();
         bool computed_result = false;
         for(; arg1_bitstring_it != arg1_bitstring.end() && arg2_bitstring_it != arg2_bitstring.end(); ++arg1_bitstring_it, ++arg2_bitstring_it)
         {
            if(*arg1_bitstring_it == bit_lattice::U || *arg2_bitstring_it == bit_lattice::U)
            {
               // <U> is UNKNOWN
               res.push_front(bit_lattice::U);
               computed_result = true;
               break;
            }
            else if((*arg1_bitstring_it == bit_lattice::ZERO && *arg2_bitstring_it == bit_lattice::ONE) || (*arg1_bitstring_it == bit_lattice::ONE && *arg2_bitstring_it == bit_lattice::ZERO))
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
      if(tree_helper::CGetType(GET_NODE(operation->op0))->get_kind() == real_type_K && tree_helper::CGetType(GET_NODE(operation->op1))->get_kind() == real_type_K)
      {
         // TODO: add check for real type inequality (mind zero sign bug)
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer Error: operation unhandled yet with real type operands -> " + GET_NODE(ga->op1)->get_kind_text());
         return res;
      }
      else
      {
         ne0();
      }
   }
#endif
#if 1
   else if(op_kind == minus_expr_K)
   {
      auto* operation = GetPointer<minus_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto me0 = [&] {
         size_t arg_size_max = std::max(arg1_bitstring.size(), arg2_bitstring.size());
         if(arg_size_max > arg2_bitstring.size())
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg_size_max);
         if(arg_size_max > arg1_bitstring.size())
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg_size_max);

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " - " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " -");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

         std::deque<bit_lattice>::const_reverse_iterator arg1_it = arg1_bitstring.rbegin();
         std::deque<bit_lattice>::const_reverse_iterator arg2_it = arg2_bitstring.rbegin();
         bit_lattice borrow = bit_lattice::ZERO;

         unsigned int max_bitsize = BitLatticeManipulator::Size(GET_NODE(ga->op0));
         for(unsigned bit_index = 0; bit_index < max_bitsize && arg1_it != arg1_bitstring.rend() && arg2_it != arg2_bitstring.rend(); ++arg1_it, ++arg2_it, ++bit_index)
         {
            res.push_front(minus_expr_map.at(*arg1_it).at(*arg2_it).at(borrow).back());
            borrow = minus_expr_map.at(*arg1_it).at(*arg2_it).at(borrow).front();
         }
         if(tree_helper::is_int(TM, output_uid) && res.size() < max_bitsize)
            res.push_front(minus_expr_map.at(arg1_bitstring.front()).at(arg2_bitstring.front()).at(borrow).back());
         else if(!tree_helper::is_int(TM, output_uid))
         {
            while(res.size() < max_bitsize)
               res.push_front(borrow);
         }
      };
      me0();
   }
#endif
#if 1
   else if(op_kind == negate_expr_K)
   {
      auto* operation = GetPointer<negate_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg_bitstring;
      unsigned int arg_uid = 0;

      if(GET_NODE(operation->op)->get_kind() == ssa_name_K)
      {
         arg_uid = GET_INDEX_NODE(operation->op);
         if(current.find(arg_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op)));
         }
         else
            arg_bitstring = current.at(arg_uid);
      }
      else if(GET_NODE(operation->op)->get_kind() == integer_cst_K or GET_NODE(operation->op)->get_kind() == real_cst_K)
      {
         arg_uid = GET_INDEX_NODE(operation->op);
         THROW_ASSERT(best.find(arg_uid) != best.end(), "unexpected condition");
         arg_bitstring = best.at(arg_uid);
      }
      else
         return res;

      auto ne0 = [&] {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = - " + STR(arg_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " -");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg_bitstring) + " =");

         bit_lattice borrow = bit_lattice::ZERO;
         unsigned int max_bitsize = BitLatticeManipulator::Size(GET_NODE(ga->op0));
         if(!tree_helper::is_int(TM, output_uid) && max_bitsize > arg_bitstring.size())
         {
            arg_bitstring = sign_extend_bitstring(arg_bitstring, tree_helper::is_int(TM, output_uid), max_bitsize);
         }
         auto arg_it = arg_bitstring.rbegin();
         for(unsigned bit_index = 0; bit_index < max_bitsize && arg_it != arg_bitstring.rend(); ++arg_it, ++bit_index)
         {
            res.push_front(minus_expr_map.at(bit_lattice::ZERO).at(*arg_it).at(borrow).back());
            borrow = minus_expr_map.at(bit_lattice::ZERO).at(*arg_it).at(borrow).front();
         }
         if(tree_helper::is_int(TM, output_uid) && res.size() < max_bitsize)
            res.push_front(minus_expr_map.at(bit_lattice::ZERO).at(arg_bitstring.front()).at(borrow).back());
      };

      if(tree_helper::is_real(TM, arg_uid))
      {
         const auto arg_size = BitLatticeManipulator::Size(tree_helper::CGetType(GET_NODE(operation->op)));
         if(arg_bitstring.size() < arg_size)
         {
            arg_bitstring = sign_extend_bitstring(arg_bitstring, false, arg_size);
         }
         THROW_ASSERT(arg_size == arg_bitstring.size(), "Real bitstring must be exact: " + bitstring_to_string(arg_bitstring) + "<" + STR(arg_bitstring.size()) + "> should be " + STR(arg_size) + " bits");
         if(arg_bitstring.front() == bit_lattice::ONE)
         {
            arg_bitstring.pop_front();
            arg_bitstring.push_front(bit_lattice::ZERO);
         }
         else if(arg_bitstring.front() == bit_lattice::ZERO)
         {
            arg_bitstring.pop_front();
            arg_bitstring.push_front(bit_lattice::ONE);
         }
         res = arg_bitstring;
      }
      else
      {
         ne0();
      }
   }
#endif
#if 1
   else if(op_kind == abs_expr_K)
   {
      auto* operation = GetPointer<abs_expr>(GET_NODE(ga->op1));

      const tree_nodeConstRef op_type = tree_helper::CGetType(GET_NODE(operation->op));
      if(not is_handled_by_bitvalue(op_type->index))
         return res;

      const unsigned int arg_uid = GET_INDEX_NODE(operation->op);

      std::deque<bit_lattice> arg_bitstring;
      if(GET_NODE(operation->op)->get_kind() == ssa_name_K)
      {
         if(current.find(arg_uid) == current.end())
            arg_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op)));
         else
            arg_bitstring = current.at(arg_uid);
      }
      else if(GET_NODE(operation->op)->get_kind() == integer_cst_K or GET_NODE(operation->op)->get_kind() == real_cst_K)
      {
         THROW_ASSERT(best.find(arg_uid) != best.end(), "unexpected condition");
         arg_bitstring = best.at(arg_uid);
      }
      else
      {
         return res;
      }

      auto ae0 = [&] {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = abs " + STR(arg_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " abs");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg_bitstring) + " =");

         const auto sign_bit = arg_bitstring.front();
         switch(sign_bit)
         {
            case bit_lattice::ZERO:
            {
               res = arg_bitstring;
               break;
            }
            case bit_lattice::ONE:
            {
               bit_lattice borrow = bit_lattice::ZERO;
               for(const auto bit : boost::adaptors::reverse(arg_bitstring))
               {
                  const auto borrow_and_bit_pair = minus_expr_map.at(bit_lattice::ZERO).at(bit).at(borrow);
                  res.push_front(borrow_and_bit_pair.back());
                  borrow = borrow_and_bit_pair.front();
               }
               if(res.size() < BitLatticeManipulator::Size(op_type))
                  res.push_front(minus_expr_map.at(bit_lattice::ZERO).at(arg_bitstring.front()).at(borrow).back());
               break;
            }
            case bit_lattice::U:
            {
               std::deque<bit_lattice> negated_bitstring;
               bit_lattice borrow = bit_lattice::ZERO;
               for(const auto bit : boost::adaptors::reverse(arg_bitstring))
               {
                  const auto borrow_and_bit_pair = minus_expr_map.at(bit_lattice::ZERO).at(bit).at(borrow);
                  negated_bitstring.push_front(borrow_and_bit_pair.back());
                  borrow = borrow_and_bit_pair.front();
               }
               if(negated_bitstring.size() < BitLatticeManipulator::Size(op_type))
                  negated_bitstring.push_front(minus_expr_map.at(bit_lattice::ZERO).at(arg_bitstring.front()).at(borrow).back());
               res = inf(arg_bitstring, negated_bitstring, output_uid);
               break;
            }
            case bit_lattice::X:
            default:
            {
               THROW_UNREACHABLE("unexpected bit lattice for sign bit" + bitstring_to_string(arg_bitstring));
               break;
            }
         }
      };

      if(tree_helper::is_real(TM, arg_uid))
      {
         const auto arg_size = BitLatticeManipulator::Size(op_type);
         if(arg_bitstring.size() < arg_size)
         {
            arg_bitstring = sign_extend_bitstring(arg_bitstring, false, arg_size);
         }
         THROW_ASSERT(arg_size == arg_bitstring.size(), "Real bitstring must be exact: " + bitstring_to_string(arg_bitstring) + "<" + STR(arg_bitstring.size()) + " should be " + STR(arg_size) + " bits");
         if(arg_bitstring.front() == bit_lattice::ONE)
         {
            arg_bitstring.pop_front();
            arg_bitstring.push_front(bit_lattice::ZERO);
         }
         res = arg_bitstring;
      }
      else
      {
         THROW_ASSERT(tree_helper::is_int(TM, output_uid) and tree_helper::is_int(TM, arg_uid), "lhs and rhs of an abs_expr must be signed");
         ae0();
      }
   }
#endif
#if 1
   else if(op_kind == bit_and_expr_K)
   {
      auto* operation = GetPointer<bit_and_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto bae0 = [&] {
         unsigned int max_size = BitLatticeManipulator::Size(GET_NODE(ga->op0));
         if(max_size > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), max_size);
         }
         if(max_size > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), max_size);
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " & " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " &");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

         std::deque<bit_lattice>::const_reverse_iterator arg1_it = arg1_bitstring.rbegin();
         std::deque<bit_lattice>::const_reverse_iterator arg2_it = arg2_bitstring.rbegin();

         for(unsigned bit_index = 0; bit_index < max_size && arg1_it != arg1_bitstring.rend() && arg2_it != arg2_bitstring.rend(); ++arg1_it, ++arg2_it, ++bit_index)
         {
            res.push_front(bit_and_expr_map.at(*arg1_it).at(*arg2_it));
         }
      };
      bae0();
   }
#endif
#if 1
   else if(op_kind == bit_ior_expr_K)
   {
      auto* operation = GetPointer<bit_ior_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto bie0 = [&] {
         if(arg1_bitstring.size() == 1 && arg1_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, arg1_uid) && !tree_helper::is_int(TM, arg1_uid))
            arg1_bitstring.push_front(bit_lattice::ZERO);
         if(arg2_bitstring.size() == 1 && arg2_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, arg2_uid) && !tree_helper::is_int(TM, arg2_uid))
            arg2_bitstring.push_front(bit_lattice::ZERO);
         unsigned int max_size = BitLatticeManipulator::Size(GET_NODE(ga->op0));
         if(max_size > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), max_size);
         }
         if(max_size > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), max_size);
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " | " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " |");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

         std::deque<bit_lattice>::const_reverse_iterator arg1_it = arg1_bitstring.rbegin();
         std::deque<bit_lattice>::const_reverse_iterator arg2_it = arg2_bitstring.rbegin();

         for(unsigned bit_index = 0; bit_index < max_size && arg1_it != arg1_bitstring.rend() && arg2_it != arg2_bitstring.rend(); ++arg1_it, ++arg2_it, ++bit_index)
         {
            res.push_front(bit_ior_expr_map.at(*arg1_it).at(*arg2_it));
         }
      };
      bie0();
   }
#endif
#if 1
   else if(op_kind == bit_ior_concat_expr_K)
   {
      auto* operation = GetPointer<bit_ior_concat_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg1_uid = 0;
      unsigned int arg2_uid = 0;

      if(GET_NODE(operation->op0)->get_kind() == ssa_name_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         if(current.find(arg1_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op0));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op0)));
         }
         else
            arg1_bitstring = current.at(arg1_uid);
      }
      else if(GET_NODE(operation->op0)->get_kind() == integer_cst_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op0))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;

      if(GET_NODE(operation->op1)->get_kind() == ssa_name_K)
      {
         arg2_uid = GET_INDEX_NODE(operation->op1);
         if(current.find(arg2_uid) == current.end())
         {
            const tree_nodeConstRef op2_type = tree_helper::CGetType(GET_NODE(operation->op1));
            if(not is_handled_by_bitvalue(op2_type->index))
               return res;
            else
               arg2_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op1)));
         }
         else
            arg2_bitstring = current.at(arg2_uid);
      }
      else if(GET_NODE(operation->op1)->get_kind() == integer_cst_K)
      {
         arg2_uid = GET_INDEX_NODE(operation->op1);
         THROW_ASSERT(best.find(arg2_uid) != best.end(), "unexpected condition");
         arg2_bitstring = best.at(arg2_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op1))->value) + " : " + bitstring_to_string(arg2_bitstring));
      }
      else
         return res;

      auto bice0 = [&] {
         long long int offset = GetPointer<integer_cst>(GET_NODE(operation->op2))->value;

         if(arg1_bitstring.size() > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
         }
         if(arg2_bitstring.size() > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " |concat " + STR(arg2_uid) + "(" + STR(offset) + ")");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " |concat");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

         std::deque<bit_lattice>::const_reverse_iterator arg1_it = arg1_bitstring.rbegin();
         std::deque<bit_lattice>::const_reverse_iterator arg2_it = arg2_bitstring.rbegin();
         long long index = 0;

         for(; arg1_it != arg1_bitstring.rend() && arg2_it != arg2_bitstring.rend(); ++arg1_it, ++arg2_it, ++index)
         {
            if(index < offset)
               res.push_front(*arg2_it);
            else
               res.push_front(*arg1_it);
         }
      };
      bice0();
   }
#endif
#if 1
   else if(op_kind == bit_xor_expr_K)
   {
      auto* operation = GetPointer<bit_xor_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto bxe0 = [&] {
         if(arg1_bitstring.size() == 1 && arg1_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, arg1_uid) && !tree_helper::is_int(TM, arg1_uid))
            arg1_bitstring.push_front(bit_lattice::ZERO);
         if(arg2_bitstring.size() == 1 && arg2_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, arg2_uid) && !tree_helper::is_int(TM, arg2_uid))
            arg2_bitstring.push_front(bit_lattice::ZERO);

         unsigned int max_size = BitLatticeManipulator::Size(GET_NODE(ga->op0));
         if(max_size > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), max_size);
         }
         if(max_size > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), max_size);
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " ^ " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " ^");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

         std::deque<bit_lattice>::const_reverse_iterator arg1_it = arg1_bitstring.rbegin();
         std::deque<bit_lattice>::const_reverse_iterator arg2_it = arg2_bitstring.rbegin();

         for(unsigned bit_index = 0; bit_index < max_size && arg1_it != arg1_bitstring.rend() && arg2_it != arg2_bitstring.rend(); ++arg1_it, ++arg2_it, ++bit_index)
         {
            res.push_front(bit_xor_expr_map.at(*arg1_it).at(*arg2_it));
         }
      };
      bxe0();
   }
#endif
#if 1
   else if(op_kind == bit_not_expr_K)
   {
      auto* operation = GetPointer<bit_not_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      unsigned int arg1_uid = 0;

      if(GET_NODE(operation->op)->get_kind() == ssa_name_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op);
         if(current.find(arg1_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op)));
         }
         else
            arg1_bitstring = current.at(arg1_uid);
      }
      else if(GET_NODE(operation->op)->get_kind() == integer_cst_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;
      auto bne0 = [&] {
         unsigned int max_size = BitLatticeManipulator::Size(GET_NODE(ga->op0));
         if(arg1_bitstring.size() == 1 && arg1_bitstring.at(0) == bit_lattice::X && !tree_helper::is_bool(TM, arg1_uid) && !tree_helper::is_int(TM, arg1_uid))
            arg1_bitstring.push_front(bit_lattice::ZERO);
         if(arg1_bitstring.size() < max_size)
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), max_size);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = ~" + STR(arg1_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " ~" + bitstring_to_string(arg1_bitstring) + " =");

         std::deque<bit_lattice>::const_reverse_iterator arg1_it = arg1_bitstring.rbegin();
         for(unsigned bit_index = 0; bit_index < max_size && arg1_it != arg1_bitstring.rend(); ++arg1_it, ++bit_index)
         {
            res.push_front(bit_xor_expr_map.at(*arg1_it).at(bit_lattice::ONE));
         }
      };
      bne0();
   }
#endif
#if 1
   else if(op_kind == truth_and_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid), "");
      auto* operation = GetPointer<truth_and_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto tae0 = [&] {
         if(arg1_bitstring.size() > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
         }
         if(arg2_bitstring.size() > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " && " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " &&");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

         bit_lattice arg_left = bit_lattice::ZERO;
         for(auto current_bit : arg1_bitstring)
         {
            if(current_bit == bit_lattice::ONE)
            {
               arg_left = bit_lattice::ONE;
               break;
            }
            else if(current_bit == bit_lattice::U)
               arg_left = bit_lattice::U;
         }
         bit_lattice arg_right = bit_lattice::ZERO;
         for(auto current_bit : arg2_bitstring)
         {
            if(current_bit == bit_lattice::ONE)
            {
               arg_right = bit_lattice::ONE;
               break;
            }
            else if(current_bit == bit_lattice::U)
               arg_right = bit_lattice::U;
         }
         res.push_front(bit_and_expr_map.at(arg_left).at(arg_right));
      };
      tae0();
   }
   else if(op_kind == truth_or_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid), "");
      auto* operation = GetPointer<truth_or_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto toe0 = [&] {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " || " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " ||");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

         bit_lattice arg_left = bit_lattice::ZERO;
         for(auto current_bit : arg1_bitstring)
         {
            if(current_bit == bit_lattice::ONE)
            {
               arg_left = bit_lattice::ONE;
               break;
            }
            else if(current_bit == bit_lattice::U)
               arg_left = bit_lattice::U;
         }
         bit_lattice arg_right = bit_lattice::ZERO;
         for(auto current_bit : arg2_bitstring)
         {
            if(current_bit == bit_lattice::ONE)
            {
               arg_right = bit_lattice::ONE;
               break;
            }
            else if(current_bit == bit_lattice::U)
               arg_right = bit_lattice::U;
         }
         res.push_front(bit_ior_expr_map.at(arg_left).at(arg_right));
      };
      toe0();
   }
   else if(op_kind == truth_xor_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid), "");
      auto* operation = GetPointer<truth_xor_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;

      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto txe0 = [&] {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " ^ " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " ^");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

         bit_lattice arg_left = bit_lattice::ZERO;
         for(auto current_bit : arg1_bitstring)
         {
            if(current_bit == bit_lattice::ONE)
            {
               arg_left = bit_lattice::ONE;
               break;
            }
            else if(current_bit == bit_lattice::U)
               arg_left = bit_lattice::U;
         }
         bit_lattice arg_right = bit_lattice::ZERO;
         for(auto current_bit : arg2_bitstring)
         {
            if(current_bit == bit_lattice::ONE)
            {
               arg_right = bit_lattice::ONE;
               break;
            }
            else if(current_bit == bit_lattice::U)
               arg_right = bit_lattice::U;
         }

         res.push_front(bit_xor_expr_map.at(arg_left).at(arg_right));
      };
      txe0();
   }
   else if(op_kind == truth_not_expr_K)
   {
      THROW_ASSERT(tree_helper::is_bool(TM, output_uid) || BitLatticeManipulator::size(TM, output_uid) == 1, "");
      auto* operation = GetPointer<truth_not_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      unsigned int arg1_uid = 0;

      if(GET_NODE(operation->op)->get_kind() == ssa_name_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op);
         if(current.find(arg1_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op)));
         }
         else
            arg1_bitstring = current.at(arg1_uid);
      }
      else if(GET_NODE(operation->op)->get_kind() == integer_cst_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;

      auto tne0 = [&] {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = !" + STR(arg1_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " !" + bitstring_to_string(arg1_bitstring) + " =");

         bit_lattice arg_left = bit_lattice::ZERO;
         for(auto current_bit : arg1_bitstring)
         {
            if(current_bit == bit_lattice::ONE)
            {
               arg_left = bit_lattice::ONE;
               break;
            }
            else if(current_bit == bit_lattice::U)
               arg_left = bit_lattice::U;
         }
         res.push_front(bit_xor_expr_map.at(arg_left).at(bit_lattice::ONE));
      };
      tne0();
   }
#endif
#if 1
   else if(op_kind == rshift_expr_K)
   {
      auto* operation = GetPointer<rshift_expr>(GET_NODE(ga->op1));
      std::deque<bit_lattice> arg1_bitstring;
      unsigned int arg1_uid = 0;
      if(GET_NODE(operation->op0)->get_kind() == ssa_name_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         if(current.find(arg1_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op0));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op0)));
         }
         else
            arg1_bitstring = current.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, first argument is ssa with bitstring -> " + bitstring_to_string(arg1_bitstring));
      }
      else if(GET_NODE(operation->op0)->get_kind() == integer_cst_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op0))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;

      if(GET_NODE(operation->op1)->get_kind() == ssa_name_K)
      {
         res = create_u_bitstring(static_cast<unsigned int>(arg1_bitstring.size()));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " >> " + STR(GET_INDEX_NODE(operation->op1)));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(res));
      }
      else if(GET_NODE(operation->op1)->get_kind() == integer_cst_K)
      {
         auto* const2 = GetPointer<integer_cst>(GET_NODE(operation->op1));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, second argument is constant -> " + STR(const2->value));
         if(const2->value < 0)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, negative right shift is undefined behavior");
            res.push_back(bit_lattice::X);
            return res;
         }

         if(arg1_bitstring.size() <= static_cast<size_t>(const2->value))
         {
            if(tree_helper::is_int(TM, arg1_uid))
               res.push_front(arg1_bitstring.front());
            else
               res.push_front(bit_lattice::ZERO);
         }
         else
         {
            size_t new_lenght = arg1_bitstring.size() - static_cast<size_t>(const2->value);
            std::deque<bit_lattice>::const_iterator arg1_it = arg1_bitstring.begin();
            while(res.size() < new_lenght)
            {
               res.push_back(*arg1_it);
               ++arg1_it;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " >> " + STR(GET_INDEX_NODE(operation->op1)));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(arg1_bitstring) + ">>" + STR(const2->value) + " => " + bitstring_to_string(res));
      }
      else
         return res;
   }
#endif
#if 1
   else if(op_kind == lut_expr_K)
   {
      res = create_u_bitstring(1);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation lut_expr: " + STR(output_uid) + " = LUT VALUE >> ins");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(res));
      return res;
   }
   else if(op_kind == extract_bit_expr_K)
   {
      auto* operation = GetPointer<extract_bit_expr>(GET_NODE(ga->op1));
      std::deque<bit_lattice> arg1_bitstring;
      unsigned int arg1_uid = 0;
      if(GET_NODE(operation->op0)->get_kind() == ssa_name_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         if(current.find(arg1_uid) == current.end())
            arg1_bitstring = create_u_bitstring(1);
         else
            arg1_bitstring = current.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, first argument is ssa with bitstring -> " + bitstring_to_string(arg1_bitstring));
      }
      else if(GET_NODE(operation->op0)->get_kind() == integer_cst_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op0))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;
      auto ebe0 = [&] {
         THROW_ASSERT(GET_NODE(operation->op1)->get_kind() == integer_cst_K, "unexpected condition");
         auto* const2 = GetPointer<integer_cst>(GET_NODE(operation->op1));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, second argument is constant -> " + STR(const2->value));
         THROW_ASSERT(const2->value >= 0, "unexpected condition");

         if(arg1_bitstring.size() <= static_cast<size_t>(const2->value))
         {
            if(tree_helper::is_int(TM, arg1_uid))
               res.push_front(arg1_bitstring.front());
            else
               res.push_front(bit_lattice::ZERO);
         }
         else
         {
            size_t new_lenght = arg1_bitstring.size() - static_cast<size_t>(const2->value);
            std::deque<bit_lattice>::const_iterator arg1_it = arg1_bitstring.begin();
            while(res.size() < new_lenght)
            {
               res.push_back(*arg1_it);
               ++arg1_it;
            }
            while(res.size() > 1)
               res.pop_front();
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " extract bit " + STR(GET_INDEX_NODE(operation->op1)));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(arg1_bitstring) + ">>" + STR(const2->value) + " => " + bitstring_to_string(res));
      };
      ebe0();
   }
#endif
#if 1
   else if(op_kind == rrotate_expr_K)
   {
      auto* operation = GetPointer<rrotate_expr>(GET_NODE(ga->op1));
      std::deque<bit_lattice> arg1_bitstring;
      unsigned int arg1_uid = 0;
      if(GET_NODE(operation->op0)->get_kind() == ssa_name_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         if(current.find(arg1_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op0));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op0)));
         }
         else
            arg1_bitstring = current.at(arg1_uid);
      }
      else if(GET_NODE(operation->op0)->get_kind() == integer_cst_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op0))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;

      if(GET_NODE(operation->op1)->get_kind() == ssa_name_K)
      {
         unsigned int precision = BitLatticeManipulator::Size(tree_helper::CGetType(GET_NODE(ga->op0)));
         res = create_u_bitstring(precision);
      }
      else if(GET_NODE(operation->op1)->get_kind() == integer_cst_K)
      {
         auto* const2 = GetPointer<integer_cst>(GET_NODE(operation->op1));
         auto arg2_value = static_cast<unsigned int>(const2->value);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(arg2_value));
         unsigned int precision = BitLatticeManipulator::Size(tree_helper::CGetType(GET_NODE(ga->op0)));

         if(precision > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), precision);
         }
         res = arg1_bitstring;
         for(unsigned int index = 0; index < arg2_value; ++index)
         {
            bit_lattice cur_bit = res.back();
            res.pop_back();
            res.push_front(cur_bit);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " rrotate " + STR(GET_INDEX_NODE(operation->op1)));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " rrotate");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, STR(arg2_value) + " =");
      }
      return res;
   }
   else if(op_kind == lrotate_expr_K)
   {
      auto* operation = GetPointer<lrotate_expr>(GET_NODE(ga->op1));
      std::deque<bit_lattice> arg1_bitstring;
      unsigned int arg1_uid = 0;
      if(GET_NODE(operation->op0)->get_kind() == ssa_name_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         if(current.find(arg1_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op0));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op0)));
         }
         else
            arg1_bitstring = current.at(arg1_uid);
      }
      else if(GET_NODE(operation->op0)->get_kind() == integer_cst_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op0))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;

      if(GET_NODE(operation->op1)->get_kind() == ssa_name_K)
      {
         unsigned int precision = BitLatticeManipulator::Size(tree_helper::CGetType(GET_NODE(ga->op0)));
         res = create_u_bitstring(precision);
      }
      else if(GET_NODE(operation->op1)->get_kind() == integer_cst_K)
      {
         auto* const2 = GetPointer<integer_cst>(GET_NODE(operation->op1));
         auto arg2_value = static_cast<unsigned int>(const2->value);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(arg2_value));
         unsigned int precision = BitLatticeManipulator::Size(tree_helper::CGetType(GET_NODE(ga->op0)));

         if(precision > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), precision);
         }
         res = arg1_bitstring;
         for(unsigned int index = 0; index < arg2_value; ++index)
         {
            bit_lattice cur_bit = res.front();
            res.pop_front();
            res.push_back(cur_bit);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " lrotate " + STR(GET_INDEX_NODE(operation->op1)));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " lrotate");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, STR(arg2_value) + " =");
      }
      return res;
   }
#endif
#if 1
   else if(op_kind == lshift_expr_K)
   {
      auto* operation = GetPointer<lshift_expr>(GET_NODE(ga->op1));
      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg1_uid = 0;
      if(GET_NODE(operation->op0)->get_kind() == ssa_name_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         if(current.find(arg1_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op0));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op0)));
         }
         else
            arg1_bitstring = current.at(arg1_uid);
      }
      else if(GET_NODE(operation->op0)->get_kind() == integer_cst_K)
      {
         arg1_uid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op0))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;

      unsigned int res_bitsize = BitLatticeManipulator::Size(GET_NODE(ga->op0));
      if(GET_NODE(operation->op1)->get_kind() == ssa_name_K)
      {
         unsigned int arg2_uid;
         arg2_uid = GET_INDEX_NODE(operation->op1);
         if(current.find(arg2_uid) == current.end())
         {
            const tree_nodeConstRef op2_type = tree_helper::CGetType(GET_NODE(operation->op1));
            if(not is_handled_by_bitvalue(op2_type->index))
               return res;
            else
               arg2_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op1)));
         }
         else
            arg2_bitstring = current.at(arg2_uid);

         long long unsigned int bsize_elev2 = 1ULL << arg2_bitstring.size();
         if(res_bitsize < bsize_elev2 || res_bitsize < bsize_elev2 + arg1_bitstring.size())
            res = create_u_bitstring(res_bitsize);
         else
            res = create_u_bitstring(static_cast<unsigned int>(arg1_bitstring.size() + bsize_elev2));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " << " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(arg1_bitstring) + "<<" + bitstring_to_string(arg2_bitstring) + " => " + bitstring_to_string(res));
      }
      else if(GET_NODE(operation->op1)->get_kind() == integer_cst_K)
      {
         auto* const2 = GetPointer<integer_cst>(GET_NODE(operation->op1));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(const2->value));
         if(const2->value < 0)
         {
            res.push_back(bit_lattice::X);
            return res;
         }

         const unsigned int arg1_bitsize = BitLatticeManipulator::Size(GET_NODE(operation->op0));
         if(res_bitsize <= static_cast<long long unsigned int>(const2->value))
            res.push_front(bit_lattice::ZERO);
         else
         {
            res = arg1_bitstring;
            while(res.size() > arg1_bitsize)
               res.pop_front();
            for(int i = 0; i < const2->value; i++)
            {
               res.push_back(bit_lattice::ZERO);
               if(res.size() > res_bitsize)
                  res.pop_front();
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " << " + STR(GET_INDEX_NODE(operation->op1)));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(arg1_bitstring) + "<<" + STR(const2->value) + " => " + bitstring_to_string(res));
      }
      else
         return res;
   }
#endif
#if 1
   else if(op_kind == nop_expr_K || op_kind == convert_expr_K || op_kind == view_convert_expr_K)
   {
      auto* operation = GetPointer<unary_expr>(GET_NODE(ga->op1));
      const tree_nodeConstRef left_type = tree_helper::CGetType(GET_NODE(ga->op0));
      const size_t left_type_size = BitLatticeManipulator::Size(left_type);
      const auto right_id = GET_INDEX_NODE(operation->op);
      const tree_nodeConstRef right_type = tree_helper::CGetType(GET_NODE(operation->op));
      const unsigned int right_type_size = BitLatticeManipulator::Size(right_type);
      if(not is_handled_by_bitvalue(right_id))
      {
         res = create_u_bitstring(right_type_size);
         return res;
      }
      if(tree_helper::is_real(TM, left_type->index) and tree_helper::is_real(TM, right_type->index))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer Error: operation unhandled yet with real type operands -> " + GET_NODE(ga->op1)->get_kind_text());
         return res;
      }

      if(GET_NODE(operation->op)->get_kind() == ssa_name_K or GET_NODE(operation->op)->get_kind() == integer_cst_K or GET_NODE(operation->op)->get_kind() == real_cst_K)
      {
         unsigned int arg1_uid = GET_INDEX_NODE(operation->op);
         if(GET_NODE(operation->op)->get_kind() == ssa_name_K)
         {
            if(current.find(arg1_uid) == current.end())
            {
               const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op));
               if(not is_handled_by_bitvalue(op1_type->index))
                  return res;
               else
                  res = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op)));
            }
            else
            {
               res = current.at(arg1_uid);
            }
         }
         else // if (GET_NODE(operation->op)->get_kind() == integer_cst_K)
         {
            THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
            res = best.at(arg1_uid);
         }

         const auto left_id = GET_INDEX_NODE(ga->op0);
         const bool left_signed = tree_helper::is_int(TM, left_id);
         const bool right_signed = tree_helper::is_int(TM, right_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(left_id) + (left_signed ? "S" : "U") + " = " + (op_kind == nop_expr_K ? "cast" : "convert") + " " + STR(right_id) + (right_signed ? "S" : "U"));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " = op:  " + bitstring_to_string(res) + "(" + STR(right_type_size) + "->" + STR(left_type_size) + ")");
         bool do_not_extend = false;
         if(left_signed && BitLatticeManipulator::Size(ga->op0) == 1 && tree_helper::is_bool(TM, right_id))
         {
            do_not_extend = true;
         }
         if(tree_helper::is_real(TM, left_type->index) and res.size() < left_type_size)
         {
            res = sign_extend_bitstring(res, res.front() == bit_lattice::U, left_type_size);
         }
         if((left_signed != right_signed and !do_not_extend) and res.size() < left_type_size)
         {
            res = sign_extend_bitstring(res, right_signed, left_type_size);
         }
         while(res.size() > left_type_size)
         {
            res.pop_front();
         }
         THROW_ASSERT(not tree_helper::is_real(TM, left_type->index) or res.size() == left_type_size, "Real type bit value should be exact");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + bitstring_to_string(res));
      }
   }
#endif
#if 1
   else if(op_kind == cond_expr_K)
   {
      auto* operation = GetPointer<cond_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      std::deque<bit_lattice> arg3_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg3_uid = 0;

      if(GET_NODE(operation->op0)->get_kind() == ssa_name_K)
      {
         const auto arg1_uid = GET_INDEX_NODE(operation->op0);
         if(current.find(arg1_uid) == current.end())
         {
            const tree_nodeConstRef op1_type = tree_helper::CGetType(GET_NODE(operation->op0));
            if(not is_handled_by_bitvalue(op1_type->index))
               return res;
            else
               arg1_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op0)));
         }
         else
            arg1_bitstring = current.at(arg1_uid);
      }
      else if(GET_NODE(operation->op0)->get_kind() == integer_cst_K)
      {
         const auto arg1_uid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op0))->value) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else if(GET_NODE(operation->op0)->get_kind() == real_cst_K)
      {
         const auto arg1_uid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.find(arg1_uid) != best.end(), "unexpected condition");
         arg1_bitstring = best.at(arg1_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<real_cst>(GET_NODE(operation->op0))->valr) + " : " + bitstring_to_string(arg1_bitstring));
      }
      else
         return res;

      if(GET_NODE(operation->op1)->get_kind() == ssa_name_K)
      {
         arg2_uid = GET_INDEX_NODE(operation->op1);
         if(current.find(arg2_uid) == current.end())
         {
            const tree_nodeConstRef op2_type = tree_helper::CGetType(GET_NODE(operation->op1));
            if(not is_handled_by_bitvalue(op2_type->index))
               return res;
            else
               arg2_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op1)));
         }
         else
            arg2_bitstring = current.at(arg2_uid);
      }
      else if(GET_NODE(operation->op1)->get_kind() == integer_cst_K)
      {
         arg2_uid = GET_INDEX_NODE(operation->op1);
         THROW_ASSERT(best.find(arg2_uid) != best.end(), "unexpected condition");
         arg2_bitstring = best.at(arg2_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op1))->value) + " : " + bitstring_to_string(arg2_bitstring));
      }
      else if(GET_NODE(operation->op1)->get_kind() == real_cst_K)
      {
         arg2_uid = GET_INDEX_NODE(operation->op1);
         THROW_ASSERT(best.find(arg2_uid) != best.end(), "unexpected condition");
         arg2_bitstring = best.at(arg2_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<real_cst>(GET_NODE(operation->op1))->valr) + " : " + bitstring_to_string(arg2_bitstring));
      }
      else
         return res;

      if(GET_NODE(operation->op2)->get_kind() == ssa_name_K)
      {
         arg3_uid = GET_INDEX_NODE(operation->op2);
         if(current.find(arg3_uid) == current.end())
         {
            const tree_nodeConstRef op3_type = tree_helper::CGetType(GET_NODE(operation->op2));
            if(not is_handled_by_bitvalue(op3_type->index))
               return res;
            else
               arg3_bitstring = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(operation->op2)));
         }
         else
            arg3_bitstring = current.at(arg3_uid);
      }
      else if(GET_NODE(operation->op2)->get_kind() == integer_cst_K)
      {
         arg3_uid = GET_INDEX_NODE(operation->op2);
         THROW_ASSERT(best.find(arg3_uid) != best.end(), "unexpected condition");
         arg3_bitstring = best.at(arg3_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<integer_cst>(GET_NODE(operation->op2))->value) + " : " + bitstring_to_string(arg3_bitstring));
      }
      else if(GET_NODE(operation->op2)->get_kind() == real_cst_K)
      {
         arg3_uid = GET_INDEX_NODE(operation->op2);
         THROW_ASSERT(best.find(arg3_uid) != best.end(), "unexpected condition");
         arg3_bitstring = best.at(arg3_uid);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, created bitstring from constant -> " + STR(GetPointer<real_cst>(GET_NODE(operation->op2))->valr) + " : " + bitstring_to_string(arg3_bitstring));
      }
      else
         return res;

      auto ce0 = [&] {
         bit_lattice arg_cond = bit_lattice::ZERO;
         for(auto current_bit : arg1_bitstring)
         {
            if(current_bit == bit_lattice::ONE)
            {
               arg_cond = bit_lattice::ONE;
               break;
            }
            else if(current_bit == bit_lattice::U)
               arg_cond = bit_lattice::U;
         }

         if(arg_cond == bit_lattice::ZERO)
         {
            // Condition is false return second arg (arg3_bitstring)
            res = arg3_bitstring;
         }
         else if(arg_cond == bit_lattice::ONE)
         {
            // Condition is false return second arg (arg3_bitstring)
            res = arg2_bitstring;
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
            const auto inf_size = std::max(arg2_bitstring.size(), arg3_bitstring.size());
            if(arg2_bitstring.size() < inf_size)
               arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), inf_size);
            if(arg3_bitstring.size() < inf_size)
               arg3_bitstring = sign_extend_bitstring(arg3_bitstring, tree_helper::is_int(TM, arg3_uid), inf_size);
            const auto out_uid = GET_INDEX_NODE(ga->op0);
            res = inf(arg2_bitstring, arg3_bitstring, out_uid);
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " ? " + bitstring_to_string(arg2_bitstring) + " : " + bitstring_to_string(arg3_bitstring));
      };
      ce0();
   }
#endif
#if 1
   else if(op_kind == min_expr_K || op_kind == max_expr_K)
   {
      auto* operation = GetPointer<binary_expr>(GET_NODE(ga->op1));

      std::deque<bit_lattice> arg1_bitstring;
      std::deque<bit_lattice> arg2_bitstring;
      unsigned int arg2_uid = 0;
      unsigned int arg1_uid = 0;
      if(!manage_forward_binary_operands(operation, arg1_uid, arg2_uid, arg1_bitstring, arg2_bitstring))
         return res;

      auto me0 = [&] {
         if(arg1_bitstring.size() > arg2_bitstring.size())
         {
            arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
         }
         if(arg2_bitstring.size() > arg1_bitstring.size())
         {
            arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer, operation: " + STR(output_uid) + " = " + STR(arg1_uid) + " " + (op_kind == min_expr_K ? std::string("min") : std::string("max")) + " " + STR(arg2_uid));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " " + (op_kind == min_expr_K ? std::string("min") : std::string("max")) + "");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " =");

#if HAVE_ASSERTS
         bool is_signed1 = tree_helper::is_int(TM, arg1_uid);
         bool is_signed2 = tree_helper::is_int(TM, arg2_uid);
#endif
         THROW_ASSERT(is_signed2 == is_signed1, "");
         res = inf(arg1_bitstring, arg2_bitstring, arg1_uid);
      };
      me0();
   }
#endif
#if 1
   else if(op_kind == call_expr_K || op_kind == aggr_init_expr_K)
   {
      auto ce0 = [&] {
         const auto call_it = direct_call_id_to_called_id.find(ga->index);
         if(call_it != direct_call_id_to_called_id.end())
         {
            const unsigned int called_id = call_it->second;
            const tree_nodeConstRef tn = TM->get_tree_node_const(called_id);
            const auto* fd = GetPointer<const function_decl>(tn);
            THROW_ASSERT(fd, "node " + STR(called_id) + " is not a function_decl");
            if(fd->bit_values.empty())
               res = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->op0)));
            else
               res = string_to_bitstring(fd->bit_values);
         }
      };
      ce0();
   }
#endif
   else if(op_kind == addr_expr_K)
   {
      auto ae0 = [&] {
         const auto* ae = GetPointer<addr_expr>(GET_NODE(ga->op1));
         auto address_size = AppM->get_address_bitsize();
         auto is_pretty_print_used = parameters->isOption(OPT_pretty_print);
         auto lt0 = lsb_to_zero(ae, is_pretty_print_used);
         const auto op_name = ae->ToString();
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "address_size: " + STR(address_size) + " lt0: " + STR(lt0));
         if(lt0 && address_size > lt0)
         {
            res = create_u_bitstring(address_size - lt0);
            for(auto index = 0u; index < lt0; ++index)
               res.push_back(bit_lattice::ZERO);
         }
      };
      ae0();
   }
#if 1
   else if(op_kind == mem_ref_K || op_kind == component_ref_K || op_kind == var_decl_K || op_kind == array_ref_K)
   {
      // do nothing
   }
#endif
   else
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "forward_transfer Error: operation unhandled yet -> " + GET_NODE(ga->op1)->get_kind_text());
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(res));
   return res;
}

void Bit_Value::forward()
{
   tree_nodeRef tn = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto* sl = GetPointer<const statement_list>(GET_NODE(fd->body));
   bool first_phase = true;
   do
   {
      if(first_phase)
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---=================== First Phase forward analysis");
      else
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---=================== Second Phase forward analysis");
      bool current_updated = true;
      while(current_updated)
      {
         current_updated = false;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Started new iteration");
         // for each basic block B in CFG do > Consider all blocks successively
         for(const auto& B_it : sl->list_of_bloc)
         {
            blocRef B = B_it.second;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(B->number));
            for(const auto& stmt : B->CGetStmtList())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + STR(stmt));
               const auto stmt_node = GET_NODE(stmt);
               if(stmt_node->get_kind() == gimple_assign_K)
               {
                  auto* ga = GetPointer<gimple_assign>(stmt_node);
                  unsigned int output_uid = GET_INDEX_NODE(ga->op0);
                  auto* ssa = GetPointer<ssa_name>(GET_NODE(ga->op0));

                  if(ssa)
                  {
                     if(not is_handled_by_bitvalue(output_uid))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(GET_NODE(ga->op0))) + " not considered id: " + STR(output_uid));
                        continue;
                     }
                     auto checkRequiredAllDefined = [&]() -> bool {
                        std::vector<std::tuple<unsigned int, unsigned int>> vars_read;
                        tree_helper::get_required_values(TM, vars_read, GET_NODE(stmt), GET_INDEX_NODE(stmt));
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---requires " + STR(vars_read.size()) + " values");
                        for(auto var_pair : vars_read)
                        {
                           unsigned int ssa_use_node_id = std::get<0>(var_pair);
                           if(ssa_use_node_id == 0)
                              continue;
                           if(not is_handled_by_bitvalue(ssa_use_node_id))
                              continue;
                           tree_nodeRef use_node = TM->get_tree_node_const(ssa_use_node_id);
                           auto* ssa_use = GetPointer<ssa_name>(use_node);

                           if(ssa_use && current.find(ssa_use_node_id) == current.end())
                           {
                              return false;
                           }
                        }
                        return true;
                     }();
                     if(first_phase && !checkRequiredAllDefined)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--inputs are not all fully analyzed by the forward Bit Value Analysis. Operation  " + GET_NODE(ga->op0)->ToString() + " postponed");
                        continue;
                     }

                     THROW_ASSERT(best.find(output_uid) != best.end(), "unexpected condition");
                     current.insert(std::make_pair(output_uid, best.at(output_uid)));
                     auto res = forward_transfer(ga);
                     current_updated = update_current(res, output_uid) or current_updated;
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + STR(stmt));
            }

            for(const auto& phi : B->CGetPhiList())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing Phi " + STR(phi));
               auto* pn = GetPointer<gimple_phi>(GET_NODE(phi));
               bool is_virtual = pn->virtual_flag;
               if(!is_virtual)
               {
                  unsigned int output_uid = GET_INDEX_NODE(pn->res);
#ifndef NDEBUG
                  auto* ssa = GetPointer<ssa_name>(GET_NODE(pn->res));
#endif
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "phi: " + STR(GET_INDEX_NODE(phi)));
                  if(not is_handled_by_bitvalue(output_uid))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(GET_NODE(pn->res))) + " not considered id: " + STR(output_uid));
                     continue;
                  }

                  if(!first_phase)
                  {
                     THROW_ASSERT(best.find(output_uid) != best.end(), "unexpected condition");
                     current.insert(std::make_pair(output_uid, best.at(output_uid)));
                  }

                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res id: " + STR(output_uid));
                  auto res = create_x_bitstring(1);
                  bool atLeastOne = false;
                  for(const auto& def_edge : pn->CGetDefEdgesList())
                  {
                     if(def_edge.first->index == pn->res->index)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping " + STR(def_edge.first) + " coming from BB" + STR(def_edge.second) + " because of ssa cycle");
                        continue;
                     }
                     if(first_phase && current.find(GET_INDEX_NODE(def_edge.first)) == current.end())
                     {
                        auto source_node = GET_NODE(def_edge.first);
                        if(GetPointer<ssa_name>(source_node))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping " + STR(def_edge.first) + " no current has been yet computed for this ssa var: " + source_node->ToString());
                           continue;
                        }
                     }
                     atLeastOne = true;
                     if(current.find(GET_INDEX_NODE(def_edge.first)) == current.end())
                     {
                        if(best.find(GET_INDEX_NODE(def_edge.first)) == best.end())
                           current[GET_INDEX_NODE(def_edge.first)] = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(pn->res)));
                        else
                           current[GET_INDEX_NODE(def_edge.first)] = best.at(GET_INDEX_NODE(def_edge.first));
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Edge " + STR(def_edge.second) + ": " + bitstring_to_string(current.at(GET_INDEX_NODE(def_edge.first))));

#if HAVE_ASSERTS
                     const auto is_signed1 = tree_helper::is_int(TM, output_uid);
                     const auto is_signed2 = tree_helper::is_int(TM, GET_INDEX_NODE(def_edge.first));
#endif
                     THROW_ASSERT(is_signed2 == is_signed1, STR(phi));
                     res = inf(res, current.at(GET_INDEX_NODE(def_edge.first)), output_uid);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---current res: " + bitstring_to_string(res));
                  }
                  if(atLeastOne)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---res: " + bitstring_to_string(res));
                     if(first_phase)
                     {
                        if(current.find(output_uid) == current.end())
                        {
                           current_updated = true;
                           current.insert(std::make_pair(output_uid, res));
                        }
                        else
                        {
                           current_updated = update_current(res, output_uid) or current_updated;
                        }
                     }
                     else
                     {
                        current_updated = update_current(res, output_uid) or current_updated;
                     }
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed Phi " + STR(phi));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(B->number));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended new iteration");
      }
      first_phase = !first_phase;
   } while(!first_phase);
}
