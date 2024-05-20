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
 * @file BitLatticeManipulator.cpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "BitLatticeManipulator.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "utility.hpp"

#include <algorithm>
#include <string>
#include <vector>

BitLatticeManipulator::BitLatticeManipulator(const tree_managerConstRef _TM, const int _bl_debug_level)
    : TM(_TM), bl_debug_level(_bl_debug_level)
{
}

BitLatticeManipulator::~BitLatticeManipulator() = default;

bool BitLatticeManipulator::IsSignedIntegerType(const tree_nodeConstRef& tn) const
{
   return signed_var.count(tn->index) || tree_helper::IsSignedIntegerType(tn);
}

std::deque<bit_lattice> BitLatticeManipulator::sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const unsigned int output_uid) const
{
   return sup(a, b, TM->GetTreeNode(output_uid));
}

std::deque<bit_lattice> BitLatticeManipulator::sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const tree_nodeConstRef& out_node) const
{
   THROW_ASSERT(!a.empty() && !b.empty(), "a.size() = " + STR(a.size()) + " b.size() = " + STR(b.size()));

   // extend the shorter of the two bitstrings
   // compute the underlying type size
   const auto kind = out_node->get_kind();
   const auto node_type = tree_helper::CGetType(out_node);
   size_t out_type_size = 0;
   if(kind == ssa_name_K || kind == parm_decl_K || kind == integer_cst_K)
   {
      out_type_size = static_cast<size_t>(tree_helper::TypeSize(node_type));
   }
   else if(kind == function_decl_K)
   {
      THROW_ASSERT(node_type->get_kind() == function_type_K || node_type->get_kind() == method_type_K,
                   "node " + STR(out_node) + " is " + node_type->get_kind_text());
      const auto ft = GetPointerS<const function_type>(node_type);
      out_type_size = static_cast<size_t>(tree_helper::TypeSize(ft->retn));
   }
   else
   {
      THROW_UNREACHABLE("unexpected sup for output_uid " + STR(out_node) + " of kind " + tree_node::GetString(kind));
   }
   THROW_ASSERT(out_type_size, "");
   const auto out_is_bool = tree_helper::IsBooleanType(node_type);
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
   const auto out_is_signed = IsSignedIntegerType(out_node);
   return ::sup(a, b, out_type_size, out_is_signed, out_is_bool);
}

std::deque<bit_lattice> BitLatticeManipulator::inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const unsigned int output_uid) const
{
   return inf(a, b, TM->GetTreeNode(output_uid));
}

std::deque<bit_lattice> BitLatticeManipulator::inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const tree_nodeConstRef& out_node) const
{
   THROW_ASSERT(!(a.empty() && b.empty()), "a.size() = " + STR(a.size()) + " b.size() = " + STR(b.size()));

   const auto kind = out_node->get_kind();
   const auto node_type = tree_helper::CGetType(out_node);
   size_t out_type_size = 0;
   if(kind == ssa_name_K || kind == parm_decl_K || kind == integer_cst_K)
   {
      out_type_size = static_cast<size_t>(tree_helper::TypeSize(node_type));
   }
   else if(kind == function_decl_K)
   {
      THROW_ASSERT(node_type->get_kind() == function_type_K || node_type->get_kind() == method_type_K,
                   "node " + STR(out_node) + " is " + node_type->get_kind_text());
      const auto ft = GetPointerS<const function_type>(node_type);
      out_type_size = static_cast<size_t>(tree_helper::TypeSize(ft->retn));
   }
   else
   {
      THROW_UNREACHABLE("unexpected sup for " + STR(out_node) + " of kind " + tree_node::GetString(kind));
   }
   THROW_ASSERT(out_type_size, "");
   const auto out_is_bool = tree_helper::IsBooleanType(node_type);
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
   const auto out_is_signed = IsSignedIntegerType(out_node);
   return ::inf(a, b, out_type_size, out_is_signed, out_is_bool);
}

std::deque<bit_lattice> BitLatticeManipulator::constructor_bitstring(const tree_nodeRef& ctor_tn,
                                                                     unsigned int ssa_node_id) const
{
   const bool ssa_is_signed = tree_helper::is_int(TM, ssa_node_id);
   THROW_ASSERT(ctor_tn->get_kind() == constructor_K, "ctor_tn is not constructor node");
   auto* c = GetPointerS<const constructor>(ctor_tn);
   std::vector<unsigned long long> array_dims;
   unsigned long long elements_bitsize;
   tree_helper::get_array_dim_and_bitsize(TM, c->type->index, array_dims, elements_bitsize);
   unsigned int initialized_elements = 0;
   std::deque<bit_lattice> current_inf;
   current_inf.push_back(bit_lattice::X);
   std::deque<bit_lattice> cur_bitstring;
   for(const auto& i : c->list_of_idx_valu)
   {
      const auto el = i.second;
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
      else if(el->get_kind() == constructor_K && GetPointerS<const constructor>(el)->type->get_kind() == array_type_K)
      {
         THROW_ASSERT(array_dims.size() > 1 || c->type->get_kind() == record_type_K,
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

bool BitLatticeManipulator::IsHandledByBitvalue(const tree_nodeConstRef& tn)
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
            const auto tn = TM->GetTreeNode(b.first);
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
