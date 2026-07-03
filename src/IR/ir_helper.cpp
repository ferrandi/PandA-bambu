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
 * @file ir_helper.cpp
 * @brief This file collects some utility functions.
 *
 *
 * @author Katia Turati <turati@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "ir_helper.hpp"

#include "Range.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <regex>
#include <set>
#include <utility>

int ir_helper::debug_level = 0;

/// function slightly different than sign_reduce_bitstring
static std::string __sign_reduce_bitstring(std::string bitstring, bool bitstring_is_signed)
{
   THROW_ASSERT(!bitstring.empty(), "");
   while(bitstring.size() > 1)
   {
      if(bitstring_is_signed)
      {
         if(bitstring.at(0) != 'U' and (bitstring.at(0) == bitstring.at(1)))
         {
            bitstring = bitstring.substr(1);
         }
         else
         {
            break;
         }
      }
      else
      {
         if((bitstring.at(0) == 'X' && bitstring.at(1) == 'X') || (bitstring.at(0) == '0' && bitstring.at(1) != 'X'))
         {
            bitstring = bitstring.substr(1);
         }
         else if(bitstring.at(0) == '0' && bitstring.at(1) == 'X')
         {
            bitstring = bitstring.substr(1);
            bitstring = bitstring.substr(1);
            bitstring = '0' + bitstring;
         }
         else
         {
            break;
         }
      }
   }
   while(bitstring.at(0) == 'X' && bitstring.size() > 1)
   {
      bitstring = bitstring.substr(1);
   }
   return bitstring;
}

unsigned long long ir_helper::Size(const ir_nodeConstRef& t)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "---Getting size of " + t->get_kind_text() + " " + STR(t->index) + ": " + t->ToString());
   switch(t->get_kind())
   {
      case ssa_node_K:
      {
         const auto sa = GetPointerS<const ssa_node>(t);
         if(sa->bit_values.size())
         {
            if(IsRealType(sa->type))
            {
               return Size(sa->type);
            }
            const auto signed_p = IsSignedIntegerType(sa->type);
            const auto bv_test = __sign_reduce_bitstring(sa->bit_values, signed_p);
            return bv_test.size();
         }
         return sa->var ? Size(sa->var) : Size(sa->type);
      }
      case array_ty_node_K:
      {
         const auto at = GetPointerS<const array_ty_node>(t);
         return static_cast<unsigned long long>(at->bitsizealloc);
      }
      case integer_ty_node_K:
      {
         const auto it = GetPointerS<const integer_ty_node>(t);
         const auto bitsize = it->bitsize;
         const auto algn = it->algn;
         if(bitsize != algn && bitsize % algn)
         {
            return bitsize;
         }
         return static_cast<unsigned long long>(it->bitsizealloc);
      }
      case constant_int_val_node_K:
      {
         const auto is_signed = IsSignedIntegerType(t);
         const auto retval = GetConstValue(t, is_signed).minBitwidth(is_signed);
         return static_cast<unsigned long long>(retval);
      }
      case function_ty_node_K:
      case pointer_ty_node_K:
      case real_ty_node_K:
      case struct_ty_node_K:
      case vector_ty_node_K:
      {
         return GetPointerS<const type_node>(t)->bitsizealloc;
      }
      case CASE_DECL_NODES:
      case CASE_UNARY_NODES:
      case CASE_BINARY_NODES:
      case CASE_TERNARY_NODES:
      case call_node_K:
      case constructor_node_K:
      case assign_stmt_K:
      case call_stmt_K:
      case phi_stmt_K:
      case return_stmt_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      {
         if(t->get_kind() == function_val_node_K || t->get_kind() == call_stmt_K)
         {
            return 32ull; // static_cast<unsigned int>(CompilerWrapper::CGetPointerSize(parameters));
         }
         return Size(CGetType(t));
      }
      case lut_node_K:
      {
         return 1ull;
      }
      case void_ty_node_K:
      {
         return 8ull;
      }
      case CASE_FAKE_NODES:
      case multi_way_if_stmt_K:
      case nop_stmt_K:
      case identifier_node_K:
      case statement_list_node_K:
      default:
      {
         THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
         break;
      }
   }
   THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
   return 0;
}

unsigned long long ir_helper::SizeAlloc(const ir_nodeConstRef& t)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "---Getting size of " + t->get_kind_text() + " " + STR(t->index) + ": " + t->ToString());
   switch(t->get_kind())
   {
      case array_ty_node_K:
      {
         const auto at = GetPointerS<const array_ty_node>(t);
         return static_cast<unsigned long long>(at->bitsizealloc);
      }
      case integer_ty_node_K:
      case function_ty_node_K:
      case pointer_ty_node_K:
      case real_ty_node_K:
      case struct_ty_node_K:
      case vector_ty_node_K:
      {
         return static_cast<unsigned long long>(GetPointerS<const type_node>(t)->bitsizealloc);
      }
      case call_stmt_K:
      {
         return 32ull;
      }
      case CASE_DECL_NODES:
      {
         if(t->get_kind() == field_val_node_K)
         {
            const auto snode = GetPointerS<const field_val_node>(t)->bitsizealloc;
            if(snode)
            {
               return static_cast<unsigned long long>(snode);
            }
         }
         return SizeAlloc(GetPointerS<const decl_node>(t)->type);
      }
      case lut_node_K:
      case CASE_UNARY_NODES:
      case CASE_BINARY_NODES:
      case CASE_TERNARY_NODES:
      case ssa_node_K:
      case call_node_K:
      case constructor_node_K:
      case assign_stmt_K:
      case phi_stmt_K:
      case return_stmt_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      {
         return SizeAlloc(CGetType(t));
      }
      case void_ty_node_K:
      {
         return 8ull;
      }
      case CASE_FAKE_NODES:
      case multi_way_if_stmt_K:
      case nop_stmt_K:
      case identifier_node_K:
      case statement_list_node_K:
      default:
      {
         THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
         break;
      }
   }
   THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
   return 0;
}

Range ir_helper::NodeRange(const ir_nodeConstRef& tn)
{
   const auto type = CGetType(tn);
   auto bw = static_cast<Range::bw_t>(Size(type));
   THROW_ASSERT(static_cast<bool>(bw),
                "Unhandled type (" + type->get_kind_text() + ") for " + tn->get_kind_text() + " " + tn->ToString());
   bool sign = false;
   APInt min, max;
   if(tn->get_kind() == constant_int_val_node_K)
   {
      min = max = GetConstValue(tn);
      return Range(Regular, bw, min, max);
   }
   if(tn->get_kind() == constant_fp_val_node_K)
   {
      const auto rc = GetPointerS<const constant_fp_val_node>(tn);
      THROW_ASSERT(bw == 64 || bw == 32, "Floating point variable with unhandled bitwidth (" + STR(bw) + ")");
      if(rc->valx.front() == '-' && rc->valr.front() != rc->valx.front())
      {
         return Range::fromBitValues(string_to_bitstring(convert_fp_to_string("-" + rc->valr, bw)), bw, false);
      }
      return Range::fromBitValues(string_to_bitstring(convert_fp_to_string(rc->valr, bw)), bw, false);
   }
   if(tn->get_kind() == constant_vector_val_node_K)
   {
      const auto vc = GetPointerS<const constant_vector_val_node>(tn);
      const auto el_type = CGetElements(type);
      const auto el_bw = Size(el_type);
      Range r(Empty, bw);
      const auto stride = static_cast<size_t>(bw / el_bw);
      const auto strides = vc->list_of_valu.size() / stride;
      THROW_ASSERT(strides * stride == vc->list_of_valu.size(), "");
      for(size_t i = 0; i < strides; ++i)
      {
         auto curr_el = NodeRange(vc->list_of_valu.at(i * stride));
         for(size_t j = 1; j < stride; ++j)
         {
            curr_el = NodeRange(vc->list_of_valu.at(i * stride + j))
                          .zextOrTrunc(bw)
                          .shl(Range(Regular, bw, el_bw * j, el_bw * j))
                          .Or(curr_el);
         }
         r = r.unionWith(curr_el);
      }
      return r;
   }
   if(type->get_kind() == integer_ty_node_K)
   {
      const auto it = GetPointerS<const integer_ty_node>(type);
      sign = !it->unsigned_flag;
      min = sign ? APInt::getSignedMinValue(bw) : APInt::getMinValue(bw);
      max = sign ? APInt::getSignedMaxValue(bw) : APInt::getMaxValue(bw);
   }
   else if(type->get_kind() == pointer_ty_node_K)
   {
      min = APInt::getMinValue(bw);
      max = APInt::getMaxValue(bw);
   }
   else if(type->get_kind() == vector_ty_node_K || type->get_kind() == array_ty_node_K)
   {
      bw = static_cast<Range::bw_t>(Size(CGetElements(type)));
      return Range(Regular, bw);
   }
   else if(type->get_kind() == struct_ty_node_K)
   {
      const auto rt = GetPointerS<const struct_ty_node>(type);
      bw = static_cast<Range::bw_t>(rt->bitsizealloc);
      THROW_ASSERT(bw, "Invalid bitwidth");
      return Range(Regular, bw);
   }
   else
   {
      THROW_UNREACHABLE("Unable to define range for type " + type->get_kind_text() + " of " + tn->ToString());
   }

   if(tn->get_kind() == ssa_node_K)
   {
      const auto ssa = GetPointerS<const ssa_node>(tn);
      if(!ssa->bit_values.empty())
      {
         const auto bvSize = static_cast<Range::bw_t>(ssa->bit_values.size());
         const auto bvRange = Range::fromBitValues(string_to_bitstring(ssa->bit_values), bvSize, sign);
         const auto varRange = Range(Regular, bw, min, max).truncate(bvSize).intersectWith(bvRange);
         return sign ? varRange.sextOrTrunc(bw) : varRange.zextOrTrunc(bw);
      }
   }
   return Range(Regular, bw, min, max);
}

Range ir_helper::TypeRange(const ir_nodeConstRef& tn, int _rt)
{
   const auto rt = static_cast<RangeType>(_rt);
   THROW_ASSERT(rt != Anti, "");
   const auto type = CGetType(tn);
   const auto bw = static_cast<Range::bw_t>(Size(type));
   THROW_ASSERT(bw, "Unhandled type (" + type->get_kind_text() + ") for " + tn->get_kind_text() + " " + tn->ToString());
   return Range(rt, bw);
}

std::string ir_helper::GetFunctionName(const ir_nodeConstRef& decl)
{
   if(decl->get_kind() == addr_node_K)
   {
      return GetFunctionName(GetPointerS<const unary_node>(decl)->op);
   }
   THROW_ASSERT(decl->get_kind() == function_val_node_K, "Node not yet supported " + decl->get_kind_text());
   const auto fd = GetPointerS<const function_val_node>(decl);
   const auto& name = (fd->builtin_flag || !fd->mngl) ? fd->name : fd->mngl;
   std::string res;
   if(name->get_kind() == identifier_node_K)
   {
      const auto in = GetPointerS<const identifier_node>(name);
      res = NormalizeTypename(in->strg);
   }
   else
   {
      THROW_ERROR(std::string("Node not yet supported ") + name->get_kind_text());
   }
   if(fd->builtin_flag && fd->body)
   {
      res = "__internal_" + res;
   }
   return res;
}

std::tuple<std::string, unsigned int, unsigned int> ir_helper::GetSourcePath(const ir_nodeConstRef& _node,
                                                                             bool& is_system)
{
   std::string include_name;
   unsigned int line_number{0};
   unsigned int column_number{0};
   auto node = _node;
   is_system = false;
   if(node->get_kind() == struct_ty_node_K)
   {
      const auto& list_of_flds = GetPointerS<const struct_ty_node>(node)->list_of_flds;
      if(!list_of_flds.empty())
      {
         node = list_of_flds.front();
      }
   }

   if(const auto dn = GetPointer<const decl_node>(node))
   {
      include_name = dn->include_name;
      line_number = dn->line_number;
      column_number = dn->column_number;
      is_system = dn->operating_system_flag || dn->library_system_flag;
   }
   else if(const auto sn = GetPointer<const IR_LocInfo>(node))
   {
      include_name = sn->include_name;
      line_number = sn->line_number;
      column_number = sn->column_number;
   }
   return {std::filesystem::path(include_name).lexically_normal().string(), line_number, column_number};
}

