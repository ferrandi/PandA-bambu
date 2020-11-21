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
 * @file bit_lattice.cpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "bit_lattice.hpp"

/// . include
#include "Parameter.hpp"

/// STD include
#include <string>

/// STL includes
#include <algorithm>
#include <vector>

/// tree includes
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility includes
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

BitLatticeManipulator::BitLatticeManipulator(const tree_managerConstRef _TM, const int _bl_debug_level) : TM(_TM), bl_debug_level(_bl_debug_level)
{
}

BitLatticeManipulator::~BitLatticeManipulator() = default;

bit_lattice BitLatticeManipulator::bit_sup(const bit_lattice a, const bit_lattice b)
{
   if(a == b)
   {
      return a;
   }
   else if(a == bit_lattice::X or b == bit_lattice::X)
   {
      return bit_lattice::X;
   }
   else if(a == bit_lattice::U)
   {
      return b;
   }
   else if(b == bit_lattice::U)
   {
      return a;
   }
   else
   {
      return bit_lattice::X;
   }
}

std::deque<bit_lattice> BitLatticeManipulator::sup(const std::deque<bit_lattice>& _a, const std::deque<bit_lattice>& _b, const size_t out_type_size, const bool out_is_signed, const bool out_is_bool)
{
   THROW_ASSERT(not _a.empty() and not _b.empty(), "a = " + std::string(_a.empty() ? "empty" : bitstring_to_string(_a)) + ", b = " + (_b.empty() ? "empty" : bitstring_to_string(_b)));
   THROW_ASSERT(out_type_size, "Size can not be zero");
   THROW_ASSERT(not out_is_bool or (out_type_size == 1), "boolean with type size != 1");
   std::deque<bit_lattice> res;
   if(out_is_bool)
   {
      res.push_back(bit_sup(_a.back(), _b.back()));
      return res;
   }

   std::deque<bit_lattice> longer = (_a.size() >= _b.size()) ? _a : _b;
   std::deque<bit_lattice> shorter = (_a.size() >= _b.size()) ? _b : _a;
   while(longer.size() > out_type_size)
   {
      longer.pop_front();
   }
   while(shorter.size() > out_type_size)
   {
      shorter.pop_front();
   }
   if(longer.size() < out_type_size)
   {
      longer = sign_extend_bitstring(longer, out_is_signed, out_type_size);
   }
   if(shorter.size() < out_type_size)
   {
      shorter = sign_extend_bitstring(shorter, out_is_signed, out_type_size);
   }
   //    // BEWARE: zero is stored as single bit, but it has to be considered as a full string
   //    //         of zeros to propagate values correctly
   //    if(shorter.size() == 1 && shorter.front() == bit_lattice::ZERO)
   //    {
   //       auto l_bw = longer.size();
   //       for(;l_bw > 1; --l_bw)
   //             shorter.push_front(bit_lattice::ZERO);
   //    }
   //    if(!out_is_signed && longer.size() > shorter.size() && longer.front() == bit_lattice::ZERO)
   //    {
   //       shorter = sign_extend_bitstring(shorter, out_is_signed, longer.size());
   //    }
   //    else
   //    {
   //       while(longer.size() > shorter.size())
   //       {
   //          if(out_is_signed)
   //          {
   //             shorter = sign_extend_bitstring(shorter, out_is_signed, longer.size());
   //          }
   //          else
   //          {
   //             longer.pop_front();
   //          }
   //       }
   //    }

   auto a_it = longer.crbegin();
   auto b_it = shorter.crbegin();
   const auto a_end = longer.crend();
   const auto b_end = shorter.crend();
   for(; a_it != a_end and b_it != b_end; a_it++, b_it++)
   {
      res.push_front(bit_sup(*a_it, *b_it));
   }

   if(res.empty())
   {
      res.push_front(bit_lattice::X);
   }
   sign_reduce_bitstring(res, out_is_signed);
   if(out_is_signed)
   {
      const bool a_sign_is_x = _a.front() == bit_lattice::X;
      const bool b_sign_is_x = _b.front() == bit_lattice::X;
      const size_t final_size = std::min(out_type_size, ((a_sign_is_x == b_sign_is_x) ? (std::min(_a.size(), _b.size())) : (a_sign_is_x ? _a.size() : _b.size())));
      const auto sign_bit = res.front();
      while(res.size() > final_size)
      {
         res.pop_front();
      }
      if(sign_bit != bit_lattice::X && res.front() == bit_lattice::X)
      {
         res.pop_front();
         res.push_front(sign_bit);
      }
   }
   else
   {
      const size_t final_size = std::min(out_type_size, std::min(_a.size(), _b.size()));
      THROW_ASSERT(final_size, "final size of sup cannot be 0");
      while(res.size() > final_size)
      {
         if(res.at(0) != bit_lattice::ZERO || (res.size() > 1 && res.at(1) != bit_lattice::X))
         {
            res.pop_front();
         }
         else
         {
            break;
         }
      }
   }

   return res;
}

