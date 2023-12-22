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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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

BitLatticeManipulator::BitLatticeManipulator(const tree_managerConstRef _TM, const int _bl_debug_level)
    : TM(_TM), bl_debug_level(_bl_debug_level)
{
}

BitLatticeManipulator::~BitLatticeManipulator() = default;

bool BitLatticeManipulator::IsSignedIntegerType(const tree_nodeConstRef& tn) const
{
   return signed_var.count(tn->index) || tree_helper::IsSignedIntegerType(tn);
}

bit_lattice BitLatticeManipulator::bit_sup(const bit_lattice a, const bit_lattice b)
{
   if(a == b)
   {
      return a;
   }
   else if(a == bit_lattice::X || b == bit_lattice::X)
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

std::deque<bit_lattice> BitLatticeManipulator::sup(const std::deque<bit_lattice>& _a, const std::deque<bit_lattice>& _b,
                                                   const size_t out_type_size, const bool out_is_signed,
                                                   const bool out_is_bool)
{
   THROW_ASSERT(!_a.empty() && !_b.empty(), "a = " + std::string(_a.empty() ? "empty" : bitstring_to_string(_a)) +
                                                ", b = " + (_b.empty() ? "empty" : bitstring_to_string(_b)));
   THROW_ASSERT(out_type_size, "Size can not be zero");
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
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
   for(; a_it != a_end && b_it != b_end; a_it++, b_it++)
   {
      res.push_front(bit_sup(*a_it, *b_it));
   }

   if(res.empty())
   {
      res.push_front(bit_lattice::X);
   }
   sign_reduce_bitstring(res, out_is_signed);
   size_t final_size;
   const bool a_sign_is_x = _a.front() == bit_lattice::X;
   const bool b_sign_is_x = _b.front() == bit_lattice::X;
   auto sign_bit = res.front();
   if(out_is_signed)
   {
      final_size =
          std::min(out_type_size,
                   ((a_sign_is_x == b_sign_is_x ||
                     ((_a.front() == bit_lattice::ZERO || _a.front() == bit_lattice::ONE) && _a.size() < _b.size()) ||
                     ((_b.front() == bit_lattice::ZERO || _b.front() == bit_lattice::ONE) && _a.size() > _b.size())) ?
                        (std::min(_a.size(), _b.size())) :
                        (a_sign_is_x ? _a.size() : _b.size())));
      THROW_ASSERT(final_size, "final size of sup cannot be 0");
   }
   else
   {
      final_size = std::min(out_type_size, (a_sign_is_x == b_sign_is_x) ?
                                               std::min(_a.size(), _b.size()) :
                                               (a_sign_is_x ? (_a.size() < _b.size() ? _a.size() : 1 + _b.size()) :
                                                              (_a.size() < _b.size() ? 1 + _a.size() : _b.size())));
      sign_bit = (a_sign_is_x == b_sign_is_x) ? sign_bit :
                                                (a_sign_is_x ? (_a.size() < _b.size() ? sign_bit : bit_lattice::ZERO) :
                                                               (_a.size() < _b.size() ? bit_lattice::ZERO : sign_bit));
      THROW_ASSERT(final_size, "final size of sup cannot be 0");
   }
   while(res.size() > final_size)
   {
      res.pop_front();
   }
   if(sign_bit != bit_lattice::X && res.front() == bit_lattice::X)
   {
      res.pop_front();
      res.push_front(sign_bit);
   }
   if(!out_is_signed)
   {
      sign_reduce_bitstring(res, out_is_signed);
   }

   return res;
}

std::deque<bit_lattice> BitLatticeManipulator::sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const unsigned int output_uid) const
{
   return sup(a, b, TM->CGetTreeNode(output_uid));
}

std::deque<bit_lattice> BitLatticeManipulator::sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const tree_nodeConstRef& out_node) const
{
   THROW_ASSERT(!a.empty() && !b.empty(), "a.size() = " + STR(a.size()) + " b.size() = " + STR(b.size()));

   // extend the shorter of the two bitstrings
   // compute the underlying type size
   const auto kind =
       out_node->get_kind() == tree_reindex_K ? GET_CONST_NODE(out_node)->get_kind() : out_node->get_kind();
   const auto node_type = tree_helper::CGetType(out_node);
   size_t out_type_size = 0;
   if(kind == ssa_name_K || kind == parm_decl_K || kind == integer_cst_K)
   {
      out_type_size = static_cast<size_t>(Size(node_type));
   }
   else if(kind == function_decl_K)
   {
      THROW_ASSERT(GET_CONST_NODE(node_type)->get_kind() == function_type_K ||
                       GET_CONST_NODE(node_type)->get_kind() == method_type_K,
                   "node " + STR(out_node) + " is " + node_type->get_kind_text());
      const auto ft = GetPointerS<const function_type>(GET_CONST_NODE(node_type));
      out_type_size = static_cast<size_t>(Size(ft->retn));
   }
   else
   {
      THROW_UNREACHABLE("unexpected sup for output_uid " + STR(out_node) + " of kind " + tree_node::GetString(kind));
   }
   THROW_ASSERT(out_type_size, "");
   const auto out_is_bool = tree_helper::IsBooleanType(node_type);
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
   const auto out_is_signed = IsSignedIntegerType(out_node);
   return sup(a, b, out_type_size, out_is_signed, out_is_bool);
}