void ir_helper::get_used_variables(bool first_level_only, const ir_nodeConstRef& tRI,
                                   CustomUnorderedSet<unsigned int>& list_of_variable)
{
   if(!tRI)
   {
      return;
   }
   const auto t = tRI;
   switch(t->get_kind())
   {
      case variable_val_node_K:
      {
         const auto vd = GetPointer<const variable_val_node>(t);
         list_of_variable.insert(tRI->index);
         if(vd->init)
         {
            get_used_variables(first_level_only, vd->init, list_of_variable);
         }
         break;
      }
      case ssa_node_K:
      {
         const auto sn = GetPointer<const ssa_node>(t);
         get_used_variables(first_level_only, sn->var, list_of_variable);
         list_of_variable.insert(tRI->index);
         if(sn->var)
         {
            const auto vd = GetPointer<const variable_val_node>(sn->var);
            if(vd && vd->init)
            {
               get_used_variables(first_level_only, vd->init, list_of_variable);
            }
         }
         break;
      }
      case argument_val_node_K:
         list_of_variable.insert(tRI->index);
         break;
      case function_val_node_K:
      {
         const auto fd = GetPointer<const function_val_node>(t);
         bool expand_p = !first_level_only;
         auto vend = fd->list_of_args.end();
         list_of_variable.insert(tRI->index);
         if(fd->body && expand_p)
         {
            for(auto i = fd->list_of_args.begin(); i != vend; ++i)
            {
               get_used_variables(first_level_only, *i, list_of_variable);
            }
            // body analysis
            get_used_variables(first_level_only, fd->body, list_of_variable);
         }
      }
      break;
      case statement_list_node_K:
      {
         const auto sl = GetPointer<const statement_list_node>(t);
         auto ib_end = sl->list_of_bloc.end();
         for(auto ib = sl->list_of_bloc.begin(); ib != ib_end; ++ib)
         {
            for(const auto& stmt : ib->second->CGetStmtList())
            {
               get_used_variables(first_level_only, stmt, list_of_variable);
            }
         }
         break;
      }
      case assign_stmt_K:
      {
         const auto me = GetPointer<const assign_stmt>(t);
         get_used_variables(first_level_only, me->op0, list_of_variable);
         get_used_variables(first_level_only, me->op1, list_of_variable);
         if(me->predicate)
         {
            get_used_variables(first_level_only, me->predicate, list_of_variable);
         }
      }
      break;
      case call_stmt_K:
      {
         const auto ce = GetPointer<const call_stmt>(t);
         get_used_variables(first_level_only, ce->fn, list_of_variable);
         for(const auto& arg : ce->args)
         {
            get_used_variables(first_level_only, arg, list_of_variable);
         }
         if(ce->predicate)
         {
            get_used_variables(first_level_only, ce->predicate, list_of_variable);
         }
      }
      break;
      case return_stmt_K:
      {
         const auto re = GetPointer<const return_stmt>(t);
         if(re->op)
         {
            get_used_variables(first_level_only, re->op, list_of_variable);
         }
      }
      break;
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointer<const unary_node>(t);
         if(list_of_variable.find(ue->op->index) != list_of_variable.end())
         {
            break;
         }
         get_used_variables(first_level_only, ue->op, list_of_variable);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointer<const binary_node>(t);
         get_used_variables(first_level_only, be->op0, list_of_variable);
         get_used_variables(first_level_only, be->op1, list_of_variable);
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto tern = GetPointer<const ternary_node>(t);
         get_used_variables(first_level_only, tern->op0, list_of_variable);
         get_used_variables(first_level_only, tern->op1, list_of_variable);
         get_used_variables(first_level_only, tern->op2, list_of_variable);
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointer<const lut_node>(t);
         get_used_variables(first_level_only, le->op0, list_of_variable);
         get_used_variables(first_level_only, le->op1, list_of_variable);
         if(le->op2)
         {
            get_used_variables(first_level_only, le->op2, list_of_variable);
         }
         if(le->op3)
         {
            get_used_variables(first_level_only, le->op3, list_of_variable);
         }
         if(le->op4)
         {
            get_used_variables(first_level_only, le->op4, list_of_variable);
         }
         if(le->op5)
         {
            get_used_variables(first_level_only, le->op5, list_of_variable);
         }
         if(le->op6)
         {
            get_used_variables(first_level_only, le->op6, list_of_variable);
         }
         if(le->op7)
         {
            get_used_variables(first_level_only, le->op7, list_of_variable);
         }
         if(le->op8)
         {
            get_used_variables(first_level_only, le->op8, list_of_variable);
         }
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointer<const multi_way_if_stmt>(t);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               get_used_variables(first_level_only, cond.first, list_of_variable);
            }
         }
         break;
      }
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
         break;
      case field_val_node_K: // used to specify the displacement
      case constructor_node_K:
      {
         const auto co = GetPointer<const constructor_node>(t);
         for(const auto& i : co->list_of_idx_valu)
         {
            get_used_variables(first_level_only, i.second, list_of_variable);
         }
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointer<const call_node>(t);
         for(const auto& arg : ce->args)
         {
            get_used_variables(first_level_only, arg, list_of_variable);
         }
         break;
      }
      case nop_stmt_K:
      case phi_stmt_K:
      case identifier_node_K:
      case module_unit_node_K:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("Node not yet supported ") + t->get_kind_text());
   }
}

bool ir_helper::IsSystemType(const ir_nodeConstRef& type)
{
   if(GetPointer<const decl_node>(type))
   {
      return GetPointer<const decl_node>(type)->operating_system_flag ||
             GetPointer<const decl_node>(type)->library_system_flag;
   }
   if(GetPointer<const type_node>(type))
   {
      return GetPointer<const type_node>(type)->system_flag;
   }
   return false;
}

bool ir_helper::IsInLibbambu(const ir_nodeConstRef& type)
{
   if(GetPointer<const decl_node>(type) && GetPointer<const decl_node>(type)->libbambu_flag)
   {
      return true;
   }
   if(GetPointer<const type_node>(type) && GetPointer<const type_node>(type)->libbambu_flag)
   {
      return true;
   }
   return false;
}

static void RecursiveGetTypesToBeDeclared(std::set<ir_nodeConstRef, IRNodeConstSorter>& returned_types,
                                          const ir_nodeConstRef& type, const bool recursion,
                                          const bool without_transformation, const bool before)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level,
                  "-->Getting types to be declared " + STR(before ? "before " : "after ") + STR(type));
   switch(type->get_kind())
   {
      case pointer_ty_node_K:
      {
         if(before)
         {
            RecursiveGetTypesToBeDeclared(returned_types, ir_helper::CGetPointedType(type), true,
                                          without_transformation, true);
         }
         break;
      }
      case array_ty_node_K:
      case vector_ty_node_K:
      {
         if(before)
         {
            RecursiveGetTypesToBeDeclared(returned_types, ir_helper::CGetElements(type), true, without_transformation,
                                          before);
         }
         break;
      }
      case struct_ty_node_K:
      {
         if(recursion)
         {
            if(before)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level,
                              "---Inserting " + STR(type) + " in the types to be declared");
               returned_types.insert(type);
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level, "-->Record type without named unqualified");
            const auto field_types = ir_helper::CGetFieldTypes(type);
            for(const auto& field_type : field_types)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level,
                              "-->Considering field type (" + STR(field_type->index) + ") " + STR(field_type));
               bool pointer_to_unnamed_structure = [&]() {
                  if(!ir_helper::IsPointerType(field_type))
                  {
                     return false;
                  }
                  const auto pointed_type = ir_helper::CGetPointedType(field_type);
                  if(GetPointer<const struct_ty_node>(pointed_type))
                  {
                     return true;
                  }
                  return false;
               }();
               /// Non pointer fields must be declared before structs, pointer fields can be declared after; in
               /// some cases they must be declared after (circular dependencies)
               if(before)
               {
                  if(!ir_helper::IsPointerType(field_type) || !pointer_to_unnamed_structure)
                  {
                     RecursiveGetTypesToBeDeclared(returned_types, field_type, true, without_transformation, true);
                  }
               }
               else
               {
                  if(pointer_to_unnamed_structure)
                  {
                     /// Here true is correct
                     RecursiveGetTypesToBeDeclared(returned_types, field_type, true, without_transformation, true);
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level,
                              "<--Considered field type (" + STR(field_type->index) + ") " + STR(field_type));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level, "<--");
         }

         break;
      }
      case integer_ty_node_K:
      case real_ty_node_K:
      case void_ty_node_K:
      {
         break;
      }
      case function_ty_node_K:
      {
         if(before)
         {
            const auto return_type = ir_helper::GetFunctionReturnType(type);
            if(return_type)
            {
               RecursiveGetTypesToBeDeclared(returned_types, return_type, true, without_transformation, true);
            }
            const auto parameters = ir_helper::GetParameterTypes(type);
            for(const auto& par : parameters)
            {
               RecursiveGetTypesToBeDeclared(returned_types, par, true, without_transformation, true);
            }
         }
         break;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
      {
         THROW_UNREACHABLE("Unexpected node: " + STR(type));
         break;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level,
                  STR("<--Got types to be declared ") + (before ? "before" : "after") + " " + STR(type));
}

std::set<ir_nodeConstRef, IRNodeConstSorter> ir_helper::GetTypesToBeDeclaredBefore(const ir_nodeConstRef& tn,
                                                                                   const bool without_transformation)
{
   std::set<ir_nodeConstRef, IRNodeConstSorter> rt;
   RecursiveGetTypesToBeDeclared(rt, tn, false, without_transformation, true);
   return rt;
}

std::set<ir_nodeConstRef, IRNodeConstSorter> ir_helper::GetTypesToBeDeclaredAfter(const ir_nodeConstRef& tn,
                                                                                  const bool without_transformation)
{
   std::set<ir_nodeConstRef, IRNodeConstSorter> rt;
   RecursiveGetTypesToBeDeclared(rt, tn, false, without_transformation, false);
   return rt;
}

ir_nodeConstRef ir_helper::GetFunctionReturnType(const ir_nodeConstRef& tn, bool void_as_null)
{
   ir_nodeConstRef fun_type;
   switch(tn->get_kind())
   {
      case function_val_node_K:
      {
         const auto* fd = GetPointerS<const function_val_node>(tn);
         fun_type = fd->type;
         break;
      }
      case function_ty_node_K:
      {
         fun_type = tn;
         break;
      }
      case array_ty_node_K:
      case call_node_K:
      case constructor_node_K:
      case field_val_node_K:
      case identifier_node_K:
      case integer_ty_node_K:
      case pointer_ty_node_K:
      case argument_val_node_K:
      case real_ty_node_K:
      case struct_ty_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
      case variable_val_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
      {
         break;
      }
   }
   if(fun_type->get_kind() == function_ty_node_K)
   {
      const auto* ft = GetPointerS<const function_ty_node>(fun_type);
      THROW_ASSERT(ft, "NodeId is not related to a valid function type");
      if(!void_as_null || ft->retn->get_kind() != void_ty_node_K)
      {
         return ft->retn;
      }
      return {};
   }
   THROW_UNREACHABLE("Not supported IR node type " + tn->get_kind_text());
   return {};
}

ir_nodeConstRef ir_helper::CGetPointedType(const ir_nodeConstRef& pointer)
{
   switch(pointer->get_kind())
   {
      case pointer_ty_node_K:
      {
         const auto pt = GetPointerS<const pointer_ty_node>(pointer);
         return pt->ptd;
      }
      case function_ty_node_K:
      {
         const auto ft = GetPointerS<const function_ty_node>(pointer);
         return CGetPointedType(ft->retn);
      }
      case array_ty_node_K:
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case integer_ty_node_K:
      case real_ty_node_K:
      case struct_ty_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
      {
         THROW_UNREACHABLE(STR(pointer) + ":" + pointer->get_kind_text() + " does not correspond to a pointer type");
      }
   }
   return {};
}

ir_nodeConstRef ir_helper::CGetElements(const ir_nodeConstRef& type)
{
   const auto at = GetPointer<const array_ty_node>(type);
   if(at)
   {
      return at->elts;
   }
   const auto vt = GetPointer<const vector_ty_node>(type);
   if(vt)
   {
      return vt->elts;
   }
   THROW_UNREACHABLE("IR node of type " + type->get_kind_text());
   return {};
}

std::vector<ir_nodeConstRef> ir_helper::GetParameterTypes(const ir_nodeConstRef& ftype)
{
   std::vector<ir_nodeConstRef> params;
   const auto Type = CGetType(ftype);
   THROW_ASSERT(Type->get_kind() == function_ty_node_K,
                "Type " + STR(Type) + " from " + STR(ftype) + " does not correspond to a function type");
   for(const auto& p : GetPointerS<const function_ty_node>(Type)->list_of_args_type)
   {
      params.push_back(p);
   }
   return params;
}

bool ir_helper::IsSameType(const ir_nodeConstRef& tn0, const ir_nodeConstRef& tn1)
{
   const auto tn0_type = CGetType(tn0);
   const auto tn1_type = CGetType(tn1);
   return tn0_type->get_kind() == tn1_type->get_kind() && Size(tn0_type) == Size(tn1_type) &&
          (tn0_type->get_kind() != integer_ty_node_K ||
           GetPointerS<const integer_ty_node>(tn0_type)->unsigned_flag ==
               GetPointerS<const integer_ty_node>(tn1_type)->unsigned_flag);
}

std::vector<ir_nodeConstRef> ir_helper::CGetFieldTypes(const ir_nodeConstRef& type)
{
   std::vector<ir_nodeConstRef> ret;
   THROW_ASSERT(type->get_kind() == struct_ty_node_K, "unexpected condition");

   const auto* rt = GetPointerS<const struct_ty_node>(type);
   for(const auto& list_of_fld : rt->list_of_flds)
   {
      if(list_of_fld->get_kind() == function_val_node_K)
      {
         continue;
      }
      ret.push_back(CGetType(list_of_fld));
   }

   return ret;
}