std::deque<bit_lattice> BitLatticeManipulator::sup(const std::deque<bit_lattice>& _a, const std::deque<bit_lattice>& _b, const unsigned int output_uid) const
{
   THROW_ASSERT(not _a.empty() and not _b.empty(), "");

   // extend the shorter of the two bitstrings
   const bool out_is_signed = (signed_var.find(output_uid) != signed_var.end()) or tree_helper::is_int(TM, output_uid);
   // compute the underlying type size
   const tree_nodeConstRef node = TM->get_tree_node_const(output_uid);
   const auto kind = node->get_kind();
   size_t out_type_size = 0;
   if(kind == ssa_name_K or kind == integer_cst_K or kind == real_cst_K)
   {
      out_type_size = size(TM, tree_helper::get_type_index(TM, output_uid));
   }
   else if(kind == function_decl_K)
   {
      const tree_nodeConstRef fu_type = tree_helper::CGetType(node);
      THROW_ASSERT(fu_type->get_kind() == function_type_K || fu_type->get_kind() == method_type_K, "node " + STR(output_uid) + " is " + fu_type->get_kind_text());
      tree_nodeRef fret_type_node;
      if(fu_type->get_kind() == function_type_K)
      {
         const auto* ft = GetPointer<const function_type>(fu_type);
         fret_type_node = GET_NODE(ft->retn);
      }
      else
      {
         const auto* mt = GetPointer<const method_type>(fu_type);
         fret_type_node = GET_NODE(mt->retn);
      }
      out_type_size = Size(fret_type_node);
   }
   else if(kind == parm_decl_K)
   {
      out_type_size = Size(tree_helper::CGetType(node));
   }
   else
   {
      THROW_UNREACHABLE("unexpected sup for output_uid " + STR(output_uid) + " of kind " + node->get_kind_text());
   }
   THROW_ASSERT(out_type_size, "");
   const bool out_is_bool = tree_helper::is_bool(TM, output_uid);
   THROW_ASSERT(not out_is_bool or (out_type_size == 1), "boolean with type size != 1");

   return sup(_a, _b, out_type_size, out_is_signed, out_is_bool);
}

bit_lattice BitLatticeManipulator::bit_inf(const bit_lattice a, const bit_lattice b)
{
   if(a == b)
   {
      return a;
   }
   else if(a == bit_lattice::U or b == bit_lattice::U)
   {
      return bit_lattice::U;
   }
   else if(a == bit_lattice::X)
   {
      return b;
   }
   else if(b == bit_lattice::X)
   {
      return a;
   }
   else
   {
      return bit_lattice::U;
   }
}