bit_lattice BitLatticeManipulator::bit_inf(const bit_lattice a, const bit_lattice b)
{
   if(a == b)
   {
      return a;
   }
   else if(a == bit_lattice::U || b == bit_lattice::U)
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

std::deque<bit_lattice> BitLatticeManipulator::inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const size_t out_type_size, const bool out_is_signed,
                                                   const bool out_is_bool)
{
   THROW_ASSERT(!(a.empty() && b.empty()), "a.size() = " + STR(a.size()) + " b.size() = " + STR(b.size()));
   THROW_ASSERT(out_type_size, "");
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
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
   for(; a_it != a_end && b_it != b_end; a_it++, b_it++)
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

std::deque<bit_lattice> BitLatticeManipulator::inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const unsigned int output_uid) const
{
   return inf(a, b, TM->CGetTreeNode(output_uid));
}

std::deque<bit_lattice> BitLatticeManipulator::inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const tree_nodeConstRef& out_node) const
{
   THROW_ASSERT(!(a.empty() && b.empty()), "a.size() = " + STR(a.size()) + " b.size() = " + STR(b.size()));

   const auto kind =
       out_node->get_kind() == tree_reindex_K ? GET_CONST_NODE(out_node)->get_kind() : out_node->get_kind();
   const auto node_type = tree_helper::CGetType(out_node);
   size_t out_type_size = 0;
   if(kind == ssa_name_K || kind == parm_decl_K || kind == integer_cst_K)
   {
      out_type_size = static_cast<size_t>(Size(node_type));
   }
   else if(kind == function_decl_K)
   {
      THROW_ASSERT(GET_CONST_NODE(node_type)->get_kind() == function_type_K ||
                       GET_CONST_NODE(node_type)->get_kind() == method_type_K,
                   "node " + STR(out_node) + " is " + node_type->get_kind_text());
      const auto ft = GetPointerS<const function_type>(GET_CONST_NODE(node_type));
      out_type_size = static_cast<size_t>(Size(ft->retn));
   }
   else
   {
      THROW_UNREACHABLE("unexpected sup for " + STR(out_node) + " of kind " + tree_node::GetString(kind));
   }
   THROW_ASSERT(out_type_size, "");
   const auto out_is_bool = tree_helper::IsBooleanType(node_type);
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
   const auto out_is_signed = IsSignedIntegerType(out_node);
   return inf(a, b, out_type_size, out_is_signed, out_is_bool);
}

/// function slightly different than tree_helper.cpp: sign_reduce_bitstring
void BitLatticeManipulator::sign_reduce_bitstring(std::deque<bit_lattice>& bitstring, bool bitstring_is_signed)
{
   THROW_ASSERT(!bitstring.empty(), "");
   while(bitstring.size() > 1)
   {
      if(bitstring_is_signed)
      {
         if(bitstring.at(0) != bit_lattice::U && bitstring.at(0) == bitstring.at(1))
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
         if((bitstring.at(0) == bit_lattice::X && bitstring.at(1) == bit_lattice::X) ||
            (bitstring.at(0) == bit_lattice::ZERO && bitstring.at(1) != bit_lattice::X))
         {
            bitstring.pop_front();
         }
         else if(bitstring.at(0) == bit_lattice::ZERO && bitstring.at(1) == bit_lattice::X)
         {
            bitstring.pop_front();
            bitstring.pop_front();
            bitstring.push_front(bit_lattice::ZERO);
         }
         else
         {
            break;
         }
      }
   }
}

std::deque<bit_lattice> BitLatticeManipulator::sign_extend_bitstring(const std::deque<bit_lattice>& bitstring,
                                                                     bool bitstring_is_signed, size_t final_size)
{
   THROW_ASSERT(final_size, "cannot sign extend a bitstring to final_size 0");
   THROW_ASSERT(final_size > bitstring.size(), "useless sign extension");
   std::deque<bit_lattice> res = bitstring;
   if(res.empty())
   {
      res.push_front(bit_lattice::X);
   }
   const auto sign_bit = (bitstring_is_signed || res.front() == bit_lattice::X) ? res.front() : bit_lattice::ZERO;
   while(res.size() < final_size)
   {
      res.push_front(sign_bit);
   }
   return res;
}