ir_nodeConstRef ir_helper::CGetType(const ir_nodeConstRef& node)
{
   switch(node->get_kind())
   {
      case call_node_K:
      {
         const auto* ce = GetPointerS<const call_node>(node);
         return ce->type;
      }
      case assign_stmt_K:
      {
         const auto* gm = GetPointerS<const assign_stmt>(node);
         return CGetType(gm->op0);
      }
      case phi_stmt_K:
      {
         const auto* gp = GetPointerS<const phi_stmt>(node);
         return CGetType(gp->res);
      }
      case return_stmt_K:
      {
         const auto* gr = GetPointerS<const return_stmt>(node);
         return CGetType(gr->op);
      }
      case multi_way_if_stmt_K:
      {
         const auto* gmwi = GetPointerS<const multi_way_if_stmt>(node);
         return CGetType(gmwi->list_of_cond.front().first);
      }
      case nop_stmt_K:
      case call_stmt_K:
      {
         return {};
      }
      case lut_node_K:
      case CASE_UNARY_NODES:
      case CASE_BINARY_NODES:
      case CASE_TERNARY_NODES:
      {
         const auto* en = GetPointerS<const expr_node>(node);
         // THROW_ASSERT(en->type, std::string("this NODE does not have a type: ") + node->get_kind_text());
         return en->type;
      }
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      {
         const auto* cn = GetPointerS<const cst_node>(node);
         return cn->type;
      }
      case constant_vector_val_node_K:
      {
         const auto* cn = GetPointerS<const cst_node>(node);
         return cn->type ? cn->type : node;
      }
      case constructor_node_K:
      {
         const auto* c = GetPointerS<const constructor_node>(node);
         return c->type;
      }
      case ssa_node_K:
      {
         const auto* sa = GetPointerS<const ssa_node>(node);
         return sa->type;
      }
      case CASE_DECL_NODES:
      {
         const auto* dn = GetPointerS<const decl_node>(node);
         return dn->type;
      }
      case CASE_TYPE_NODES:
      {
         return node;
      }
      case CASE_FAKE_NODES:
      case identifier_node_K:
      case statement_list_node_K:
      default:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, std::string("Node not yet supported ") + node->get_kind_text());
      }
   }
   return node;
}

bool ir_helper::IsStructType(const ir_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   return Type->get_kind() == struct_ty_node_K;
}

static void getBuiltinFieldTypes(const ir_nodeConstRef& type, std::list<ir_nodeConstRef>& listOfTypes,
                                 CustomUnorderedSet<unsigned int>& already_visited)
{
   if(already_visited.count(type->index))
   {
      return;
   }
   already_visited.insert(type->index);
   if(type->get_kind() == struct_ty_node_K)
   {
      const auto rt = GetPointerS<const struct_ty_node>(type);
      for(const auto& fld : rt->list_of_flds)
      {
         if(fld->get_kind() == function_val_node_K)
         {
            continue;
         }
         const auto fdType = ir_helper::CGetType(fld);
         getBuiltinFieldTypes(fdType, listOfTypes, already_visited);
      }
   }
   else if(type->get_kind() == array_ty_node_K)
   {
      const auto at = GetPointerS<const array_ty_node>(type);
      THROW_ASSERT(at->elts, "elements type expected");
      getBuiltinFieldTypes(at->elts, listOfTypes, already_visited);
   }
   else if(type->get_kind() == vector_ty_node_K)
   {
      const auto vt = GetPointerS<const vector_ty_node>(type);
      THROW_ASSERT(vt->elts, "elements type expected");
      getBuiltinFieldTypes(vt->elts, listOfTypes, already_visited);
   }
   else
   {
      listOfTypes.push_back(type);
   }
}

static bool same_size_fields(const ir_nodeConstRef& t)
{
   std::list<ir_nodeConstRef> listOfTypes;
   CustomUnorderedSet<unsigned int> already_visited;
   getBuiltinFieldTypes(t, listOfTypes, already_visited);
   if(listOfTypes.empty())
   {
      return false;
   }
   if(ir_helper::IsStructType(t))
   {
      for(const auto& fld : GetPointerS<const struct_ty_node>(t)->list_of_flds)
      {
         if(GetPointerS<const field_val_node>(fld)->bitfield)
         {
            return false;
         }
      }
   }

   const auto sizeFlds = ir_helper::SizeAlloc(listOfTypes.front());
   if(ceil_pow2(sizeFlds) != sizeFlds)
   {
      return false;
   }
   for(const auto& fldType : listOfTypes)
   {
      if(sizeFlds != ir_helper::SizeAlloc(fldType))
      {
         return false;
      }
   }
   return true;
}

bool ir_helper::IsArrayEquivType(const ir_nodeConstRef& type)
{
   THROW_ASSERT(type, "expected a type index");
   if(type->get_kind() == array_ty_node_K)
   {
      return true;
   }
   if(type->get_kind() == struct_ty_node_K)
   {
      return same_size_fields(type);
   }
   return false;
}

bool ir_helper::IsArrayType(const ir_nodeConstRef& _type)
{
   const auto type = CGetType(_type);
   THROW_ASSERT(type, "expected a type index");
   return type->get_kind() == array_ty_node_K;
}

ir_nodeConstRef ir_helper::CGetArrayBaseType(const ir_nodeConstRef& type)
{
   std::list<ir_nodeConstRef> listOfTypes;
   CustomUnorderedSet<unsigned int> already_visited;
   const auto Type = CGetType(type);
   getBuiltinFieldTypes(Type, listOfTypes, already_visited);
   THROW_ASSERT(!listOfTypes.empty(), "at least one type is expected");
   return listOfTypes.front();
}

bool ir_helper::IsPointerType(const ir_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   if(Type->get_kind() == pointer_ty_node_K)
   {
      return true;
   }
   return false;
}

bool ir_helper::IsFunctionDeclaration(const ir_nodeConstRef& type)
{
   THROW_ASSERT(type, "expected a type");
   return type->get_kind() == function_val_node_K;
}

bool ir_helper::IsFunctionImplemented(const ir_nodeConstRef& decl_node)
{
   return decl_node->get_kind() == function_val_node_K && GetPointerS<const function_val_node>(decl_node)->body;
}

bool ir_helper::IsVectorType(const ir_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   return Type->get_kind() == vector_ty_node_K;
}

bool ir_helper::is_a_misaligned_vector(const ir_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetIRNode(index);
   THROW_ASSERT(T, "this index does not exist: " + STR(index));
   if(!IsVectorType(T))
   {
      return false;
   }
   if(GetPointer<const unaligned_mem_access_node>(T))
   {
      return true;
   }
   const auto Type = CGetType(T);
   THROW_ASSERT(Type, "expected a type index");
   const auto vt = GetPointer<const vector_ty_node>(Type);
   THROW_ASSERT(vt, "expected a vector type");
   return vt->algn != SizeAlloc(Type);
}

bool ir_helper::is_an_addr_node(const ir_managerConstRef& TM, const unsigned int index)
{
   return TM->GetIRNode(index)->get_kind() == addr_node_K;
}

unsigned int ir_helper::GetFunctionIdFromOpId(const ir_managerConstRef& TM, const unsigned int op_id)
{
   const auto op_node = TM->GetIRNode(op_id);
   const auto* stmt = GetPointer<const node_stmt>(op_node);
   if(!stmt || !stmt->parent || stmt->parent->get_kind() != function_val_node_K)
   {
      return 0;
   }
   return stmt->parent->index;
}

unsigned int ir_helper::GetSsaNameNodeIdFromOpId(const ir_managerConstRef& TM, const unsigned int op_id)
{
   const auto op_node = TM->GetIRNode(op_id);
   if(!op_node)
   {
      return 0;
   }
   if(op_node->get_kind() == assign_stmt_K)
   {
      const auto g_as_node = GetPointer<const assign_stmt>(op_node);
      return g_as_node ? g_as_node->op0->index : 0;
   }
   if(op_node->get_kind() == phi_stmt_K)
   {
      const auto g_phi_node = GetPointer<const phi_stmt>(op_node);
      return g_phi_node ? g_phi_node->res->index : 0;
   }
   return 0;
}

bool ir_helper::HasToBeDeclared(const ir_nodeConstRef& type)
{
   THROW_ASSERT(GetPointer<const type_node>(type),
                "IR node " + STR(type) + " is not a type_node but " + type->get_kind_text());
   return type->get_kind() == struct_ty_node_K;
}

bool ir_helper::IsFunctionType(const ir_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   return Type->get_kind() == function_ty_node_K;
}

bool ir_helper::IsFunctionPointerType(const ir_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   if(Type->get_kind() == pointer_ty_node_K)
   {
      const auto ptd = GetPointerS<const pointer_ty_node>(Type)->ptd;
      if(ptd->get_kind() == function_ty_node_K)
      {
         return true;
      }
   }
   return false;
}

bool ir_helper::IsBooleanType(const ir_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   return Type->get_kind() == integer_ty_node_K && GetPointerS<const integer_ty_node>(Type)->bitsize == 1;
}

bool ir_helper::IsVoidType(const ir_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   return Type->get_kind() == void_ty_node_K;
}

bool ir_helper::IsPositiveIntegerValue(const ir_nodeConstRef& type)
{
   if(type->get_kind() == ssa_node_K && GetPointerS<const ssa_node>(type)->min)
   {
      const auto& minimum = GetPointerS<const ssa_node>(type)->min;
      THROW_ASSERT(minimum->get_kind() == constant_int_val_node_K, "expected an integer const: " + STR(type));
      const auto min_value = GetConstValue(minimum);
      return min_value >= 0;
   }
   return false;
}

bool ir_helper::IsSignedIntegerType(const ir_nodeConstRef& type)
{
   const auto tnode = CGetType(type);
   return tnode->get_kind() == integer_ty_node_K && !GetPointerS<const integer_ty_node>(tnode)->unsigned_flag;
}

bool ir_helper::IsRealType(const ir_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   return Type->get_kind() == real_ty_node_K;
}

bool ir_helper::IsUnsignedIntegerType(const ir_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   if(Type->get_kind() == integer_ty_node_K)
   {
      return GetPointerS<const integer_ty_node>(Type)->unsigned_flag;
   }
   return false;
}

bool ir_helper::IsScalarType(const ir_nodeConstRef& type)
{
   return IsSignedIntegerType(type) || IsRealType(type) || IsUnsignedIntegerType(type) || IsBooleanType(type);
}

bool ir_helper::IsVariableType(const ir_nodeConstRef& node)
{
   const auto node_kind = node->get_kind();
   switch(node_kind)
   {
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      case ssa_node_K:
      case function_val_node_K:
      case variable_val_node_K:
      case argument_val_node_K:
      case call_node_K:
         return true;
      /// The following one are not considered as variables, but as operations on the variables
      case constructor_node_K:
      case field_val_node_K:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
         return false;
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                          "ir_helper::is_a_variable - variable is not supported: " + ir_node::GetString(node_kind));
   }
   return true;
}

static ir_nodeRef _GetBaseVariable(const ir_nodeRef& node, std::vector<ir_nodeRef>* field_offset,
                                   bool first_call = false)
{
   switch(node->get_kind())
   {
      case function_val_node_K:
      case constant_int_val_node_K:
      case argument_val_node_K:
      case variable_val_node_K:
      {
         return node;
      }
      case ssa_node_K:
      {
         const auto* sn = GetPointerS<const ssa_node>(node);
         if(field_offset == nullptr && sn->use_set.is_a_singleton())
         {
            return _GetBaseVariable(sn->use_set.variables.front(), field_offset);
         }
         const auto def_stmt = sn->GetDefStmt();
         std::vector<ir_nodeRef> local_field_offset;
         const auto base = _GetBaseVariable(def_stmt, field_offset ? &local_field_offset : nullptr);
         if(base && base->index != node->index)
         {
            if(field_offset)
            {
               field_offset->insert(field_offset->end(), local_field_offset.begin(), local_field_offset.end());
            }
            return _GetBaseVariable(base, field_offset);
         }
         if(sn->var)
         {
            return _GetBaseVariable(sn->var, field_offset);
         }
         return node;
      }
      case assign_stmt_K:
      {
         const auto* ga = GetPointerS<const assign_stmt>(node);
         return _GetBaseVariable(ga->op1, field_offset);
      }
      case phi_stmt_K:
      {
         const auto* gp = GetPointerS<const phi_stmt>(node);
         if(field_offset)
         {
            THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                             "ir_helper::GetBaseVariable::phi_stmt_K - not supported pattern");
         }
         return gp->res;
      }
      case nop_stmt_K:
      {
         break;
      }
      case addr_node_K:
      case unaligned_mem_access_node_K:
      case nop_node_K:
      case bitcast_node_K:
      {
         const auto* ue = GetPointerS<const unary_node>(node);
         if(!first_call && (node->get_kind() == unaligned_mem_access_node_K))
         {
            break;
         }
         return _GetBaseVariable(ue->op, field_offset);
      }
      case mem_access_node_K:
      case gep_node_K:
      {
         const auto* be = GetPointerS<const binary_node>(node);
         if(!first_call && node->get_kind() == mem_access_node_K)
         {
            break;
         }
         if(field_offset)
         {
            field_offset->push_back(be->op1);
         }
         return _GetBaseVariable(be->op0, field_offset);
      }
      case abs_node_K:
      case and_node_K:
      case concat_bit_node_K:
      case or_node_K:
      case not_node_K:
      case xor_node_K:
      case call_node_K:
      case select_node_K:
      case constructor_node_K:
      case eq_node_K:
      case extract_bit_node_K:
      case extractelement_node_K:
      case extractvalue_node_K:
      case field_val_node_K:
      case fptoi_node_K:
      case itofp_node_K:
      case frem_node_K:
      case fshl_node_K:
      case fshr_node_K:
      case ge_node_K:
      case call_stmt_K:
      case multi_way_if_stmt_K:
      case return_stmt_K:
      case gt_node_K:
      case identifier_node_K:
      case insertelement_node_K:
      case insertvalue_node_K:
      case le_node_K:
      case shl_node_K:
      case lt_node_K:
      case lut_node_K:
      case max_node_K:
      case min_node_K:
      case sub_node_K:
      case mul_node_K:
      case ne_node_K:
      case neg_node_K:
      case add_node_K:
      case fdiv_node_K:
      case constant_fp_val_node_K:
      case shr_node_K:
      case sub_sat_node_K:
      case add_sat_node_K:
      case statement_list_node_K:
      case ternary_ss_node_K:
      case ternary_sa_node_K:
      case ternary_add_node_K:
      case ternary_as_node_K:
      case module_unit_node_K:
      case idiv_node_K:
      case irem_node_K:
      case shufflevector_node_K:
      case constant_vector_val_node_K:
      case widen_mul_node_K:
      {
         break;
      }
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      default:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "ir_helper::GetBaseVariable - variable type is not supported: " +
                                                         STR(node) + "-" + std::string(node->get_kind_text()));
         break;
      }
   }
   return {};
}