std::deque<bit_lattice> BitLatticeManipulator::inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b, const size_t out_type_size, const bool out_is_signed, const bool out_is_bool)
{
   THROW_ASSERT(not(a.empty() and b.empty()), "a.size() = " + STR(a.size()) + " b.size() = " + STR(b.size()));
   THROW_ASSERT(out_type_size, "");
   THROW_ASSERT(not out_is_bool or (out_type_size == 1), "boolean with type size != 1");
   std::deque<bit_lattice> res;
   if(out_is_bool)
   {
      res.push_back(bit_inf(a.back(), b.back()));
      return res;
   }

   std::deque<bit_lattice> a_copy = a;
   std::deque<bit_lattice> b_copy = b;
   sign_reduce_bitstring(a_copy, out_is_signed);
   sign_reduce_bitstring(b_copy, out_is_signed);

   // a_tmp is the longer bistring
   std::deque<bit_lattice> a_tmp = (a_copy.size() >= b_copy.size()) ? a_copy : b_copy;
   std::deque<bit_lattice> b_tmp = (a_copy.size() >= b_copy.size()) ? b_copy : a_copy;

   if(a_tmp.size() > b_tmp.size())
   {
      b_tmp = sign_extend_bitstring(b_tmp, out_is_signed, a_tmp.size());
   }

   auto a_it = a_tmp.crbegin();
   auto b_it = b_tmp.crbegin();
   const auto a_end = a_tmp.crend();
   const auto b_end = b_tmp.crend();
   for(; a_it != a_end and b_it != b_end; a_it++, b_it++)
   {
      res.push_front(bit_inf(*a_it, *b_it));
   }

   if(res.empty())
   {
      res.push_front(bit_lattice::X);
   }

   sign_reduce_bitstring(res, out_is_signed);

   while(res.size() > out_type_size)
   {
      res.pop_front();
   }
   return res;
}

std::deque<bit_lattice> BitLatticeManipulator::inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b, const unsigned int output_uid) const
{
   THROW_ASSERT(not(a.empty() and b.empty()), "a.size() = " + STR(a.size()) + " b.size() = " + STR(b.size()));

   const tree_nodeConstRef node = TM->get_tree_node_const(output_uid);
   const auto kind = node->get_kind();
   unsigned int out_type_size = 0;
   if(kind == ssa_name_K or kind == integer_cst_K or kind == real_cst_K)
   {
      out_type_size = size(TM, tree_helper::get_type_index(TM, output_uid));
   }
   else if(kind == function_decl_K)
   {
      const tree_nodeConstRef fu_type = tree_helper::CGetType(node);
      THROW_ASSERT(fu_type->get_kind() == function_type_K, "node " + STR(output_uid) + " is " + fu_type->get_kind_text());
      const auto* ft = GetPointer<const function_type>(fu_type);
      out_type_size = Size(GET_NODE(ft->retn));
   }
   else if(kind == parm_decl_K)
   {
      out_type_size = Size(tree_helper::CGetType(node));
   }
   else
   {
      THROW_UNREACHABLE("unexpected sup for output_uid " + STR(output_uid) + " of kind " + node->get_kind_text());
   }
   THROW_ASSERT(out_type_size, "");
   const bool out_is_bool = tree_helper::is_bool(TM, output_uid);
   THROW_ASSERT(not out_is_bool or (out_type_size == 1), "boolean with type size != 1");
   bool out_is_signed = signed_var.find(output_uid) != signed_var.end() or tree_helper::is_int(TM, output_uid);

   return inf(a, b, out_type_size, out_is_signed, out_is_bool);
}

void BitLatticeManipulator::sign_reduce_bitstring(std::deque<bit_lattice>& bitstring, bool bitstring_is_signed)
{
   THROW_ASSERT(not bitstring.empty(), "");
   while(bitstring.size() > 1)
   {
      if(bitstring_is_signed)
      {
         if(bitstring.at(0) != bit_lattice::U and bitstring.at(0) == bitstring.at(1))
         {
            bitstring.pop_front();
         }
         else
         {
            break;
         }
      }
      else
      {
         if((bitstring.at(0) == bit_lattice::X and bitstring.at(1) == bit_lattice::X) or (bitstring.at(0) == bit_lattice::ZERO and bitstring.at(1) != bit_lattice::X))
         {
            bitstring.pop_front();
         }
         else
         {
            break;
         }
      }
   }
}