std::deque<bit_lattice> BitLatticeManipulator::constructor_bitstring(const tree_nodeRef& ctor_tn,
                                                                     unsigned int ssa_node_id) const
{
   const bool ssa_is_signed = tree_helper::is_int(TM, ssa_node_id);
   THROW_ASSERT(ctor_tn->get_kind() == constructor_K, "ctor_tn is not constructor node");
   auto* c = GetPointerS<const constructor>(ctor_tn);
   std::vector<unsigned long long> array_dims;
   unsigned long long elements_bitsize;
   tree_helper::get_array_dim_and_bitsize(TM, GET_INDEX_CONST_NODE(c->type), array_dims, elements_bitsize);
   unsigned int initialized_elements = 0;
   std::deque<bit_lattice> current_inf;
   current_inf.push_back(bit_lattice::X);
   std::deque<bit_lattice> cur_bitstring;
   for(const auto& i : c->list_of_idx_valu)
   {
      const auto el = GET_CONST_NODE(i.second);
      THROW_ASSERT(el, "unexpected condition");

      if(el->get_kind() == integer_cst_K)
      {
         cur_bitstring =
             create_bitstring_from_constant(tree_helper::GetConstValue(i.second), elements_bitsize, ssa_is_signed);
      }
      else if(el->get_kind() == real_cst_K)
      {
         THROW_ASSERT(elements_bitsize == 64 || elements_bitsize == 32,
                      "Unhandled real type size (" + STR(elements_bitsize) + ")");
         const auto real_const = GetPointerS<const real_cst>(el);
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
      else if(el->get_kind() == constructor_K &&
              GetPointer<const array_type>(GET_CONST_NODE(GetPointerS<const constructor>(el)->type)))
      {
         THROW_ASSERT(array_dims.size() > 1 || GET_NODE(c->type)->get_kind() == record_type_K,
                      "invalid nested constructors:" + ctor_tn->ToString() + " " + STR(array_dims.size()));
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

std::deque<bit_lattice> BitLatticeManipulator::string_cst_bitstring(const tree_nodeRef& strcst_tn,
                                                                    unsigned int ssa_node_id) const
{
   THROW_ASSERT(strcst_tn->get_kind() == string_cst_K, "strcst_tn is not a string_cst node");
   auto* sc = GetPointerS<string_cst>(strcst_tn);
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

bool BitLatticeManipulator::IsHandledByBitvalue(const tree_nodeConstRef& tn) const
{
   const auto type = tree_helper::CGetType(tn);
   return !(tree_helper::IsRealType(type) || tree_helper::IsComplexType(type) || tree_helper::IsVectorType(type) ||
            tree_helper::IsStructType(type));
}

bool BitLatticeManipulator::mix()
{
   auto updated = false;
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
#ifndef NDEBUG
            const auto tn = TM->CGetTreeNode(b.first);
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, bl_debug_level,
                           "Changes in " +
                               STR(tn->get_kind() == function_decl_K ?
                                       (tree_helper::GetMangledFunctionName(GetPointerS<const function_decl>(tn)) +
                                        " return value") :
                                       STR(tn)) +
                               " Cur is " + bitstring_to_string(cur_lattice) + " Best is " +
                               bitstring_to_string(best_lattice) + " Sup is " + bitstring_to_string(sup_lattice));
         }
      }
   }
   return updated;
}

bool BitLatticeManipulator::update_current(std::deque<bit_lattice>& res, const tree_nodeConstRef& tn)
{
   if(!res.empty())
   {
      const auto out_is_signed = IsSignedIntegerType(tn);
      sign_reduce_bitstring(res, out_is_signed);
      if(out_is_signed && res.front() == bit_lattice::X)
      {
         res.front() = bit_lattice::ZERO;
      }

      THROW_ASSERT(best.count(tn->index), "");
      const auto sup_lattice = bitstring_constant(res) ? res : sup(res, best.at(tn->index), tn);
      auto& cur_lattice = current[tn->index];
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

std::deque<bit_lattice> create_x_bitstring(size_t lenght)
{
   return std::deque<bit_lattice>(lenght, bit_lattice::X);
}

std::deque<bit_lattice> create_bitstring_from_constant(integer_cst_t value, unsigned long long len, bool signed_value)
{
   std::deque<bit_lattice> res;
   if(value == 0)
   {
      res.push_front(bit_lattice::ZERO);
      return res;
   }
   unsigned bit;
   for(bit = 0; bit < len && value != 0; bit++, value >>= 1)
   {
      res.push_front((value & 1) ? bit_lattice::ONE : bit_lattice::ZERO);
   }
   if(bit < len && signed_value)
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

unsigned long long BitLatticeManipulator::size(const tree_managerConstRef tm, unsigned int index)
{
   return BitLatticeManipulator::Size(tm->CGetTreeNode(index));
}

unsigned long long BitLatticeManipulator::Size(const tree_nodeConstRef& t)
{
   switch(t->get_kind())
   {
      case tree_reindex_K:
      {
         return Size(GET_CONST_NODE(t));
      }
      case CASE_DECL_NODES:
      {
         THROW_ASSERT(GetPointerS<const decl_node>(t)->type, "expected a var decl type");
         if(t->get_kind() == function_decl_K)
         {
            return 32u;
         }
         return Size(GET_NODE(GetPointerS<const decl_node>(t)->type));
      }
      case ssa_name_K:
      {
         const auto sa = GetPointerS<const ssa_name>(t);
         THROW_ASSERT(sa->type, "Expected an ssa_name type");
         return Size(GET_CONST_NODE(sa->type));
      }
      case array_type_K:
      {
         const auto at = GetPointerS<const array_type>(t);
         if(at->size)
         {
            if(tree_helper::IsConstant(at->size))
            {
               const auto val = tree_helper::GetConstValue(at->size);
               THROW_ASSERT(val >= 0, "");
               return static_cast<unsigned long long>(val);
            }
            return 32u;
         }
         break;
      }
      case boolean_type_K:
      {
         return 1u;
      }
      case CharType_K:
      case complex_type_K:
      case enumeral_type_K:
      case function_type_K:
      case method_type_K:
      case nullptr_type_K:
      case pointer_type_K:
      case real_type_K:
      case record_type_K:
      case reference_type_K:
      case type_pack_expansion_K:
      case type_argument_pack_K:
      case union_type_K:
      case vector_type_K:
      {
         const auto tn = GetPointer<const type_node>(t);
         THROW_ASSERT(tn->size, "");
         const auto val = tree_helper::GetConstValue(tn->size);
         THROW_ASSERT(val >= 0, "");
         return static_cast<unsigned long long>(val);
      }
      case integer_type_K:
      {
         const auto it = GetPointer<const integer_type>(t);
         if(it->prec != it->algn && it->prec % it->algn)
         {
            return it->prec;
         }
         THROW_ASSERT(it->size, "");
         const auto val = tree_helper::GetConstValue(it->size);
         THROW_ASSERT(val >= 0, "");
         return static_cast<unsigned long long>(val);
      }
      case void_type_K:
      {
         return 32u;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<const call_expr>(t);
         return Size(GET_NODE(ce->type));
      }
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointerS<const expr_node>(t);
         return Size(GET_NODE(te->type));
      }
      case lut_expr_K:
      {
         return 1u;
      }
      case array_ref_K:
      {
         const auto ar = GetPointerS<const array_ref>(t);
         return Size(GET_NODE(ar->type));
      }
      case CASE_CST_NODES:
      {
         const auto sc = GetPointerS<const cst_node>(t);
         return Size(GET_NODE(sc->type));
      }
      case constructor_K:
      {
         const auto c = GetPointerS<const constructor>(t);
         return Size(GET_NODE(c->type));
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref461>(t);
         return Size(GET_NODE(tmr->type));
      }
      case gimple_call_K:
      {
         return 32u;
      }
      case array_range_ref_K:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case identifier_node_K:
      case lang_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case set_type_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case template_type_parm_K:
      case tree_list_K:
      case tree_vec_K:
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
      case error_mark_K:
      default:
      {
         THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
      }
   }
   THROW_ERROR("unexpected pattern");
   return 0u;
}

bool BitLatticeManipulator::isBetter(const std::string& a_string, const std::string& b_string)
{
   auto av = string_to_bitstring(a_string);
   auto bv = string_to_bitstring(b_string);
   auto a_it = av.crbegin();
   auto b_it = bv.crbegin();
   const auto a_end = av.crend();
   const auto b_end = bv.crend();
   for(; a_it != a_end && b_it != b_end; a_it++, b_it++)
   {
      if((*b_it == bit_lattice::U && *a_it != bit_lattice::U) || (*b_it != bit_lattice::X && *a_it == bit_lattice::X))
      {
         return true;
      }
   }
   return a_string.size() < b_string.size();
}