ir_nodeRef ir_helper::GetBaseVariable(const ir_nodeRef& node, std::vector<ir_nodeRef>* field_offset)
{
   return _GetBaseVariable(node, field_offset, true);
}

const PointToSolution& ir_helper::GetPointToSet(const ir_nodeConstRef& node)
{
   static const PointToSolution unresolved = []() {
      PointToSolution pts;
      pts.anything = true;
      return pts;
   }();

   THROW_ASSERT(node, "expected positive non zero numbers");
   switch(node->get_kind())
   {
      case ssa_node_K:
      {
         const auto sn = GetPointerS<const ssa_node>(node);
         if(sn->use_set.is_fully_resolved())
         {
            return sn->use_set;
         }
         const auto base = GetBaseVariable(sn->GetDefStmt());
         if(base && base->index != sn->index)
         {
            return GetPointToSet(base);
         }
         return unresolved;
      }
      case function_val_node_K:
      case constant_int_val_node_K:
      case argument_val_node_K:
      case variable_val_node_K:
      {
         return unresolved;
      }
      case unaligned_mem_access_node_K:
      {
         const auto mir = GetPointerS<const unaligned_mem_access_node>(node);
         return GetPointToSet(mir->op);
      }
      case mem_access_node_K:
      {
         const auto mr = GetPointerS<const mem_access_node>(node);
         return GetPointToSet(mr->op);
      }
      case addr_node_K:
      {
         const auto ae = GetPointerS<const addr_node>(node);
         return GetPointToSet(ae->op);
      }
      case bitcast_node_K:
      {
         const auto bitcast_expr = GetPointerS<const bitcast_node>(node);
         return GetPointToSet(bitcast_expr->op);
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case statement_list_node_K:
      case and_node_K:
      case or_node_K:
      case xor_node_K:
      case eq_node_K:
      case ge_node_K:
      case gt_node_K:
      case le_node_K:
      case shl_node_K:
      case lt_node_K:
      case max_node_K:
      case min_node_K:
      case sub_node_K:
      case mul_node_K:
      case ne_node_K:
      case add_node_K:
      case gep_node_K:
      case fdiv_node_K:
      case shr_node_K:
      case idiv_node_K:
      case irem_node_K:
      case widen_mul_node_K:
      case shufflevector_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      case field_val_node_K:
      case module_unit_node_K:
      case select_node_K:
      case ternary_add_node_K:
      case ternary_as_node_K:
      case ternary_sa_node_K:
      case ternary_ss_node_K:
      case fshl_node_K:
      case fshr_node_K:
      case concat_bit_node_K:
      case abs_node_K:
      case not_node_K:
      case fptoi_node_K:
      case itofp_node_K:
      case lut_node_K:
      case neg_node_K:
      case nop_node_K:
      case extract_bit_node_K:
      case add_sat_node_K:
      case sub_sat_node_K:
      case extractvalue_node_K:
      case insertvalue_node_K:
      case extractelement_node_K:
      case insertelement_node_K:
      case frem_node_K:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "ir_helper::GetPointToSet - variable type is not supported: " +
                                                         STR(node) + "-" + std::string(node->get_kind_text()));
   }
   return unresolved;
}

bool ir_helper::IsParameter(const ir_nodeConstRef& node)
{
   if(node->get_kind() == argument_val_node_K)
   {
      return true;
   }
   if(node->get_kind() != ssa_node_K)
   {
      return false;
   }
   const auto sn = GetPointerS<const ssa_node>(node);
   return sn->GetDefStmt()->get_kind() == nop_stmt_K && sn->var && sn->var->get_kind() == argument_val_node_K;
}

bool ir_helper::IsSsaName(const ir_nodeConstRef& tn)
{
   return tn->get_kind() == ssa_node_K;
}

bool ir_helper::IsVirtual(const ir_nodeConstRef& tn)
{
   if(tn->get_kind() == ssa_node_K)
   {
      return GetPointerS<const ssa_node>(tn)->virtual_flag;
   }
   return false;
}

bool ir_helper::IsStaticDeclaration(const ir_nodeConstRef& decl)
{
   const auto vd = GetPointer<const variable_val_node>(decl);
   if(!vd)
   {
      const auto fd = GetPointer<const function_val_node>(decl);
      if(!fd)
      {
         return false;
      }
      return fd->static_flag;
   }
   return vd->static_flag;
}

bool ir_helper::IsExternDeclaration(const ir_nodeConstRef& decl)
{
   const auto vd = GetPointer<const variable_val_node>(decl);
   if(!vd)
   {
      const auto fd = GetPointer<const function_val_node>(decl);
      if(!fd)
      {
         return false;
      }
      else
      {
         return !fd->body;
      }
   }
   return vd->extern_flag;
}

integer_cst_t ir_helper::get_integer_cst_value(const constant_int_val_node* ic)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Getting integer const value");
   THROW_ASSERT(ic != nullptr, "unexpected condition");
   THROW_ASSERT(ic->type, "Something wrong");
   const auto type = ic->type;
   THROW_ASSERT(GetPointer<integer_ty_node>(type) || type->get_kind() == pointer_ty_node_K,
                "Expected a integer_ty_node or a pointer_ty_node. Found: " + STR(ic->type->index) + " " +
                    type->get_kind_text());
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Constant is " + STR(ic->value));
   return ic->value;
}

integer_cst_t ir_helper::GetConstValue(const ir_nodeConstRef& tn, bool is_signed)
{
   THROW_ASSERT(tn && tn->get_kind() == constant_int_val_node_K, "unexpected condition");
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Getting integer const value");
   const auto ic = GetPointerS<const constant_int_val_node>(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Constant is " + STR(ic->value));
   if(!is_signed)
   {
      const auto bitwidth = Size(ic->type);
      return ic->value & ((integer_cst_t(1) << bitwidth) - 1);
   }
   return ic->value;
}

unsigned int ir_helper::get_array_var(const ir_managerConstRef& TM, const unsigned int index, bool, bool& two_dim_p)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   ir_nodeRef node = TM->GetIRNode(index);
   const auto gms = GetPointer<assign_stmt>(node);
   two_dim_p = false;
   auto ae = GetPointer<addr_node>(gms->op1);
   if(ae)
   {
      THROW_ERROR("Unexpected pattern " + STR(index));
      return 0;
   }
   const auto ne = GetPointer<nop_node>(gms->op1);
   if(ne)
   {
      ae = GetPointer<addr_node>(ne->op);
      if(ae)
      {
         const auto vd = GetPointer<variable_val_node>(ae->op);
         if(vd)
         {
            return ae->op->index;
         }
         else
         {
            THROW_ERROR("Unexpected pattern " + STR(index));
            return 0;
         }
      }
   }
   THROW_ERROR("Unexpected pattern " + STR(index));
   return 0;
}

bool ir_helper::is_concat_or_node(const ir_managerConstRef& TM, const unsigned int index)
{
   ir_nodeRef node = TM->GetIRNode(index);
   const auto ga = GetPointer<assign_stmt>(node);
   if(ga)
   {
      const auto bie = GetPointer<or_node>(ga->op1);
      if(bie)
      {
         ir_nodeRef op0 = bie->op0;
         ir_nodeRef op1 = bie->op1;
         if(op0->get_kind() == ssa_node_K && op1->get_kind() == ssa_node_K)
         {
            const auto op0_ssa = GetPointer<ssa_node>(op0);
            const auto op1_ssa = GetPointer<ssa_node>(op1);
            if(!op0_ssa->bit_values.empty() && !op1_ssa->bit_values.empty())
            {
               std::string::const_reverse_iterator it0 = op0_ssa->bit_values.rbegin();
               std::string::const_reverse_iterator it1 = op1_ssa->bit_values.rbegin();
               std::string::const_reverse_iterator it0_end = op0_ssa->bit_values.rend();
               std::string::const_reverse_iterator it1_end = op1_ssa->bit_values.rend();
               for(; it0 != it0_end && it1 != it1_end; ++it0, ++it1)
               {
                  if(*it0 != '0' && *it1 != '0')
                  {
                     return false;
                  }
               }
               return true;
            }
         }
      }
   }
   return false;
}

bool ir_helper::IsConstant(const ir_nodeConstRef& node)
{
   switch(node->get_kind())
   {
      case CASE_CST_NODES:
         return true;
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      default:
         return false;
   }
   return false;
}

std::string ir_helper::op_symbol(const ir_nodeConstRef& op)
{
   return op_symbol(op.get());
}

std::string ir_helper::op_symbol(const ir_node* op)
{
   THROW_ASSERT(op, "Null IR node");
   switch(op->get_kind())
   {
      case or_node_K:
         return "|";

      case xor_node_K:
         return "^";

      case addr_node_K:
      case and_node_K:
      {
         if(dynamic_cast<const addr_node*>(op))
         {
            const auto ae = static_cast<const addr_node*>(op);
            const auto tn = ae->op;
         }
         return "&";
      }

      case eq_node_K:
         return "==";

      case ne_node_K:
         return "!=";

      case lt_node_K:
      case lut_node_K:
         return "<";

      case le_node_K:
         return "<=";

      case gt_node_K:
         return ">";

      case ge_node_K:
         return ">=";

      case shl_node_K:
         return "<<";

      case shr_node_K:
         return ">>";

      case add_node_K:
      case gep_node_K:
         return "+";

      case neg_node_K:
      case sub_node_K:
         return "-";

      case not_node_K:
         return "~";

      case mul_node_K:
      case unaligned_mem_access_node_K:
      case widen_mul_node_K:
         return "*";

      case idiv_node_K:
      case fdiv_node_K:
         return "/";

      case irem_node_K:
         return "%";

         return " --";

         return " ++";

      case fptoi_node_K:
      case min_node_K:
         return "";
      case abs_node_K:
      case call_node_K:
      case constructor_node_K:
      case itofp_node_K:
      case identifier_node_K:
      case max_node_K:
      case mem_access_node_K:
      case nop_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case bitcast_node_K:
      case extract_bit_node_K:
      case add_sat_node_K:
      case sub_sat_node_K:
      case extractvalue_node_K:
      case extractelement_node_K:
      case frem_node_K:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("op_symbol not yet supported ") + op->get_kind_text() + " in node " + STR(op->index));
         return "";
   }
}

unsigned long long ir_helper::GetArrayElementSize(const ir_nodeConstRef& node)
{
   if(node->get_kind() == struct_ty_node_K)
   {
      const auto rt = GetPointerS<const struct_ty_node>(node);
      auto fd = rt->list_of_flds[0];
      THROW_ASSERT(fd->get_kind() == field_val_node_K, "expected a field_val_node");
      return GetArrayElementSize(GetPointerS<const field_val_node>(fd)->type);
   }
   if(node->get_kind() != array_ty_node_K)
   {
      return SizeAlloc(node);
   }
   const auto at = GetPointerS<const array_ty_node>(node);
   THROW_ASSERT(at->elts, "elements type expected");
   const auto elts = at->elts;
   unsigned long long return_value;
   if(elts->get_kind() == array_ty_node_K)
   {
      return_value = GetArrayElementSize(at->elts);
   }
   else
   {
      const auto type = CGetType(at->elts);
      return_value = SizeAlloc(type);
      const auto fd = GetPointer<const field_val_node>(type);
      if(!fd || !fd->bitfield)
      {
         return_value = std::max(8ull, return_value);
      }
   }
   return return_value;
}

void ir_helper::get_array_dim_and_bitsize(const ir_managerConstRef& TM, const unsigned int index,
                                          std::vector<unsigned long long>& dims, unsigned long long& elts_bitsize)
{
   ir_nodeRef node = TM->GetIRNode(index);
   if(node->get_kind() == struct_ty_node_K)
   {
      elts_bitsize = GetArrayElementSize(node);
      dims.push_back(SizeAlloc(node) / elts_bitsize);
      return;
   }
   THROW_ASSERT(node->get_kind() == array_ty_node_K, "array_ty_node expected: @" + STR(index));
   const auto at = GetPointer<array_ty_node>(node);
   if(!at->nelements)
   {
      dims.push_back(1); // at least one element is expected
   }
   else
   {
      dims.push_back(at->nelements);
   }
   THROW_ASSERT(at->elts, "elements type expected");
   ir_nodeRef elts = at->elts;
   if(elts->get_kind() == array_ty_node_K)
   {
      get_array_dim_and_bitsize(TM, at->elts->index, dims, elts_bitsize);
   }
   else
   {
      const auto etype = CGetType(at->elts);
      elts_bitsize = SizeAlloc(etype);
      const auto fd = GetPointer<const field_val_node>(etype);
      if(!fd || !fd->bitfield)
      {
         elts_bitsize = std::max(8ull, elts_bitsize);
      }
   }
}