std::deque<bit_lattice> BitLatticeManipulator::sign_extend_bitstring(const std::deque<bit_lattice>& bitstring, bool bitstring_is_signed, size_t final_size)
{
   THROW_ASSERT(final_size, "cannot sign extend a bitstring to final_size 0");
   THROW_ASSERT(final_size > bitstring.size(), "useless sign extension");
   std::deque<bit_lattice> res = bitstring;
   if(res.empty())
   {
      res.push_front(bit_lattice::X);
   }
   const auto sign_bit = (bitstring_is_signed or res.front() == bit_lattice::X) ? res.front() : bit_lattice::ZERO;
   while(res.size() < final_size)
   {
      res.push_front(sign_bit);
   }
   return res;
}

std::deque<bit_lattice> BitLatticeManipulator::constructor_bitstring(const tree_nodeRef& ctor_tn, unsigned int ssa_node_id) const
{
   const bool ssa_is_signed = tree_helper::is_int(TM, ssa_node_id);
   THROW_ASSERT(ctor_tn->get_kind() == constructor_K, "ctor_tn is not constructor node");
   auto* c = GetPointer<constructor>(ctor_tn);
   std::vector<unsigned int> array_dims;
   unsigned int elements_bitsize;
   tree_helper::get_array_dim_and_bitsize(TM, GET_INDEX_CONST_NODE(c->type), array_dims, elements_bitsize);
   unsigned int initialized_elements = 0;
   std::deque<bit_lattice> current_inf;
   current_inf.push_back(bit_lattice::X);
   std::deque<bit_lattice> cur_bitstring;
   for(const auto& i : c->list_of_idx_valu)
   {
      tree_nodeRef el = GET_NODE(i.second);
      THROW_ASSERT(el, "unexpected condition");

      if(el->get_kind() == integer_cst_K)
      {
         cur_bitstring = create_bitstring_from_constant(GetPointer<integer_cst>(el)->value, elements_bitsize, ssa_is_signed);
      }
      else if(el->get_kind() == real_cst_K)
      {
         THROW_ASSERT(elements_bitsize == 64 || elements_bitsize == 32, "Unhandled real type size (" + STR(elements_bitsize) + ")");
         auto* real_const = GetPointer<real_cst>(el);
         if(real_const->valx.front() == '-' && real_const->valr.front() != real_const->valx.front())
         {
            cur_bitstring = string_to_bitstring(convert_fp_to_string("-" + real_const->valr, elements_bitsize));
         }
         else
         {
            cur_bitstring = string_to_bitstring(convert_fp_to_string(real_const->valr, elements_bitsize));
         }
         sign_reduce_bitstring(cur_bitstring, ssa_is_signed);
      }
      else if(el->get_kind() == constructor_K && GetPointer<array_type>(GET_CONST_NODE(GetPointer<constructor>(el)->type)))
      {
         THROW_ASSERT(array_dims.size() > 1 || GET_NODE(c->type)->get_kind() == record_type_K, "invalid nested constructors:" + ctor_tn->ToString() + " " + STR(array_dims.size()));
         cur_bitstring = constructor_bitstring(el, ssa_node_id);
      }
      else
      {
         cur_bitstring = create_u_bitstring(elements_bitsize);
      }

      current_inf = inf(current_inf, cur_bitstring, ssa_node_id);
      initialized_elements++;
   }
   if(initialized_elements < array_dims.front())
   {
      current_inf = inf(current_inf, create_bitstring_from_constant(0, elements_bitsize, ssa_is_signed), ssa_node_id);
   }
   return current_inf;
}

std::deque<bit_lattice> BitLatticeManipulator::string_cst_bitstring(const tree_nodeRef& strcst_tn, unsigned int ssa_node_id) const
{
   THROW_ASSERT(strcst_tn->get_kind() == string_cst_K, "strcst_tn is not a string_cst node");
   auto* sc = GetPointer<string_cst>(strcst_tn);
   const std::string str = sc->strg;
   const bool node_is_signed = tree_helper::is_int(TM, ssa_node_id);
   auto current_inf = create_bitstring_from_constant(0, 8, node_is_signed);
   for(const auto c : str)
   {
      auto current_el = create_bitstring_from_constant(static_cast<long long int>(c), 8, node_is_signed);
      current_inf = inf(current_inf, current_el, ssa_node_id);
   }
   return current_inf;
}

