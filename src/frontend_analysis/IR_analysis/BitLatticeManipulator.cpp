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
 * @file BitLatticeManipulator.cpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "BitLatticeManipulator.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

#include <algorithm>
#include <string>
#include <vector>

BitLatticeManipulator::BitLatticeManipulator(const ir_managerRef& _TM, const int _bl_debug_level)
    : TM(_TM), bl_debug_level(_bl_debug_level)
{
}

bool BitLatticeManipulator::IsSignedIntegerType(const ir_nodeConstRef& tn) const
{
   return signed_var.count(tn->index) || ir_helper::IsSignedIntegerType(tn);
}

std::deque<bit_lattice> BitLatticeManipulator::sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const ir_nodeConstRef& out_node) const
{
   THROW_ASSERT(!a.empty() && !b.empty(), "a.size() = " + STR(a.size()) + " b.size() = " + STR(b.size()));

   // extend the shorter of the two bitstrings
   // compute the underlying type size
   const auto kind = out_node->get_kind();
   const auto node_type = ir_helper::CGetType(out_node);
   size_t out_type_size = 0;
   if(kind == ssa_node_K || kind == argument_val_node_K || kind == constant_int_val_node_K)
   {
      out_type_size = static_cast<size_t>(ir_helper::TypeSize(node_type));
   }
   else if(kind == function_val_node_K)
   {
      THROW_ASSERT(node_type->get_kind() == function_ty_node_K,
                   "node " + STR(out_node) + " is " + node_type->get_kind_text());
      const auto ft = GetPointerS<const function_ty_node>(node_type);
      out_type_size = static_cast<size_t>(ir_helper::TypeSize(ft->retn));
   }
   else
   {
      THROW_UNREACHABLE("unexpected sup for output_uid " + STR(out_node) + " of kind " + ir_node::GetString(kind));
   }
   THROW_ASSERT(out_type_size, "");
   const auto out_is_bool = ir_helper::IsBooleanType(node_type);
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
   const auto out_is_signed = IsSignedIntegerType(out_node);
   return ::sup(a, b, out_type_size, out_is_signed, out_is_bool);
}

std::deque<bit_lattice> BitLatticeManipulator::inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                                                   const ir_nodeConstRef& out_node) const
{
   THROW_ASSERT(!(a.empty() && b.empty()), "a.size() = " + STR(a.size()) + " b.size() = " + STR(b.size()));

   const auto kind = out_node->get_kind();
   const auto node_type = ir_helper::CGetType(out_node);
   size_t out_type_size = 0;
   if(kind == ssa_node_K || kind == argument_val_node_K || kind == constant_int_val_node_K)
   {
      out_type_size = static_cast<size_t>(ir_helper::TypeSize(node_type));
   }
   else if(kind == function_val_node_K)
   {
      THROW_ASSERT(node_type->get_kind() == function_ty_node_K,
                   "node " + STR(out_node) + " is " + node_type->get_kind_text());
      const auto ft = GetPointerS<const function_ty_node>(node_type);
      out_type_size = static_cast<size_t>(ir_helper::TypeSize(ft->retn));
   }
   else
   {
      THROW_UNREACHABLE("unexpected sup for " + STR(out_node) + " of kind " + ir_node::GetString(kind));
   }
   THROW_ASSERT(out_type_size, "");
   const auto out_is_bool = ir_helper::IsBooleanType(node_type);
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
   const auto out_is_signed = IsSignedIntegerType(out_node);
   return ::inf(a, b, out_type_size, out_is_signed, out_is_bool);
}

std::deque<bit_lattice> BitLatticeManipulator::constructor_bitstring(const ir_nodeRef& ctor_tn,
                                                                     const ir_nodeRef& ssa_node,
                                                                     unsigned long long element_size) const
{
   const bool ssa_is_signed = ir_helper::IsSignedIntegerType(ssa_node);
   THROW_ASSERT(ctor_tn->get_kind() == constructor_node_K, "ctor_tn is not constructor_node node");
   auto* c = GetPointerS<const constructor_node>(ctor_tn);
   std::vector<unsigned long long> array_dims;
   unsigned long long elements_bitsize;
   ir_helper::get_array_dim_and_bitsize(TM, c->type->index, array_dims, elements_bitsize);
   unsigned int initialized_elements = 0;
   std::deque<bit_lattice> current_inf;
   current_inf.push_back(bit_lattice::X);
   if(elements_bitsize != element_size)
   {
      return current_inf;
   }
   std::deque<bit_lattice> cur_bitstring;
   for(const auto& i : c->list_of_idx_valu)
   {
      const auto el = i.second;
      THROW_ASSERT(el, "unexpected condition");
      if(i.first->get_kind() == field_val_node_K && GetPointerS<field_val_node>(i.first)->bitfield)
      {
         std::deque<bit_lattice> no_result;
         no_result.push_back(bit_lattice::X);
         return no_result;
      }

      if(el->get_kind() == constant_int_val_node_K)
      {
         cur_bitstring =
             create_bitstring_from_constant(ir_helper::GetConstValue(i.second), elements_bitsize, ssa_is_signed);
      }
      else if(el->get_kind() == constant_fp_val_node_K)
      {
         THROW_ASSERT(elements_bitsize == 64 || elements_bitsize == 32,
                      "Unhandled real type size (" + STR(elements_bitsize) + ")");
         const auto real_const = GetPointerS<const constant_fp_val_node>(el);
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
      else if(el->get_kind() == constructor_node_K &&
              GetPointerS<const constructor_node>(el)->type->get_kind() == array_ty_node_K)
      {
         THROW_ASSERT(array_dims.size() > 1 || c->type->get_kind() == struct_ty_node_K,
                      "invalid nested constructors:" + ctor_tn->ToString() + " " + STR(array_dims.size()));
         cur_bitstring = constructor_bitstring(el, ssa_node, element_size);
      }
      else
      {
         cur_bitstring = create_u_bitstring(elements_bitsize);
      }

      current_inf = inf(current_inf, cur_bitstring, ssa_node);
      initialized_elements++;
   }
   if(initialized_elements < array_dims.front())
   {
      current_inf = inf(current_inf, create_bitstring_from_constant(0, elements_bitsize, ssa_is_signed), ssa_node);
   }
   return current_inf;
}

bool BitLatticeManipulator::IsHandledByBitvalue(const ir_nodeConstRef& tn)
{
   const auto type = ir_helper::CGetType(tn);
   return !(ir_helper::IsRealType(type) || ir_helper::IsVectorType(type) || ir_helper::IsStructType(type));
}

bool BitLatticeManipulator::mix()
{
   auto updated = false;
   for(auto& [idx, best_lattice] : best)
   {
      const auto c = current.find(idx);
      if(c != current.end())
      {
         const auto cur_lattice = c->second;
         const auto tn = TM->GetIRNode(idx);
         const auto sup_lattice = sup(cur_lattice, best_lattice, tn);
         if(best_lattice != sup_lattice)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, bl_debug_level,
                           "Changes in " +
                               STR(tn->get_kind() == function_val_node_K ?
                                       (ir_helper::GetFunctionName(tn) + " return value") :
                                       STR(tn)) +
                               " Cur is " + bitstring_to_string(cur_lattice) + " Best is " +
                               bitstring_to_string(best_lattice) + " Sup is " + bitstring_to_string(sup_lattice));
            best_lattice = sup_lattice;
            updated = true;
         }
      }
   }
   return updated;
}

bool BitLatticeManipulator::update_current(std::deque<bit_lattice>& res, const ir_nodeConstRef& tn)
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