std::vector<unsigned long long> ir_helper::GetArrayDimensions(const ir_nodeConstRef& node)
{
   std::vector<unsigned long long> dims;
   std::function<void(const ir_nodeConstRef&)> get_array_dim_recurse;
   get_array_dim_recurse = [&](const ir_nodeConstRef& tn) -> void {
      if(tn->get_kind() == struct_ty_node_K)
      {
         auto elmt_bitsize = GetArrayElementSize(tn);
         dims.push_back(SizeAlloc(tn) / elmt_bitsize);
         return;
      }
      THROW_ASSERT(tn->get_kind() == array_ty_node_K, "array_ty_node expected: @" + STR(tn));
      const auto at = GetPointerS<const array_ty_node>(tn);
      dims.push_back(at->nelements);
      THROW_ASSERT(at->elts, "elements type expected");
      const auto elts = at->elts;
      if(elts->get_kind() == array_ty_node_K)
      {
         get_array_dim_recurse(at->elts);
      }
   };
   get_array_dim_recurse(node);
   return dims;
}

unsigned long long ir_helper::GetArrayTotalSize(const ir_nodeConstRef& node)
{
   auto num_elements = 1ull;
   for(const auto& i : GetArrayDimensions(node))
   {
      num_elements *= i;
   }
   return num_elements;
}

unsigned int ir_helper::get_var_alignment(const ir_managerConstRef& TM, unsigned int var)
{
   const auto varnode = TM->GetIRNode(var);
   const auto vd = GetPointer<const variable_val_node>(varnode);
   if(vd)
   {
      return vd->algn < 8 ? 1 : (vd->algn / 8);
   }
   return 1;
}

std::string ir_helper::NormalizeTypename(const std::string& id)
{
   static const std::regex rbase("[.:$]+");
   static const std::regex rtmpl("[*&<>\\-]|[, ]+");
   std::string norm_typename;
   std::regex_replace(std::back_inserter(norm_typename), id.cbegin(), id.cend(), rbase, "_");
   const auto tmpl_start = norm_typename.find_first_of('<');
   if(tmpl_start != std::string::npos)
   {
      const auto tmpl_end = norm_typename.find_last_of('>');
      THROW_ASSERT(tmpl_end != std::string::npos, "");
      auto norm_template = norm_typename.substr(0, tmpl_start);
      std::regex_replace(std::back_inserter(norm_template), norm_typename.cbegin() + static_cast<long int>(tmpl_start),
                         norm_typename.cbegin() + static_cast<long int>(tmpl_end + 1U), rtmpl, "_");
      return norm_template;
   }
   return norm_typename;
}