bool BitLatticeManipulator::is_handled_by_bitvalue(unsigned int type_id) const
{
   return not tree_helper::is_a_complex(TM, type_id) and not tree_helper::is_a_vector(TM, type_id) and not tree_helper::is_a_struct(TM, type_id);
}

bool BitLatticeManipulator::mix()
{
   bool updated = false;

   for(auto& b : best)
   {
      const auto c = current.find(b.first);
      if(c != current.end())
      {
         const auto cur_lattice = c->second;
         const auto best_lattice = b.second;
         const auto sup_lattice = sup(cur_lattice, best_lattice, b.first);
         if(best_lattice != sup_lattice)
         {
            b.second = sup_lattice;
            updated = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, bl_debug_level, "Changes in " + STR(b.first) + " Cur is " + bitstring_to_string(cur_lattice) + " Best is " + bitstring_to_string(best_lattice) + " Sup is " + bitstring_to_string(sup_lattice));
         }
      }
   }
   return updated;
}

bool BitLatticeManipulator::update_current(std::deque<bit_lattice>& res, unsigned int output_uid)
{
   if(not res.empty())
   {
      const auto out_is_signed = signed_var.find(output_uid) != signed_var.end() or tree_helper::is_int(TM, output_uid);
      sign_reduce_bitstring(res, out_is_signed);

      auto& cur_lattice = current.at(output_uid);
      const auto best_lattice = best.at(output_uid);
      auto sup_lattice = bitstring_constant(res) ? res : sup(res, best_lattice, output_uid);
      if(cur_lattice != sup_lattice)
      {
         cur_lattice = sup_lattice;
         return true;
      }
   }
   return false;
}

void BitLatticeManipulator::clear()
{
   current.clear();
   best.clear();
   signed_var.clear();
}

std::deque<bit_lattice> create_u_bitstring(size_t lenght)
{
   return std::deque<bit_lattice>(lenght, bit_lattice::U);
}

std::deque<bit_lattice> create_x_bitstring(unsigned int lenght)
{
   return std::deque<bit_lattice>(lenght, bit_lattice::X);
}

std::deque<bit_lattice> create_bitstring_from_constant(long long int value_int, unsigned int len, bool signed_value)
{
   auto value = static_cast<long long unsigned int>(value_int);
   std::deque<bit_lattice> res;
   if(value == 0)
   {
      res.push_front(bit_lattice::ZERO);
      return res;
   }
   unsigned bit;
   for(bit = 0; bit < len and value > 0; bit++, value /= 2)
   {
      if(value % 2)
      {
         res.push_front(bit_lattice::ONE);
      }
      else
      {
         res.push_front(bit_lattice::ZERO);
      }
   }
   if(bit < len and signed_value)
   {
      res.push_front(bit_lattice::ZERO);
   }
   return res;
}

std::string bitstring_to_string(const std::deque<bit_lattice>& bitstring)
{
   std::string res;
   for(const auto bit : bitstring)
   {
      switch(bit)
      {
         case(bit_lattice::U):
            res.push_back('U');
            break;
         case(bit_lattice::X):
            res.push_back('X');
            break;
         case(bit_lattice::ZERO):
            res.push_back('0');
            break;
         case(bit_lattice::ONE):
            res.push_back('1');
            break;
         default:
            THROW_ERROR("unexpected value in bit_lattice");
            break;
      }
   }
   return res;
}

std::deque<bit_lattice> string_to_bitstring(const std::string& s)
{
   std::deque<bit_lattice> res;
   for(const auto bit : s)
   {
      switch(bit)
      {
         case('U'):
            res.push_back(bit_lattice::U);
            break;
         case('X'):
            res.push_back(bit_lattice::X);
            break;
         case('0'):
            res.push_back(bit_lattice::ZERO);
            break;
         case('1'):
            res.push_back(bit_lattice::ONE);
            break;
         default:
            THROW_ERROR(std::string("unexpected char in bitvalue string: ") + bit);
            break;
      }
   }
   return res;
}