std::string ir_helper::PrintType(const ir_nodeConstRef& original_type, bool print_qualifiers, bool print_storage,
                                 const ir_nodeConstRef& var, const std::unique_ptr<var_pp_functor>& vppf,
                                 const std::string& prefix, const std::string& tail)
{
   bool skip_var_printing = false;
   const auto node_type = (original_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Printing type " + STR(original_type) + "(" + STR(node_type) + ") - Var " + STR(var));
   std::string res;
   if(var)
   {
      if(var->get_kind() == variable_val_node_K)
      {
         const auto vd = GetPointerS<const variable_val_node>(var);
         if(print_storage)
         {
            if(vd->extern_flag)
            {
               res += "extern ";
            }
            if(vd->static_flag)
            {
               res += "static ";
            }
         }
      }
   }
   switch(node_type->get_kind())
   {
      case function_val_node_K:
      {
         const auto fd = GetPointerS<const function_val_node>(node_type);
         const auto function_name = GetFunctionName(node_type);
         if(!fd->body)
         {
            res = "extern ";
         }
         else if(fd->static_flag)
         {
            res = "static ";
         }
         else if(fd->mngl && function_name != "main")
         {
            res = "\n#ifdef __cplusplus\nextern \"C\"\n#endif\n";
         }
         const auto dn = GetPointer<const decl_node>(node_type);
         THROW_ASSERT(dn, "expected a declaration node");
         ir_nodeRef ftype = dn->type;
         /* Print type declaration.  */
         if(ftype->get_kind() == function_ty_node_K)
         {
            const auto ft = GetPointerS<const function_ty_node>(ftype);
            res += PrintType(ft->retn);
         }
         else
         {
            THROW_ERROR(std::string("IR node not currently supported ") + node_type->get_kind_text());
         }
         res += " ";

         /* Print function name.  */
         THROW_ASSERT(dn->name, "expected a name");
         res += function_name;
         res += "(";
         if(!fd->list_of_args.empty())
         {
            for(unsigned int i = 0; i < (fd->list_of_args.size()); i++)
            {
               if(i > 0)
               {
                  res += ", ";
               }
               res +=
                   PrintType(CGetType(fd->list_of_args[i]), print_qualifiers, print_storage, fd->list_of_args[i], vppf);
            }
         }
         else
         {
            const auto ft = GetPointerS<const function_ty_node>(ftype);
            bool first_p = false;
            for(const auto& p : ft->list_of_args_type)
            {
               if(!first_p)
               {
                  first_p = true;
               }
               else
               {
                  res += ",";
               }
               res += PrintType(p, print_qualifiers);
            }
         }

         if((!fd->list_of_args.empty() || !GetPointerS<const function_ty_node>(ftype)->list_of_args_type.empty()) &&
            GetPointerS<const function_ty_node>(ftype)->varargs_flag)
         {
            res += ", ... ";
         }
         res += ")";
         break;
      }
      case identifier_node_K:
      {
         const auto in = GetPointerS<const identifier_node>(node_type);
         res += NormalizeTypename(in->strg);
         skip_var_printing = true;
         break;
      }
      case void_ty_node_K:
      case integer_ty_node_K:
      case real_ty_node_K:
      case vector_ty_node_K:
      {
         const auto tn = GetPointerS<const type_node>(node_type);
         /* const internally are not considered as constant...*/
         if(node_type->get_kind() == vector_ty_node_K)
         {
            const auto vt = GetPointerS<const vector_ty_node>(node_type);

            // THROW_ERROR(std::string("Node not yet supported:<unnamed type> ") +
            // node_type->get_kind_text()+STR(type));
            THROW_ASSERT(vt->elts, "expected the type of the elements of the vector");
            res += PrintType(vt->elts);
            const auto vector_size = [&]() -> unsigned int {
               unsigned int v = vt->algn / 8;
               v--;
               v |= v >> 1;
               v |= v >> 2;
               v |= v >> 4;
               v |= v >> 8;
               v |= v >> 16;
               v++;
               return v;
            }();
            res += " __attribute__((vector_size(" + STR(vector_size) + ")))";
         }
         else
         {
            switch(node_type->get_kind())
            {
               case integer_ty_node_K:
               {
                  const auto it = GetPointerS<const integer_ty_node>(node_type);
                  if(it->unsigned_flag)
                  {
                     res += "unsigned ";
                  }
                  if(it->bitsize != tn->algn)
                  {
                     if(it->bitsize > 64)
                     {
                        res += "int __attribute__((vector_size(16)))";
                     }
                     else if(it->bitsize > 32)
                     {
                        res += "long long int";
                     }
                     else if(it->bitsize > 16)
                     {
                        res += "int";
                     }
                     else if(it->bitsize > 8)
                     {
                        res += "short";
                     }
                     else
                     {
                        res += "char";
                     }
                  }
                  else if(tn->algn == 8)
                  {
                     res += "char";
                  }
                  else if(tn->algn == 16)
                  {
                     res += "short";
                  }
                  else if(tn->algn == 32)
                  {
                     res += "int";
                  }
                  else if(tn->algn == 64)
                  {
                     res += "long long";
                  }
                  else if(tn->algn == 128)
                  {
                     res += "int __attribute__((vector_size(16)))";
                  }
                  else
                  {
                     res += "_BitInt(" + STR(tn->algn) + ")";
                  }
                  break;
               }
               case void_ty_node_K:
                  res += "void";
                  break;
               case real_ty_node_K:
               {
                  const auto rt = GetPointerS<const real_ty_node>(node_type);
                  if(rt->bitsize == 32)
                  {
                     res += "float";
                  }
                  else if(rt->bitsize == 64)
                  {
                     res += "double";
                  }
                  else if(rt->bitsize == 80)
                  {
                     res += "__float80";
                  }
                  else if(rt->bitsize == 128)
                  {
                     res += "__float128";
                  }
                  else
                  {
                     THROW_ERROR(std::string("Real type not yet supported ") + STR(original_type));
                  }
                  break;
               }
               case array_ty_node_K:
               case call_node_K:
               case constructor_node_K:
               case function_ty_node_K:
               case identifier_node_K:
               case pointer_ty_node_K:
               case struct_ty_node_K:
               case ssa_node_K:
               case statement_list_node_K:
               case vector_ty_node_K:
               case lut_node_K:
               case CASE_BINARY_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_NODE_STMTS:
               case CASE_TERNARY_NODES:
               case CASE_UNARY_NODES:
               default:
                  THROW_ERROR(std::string("Node not yet supported ") + node_type->get_kind_text() + " " +
                              STR(original_type));
            }
         }
         break;
      }
      case pointer_ty_node_K:
      {
         THROW_ASSERT(node_type->get_kind() == pointer_ty_node_K, "unexpected case");

         const auto ir_type = GetPointerS<const pointer_ty_node>(node_type);
         res = "*";
         /* const internally are not considered as constant...*/
         res = PrintType(ir_type->ptd, print_qualifiers, print_storage, var, vppf, prefix + res, tail);
         skip_var_printing = true;
         break;
      }
      case function_ty_node_K:
      {
         const auto ft = GetPointerS<const function_ty_node>(node_type);
         res += PrintType(ft->retn, true);
         res += "(" + prefix;
         if(var)
         {
            THROW_ASSERT(vppf, "expected a functor");
            res += " " + (*vppf)(var->index);
         }
         res += tail + ")(";
         bool first_p = false;
         for(const auto& p : ft->list_of_args_type)
         {
            if(!first_p)
            {
               first_p = true;
            }
            else
            {
               res += ",";
            }
            res += PrintType(p, print_qualifiers);
         }
         if(ft->varargs_flag && !ft->list_of_args_type.empty())
         {
            res += ", ... ";
         }
         else if(ft->varargs_flag)
         {
            THROW_ERROR("ISO C requires a named parameter before '...'");
         }
         res += ")";
         skip_var_printing = true;
         break;
      }
      case array_ty_node_K:
      {
         const auto at = GetPointerS<const array_ty_node>(node_type);

         std::string local_prefix;
         std::string local_tail;
         /* Print array's type */
         /// Compute the dimensions
         if(at->bitsizealloc)
         {
            const auto tn = GetPointerS<const type_node>(at->elts);
            local_tail += "[";
            local_tail += STR(at->bitsizealloc / tn->bitsizealloc);
            local_tail += "]";
         }
         else
         {
            local_tail += "[]";
         }
         if(!prefix.empty())
         {
            local_prefix += "(" + prefix;
         }
         if(var)
         {
            THROW_ASSERT(vppf, "expected a functor");
            local_prefix += " " + (*vppf)(var->index);
         }
         if(!prefix.empty())
         {
            local_tail = ")" + local_tail;
         }

         res += PrintType(at->elts, print_qualifiers, print_storage, nullptr, nullptr, "",
                          local_prefix + tail + local_tail);
         /// add alignment
         if(var && var->get_kind() == field_val_node_K)
         {
            unsigned int type_align = at->algn;
            unsigned int var_align;
            bool is_a_pointerP = false;
            bool is_static = false;
            switch(var->get_kind())
            {
               case field_val_node_K:
               {
                  const auto fd = GetPointerS<const field_val_node>(var);
                  var_align = fd->algn;
                  is_a_pointerP = fd->type->get_kind() == pointer_ty_node_K;
                  break;
               }
               case argument_val_node_K:
                  // var_align = GetPointer<argument_val_node>(var)->algn;
                  var_align = type_align;
                  break;
               case variable_val_node_K:
               {
                  const auto vd = GetPointerS<const variable_val_node>(var);
                  var_align = vd->algn;
                  is_a_pointerP = vd->type->get_kind() == pointer_ty_node_K;
                  is_static = vd->static_flag;
                  break;
               }
               case call_node_K:
               case constructor_node_K:
               case function_val_node_K:
               case identifier_node_K:
               case ssa_node_K:
               case statement_list_node_K:
               case module_unit_node_K:
               case lut_node_K:
               case CASE_BINARY_NODES:
               case CASE_CST_NODES:
               case CASE_FAKE_NODES:
               case CASE_NODE_STMTS:
               case CASE_TERNARY_NODES:
               case CASE_TYPE_NODES:
               case CASE_UNARY_NODES:
               default:
                  var_align = type_align;
                  break;
            }
            if(var_align > type_align && !is_a_pointerP && !is_static)
            {
               res += " __attribute__((aligned(" + STR(var_align / 8) + ")))";
            }
         }
         skip_var_printing = true;

         break;
      }
      case struct_ty_node_K:
      {
         const auto rt = GetPointerS<const struct_ty_node>(node_type);
         if(rt->name)
         {
            const auto struct_name = PrintType(rt->name);
            if(struct_name == "_IO_FILE")
            {
               res += "FILE";
            }
            else
            {
               res += "struct " + NormalizeTypename(struct_name);
            }
         }
         else
         {
            res += "struct Internal_" + STR(node_type->index);
         }
         break;
      }
      case argument_val_node_K:
      {
         const auto pd = GetPointer<const argument_val_node>(node_type);
         if(pd->readonly_flag)
         {
            res += print_qualifiers ? "const " : "/*const*/ ";
         }
         res += PrintType(pd->type, print_qualifiers);
         break;
      }
      case call_node_K:
      case constructor_node_K:
      case field_val_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case variable_val_node_K:
      case shufflevector_node_K:
      case select_node_K:
      case ternary_add_node_K:
      case ternary_as_node_K:
      case ternary_sa_node_K:
      case ternary_ss_node_K:
      case fshl_node_K:
      case fshr_node_K:
      case concat_bit_node_K:
      case lut_node_K:
      case insertvalue_node_K:
      case insertelement_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_UNARY_NODES:
      default:
         THROW_UNREACHABLE("Type not yet supported " + STR(original_type) + " " + node_type->get_kind_text() + " " +
                           (var ? STR(var) : ""));
   }
   if(!skip_var_printing)
   {
      res += prefix;
      if(var)
      {
         THROW_ASSERT(vppf, "expected a functor");
         res += " " + (*vppf)(var->index);
      }
      res += tail;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Printed type " + STR(original_type) + ": " + res);
   return res;
}

FunctionExpander::serialization FunctionExpander::get_serialization(const std::string& name) const
{
   if(no_serialize.find(name) != no_serialize.end())
   {
      return none;
   }
   if(internal_serialize.find(name) != internal_serialize.end())
   {
      return internal;
   }
   return total;
}

bool FunctionExpander::is_transparent(const std::string& name) const
{
   return (transparent.find(name) != transparent.end());
}

void FunctionExpander::check_lib_type(const ir_nodeRef& var)
{
   THROW_ASSERT(GetPointer<decl_node>(var), "Checking type of not a decl_node");
   const auto dn = GetPointer<decl_node>(var);
   std::string include_name = dn->include_name;
   auto it_end = headers.end();
   for(auto it = headers.begin(); it != it_end; ++it)
   {
      if(include_name.find(*it) != std::string::npos && dn->type)
      {
         if(GetPointer<type_node>(dn->type))
         {
            lib_types.insert(dn->type);
         }
      }
   }
}

bool FunctionExpander::operator()(const ir_nodeRef& tn) const
{
   THROW_ASSERT(GetPointer<type_node>(tn) || GetPointer<function_val_node>(tn),
                "tn is not a node of type type_node nor function_val_node");
   if(lib_types.find(tn) != lib_types.end())
   {
      return false;
   }
   return true;
}

FunctionExpander::FunctionExpander()
{
   internal_serialize.insert("printf");
   transparent.insert("__builtin_va_start");
   headers.insert("stdio.h");
}

ir_nodeConstRef ir_helper::GetFormalIth(const ir_nodeConstRef& obj, unsigned int parm_index)
{
   if(obj->get_kind() == call_stmt_K)
   {
      const auto* gc = GetPointerS<const call_stmt>(obj);
      THROW_ASSERT(gc->fn, "unexpected condition");

      const auto fn_type = CGetType(gc->fn);
      if(fn_type->get_kind() == pointer_ty_node_K)
      {
         const auto* pt = GetPointerS<const pointer_ty_node>(fn_type);
         THROW_ASSERT(pt->ptd, "unexpected pattern");
         const auto* ft = GetPointer<const function_ty_node>(pt->ptd);
         if(ft && ft->varargs_flag)
         {
            return {};
         }
         if(ft && !ft->list_of_args_type.empty())
         {
            unsigned int ith = 0;
            for(const auto& p : ft->list_of_args_type)
            {
               if(parm_index == ith)
               {
                  return p;
               }
               ++ith;
            }
            THROW_UNREACHABLE("unexpected pattern");
            return {};
         }
         /// parameters are not available through function_ty_node but only through function_val_node
         THROW_ASSERT(gc->fn->get_kind() == addr_node_K, "Unexpected pattern");
         const auto* ue = GetPointerS<const unary_node>(gc->fn);
         THROW_ASSERT(ue->op->get_kind() == function_val_node_K, "Unexpected pattern");
         return GetFormalIth(ue->op, parm_index);
      }
      THROW_UNREACHABLE("unexpected pattern");
      return {};
   }
   if(obj->get_kind() == assign_stmt_K)
   {
      const auto* ga = GetPointerS<const assign_stmt>(obj);
      return GetFormalIth(ga->op1, parm_index);
   }
   if(obj->get_kind() == call_node_K)
   {
      const auto* ce = GetPointerS<const call_node>(obj);
      const auto fn_type = CGetType(ce->fn);
      if(fn_type->get_kind() == pointer_ty_node_K)
      {
         const auto* pt = GetPointerS<const pointer_ty_node>(fn_type);
         THROW_ASSERT(pt->ptd, "unexpected pattern");
         const auto* ft = GetPointer<const function_ty_node>(pt->ptd);
         if(ft && ft->varargs_flag)
         {
            return {};
         }
         if(ft && !ft->list_of_args_type.empty())
         {
            unsigned int ith = 0;
            for(const auto& p : ft->list_of_args_type)
            {
               if(parm_index == ith)
               {
                  return p;
               }
               ++ith;
            }
            THROW_UNREACHABLE("unexpected pattern");
            return {};
         }
         /// parameters are not available through function_ty_node but only through function_val_node
         THROW_ASSERT(ce->fn->get_kind() == addr_node_K, "Unexpected pattern");
         const auto* ue = GetPointerS<const unary_node>(ce->fn);
         THROW_ASSERT(ue->op->get_kind(), "Unexpected pattern");
         return GetFormalIth(ue->op, parm_index);
      }
      THROW_UNREACHABLE("unexpected pattern");
      return {};
   }
   if(obj->get_kind() == function_val_node_K)
   {
      const auto* fd = GetPointerS<const function_val_node>(obj);
      unsigned int ith = 0;
      for(const auto& i : fd->list_of_args)
      {
         if(parm_index == ith)
         {
            return CGetType(i);
         }
         ++ith;
      }
      THROW_UNREACHABLE("index out of bounds: " + std::to_string(ith) + "/" + std::to_string(fd->list_of_args.size()) +
                        " @" + std::to_string(fd->index));
   }
   return {};
}

bool ir_helper::IsPackedType(const ir_nodeConstRef& type)
{
   THROW_ASSERT(GetPointer<const decl_node>(type), "unexpected pattern" + type->get_kind_text());
   auto node_type = GetPointer<const decl_node>(type)->type;
   switch(node_type->get_kind())
   {
      case struct_ty_node_K:
      {
         auto rt = GetPointerS<const struct_ty_node>(node_type);
         if(rt->packed_flag)
         {
            return true;
         }
         for(auto& list_of_fld : rt->list_of_flds)
         {
            const auto fd = GetPointer<const field_val_node>(list_of_fld);
            if(fd && fd->packed_flag)
            {
               return true;
            }
         }
         break;
      }
      case array_ty_node_K:
      case pointer_ty_node_K:
      case function_ty_node_K:
      case integer_ty_node_K:
      case real_ty_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
      {
         break;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
      {
         THROW_UNREACHABLE("Unexpected type " + node_type->get_kind_text());
      }
   }
   return false;
}

bool ir_helper::IsPackedAccess(const ir_nodeConstRef& tn)
{
   bool res = false;
   switch(tn->get_kind())
   {
      case mem_access_node_K:
      {
         const auto mr = GetPointerS<const mem_access_node>(tn);
         return IsPackedAccess(mr->op);
      }
      case addr_node_K:
      {
         const auto ae = GetPointerS<const addr_node>(tn);
         return IsPackedAccess(ae->op);
      }
      case ssa_node_K:
      {
         const auto sn = GetPointerS<const ssa_node>(tn);
         const auto def_stmt = sn->GetDefStmt();
         if(def_stmt->get_kind() == assign_stmt_K)
         {
            const auto ga = GetPointer<const assign_stmt>(def_stmt);
            if(ga->temporary_address)
            {
               return IsPackedAccess(ga->op1);
            }
         }
         break;
      }
      case extractelement_node_K:
      case extractvalue_node_K:
      case insertelement_node_K:
      case insertvalue_node_K:
      {
         res = true;
         break;
      }
      case abs_node_K:
      case and_node_K:
      case concat_bit_node_K:
      case or_node_K:
      case not_node_K:
      case xor_node_K:
      case call_node_K:
      case select_node_K:
      case constructor_node_K:
      case eq_node_K:
      case extract_bit_node_K:
      case field_val_node_K:
      case fptoi_node_K:
      case itofp_node_K:
      case frem_node_K:
      case fshl_node_K:
      case fshr_node_K:
      case function_val_node_K:
      case ge_node_K:
      case gt_node_K:
      case identifier_node_K:
      case le_node_K:
      case shl_node_K:
      case lt_node_K:
      case lut_node_K:
      case max_node_K:
      case min_node_K:
      case sub_node_K:
      case unaligned_mem_access_node_K:
      case mul_node_K:
      case ne_node_K:
      case neg_node_K:
      case nop_node_K:
      case argument_val_node_K:
      case add_node_K:
      case gep_node_K:
      case fdiv_node_K:
      case shr_node_K:
      case sub_sat_node_K:
      case add_sat_node_K:
      case statement_list_node_K:
      case ternary_ss_node_K:
      case ternary_sa_node_K:
      case ternary_add_node_K:
      case ternary_as_node_K:
      case module_unit_node_K:
      case idiv_node_K:
      case irem_node_K:
      case variable_val_node_K:
      case shufflevector_node_K:
      case bitcast_node_K:
      case widen_mul_node_K:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TYPE_NODES:
      {
         break;
      }
      default:
         THROW_ERROR("elements not yet supported: " + tn->get_kind_text());
   }

   return res;
}

unsigned long long ir_helper::AccessedMaximumBitsize(const ir_nodeConstRef& type_node, unsigned long long bitsize)
{
   switch(type_node->get_kind())
   {
      case array_ty_node_K:
      {
         const auto atype = GetPointerS<const array_ty_node>(type_node);
         return AccessedMaximumBitsize(atype->elts, bitsize);
      }
      case struct_ty_node_K:
      {
         const auto rt = GetPointerS<const struct_ty_node>(type_node);
         for(const auto& fli : rt->list_of_flds)
         {
            if(fli->get_kind() == function_val_node_K)
            {
               continue;
            }
            if(fli->get_kind() == variable_val_node_K)
            {
               bitsize = AccessedMaximumBitsize(GetPointerS<const variable_val_node>(fli)->type, bitsize);
            }
            else
            {
               bitsize = AccessedMaximumBitsize(fli, bitsize);
            }
         }
         return bitsize;
      }
      case field_val_node_K:
      {
         const auto fd_type_node = CGetType(type_node);
         return AccessedMaximumBitsize(fd_type_node, bitsize);
      }
      case real_ty_node_K:
      case integer_ty_node_K:
      case pointer_ty_node_K:
      case void_ty_node_K:
      case vector_ty_node_K:
      {
         return std::max(bitsize, SizeAlloc(type_node));
      }
      case function_val_node_K:
      case function_ty_node_K:
      {
         return 32;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case argument_val_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case variable_val_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
         THROW_ERROR("elements not yet supported: " + type_node->get_kind_text() + " " + STR(type_node->index));
   }
   return 0;
}

unsigned long long ir_helper::AccessedMinimunBitsize(const ir_nodeConstRef& type_node, unsigned long long bitsize)
{
   switch(type_node->get_kind())
   {
      case array_ty_node_K:
      {
         const auto atype = GetPointerS<const array_ty_node>(type_node);
         return AccessedMinimunBitsize(atype->elts, bitsize);
      }
      case struct_ty_node_K:
      {
         const auto rt = GetPointerS<const struct_ty_node>(type_node);
         for(const auto& fli : rt->list_of_flds)
         {
            if(fli->get_kind() == function_val_node_K)
            {
               continue;
            }
            if(fli->get_kind() == variable_val_node_K)
            {
               bitsize = AccessedMinimunBitsize(GetPointerS<const variable_val_node>(fli)->type, bitsize);
            }
            else
            {
               bitsize = AccessedMinimunBitsize(fli, bitsize);
            }
         }
         return bitsize;
      }
      case field_val_node_K:
      {
         const auto fd_type_node = CGetType(type_node);
         return AccessedMinimunBitsize(fd_type_node, bitsize);
      }
      case real_ty_node_K:
      case integer_ty_node_K:
      case pointer_ty_node_K:
      case void_ty_node_K:
      case vector_ty_node_K:
      {
         return std::min(bitsize, SizeAlloc(type_node));
      }
      case call_node_K:
      case constructor_node_K:
      case function_val_node_K:
      case function_ty_node_K:
      case identifier_node_K:
      case argument_val_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case variable_val_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
         THROW_ERROR("elements not yet supported: " + type_node->get_kind_text());
   }
   return 0;
}

size_t ir_helper::AllocatedMemorySize(const ir_nodeConstRef& parameter)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Analyzing " + parameter->ToString());
   switch(parameter->get_kind())
   {
      case(addr_node_K):
      {
         const auto ae = GetPointer<const addr_node>(parameter);
         /// Note that this part can not be transfromed in recursion because size of array ref corresponds to the
         /// size of the element itself
         const ir_nodeRef addr_node_argument = ae->op;
         switch(addr_node_argument->get_kind())
         {
            case(mem_access_node_K):
            case(argument_val_node_K):
            case(variable_val_node_K):
            {
               const size_t byte_parameter_size = AllocatedMemorySize(addr_node_argument);
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
               return byte_parameter_size;
            }
            case call_node_K:
            case constructor_node_K:
            case identifier_node_K:
            case constant_int_val_node_K:
            case constant_fp_val_node_K:
            case ssa_node_K:
            case statement_list_node_K:
            case constant_vector_val_node_K:
            case field_val_node_K:
            case function_val_node_K:
            case module_unit_node_K:
            case select_node_K:
            case ternary_add_node_K:
            case ternary_as_node_K:
            case ternary_sa_node_K:
            case ternary_ss_node_K:
            case fshl_node_K:
            case fshr_node_K:
            case concat_bit_node_K:
            case and_node_K:
            case or_node_K:
            case xor_node_K:
            case eq_node_K:
            case ge_node_K:
            case gt_node_K:
            case le_node_K:
            case shl_node_K:
            case lt_node_K:
            case lut_node_K:
            case max_node_K:
            case min_node_K:
            case sub_node_K:
            case mul_node_K:
            case ne_node_K:
            case add_node_K:
            case gep_node_K:
            case fdiv_node_K:
            case shr_node_K:
            case idiv_node_K:
            case irem_node_K:
            case widen_mul_node_K:
            case shufflevector_node_K:
            case extract_bit_node_K:
            case add_sat_node_K:
            case sub_sat_node_K:
            case extractvalue_node_K:
            case insertvalue_node_K:
            case extractelement_node_K:
            case insertelement_node_K:
            case frem_node_K:
            case CASE_FAKE_NODES:
            case CASE_NODE_STMTS:
            case CASE_TYPE_NODES:
            case abs_node_K:
            case addr_node_K:
            case not_node_K:
            case fptoi_node_K:
            case unaligned_mem_access_node_K:
            case neg_node_K:
            case nop_node_K:
            case bitcast_node_K:
            case itofp_node_K:
            default:
            {
               THROW_UNREACHABLE("Unsupported addr_node argument " + addr_node_argument->get_kind_text());
            }
         }
         break;
      }
      case(array_ty_node_K):
      {
         const auto at = GetPointer<const array_ty_node>(parameter);
         /// This call check if we can perform deep copy of the single element
         AllocatedMemorySize(at->elts);
         const size_t bit_parameter_size = SizeAlloc(parameter);
         /// Round to upper multiple word size
         const size_t byte_parameter_size = bit_parameter_size / 8;
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(struct_ty_node_K):
      {
         size_t fields_pointed_size = 0;
         const auto rt = GetPointer<const struct_ty_node>(parameter);
         const std::vector<ir_nodeRef>& list_of_fields = rt->list_of_flds;
         /// This calls check if we can perform deep copy of the single element
         std::vector<ir_nodeRef>::const_iterator field, field_end = list_of_fields.end();
         for(field = list_of_fields.begin(); field != field_end; ++field)
         {
            if((*field)->get_kind() == function_val_node_K)
            {
               continue;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Analyzing field " + (*field)->ToString());
            AllocatedMemorySize(CGetType(*field));
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Analyzed field " + (*field)->ToString());
         }
         const size_t bit_parameter_size = SizeAlloc(parameter) + fields_pointed_size;
         /// Round to upper multiple word size
         const size_t byte_parameter_size = bit_parameter_size / 8;
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(field_val_node_K):
      {
         const size_t byte_parameter_size = AllocatedMemorySize(CGetType(parameter));
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(integer_ty_node_K):
      case(real_ty_node_K):
      {
         const size_t bit_parameter_size = SizeAlloc(parameter);
         /// Round to upper multiple word size
         const size_t byte_parameter_size = bit_parameter_size / 8;
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(mem_access_node_K):
      {
         const auto mr = GetPointer<const mem_access_node>(parameter);
         const size_t byte_parameter_size = AllocatedMemorySize(mr->op);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(argument_val_node_K):
      case(ssa_node_K):
      case(variable_val_node_K):
      {
         THROW_ASSERT(GetPointer<const ssa_node>(parameter)->GetDefStmt(), "unexpected condition");
         const auto ptype = CGetType(parameter);
         const auto byte_parameter_size = AllocatedMemorySize(IsPointerType(ptype) ? CGetPointedType(ptype) : ptype);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case call_node_K:
      case select_node_K:
      case constructor_node_K:
      case ternary_add_node_K:
      case ternary_as_node_K:
      case ternary_sa_node_K:
      case ternary_ss_node_K:
      case fshl_node_K:
      case fshr_node_K:
      case concat_bit_node_K:
      case function_val_node_K:
      case identifier_node_K:
      case constant_int_val_node_K:
      case pointer_ty_node_K:
      case constant_fp_val_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case constant_vector_val_node_K:
      case void_ty_node_K:
      case function_ty_node_K:
      case vector_ty_node_K:
      case abs_node_K:
      case not_node_K:
      case fptoi_node_K:
      case itofp_node_K:
      case unaligned_mem_access_node_K:
      case lut_node_K:
      case neg_node_K:
      case nop_node_K:
      case bitcast_node_K:
      case and_node_K:
      case or_node_K:
      case xor_node_K:
      case eq_node_K:
      case ge_node_K:
      case gt_node_K:
      case le_node_K:
      case shl_node_K:
      case lt_node_K:
      case max_node_K:
      case min_node_K:
      case sub_node_K:
      case mul_node_K:
      case ne_node_K:
      case add_node_K:
      case gep_node_K:
      case fdiv_node_K:
      case shr_node_K:
      case idiv_node_K:
      case irem_node_K:
      case shufflevector_node_K:
      case widen_mul_node_K:
      case extract_bit_node_K:
      case add_sat_node_K:
      case sub_sat_node_K:
      case extractvalue_node_K:
      case insertvalue_node_K:
      case extractelement_node_K:
      case insertelement_node_K:
      case frem_node_K:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      default:
      {
         THROW_UNREACHABLE("Unsupported IR node type " + parameter->get_kind_text() + " (" + parameter->ToString() +
                           ")");
      }
   }
   return 0;
}

size_t ir_helper::CountPointers(const ir_nodeConstRef& tn)
{
   size_t counter = 0;
   switch(tn->get_kind())
   {
      case integer_ty_node_K:
      case real_ty_node_K:
      {
         return 0;
      }
      case field_val_node_K:
      case argument_val_node_K:
      {
         return CountPointers(CGetType(tn));
      }
      case pointer_ty_node_K:
      {
         return 1;
      }
      case struct_ty_node_K:
      {
         const auto rt = GetPointer<const struct_ty_node>(tn);
         const std::vector<ir_nodeRef> list_of_fields = rt->list_of_flds;
         std::vector<ir_nodeRef>::const_iterator field, field_end = list_of_fields.end();
         for(field = list_of_fields.begin(); field != field_end; ++field)
         {
            if((*field)->get_kind() == function_val_node_K)
            {
               continue;
            }
            counter += CountPointers(*field);
         }
         return counter;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case function_val_node_K:
      case module_unit_node_K:
      case variable_val_node_K:
      case array_ty_node_K:
      case function_ty_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_UNARY_NODES:
      default:
      {
         THROW_UNREACHABLE("Unsupported type node " + tn->get_kind_text());
      }
   }
   return counter;
}

unsigned int ir_helper::get_multi_way_if_pos(const ir_managerConstRef& TM, unsigned int node_id,
                                             unsigned int looked_for_cond)
{
   const auto t = TM->GetIRNode(node_id);
   const auto gmwi = GetPointer<const multi_way_if_stmt>(t);
   unsigned int pos = 0;
   for(auto const& cond : gmwi->list_of_cond)
   {
      if(cond.first && cond.first->index == looked_for_cond)
      {
         return pos;
      }
      pos++;
   }
   THROW_ERROR("cond not found in multi_way_if_stmt " + t->ToString() + " looked_for_cond " + STR(looked_for_cond));
   return pos;
}

void ir_helper::compute_ssa_uses_rec_ptr(const ir_nodeConstRef& curr_tn, CustomOrderedSet<const ssa_node*>& ssa_uses)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->compute_ssa_uses_rec_ptr " + curr_tn->ToString());
   const auto gn = GetPointer<const node_stmt>(curr_tn);
   if(gn)
   {
      if(gn->memuse)
      {
         compute_ssa_uses_rec_ptr(gn->memuse, ssa_uses);
      }
      if(!gn->vuses.empty())
      {
         for(const auto& vuse : gn->vuses)
         {
            compute_ssa_uses_rec_ptr(vuse, ssa_uses);
         }
      }
   }
   switch(curr_tn->get_kind())
   {
      case return_stmt_K:
      {
         const auto re = GetPointer<const return_stmt>(curr_tn);
         if(re->op)
         {
            compute_ssa_uses_rec_ptr(re->op, ssa_uses);
         }
         break;
      }
      case assign_stmt_K:
      {
         const auto me = GetPointer<const assign_stmt>(curr_tn);
         if(me->op0->get_kind() != ssa_node_K)
         {
            compute_ssa_uses_rec_ptr(me->op0, ssa_uses);
         }
         compute_ssa_uses_rec_ptr(me->op1, ssa_uses);
         if(me->predicate)
         {
            compute_ssa_uses_rec_ptr(me->predicate, ssa_uses);
         }
         break;
      }
      case nop_stmt_K:
      {
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointer<const call_node>(curr_tn);
         compute_ssa_uses_rec_ptr(ce->fn, ssa_uses);
         for(const auto& arg : ce->args)
         {
            compute_ssa_uses_rec_ptr(arg, ssa_uses);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto ce = GetPointer<const call_stmt>(curr_tn);
         if(ce->predicate)
         {
            compute_ssa_uses_rec_ptr(ce->predicate, ssa_uses);
         }
         compute_ssa_uses_rec_ptr(ce->fn, ssa_uses);
         for(const auto& arg : ce->args)
         {
            compute_ssa_uses_rec_ptr(arg, ssa_uses);
         }
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointer<const unary_node>(curr_tn);
         compute_ssa_uses_rec_ptr(ue->op, ssa_uses);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointer<const binary_node>(curr_tn);
         compute_ssa_uses_rec_ptr(be->op0, ssa_uses);
         compute_ssa_uses_rec_ptr(be->op1, ssa_uses);
         break;
      }
      /*ternary expressions*/
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointer<const ternary_node>(curr_tn);
         compute_ssa_uses_rec_ptr(te->op0, ssa_uses);
         compute_ssa_uses_rec_ptr(te->op1, ssa_uses);
         if(te->op2)
         {
            compute_ssa_uses_rec_ptr(te->op2, ssa_uses);
         }
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointer<const lut_node>(curr_tn);
         compute_ssa_uses_rec_ptr(le->op0, ssa_uses);
         compute_ssa_uses_rec_ptr(le->op1, ssa_uses);
         if(le->op2)
         {
            compute_ssa_uses_rec_ptr(le->op2, ssa_uses);
         }
         if(le->op3)
         {
            compute_ssa_uses_rec_ptr(le->op3, ssa_uses);
         }
         if(le->op4)
         {
            compute_ssa_uses_rec_ptr(le->op4, ssa_uses);
         }
         if(le->op5)
         {
            compute_ssa_uses_rec_ptr(le->op5, ssa_uses);
         }
         if(le->op6)
         {
            compute_ssa_uses_rec_ptr(le->op6, ssa_uses);
         }
         if(le->op7)
         {
            compute_ssa_uses_rec_ptr(le->op7, ssa_uses);
         }
         if(le->op8)
         {
            compute_ssa_uses_rec_ptr(le->op8, ssa_uses);
         }
         break;
      }
      case constructor_node_K:
      {
         const auto c = GetPointer<const constructor_node>(curr_tn);
         for(const auto& iv : c->list_of_idx_valu)
         {
            compute_ssa_uses_rec_ptr(iv.second, ssa_uses);
         }
         break;
      }
      case variable_val_node_K:
      {
         /// var decl performs an assignment when init is not null
         // const auto vd = GetPointer<const variable_val_node>(curr_tn);
         // if(vd->init)
         //   compute_ssa_uses_rec_ptr(vd->init, ssa_uses);
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointer<const multi_way_if_stmt>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               compute_ssa_uses_rec_ptr(cond.first, ssa_uses);
            }
         }
         break;
      }
      case phi_stmt_K:
      case argument_val_node_K:
      case function_val_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      case field_val_node_K:
      {
         break;
      }
      case ssa_node_K:
      {
         ssa_uses.insert(GetPointer<const ssa_node>(curr_tn));
         break;
      }
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      {
         THROW_UNREACHABLE("Node is " + curr_tn->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--computed_ssa_uses_rec_ptr " + curr_tn->ToString());
}

static void ComputeSsaUses(const ir_nodeRef& tn, IRNodeMap<size_t>& ssa_uses)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level,
                  "-->Computing ssa uses in " + tn->ToString() + " (" + tn->get_kind_text() + ")");
   const auto recurse_virtuals = [&](const node_stmt* gn) {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level, "Computing virtual ssa uses");
      if(gn->memuse)
      {
         ComputeSsaUses(gn->memuse, ssa_uses);
      }
      for(const auto& vuse : gn->vuses)
      {
         ComputeSsaUses(vuse, ssa_uses);
      }
      for(const auto& vover : gn->vovers)
      {
         ComputeSsaUses(vover, ssa_uses);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level, "Computed virtual ssa uses");
   };

   switch(tn->get_kind())
   {
      case return_stmt_K:
      {
         const auto re = GetPointerS<return_stmt>(tn);
         recurse_virtuals(re);
         if(re->op)
         {
            ComputeSsaUses(re->op, ssa_uses);
         }
         break;
      }
      case assign_stmt_K:
      {
         const auto ga = GetPointerS<assign_stmt>(tn);
         recurse_virtuals(ga);
         if(ga->op0->get_kind() != ssa_node_K)
         {
            ComputeSsaUses(ga->op0, ssa_uses);
         }
         ComputeSsaUses(ga->op1, ssa_uses);
         if(ga->predicate)
         {
            ComputeSsaUses(ga->predicate, ssa_uses);
         }
         break;
      }
      case nop_stmt_K:
      {
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointerS<call_node>(tn);
         ComputeSsaUses(ce->fn, ssa_uses);
         for(const auto& arg : ce->args)
         {
            ComputeSsaUses(arg, ssa_uses);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto gc = GetPointerS<call_stmt>(tn);
         recurse_virtuals(gc);
         if(gc->predicate)
         {
            ComputeSsaUses(gc->predicate, ssa_uses);
         }
         ComputeSsaUses(gc->fn, ssa_uses);
         for(const auto& arg : gc->args)
         {
            ComputeSsaUses(arg, ssa_uses);
         }
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<unary_node>(tn);
         if(ue->op->get_kind() != function_val_node_K)
         {
            ComputeSsaUses(ue->op, ssa_uses);
         }
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<binary_node>(tn);
         ComputeSsaUses(be->op0, ssa_uses);
         ComputeSsaUses(be->op1, ssa_uses);
         break;
      }
      /*ternary expressions*/
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<ternary_node>(tn);
         ComputeSsaUses(te->op0, ssa_uses);
         ComputeSsaUses(te->op1, ssa_uses);
         if(te->op2)
         {
            ComputeSsaUses(te->op2, ssa_uses);
         }
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointerS<lut_node>(tn);
         ComputeSsaUses(le->op0, ssa_uses);
         ComputeSsaUses(le->op1, ssa_uses);
         if(le->op2)
         {
            ComputeSsaUses(le->op2, ssa_uses);
         }
         if(le->op3)
         {
            ComputeSsaUses(le->op3, ssa_uses);
         }
         if(le->op4)
         {
            ComputeSsaUses(le->op4, ssa_uses);
         }
         if(le->op5)
         {
            ComputeSsaUses(le->op5, ssa_uses);
         }
         if(le->op6)
         {
            ComputeSsaUses(le->op6, ssa_uses);
         }
         if(le->op7)
         {
            ComputeSsaUses(le->op7, ssa_uses);
         }
         if(le->op8)
         {
            ComputeSsaUses(le->op8, ssa_uses);
         }
         break;
      }
      case constructor_node_K:
      {
         const auto c = GetPointerS<constructor_node>(tn);
         for(const auto& iv : c->list_of_idx_valu)
         {
            ComputeSsaUses(iv.second, ssa_uses);
         }
         break;
      }
      case variable_val_node_K:
      {
         /// var decl performs an assignment when init is not null
         // const auto vd = GetPointerS<variable_val_node>(tn);
         // if(vd->init)
         // {
         //   ComputeSsaUses(vd->init, ssa_uses);
         // }
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<multi_way_if_stmt>(tn);
         recurse_virtuals(gmwi);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               ComputeSsaUses(cond.first, ssa_uses);
            }
         }
         break;
      }
      case phi_stmt_K:
      {
         const auto gp = GetPointerS<phi_stmt>(tn);
         recurse_virtuals(gp);
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            ComputeSsaUses(def_edge.first, ssa_uses);
         }
         break;
      }
      case argument_val_node_K:
      case function_val_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      case field_val_node_K:
      {
         break;
      }
      case ssa_node_K:
      {
         ssa_uses[tn]++;
         break;
      }
      case CASE_FAKE_NODES:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case CASE_TYPE_NODES:
      {
         THROW_UNREACHABLE("Node is " + tn->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, ir_helper::debug_level, "<--Computed ssa uses in @" + STR(tn->index));
}

IRNodeMap<size_t> ir_helper::ComputeSsaUses(const ir_nodeRef& tn)
{
   IRNodeMap<size_t> ret_value;
   ::ComputeSsaUses(tn, ret_value);
   return ret_value;
}

bool ir_helper::is_a_nop_function_decl(const function_val_node* fd)
{
   if(fd->body)
   {
      const auto sl = GetPointerS<const statement_list_node>(fd->body);
      if(!sl->list_of_bloc.empty())
      {
         auto bb_number = sl->list_of_bloc.size();
         if(sl->list_of_bloc.count(bloc::ENTRY_BLOCK_ID))
         {
            --bb_number;
         }
         if(sl->list_of_bloc.count(bloc::EXIT_BLOCK_ID))
         {
            --bb_number;
         }
         if(bb_number > 1)
         {
            return false;
         }
         if(bb_number == 0)
         {
            return true;
         }
         blocRef single_bb;
         for(const auto& lob_it : sl->list_of_bloc)
         {
            if(lob_it.first != bloc::ENTRY_BLOCK_ID && lob_it.first != bloc::EXIT_BLOCK_ID)
            {
               single_bb = lob_it.second;
            }
         }
         THROW_ASSERT(single_bb, "unexpected condition");
         if(!single_bb->CGetStmtList().empty())
         {
            const auto stmt_number = single_bb->CGetStmtList().size();
            if(stmt_number > 1)
            {
               return false;
            }
            const auto& single_stmt = single_bb->CGetStmtList().front();
            const auto gr = GetPointer<const return_stmt>(single_stmt);
            if(gr)
            {
               return !gr->op;
            }
            else
            {
               return false;
            }
         }
         else
         {
            return true;
         }
      }
      else
      {
         return true;
      }
   }
   else
   {
      return false;
   }
}

void ir_helper::get_required_values(std::vector<std::tuple<unsigned int, unsigned int>>& required,
                                    const ir_nodeConstRef& tn)
{
   auto tn_kind = tn->get_kind();
   switch(tn_kind)
   {
      case constructor_node_K:
      {
         const auto co = GetPointerS<const constructor_node>(tn);
         if(IsVectorType(co->type))
         {
            for(const auto& iv : co->list_of_idx_valu)
            {
               required.emplace_back(iv.second->index, 0);
            }
         }
         else
         {
            required.emplace_back(tn->index, 0);
         }
         break;
      }
      case ssa_node_K:
      case constant_fp_val_node_K:
      case constant_int_val_node_K:
      case constant_vector_val_node_K:
      case variable_val_node_K:
      case argument_val_node_K:
      {
         required.emplace_back(tn->index, 0);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<const binary_node>(tn);
         get_required_values(required, be->op0);
         get_required_values(required, be->op1);
         break;
      }
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<const unary_node>(tn);
         if(tn->get_kind() == addr_node_K)
         {
            required.emplace_back(tn->index, 0);
         }
         else
         {
            get_required_values(required, ue->op);
         }
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<const ternary_node>(tn);
         get_required_values(required, te->op0);
         get_required_values(required, te->op1);
         get_required_values(required, te->op2);
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointerS<const lut_node>(tn);
         get_required_values(required, le->op0);
         get_required_values(required, le->op1);
         if(le->op2)
         {
            get_required_values(required, le->op2);
         }
         if(le->op3)
         {
            get_required_values(required, le->op3);
         }
         if(le->op4)
         {
            get_required_values(required, le->op4);
         }
         if(le->op5)
         {
            get_required_values(required, le->op5);
         }
         if(le->op6)
         {
            get_required_values(required, le->op6);
         }
         if(le->op7)
         {
            get_required_values(required, le->op7);
         }
         if(le->op8)
         {
            get_required_values(required, le->op8);
         }
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<const multi_way_if_stmt>(tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               get_required_values(required, cond.first);
            }
         }
         break;
      }
      case assign_stmt_K:
      {
         const auto gm = GetPointerS<const assign_stmt>(tn);
         const auto op0_kind = gm->op0->get_kind();
         const auto op1_kind = gm->op1->get_kind();

         if(op0_kind == unaligned_mem_access_node_K || op0_kind == mem_access_node_K)
         {
            get_required_values(required, gm->op1);
            get_required_values(required, gm->op0);
         }
         else
         {
            if(op1_kind == unaligned_mem_access_node_K || op1_kind == mem_access_node_K)
            {
               required.emplace_back(0, 0);
            }
            get_required_values(required, gm->op1);
         }
         if(gm->predicate && gm->predicate->get_kind() != constant_int_val_node_K && op1_kind != call_node_K)
         {
            get_required_values(required, gm->predicate);
         }
         break;
      }
      case return_stmt_K:
      {
         const auto rt = GetPointerS<const return_stmt>(tn);
         if(rt->op)
         {
            get_required_values(required, rt->op);
         }
         break;
      }
      case phi_stmt_K:
      {
         const auto gp = GetPointerS<const phi_stmt>(tn);
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            required.emplace_back(def_edge.first->index, 0);
         }
         break;
      }
      case nop_stmt_K:
      {
         /// this has not to be synthesized
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointerS<const call_node>(tn);
         for(const auto& arg : ce->args)
         {
            required.emplace_back(arg->index, 0);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto ce = GetPointerS<const call_stmt>(tn);
         const function_val_node* fd = nullptr;
         if(ce->fn->get_kind() == addr_node_K)
         {
            const auto ue = GetPointerS<const unary_node>(ce->fn);
            fd = GetPointerS<const function_val_node>(ue->op);
         }
         if(!fd || !is_a_nop_function_decl(fd))
         {
            for(const auto& arg : ce->args)
            {
               required.emplace_back(arg->index, 0);
            }
         }
         break;
      }
      case field_val_node_K:
      {
         const auto fd = GetPointerS<const field_val_node>(tn);
         const auto ull_value = fd->offset;
         THROW_ASSERT(ull_value >= 0, "");
         required.emplace_back(0, static_cast<unsigned int>(ull_value / 8)); /// offset has an offset in bits
         if(ull_value % 8 != 0)
         {
            THROW_ERROR("bitfields are not yet supported: " + fd->ToString());
         }
         break;
      }
      case function_val_node_K:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      {
         THROW_ERROR("Operation not yet supported: " + std::string(tn->get_kind_text()));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
}

bool ir_helper::LastStatement(const ir_nodeConstRef& statement)
{
   switch(statement->get_kind())
   {
      case assign_stmt_K:
      case call_stmt_K:
      case nop_stmt_K:
      {
         return false;
      }
      case multi_way_if_stmt_K:
      case phi_stmt_K:
      case return_stmt_K:
      {
         return true;
      }
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case lut_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      default:
      {
         THROW_UNREACHABLE("Unexpected statement: " + statement->ToString());
         break;
      }
   }
   return true;
}

static bool is_mem_access(const ir_nodeRef& op, const CustomOrderedSet<unsigned int>& fun_mem_data = {})
{
   const auto k = op->get_kind();

   return k == unaligned_mem_access_node_K || k == mem_access_node_K || fun_mem_data.count(op->index);
}

bool ir_helper::IsStore(const ir_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data)
{
   if(tn->get_kind() != assign_stmt_K)
   {
      return false;
   }
   const auto ga = GetPointerS<const assign_stmt>(tn);
   if(ga->op1->get_kind() == bitcast_node_K)
   {
      const auto op0_type = CGetType(ga->op0);
      if(op0_type->get_kind() == struct_ty_node_K)
      {
         return true;
      }
      const auto bitcast_expr = GetPointerS<const bitcast_node>(ga->op1);
      const auto bitcast_op_type = CGetType(bitcast_expr->op);
      if(bitcast_op_type->get_kind() == vector_ty_node_K && op0_type->get_kind() == array_ty_node_K)
      {
         return true;
      }
   }
   return is_mem_access(ga->op0, fun_mem_data);
}

bool ir_helper::IsLoad(const ir_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data)
{
   if(tn->get_kind() != assign_stmt_K)
   {
      return false;
   }
   const auto ga = GetPointerS<const assign_stmt>(tn);
   if(ga->op1->get_kind() == bitcast_node_K)
   {
      const auto bitcast_expr = GetPointerS<const bitcast_node>(ga->op1);
      const auto bitcast_op_type = CGetType(bitcast_expr->op);
      if(bitcast_op_type->get_kind() == struct_ty_node_K)
      {
         return true;
      }
      const auto op0_type = CGetType(ga->op0);
      if(bitcast_op_type->get_kind() == array_ty_node_K && op0_type->get_kind() == vector_ty_node_K)
      {
         return true;
      }
   }
   return is_mem_access(ga->op1, fun_mem_data);
}

bool ir_helper::IsLut(const ir_nodeConstRef& tn)
{
   if(tn->get_kind() != assign_stmt_K)
   {
      return false;
   }
   return GetPointerS<const assign_stmt>(tn)->op1->get_kind() == lut_node_K;
}