bool bitstring_constant(const std::deque<bit_lattice>& a)
{
   bool res = true;
   for(const auto i : a)
   {
      if(i == bit_lattice::U || i == bit_lattice::X)
      {
         res = false;
         break;
      }
   }
   return res;
}

unsigned int BitLatticeManipulator::size(const tree_managerConstRef tm, unsigned int index)
{
   return BitLatticeManipulator::Size(tm->get_tree_node_const(index));
}

unsigned int BitLatticeManipulator::Size(const tree_nodeConstRef t)
{
   unsigned int return_value = 0;
   switch(t->get_kind())
   {
      case tree_reindex_K:
      {
         return_value = Size(GET_CONST_NODE(t));
         break;
      }
      case var_decl_K:
      {
         THROW_ASSERT(GetPointer<const var_decl>(t)->type, "expected a var decl type");
         return_value = Size(GET_NODE(GetPointer<const var_decl>(t)->type));
         break;
      }
      case parm_decl_K:
      {
         THROW_ASSERT(GetPointer<const parm_decl>(t)->type, "expected a var decl type");
         return_value = Size(GET_NODE(GetPointer<const parm_decl>(t)->type));
         break;
      }
      case result_decl_K:
      {
         THROW_ASSERT(GetPointer<const result_decl>(t)->type, "expected a var decl type");
         return_value = Size(GET_NODE(GetPointer<const result_decl>(t)->type));
         break;
      }
      case ssa_name_K:
      {
         const auto* sa = GetPointer<const ssa_name>(t);
         THROW_ASSERT(GetPointer<const ssa_name>(t), "Expected an ssa_name");
         if(!sa->bit_values.empty())
         {
            if(GET_NODE(sa->type)->get_kind() == real_type_K)
            {
               return Size(GET_NODE(sa->type));
            }
            return_value = static_cast<unsigned int>(sa->bit_values.size());
            break;
         }
         else if(sa->min && sa->max && GET_NODE(sa->min)->get_kind() == integer_cst_K && GET_NODE(sa->max)->get_kind() == integer_cst_K)
         {
            const tree_nodeRef type = sa->var ? GET_NODE(GetPointer<decl_node>(GET_NODE(sa->var))->type) : GET_NODE(sa->type);
            if(type->get_kind() != integer_type_K && type->get_kind() != enumeral_type_K)
            {
               return_value = Size(type);
               break;
            }
            long long max = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(sa->max)));
            long long min = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(sa->min)));
            if(min == max) /// It may happen with GCC8 -O0
            {
               return_value = Size(type);
               break;
            }

            long long min_it;
            long long max_it;
            bool unsigned_p;
            if(type->get_kind() == integer_type_K)
            {
               const integer_type* it = GetPointer<integer_type>(type);
               min_it = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->min)));
               max_it = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->max)));
               unsigned_p = it->unsigned_flag;
            }
            else
            {
               const enumeral_type* it = GetPointer<enumeral_type>(type);
               min_it = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->min)));
               max_it = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->max)));
               unsigned_p = it->unsigned_flag;
            }
            if((min_it == min and min != 0) or max_it == max)
            {
               return_value = sa->var ? Size(GET_NODE(sa->var)) : Size(GET_NODE(sa->type));
               break;
            }
            if(unsigned_p)
            {
               return_value = (64 - static_cast<unsigned>(__builtin_clzll(static_cast<unsigned long long>(max))));
            }
            else
            {
               if(max == -1 || max == 0)
               {
                  return_value = 1;
               }
               else if(max < -1)
               {
                  return_value = 65u - static_cast<unsigned>(__builtin_clzll(~static_cast<unsigned long long>(max)));
               }
               else
               {
                  return_value = 65u - static_cast<unsigned>(__builtin_clzll(static_cast<unsigned long long>(max)));
               }
               if(min < -1)
               {
                  unsigned minbits = 65u - static_cast<unsigned>(__builtin_clzll(~static_cast<unsigned long long>(min)));
                  return_value = std::max(return_value, minbits);
               }
               else if(min == -1)
               {
                  return_value = std::max(return_value, 1u);
               }
            }
         }
         else if(sa->var)
         {
            return_value = Size(GET_NODE(sa->var));
         }
         else
         {
            return_value = Size(GET_NODE(sa->type));
         }
         break;
      }
      case field_decl_K:
      {
         THROW_ASSERT(GetPointer<const field_decl>(t)->type, "expected a field decl type");
         return_value = Size(GET_NODE(GetPointer<const field_decl>(t)->type));
         break;
      }
      case pointer_type_K:
      {
         const auto* ic = GetPointer<const integer_cst>(GET_NODE(GetPointer<const pointer_type>(t)->size));
         return_value = static_cast<unsigned int>(tree_helper::get_integer_cst_value(ic));
         break;
      }
      case reference_type_K:
      {
         const auto* ic = GetPointer<const integer_cst>(GET_NODE(GetPointer<const reference_type>(t)->size));
         return_value = static_cast<unsigned int>(tree_helper::get_integer_cst_value(ic));
         break;
      }
      case array_type_K:
      {
         const auto* at = GetPointer<const array_type>(t);
         if(at->size)
         {
            const auto* ic = GetPointer<const integer_cst>(GET_NODE(at->size));
            if(ic)
            {
               return_value = static_cast<unsigned int>(tree_helper::get_integer_cst_value(ic));
            }
            else
            {
               return_value = 32;
            }
         }
         else
         {
            return_value = 0;
         }
         break;
      }

      case boolean_type_K:
      {
         return_value = 1;
         break;
      }
      case void_type_K:
      {
         return_value = 32;
         break;
      }
      case enumeral_type_K:
      {
         const auto* et = GetPointer<const enumeral_type>(t);
         if(et->min && et->max && GET_NODE(et->min)->get_kind() == integer_cst_K && GET_NODE(et->max)->get_kind() == integer_cst_K)
         {
            long long max = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(et->max)));
            long long min = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(et->min)));
            if(et->unsigned_flag)
            {
               return_value = 64u - static_cast<unsigned>(__builtin_clzll(static_cast<unsigned long long>(max)));
            }
            else
            {
               if(max == -1 || max == 0)
               {
                  return_value = 1;
               }
               else if(max < -1)
               {
                  return_value = 65u - static_cast<unsigned>(__builtin_clzll(~static_cast<unsigned long long>(max)));
               }
               else
               {
                  return_value = 65u - static_cast<unsigned>(__builtin_clzll(static_cast<unsigned long long>(max)));
               }
               if(min < -1)
               {
                  unsigned minbits = 65u - static_cast<unsigned>(__builtin_clzll(~static_cast<unsigned long long>(min)));
                  return_value = std::max(return_value, minbits);
               }
               else if(min == -1)
               {
                  return_value = std::max(return_value, 1u);
               }
            }
         }
         else
         {
            if(!GetPointer<const type_node>(t)->size)
            {
               return_value = 0;
               break;
            }
            const auto* ic = GetPointer<const integer_cst>(GET_NODE(GetPointer<const type_node>(t)->size));
            return_value = static_cast<unsigned int>(tree_helper::get_integer_cst_value(ic));
         }
         break;
      }
      case vector_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case type_argument_pack_K:
      case real_type_K:
      case complex_type_K:
      case function_type_K:
      case method_type_K:
      case union_type_K:
      case record_type_K:
      {
         if(!GetPointer<const type_node>(t)->size)
         {
            return_value = 0;
            break;
         }
         const auto* ic = GetPointer<const integer_cst>(GET_NODE(GetPointer<const type_node>(t)->size));
         return_value = static_cast<unsigned int>(tree_helper::get_integer_cst_value(ic));
         break;
      }
      case integer_type_K:
      {
         const auto* it = GetPointer<const integer_type>(t);
         unsigned int prec = it->prec;
         unsigned int algn = it->algn;
         if(prec != algn && prec % algn)
         {
            return_value = prec;
         }
         else
         {
            auto* ic = GetPointer<integer_cst>(GET_NODE(it->size));
            return_value = static_cast<unsigned int>(tree_helper::get_integer_cst_value(ic));
         }
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto* ce = GetPointer<const call_expr>(t);
         return_value = Size(GET_NODE(ce->type));
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto* ue = GetPointer<const unary_expr>(t);
         THROW_ASSERT(ue->type, "Expected an unary expr with type");
         return_value = Size(GET_NODE(ue->type));
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto* be = GetPointer<const binary_expr>(t);
         return_value = Size(GET_NODE(be->type));
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto* te = GetPointer<const ternary_expr>(t);
         return_value = Size(GET_NODE(te->type));
         break;
      }
      case lut_expr_K:
      {
         return_value = 1;
         break;
      }
      case array_ref_K:
      {
         const auto* ar = GetPointer<const array_ref>(t);
         return_value = Size(GET_NODE(ar->type));
         break;
      }
      case string_cst_K:
      {
         const auto* sc = GetPointer<const string_cst>(t);
         return_value = Size(GET_NODE(sc->type));
         break;
      }
      case vector_cst_K:
      {
         const auto* vc = GetPointer<const vector_cst>(t);
         return_value = Size(GET_NODE(vc->type));
         break;
      }
      case integer_cst_K:
      {
         const auto* ic = GetPointer<const integer_cst>(t);
         tree_nodeRef Type = GET_NODE(ic->type);
         auto ic_value = static_cast<long long unsigned int>(ic->value);
         if(ic_value == 0)
         {
            return_value = 1;
         }
         else
         {
            return_value = Size(Type);
            bool is_integer_type = (GetPointer<enumeral_type>(Type) && !GetPointer<enumeral_type>(Type)->unsigned_flag) || (GetPointer<integer_type>(Type) && !GetPointer<integer_type>(Type)->unsigned_flag);
            unsigned int counter = 0;
            while(ic_value > 0)
            {
               ic_value /= 2;
               ++counter;
               if(counter >= return_value)
               {
                  break;
               }
            }
            if(counter < return_value && is_integer_type)
            {
               return_value = counter + 1;
            }
            else if(counter == return_value && is_integer_type && ic->value < 0)
            {
               /// count leading ONEs
               unsigned int index = return_value - 1;
               while(((1ULL << index) & static_cast<long long unsigned int>(ic->value)) && index > 0)
               {
                  --counter;
                  --index;
               }
               ++counter;
               return_value = counter;
            }
            else
            {
               return_value = counter;
            }
         }
         break;
      }
      case real_cst_K:
      {
         const auto* rc = GetPointer<const real_cst>(t);
         return_value = Size(GET_NODE(rc->type));
         break;
      }
      case complex_cst_K:
      {
         const auto* cc = GetPointer<const complex_cst>(t);
         return_value = Size(GET_NODE(cc->type));
         break;
      }
      case constructor_K:
      {
         const auto* c = GetPointer<const constructor>(t);
         return_value = Size(GET_NODE(c->type));
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointer<const target_mem_ref461>(t);
         return_value = Size(GET_NODE(tmr->type));
         break;
      }
      case gimple_call_K:
      case function_decl_K:
      {
         return_value = 32; // static_cast<unsigned int>(GccWrapper::CGetPointerSize(parameters));
         break;
      }
      case array_range_ref_K:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case const_decl_K:
      case identifier_node_K:
      case label_decl_K:
      case lang_type_K:
      case namespace_decl_K:
      case offset_type_K:
      case qual_union_type_K:
      case set_type_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case template_type_parm_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case typename_type_K:
      case CASE_CPP_NODES:
      case last_tree_K:
      case none_K:
      case placeholder_expr_K:
      case gimple_asm_K:
      case gimple_assign_K:
      case gimple_bind_K:
      case gimple_cond_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_multi_way_if_K:
      case gimple_nop_K:
      case gimple_phi_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_return_K:
      case gimple_switch_K:
      case gimple_while_K:
      case CASE_PRAGMA_NODES:
      case void_cst_K:
      case error_mark_K:
      default:
      {
         THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
      }
   }
   return return_value;
}