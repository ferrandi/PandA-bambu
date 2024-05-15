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
 * @file tree_helper.cpp
 * @brief This file collects some utility functions.
 *
 *
 * @author Katia Turati <turati@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "tree_helper.hpp"

#include "Range.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <regex>
#include <set>
#include <utility>

/// built in type without parameter
///"sc_in_resolved" is not a template and inherits from sc_in<sc_dt::sc_logic>
///"sc_inout_resolved" is not a template and inherits from sc_inout<sc_dt::sc_logic>
///"sc_out_resolved" is not a template and inherits from sc_inout_resolved
///"sc_in_clk" is not a template and is a typedef of sc_in<bool>
///"sc_out_clk" is not a template and is a typedef of sc_out<bool>
///"sc_inout_clk" is not a template and is a typedef of sc_inout<bool>

/// builtin types with parameter
///"sc_lv" is a template, has a parameter (W)
///"sc_bv" is a template, has a parameter (W)
///"sc_in_rv" is a template, has a parameter (W) and inherits from sc_in<sc_dt::sc_lv<W> >
///"sc_out_rv" is a template, has a parameter (W) and inherits from sc_inout_rv<W>
///"sc_inout_rv" is a template, has a parameter (W) and inherits from sc_inout<sc_dt::sc_lv<W> >
///"sc_signal_rv" is a template, has a parameter (W) and inherits from sc_signal<sc_dt::sc_lv<W> >

///"sc_fifo_in" is a template, has a parameter (T) and inherits from sc_port<sc_fifo_in_if<T>,0>
///"sc_fifo_out" is a template, has a parameter (T) and inherits from sc_port<sc_fifo_out_if<T>,0>

const std::set<std::string> tree_helper::SC_tmpl_class = {"sc_signal", "sc_in",   "sc_out",
                                                          "sc_inout",  "sc_port", "sc_export"};

const std::set<std::string> tree_helper::SC_builtin_scalar_type = {
    "sc_int",    "sc_uint",     "sc_biguint", "sc_bigint",   "sc_lv", "sc_in_rv",
    "sc_out_rv", "sc_inout_rv", "sc_bv",      "sc_signal_rv"}; /*check "sc_fix","sc_ufix"  "sc_fifo", "sc_buffer",
                                                                  "sc_fixed", "sc_ufixed" */

const std::set<std::string> tree_helper::TLM_SC_builtin_scalar_type = {
    "tlm_blocking_put_if", "tlm_blocking_get_if", "tlm_nonblocking_get_if", "tlm_nonblocking_put_if",
    "tlm_transport_if" // TLM objects
};

int tree_helper::debug_level = 0;

tree_helper::tree_helper() = default;

tree_helper::~tree_helper() = default;

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

unsigned long long tree_helper::Size(const tree_nodeConstRef& t)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "---Getting size of " + t->get_kind_text() + " " + STR(t->index) + ": " + t->ToString());
   switch(t->get_kind())
   {
      case ssa_name_K:
      {
         const auto sa = GetPointerS<const ssa_name>(t);
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
      case array_type_K:
      {
         const auto at = GetPointerS<const array_type>(t);
         if(at->size)
         {
            const auto ic = GetPointer<const integer_cst>(at->size);
            if(ic)
            {
               return static_cast<unsigned long long>(GetConstValue(at->size));
            }
            THROW_UNREACHABLE("What should be the size here? " + t->ToString());
            return 32ull; // TODO: should this be pointer size? or should be zero?
         }
         return 0;
      }
      case enumeral_type_K:
      {
         const auto retval = [&]() -> integer_cst_t {
            const auto et = GetPointerS<const enumeral_type>(t);
            if(et->min && et->max && et->min->get_kind() == integer_cst_K && et->max->get_kind() == integer_cst_K)
            {
               const auto max = GetConstValue(et->max, !et->unsigned_flag);
               const auto min = GetConstValue(et->min, !et->unsigned_flag);
               if(et->unsigned_flag)
               {
                  return max.minBitwidth(false);
               }
               else
               {
                  return std::max(min.minBitwidth(true), max.minBitwidth(true));
               }
            }
            else
            {
               const auto snode = GetPointerS<const type_node>(t)->size;
               if(!snode)
               {
                  return 0;
               }
               return GetConstValue(snode);
            }
         }();
         return static_cast<unsigned long long>(retval);
      }
      case integer_type_K:
      {
         const auto it = GetPointerS<const integer_type>(t);
         const auto prec = it->prec;
         const auto algn = it->algn;
         if(prec != algn && prec % algn)
         {
            return prec;
         }
         return static_cast<unsigned long long>(GetConstValue(it->size));
      }
      case integer_cst_K:
      {
         const auto is_signed = IsSignedIntegerType(t);
         const auto retval = GetConstValue(t, is_signed).minBitwidth(is_signed);
         return static_cast<unsigned long long>(retval);
      }
      case CharType_K:
      case complex_type_K:
      case function_type_K:
      case method_type_K:
      case nullptr_type_K:
      case pointer_type_K:
      case real_type_K:
      case record_type_K:
      case reference_type_K:
      case type_argument_pack_K:
      case type_pack_expansion_K:
      case union_type_K:
      case vector_type_K:
      {
         const auto snode = GetPointerS<const type_node>(t)->size;
         return snode ? static_cast<unsigned long long>(GetConstValue(snode)) : 0;
      }
      case CASE_DECL_NODES:
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case aggr_init_expr_K:
      case array_ref_K:
      case call_expr_K:
      case complex_cst_K:
      case constructor_K:
      case gimple_assign_K:
      case gimple_call_K:
      case gimple_phi_K:
      case gimple_return_K:
      case real_cst_K:
      case string_cst_K:
      case target_mem_ref461_K:
      case target_mem_ref_K:
      case vector_cst_K:
      case void_cst_K:
      {
         if(t->get_kind() == function_decl_K || t->get_kind() == gimple_call_K)
         {
            return 32ull; // static_cast<unsigned int>(CompilerWrapper::CGetPointerSize(parameters));
         }
         return Size(CGetType(t));
      }
      case boolean_type_K:
      case lut_expr_K:
      {
         return 1ull;
      }
      case void_type_K:
      {
         return 8ull;
      }
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case array_range_ref_K:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case error_mark_K:
      case gimple_asm_K:
      case gimple_bind_K:
      case gimple_cond_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_multi_way_if_K:
      case gimple_nop_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_switch_K:
      case gimple_while_K:
      case identifier_node_K:
      case lang_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case set_type_K:
      case statement_list_K:
      case target_expr_K:
      case template_type_parm_K:
      case tree_list_K:
      case tree_vec_K:
      case typename_type_K:
      default:
      {
         THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
         break;
      }
   }
   THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
   return 0;
}

unsigned long long tree_helper::SizeAlloc(const tree_nodeConstRef& t)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "---Getting size of " + t->get_kind_text() + " " + STR(t->index) + ": " + t->ToString());
   switch(t->get_kind())
   {
      case array_type_K:
      {
         const auto at = GetPointerS<const array_type>(t);
         if(at->size)
         {
            const auto ic = GetPointer<const integer_cst>(at->size);
            if(ic)
            {
               return static_cast<unsigned long long>(GetConstValue(at->size));
            }
            THROW_UNREACHABLE("What should be the size here? " + t->ToString());
            return 32ull; // TODO: should this be pointer size? or should be zero?
         }
         return 0;
      }
      case integer_type_K:
      case enumeral_type_K:
      case CharType_K:
      case complex_type_K:
      case function_type_K:
      case method_type_K:
      case nullptr_type_K:
      case pointer_type_K:
      case real_type_K:
      case record_type_K:
      case reference_type_K:
      case type_argument_pack_K:
      case type_pack_expansion_K:
      case union_type_K:
      case vector_type_K:
      case boolean_type_K:
      {
         const auto snode = GetPointerS<const type_node>(t)->size;
         THROW_ASSERT(snode, "unexpected pattern");
         return static_cast<unsigned long long>(GetConstValue(snode));
      }
      case gimple_call_K:
      {
         return 32ull;
      }
      case CASE_DECL_NODES:
      {
         if(t->get_kind() == field_decl_K)
         {
            const auto snode = GetPointerS<const field_decl>(t)->size;
            if(snode)
            {
               return static_cast<unsigned long long>(GetConstValue(snode));
            }
         }
         return SizeAlloc(GetPointerS<const decl_node>(t)->type);
      }
      case lut_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case ssa_name_K:
      case aggr_init_expr_K:
      case array_ref_K:
      case call_expr_K:
      case constructor_K:
      case gimple_assign_K:
      case gimple_phi_K:
      case gimple_return_K:
      case integer_cst_K:
      case real_cst_K:
      case complex_cst_K:
      case string_cst_K:
      case target_mem_ref461_K:
      case target_mem_ref_K:
      case vector_cst_K:
      case void_cst_K:
      {
         return SizeAlloc(CGetType(t));
      }
      case void_type_K:
      {
         return 8ull;
      }
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case array_range_ref_K:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case error_mark_K:
      case gimple_asm_K:
      case gimple_bind_K:
      case gimple_cond_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_multi_way_if_K:
      case gimple_nop_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_switch_K:
      case gimple_while_K:
      case identifier_node_K:
      case lang_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case set_type_K:
      case statement_list_K:
      case target_expr_K:
      case template_type_parm_K:
      case tree_list_K:
      case tree_vec_K:
      case typename_type_K:
      default:
      {
         THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
         break;
      }
   }
   THROW_UNREACHABLE(std::string("Unexpected type pattern ") + t->get_kind_text());
   return 0;
}

unsigned long long tree_helper::TypeSize(const tree_nodeConstRef& tn)
{
   return Size(CGetType(tn));
}

RangeRef tree_helper::Range(const tree_nodeConstRef& tn)
{
   const auto type = CGetType(tn);
   auto bw = static_cast<Range::bw_t>(Size(type));
   THROW_ASSERT(static_cast<bool>(bw) || tn->get_kind() == string_cst_K,
                "Unhandled type (" + type->get_kind_text() + ") for " + tn->get_kind_text() + " " + tn->ToString());
   bool sign = false;
   APInt min, max;
   if(tn->get_kind() == integer_cst_K)
   {
      const auto ic = GetPointerS<const integer_cst>(tn);
      min = max = tree_helper::get_integer_cst_value(ic);
      return RangeRef(new class Range(Regular, bw, min, max));
   }
   else if(tn->get_kind() == string_cst_K)
   {
      const auto sc = GetPointerS<const string_cst>(tn);
      bw = 8;
      RangeRef r(
          new class Range(Regular, bw, 0, 0)); // Zero is the string terminator, thus it should always be included
      for(const auto& c : sc->strg)
      {
         r = r->unionWith(RangeRef(new class Range(Regular, bw, c, c)));
      }
      return r;
   }
   else if(tn->get_kind() == real_cst_K)
   {
      const auto rc = GetPointerS<const real_cst>(tn);
      THROW_ASSERT(bw == 64 || bw == 32, "Floating point variable with unhandled bitwidth (" + STR(bw) + ")");
      if(rc->valx.front() == '-' && rc->valr.front() != rc->valx.front())
      {
         return Range::fromBitValues(string_to_bitstring(convert_fp_to_string("-" + rc->valr, bw)), bw, false);
      }
      else
      {
         return Range::fromBitValues(string_to_bitstring(convert_fp_to_string(rc->valr, bw)), bw, false);
      }
   }
   else if(tn->get_kind() == vector_cst_K)
   {
      const auto vc = GetPointerS<const vector_cst>(tn);
      const auto el_type = CGetElements(type);
      const auto el_bw = Size(el_type);
      RangeRef r(new class Range(Empty, bw));
      const auto stride = static_cast<size_t>(bw / el_bw);
      const auto strides = vc->list_of_valu.size() / stride;
      THROW_ASSERT(strides * stride == vc->list_of_valu.size(), "");
      for(size_t i = 0; i < strides; ++i)
      {
         auto curr_el = Range(vc->list_of_valu.at(i * stride));
         for(size_t j = 1; j < stride; ++j)
         {
            curr_el = Range(vc->list_of_valu.at(i * stride + j))
                          ->zextOrTrunc(bw)
                          ->shl(RangeRef(new class Range(Regular, bw, el_bw * j, el_bw * j)))
                          ->Or(curr_el);
         }
         r = r->unionWith(curr_el);
      }
      return r;
   }
   else if(type->get_kind() == integer_type_K)
   {
      const auto it = GetPointerS<const integer_type>(type);
      sign = !it->unsigned_flag;
      min = sign ? APInt::getSignedMinValue(bw) : APInt::getMinValue(bw);
      max = sign ? APInt::getSignedMaxValue(bw) : APInt::getMaxValue(bw);
   }
   else if(type->get_kind() == enumeral_type_K)
   {
      const auto et = GetPointerS<const enumeral_type>(type);
      sign = !et->unsigned_flag;
      min = sign ? APInt::getSignedMinValue(bw) : APInt::getMinValue(bw);
      max = sign ? APInt::getSignedMaxValue(bw) : APInt::getMaxValue(bw);
   }
   else if(type->get_kind() == boolean_type_K)
   {
      min = 0;
      max = 1;
      bw = 1;
   }
   else if(type->get_kind() == pointer_type_K)
   {
      min = APInt::getMinValue(bw);
      max = APInt::getMaxValue(bw);
   }
   else if(type->get_kind() == vector_type_K || type->get_kind() == array_type_K)
   {
      bw = static_cast<Range::bw_t>(Size(CGetElements(type)));
      return RangeRef(new class Range(Regular, bw));
   }
   else if(type->get_kind() == record_type_K)
   {
      const auto rt = GetPointerS<const record_type>(type);
      THROW_ASSERT(rt->size->get_kind() == integer_cst_K && GetConstValue(rt->size) >= 0, "record_type has no size");
      bw = static_cast<Range::bw_t>(GetConstValue(rt->size));
      THROW_ASSERT(bw, "Invalid bitwidth");
      return RangeRef(new class Range(Regular, bw));
   }
   else
   {
      THROW_UNREACHABLE("Unable to define range for type " + type->get_kind_text() + " of " + tn->ToString());
   }

   if(tn->get_kind() == ssa_name_K)
   {
      const auto ssa = GetPointerS<const ssa_name>(tn);
      if(!ssa->bit_values.empty())
      {
         const auto bvSize = static_cast<Range::bw_t>(ssa->bit_values.size());
         const auto bvRange = Range::fromBitValues(string_to_bitstring(ssa->bit_values), bvSize, sign);
         const auto varRange =
             RangeRef(new class Range(Regular, bw, min, max))->truncate(bvSize)->intersectWith(bvRange);
         return sign ? varRange->sextOrTrunc(bw) : varRange->zextOrTrunc(bw);
      }
   }
   return RangeRef(new class Range(Regular, bw, min, max));
}

RangeRef tree_helper::TypeRange(const tree_nodeConstRef& tn, int _rt)
{
   const auto rt = static_cast<RangeType>(_rt);
   THROW_ASSERT(rt != Anti, "");
   const auto type = CGetType(tn);
   const auto bw = static_cast<Range::bw_t>(Size(type));
   THROW_ASSERT(bw, "Unhandled type (" + type->get_kind_text() + ") for " + tn->get_kind_text() + " " + tn->ToString());
   return RangeRef(new class Range(rt, bw));
}

RangeRef tree_helper::TypeRange(const tree_nodeConstRef& tn)
{
   return TypeRange(tn, Regular);
}

std::string tree_helper::GetTemplateTypeName(const tree_nodeConstRef& type)
{
   if(type->get_kind() == record_type_K)
   {
      const auto rect = GetPointerS<const record_type>(type);
      if(rect->tmpl_args) /*the class is a template*/
      {
         for(auto& list_of_fld : rect->list_of_flds)
         {
            if(list_of_fld->get_kind() == type_decl_K)
            {
               const auto td = GetPointerS<const type_decl>(list_of_fld);
               if(td->name->get_kind() == identifier_node_K)
               {
                  const auto idn = GetPointerS<const identifier_node>(td->name);
                  return (idn->strg);
               }
            }
         }
      }
   }
   return "";
}

std::string tree_helper::GetRecordTypeName(const tree_nodeConstRef& type)
{
   if(type->get_kind() == record_type_K)
   {
      const auto rect = GetPointerS<const record_type>(type);
      if(rect->name)
      {
         if(rect->name->get_kind() == type_decl_K)
         {
            const auto td = GetPointerS<const type_decl>(rect->name);
            if(td->name->get_kind() == identifier_node_K)
            {
               const auto idn = GetPointerS<const identifier_node>(td->name);
               return (idn->strg);
            }
         }
      }
   }
   return "";
}

std::string tree_helper::name_function(const tree_managerConstRef& tm, const unsigned int index)
{
   const auto t = tm->GetTreeNode(index);
   return GetFunctionName(tm, t);
}

std::string tree_helper::GetFunctionName(const tree_managerConstRef& TM, const tree_nodeConstRef& decl)
{
   if(decl->get_kind() == function_decl_K)
   {
      const auto fd = GetPointerS<const function_decl>(decl);
      return print_function_name(TM, fd);
   }
   else if(decl->get_kind() == addr_expr_K)
   {
      return GetFunctionName(TM, GetPointerS<const unary_expr>(decl)->op);
   }
   else
   {
      THROW_ERROR("Node not yet supported " + decl->get_kind_text());
   }
   return "";
}

std::string tree_helper::GetMangledFunctionName(const function_decl* fd)
{
   if(fd->builtin_flag || (!fd->mngl && fd->name))
   {
      THROW_ASSERT(fd->name, "unexpected condition");
      THROW_ASSERT(fd->name->get_kind() == identifier_node_K, "unexpected condition");
      return NormalizeTypename(GetPointerS<const identifier_node>(fd->name)->strg);
   }
   else if(fd->mngl)
   {
      THROW_ASSERT(fd->mngl->get_kind() == identifier_node_K, "unexpected condition");
      return NormalizeTypename(GetPointerS<const identifier_node>(fd->mngl)->strg);
   }
   THROW_ERROR("unexpected condition");
   return "";
}

std::string tree_helper::print_function_name(const tree_managerConstRef& TM, const function_decl* fd)
{
   const auto& name = (fd->builtin_flag || !fd->mngl) ? fd->name : fd->mngl;
   std::string res;
   if(name->get_kind() == identifier_node_K)
   {
      const auto in = GetPointerS<const identifier_node>(name);
      if(in->operator_flag)
      {
         res = "operator ";
         for(const auto& attr : fd->list_attr)
         {
            if(attr == TreeVocabularyTokenTypes_TokenEnum::TOK_PUBLIC ||
               attr == TreeVocabularyTokenTypes_TokenEnum::TOK_PRIVATE ||
               attr == TreeVocabularyTokenTypes_TokenEnum::TOK_PROTECTED ||
               attr == TreeVocabularyTokenTypes_TokenEnum::TOK_OPERATOR ||
               attr == TreeVocabularyTokenTypes_TokenEnum::TOK_MEMBER ||
               attr == TreeVocabularyTokenTypes_TokenEnum::TOK_CONVERSION)
            {
               continue;
            }
            else if(attr == TreeVocabularyTokenTypes_TokenEnum::TOK_RSHIFT)
            {
               res += ">>";
            }
            else if(attr == TreeVocabularyTokenTypes_TokenEnum::TOK_LSHIFT)
            {
               res += "<<";
            }
            else if(attr == TreeVocabularyTokenTypes_TokenEnum::TOK_ASSIGN)
            {
               res += "=";
            }
            else
            {
               res += TI_getTokenName(attr);
            }
         }
         res = NormalizeTypename(res);
      }
      else
      {
         res = NormalizeTypename(in->strg);
      }
   }
   else
   {
      THROW_ERROR(std::string("Node not yet supported ") + name->get_kind_text());
   }
   if(fd->builtin_flag && fd->body && !TM->is_top_function(fd))
   {
      res = "__internal_" + res;
   }
   return res;
}

std::tuple<std::string, unsigned int, unsigned int> tree_helper::GetSourcePath(const tree_nodeConstRef& _node,
                                                                               bool& is_system)
{
   is_system = false;
   auto node = _node;
   std::string include_name;
   unsigned int line_number = 0;
   unsigned int column_number = 0;
   if(GetPointer<const type_node>(node))
   {
      const auto tn = GetPointerS<const type_node>(node);
      if(tn->name && GetPointer<const decl_node>(tn->name))
      {
         node = tn->name;
      }
      else
      {
         if(GetPointer<const union_type>(node))
         {
            const auto& list_of_flds = GetPointerS<const union_type>(node)->list_of_flds;
            if(!list_of_flds.empty())
            {
               const auto field = list_of_flds[0];
               if(GetPointer<const decl_node>(field))
               {
                  const auto dn = GetPointerS<const decl_node>(field);
                  include_name = dn->include_name;
                  line_number = dn->line_number;
                  column_number = dn->column_number;
                  is_system = dn->operating_system_flag or dn->library_system_flag;
               }
            }
         }
         if(GetPointer<const record_type>(node))
         {
            const auto& list_of_flds = GetPointerS<const record_type>(node)->list_of_flds;
            if(!list_of_flds.empty())
            {
               const auto field = list_of_flds.at(0);
               if(GetPointer<const decl_node>(field))
               {
                  const auto dn = GetPointerS<const decl_node>(field);
                  include_name = dn->include_name;
                  line_number = dn->line_number;
                  column_number = dn->column_number;
                  is_system = dn->operating_system_flag or dn->library_system_flag;
               }
            }
         }
      }
   }
   if(GetPointer<const decl_node>(node))
   {
      const auto dn = GetPointerS<const decl_node>(node);
      include_name = dn->include_name;
      line_number = dn->line_number;
      column_number = dn->column_number;
      is_system = dn->operating_system_flag or dn->library_system_flag;
   }
   if(include_name != "<built-in>")
   {
      std::error_code ec;
      const auto canon_path = std::filesystem::weakly_canonical(include_name, ec);
      if(!ec)
      {
         include_name = canon_path.string();
      }
   }
   return {include_name, line_number, column_number};
}

void tree_helper::get_used_variables(bool first_level_only, const tree_nodeConstRef& tRI,
                                     CustomUnorderedSet<unsigned int>& list_of_variable)
{
   if(!tRI)
   {
      return;
   }
   const auto t = tRI;
   switch(t->get_kind())
   {
      case result_decl_K: // tree_to_graph considers this object as particular type of variable
      {
         const auto rd = GetPointer<const result_decl>(t);
         list_of_variable.insert(tRI->index);
         if(rd->init)
         {
            get_used_variables(first_level_only, rd->init, list_of_variable);
         }
         break;
      }
      case var_decl_K:
      {
         const auto vd = GetPointer<const var_decl>(t);
         list_of_variable.insert(tRI->index);
         if(vd->init)
         {
            get_used_variables(first_level_only, vd->init, list_of_variable);
         }
         break;
      }
      case ssa_name_K:
      {
         const auto sn = GetPointer<const ssa_name>(t);
         get_used_variables(first_level_only, sn->var, list_of_variable);
         list_of_variable.insert(tRI->index);
         if(sn->var)
         {
            const auto vd = GetPointer<const var_decl>(sn->var);
            if(vd && vd->init)
            {
               get_used_variables(first_level_only, vd->init, list_of_variable);
            }
         }
         break;
      }
      case parm_decl_K:
         list_of_variable.insert(tRI->index);
         break;
      case function_decl_K:
      {
         const auto fd = GetPointer<const function_decl>(t);
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
      case statement_list_K:
      {
         const auto sl = GetPointer<const statement_list>(t);
         auto end = sl->list_of_stmt.end();
         auto i = sl->list_of_stmt.begin();
         if(i != end)
         {
            for(; i != end; ++i)
            {
               get_used_variables(first_level_only, *i, list_of_variable);
            }
         }
         else
         {
            auto ib_end = sl->list_of_bloc.end();
            for(auto ib = sl->list_of_bloc.begin(); ib != ib_end; ++ib)
            {
               for(const auto& stmt : ib->second->CGetStmtList())
               {
                  get_used_variables(first_level_only, stmt, list_of_variable);
               }
            }
         }
      }
      break;
      case tree_vec_K:
      {
         const auto tv = GetPointer<const tree_vec>(t);
         auto end = tv->list_of_op.end();
         for(auto i = tv->list_of_op.begin(); i != end; ++i)
         {
            get_used_variables(first_level_only, *i, list_of_variable);
         }
      }
      break;
      case gimple_cond_K:
      {
         const auto gc = GetPointer<const gimple_cond>(t);
         get_used_variables(first_level_only, gc->op0, list_of_variable);
      }
      break;
      case gimple_assign_K:
      {
         const auto me = GetPointer<const gimple_assign>(t);
         get_used_variables(first_level_only, me->op0, list_of_variable);
         get_used_variables(first_level_only, me->op1, list_of_variable);
         if(me->predicate)
         {
            get_used_variables(first_level_only, me->predicate, list_of_variable);
         }
      }
      break;
      case gimple_return_K:
      {
         const auto re = GetPointer<const gimple_return>(t);
         if(re->op)
         {
            get_used_variables(first_level_only, re->op, list_of_variable);
         }
      }
      break;
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointer<const unary_expr>(t);
         if(list_of_variable.find(ue->op->index) != list_of_variable.end())
         {
            break;
         }
         get_used_variables(first_level_only, ue->op, list_of_variable);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointer<const binary_expr>(t);
         get_used_variables(first_level_only, be->op0, list_of_variable);
         get_used_variables(first_level_only, be->op1, list_of_variable);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto tern = GetPointer<const ternary_expr>(t);
         get_used_variables(first_level_only, tern->op0, list_of_variable);
         get_used_variables(first_level_only, tern->op1, list_of_variable);
         get_used_variables(first_level_only, tern->op2, list_of_variable);
         break;
      }
      /* quaternary expressions.*/
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointer<const quaternary_expr>(t);
         get_used_variables(first_level_only, qe->op0, list_of_variable);
         get_used_variables(first_level_only, qe->op1, list_of_variable);
         get_used_variables(first_level_only, qe->op2, list_of_variable);
         get_used_variables(first_level_only, qe->op3, list_of_variable);
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointer<const lut_expr>(t);
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
      case gimple_switch_K:
      {
         const auto s = GetPointer<const gimple_switch>(t);
         get_used_variables(first_level_only, s->op0, list_of_variable);
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointer<const gimple_multi_way_if>(t);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               get_used_variables(first_level_only, cond.first, list_of_variable);
            }
         }
         break;
      }
      case label_decl_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
         break;
      case field_decl_K: // used to specify the displacement
      case case_label_expr_K:
      case gimple_label_K:
         break;
      case constructor_K:
      {
         const auto co = GetPointer<const constructor>(t);
         for(const auto& i : co->list_of_idx_valu)
         {
            get_used_variables(first_level_only, i.second, list_of_variable);
         }
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointer<const call_expr>(t);
         for(const auto& arg : ce->args)
         {
            get_used_variables(first_level_only, arg, list_of_variable);
         }
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointer<const gimple_call>(t);
         for(const auto& arg : ce->args)
         {
            get_used_variables(first_level_only, arg, list_of_variable);
         }
         break;
      }
      case gimple_asm_K:
      {
         const auto ae = GetPointer<const gimple_asm>(t);
         if(ae->in)
         {
            get_used_variables(first_level_only, ae->in, list_of_variable);
         }
         if(ae->out)
         {
            get_used_variables(first_level_only, ae->out, list_of_variable);
         }
         if(ae->clob)
         {
            get_used_variables(first_level_only, ae->clob, list_of_variable);
         }
         break;
      }
      case tree_list_K:
      {
         auto tl = GetPointer<const tree_list>(t);
         std::list<const tree_list*> tl_list;
         do
         {
            tl_list.push_back(tl);
            tl = tl->chan ? GetPointer<const tree_list>(tl->chan) : nullptr;
         } while(tl);
         for(auto tl_current0 : tl_list)
         {
            if(tl_current0->purp)
            {
               get_used_variables(first_level_only, tl_current0->purp, list_of_variable);
            }
            if(tl_current0->valu)
            {
               get_used_variables(first_level_only, tl_current0->valu, list_of_variable);
            }
         }
         break;
      }
      case gimple_for_K:
      {
         const auto fe = GetPointer<const gimple_for>(t);
         get_used_variables(first_level_only, fe->op0, list_of_variable);
         get_used_variables(first_level_only, fe->op1, list_of_variable);
         get_used_variables(first_level_only, fe->op2, list_of_variable);
         break;
      }
      case gimple_while_K:
      {
         const auto we = GetPointer<const gimple_while>(t);
         get_used_variables(first_level_only, we->op0, list_of_variable);
         break;
      }
      case binfo_K:
      case block_K:
      case const_decl_K:
      case gimple_bind_K:
      case gimple_goto_K:
      case gimple_nop_K:
      case gimple_phi_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case identifier_node_K:
      case namespace_decl_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case type_decl_K:
      case error_mark_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("Node not yet supported ") + t->get_kind_text());
   }
}

bool tree_helper::look_for_binfo_inheritance(const binfo* b, const std::string& bcs)
{
   if(b)
   {
      if(b->type)
      {
         const auto rt = GetPointer<const record_type>(b->type);
         if(rt && rt->get_maybe_name() == bcs)
         {
            return true;
         }
      }
      for(unsigned int i = 0; i < b->get_baseinfo_size(); i++)
      {
         tree_nodeRef binf = b->get_base(i);
         const auto bnf = GetPointer<const binfo>(binf);
         bool found = look_for_binfo_inheritance(bnf, bcs);
         if(found)
         {
            return true;
         }
      }
   }
   return false;
}

tree_nodeRef tree_helper::find_obj_type_ref_function(const tree_nodeConstRef& tn)
{
   unsigned int ind = tn->index;
   const auto otr = GetPointer<const obj_type_ref>(tn);
   THROW_ASSERT(otr, "tree node is not an obj_type_ref");
   THROW_ASSERT(otr->type && otr->op1 && otr->op2, "obj_type_ref has missing fields");
   tree_nodeRef type;
   unsigned int function_type;

   const auto t_pt = GetPointer<const pointer_type>(otr->type);
   THROW_ASSERT(t_pt, "Expected a pointer_type");
   function_type = t_pt->ptd->index;
   THROW_ASSERT(GetPointer<const method_type>(t_pt->ptd), "expected a method_type");
   type = GetPointer<const method_type>(t_pt->ptd)->clas;

   if(type)
   {
      const auto rt = GetPointer<const record_type>(type);
      if(rt)
      {
         for(auto& list_of_fnc : rt->list_of_fncs)
         {
            const auto fd = GetPointer<const function_decl>(list_of_fnc);
            if(fd && fd->type->index == function_type)
            {
               return list_of_fnc;
            }
         }
      }
      else
      {
         THROW_ERROR(std::string("not supported case for obj_type_ref(") + STR(ind) + std::string(")"));
      }
   }
   else
   {
      THROW_ERROR(std::string("not supported case for obj_type_ref(") + STR(ind) + std::string(")"));
   }
   THROW_ERROR(std::string("obj_type_ref Function not found (") + STR(ind) + std::string(")"));
   return {};
}

bool tree_helper::is_system(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto curr_tn = TM->GetTreeNode(index);
   return IsSystemType(curr_tn);
}

bool tree_helper::IsSystemType(const tree_nodeConstRef& type)
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

bool tree_helper::IsInLibbambu(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto curr_tn = TM->GetTreeNode(index);
   return IsInLibbambu(curr_tn);
}

bool tree_helper::IsInLibbambu(const tree_nodeConstRef& type)
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

std::set<tree_nodeConstRef, TreeNodeConstSorter>
tree_helper::GetTypesToBeDeclaredBefore(const tree_nodeConstRef& tn, const bool without_transformation)
{
   std::set<tree_nodeConstRef, TreeNodeConstSorter> rt;
   RecursiveGetTypesToBeDeclared(rt, tn, false, without_transformation, true);
   return rt;
}

std::set<tree_nodeConstRef, TreeNodeConstSorter>
tree_helper::GetTypesToBeDeclaredAfter(const tree_nodeConstRef& tn, const bool without_transformation)
{
   std::set<tree_nodeConstRef, TreeNodeConstSorter> rt;
   RecursiveGetTypesToBeDeclared(rt, tn, false, without_transformation, false);
   return rt;
}

void tree_helper::RecursiveGetTypesToBeDeclared(std::set<tree_nodeConstRef, TreeNodeConstSorter>& returned_types,
                                                const tree_nodeConstRef& type, const bool recursion,
                                                const bool without_transformation, const bool before)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Getting types to be declared " + STR(before ? "before " : "after ") + STR(type));
   switch(type->get_kind())
   {
      case pointer_type_K:
      {
         if(before)
         {
            RecursiveGetTypesToBeDeclared(returned_types, CGetPointedType(type), true, without_transformation, true);
         }
         break;
      }
      case reference_type_K:
      {
         if(before)
         {
            const auto rt = GetPointerS<const reference_type>(type);
            RecursiveGetTypesToBeDeclared(returned_types, rt->refd, true, without_transformation, true);
         }
         break;
      }
      case array_type_K:
      case vector_type_K:
      {
         if(before)
         {
            const auto* tn = GetPointerS<const type_node>(type);
            if(recursion && tn->name && tn->name->get_kind() == type_decl_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "---Inserting " + STR(type) + " in the types to be declared");
               returned_types.insert(type);
            }
            else
            {
               RecursiveGetTypesToBeDeclared(returned_types, CGetElements(type), true, without_transformation, before);
            }
         }
         break;
      }
      case record_type_K:
      {
         if(recursion)
         {
            if(before)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "---Inserting " + STR(type) + " in the types to be declared");
               returned_types.insert(type);
            }
         }
         else
         {
            const auto* rt = GetPointerS<const record_type>(type);
            if(rt->unql && (GetPointerS<const record_type>(rt->unql)->name || without_transformation))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Record type with named unqualified");
               if((!before && !IsAligned(type)) || (before && IsAligned(type)))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 "---Inserting " + STR(rt->unql) + " in the types to be declared");
                  returned_types.insert(rt->unql);
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Record type without named unqualified");
               const auto field_types = CGetFieldTypes(type);
               for(const auto& field_type : field_types)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 "-->Considering field type (" + STR(field_type->index) + ") " + STR(field_type));
                  bool pointer_to_unnamed_structure = [&]() {
                     if(!IsPointerType(field_type))
                     {
                        return false;
                     }
                     const auto pointed_type = CGetPointedType(field_type);
                     if(GetPointer<const record_type>(pointed_type) &&
                        GetPointer<const record_type>(pointed_type)->name->get_kind() != type_decl_K)
                     {
                        return true;
                     }
                     if(GetPointer<const union_type>(pointed_type) &&
                        GetPointer<const union_type>(pointed_type)->name->get_kind() != type_decl_K)
                     {
                        return true;
                     }
                     return false;
                  }();
                  /// Non pointer fields must be declared before structs, pointer fields can be declared after; in
                  /// some cases they must be declared after (circular dependencies)
                  if(before)
                  {
                     if(!IsPointerType(field_type) || !pointer_to_unnamed_structure)
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
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 "<--Considered field type (" + STR(field_type->index) + ") " + STR(field_type));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
            }
         }
         break;
      }
      case union_type_K:
      {
         if(recursion)
         {
            if(before)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "---Inserting " + STR(type) + " in the types to be declared");
               returned_types.insert(type);
            }
         }
         else
         {
            const auto ut = GetPointerS<const union_type>(type);
            if(ut->unql && (GetPointerS<const union_type>(ut->unql)->name || without_transformation))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Union type with named unqualified");
               if((!before && !IsAligned(type)) || (before && IsAligned(type)))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 "---Inserting " + STR(ut->unql) + " in the types to be declared");
                  returned_types.insert(ut->unql);
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Union type without named unqualified");
               const auto field_types = CGetFieldTypes(type);
               for(const auto& field_type : field_types)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 "-->Considering field type (" + STR(field_type->index) + ") " + STR(field_type));
                  const auto pointer_to_unnamed_structure = [&]() {
                     if(!IsPointerType(field_type))
                     {
                        return false;
                     }
                     const auto pointed_type = CGetPointedType(field_type);
                     if(GetPointer<const record_type>(pointed_type) &&
                        GetPointer<const record_type>(pointed_type)->name->get_kind() != type_decl_K)
                     {
                        return true;
                     }
                     if(GetPointer<const union_type>(pointed_type) &&
                        GetPointer<const union_type>(pointed_type)->name->get_kind() != type_decl_K)
                     {
                        return true;
                     }
                     return false;
                  }();
                  /// Non pointer fields must be declared before structs, pointer fields can be declared after; in
                  /// some cases they must be declared after (circular dependencies)
                  if(before)
                  {
                     if(!IsPointerType(field_type) || !pointer_to_unnamed_structure)
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
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 "<--Considered field type (" + STR(field_type->index) + ") " + STR(field_type));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
            }
         }
         break;
      }
      case enumeral_type_K:
      {
         if(recursion)
         {
            if(before)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "---Inserting " + STR(type) + " in the types to be declared");
               returned_types.insert(type);
            }
         }
         else
         {
            const auto* et = GetPointerS<const enumeral_type>(type);
            if(et->unql && GetPointerS<const enumeral_type>(et->unql)->name)
            {
               if(before)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                 "---Inserting " + STR(et->unql) + " in the types to be declared");
                  returned_types.insert(et->unql);
               }
            }
         }
         break;
      }
      case boolean_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case integer_type_K:
      case real_type_K:
      {
         if(before)
         {
            const auto* tn = GetPointerS<const type_node>(type);
            if(tn->name && tn->name->get_kind() == type_decl_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "---Inserting " + STR(type) + " in the types to be declared");
               returned_types.insert(type);
            }
         }
         break;
      }
      case void_type_K:
      {
         break;
      }
      case method_type_K:
      case function_type_K:
      {
         if(before)
         {
            const auto return_type = GetFunctionReturnType(type);
            if(return_type)
            {
               RecursiveGetTypesToBeDeclared(returned_types, return_type, true, without_transformation, true);
            }
            const auto parameters = GetParameterTypes(type);
            for(const auto& par : parameters)
            {
               RecursiveGetTypesToBeDeclared(returned_types, par, true, without_transformation, true);
            }
         }
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case lang_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case set_type_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case tree_list_K:
      case tree_vec_K:
      case typename_type_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
      {
         THROW_UNREACHABLE("Unexpected node: " + STR(type));
         break;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  STR("<--Got types to be declared ") + (before ? "before" : "after") + " " + STR(type));
}

unsigned int tree_helper::GetRealType(const tree_managerConstRef& TM, unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return GetRealType(T)->index;
}

tree_nodeConstRef tree_helper::GetRealType(const tree_nodeConstRef& type)
{
   const auto utype = GetUnqualifiedType(type);
   return utype ? utype : type;
}

unsigned int tree_helper::get_type_index(const tree_managerConstRef& TM, const unsigned int index,
                                         long long int& vec_size, bool& is_a_pointer, bool& is_a_function)
{
   is_a_pointer = false;
   is_a_function = false;
   vec_size = 0;
   const auto T = TM->GetTreeNode(index);
   auto Type = CGetType(T);
   THROW_ASSERT(Type, "expected a type index " + STR(index) + " " + T->ToString());
   const auto type_index = Type->index;
   if(Type->get_kind() == pointer_type_K)
   {
      is_a_pointer = true;
   }
   else if(Type->get_kind() == reference_type_K)
   {
      is_a_pointer = true; // reference objects are assimilated to pointers
   }
   else if(Type->get_kind() == array_type_K)
   {
      const auto at = GetPointer<const array_type>(Type);
      if(!at->domn)
      {
         is_a_pointer = true;
      }
   }
   else if(T->get_kind() == function_decl_K)
   {
      is_a_function = true;
   }
   return type_index;
}

tree_nodeConstRef tree_helper::GetFunctionReturnType(const tree_nodeConstRef& tn, bool void_as_null)
{
   tree_nodeConstRef fun_type;
   switch(tn->get_kind())
   {
      case function_decl_K:
      {
         const auto fd = GetPointerS<const function_decl>(tn);
         fun_type = fd->type;
         break;
      }
      case method_type_K:
      case function_type_K:
      {
         fun_type = tn;
         break;
      }
      case array_type_K:
      case binfo_K:
      case block_K:
      case boolean_type_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case const_decl_K:
      case constructor_K:
      case enumeral_type_K:
      case field_decl_K:
      case identifier_node_K:
      case integer_type_K:
      case label_decl_K:
      case lang_type_K:
      case namespace_decl_K:
      case offset_type_K:
      case pointer_type_K:
      case parm_decl_K:
      case qual_union_type_K:
      case real_type_K:
      case record_type_K:
      case reference_type_K:
      case result_decl_K:
      case set_type_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case typename_type_K:
      case union_type_K:
      case vector_type_K:
      case void_type_K:
      case var_decl_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
      {
         break;
      }
   }
   if(fun_type->get_kind() == function_type_K || fun_type->get_kind() == method_type_K)
   {
      const auto ft = GetPointerS<const function_type>(fun_type);
      THROW_ASSERT(ft, "NodeId is not related to a valid function type");
      if(!void_as_null || ft->retn->get_kind() != void_type_K)
      {
         return ft->retn;
      }
      else
      {
         return {};
      }
   }
   THROW_UNREACHABLE("Not supported tree node type " + tn->get_kind_text());
   return {};
}

unsigned int tree_helper::get_pointed_type(const tree_managerConstRef& TM, const int unsigned index)
{
   const auto T = TM->GetTreeNode(index);
   switch(T->get_kind())
   {
      case(pointer_type_K):
      {
         const auto pt = GetPointer<const pointer_type>(T);
         return pt->ptd->index;
      }
      case reference_type_K:
      {
         const auto rt = GetPointer<const reference_type>(T);
         return rt->refd->index;
      }
      case(function_type_K):
      {
         const auto ft = GetPointer<const function_type>(T);
         return get_pointed_type(TM, ft->retn->index);
      }
      case(method_type_K):
      {
         const auto mt = GetPointer<const method_type>(T);
         return get_pointed_type(TM, mt->retn->index);
      }
      case array_type_K:
      case binfo_K:
      case block_K:
      case boolean_type_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case const_decl_K:
      case constructor_K:
      case enumeral_type_K:
      case field_decl_K:
      case function_decl_K:
      case identifier_node_K:
      case integer_type_K:
      case label_decl_K:
      case lang_type_K:
      case namespace_decl_K:
      case offset_type_K:
      case parm_decl_K:
      case qual_union_type_K:
      case real_type_K:
      case record_type_K:
      case result_decl_K:
      case set_type_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case typename_type_K:
      case union_type_K:
      case vector_type_K:
      case void_type_K:
      case var_decl_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
      {
         THROW_ASSERT(false, "Index " + STR(index) + " does not correspond to a pointer type");
      }
   }
   return 0;
}

tree_nodeConstRef tree_helper::CGetPointedType(const tree_nodeConstRef& pointer)
{
   switch(pointer->get_kind())
   {
      case pointer_type_K:
      {
         const auto pt = GetPointerS<const pointer_type>(pointer);
         return pt->ptd;
      }
      case reference_type_K:
      {
         const auto pt = GetPointerS<const reference_type>(pointer);
         return pt->refd;
      }
      case function_type_K:
      {
         const auto ft = GetPointerS<const function_type>(pointer);
         return CGetPointedType(ft->retn);
      }
      case method_type_K:
      {
         const auto mt = GetPointerS<const method_type>(pointer);
         return CGetPointedType(mt->retn);
      }
      case array_type_K:
      case binfo_K:
      case block_K:
      case boolean_type_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case constructor_K:
      case enumeral_type_K:
      case identifier_node_K:
      case integer_type_K:
      case lang_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case real_type_K:
      case record_type_K:
      case set_type_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case tree_list_K:
      case tree_vec_K:
      case typename_type_K:
      case union_type_K:
      case vector_type_K:
      case void_type_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
      {
         THROW_UNREACHABLE(STR(pointer) + ":" + pointer->get_kind_text() + " does not correspond to a pointer type");
      }
   }
   return {};
}

unsigned int tree_helper::GetElements(const tree_managerConstRef& TM, const unsigned int index)
{
   return CGetElements(TM->GetTreeNode(index))->index;
}

tree_nodeConstRef tree_helper::CGetElements(const tree_nodeConstRef& type)
{
   const auto at = GetPointer<const array_type>(type);
   if(at)
   {
      return at->elts;
   }
   const auto vt = GetPointer<const vector_type>(type);
   if(vt)
   {
      return vt->elts;
   }
   THROW_UNREACHABLE("Tree node of type " + type->get_kind_text());
   return {};
}

std::string tree_helper::get_type_name(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto type = CGetType(TM->GetTreeNode(index));
   THROW_ASSERT(type, "Node type not type_node");
   const auto tn = GetPointerS<const type_node>(type);
   if(tn->name)
   {
      tree_nodeRef name;
      if(tn->name->get_kind() == type_decl_K)
      {
         name = GetPointerS<const type_decl>(tn->name)->name;
         if(!name)
         {
            return "Internal_" + STR(type->index);
         }
      }
      else
      {
         name = tn->name;
      }
      THROW_ASSERT(name && name->get_kind() == identifier_node_K, "Not an identifier node:" + STR(index));
      return GetPointerS<const identifier_node>(name)->strg;
   }
   else
   {
      return "Internal_" + STR(type->index);
   }
}

std::string tree_helper::GetTypeName(const tree_nodeConstRef& type)
{
   THROW_ASSERT(type, "expected a type");

   switch(type->get_kind())
   {
      case pointer_type_K:
      {
         return GetTypeName((GetPointerS<const pointer_type>(type)->ptd)) + "*";
      }
      case reference_type_K:
      {
         return GetTypeName((GetPointerS<const reference_type>(type)->refd)) + "&";
      }
      case record_type_K:
      {
         const auto rect = GetPointerS<const record_type>(type);
         std::string nt;
         if(rect->name)
         {
            if(rect->name->get_kind() == type_decl_K)
            {
               const auto td = GetPointerS<const type_decl>(rect->name);
               if(td->name->get_kind() == identifier_node_K)
               {
                  const auto idn = GetPointerS<const identifier_node>(td->name);
                  nt = idn->strg;
               }
               else
               {
                  THROW_ERROR("unexpected record type pattern: " + STR(type));
               }
            }
            else if(rect->name->get_kind() == identifier_node_K)
            {
               const auto idn = GetPointerS<const identifier_node>(rect->name);
               nt = "struct " + NormalizeTypename(idn->strg);
            }
            else
            {
               THROW_ERROR("unexpected record type pattern: " + STR(type));
            }
         }
         else
         {
            return "_unnamed_" + STR(type->index);
         }
         if(SC_tmpl_class.find(nt) == SC_tmpl_class.end())
         {
            return nt;
         }
         else if(rect->tmpl_args) /*the class has template parameters*/
         {
            const auto rtv = GetPointerS<const tree_vec>(rect->tmpl_args);
            THROW_ASSERT(rtv->lngt == 1 || nt == "sc_port", "Expected just one element");
            return GetTypeName(rtv->list_of_op[0]);
         }
         else
         {
            THROW_ERROR("Unexpected template parameter pattern");
         }
         return ""; // unreachable code
      }
      case union_type_K:
      {
         const auto unt = GetPointerS<const union_type>(type);
         std::string nt;
         if(unt->name)
         {
            if(unt->name->get_kind() == type_decl_K)
            {
               const auto td = GetPointerS<const type_decl>(unt->name);
               if(td->name->get_kind() == identifier_node_K)
               {
                  const auto idn = GetPointerS<const identifier_node>(td->name);
                  nt = idn->strg;
               }
               else
               {
                  THROW_ERROR("unexpected record type pattern: " + STR(type));
               }
            }
            else if(unt->name->get_kind() == identifier_node_K)
            {
               const auto idn = GetPointerS<const identifier_node>(unt->name);
               nt = "union " + idn->strg;
            }
            else
            {
               THROW_ERROR("unexpected record type pattern: " + STR(type));
            }
         }
         else
         {
            return "_unnamed_" + STR(type->index);
         }
         if(SC_tmpl_class.find(nt) == SC_tmpl_class.end())
         {
            return nt;
         }
         else
         {
            THROW_ERROR("Unexpected template parameter pattern");
         }
         return ""; // unreachable code
      }
      case array_type_K:
      {
         const auto at = GetPointerS<const array_type>(type);
         std::string vec_size_string;
         if(at->domn)
         {
            const auto domain = at->domn;
            const auto it = GetPointer<const integer_type>(domain);
            THROW_ASSERT(it, "expected an integer type as array domain");
            if(it->max)
            {
               const auto ic = GetPointer<const integer_cst>(it->max);
               if(ic)
               {
                  const auto vec_size = GetConstValue(it->max) + 1;
                  vec_size_string = "[" + STR(vec_size) + "]";
               }
               else
               {
                  vec_size_string = "[]";
               }
            }
         }
         return GetTypeName(at->elts) + vec_size_string;
      }
      case enumeral_type_K:
      {
         const auto et = GetPointerS<const enumeral_type>(type);
         if(et->name)
         {
            if(et->name->get_kind() == type_decl_K)
            {
               const auto td = GetPointerS<const type_decl>(et->name);
               const auto in = GetPointerS<const identifier_node>(td->name);
               return in->strg;
            }
            else if(et->name->get_kind() == identifier_node_K)
            {
               const auto in = GetPointerS<const identifier_node>(et->name);
               return "enum " + in->strg;
            }
         }
         return "enum Internal_" + STR(type->index);
      }
      case boolean_type_K:
      case integer_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case real_type_K:
      case complex_type_K:
      case void_type_K:
      case vector_type_K:
      {
         const auto tnode = GetPointerS<const type_node>(type);
         if(!tnode->name)
         {
            if(type->get_kind() == integer_type_K)
            {
               return "int";
            }
            return STR(type->index);
         }
         if(tnode->name->get_kind() == type_decl_K)
         {
            const auto tdecl = GetPointerS<const type_decl>(tnode->name);
            THROW_ASSERT(tdecl->name->get_kind() == identifier_node_K, "unexpected type name pattern");
            const auto idn = GetPointerS<const identifier_node>(tdecl->name);
            return idn->strg;
         }
         else if(tnode->name->get_kind() == identifier_node_K)
         {
            const auto idn = GetPointerS<const identifier_node>(tnode->name);
            return idn->strg;
         }
         else
         {
            THROW_UNREACHABLE(std::string("unexpected builtin type pattern ") + type->get_kind_text() + " " +
                              STR(type));
         }
         break;
      }
      case function_type_K:
      {
         std::string retn = GetTypeName(GetPointerS<const function_type>(type)->retn);
         retn += "(*)(";
         if(GetPointerS<const function_type>(type)->prms)
         {
            retn += GetTypeName(GetPointerS<const function_type>(type)->prms);
         }
         retn += ")";
         return retn;
      }
      case method_type_K:
      {
         std::string retn = GetTypeName(GetPointerS<const method_type>(type)->retn);
         retn += "(*)(";
         retn += GetTypeName(GetPointerS<const method_type>(type)->prms);
         retn += ")";
         return retn;
      }
      case tree_list_K:
      {
         auto tl = GetPointerS<const tree_list>(type);
         std::string retn;
         if(tl->valu)
         {
            retn = GetTypeName(tl->valu);
         }
         std::list<const tree_list*> tl_list;
         while(tl->chan)
         {
            tl = GetPointerS<const tree_list>(tl->chan);
            tl_list.push_back(tl);
         }
         for(const auto& valu : tl_list)
         {
            retn += "," + GetTypeName(valu->valu);
         }
         return retn;
      }
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      case aggr_init_expr_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case lang_type_K:
      case lut_expr_K:
      case offset_type_K:
      case qual_union_type_K:
      case set_type_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref461_K:
      case target_mem_ref_K:
      case template_type_parm_K:
      case tree_vec_K:
      case type_argument_pack_K:
      case typename_type_K:
      default:
         THROW_UNREACHABLE(std::string("unexpected type pattern ") + type->get_kind_text() + " " + STR(type));
         return "";
   }
   return "";
}

void tree_helper::get_parameter_types(const tree_managerConstRef& TM, const unsigned int index,
                                      std::list<unsigned int>& params)
{
   const auto pv = GetParameterTypes(TM->GetTreeNode(index));
   std::transform(pv.begin(), pv.end(), std::back_inserter(params),
                  [](const tree_nodeConstRef& tn) { return tn->index; });
}

std::vector<tree_nodeConstRef> tree_helper::GetParameterTypes(const tree_nodeConstRef& ftype)
{
   std::vector<tree_nodeConstRef> params;
   const auto Type = CGetType(ftype);
   THROW_ASSERT(Type, "expected a type");
   THROW_ASSERT(Type->get_kind() == function_type_K || Type->get_kind() == method_type_K,
                "Type " + STR(Type) + " from " + STR(ftype) + " does not correspond to a function type");
   if(GetPointerS<const function_type>(Type)->prms)
   {
      auto tl = GetPointerS<const tree_list>(GetPointerS<const function_type>(Type)->prms);
      params.push_back(tl->valu);
      while(tl->chan)
      {
         tl = GetPointerS<const tree_list>(tl->chan);
         params.push_back(tl->valu);
      }
   }
   return params;
}

bool tree_helper::IsSameType(const tree_nodeConstRef& tn0, const tree_nodeConstRef& tn1)
{
   const auto tn0_type = CGetType(tn0);
   const auto tn1_type = CGetType(tn1);
   return tn0_type->get_kind() == tn1_type->get_kind() && Size(tn0_type) == Size(tn1_type) &&
          (tn0_type->get_kind() != integer_type_K || GetPointerS<const integer_type>(tn0_type)->unsigned_flag ==
                                                         GetPointerS<const integer_type>(tn1_type)->unsigned_flag) &&
          (tn0_type->get_kind() != enumeral_type_K || GetPointerS<const enumeral_type>(tn0_type)->unsigned_flag ==
                                                          GetPointerS<const enumeral_type>(tn1_type)->unsigned_flag);
}

unsigned int tree_helper::get_type_index(const tree_managerConstRef& TM, const unsigned int index)
{
   bool is_a_pointer;
   bool is_a_function;
   long long int vec_size;
   return get_type_index(TM, index, vec_size, is_a_pointer, is_a_function);
}

std::vector<tree_nodeConstRef> tree_helper::CGetFieldTypes(const tree_nodeConstRef& type)
{
   std::vector<tree_nodeConstRef> ret;
   if(type->get_kind() == record_type_K)
   {
      const auto rt = GetPointerS<const record_type>(type);
      for(const auto& list_of_fld : rt->list_of_flds)
      {
         if(list_of_fld->get_kind() == type_decl_K)
         {
            continue;
         }
         if(list_of_fld->get_kind() == function_decl_K)
         {
            continue;
         }
         ret.push_back(CGetType(list_of_fld));
      }
   }
   else if(type->get_kind() == union_type_K)
   {
      const auto ut = GetPointerS<const union_type>(type);
      for(const auto& list_of_fld : ut->list_of_flds)
      {
         ret.push_back(CGetType(list_of_fld));
      }
   }
   else
   {
      THROW_UNREACHABLE("Asking fields type of not a type. Tree node is " + type->ToString());
   }
   return ret;
}

unsigned int tree_helper::get_field_idx(const tree_managerConstRef& TM, const unsigned int index, unsigned int idx)
{
   const auto node = TM->GetTreeNode(index);
   return GetFieldIdx(node, idx)->index;
}

tree_nodeRef tree_helper::GetFieldIdx(const tree_nodeConstRef& type, unsigned int idx)
{
   THROW_ASSERT(GetPointer<const record_type>(type) || GetPointer<const union_type>(type),
                "expected record or union type");
   const auto rt = GetPointer<const record_type>(type);
   const auto ut = GetPointer<const union_type>(type);
   if(rt)
   {
      THROW_ASSERT(idx < rt->list_of_flds.size(), "unexpected index for list of fields");
      return rt->list_of_flds[idx];
   }
   else if(ut)
   {
      THROW_ASSERT(idx < ut->list_of_flds.size(), "unexpected index for list of fields");
      return ut->list_of_flds[idx];
   }
   THROW_ERROR("unexpected behavior");
   return nullptr;
}

tree_nodeConstRef tree_helper::CGetType(const tree_nodeConstRef& node)
{
   switch(node->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<const call_expr>(node);
         return ce->type;
      }
      case gimple_assign_K:
      {
         const auto gm = GetPointerS<const gimple_assign>(node);
         return CGetType(gm->op0);
      }
      case gimple_phi_K:
      {
         const auto gp = GetPointerS<const gimple_phi>(node);
         return CGetType(gp->res);
      }
      case gimple_return_K:
      {
         const auto gr = GetPointerS<const gimple_return>(node);
         return CGetType(gr->op);
      }
      case gimple_for_K:
      case gimple_while_K:
      {
         const auto gw = GetPointerS<const gimple_while>(node);
         return CGetType(gw->op0);
      }
      case gimple_asm_K:
      case gimple_bind_K:
      case gimple_resx_K:
      case gimple_switch_K:
      case gimple_label_K:
      case gimple_goto_K:
      case gimple_nop_K:
      case gimple_call_K:
      case gimple_cond_K:
      case gimple_multi_way_if_K:
      case gimple_pragma_K:
      {
         return {};
      }
      case lut_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto en = GetPointerS<const expr_node>(node);
         THROW_ASSERT(en->type, std::string("this NODE does not have a type: ") + node->get_kind_text());
         return en->type;
      }
      case integer_cst_K:
      case real_cst_K:
      {
         const auto cn = GetPointerS<const cst_node>(node);
         return cn->type;
      }
      case string_cst_K:
      case vector_cst_K:
      case complex_cst_K:
      {
         const auto cn = GetPointerS<const cst_node>(node);
         return cn->type ? cn->type : node;
      }
      case constructor_K:
      {
         const auto c = GetPointerS<const constructor>(node);
         return c->type;
      }
      case ssa_name_K:
      {
         const auto sa = GetPointerS<const ssa_name>(node);
         return sa->type;
      }
      case target_mem_ref_K:
      {
         const auto tm = GetPointerS<const target_mem_ref>(node);
         return CGetType(tm->orig);
      }
      case target_mem_ref461_K:
      {
         const auto tm = GetPointerS<const target_mem_ref461>(node);
         return tm->type;
      }
      case CASE_DECL_NODES:
      {
         const auto dn = GetPointerS<const decl_node>(node);
         return dn->type;
      }
      case CASE_TYPE_NODES:
      {
         return node;
      }
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case error_mark_K:
      case gimple_predict_K:
      case identifier_node_K:
      case statement_list_K:
      case target_expr_K:
      case tree_list_K:
      case tree_vec_K:
      case void_cst_K:
      default:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, std::string("Node not yet supported ") + node->get_kind_text());
      }
   }
   return node;
}

bool tree_helper::is_an_enum(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsEnumType(T);
}

bool tree_helper::IsEnumType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   return Type->get_kind() == enumeral_type_K;
}

bool tree_helper::is_a_struct(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsStructType(T);
}

bool tree_helper::IsStructType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   return Type->get_kind() == record_type_K;
}

bool tree_helper::is_an_union(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsUnionType(T);
}

bool tree_helper::IsUnionType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   return Type->get_kind() == union_type_K;
}

bool tree_helper::is_a_complex(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsComplexType(T);
}

bool tree_helper::IsComplexType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   return Type->get_kind() == complex_type_K;
}

static void getBuiltinFieldTypes(const tree_nodeConstRef& type, std::list<tree_nodeConstRef>& listOfTypes,
                                 CustomUnorderedSet<unsigned int>& already_visited)
{
   if(already_visited.count(type->index))
   {
      return;
   }
   already_visited.insert(type->index);
   if(type->get_kind() == record_type_K)
   {
      const auto rt = GetPointerS<const record_type>(type);
      for(const auto& fld : rt->list_of_flds)
      {
         if(fld->get_kind() == type_decl_K)
         {
            continue;
         }
         if(fld->get_kind() == function_decl_K)
         {
            continue;
         }
         const auto fdType = tree_helper::CGetType(fld);
         getBuiltinFieldTypes(fdType, listOfTypes, already_visited);
      }
   }
   else if(type->get_kind() == union_type_K)
   {
      const auto ut = GetPointerS<const union_type>(type);
      for(const auto& fld : ut->list_of_flds)
      {
         const auto fdType = tree_helper::CGetType(fld);
         getBuiltinFieldTypes(fdType, listOfTypes, already_visited);
      }
   }
   else if(type->get_kind() == array_type_K)
   {
      const auto at = GetPointerS<const array_type>(type);
      THROW_ASSERT(at->elts, "elements type expected");
      getBuiltinFieldTypes(at->elts, listOfTypes, already_visited);
   }
   else if(type->get_kind() == vector_type_K)
   {
      const auto vt = GetPointerS<const vector_type>(type);
      THROW_ASSERT(vt->elts, "elements type expected");
      getBuiltinFieldTypes(vt->elts, listOfTypes, already_visited);
   }
   else
   {
      listOfTypes.push_back(type);
   }
}

static bool same_size_fields(const tree_nodeConstRef& t)
{
   std::list<tree_nodeConstRef> listOfTypes;
   CustomUnorderedSet<unsigned int> already_visited;
   getBuiltinFieldTypes(t, listOfTypes, already_visited);
   if(listOfTypes.empty())
   {
      return false;
   }
   if(tree_helper::IsStructType(t))
   {
      for(const auto& fld : GetPointerS<const record_type>(t)->list_of_flds)
      {
         if(GetPointerS<const field_decl>(fld)->is_bitfield())
         {
            return false;
         }
      }
   }

   const auto sizeFlds = tree_helper::SizeAlloc(listOfTypes.front());
   if(ceil_pow2(sizeFlds) != sizeFlds)
   {
      return false;
   }
   for(const auto& fldType : listOfTypes)
   {
      if(sizeFlds != tree_helper::SizeAlloc(fldType))
      {
         return false;
      }
   }
   return true;
}

bool tree_helper::is_an_array(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsArrayEquivType(T);
}

bool tree_helper::IsArrayEquivType(const tree_nodeConstRef& type)
{
   THROW_ASSERT(type, "expected a type index");
   if(type->get_kind() == array_type_K)
   {
      return true;
   }
   else if(type->get_kind() == record_type_K || type->get_kind() == union_type_K)
   {
      return same_size_fields(type);
   }
   return false;
}

bool tree_helper::IsArrayType(const tree_nodeConstRef& _type)
{
   const auto type = CGetType(_type);
   THROW_ASSERT(type, "expected a type index");
   return type->get_kind() == array_type_K;
}

tree_nodeConstRef tree_helper::CGetArrayBaseType(const tree_nodeConstRef& type)
{
   std::list<tree_nodeConstRef> listOfTypes;
   CustomUnorderedSet<unsigned int> already_visited;
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   getBuiltinFieldTypes(Type, listOfTypes, already_visited);
   THROW_ASSERT(!listOfTypes.empty(), "at least one type is expected");
   return listOfTypes.front();
}

bool tree_helper::is_a_pointer(const tree_managerConstRef& TM, const unsigned int index)
{
   return IsPointerType(TM->GetTreeNode(index));
}

bool tree_helper::IsPointerType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   if(Type->get_kind() == pointer_type_K)
   {
      return true;
   }
   else if(Type->get_kind() == reference_type_K)
   {
      return true; // reference objects are assimilated to pointers
   }
   else if(Type->get_kind() == array_type_K)
   {
      const auto at = GetPointerS<const array_type>(Type);
      if(!at->domn)
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_a_function(const tree_managerConstRef& TM, const unsigned int index)
{
   return IsFunctionDeclaration(TM->GetTreeNode(index));
}

bool tree_helper::IsFunctionDeclaration(const tree_nodeConstRef& type)
{
   THROW_ASSERT(type, "expected a type");
   return type->get_kind() == function_decl_K;
}

bool tree_helper::is_a_vector(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsVectorType(T);
}

bool tree_helper::IsVectorType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   return Type->get_kind() == vector_type_K;
}

bool tree_helper::is_a_misaligned_vector(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   THROW_ASSERT(T, "this index does not exist: " + STR(index));
   if(!IsVectorType(T))
   {
      return false;
   }
   if(GetPointer<const misaligned_indirect_ref>(T))
   {
      return true;
   }
   const auto Type = CGetType(T);
   THROW_ASSERT(Type, "expected a type index");
   const auto vt = GetPointer<const vector_type>(Type);
   THROW_ASSERT(vt, "expected a vector type");
   return vt->algn != SizeAlloc(Type);
}

bool tree_helper::is_an_addr_expr(const tree_managerConstRef& TM, const unsigned int index)
{
   return TM->GetTreeNode(index)->get_kind() == addr_expr_K;
}

bool tree_helper::HasToBeDeclared(const tree_managerConstRef& TM, const tree_nodeConstRef& type)
{
   THROW_ASSERT(GetPointer<const type_node>(type),
                "Tree node " + STR(type) + " is not a type_node but " + type->get_kind_text());
   if(GetPointer<const type_node>(type)->name)
   {
      const auto name = GetPointer<const type_node>(type)->name;
      if(name->get_kind() == type_decl_K)
      {
         const auto td = GetPointer<const type_decl>(name);
         if(td->include_name == "<built-in>")
         {
            return false;
         }
         if(GetPointer<const complex_type>(type))
         {
            const auto name1 = PrintType(TM, type);
            const auto splitted = string_to_container<std::vector<std::string>>(name1, " ");
            if(splitted.size() > 1 &&
               (splitted[0] == "_Complex" || splitted[0] == "__complex__" || splitted[0] == "complex"))
            {
               return false;
            }
         }
         return true;
      }
   }
   return type->get_kind() == record_type_K || type->get_kind() == union_type_K || type->get_kind() == enumeral_type_K;
}

bool tree_helper::is_function_type(const tree_managerConstRef& TM, const unsigned int index)
{
   return IsFunctionType(TM->GetTreeNode(index));
}

bool tree_helper::IsFunctionType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   return Type->get_kind() == function_type_K || Type->get_kind() == method_type_K;
}

bool tree_helper::is_function_pointer_type(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsFunctionPointerType(T);
}

bool tree_helper::IsFunctionPointerType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   if(Type->get_kind() == pointer_type_K)
   {
      const auto ptd = GetPointerS<const pointer_type>(Type)->ptd;
      if(ptd->get_kind() == function_type_K || ptd->get_kind() == method_type_K)
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_bool(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsBooleanType(T);
}

bool tree_helper::IsBooleanType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   if(Type->get_kind() == boolean_type_K)
   {
      return true;
   }
   const auto type_name = GetTypeName(Type);
   return type_name == "sc_logic" || type_name == "sc_in_resolved" || type_name == "sc_inout_resolved" ||
          type_name == "sc_out_resolved" || type_name == "sc_in_clk" || type_name == "sc_inout_clk" ||
          type_name == "sc_out_clk" || type_name == "sc_bit" || type_name == "sc_clock";
}

bool tree_helper::is_a_void(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsVoidType(T);
}

bool tree_helper::IsVoidType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   return Type->get_kind() == void_type_K;
}

bool tree_helper::is_natural(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto var = TM->GetTreeNode(index);
   return IsPositiveIntegerValue(var);
}

bool tree_helper::IsPositiveIntegerValue(const tree_nodeConstRef& type)
{
   if(GetPointer<const ssa_name>(type) && GetPointer<const ssa_name>(type)->min)
   {
      const auto& minimum = GetPointer<const ssa_name>(type)->min;
      THROW_ASSERT(minimum->get_kind() == integer_cst_K, "expected an integer const: " + STR(type));
      const auto min_value = GetConstValue(minimum);
      return min_value >= 0;
   }
   return false;
}

bool tree_helper::is_int(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsSignedIntegerType(T);
}

bool tree_helper::IsSignedIntegerType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   if(Type->get_kind() == enumeral_type_K)
   {
      return !GetPointerS<const enumeral_type>(Type)->unsigned_flag;
   }
   else if(Type->get_kind() == integer_type_K)
   {
      return !GetPointerS<const integer_type>(Type)->unsigned_flag;
   }
   const auto type_name = GetTypeName(Type);
   return type_name == "sc_int";
}

bool tree_helper::is_real(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsRealType(T);
}

bool tree_helper::IsRealType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   return Type->get_kind() == real_type_K;
}

bool tree_helper::is_unsigned(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsUnsignedIntegerType(T);
}

bool tree_helper::IsUnsignedIntegerType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   if(Type->get_kind() == enumeral_type_K)
   {
      return GetPointerS<const enumeral_type>(Type)->unsigned_flag;
   }
   if(Type->get_kind() == integer_type_K)
   {
      return GetPointerS<const integer_type>(Type)->unsigned_flag;
   }
   const auto type_name = GetTypeName(Type);
   return type_name == "sc_uint" || type_name == "sc_lv" || type_name == "sc_in_rv" || type_name == "sc_out_rv" ||
          type_name == "sc_inout_rv" || type_name == "sc_bv" || type_name == "sc_signal_rv";
}

bool tree_helper::is_scalar(const tree_managerConstRef& TM, const unsigned int var)
{
   return IsScalarType(TM->GetTreeNode(var));
}

bool tree_helper::IsScalarType(const tree_nodeConstRef& type)
{
   return IsSignedIntegerType(type) || IsRealType(type) || IsUnsignedIntegerType(type) || IsBooleanType(type);
}

bool tree_helper::is_module(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string mod_st = "sc_module";
   const std::string mod_name_st = "sc_module_name";
   const std::string ifc_st = "sc_interface";
   const auto* rt = GetPointer<const record_type>(TM->GetTreeNode(index));
   if(rt && rt->binf)
   {
      const auto bi = GetPointer<const binfo>(rt->binf);
      if(bi && look_for_binfo_inheritance(bi, mod_st) && !look_for_binfo_inheritance(bi, mod_name_st) &&
         !look_for_binfo_inheritance(bi, ifc_st))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_builtin_channel(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(GetPointer<const record_type>(TM->GetTreeNode(index)), "a record type is expected");
   std::string rec_name = GetRecordTypeName(TM->GetTreeNode(index));
   return rec_name == "sc_fifo" || rec_name == "tlm_fifo" || rec_name == "sc_mutex" || rec_name == "sc_semaphore";
}

bool tree_helper::is_channel(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string mod_st = "sc_module";
   const std::string ifc_st = "sc_interface";
   const auto* rt = GetPointer<const record_type>(TM->GetTreeNode(index));
   if(rt && rt->binf)
   {
      const auto bi = GetPointer<const binfo>(rt->binf);
      if(bi && look_for_binfo_inheritance(bi, mod_st) && look_for_binfo_inheritance(bi, ifc_st))
      {
         return true;
      }
      else
      {
         return is_builtin_channel(TM, index);
      }
   }
   else if(rt)
   {
      return is_builtin_channel(TM, index);
   }
   return false;
}

bool tree_helper::is_signal(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string ifc_st = "sc_interface";
   const std::string pch_st = "sc_prim_channel";
   const std::string sig_st = "sc_signal";
   const std::string clock_st = "sc_clock";
   const auto* rt = GetPointer<const record_type>(TM->GetTreeNode(index));
   if(rt && rt->binf)
   {
      const auto bi = GetPointer<const binfo>(rt->binf);
      if(bi && look_for_binfo_inheritance(bi, pch_st) &&
         ((look_for_binfo_inheritance(bi, sig_st) && look_for_binfo_inheritance(bi, ifc_st))) &&
         rt->get_maybe_name() != clock_st)
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_clock(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string ifc_st = "sc_interface";
   const std::string pch_st = "sc_prim_channel";
   const std::string clock_st = "sc_clock";
   const auto* rt = GetPointer<const record_type>(TM->GetTreeNode(index));
   if(rt && rt->binf)
   {
      const auto bi = GetPointer<const binfo>(rt->binf);
      if(bi && (look_for_binfo_inheritance(bi, ifc_st) && look_for_binfo_inheritance(bi, pch_st) &&
                rt->get_maybe_name() == clock_st))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_SC_BIND_PROXY_NIL(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   auto curr_tn = TM->GetTreeNode(index);
   if(curr_tn->get_kind() == addr_expr_K)
   {
      curr_tn = GetPointer<const addr_expr>(curr_tn)->op;
   }
   if(curr_tn->get_kind() == var_decl_K)
   {
      const auto vd = GetPointer<const var_decl>(curr_tn);
      if(vd->name && vd->name->get_kind() == identifier_node_K)
      {
         const auto id = GetPointer<const identifier_node>(vd->name);
         std::string strg = id->strg;
         return strg.find("SC_BIND_PROXY_NIL") != std::string::npos;
      }
   }
   return false;
}

bool tree_helper::is_port(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string port_st = "sc_port";
   const std::string sc_export_st = "sc_export";
   const auto* rt = GetPointer<const record_type>(TM->GetTreeNode(index));
   if(rt && rt->binf)
   {
      const auto bi = GetPointer<const binfo>(rt->binf);
      if(bi && (look_for_binfo_inheritance(bi, port_st) || rt->get_maybe_name().find(sc_export_st) == 0))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_in_port(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string sc_in_st = "sc_in";
   const auto* rt = GetPointer<const record_type>(TM->GetTreeNode(index));
   if(rt && rt->binf)
   {
      const auto bi = GetPointer<const binfo>(rt->binf);
      if(bi && look_for_binfo_inheritance(bi, sc_in_st))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_out_port(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string sc_out_st =
       "sc_out"; // several out port are actually inout port (e.g., sc_out_resolved and sc_out_rv)
   const auto* rt = GetPointer<const record_type>(TM->GetTreeNode(index));
   if(rt && rt->binf)
   {
      const auto bi = GetPointer<const binfo>(rt->binf);
      if(bi && look_for_binfo_inheritance(bi, sc_out_st))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_inout_port(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string sc_inout_st = "sc_inout";
   const auto* rt = GetPointer<const record_type>(TM->GetTreeNode(index));
   if(rt && rt->binf)
   {
      const auto bi = GetPointer<const binfo>(rt->binf);
      if(bi && look_for_binfo_inheritance(bi, sc_inout_st))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_event(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string event_st = "sc_event";
   const auto* rt = GetPointer<const record_type>(TM->GetTreeNode(index));
   if(rt && rt->binf)
   {
      const auto bi = GetPointer<const binfo>(rt->binf);
      if(bi && look_for_binfo_inheritance(bi, event_st))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_a_variable(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto node = TM->GetTreeNode(index);
   return IsVariableType(node);
}

bool tree_helper::IsVariableType(const tree_nodeConstRef& node)
{
   const auto node_kind = node->get_kind();
   switch(node_kind)
   {
      case integer_cst_K:
      case real_cst_K:
      case complex_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case ssa_name_K:
      case function_decl_K:
      case var_decl_K:
      case parm_decl_K:
      case label_decl_K:
      case result_decl_K:
      case call_expr_K:
      case aggr_init_expr_K:
         return true;
      /// The following one are not considered as variables, but as operations on the variables
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case const_decl_K:
      case constructor_K:
      case field_decl_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case string_cst_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
         return false;
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                          "tree_helper::is_a_variable - variable is not supported: " + tree_node::GetString(node_kind));
   }
   return true;
}

static tree_nodeRef check_for_simple_pointer_arithmetic(const tree_nodeRef& node,
                                                        std::vector<tree_nodeRef>* field_offset)
{
   switch(node->get_kind())
   {
      case gimple_assign_K:
      {
         const auto ga = GetPointerS<const gimple_assign>(node);
         if(ga->temporary_address)
         {
            const auto ae = GetPointer<const addr_expr>(ga->op1);
            if(ae)
            {
               return check_for_simple_pointer_arithmetic(ae->op, field_offset);
            }
            else
            {
               const auto ppe = GetPointer<const pointer_plus_expr>(ga->op1);
               if(ppe)
               {
                  if(field_offset)
                  {
                     field_offset->push_back(ppe->op1);
                  }
                  return check_for_simple_pointer_arithmetic(ppe->op0, field_offset);
               }
               else
               {
                  const auto ne = GetPointer<const nop_expr>(ga->op1);
                  if(ne)
                  {
                     return check_for_simple_pointer_arithmetic(ne->op, field_offset);
                  }
                  else
                  {
                     const auto vce = GetPointer<const view_convert_expr>(ga->op1);
                     if(vce)
                     {
                        return check_for_simple_pointer_arithmetic(vce->op, field_offset);
                     }
                     else
                     {
                        return check_for_simple_pointer_arithmetic(ga->op1, field_offset);
                     }
                  }
               }
            }
         }
         else if(GetPointer<const pointer_plus_expr>(ga->op1))
         {
            const auto ppe = GetPointer<const pointer_plus_expr>(ga->op1);
            if(field_offset)
            {
               field_offset->push_back(ppe->op1);
            }
            return check_for_simple_pointer_arithmetic(ppe->op0, field_offset);
         }
         else if(GetPointer<const nop_expr>(ga->op1))
         {
            const auto ne = GetPointer<const nop_expr>(ga->op1);
            return check_for_simple_pointer_arithmetic(ne->op, field_offset);
         }
         else if(GetPointer<const view_convert_expr>(ga->op1))
         {
            const auto vce = GetPointer<const view_convert_expr>(ga->op1);
            return check_for_simple_pointer_arithmetic(vce->op, field_offset);
         }
         else if(GetPointer<const addr_expr>(ga->op1))
         {
            const auto ae = GetPointer<const addr_expr>(ga->op1);
            if(ae->op->get_kind() != mem_ref_K && ae->op->get_kind() != var_decl_K &&
               ae->op->get_kind() != function_decl_K)
            {
               THROW_ERROR("unexpected condition");
            }
            return check_for_simple_pointer_arithmetic(ae->op, field_offset);
         }
         else
         {
            return {};
         }
      }
      case mem_ref_K:
      {
         const auto mr = GetPointer<const mem_ref>(node);
         if(field_offset)
         {
            field_offset->push_back(mr->op1);
         }
         return check_for_simple_pointer_arithmetic(mr->op0, field_offset);
      }
      case pointer_plus_expr_K:
      {
         const auto ppe = GetPointer<const pointer_plus_expr>(node);
         if(field_offset)
         {
            field_offset->push_back(ppe->op1);
         }
         return check_for_simple_pointer_arithmetic(ppe->op0, field_offset);
      }
      case view_convert_expr_K:
      {
         const auto vce = GetPointer<const view_convert_expr>(node);
         return check_for_simple_pointer_arithmetic(vce->op, field_offset);
      }
      case addr_expr_K:
      {
         const auto ae = GetPointer<const addr_expr>(node);
         if(ae->op->get_kind() != mem_ref_K && ae->op->get_kind() != var_decl_K &&
            ae->op->get_kind() != function_decl_K)
         {
            THROW_ERROR("unexpected condition");
         }
         return check_for_simple_pointer_arithmetic(ae->op, field_offset);
      }
      case array_ref_K:
      {
         const auto ar = GetPointer<const array_ref>(node);
         return check_for_simple_pointer_arithmetic(ar->op0, field_offset);
      }
      case parm_decl_K:
      case var_decl_K:
      {
         return node;
      }
      case ssa_name_K:
      {
         auto ssa = GetPointerS<const ssa_name>(node);
         auto defStmt = ssa->CGetDefStmt();
         if(defStmt->get_kind() == gimple_nop_K)
         {
            return node;
         }
         return check_for_simple_pointer_arithmetic(ssa->CGetDefStmt(), field_offset);
      }

      case target_mem_ref461_K:
      case component_ref_K:
      case realpart_expr_K:
      case imagpart_expr_K:
      case bit_field_ref_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case statement_list_K:
      case target_mem_ref_K:
      case tree_list_K:
      case tree_vec_K:
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
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
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
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case const_decl_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case result_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case type_decl_K:
      case CASE_FAKE_NODES:
      case gimple_asm_K:
      case gimple_bind_K:
      case gimple_call_K:
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
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case cond_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case fshl_expr_K:
      case fshr_expr_K:
      case bit_ior_concat_expr_K:
      case abs_expr_K:
      case alignof_expr_K:
      case arrow_expr_K:
      case bit_not_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case convert_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fix_trunc_expr_K:
      case float_expr_K:
      case indirect_ref_K:
      case misaligned_indirect_ref_K:
      case loop_expr_K:
      case lut_expr_K:
      case negate_expr_K:
      case non_lvalue_expr_K:
      case nop_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case truth_not_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      case CASE_TYPE_NODES:
      case array_range_ref_K:
      case error_mark_K:
      case target_expr_K:
      case paren_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case insertvalue_expr_K:
      case extractelement_expr_K:
      case insertelement_expr_K:
      case frem_expr_K:
      {
         return {};
      }
      default:
         THROW_UNREACHABLE("");
         return {};
   }
   return {};
}

unsigned int tree_helper::get_base_index(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto node = TM->GetTreeNode(index);
   const auto var = GetBaseVariable(node);
   return var ? var->index : 0;
}

tree_nodeRef tree_helper::GetBaseVariable(const tree_nodeRef& node, std::vector<tree_nodeRef>* field_offset)
{
   switch(node->get_kind())
   {
      case ssa_name_K:
      {
         const auto sn = GetPointerS<const ssa_name>(node);
         if(field_offset == nullptr && sn->use_set->is_a_singleton())
         {
            if(sn->use_set->variables.front()->get_kind() == function_decl_K)
            {
               return sn->use_set->variables.front();
            }
            else
            {
               return GetBaseVariable(sn->use_set->variables.front(), field_offset);
            }
         }
         std::vector<tree_nodeRef> local_field_offset;
         const auto base =
             check_for_simple_pointer_arithmetic(sn->CGetDefStmt(), field_offset ? &local_field_offset : nullptr);
         if(base)
         {
            if(field_offset && !local_field_offset.empty())
            {
               for(const auto& fld : local_field_offset)
               {
                  field_offset->push_back(fld);
               }
            }
            return GetBaseVariable(base, field_offset);
         }

         if(sn->var)
         {
            if(sn->var->get_kind() == function_decl_K)
            {
               return sn->var;
            }
            else
            {
               return GetBaseVariable(sn->var, field_offset);
            }
         }
         else
         {
            return node;
         }
      }
      case result_decl_K:
      case parm_decl_K:
      case var_decl_K:
      case string_cst_K:
      case integer_cst_K:
      {
         return node;
      }
      case indirect_ref_K:
      {
         const auto ir = GetPointerS<const indirect_ref>(node);
         return GetBaseVariable(ir->op, field_offset);
      }
      case misaligned_indirect_ref_K:
      {
         const auto mir = GetPointerS<const misaligned_indirect_ref>(node);
         return GetBaseVariable(mir->op, field_offset);
      }
      case mem_ref_K:
      {
         const auto mr = GetPointerS<const mem_ref>(node);
         auto baseVar = GetBaseVariable(mr->op0, field_offset);
         if(baseVar && field_offset && IsStructType(baseVar))
         {
            return baseVar;
         }
         else
         {
            return baseVar;
         }
      }
      case array_ref_K:
      {
         const auto ar = GetPointerS<const array_ref>(node);
         return GetBaseVariable(ar->op0, field_offset);
      }
      case component_ref_K:
      {
         const auto cr = GetPointerS<const component_ref>(node);
         return GetBaseVariable(cr->op0, field_offset);
      }
      case realpart_expr_K:
      {
         const auto rpe = GetPointerS<const realpart_expr>(node);
         return GetBaseVariable(rpe->op, field_offset);
      }
      case imagpart_expr_K:
      {
         const auto rpe = GetPointerS<const imagpart_expr>(node);
         return GetBaseVariable(rpe->op, field_offset);
      }
      case bit_field_ref_K:
      {
         const auto bfr = GetPointerS<const bit_field_ref>(node);
         return GetBaseVariable(bfr->op0, field_offset);
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref>(node);
         if(tmr->symbol)
         {
            return GetBaseVariable(tmr->symbol, field_offset);
         }
         else if(tmr->base)
         {
            return GetBaseVariable(tmr->base, field_offset);
         }
         else if(tmr->idx)
         {
            return GetBaseVariable(tmr->idx, field_offset);
         }
         else
         {
            THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                             "tree_helper::GetBaseVariable::target_mem_ref_K - variable type pattern not supported: " +
                                 STR(node));
         }
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref461>(node);
         if(tmr->base)
         {
            return GetBaseVariable(tmr->base, field_offset);
         }
         else if(tmr->idx)
         {
            return GetBaseVariable(tmr->idx, field_offset);
         }
         else if(tmr->idx2)
         {
            return GetBaseVariable(tmr->idx2, field_offset);
         }
         else
         {
            THROW_ERROR_CODE(
                NODE_NOT_YET_SUPPORTED_EC,
                "tree_helper::GetBaseVariable::target_mem_ref461_K - variable type pattern not supported: " +
                    STR(node));
         }
         break;
      }
      case addr_expr_K:
      {
         const auto ae = GetPointerS<const addr_expr>(node);
         const auto addr_expr_op = ae->op;

         switch(addr_expr_op->get_kind())
         {
            case ssa_name_K:
            case var_decl_K:
            case parm_decl_K:
            case string_cst_K:
            case result_decl_K:
            {
               return ae->op;
            }
            case array_ref_K:
            {
               const auto ar = GetPointerS<const array_ref>(addr_expr_op);
               if(ar->op1->get_kind() == integer_cst_K && GetConstValue(ar->op1) == 0)
               {
                  switch(ar->op0->get_kind())
                  {
                     case ssa_name_K:
                     case var_decl_K:
                     case parm_decl_K:
                     case string_cst_K:
                     {
                        return ar->op0;
                     }
                     case binfo_K:
                     case block_K:
                     case call_expr_K:
                     case aggr_init_expr_K:
                     case case_label_expr_K:
                     case complex_cst_K:
                     case const_decl_K:
                     case constructor_K:
                     case field_decl_K:
                     case function_decl_K:
                     case identifier_node_K:
                     case integer_cst_K:
                     case label_decl_K:
                     case namespace_decl_K:
                     case real_cst_K:
                     case result_decl_K:
                     case statement_list_K:
                     case target_expr_K:
                     case target_mem_ref_K:
                     case target_mem_ref461_K:
                     case translation_unit_decl_K:
                     case template_decl_K:
                     case using_decl_K:
                     case tree_list_K:
                     case tree_vec_K:
                     case type_decl_K:
                     case vector_cst_K:
                     case void_cst_K:
                     case error_mark_K:
                     case lut_expr_K:
                     case CASE_BINARY_EXPRESSION:
                     case CASE_CPP_NODES:
                     case CASE_FAKE_NODES:
                     case CASE_GIMPLE_NODES:
                     case CASE_PRAGMA_NODES:
                     case CASE_QUATERNARY_EXPRESSION:
                     case CASE_TERNARY_EXPRESSION:
                     case CASE_TYPE_NODES:
                     case CASE_UNARY_EXPRESSION:
                     default:
                        THROW_ERROR("addr_expr-array_ref[0] pattern not supported: " +
                                    std::string(addr_expr_op->get_kind_text()) + " @" + STR(node));
                  }
               }
               else
               {
                  return node;
               }
               break;
            }
            case array_range_ref_K:
            case binfo_K:
            case block_K:
            case call_expr_K:
            case aggr_init_expr_K:
            case case_label_expr_K:
            case complex_cst_K:
            case const_decl_K:
            case constructor_K:
            case field_decl_K:
            case function_decl_K:
            case identifier_node_K:
            case integer_cst_K:
            case label_decl_K:
            case namespace_decl_K:
            case real_cst_K:
            case statement_list_K:
            case target_expr_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case translation_unit_decl_K:
            case template_decl_K:
            case using_decl_K:
            case tree_list_K:
            case tree_vec_K:
            case type_decl_K:
            case vector_cst_K:
            case void_cst_K:
            case error_mark_K:
            case lut_expr_K:
            case CASE_BINARY_EXPRESSION:
            case CASE_CPP_NODES:
            case CASE_FAKE_NODES:
            case CASE_GIMPLE_NODES:
            case CASE_PRAGMA_NODES:
            case CASE_TERNARY_EXPRESSION:
            case CASE_TYPE_NODES:
            case CASE_UNARY_EXPRESSION:
            default:
               THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" +
                           STR(node));
         }
         break;
      }
      case view_convert_expr_K:
      {
         const auto vc = GetPointerS<const view_convert_expr>(node);
         const auto vc_expr_op = vc->op;

         switch(vc_expr_op->get_kind())
         {
            case ssa_name_K:
            {
               const auto sn = GetPointerS<const ssa_name>(vc->op);
               if(!sn->var)
               {
                  return {};
               }
               const auto pd = GetPointer<const parm_decl>(sn->var);
               if(pd)
               {
                  return sn->var;
               }
               else
               {
                  THROW_ERROR("view_convert_expr pattern currently not supported: " + vc->op->get_kind_text() + " @" +
                              STR(node));
               }
               break;
            }
            case var_decl_K:
            {
               return vc->op;
            }
            case integer_cst_K:
            {
               return node;
            }
            case complex_cst_K:
            case real_cst_K:
            case string_cst_K:
            case vector_cst_K:
            case void_cst_K:
            case binfo_K:
            case block_K:
            case call_expr_K:
            case aggr_init_expr_K:
            case case_label_expr_K:
            case const_decl_K:
            case constructor_K:
            case field_decl_K:
            case function_decl_K:
            case identifier_node_K:
            case label_decl_K:
            case namespace_decl_K:
            case parm_decl_K:
            case result_decl_K:
            case statement_list_K:
            case target_expr_K:
            case translation_unit_decl_K:
            case template_decl_K:
            case using_decl_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case tree_list_K:
            case tree_vec_K:
            case type_decl_K:
            case error_mark_K:
            case lut_expr_K:
            case CASE_BINARY_EXPRESSION:
            case CASE_CPP_NODES:
            case CASE_FAKE_NODES:
            case CASE_GIMPLE_NODES:
            case CASE_PRAGMA_NODES:
            case CASE_QUATERNARY_EXPRESSION:
            case CASE_TERNARY_EXPRESSION:
            case CASE_TYPE_NODES:
            case CASE_UNARY_EXPRESSION:
            default:
               THROW_ERROR("view_convert_expr pattern not supported: " + std::string(vc_expr_op->get_kind_text()) +
                           " @" + STR(node));
         }
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case statement_list_K:
      case tree_list_K:
      case tree_vec_K:
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
      case vec_cond_expr_K:
      case vec_perm_expr_K:
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
      case complex_cst_K:
      case real_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case const_decl_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case type_decl_K:
      case array_range_ref_K:
      case target_expr_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case cond_expr_K:
      case dot_prod_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case fshl_expr_K:
      case fshr_expr_K:
      case bit_ior_concat_expr_K:
      case abs_expr_K:
      case alignof_expr_K:
      case arrow_expr_K:
      case bit_not_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case convert_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fix_trunc_expr_K:
      case float_expr_K:
      case loop_expr_K:
      case lut_expr_K:
      case negate_expr_K:
      case non_lvalue_expr_K:
      case nop_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case truth_not_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      case error_mark_K:
      case paren_expr_K:
      case extract_bit_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case insertvalue_expr_K:
      case extractelement_expr_K:
      case insertelement_expr_K:
      case frem_expr_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "tree_helper::GetBaseVariable - variable type is not supported: " +
                                                         STR(node) + "-" + std::string(node->get_kind_text()));
   }
   return {};
}

bool tree_helper::is_fully_resolved(const tree_managerConstRef& TM, const unsigned int index,
                                    CustomOrderedSet<unsigned int>& res_set)
{
   const auto node = TM->GetTreeNode(index);
   return IsPointerResolved(node, res_set);
}

bool tree_helper::IsPointerResolved(const tree_nodeConstRef& node, CustomOrderedSet<unsigned int>& res_set)
{
   THROW_ASSERT(node, "expected positive non zero numbers");
   switch(node->get_kind())
   {
      case ssa_name_K:
      {
         const auto sn = GetPointerS<const ssa_name>(node);
         if(sn->use_set->is_fully_resolved())
         {
            for(const auto& v : sn->use_set->variables)
            {
               res_set.insert(v->index);
            }
            return true;
         }
         else
         {
            const auto base = check_for_simple_pointer_arithmetic(sn->CGetDefStmt(), nullptr);
            if(base)
            {
               return IsPointerResolved(base, res_set);
            }
            else
            {
               return false;
            }
         }
      }
      case result_decl_K:
      case parm_decl_K:
      case var_decl_K:
      case string_cst_K:
      case integer_cst_K:
      {
         return false;
      }
      case indirect_ref_K:
      {
         const auto ir = GetPointerS<const indirect_ref>(node);
         return IsPointerResolved(ir->op, res_set);
      }
      case misaligned_indirect_ref_K:
      {
         const auto mir = GetPointerS<const misaligned_indirect_ref>(node);
         return IsPointerResolved(mir->op, res_set);
      }
      case mem_ref_K:
      {
         const auto mr = GetPointerS<const mem_ref>(node);
         return IsPointerResolved(mr->op0, res_set);
      }
      case array_ref_K:
      {
         const auto ar = GetPointerS<const array_ref>(node);
         return IsPointerResolved(ar->op0, res_set);
      }
      case component_ref_K:
      {
         const auto cr = GetPointerS<const component_ref>(node);
         return IsPointerResolved(cr->op0, res_set);
      }
      case realpart_expr_K:
      {
         const auto rpe = GetPointerS<const realpart_expr>(node);
         return IsPointerResolved(rpe->op, res_set);
      }
      case imagpart_expr_K:
      {
         const auto rpe = GetPointerS<const imagpart_expr>(node);
         return IsPointerResolved(rpe->op, res_set);
      }
      case bit_field_ref_K:
      {
         const auto bfr = GetPointerS<const bit_field_ref>(node);
         return IsPointerResolved(bfr->op0, res_set);
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref>(node);
         if(tmr->symbol)
         {
            return IsPointerResolved(tmr->symbol, res_set);
         }
         else if(tmr->base)
         {
            return IsPointerResolved(tmr->base, res_set);
         }
         else
         {
            return false;
         }
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref461>(node);
         if(tmr->base)
         {
            return IsPointerResolved(tmr->base, res_set);
         }
         else
         {
            return false;
         }
      }
      case addr_expr_K:
      {
         const auto ae = GetPointerS<const addr_expr>(node);
         return IsPointerResolved(ae->op, res_set);
      }
      case view_convert_expr_K:
      {
         const auto vc = GetPointerS<const view_convert_expr>(node);
         return IsPointerResolved(vc->op, res_set);
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case statement_list_K:
      case tree_list_K:
      case tree_vec_K:
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
      case vec_cond_expr_K:
      case vec_perm_expr_K:
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
      case complex_cst_K:
      case real_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case const_decl_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case type_decl_K:
      case array_range_ref_K:
      case target_expr_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case cond_expr_K:
      case dot_prod_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case fshl_expr_K:
      case fshr_expr_K:
      case bit_ior_concat_expr_K:
      case abs_expr_K:
      case alignof_expr_K:
      case arrow_expr_K:
      case bit_not_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case convert_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fix_trunc_expr_K:
      case float_expr_K:
      case loop_expr_K:
      case lut_expr_K:
      case negate_expr_K:
      case non_lvalue_expr_K:
      case nop_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case truth_not_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      case error_mark_K:
      case extract_bit_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case insertvalue_expr_K:
      case extractelement_expr_K:
      case insertelement_expr_K:
      case frem_expr_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      case paren_expr_K:
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                          "tree_helper::IsPointerResolved - variable type is not supported: " + STR(node) + "-" +
                              std::string(node->get_kind_text()));
   }
   return false;
}

bool tree_helper::is_volatile(const tree_managerConstRef& TM, const unsigned int index)
{
   return IsVolatile(TM->GetTreeNode(index));
}

bool tree_helper::IsVolatile(const tree_nodeConstRef& node)
{
   const auto sa = GetPointer<const ssa_name>(node);
   if(!sa)
   {
      // variable or indirect ref
      const auto n = CGetType(node);
      const auto tn = GetPointerS<const type_node>(n);
      return tn->qual == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_V ||
             tn->qual == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_VR ||
             tn->qual == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CVR;
   }
   return sa->volatile_flag;
}

bool tree_helper::is_parameter(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->GetTreeNode(index);
   if(GetPointer<parm_decl>(node))
   {
      return true;
   }
   const auto sn = GetPointer<ssa_name>(node);
   if(!sn)
   {
      return false;
   }
   return sn->CGetDefStmt()->get_kind() == gimple_nop_K && sn->var && sn->var->get_kind() == parm_decl_K;
}

bool tree_helper::is_ssa_name(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->GetTreeNode(index);
   const auto sn = GetPointer<ssa_name>(node);
   return sn != nullptr;
}

bool tree_helper::is_virtual(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->GetTreeNode(index);
   const auto sn = GetPointer<ssa_name>(node);
   if(!sn)
   {
      return false;
   }
   return sn->virtual_flag;
}

bool tree_helper::is_static(const tree_managerConstRef& TM, const unsigned int index)
{
   return IsStaticDeclaration(TM->GetTreeNode(index));
}

bool tree_helper::IsStaticDeclaration(const tree_nodeConstRef& decl)
{
   const auto vd = GetPointer<const var_decl>(decl);
   if(!vd)
   {
      const auto fd = GetPointer<const function_decl>(decl);
      if(!fd)
      {
         return false;
      }
      else
      {
         return fd->static_flag;
      }
   }
   return vd->static_flag;
}

bool tree_helper::is_extern(const tree_managerConstRef& TM, const unsigned int index)
{
   return IsExternDeclaration(TM->GetTreeNode(index));
}

bool tree_helper::IsExternDeclaration(const tree_nodeConstRef& decl)
{
   const auto vd = GetPointer<const var_decl>(decl);
   if(!vd)
   {
      const auto fd = GetPointer<const function_decl>(decl);
      if(!fd)
      {
         return false;
      }
      else
      {
         return fd->undefined_flag;
      }
   }
   return vd->extern_flag;
}

bool tree_helper::is_const_type(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto T = TM->GetTreeNode(index);
   return IsConstType(T);
}

bool tree_helper::IsConstType(const tree_nodeConstRef& type)
{
   const auto Type = CGetType(type);
   THROW_ASSERT(Type, "expected a type");
   const auto quals = GetPointer<const type_node>(Type)->qual;
   return quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN &&
          (quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_C ||
           quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CV ||
           quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CVR);
}

std::string tree_helper::return_qualifier_prefix(const TreeVocabularyTokenTypes_TokenEnum quals)
{
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_R)
   {
      return "r_";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_V)
   {
      return "v_";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_VR)
   {
      return "vr_";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_C)
   {
      return "c_";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CR)
   {
      return "cr_";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CV)
   {
      return "cv_";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CVR)
   {
      return "cvr_";
   }
   THROW_UNREACHABLE("not supported qualifier " + STR(static_cast<unsigned int>(quals)));
   return "";
}

std::string tree_helper::return_C_qualifiers(const TreeVocabularyTokenTypes_TokenEnum quals, bool real_const)
{
   if(quals == TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
   {
      return "";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_R)
   {
      return "__restrict__ ";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_V)
   {
      return "volatile ";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_VR)
   {
      return "volatile __restrict__ ";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_C)
   {
      return real_const ? "const " : "/*const*/ ";
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CR)
   {
      return (real_const ? "const" : "/*const*/") + std::string(" __restrict__ ");
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CV)
   {
      return (real_const ? "const" : "/*const*/") + std::string(" volatile ");
   }
   else if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CVR)
   {
      return (real_const ? "const" : "/*const*/") + std::string(" volatile __restrict__ ");
   }
   THROW_UNREACHABLE("not supported qualifier " + STR(static_cast<unsigned int>(quals)));
   return "";
}

integer_cst_t tree_helper::get_integer_cst_value(const integer_cst* ic)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Getting integer const value");
   THROW_ASSERT(ic != nullptr, "unexpected condition");
   THROW_ASSERT(ic->type, "Something wrong");
   const auto type = ic->type;
   THROW_ASSERT(GetPointer<integer_type>(type) || type->get_kind() == pointer_type_K ||
                    type->get_kind() == reference_type_K || type->get_kind() == boolean_type_K ||
                    type->get_kind() == enumeral_type_K,
                "Expected a integer_type, pointer_type, reference_type, boolean_type, enumeral_type. Found: " +
                    STR(ic->type->index) + " " + type->get_kind_text());
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Constant is " + STR(ic->value));
   return ic->value;
}

integer_cst_t tree_helper::GetConstValue(const tree_nodeConstRef& tn, bool is_signed)
{
   THROW_ASSERT(tn, "unexpected condition");
   THROW_ASSERT(tn->get_kind() == integer_cst_K, "unexpected condition");
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Getting integer const value");
   const auto ic = GetPointerS<const integer_cst>(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Constant is " + STR(ic->value));
   if(!is_signed)
   {
      const auto bitwidth = Size(ic->type);
      return ic->value & ((integer_cst_t(1) << bitwidth) - 1);
   }
   return ic->value;
}

unsigned int tree_helper::get_array_var(const tree_managerConstRef& TM, const unsigned int index, bool is_written,
                                        bool& two_dim_p)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->GetTreeNode(index);
   const auto gms = GetPointer<gimple_assign>(node);
   array_ref* ar;
   if(is_written)
   {
      ar = GetPointer<array_ref>(gms->op0);
   }
   else
   {
      ar = GetPointer<array_ref>(gms->op1);
   }
   two_dim_p = false;
   if(ar)
   {
      if(GetPointer<array_ref>(ar->op0))
      {
         ar = GetPointer<array_ref>(ar->op0);
         if(GetPointer<array_ref>(ar->op0))
         {
            THROW_ERROR("n-dimension array not yet supported (n > 2)");
         }
         two_dim_p = true;
         return ar->op0->index;
      }
      else
      {
         return ar->op0->index;
      }
   }
   target_mem_ref* tmr;
   if(is_written)
   {
      tmr = GetPointer<target_mem_ref>(gms->op0);
   }
   else
   {
      tmr = GetPointer<target_mem_ref>(gms->op1);
   }
   if(tmr)
   {
      if(tmr->symbol)
      {
         return tmr->symbol->index;
      }
      else if(tmr->base)
      {
         return tmr->base->index;
      }
      else
      {
         THROW_ERROR("Unexpected pattern");
      }
   }
   target_mem_ref461* tmr461;
   if(is_written)
   {
      tmr461 = GetPointer<target_mem_ref461>(gms->op0);
   }
   else
   {
      tmr461 = GetPointer<target_mem_ref461>(gms->op1);
   }
   if(tmr461)
   {
      if(tmr461->base)
      {
         return tmr461->base->index;
      }
      else
      {
         THROW_ERROR("Unexpected pattern");
      }
   }
   auto ae = GetPointer<addr_expr>(gms->op1);
   if(ae)
   {
      ar = GetPointer<array_ref>(ae->op);
      if(ar)
      {
         if(GetPointer<array_ref>(ar->op0))
         {
            ar = GetPointer<array_ref>(ar->op0);
            if(GetPointer<array_ref>(ar->op0))
            {
               THROW_ERROR("n-dimension array not yet supported (n > 2)");
            }
            two_dim_p = true;
            return ar->op0->index;
         }
         else
         {
            return ar->op0->index;
         }
      }
      else
      {
         THROW_ERROR("Unexpected pattern " + STR(index));
         return 0;
      }
   }
   const auto ne = GetPointer<nop_expr>(gms->op1);
   if(ne)
   {
      ae = GetPointer<addr_expr>(ne->op);
      if(ae)
      {
         ar = GetPointer<array_ref>(ae->op);
         if(ar)
         {
            if(GetPointer<array_ref>(ar->op0))
            {
               ar = GetPointer<array_ref>(ar->op0);
               if(GetPointer<array_ref>(ar->op0))
               {
                  THROW_ERROR("n-dimension array not yet supported (n > 2)");
               }
               two_dim_p = true;
               return ar->op0->index;
            }
            else
            {
               return ar->op0->index;
            }
         }
         const auto vd = GetPointer<var_decl>(ae->op);
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

bool tree_helper::is_concat_bit_ior_expr(const tree_managerConstRef& TM, const unsigned int index)
{
   tree_nodeRef node = TM->GetTreeNode(index);
   const auto ga = GetPointer<gimple_assign>(node);
   if(ga)
   {
      const auto bie = GetPointer<bit_ior_expr>(ga->op1);
      if(bie)
      {
         tree_nodeRef op0 = bie->op0;
         tree_nodeRef op1 = bie->op1;
         if(op0->get_kind() == ssa_name_K && op1->get_kind() == ssa_name_K)
         {
            const auto op0_ssa = GetPointer<ssa_name>(op0);
            const auto op1_ssa = GetPointer<ssa_name>(op1);
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

bool tree_helper::is_simple_pointer_plus_test(const tree_managerConstRef& TM, const unsigned int index)
{
   tree_nodeRef node = TM->GetTreeNode(index);
   const auto ga = GetPointer<gimple_assign>(node);
   if(ga)
   {
      const auto ppe = GetPointer<pointer_plus_expr>(ga->op1);
      if(ppe)
      {
         tree_nodeRef op1 = ppe->op1;
         if(op1->get_kind() == integer_cst_K)
         {
            tree_nodeRef op0 = ppe->op0;
            if(op0->get_kind() == addr_expr_K)
            {
               return true;
            }
            else if(op0->get_kind() == ssa_name_K)
            {
               auto temp_def = GetPointer<const ssa_name>(op0)->CGetDefStmt();
               const auto ga_def = GetPointer<gimple_assign>(temp_def);
               if(ga_def)
               {
                  if(ga_def->op1->get_kind() == addr_expr_K)
                  {
                     return true;
                  }
               }
            }
         }
      }
   }
   return false;
}

bool tree_helper::is_constant(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto node = TM->GetTreeNode(index);
   return IsConstant(node);
}

bool tree_helper::IsConstant(const tree_nodeConstRef& node)
{
   switch(node->get_kind())
   {
      case CASE_CST_NODES:
         return true;
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      default:
         return false;
   }
   return false;
}

std::string tree_helper::op_symbol(const tree_nodeConstRef& op)
{
   return op_symbol(op.get());
}

std::string tree_helper::op_symbol(const tree_node* op)
{
   THROW_ASSERT(op, "Null tree node");
   switch(op->get_kind())
   {
      case modify_expr_K:
         return "=";

      case truth_or_expr_K:
      case truth_orif_expr_K:
         return "||";

      case truth_and_expr_K:
      case truth_andif_expr_K:
         return "&&";

      case bit_ior_expr_K:
         return "|";

      case truth_xor_expr_K:
      case bit_xor_expr_K:
         return "^";

      case addr_expr_K:
      case bit_and_expr_K:
      {
         if(dynamic_cast<const addr_expr*>(op))
         {
            const auto ae = static_cast<const addr_expr*>(op);
            const auto tn = ae->op;
            if(GetPointer<const array_ref>(tn))
            {
               const auto ar = GetPointerS<const array_ref>(tn);
               if(ar->op0->get_kind() == string_cst_K && ar->op1->get_kind() == integer_cst_K &&
                  !GetConstValue(ar->op1))
               {
                  return "";
               }
            }
            if(GetPointer<const string_cst>(tn))
            {
               return "";
            }
         }
         return "&";
      }

      case eq_expr_K:
         return "==";

      case ne_expr_K:
         return "!=";

      case lt_expr_K:
      case lut_expr_K:
      case unlt_expr_K:
         return "<";

      case le_expr_K:
      case unle_expr_K:
         return "<=";

      case gt_expr_K:
      case ungt_expr_K:
         return ">";

      case ge_expr_K:
      case unge_expr_K:
         return ">=";

      case vec_lshift_expr_K:
      case lshift_expr_K:
         return "<<";

      case vec_rshift_expr_K:
      case rshift_expr_K:
         return ">>";

      case plus_expr_K:
      case pointer_plus_expr_K:
      case widen_sum_expr_K:
         return "+";

      case negate_expr_K:
      case minus_expr_K:
         return "-";

      case bit_not_expr_K:
         return "~";

      case truth_not_expr_K:
         return "!";

      case mult_expr_K:
      case mult_highpart_expr_K:
      case indirect_ref_K:
      case misaligned_indirect_ref_K:
      case widen_mult_expr_K:
         return "*";

      case trunc_div_expr_K:
      case ceil_div_expr_K:
      case floor_div_expr_K:
      case round_div_expr_K:
      case rdiv_expr_K:
      case exact_div_expr_K:
         return "/";

      case trunc_mod_expr_K:
      case ceil_mod_expr_K:
      case floor_mod_expr_K:
      case round_mod_expr_K:
         return "%";

      case predecrement_expr_K:
         return " --";

      case preincrement_expr_K:
         return " ++";

      case postdecrement_expr_K:
         return "-- ";

      case postincrement_expr_K:
         return "+=";

      case reference_expr_K:
         return "";
      case realpart_expr_K:
         return "__real__ ";
      case imagpart_expr_K:
         return "__imag__ ";
      case fix_trunc_expr_K:
      case min_expr_K:
         return "";
      case abs_expr_K:
      case alignof_expr_K:
      case arrow_expr_K:
      case assert_expr_K:
      case binfo_K:
      case block_K:
      case buffer_ref_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case card_expr_K:
      case case_label_expr_K:
      case catch_expr_K:
      case cleanup_point_expr_K:
      case complex_expr_K:
      case compound_expr_K:
      case conj_expr_K:
      case constructor_K:
      case convert_expr_K:
      case eh_filter_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fdesc_expr_K:
      case float_expr_K:
      case goto_subroutine_K:
      case identifier_node_K:
      case in_expr_K:
      case init_expr_K:
      case loop_expr_K:
      case ltgt_expr_K:
      case lrotate_expr_K:
      case max_expr_K:
      case mem_ref_K:
      case non_lvalue_expr_K:
      case nop_expr_K:
      case ordered_expr_K:
      case range_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case reinterpret_cast_expr_K:
      case rrotate_expr_K:
      case set_le_expr_K:
      case sizeof_expr_K:
      case ssa_name_K:
      case static_cast_expr_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case throw_expr_K:
      case tree_list_K:
      case tree_vec_K:
      case try_catch_expr_K:
      case try_finally_K:
      case uneq_expr_K:
      case unordered_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case vec_extracteven_expr_K:
      case vec_extractodd_expr_K:
      case vec_interleavehigh_expr_K:
      case vec_interleavelow_expr_K:
      case vec_pack_fix_trunc_expr_K:
      case vec_pack_sat_expr_K:
      case vec_pack_trunc_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case view_convert_expr_K:
      case widen_mult_hi_expr_K:
      case widen_mult_lo_expr_K:
      case with_size_expr_K:
      case error_mark_K:
      case paren_expr_K:
      case extract_bit_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      case frem_expr_K:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR(std::string("op_symbol not yet supported ") + op->get_kind_text() + " in node " + STR(op->index));
         return "";
   }
}

unsigned long long tree_helper::get_array_data_bitsize(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto node = TM->GetTreeNode(index);
   return GetArrayElementSize(node);
}

unsigned long long tree_helper::GetArrayElementSize(const tree_nodeConstRef& node)
{
   if(node->get_kind() == record_type_K)
   {
      const auto rt = GetPointerS<const record_type>(node);
      auto fd = rt->list_of_flds[0];
      THROW_ASSERT(fd->get_kind() == field_decl_K, "expected a field_decl");
      return GetArrayElementSize(GetPointerS<const field_decl>(fd)->type);
   }
   if(node->get_kind() == union_type_K)
   {
      const auto ut = GetPointerS<const union_type>(node);
      const auto fd = ut->list_of_flds[0];
      THROW_ASSERT(fd->get_kind() == field_decl_K, "expected a field_decl");
      return GetArrayElementSize(GetPointerS<const field_decl>(fd)->type);
   }
   if(node->get_kind() != array_type_K)
   {
      return SizeAlloc(node);
   }
   const auto at = GetPointerS<const array_type>(node);
   THROW_ASSERT(at->elts, "elements type expected");
   const auto elts = at->elts;
   unsigned long long return_value;
   if(elts->get_kind() == array_type_K)
   {
      return_value = GetArrayElementSize(at->elts);
   }
   else
   {
      const auto type = CGetType(at->elts);
      return_value = SizeAlloc(type);
      const auto fd = GetPointer<const field_decl>(type);
      if(!fd || !fd->is_bitfield())
      {
         return_value = std::max(8ull, return_value);
      }
   }
   return return_value;
}

void tree_helper::get_array_dim_and_bitsize(const tree_managerConstRef& TM, const unsigned int index,
                                            std::vector<unsigned long long>& dims, unsigned long long& elts_bitsize)
{
   tree_nodeRef node = TM->GetTreeNode(index);
   if(node->get_kind() == record_type_K || node->get_kind() == union_type_K)
   {
      elts_bitsize = get_array_data_bitsize(TM, index);
      dims.push_back(SizeAlloc(node) / elts_bitsize);
      return;
   }
   THROW_ASSERT(node->get_kind() == array_type_K, "array_type expected: @" + STR(index));
   const auto at = GetPointer<array_type>(node);
   if(!at->domn)
   {
      dims.push_back(1); // at least one element is expected
   }
   else
   {
      tree_nodeRef domn = at->domn;
      THROW_ASSERT(domn->get_kind() == integer_type_K, "expected an integer type as domain");
      const auto it = GetPointer<integer_type>(domn);
      integer_cst_t min_value = 0;
      integer_cst_t max_value = 0;
      if(it->min)
      {
         min_value = GetConstValue(it->min);
      }
      if(it->max)
      {
         max_value = GetConstValue(it->max);
      }
      const auto range_domain = max_value - min_value + 1;
      THROW_ASSERT(range_domain >= 0, "Negative range not expected");
      dims.push_back(static_cast<unsigned long long>(range_domain));
   }
   THROW_ASSERT(at->elts, "elements type expected");
   tree_nodeRef elts = at->elts;
   if(elts->get_kind() == array_type_K)
   {
      get_array_dim_and_bitsize(TM, at->elts->index, dims, elts_bitsize);
   }
   else
   {
      const auto etype = CGetType(at->elts);
      elts_bitsize = SizeAlloc(etype);
      const auto fd = GetPointer<const field_decl>(etype);
      if(!fd || !fd->is_bitfield())
      {
         elts_bitsize = std::max(8ull, elts_bitsize);
      }
   }
}

void tree_helper::get_array_dimensions(const tree_managerConstRef& TM, const unsigned int index,
                                       std::vector<unsigned long long>& dims)
{
   const auto node = TM->GetTreeNode(index);
   dims = GetArrayDimensions(node);
}

std::vector<unsigned long long> tree_helper::GetArrayDimensions(const tree_nodeConstRef& node)
{
   std::vector<unsigned long long> dims;
   std::function<void(const tree_nodeConstRef&)> get_array_dim_recurse;
   get_array_dim_recurse = [&](const tree_nodeConstRef& tn) -> void {
      if(tn->get_kind() == record_type_K || tn->get_kind() == union_type_K)
      {
         auto elmt_bitsize = GetArrayElementSize(tn);
         dims.push_back(SizeAlloc(tn) / elmt_bitsize);
         return;
      }
      THROW_ASSERT(tn->get_kind() == array_type_K, "array_type expected: @" + STR(tn));
      const auto at = GetPointerS<const array_type>(tn);
      const auto domn = at->domn;
      THROW_ASSERT(domn->get_kind() == integer_type_K, "expected an integer type as domain");
      const auto it = GetPointerS<const integer_type>(domn);
      integer_cst_t min_value = 0;
      integer_cst_t max_value = 0;
      if(it->min && it->min->get_kind() == integer_cst_K && it->max && it->max->get_kind() == integer_cst_K)
      {
         if(it->min)
         {
            min_value = GetConstValue(it->min);
         }
         if(it->max)
         {
            max_value = GetConstValue(it->max);
         }
         THROW_ASSERT(max_value >= min_value, "");
         const auto range_domain = static_cast<unsigned long long>(max_value - min_value) + 1;
         dims.push_back(range_domain);
      }
      else
      {
         dims.push_back(0); // variable size array may fall in this case
      }
      THROW_ASSERT(at->elts, "elements type expected");
      const auto elts = at->elts;
      if(elts->get_kind() == array_type_K)
      {
         get_array_dim_recurse(at->elts);
      }
   };
   get_array_dim_recurse(node);
   return dims;
}

unsigned long long tree_helper::get_array_num_elements(const tree_managerConstRef& TM, const unsigned int index)
{
   const auto node = TM->GetTreeNode(index);
   return GetArrayTotalSize(node);
}

unsigned long long tree_helper::GetArrayTotalSize(const tree_nodeConstRef& node)
{
   auto num_elements = 1ull;
   for(const auto& i : GetArrayDimensions(node))
   {
      num_elements *= i;
   }
   return num_elements;
}

void tree_helper::extract_array_indexes(const tree_managerConstRef& TM, const unsigned int index,
                                        std::vector<unsigned long long>& indexes,
                                        std::vector<unsigned long long>& size_indexes, unsigned int& base_object)
{
   const auto node = TM->GetTreeNode(index);
   THROW_ASSERT(node->get_kind() == array_ref_K, "array_ref expected: @" + STR(index));
   const auto ar = GetPointerS<const array_ref>(node);
   base_object = ar->op0->index;
   if(ar->op0->get_kind() == array_ref_K)
   {
      const auto nested_ar = GetPointerS<const array_ref>(ar->op0);
      const auto at = GetPointerS<const array_type>(nested_ar->type);
      const auto domn = at->domn;
      THROW_ASSERT(domn->get_kind() == integer_type_K, "expected an integer type as domain");
      const auto it = GetPointerS<const integer_type>(domn);
      integer_cst_t min_value = 0;
      integer_cst_t max_value = 0;
      if(it->min)
      {
         min_value = GetConstValue(it->min);
      }
      if(it->max)
      {
         max_value = GetConstValue(it->max);
      }
      THROW_ASSERT(max_value >= min_value, "");
      const auto range_domain = static_cast<unsigned long long>(max_value - min_value) + 1;
      size_indexes.push_back(range_domain);
      extract_array_indexes(TM, base_object, indexes, size_indexes, base_object);
      indexes.push_back(nested_ar->op1->index);
   }
}

unsigned int tree_helper::GetUnqualified(const tree_managerConstRef& TM, unsigned int type)
{
   const auto utype = GetUnqualifiedType(TM->GetTreeNode(type));
   return utype ? utype->index : 0;
}

tree_nodeConstRef tree_helper::GetUnqualifiedType(const tree_nodeConstRef& type)
{
   if(GetPointer<const type_node>(type) && GetPointerS<const type_node>(type)->unql)
   {
      return GetPointerS<const type_node>(type)->unql;
   }
   return nullptr;
}

bool tree_helper::IsAligned(const tree_managerConstRef& TM, unsigned int type)
{
   return IsAligned(TM->GetTreeNode(type));
}

bool tree_helper::IsAligned(const tree_nodeConstRef& type)
{
   const auto tn = GetPointer<const type_node>(type);
   THROW_ASSERT(tn, "Tree node " + STR(type) + " is of type " + type->get_kind_text());
   return tn->unql && tn->algn != GetPointerS<const type_node>(tn->unql)->algn;
}

unsigned int tree_helper::get_var_alignment(const tree_managerConstRef& TM, unsigned int var)
{
   const auto varnode = TM->GetTreeNode(var);
   const auto vd = GetPointer<const var_decl>(varnode);
   if(vd)
   {
      return vd->algn < 8 ? 1 : (vd->algn / 8);
   }
   return 1;
}

std::string tree_helper::NormalizeTypename(const std::string& id)
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

std::string tree_helper::print_type(const tree_managerConstRef& TM, unsigned int original_type, bool global,
                                    bool print_qualifiers, bool print_storage, unsigned int var,
                                    const var_pp_functorConstRef& vppf, const std::string& prefix,
                                    const std::string& tail)
{
   const auto t = TM->GetTreeNode(original_type);
   return PrintType(TM, t, global, print_qualifiers, print_storage, var ? TM->GetTreeNode(var) : tree_nodeConstRef(),
                    vppf, prefix, tail);
}

std::string tree_helper::PrintType(const tree_managerConstRef& TM, const tree_nodeConstRef& original_type, bool global,
                                   bool print_qualifiers, bool print_storage, const tree_nodeConstRef& var,
                                   const var_pp_functorConstRef& vppf, const std::string& prefix,
                                   const std::string& tail)
{
   bool skip_var_printing = false;
   const auto node_type = GetRealType(original_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Printing type " + STR(original_type) + "(" + STR(node_type) + ") - Var " + STR(var));
   std::string res;
   if(var)
   {
      if(var->get_kind() == var_decl_K)
      {
         const auto vd = GetPointerS<const var_decl>(var);
         if(print_storage)
         {
            if(vd->extern_flag)
            {
               res += "extern ";
            }
            if(vd->static_flag || vd->static_static_flag)
            {
               res += "static ";
            }
         }
      }
   }
   switch(node_type->get_kind())
   {
      case function_decl_K:
      {
         const auto fd = GetPointerS<const function_decl>(node_type);
         const auto function_name = print_function_name(TM, fd);
         if(fd->undefined_flag)
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
         tree_nodeRef ftype = dn->type;
         /* Print type declaration.  */
         if(ftype->get_kind() == function_type_K || ftype->get_kind() == method_type_K)
         {
            const auto ft = GetPointerS<const function_type>(ftype);
            res += PrintType(TM, ft->retn, global);
         }
         else
         {
            THROW_ERROR(std::string("tree node not currently supported ") + node_type->get_kind_text());
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
               res += PrintType(TM, CGetType(fd->list_of_args[i]), global, print_qualifiers, print_storage,
                                fd->list_of_args[i], vppf);
            }
         }
         else if(TM->get_implementation_node(node_type->index) &&
                 TM->get_implementation_node(node_type->index) != node_type->index)
         {
            skip_var_printing = true;
            const auto type = TM->GetTreeNode(TM->get_implementation_node(node_type->index));
            res = PrintType(TM, type, global, print_qualifiers, print_storage, nullptr, vppf);
            break;
         }
         else if(GetPointer<const function_type>(ftype)->prms)
         {
            if(ftype->get_kind() == function_type_K)
            {
               const auto ft = GetPointerS<const function_type>(ftype);
               res += PrintType(TM, ft->prms, global);
            }
            else if(ftype->get_kind() == method_type_K)
            {
               const auto mt = GetPointerS<const method_type>(ftype);
               res += PrintType(TM, mt->prms, global);
            }
            else
            {
               THROW_ERROR(std::string("tree node not currently supported ") + node_type->get_kind_text());
            }
         }
         if((!fd->list_of_args.empty() || GetPointerS<const function_type>(ftype)->prms) &&
            GetPointerS<const function_type>(ftype)->varargs_flag)
         {
            res += ", ... ";
         }
         res += ")";
         break;
      }
      case type_decl_K:
      {
         const auto td = GetPointerS<const type_decl>(node_type);
         if(td->name)
         {
            const auto name = td->name;
            if(name->get_kind() == identifier_node_K)
            {
               const auto in = GetPointerS<const identifier_node>(name);
               /// patch for unsigned char
               std::string typename_value = NormalizeTypename(in->strg);
               if(typename_value == "char" && GetPointer<const integer_type>(td->type) &&
                  GetPointer<const integer_type>(td->type)->unsigned_flag)
               {
                  res += "unsigned " + typename_value;
                  /// patch for va_list
               }
               else if(typename_value == "__va_list_tag")
               {
                  res += "va_list";
               }
               else if(td->type->get_kind() == complex_type_K)
               {
                  const auto splitted = string_to_container<std::vector<std::string>>(typename_value, " ");
                  if((splitted[0] == "_Complex" || splitted[0] == "__complex__" || splitted[0] == "complex"))
                  {
                     res += "__complex__";
                     for(unsigned int ci = 1u; ci < splitted.size(); ci++)
                     {
                        res += " " + splitted[ci];
                     }
                  }
                  else
                  {
                     res += typename_value;
                  }
               }
               else
               {
                  res += typename_value;
               }
            }
            else
            {
               THROW_ERROR(std::string("Node not yet supported: ") + node_type->get_kind_text());
            }
         }
         else
         {
            THROW_ERROR(std::string("Node not yet supported: ") + node_type->get_kind_text());
         }
         skip_var_printing = true;
         break;
      }
      case identifier_node_K:
      {
         const auto in = GetPointerS<const identifier_node>(node_type);
         res += NormalizeTypename(in->strg);
         skip_var_printing = true;
         break;
      }
      case void_type_K:
      case integer_type_K:
      case real_type_K:
      case complex_type_K:
      case vector_type_K:
      case boolean_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      {
         const auto tn = GetPointerS<const type_node>(node_type);
         /* const internally are not considered as constant...*/
         if(var && !tn->name)
         {
            const auto is_global_var = GetPointer<const var_decl>(var) &&
                                       (!(GetPointer<const var_decl>(var)->scpe) ||
                                        (GetPointer<const var_decl>(var)->scpe->get_kind() == translation_unit_decl_K));
            res += return_C_qualifiers(tn->qual, is_global_var);
         }
         else if(global || print_qualifiers)
         {
            res += return_C_qualifiers(tn->qual, true);
         }
         if(tn->name && (tn->name->get_kind() != type_decl_K || !tn->system_flag))
         {
            const auto name = tn->name;
            if(name->get_kind() == identifier_node_K)
            {
               const auto in = GetPointerS<const identifier_node>(name);
               res += NormalizeTypename(in->strg);
            }
            else if(name->get_kind() == type_decl_K)
            {
               res += PrintType(TM, tn->name, global);
            }
            else
            {
               THROW_ERROR(std::string("Node not yet supported: ") +
                           node_type->get_kind_text()); // p_string(buffer, get_unnamed(node));
            }
         }
         else if(node_type->get_kind() == complex_type_K)
         {
            const auto ct = GetPointerS<const complex_type>(node_type);
            if(ct->unql)
            {
               res += PrintType(TM, ct->unql, global);
            }
            else
            {
               res += "_Complex ";
               if(GetPointer<const complex_type>(node_type)->unsigned_flag)
               {
                  res += "unsigned ";
               }
               if(GetPointer<const complex_type>(node_type)->real_flag)
               {
                  if(tn->algn == 32)
                  {
                     res += "float";
                  }
                  else if(tn->algn == 64)
                  {
                     res += "double";
                  }
                  else if(tn->algn == 80)
                  {
                     res += "__float80";
                  }
                  else if(tn->algn == 128)
                  {
                     res += "__float128";
                  }
                  else
                  {
                     THROW_ERROR(std::string("Complex Real type not yet supported ") + STR(original_type));
                  }
               }
               else
               {
                  if(tn->algn == 8)
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
                  else
                  {
                     THROW_ERROR(std::string("Node not yet supported: ") + node_type->get_kind_text() + STR(var));
                  }
               }
            }
         }
         else if(node_type->get_kind() == vector_type_K)
         {
            const auto vt = GetPointerS<const vector_type>(node_type);
            if(vt->unql)
            {
               res += PrintType(TM, vt->unql, global);
            }
            else
            {
               /// Ad hoc management of vector of bool
               if(vt->elts->get_kind() == boolean_type_K)
               {
                  res += "int __attribute__((vector_size(" + STR(SizeAlloc(node_type) * 4) + ")))";
               }
               else
               {
                  // THROW_ERROR(std::string("Node not yet supported:<unnamed type> ") +
                  // node_type->get_kind_text()+STR(type));
                  THROW_ASSERT(vt->elts, "expected the type of the elements of the vector");
                  res += PrintType(TM, vt->elts, global);
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
            }
         }
         else
         {
            switch(node_type->get_kind())
            {
               case integer_type_K:
               {
                  const auto it = GetPointerS<const integer_type>(node_type);
                  if(it->unsigned_flag)
                  {
                     res += "unsigned ";
                  }
                  if(it->prec != tn->algn)
                  {
                     if(it->prec > 64)
                     {
                        res += "int __attribute__((vector_size(16)))";
                     }
                     else if(it->prec > 32)
                     {
                        res += "long long int";
                     }
                     else if(it->prec > 16)
                     {
                        res += "int";
                     }
                     else if(it->prec > 8)
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
               case enumeral_type_K:
                  break;
               case void_type_K:
                  res += "void";
                  break;
               case boolean_type_K:
                  res += "_Bool";
                  break;
               case real_type_K:
               {
                  const auto rt = GetPointerS<const real_type>(node_type);
                  if(rt->prec == 32)
                  {
                     res += "float";
                  }
                  else if(rt->prec == 64)
                  {
                     res += "double";
                  }
                  else if(rt->prec == 80)
                  {
                     res += "__float80";
                  }
                  else if(rt->prec == 128)
                  {
                     res += "__float128";
                  }
                  else
                  {
                     THROW_ERROR(std::string("Real type not yet supported ") + STR(original_type));
                  }
                  break;
               }
               case type_pack_expansion_K:
               {
                  res += "<type_pack_expansion>";
                  break;
               }
               case array_type_K:
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case CharType_K:
               case nullptr_type_K:
               case complex_type_K:
               case constructor_K:
               case function_type_K:
               case identifier_node_K:
               case lang_type_K:
               case method_type_K:
               case offset_type_K:
               case pointer_type_K:
               case qual_union_type_K:
               case record_type_K:
               case reference_type_K:
               case set_type_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case template_type_parm_K:
               case type_argument_pack_K:
               case tree_list_K:
               case tree_vec_K:
               case typename_type_K:
               case union_type_K:
               case vector_type_K:
               case error_mark_K:
               case lut_expr_K:
               case CASE_BINARY_EXPRESSION:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_GIMPLE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_UNARY_EXPRESSION:
               default:
                  THROW_ERROR(std::string("Node not yet supported ") + node_type->get_kind_text() + " " +
                              STR(original_type));
            }
         }
         break;
      }
      case pointer_type_K:
      case reference_type_K:
      {
         if(node_type->get_kind() == pointer_type_K)
         {
            const auto tree_type = GetPointerS<const pointer_type>(node_type);
            if(tree_type->name && tree_type->name->get_kind() == type_decl_K)
            {
               const auto td = GetPointerS<const type_decl>(tree_type->name);
               if(td->name && td->name->get_kind() == identifier_node_K)
               {
                  const auto id = GetPointerS<const identifier_node>(td->name);
                  if(id->strg == "va_list")
                  {
                     res = "va_list";
                     break;
                  }
               }
            }
            const auto rt = GetPointerS<const record_type>(tree_type->ptd);
            if(tree_type->ptd && tree_type->ptd->get_kind() == record_type_K && rt->name &&
               rt->name->get_kind() == type_decl_K)
            {
               const auto td = GetPointerS<const type_decl>(rt->name);
               if(td->name && td->name->get_kind() == identifier_node_K)
               {
                  const auto id = GetPointerS<const identifier_node>(td->name);
                  if(id->strg == "va_list" || id->strg == "__va_list_tag")
                  {
                     res = "va_list";
                     break;
                  }
               }
            }
            res = "*";
            /* const internally are not considered as constant...*/
            res += return_C_qualifiers(tree_type->qual, var && print_qualifiers);
            res = PrintType(TM, tree_type->ptd, global, print_qualifiers, print_storage, var, vppf, prefix + res, tail);
            skip_var_printing = true;
            break;
         }
         else
         {
            res = "/*&*/*"; /// references are translated as pointer type objects
            const auto tree_type = GetPointerS<const reference_type>(node_type);
            res =
                PrintType(TM, tree_type->refd, global, print_qualifiers, print_storage, var, vppf, prefix + res, tail);
            skip_var_printing = true;
            break;
         }
         break;
      }
      case function_type_K:
      {
         const auto ft = GetPointerS<const function_type>(node_type);
         res += PrintType(TM, ft->retn, global, true);
         res += "(" + prefix;
         if(var)
         {
            THROW_ASSERT(vppf, "expected a functor");
            res += " " + (*vppf)(var->index);
         }
         res += tail + ")(";
         if(ft->prms)
         {
            res += PrintType(TM, ft->prms, global, true);
         }
         if(ft->varargs_flag && ft->prms)
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
      case method_type_K:
      {
         const auto mt = GetPointerS<const method_type>(node_type);
         res += PrintType(TM, mt->clas, global);
         res += "::";
         res += PrintType(TM, mt->retn, global);
         res += "(" + prefix;
         if(var)
         {
            THROW_ASSERT(vppf, "expected a functor");
            res += " " + (*vppf)(var->index);
         }
         res += tail + ")(";
         if(mt->prms)
         {
            res += PrintType(TM, mt->prms, global);
         }
         res += ")";
         skip_var_printing = true;
         break;
      }
      case array_type_K:
      {
         const auto at = GetPointerS<const array_type>(node_type);
         if(at->name)
         {
            res += PrintType(TM, at->name);
         }
         else
         {
            std::string local_prefix;
            std::string local_tail;
            /* Print array's type */
            /// Compute the dimensions
            if(at->size)
            {
               const auto array_length = at->size;
               if(array_length->get_kind() == integer_cst_K)
               {
                  const auto tn = GetPointerS<const type_node>(at->elts);
                  local_tail += "[";
                  local_tail += STR(GetConstValue(at->size) / GetConstValue(tn->size));
                  local_tail += "]";
               }
               else if(array_length->get_kind() == var_decl_K)
               {
                  local_tail += "[";
                  local_tail += "]";
               }
               else
               {
                  THROW_ERROR("array print_type not supported " + STR(at->size->index));
               }
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

            res += PrintType(TM, at->elts, global, print_qualifiers, print_storage, nullptr, nullptr, "",
                             local_prefix + tail + local_tail);
            /// add alignment
            if(var && var->get_kind() == field_decl_K)
            {
               unsigned int type_align = at->algn;
               unsigned int var_align;
               bool is_a_pointerP = false;
               bool is_static = false;
               switch(var->get_kind())
               {
                  case field_decl_K:
                  {
                     const auto fd = GetPointerS<const field_decl>(var);
                     var_align = fd->algn;
                     is_a_pointerP = fd->type->get_kind() == pointer_type_K || fd->type->get_kind() == reference_type_K;
                     break;
                  }
                  case parm_decl_K:
                     // var_align = GetPointer<parm_decl>(var)->algn;
                     var_align = type_align;
                     break;
                  case result_decl_K:
                     // var_align = GetPointer<result_decl>(var)->algn;
                     var_align = type_align;
                     break;
                  case var_decl_K:
                  {
                     const auto vd = GetPointerS<const var_decl>(var);
                     var_align = vd->algn;
                     is_a_pointerP = vd->type->get_kind() == pointer_type_K || vd->type->get_kind() == reference_type_K;
                     is_static = vd->static_flag;
                     break;
                  }
                  case binfo_K:
                  case block_K:
                  case call_expr_K:
                  case aggr_init_expr_K:
                  case case_label_expr_K:
                  case const_decl_K:
                  case constructor_K:
                  case function_decl_K:
                  case identifier_node_K:
                  case label_decl_K:
                  case namespace_decl_K:
                  case ssa_name_K:
                  case statement_list_K:
                  case target_expr_K:
                  case target_mem_ref_K:
                  case target_mem_ref461_K:
                  case translation_unit_decl_K:
                  case template_decl_K:
                  case using_decl_K:
                  case tree_list_K:
                  case tree_vec_K:
                  case type_decl_K:
                  case error_mark_K:
                  case lut_expr_K:
                  case CASE_BINARY_EXPRESSION:
                  case CASE_CPP_NODES:
                  case CASE_CST_NODES:
                  case CASE_FAKE_NODES:
                  case CASE_GIMPLE_NODES:
                  case CASE_PRAGMA_NODES:
                  case CASE_QUATERNARY_EXPRESSION:
                  case CASE_TERNARY_EXPRESSION:
                  case CASE_TYPE_NODES:
                  case CASE_UNARY_EXPRESSION:
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
         }
         break;
      }
      case component_ref_K:
      {
         const auto cr = GetPointer<const component_ref>(node_type);
         res += PrintType(TM, cr->type, global);
         break;
      }
      case enumeral_type_K:
      {
         const auto et = GetPointer<const enumeral_type>(node_type);
         if(et->name && (et->name->get_kind() == type_decl_K || et->unql))
         {
            res += PrintType(TM, et->name, global);
         }
         else if(et->name)
         {
            res += "enum " + PrintType(TM, et->name, global);
         }
         else if(et->unql)
         {
            res += "Internal_" + STR(node_type->index);
         }
         else
         {
            res += "enum Internal_" + STR(node_type->index);
         }
         break;
      }
      case record_type_K:
      {
         const auto rt = GetPointerS<const record_type>(node_type);
         res += return_C_qualifiers(rt->qual, print_qualifiers);
         if(rt->name && (rt->name->get_kind() == type_decl_K || (rt->unql && !rt->system_flag)))
         {
            res += (rt->unql ? "" : "struct ") + PrintType(TM, rt->name, global);
         }
         else if(rt->name)
         {
            const auto struct_name = PrintType(TM, rt->name, global);
            if(struct_name == "_IO_FILE")
            {
               res += "FILE";
            }
            else
            {
               res += "struct " + NormalizeTypename(struct_name);
            }
         }
         else if(rt->unql)
         {
            res += "Internal_" + STR(node_type->index);
         }
         else
         {
            res += "struct Internal_" + STR(node_type->index);
         }
         if(rt->tmpl_args)
         {
            std::stringstream ss;
            const auto& tmpl_args = GetPointerS<const tree_vec>(rt->tmpl_args)->list_of_op;
            for(const auto& targ : tmpl_args)
            {
               ss << "_" << targ;
            }
            res += NormalizeTypename(ss.str());
         }
         break;
      }
      case union_type_K:
      {
         const auto ut = GetPointerS<const union_type>(node_type);
         res += return_C_qualifiers(ut->qual, print_qualifiers);
         if(ut->name && (ut->name->get_kind() == type_decl_K || (ut->unql && !ut->system_flag)))
         {
            res += PrintType(TM, ut->name, global);
         }
         else if(ut->name)
         {
            res += "union " + PrintType(TM, ut->name, global);
         }
         else if(ut->unql)
         {
            res += "Internal_" + STR(node_type->index);
         }
         else
         {
            res += "union Internal_" + STR(node_type->index);
         }
         break;
      }
      case tree_list_K:
      {
         THROW_ASSERT(!var, "Received something of unexpected");
         auto lnode = GetPointer<const tree_list>(node_type);
         res += PrintType(TM, lnode->valu, global, print_qualifiers);
         /// tree_list are used for parameters declaration: in that case void_type has to be removed from the last
         /// type parameter
         std::list<tree_nodeRef> prmtrs;
         while(lnode->chan)
         {
            lnode = GetPointer<const tree_list>(lnode->chan);
            if(!GetPointer<const void_type>(lnode->valu))
            {
               prmtrs.push_back(lnode->valu);
            }
         }
         for(const auto& valu : prmtrs)
         {
            res += "," + PrintType(TM, valu, global, print_qualifiers);
         }
         break;
      }
      case template_type_parm_K:
      {
         const auto ttp = GetPointer<const template_type_parm>(node_type);
         res += PrintType(TM, ttp->name, global, print_qualifiers);
         break;
      }
      case typename_type_K:
      {
         const auto tt = GetPointer<const typename_type>(node_type);
         res += PrintType(TM, tt->name, global, print_qualifiers);
         break;
      }
      case template_decl_K:
      {
         const auto td = GetPointer<const template_decl>(node_type);
         res += print_type(TM, td->name->index, global, print_qualifiers);
         break;
      }
      case parm_decl_K:
      {
         const auto pd = GetPointer<const parm_decl>(node_type);
         if(pd->readonly_flag)
         {
            res += print_qualifiers ? "const " : "/*const*/ ";
         }
         res += PrintType(TM, pd->type, global, print_qualifiers);
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case const_decl_K:
      case constructor_K:
      case field_decl_K:
      case label_decl_K:
      case lang_type_K:
      case namespace_decl_K:
      case offset_type_K:
      case qual_union_type_K:
      case result_decl_K:
      case set_type_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case type_argument_pack_K:
      case translation_unit_decl_K:
      case using_decl_K:
      case tree_vec_K:
      case var_decl_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case bit_field_ref_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case cond_expr_K:
      case dot_prod_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case fshl_expr_K:
      case fshr_expr_K:
      case bit_ior_concat_expr_K:
      case error_mark_K:
      case lut_expr_K:
      case insertvalue_expr_K:
      case insertelement_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
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

void FunctionExpander::check_lib_type(const tree_nodeRef& var)
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
            const auto tn = GetPointer<type_node>(dn->type);
            if(tn && tn->unql)
            {
               lib_types.insert(tn->unql);
            }
         }
      }
   }
}

bool FunctionExpander::operator()(const tree_nodeRef& tn) const
{
   THROW_ASSERT(GetPointer<type_node>(tn) || GetPointer<function_decl>(tn),
                "tn is not a node of type type_node nor function_decl");
   if(lib_types.find(tn) != lib_types.end())
   {
      return false;
   }
   if(tn->get_kind() == record_type_K)
   {
      const auto rt = GetPointer<record_type>(tn);
      if(rt->ptrmem_flag)
      {
         return false;
      }
   }
   const auto type = GetPointer<type_node>(tn);
   if(type && type->name)
   {
      const auto td = GetPointer<type_decl>(type->name);
      if(td)
      {
         std::string include_name = td->include_name;
         auto it_end = headers.end();
         for(auto it = headers.begin(); it != it_end; ++it)
         {
            if(include_name.find(*it) != std::string::npos)
            {
               return false;
            }
         }
      }
   }
   return true;
}

FunctionExpander::FunctionExpander()
{
   internal_serialize.insert("printf");
   transparent.insert("__builtin_va_start");
   headers.insert("stdio.h");
}

tree_nodeConstRef tree_helper::GetFormalIth(const tree_nodeConstRef& obj, unsigned int parm_index)
{
   if(obj->get_kind() == gimple_call_K)
   {
      const auto gc = GetPointerS<const gimple_call>(obj);
      THROW_ASSERT(gc->fn, "unexpected condition");

      const auto fn_type = CGetType(gc->fn);
      if(fn_type->get_kind() == pointer_type_K)
      {
         const auto pt = GetPointerS<const pointer_type>(fn_type);
         THROW_ASSERT(pt->ptd, "unexpected pattern");
         const auto ft = GetPointer<const function_type>(pt->ptd);
         if(ft && ft->varargs_flag)
         {
            return {};
         }
         else if(ft && ft->prms)
         {
            auto tl = GetPointerS<const tree_list>(ft->prms);
            THROW_ASSERT(tl, "unexpected condition");
            unsigned int ith = 0;
            if(parm_index == ith)
            {
               return tl->valu;
            }
            while(tl->chan)
            {
               ++ith;
               tl = GetPointerS<const tree_list>(tl->chan);
               if(parm_index == ith)
               {
                  return tl->valu;
               }
            }
            THROW_ERROR("unexpected pattern");
            return {};
         }
         else
         {
            const auto fn_node = gc->fn;
            /// parameters are not available through function_type but only through function_decl
            THROW_ASSERT(fn_node->get_kind() == addr_expr_K, "Unexpected pattern");
            const auto ue = GetPointerS<const unary_expr>(fn_node);
            const auto fn = ue->op;
            THROW_ASSERT(fn->get_kind() == function_decl_K, "Unexpected pattern");
            return GetFormalIth(fn, parm_index);
         }
      }
      else
      {
         THROW_ERROR("unexpected pattern");
         return {};
      }
   }
   else if(obj->get_kind() == gimple_assign_K)
   {
      const auto ga = GetPointerS<const gimple_assign>(obj);
      return GetFormalIth(ga->op1, parm_index);
   }
   else if(obj->get_kind() == call_expr_K || obj->get_kind() == aggr_init_expr_K)
   {
      const auto ce = GetPointerS<const call_expr>(obj);
      const auto fn_type = CGetType(ce->fn);
      if(fn_type->get_kind() == pointer_type_K)
      {
         const auto pt = GetPointerS<const pointer_type>(fn_type);
         THROW_ASSERT(pt->ptd, "unexpected pattern");
         const auto ft = GetPointer<const function_type>(pt->ptd);
         if(ft && ft->varargs_flag)
         {
            return {};
         }
         else if(ft && ft->prms)
         {
            auto tl = GetPointerS<const tree_list>(ft->prms);
            unsigned int ith = 0;
            if(parm_index == ith)
            {
               return tl->valu;
            }
            while(tl->chan)
            {
               ++ith;
               tl = GetPointerS<const tree_list>(tl->chan);
               if(parm_index == ith)
               {
                  return tl->valu;
               }
            }
            THROW_ERROR("unexpected pattern");
            return {};
         }
         else
         {
            const auto fn_node = ce->fn;
            /// parameters are not available through function_type but only through function_decl
            THROW_ASSERT(fn_node->get_kind() == addr_expr_K, "Unexpected pattern");
            const auto ue = GetPointerS<const unary_expr>(fn_node);
            const auto fn = ue->op;
            THROW_ASSERT(fn->get_kind(), "Unexpected pattern");
            return GetFormalIth(fn, parm_index);
         }
      }
      else
      {
         THROW_ERROR("unexpected pattern");
         return {};
      }
   }
   else if(obj->get_kind() == function_decl_K)
   {
      const auto fd = GetPointerS<const function_decl>(obj);
      unsigned int ith = 0;
      for(const auto& i : fd->list_of_args)
      {
         if(parm_index == ith)
         {
            return CGetType(i);
         }
         ++ith;
      }
   }
   return {};
}

unsigned int tree_helper::get_formal_ith(const tree_managerConstRef& TM, unsigned int index_obj,
                                         unsigned int parm_index)
{
   const auto t = GetFormalIth(TM->GetTreeNode(index_obj), parm_index);
   return t ? t->index : 0;
}

bool tree_helper::is_packed(const tree_managerConstRef& TreeM, unsigned int node_index)
{
   const auto node = TreeM->GetTreeNode(node_index);
   return IsPackedType(node);
}

bool tree_helper::IsPackedType(const tree_nodeConstRef& type)
{
   THROW_ASSERT(GetPointer<const decl_node>(type), "unexpected pattern" + type->get_kind_text());
   if(GetPointer<const decl_node>(type)->packed_flag)
   {
      return true;
   }
   auto node_type = GetPointer<const decl_node>(type)->type;
   if(GetPointer<const type_decl>(node_type))
   {
      node_type = GetPointer<const type_decl>(node_type)->type;
   }
   switch(node_type->get_kind())
   {
      case record_type_K:
      {
         auto rt = GetPointerS<const record_type>(node_type);
         if(rt->unql)
         {
            rt = GetPointerS<const record_type>(rt->unql);
         }
         THROW_ASSERT(!rt->unql, "unexpected pattern");
         if(rt->packed_flag)
         {
            return true;
         }
         for(auto& list_of_fld : rt->list_of_flds)
         {
            const auto fd = GetPointer<const field_decl>(list_of_fld);
            if(fd && fd->packed_flag)
            {
               return true;
            }
         }
         break;
      }
      case union_type_K:
      {
         auto ut = GetPointerS<const union_type>(node_type);
         if(ut->unql)
         {
            ut = GetPointerS<const union_type>(ut->unql);
         }
         THROW_ASSERT(!ut->unql, "unexpected pattern");
         if(ut->packed_flag)
         {
            return true;
         }

         /// Print the contents of the structure
         for(const auto& list_of_fld : ut->list_of_flds)
         {
            const auto fd = GetPointerS<const field_decl>(list_of_fld);
            if(fd->packed_flag)
            {
               return true;
            }
         }
         break;
      }
      case enumeral_type_K:
      {
         auto et = GetPointerS<const enumeral_type>(node_type);
         if(et->unql)
         {
            et = GetPointerS<const enumeral_type>(et->unql);
         }
         THROW_ASSERT(!et->unql, "unexpected pattern");
         if(et->packed_flag)
         {
            return true;
         }
         break;
      }
      case array_type_K:
      case pointer_type_K:
      case boolean_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case function_type_K:
      case integer_type_K:
      case lang_type_K:
      case method_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case real_type_K:
      case reference_type_K:
      case set_type_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case typename_type_K:
      case vector_type_K:
      case void_type_K:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
      {
         THROW_UNREACHABLE("Unexpected type " + node_type->get_kind_text());
      }
   }
   return false;
}

bool tree_helper::is_packed_access(const tree_managerConstRef& TreeM, unsigned int node_index)
{
   const auto t = TreeM->GetTreeNode(node_index);
   bool res = false;
   switch(t->get_kind())
   {
      case mem_ref_K:
      {
         const auto mr = GetPointer<const mem_ref>(t);
         return is_packed_access(TreeM, mr->op0->index);
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointer<const target_mem_ref461>(t);
         return is_packed_access(TreeM, tmr->base->index);
      }
      case component_ref_K:
      {
         const auto cr = GetPointer<const component_ref>(t);
         const auto fd = cr->op1;
         if(GetPointer<const field_decl>(fd) && GetPointer<const field_decl>(fd)->packed_flag)
         {
            res = true;
         }
         break;
      }
      case realpart_expr_K:
      case imagpart_expr_K:
      case bit_field_ref_K:
      case array_ref_K:
      {
         res = false;
         break;
      }
      case addr_expr_K:
      {
         const auto ae = GetPointer<const addr_expr>(t);
         return is_packed_access(TreeM, ae->op->index);
      }
      case ssa_name_K:
      {
         const auto sn = GetPointer<const ssa_name>(t);
         const auto def_stmt = sn->CGetDefStmt();
         if(def_stmt->get_kind() == gimple_assign_K)
         {
            const auto ga = GetPointer<const gimple_assign>(def_stmt);
            if(ga->temporary_address)
            {
               return is_packed_access(TreeM, ga->op1->index);
            }
         }
         res = false;
         break;
      }
      case var_decl_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case statement_list_K:
      case target_mem_ref_K:
      case tree_list_K:
      case tree_vec_K:
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
      case paren_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
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
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case const_decl_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case parm_decl_K:
      case result_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case type_decl_K:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case cond_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case fshl_expr_K:
      case fshr_expr_K:
      case bit_ior_concat_expr_K:
      case abs_expr_K:
      case alignof_expr_K:
      case arrow_expr_K:
      case bit_not_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case convert_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fix_trunc_expr_K:
      case float_expr_K:
      case indirect_ref_K:
      case misaligned_indirect_ref_K:
      case loop_expr_K:
      case lut_expr_K:
      case negate_expr_K:
      case non_lvalue_expr_K:
      case nop_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case truth_not_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case view_convert_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      case CASE_TYPE_NODES:
      case array_range_ref_K:
      case target_expr_K:
      case error_mark_K:
      case extract_bit_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case frem_expr_K:
      {
         res = false;
         break;
      }
      case extractvalue_expr_K:
      case insertvalue_expr_K:
      case extractelement_expr_K:
      case insertelement_expr_K:
      {
         res = true;
         break;
      }
      default:
         THROW_ERROR("elements not yet supported: " + t->get_kind_text());
   }

   return res;
}

unsigned long long tree_helper::AccessedMaximumBitsize(const tree_nodeConstRef& type_node, unsigned long long bitsize)
{
   switch(type_node->get_kind())
   {
      case array_type_K:
      {
         const auto atype = GetPointerS<const array_type>(type_node);
         return AccessedMaximumBitsize(atype->elts, bitsize);
      }
      case record_type_K:
      {
         const auto rt = GetPointerS<const record_type>(type_node);
         for(const auto& fli : rt->list_of_flds)
         {
            if(fli->get_kind() == type_decl_K)
            {
               continue;
            }
            if(fli->get_kind() == const_decl_K)
            {
               continue;
            }
            if(fli->get_kind() == template_decl_K)
            {
               continue;
            }
            if(fli->get_kind() == function_decl_K)
            {
               continue;
            }
            if(fli->get_kind() == var_decl_K)
            {
               bitsize = AccessedMaximumBitsize(GetPointerS<const var_decl>(fli)->type, bitsize);
            }
            else
            {
               bitsize = AccessedMaximumBitsize(fli, bitsize);
            }
         }
         return bitsize;
      }
      case union_type_K:
      {
         const auto ut = GetPointerS<const union_type>(type_node);
         for(const auto& fli : ut->list_of_flds)
         {
            bitsize = AccessedMaximumBitsize(fli, bitsize);
         }
         return bitsize;
      }
      case field_decl_K:
      {
         const auto fd_type_node = CGetType(type_node);
         return AccessedMaximumBitsize(fd_type_node, bitsize);
      }
      case complex_type_K:
      {
         return std::max(bitsize, SizeAlloc(type_node) / 2); /// it is composed by two identical parts
      }
      case real_type_K:
      case integer_type_K:
      case enumeral_type_K:
      case pointer_type_K:
      case reference_type_K:
      case void_type_K:
      case vector_type_K:
      {
         return std::max(bitsize, SizeAlloc(type_node));
      }
      case function_decl_K:
      case function_type_K:
      case method_type_K:
      {
         return 32;
      }
      case boolean_type_K:
      {
         return 8;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case const_decl_K:
      case constructor_K:
      case identifier_node_K:
      case label_decl_K:
      case lang_type_K:
      case namespace_decl_K:
      case offset_type_K:
      case parm_decl_K:
      case qual_union_type_K:
      case result_decl_K:
      case set_type_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case typename_type_K:
      case var_decl_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR("elements not yet supported: " + type_node->get_kind_text() + " " + STR(type_node->index));
   }
   return 0;
}

unsigned long long tree_helper::AccessedMinimunBitsize(const tree_nodeConstRef& type_node, unsigned long long bitsize)
{
   switch(type_node->get_kind())
   {
      case array_type_K:
      {
         const auto atype = GetPointerS<const array_type>(type_node);
         return AccessedMinimunBitsize(atype->elts, bitsize);
      }
      case record_type_K:
      {
         const auto rt = GetPointerS<const record_type>(type_node);
         for(const auto& fli : rt->list_of_flds)
         {
            if(fli->get_kind() == type_decl_K)
            {
               continue;
            }
            if(fli->get_kind() == const_decl_K)
            {
               continue;
            }
            if(fli->get_kind() == template_decl_K)
            {
               continue;
            }
            if(fli->get_kind() == function_decl_K)
            {
               continue;
            }
            if(fli->get_kind() == var_decl_K)
            {
               bitsize = AccessedMinimunBitsize(GetPointerS<const var_decl>(fli)->type, bitsize);
            }
            else
            {
               bitsize = AccessedMinimunBitsize(fli, bitsize);
            }
         }
         return bitsize;
      }
      case union_type_K:
      {
         const auto ut = GetPointerS<const union_type>(type_node);
         for(const auto& fli : ut->list_of_flds)
         {
            bitsize = AccessedMinimunBitsize(fli, bitsize);
         }
         return bitsize;
      }
      case field_decl_K:
      {
         const auto fd_type_node = CGetType(type_node);
         return AccessedMinimunBitsize(fd_type_node, bitsize);
      }
      case complex_type_K:
      {
         return std::min(bitsize, SizeAlloc(type_node) / 2); /// it is composed by two identical parts
      }
      case real_type_K:
      case integer_type_K:
      case enumeral_type_K:
      case pointer_type_K:
      case reference_type_K:
      case void_type_K:
      case vector_type_K:
      {
         return std::min(bitsize, SizeAlloc(type_node));
      }
      case boolean_type_K:
      {
         return 8;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case const_decl_K:
      case constructor_K:
      case function_decl_K:
      case function_type_K:
      case identifier_node_K:
      case label_decl_K:
      case lang_type_K:
      case method_type_K:
      case namespace_decl_K:
      case offset_type_K:
      case parm_decl_K:
      case qual_union_type_K:
      case result_decl_K:
      case set_type_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case typename_type_K:
      case var_decl_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR("elements not yet supported: " + type_node->get_kind_text());
   }
   return 0;
}

size_t tree_helper::AllocatedMemorySize(const tree_nodeConstRef& parameter)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Analyzing " + parameter->ToString());
   switch(parameter->get_kind())
   {
      case(addr_expr_K):
      {
         const auto ae = GetPointer<const addr_expr>(parameter);
         /// Note that this part can not be transfromed in recursion because size of array ref corresponds to the
         /// size of the element itself
         const tree_nodeRef addr_expr_argument = ae->op;
         switch(addr_expr_argument->get_kind())
         {
            case(array_ref_K):
            {
               const array_ref* ar = GetPointer<array_ref>(addr_expr_argument);
               const size_t byte_parameter_size = AllocatedMemorySize(ar->op0);
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
               return byte_parameter_size;
            }
            case(component_ref_K):
            case(mem_ref_K):
            case(parm_decl_K):
            case(string_cst_K):
            case(var_decl_K):
            {
               const size_t byte_parameter_size = AllocatedMemorySize(addr_expr_argument);
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
               return byte_parameter_size;
            }
            case array_range_ref_K:
            case binfo_K:
            case block_K:
            case call_expr_K:
            case aggr_init_expr_K:
            case case_label_expr_K:
            case complex_cst_K:
            case constructor_K:
            case identifier_node_K:
            case integer_cst_K:
            case real_cst_K:
            case ssa_name_K:
            case statement_list_K:
            case target_expr_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case tree_list_K:
            case tree_vec_K:
            case vector_cst_K:
            case void_cst_K:
            case const_decl_K:
            case field_decl_K:
            case function_decl_K:
            case label_decl_K:
            case namespace_decl_K:
            case result_decl_K:
            case translation_unit_decl_K:
            case template_decl_K:
            case using_decl_K:
            case type_decl_K:
            case bit_field_ref_K:
            case vtable_ref_K:
            case with_cleanup_expr_K:
            case obj_type_ref_K:
            case save_expr_K:
            case cond_expr_K:
            case dot_prod_expr_K:
            case ternary_plus_expr_K:
            case ternary_pm_expr_K:
            case ternary_mp_expr_K:
            case ternary_mm_expr_K:
            case fshl_expr_K:
            case fshr_expr_K:
            case bit_ior_concat_expr_K:
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
            case lut_expr_K:
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
            case vec_cond_expr_K:
            case vec_pack_trunc_expr_K:
            case vec_pack_sat_expr_K:
            case vec_pack_fix_trunc_expr_K:
            case vec_perm_expr_K:
            case vec_extracteven_expr_K:
            case vec_extractodd_expr_K:
            case vec_interleavehigh_expr_K:
            case vec_interleavelow_expr_K:
            case error_mark_K:
            case extract_bit_expr_K:
            case sat_plus_expr_K:
            case sat_minus_expr_K:
            case extractvalue_expr_K:
            case insertvalue_expr_K:
            case extractelement_expr_K:
            case insertelement_expr_K:
            case frem_expr_K:
            case CASE_CPP_NODES:
            case CASE_FAKE_NODES:
            case CASE_GIMPLE_NODES:
            case CASE_PRAGMA_NODES:
            case CASE_TYPE_NODES:
            case CASE_UNARY_EXPRESSION:
            default:
            {
               THROW_UNREACHABLE("Unsupported addr_expr argument " + addr_expr_argument->get_kind_text());
            }
         }
         break;
      }
      case(array_type_K):
      {
         const auto at = GetPointer<const array_type>(parameter);
         /// This call check if we can perform deep copy of the single element
         AllocatedMemorySize(at->elts);
         const size_t bit_parameter_size = SizeAlloc(parameter);
         /// Round to upper multiple word size
         const size_t byte_parameter_size = bit_parameter_size / 8;
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(record_type_K):
      {
         size_t fields_pointed_size = 0;
         const auto rt = GetPointer<const record_type>(parameter);
         const std::vector<tree_nodeRef>& list_of_fields = rt->list_of_flds;
         /// This calls check if we can perform deep copy of the single element
         std::vector<tree_nodeRef>::const_iterator field, field_end = list_of_fields.end();
         for(field = list_of_fields.begin(); field != field_end; ++field)
         {
            if((*field)->get_kind() == type_decl_K)
            {
               continue;
            }
            if((*field)->get_kind() == function_decl_K)
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
      case(component_ref_K):
      {
         const auto cr = GetPointer<const component_ref>(parameter);
         if(GetPointer<const union_type>(CGetType(cr->op0)))
         {
            THROW_ERROR("Offloading fields of union is not supported");
         }
         const size_t byte_parameter_size = AllocatedMemorySize(cr->op1);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(field_decl_K):
      {
         const size_t byte_parameter_size = AllocatedMemorySize(CGetType(parameter));
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(enumeral_type_K):
      case(integer_type_K):
      case(real_type_K):
      case(string_cst_K):
      {
         const size_t bit_parameter_size = SizeAlloc(parameter);
         /// Round to upper multiple word size
         const size_t byte_parameter_size = bit_parameter_size / 8;
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(mem_ref_K):
      {
         const auto mr = GetPointer<const mem_ref>(parameter);
         const size_t byte_parameter_size = AllocatedMemorySize(mr->op0);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case(parm_decl_K):
      case(ssa_name_K):
      case(var_decl_K):
      {
         const auto sn = GetPointer<const ssa_name>(parameter);
         if(sn && (sn->var->get_kind() == parm_decl_K) && sn->CGetDefStmts().empty())
         {
            return AllocatedMemorySize(sn->var);
         }

         const auto ptype = CGetType(parameter);
         const auto byte_parameter_size = AllocatedMemorySize(IsPointerType(ptype) ? CGetPointedType(ptype) : ptype);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "<--Analyzed " + parameter->ToString() + " - Size is " + STR(byte_parameter_size));
         return byte_parameter_size;
      }
      case binfo_K:
      case bit_field_ref_K:
      case block_K:
      case boolean_type_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_cst_K:
      case complex_type_K:
      case cond_expr_K:
      case const_decl_K:
      case constructor_K:
      case dot_prod_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case fshl_expr_K:
      case fshr_expr_K:
      case bit_ior_concat_expr_K:
      case function_decl_K:
      case identifier_node_K:
      case integer_cst_K:
      case label_decl_K:
      case lang_type_K:
      case method_type_K:
      case namespace_decl_K:
      case obj_type_ref_K:
      case offset_type_K:
      case paren_expr_K:
      case pointer_type_K:
      case qual_union_type_K:
      case real_cst_K:
      case result_decl_K:
      case save_expr_K:
      case set_type_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_list_K:
      case tree_vec_K:
      case type_decl_K:
      case typename_type_K:
      case union_type_K:
      case vector_cst_K:
      case void_cst_K:
      case void_type_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case function_type_K:
      case reference_type_K:
      case vector_type_K:
      case abs_expr_K:
      case alignof_expr_K:
      case arrow_expr_K:
      case bit_not_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case convert_expr_K:
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
      case lut_expr_K:
      case negate_expr_K:
      case non_lvalue_expr_K:
      case nop_expr_K:
      case realpart_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case truth_not_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case view_convert_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
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
      case vec_cond_expr_K:
      case vec_perm_expr_K:
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
      case error_mark_K:
      case extract_bit_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case insertvalue_expr_K:
      case extractelement_expr_K:
      case insertelement_expr_K:
      case frem_expr_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      default:
      {
         THROW_UNREACHABLE("Unsupported tree node type " + parameter->get_kind_text() + " (" + parameter->ToString() +
                           ")");
      }
   }
   return 0;
}

size_t tree_helper::CountPointers(const tree_nodeConstRef& tn)
{
   size_t counter = 0;
   switch(tn->get_kind())
   {
      case enumeral_type_K:
      case integer_type_K:
      case real_type_K:
      {
         return 0;
      }
      case field_decl_K:
      case parm_decl_K:
      {
         return CountPointers(CGetType(tn));
      }
      case reference_type_K:
      case pointer_type_K:
      {
         return 1;
      }
      case record_type_K:
      {
         const auto rt = GetPointer<const record_type>(tn);
         const std::vector<tree_nodeRef> list_of_fields = rt->list_of_flds;
         std::vector<tree_nodeRef>::const_iterator field, field_end = list_of_fields.end();
         for(field = list_of_fields.begin(); field != field_end; ++field)
         {
            if((*field)->get_kind() == type_decl_K)
            {
               continue;
            }
            if((*field)->get_kind() == function_decl_K)
            {
               continue;
            }
            counter += CountPointers(*field);
         }
         return counter;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case lang_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case set_type_K:
      case ssa_name_K:
      case statement_list_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case template_type_parm_K:
      case type_argument_pack_K:
      case tree_list_K:
      case tree_vec_K:
      case typename_type_K:
      case const_decl_K:
      case function_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case result_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case type_decl_K:
      case var_decl_K:
      case array_type_K:
      case boolean_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case function_type_K:
      case method_type_K:
      case union_type_K:
      case vector_type_K:
      case void_type_K:
      case target_expr_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
      {
         THROW_UNREACHABLE("Unsupported type node " + tn->get_kind_text());
      }
   }
   return counter;
}

unsigned int tree_helper::get_multi_way_if_pos(const tree_managerConstRef& TM, unsigned int node_id,
                                               unsigned int looked_for_cond)
{
   const auto t = TM->GetTreeNode(node_id);
   const auto gmwi = GetPointer<const gimple_multi_way_if>(t);
   unsigned int pos = 0;
   for(auto const& cond : gmwi->list_of_cond)
   {
      if(cond.first && cond.first->index == looked_for_cond)
      {
         return pos;
      }
      pos++;
   }
   THROW_ERROR("cond not found in gimple_multi_way_if " + t->ToString() + " looked_for_cond " + STR(looked_for_cond));
   return pos;
}

void tree_helper::compute_ssa_uses_rec_ptr(const tree_nodeConstRef& curr_tn,
                                           CustomOrderedSet<const ssa_name*>& ssa_uses)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->compute_ssa_uses_rec_ptr " + curr_tn->ToString());
   const auto gn = GetPointer<const gimple_node>(curr_tn);
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
      case gimple_return_K:
      {
         const auto re = GetPointer<const gimple_return>(curr_tn);
         if(re->op)
         {
            compute_ssa_uses_rec_ptr(re->op, ssa_uses);
         }
         break;
      }
      case gimple_assign_K:
      {
         const auto me = GetPointer<const gimple_assign>(curr_tn);
         if(me->op0->get_kind() != ssa_name_K)
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
      case gimple_nop_K:
      {
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointer<const call_expr>(curr_tn);
         compute_ssa_uses_rec_ptr(ce->fn, ssa_uses);
         for(const auto& arg : ce->args)
         {
            compute_ssa_uses_rec_ptr(arg, ssa_uses);
         }
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointer<const gimple_call>(curr_tn);
         compute_ssa_uses_rec_ptr(ce->fn, ssa_uses);
         for(const auto& arg : ce->args)
         {
            compute_ssa_uses_rec_ptr(arg, ssa_uses);
         }
         break;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointer<const gimple_cond>(curr_tn);
         compute_ssa_uses_rec_ptr(gc->op0, ssa_uses);
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointer<const unary_expr>(curr_tn);
         compute_ssa_uses_rec_ptr(ue->op, ssa_uses);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointer<const binary_expr>(curr_tn);
         compute_ssa_uses_rec_ptr(be->op0, ssa_uses);
         compute_ssa_uses_rec_ptr(be->op1, ssa_uses);
         break;
      }
      /*ternary expressions*/
      case gimple_switch_K:
      {
         const auto se = GetPointer<const gimple_switch>(curr_tn);
         compute_ssa_uses_rec_ptr(se->op0, ssa_uses);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointer<const ternary_expr>(curr_tn);
         compute_ssa_uses_rec_ptr(te->op0, ssa_uses);
         compute_ssa_uses_rec_ptr(te->op1, ssa_uses);
         if(te->op2)
         {
            compute_ssa_uses_rec_ptr(te->op2, ssa_uses);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointer<const quaternary_expr>(curr_tn);
         compute_ssa_uses_rec_ptr(qe->op0, ssa_uses);
         compute_ssa_uses_rec_ptr(qe->op1, ssa_uses);
         if(qe->op2)
         {
            compute_ssa_uses_rec_ptr(qe->op2, ssa_uses);
         }
         if(qe->op3)
         {
            compute_ssa_uses_rec_ptr(qe->op3, ssa_uses);
         }
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointer<const lut_expr>(curr_tn);
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
      case constructor_K:
      {
         const auto c = GetPointer<const constructor>(curr_tn);
         for(const auto& iv : c->list_of_idx_valu)
         {
            compute_ssa_uses_rec_ptr(iv.second, ssa_uses);
         }
         break;
      }
      case var_decl_K:
      {
         /// var decl performs an assignment when init is not null
         // const auto vd = GetPointer<const var_decl>(curr_tn);
         // if(vd->init)
         //   compute_ssa_uses_rec_ptr(vd->init, ssa_uses);
         break;
      }
      case gimple_asm_K:
      {
         const auto ae = GetPointer<const gimple_asm>(curr_tn);
         // if(ae->out)
         //   compute_ssa_uses_rec_ptr(ae->out, ssa_uses);
         if(ae->in)
         {
            compute_ssa_uses_rec_ptr(ae->in, ssa_uses);
         }
         if(ae->clob)
         {
            compute_ssa_uses_rec_ptr(ae->in, ssa_uses);
         }
         break;
      }
      case gimple_goto_K:
      {
         const auto ge = GetPointer<const gimple_goto>(curr_tn);
         compute_ssa_uses_rec_ptr(ge->op, ssa_uses);
         break;
      }
      case tree_list_K:
      {
         auto tl = GetPointer<const tree_list>(curr_tn);
         std::list<const tree_list*> tl_list;
         do
         {
            tl_list.push_back(tl);
            tl = tl->chan ? GetPointer<const tree_list>(tl->chan) : nullptr;
         } while(tl);
         for(const auto& tl_current0 : tl_list)
         {
            if(tl_current0->purp)
            {
               compute_ssa_uses_rec_ptr(tl_current0->purp, ssa_uses);
            }
            if(tl_current0->valu)
            {
               compute_ssa_uses_rec_ptr(tl_current0->valu, ssa_uses);
            }
         }
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointer<const gimple_multi_way_if>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               compute_ssa_uses_rec_ptr(cond.first, ssa_uses);
            }
         }
         break;
      }
      case gimple_phi_K:
      case result_decl_K:
      case parm_decl_K:
      case function_decl_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case field_decl_K:
      case label_decl_K:
      case gimple_label_K:
      case CASE_PRAGMA_NODES:
      {
         break;
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointer<const target_mem_ref>(curr_tn);
         if(tmr->base)
         {
            compute_ssa_uses_rec_ptr(tmr->base, ssa_uses);
         }
         if(tmr->symbol)
         {
            compute_ssa_uses_rec_ptr(tmr->symbol, ssa_uses);
         }
         if(tmr->idx)
         {
            compute_ssa_uses_rec_ptr(tmr->idx, ssa_uses);
         }
         /// step and offset are constants
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointer<const target_mem_ref461>(curr_tn);
         if(tmr->base)
         {
            compute_ssa_uses_rec_ptr(tmr->base, ssa_uses);
         }
         if(tmr->idx)
         {
            compute_ssa_uses_rec_ptr(tmr->idx, ssa_uses);
         }
         if(tmr->idx2)
         {
            compute_ssa_uses_rec_ptr(tmr->idx2, ssa_uses);
         }
         /// step and offset are constants
         break;
      }
      case ssa_name_K:
      {
         ssa_uses.insert(GetPointer<const ssa_name>(curr_tn));
         break;
      }
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case const_decl_K:
      case error_mark_K:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_while_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case template_decl_K:
      case translation_unit_decl_K:
      case tree_vec_K:
      case type_decl_K:
      case using_decl_K:
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

TreeNodeMap<size_t> tree_helper::ComputeSsaUses(const tree_nodeRef& tn)
{
   TreeNodeMap<size_t> ret_value;
   ComputeSsaUses(tn, ret_value);
   return ret_value;
}

void tree_helper::ComputeSsaUses(const tree_nodeRef& tn, TreeNodeMap<size_t>& ssa_uses)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Computing ssa uses in " + tn->ToString() + " (" + tn->get_kind_text() + ")");
   const auto recurse_virtuals = [&](const gimple_node* gn) {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Computing virtual ssa uses");
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
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Computed virtual ssa uses");
   };

   switch(tn->get_kind())
   {
      case gimple_return_K:
      {
         const auto re = GetPointerS<gimple_return>(tn);
         recurse_virtuals(re);
         if(re->op)
         {
            ComputeSsaUses(re->op, ssa_uses);
         }
         break;
      }
      case gimple_assign_K:
      {
         const auto ga = GetPointerS<gimple_assign>(tn);
         recurse_virtuals(ga);
         if(ga->op0->get_kind() != ssa_name_K)
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
      case gimple_nop_K:
      {
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<call_expr>(tn);
         ComputeSsaUses(ce->fn, ssa_uses);
         for(const auto& arg : ce->args)
         {
            ComputeSsaUses(arg, ssa_uses);
         }
         break;
      }
      case gimple_call_K:
      {
         const auto gc = GetPointerS<gimple_call>(tn);
         recurse_virtuals(gc);
         ComputeSsaUses(gc->fn, ssa_uses);
         for(const auto& arg : gc->args)
         {
            ComputeSsaUses(arg, ssa_uses);
         }
         break;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<gimple_cond>(tn);
         recurse_virtuals(gc);
         ComputeSsaUses(gc->op0, ssa_uses);
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<unary_expr>(tn);
         if(ue->op->get_kind() != function_decl_K)
         {
            ComputeSsaUses(ue->op, ssa_uses);
         }
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<binary_expr>(tn);
         ComputeSsaUses(be->op0, ssa_uses);
         ComputeSsaUses(be->op1, ssa_uses);
         break;
      }
      /*ternary expressions*/
      case gimple_switch_K:
      {
         const auto se = GetPointerS<gimple_switch>(tn);
         recurse_virtuals(se);
         ComputeSsaUses(se->op0, ssa_uses);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointerS<ternary_expr>(tn);
         ComputeSsaUses(te->op0, ssa_uses);
         ComputeSsaUses(te->op1, ssa_uses);
         if(te->op2)
         {
            ComputeSsaUses(te->op2, ssa_uses);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointerS<quaternary_expr>(tn);
         ComputeSsaUses(qe->op0, ssa_uses);
         ComputeSsaUses(qe->op1, ssa_uses);
         if(qe->op2)
         {
            ComputeSsaUses(qe->op2, ssa_uses);
         }
         if(qe->op3)
         {
            ComputeSsaUses(qe->op3, ssa_uses);
         }
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointerS<lut_expr>(tn);
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
      case constructor_K:
      {
         const auto c = GetPointerS<constructor>(tn);
         for(const auto& iv : c->list_of_idx_valu)
         {
            ComputeSsaUses(iv.second, ssa_uses);
         }
         break;
      }
      case var_decl_K:
      {
         /// var decl performs an assignment when init is not null
         // const auto vd = GetPointerS<var_decl>(tn);
         // if(vd->init)
         // {
         //   ComputeSsaUses(vd->init, ssa_uses);
         // }
         break;
      }
      case gimple_asm_K:
      {
         const auto ae = GetPointerS<gimple_asm>(tn);
         recurse_virtuals(ae);
         if(ae->out)
         {
            ComputeSsaUses(ae->out, ssa_uses);
         }
         if(ae->in)
         {
            ComputeSsaUses(ae->in, ssa_uses);
         }
         if(ae->clob)
         {
            ComputeSsaUses(ae->clob, ssa_uses);
         }
         break;
      }
      case gimple_goto_K:
      {
         const auto ge = GetPointerS<gimple_goto>(tn);
         recurse_virtuals(ge);
         ComputeSsaUses(ge->op, ssa_uses);
         break;
      }
      case tree_list_K:
      {
         auto tl = GetPointerS<const tree_list>(tn);
         do
         {
            if(tl->purp)
            {
               ComputeSsaUses(tl->purp, ssa_uses);
            }
            if(tl->valu)
            {
               ComputeSsaUses(tl->valu, ssa_uses);
            }
            tl = tl->chan ? GetPointerS<const tree_list>(tl->chan) : nullptr;
         } while(tl);
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointerS<gimple_multi_way_if>(tn);
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
      case gimple_phi_K:
      {
         const auto gp = GetPointerS<gimple_phi>(tn);
         recurse_virtuals(gp);
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            ComputeSsaUses(def_edge.first, ssa_uses);
         }
         break;
      }
      case result_decl_K:
      case parm_decl_K:
      case function_decl_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case field_decl_K:
      case label_decl_K:
      case gimple_label_K:
      case CASE_PRAGMA_NODES:
      {
         break;
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<target_mem_ref>(tn);
         if(tmr->base)
         {
            ComputeSsaUses(tmr->base, ssa_uses);
         }
         if(tmr->symbol)
         {
            ComputeSsaUses(tmr->symbol, ssa_uses);
         }
         if(tmr->idx)
         {
            ComputeSsaUses(tmr->idx, ssa_uses);
         }
         /// step and offset are constants
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<target_mem_ref461>(tn);
         if(tmr->base)
         {
            ComputeSsaUses(tmr->base, ssa_uses);
         }
         if(tmr->idx)
         {
            ComputeSsaUses(tmr->idx, ssa_uses);
         }
         if(tmr->idx2)
         {
            ComputeSsaUses(tmr->idx2, ssa_uses);
         }
         /// step and offset are constants
         break;
      }
      case ssa_name_K:
      {
         ssa_uses[tn]++;
         break;
      }
      case gimple_pragma_K:
      {
         break;
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case CASE_CPP_NODES:
      case const_decl_K:
      case CASE_FAKE_NODES:
      case gimple_for_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_bind_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_vec_K:
      case type_decl_K:
      case CASE_TYPE_NODES:
      case gimple_while_K:
      case error_mark_K:
      {
         THROW_UNREACHABLE("Node is " + tn->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Computed ssa uses in @" + STR(tn->index));
}

bool tree_helper::is_a_nop_function_decl(const function_decl* fd)
{
   if(fd->body)
   {
      const auto sl = GetPointerS<const statement_list>(fd->body);
      if(!sl->list_of_stmt.empty())
      {
         return false;
      }
      else if(!sl->list_of_bloc.empty())
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
            const auto gr = GetPointer<const gimple_return>(single_stmt);
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

void tree_helper::get_required_values(std::vector<std::tuple<unsigned int, unsigned int>>& required,
                                      const tree_nodeConstRef& tn)
{
   auto tn_kind = tn->get_kind();
   switch(tn_kind)
   {
      case constructor_K:
      {
         const auto co = GetPointerS<const constructor>(tn);
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
      case ssa_name_K:
      case string_cst_K:
      case real_cst_K:
      case integer_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case var_decl_K:
      case parm_decl_K:
      case result_decl_K:
      {
         required.emplace_back(tn->index, 0);
         break;
      }
      case gimple_while_K:
      {
         const auto we = GetPointerS<const gimple_while>(tn);
         get_required_values(required, we->op0);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<const binary_expr>(tn);
         get_required_values(required, be->op0);
         if(tn->get_kind() != assert_expr_K)
         {
            get_required_values(required, be->op1);
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<const unary_expr>(tn);
         if(tn->get_kind() == addr_expr_K /*|| tn->get_kind() == imagpart_expr_K || tn->get_kind() == realpart_expr_K*/)
         {
            required.emplace_back(tn->index, 0);
         }
         else
         {
            get_required_values(required, ue->op);
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         if(tn_kind == component_ref_K)
         {
            const auto cr = GetPointerS<const component_ref>(tn);
            required.emplace_back(cr->op0->index, 0);
            get_required_values(required, cr->op1);
         }
         else if(tn_kind == bit_field_ref_K)
         {
            const auto te = GetPointerS<const ternary_expr>(tn);
            required.emplace_back(te->op0->index, 0);
            get_required_values(required, te->op1);
            get_required_values(required, te->op2);
         }
         else if(tn_kind == obj_type_ref_K || tn_kind == save_expr_K || tn_kind == vtable_ref_K ||
                 tn_kind == with_cleanup_expr_K)
         {
            THROW_ERROR("Operation not yet supported: " + std::string(tn->get_kind_text()));
         }
         else
         {
            const auto te = GetPointerS<const ternary_expr>(tn);
            get_required_values(required, te->op0);
            get_required_values(required, te->op1);
            get_required_values(required, te->op2);
         }
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointerS<const lut_expr>(tn);
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
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<const gimple_cond>(tn);
         get_required_values(required, gc->op0);
         break;
      }
      case gimple_switch_K:
      {
         const auto se = GetPointerS<const gimple_switch>(tn);
         get_required_values(required, se->op0);
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointerS<const gimple_multi_way_if>(tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               get_required_values(required, cond.first);
            }
         }
         break;
      }
      case array_ref_K:
      {
         const auto ar = GetPointerS<const array_ref>(tn);
         required.emplace_back(ar->op0->index, 0);
         get_required_values(required, ar->op1);
         break;
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref>(tn);
         if(tmr->symbol)
         {
            required.emplace_back(tmr->symbol->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->base)
         {
            required.emplace_back(tmr->base->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->idx)
         {
            required.emplace_back(tmr->idx->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->step)
         {
            required.emplace_back(tmr->step->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->offset)
         {
            required.emplace_back(tmr->offset->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref461>(tn);
         if(tmr->base)
         {
            required.emplace_back(tmr->base->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->idx)
         {
            required.emplace_back(tmr->idx->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->step)
         {
            required.emplace_back(tmr->step->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->idx2)
         {
            required.emplace_back(tmr->idx2->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->offset)
         {
            required.emplace_back(tmr->offset->index, 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         break;
      }
      case gimple_assign_K:
      {
         const auto gm = GetPointerS<const gimple_assign>(tn);
         if(!gm->init_assignment && !gm->clobber)
         {
            const auto op0_kind = gm->op0->get_kind();
            const auto op1_kind = gm->op1->get_kind();

            if(op0_kind == component_ref_K || op0_kind == indirect_ref_K || op0_kind == misaligned_indirect_ref_K ||
               op0_kind == mem_ref_K || op0_kind == array_ref_K || op0_kind == target_mem_ref_K ||
               op0_kind == target_mem_ref461_K || op0_kind == bit_field_ref_K)
            {
               get_required_values(required, gm->op1);
               get_required_values(required, gm->op0);
            }
            else
            {
               bool is_a_vector_bitfield = false;
               if(op1_kind == bit_field_ref_K)
               {
                  const auto bfr = GetPointerS<const bit_field_ref>(gm->op1);
                  if(IsVectorType(bfr->op0))
                  {
                     is_a_vector_bitfield = true;
                  }
               }
               if(op1_kind == component_ref_K || op1_kind == indirect_ref_K || op1_kind == misaligned_indirect_ref_K ||
                  op1_kind == mem_ref_K || op1_kind == array_ref_K || op1_kind == target_mem_ref_K ||
                  op1_kind == target_mem_ref461_K || (op1_kind == bit_field_ref_K && !is_a_vector_bitfield))
               {
                  required.emplace_back(0, 0);
               }
               get_required_values(required, gm->op1);
            }
            if(gm->predicate)
            {
               get_required_values(required, gm->predicate);
            }
         }
         break;
      }
      case gimple_return_K:
      {
         const auto rt = GetPointerS<const gimple_return>(tn);
         if(rt->op)
         {
            get_required_values(required, rt->op);
         }
         break;
      }
      case gimple_phi_K:
      {
         const auto gp = GetPointerS<const gimple_phi>(tn);
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            required.emplace_back(def_edge.first->index, 0);
         }
         break;
      }
      case label_decl_K:
      case gimple_label_K:
      case gimple_goto_K:
      case gimple_nop_K:
      case gimple_pragma_K:
      case CASE_PRAGMA_NODES:
      {
         /// this has not to be synthesized
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<const call_expr>(tn);
         for(const auto& arg : ce->args)
         {
            required.emplace_back(arg->index, 0);
         }
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointerS<const gimple_call>(tn);
         const function_decl* fd = nullptr;
         if(ce->fn->get_kind() == addr_expr_K)
         {
            const auto ue = GetPointerS<const unary_expr>(ce->fn);
            fd = GetPointerS<const function_decl>(ue->op);
         }
         else if(ce->fn->get_kind() == obj_type_ref_K)
         {
            const auto temp_node = find_obj_type_ref_function(ce->fn);
            fd = GetPointerS<const function_decl>(temp_node);
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
      case tree_list_K:
      {
         auto tl = GetPointerS<const tree_list>(tn);
         std::list<const tree_list*> tl_list;
         do
         {
            tl_list.push_back(tl);
            tl = tl->chan ? GetPointer<const tree_list>(tl->chan) : nullptr;
         } while(tl);
         for(const auto tl_current0 : tl_list)
         {
            required.emplace_back(tl_current0->valu->index, 0);
         }
         break;
      }
      case field_decl_K:
      {
         const auto fd = GetPointerS<const field_decl>(tn);
         THROW_ASSERT(GetPointerS<const integer_cst>(fd->bpos),
                      "non-constant field offset (variable lenght object) currently not supported: " +
                          STR(fd->bpos->index));
         const auto ull_value = GetConstValue(fd->bpos);
         THROW_ASSERT(ull_value >= 0, "");
         required.emplace_back(0, static_cast<unsigned int>(ull_value / 8)); /// bpos has an offset in bits
         if(ull_value % 8 != 0)
         {
            THROW_ERROR("bitfields are not yet supported: " + fd->ToString());
         }
         break;
      }
      case gimple_asm_K:
      {
         const auto ga = GetPointerS<const gimple_asm>(tn);
         if(ga->in)
         {
            get_required_values(required, ga->in);
         }
         break;
      }
      case array_range_ref_K:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case const_decl_K:
      case function_decl_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case tree_vec_K:
      case type_decl_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case error_mark_K:
      {
         THROW_ERROR("Operation not yet supported: " + std::string(tn->get_kind_text()));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
}

bool tree_helper::LastStatement(const tree_nodeConstRef& statement)
{
   switch(statement->get_kind())
   {
      case gimple_asm_K:
      case gimple_assign_K:
      case gimple_bind_K:
      case gimple_call_K:
      case gimple_label_K:
      case gimple_nop_K:
      case gimple_predict_K:
      case gimple_resx_K:
      {
         return false;
      }
      case gimple_cond_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_multi_way_if_K:
      case gimple_phi_K:
      case gimple_pragma_K:
      case gimple_return_K:
      case gimple_switch_K:
      case gimple_while_K:
      {
         return true;
      }
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      case aggr_init_expr_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case lut_expr_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref461_K:
      case target_mem_ref_K:
      case tree_list_K:
      case tree_vec_K:
      default:
      {
         THROW_UNREACHABLE("Unexpected statement: " + statement->ToString());
         break;
      }
   }
   return true;
}

size_t tree_helper::GetFunctionSize(const tree_managerConstRef& TM, const unsigned int function_index)
{
   const auto tn = TM->GetTreeNode(function_index);
   const auto fd = GetPointer<const function_decl>(tn);
   THROW_ASSERT(fd, tn->ToString());
   THROW_ASSERT(fd->body, "Function " + fd->ToString() + " is without body");
   const auto sl = GetPointer<const statement_list>(fd->body);
   size_t ret_value = 0;
   for(const auto& block : sl->list_of_bloc)
   {
      ret_value += block.second->CGetStmtList().size();
      ret_value += block.second->CGetPhiList().size();
   }
   return ret_value;
}

std::string tree_helper::get_asm_string(const tree_managerConstRef& TM, const unsigned int node_index)
{
   const auto tn = TM->GetTreeNode(node_index);
   const auto gasm = GetPointer<const gimple_asm>(tn);
   THROW_ASSERT(gasm, tn->ToString());
   return gasm->str;
}

bool tree_helper::IsStore(const tree_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data)
{
   if(tn->get_kind() != gimple_assign_K)
   {
      return false;
   }
   const auto ga = GetPointerS<const gimple_assign>(tn);
   const auto op0_kind = ga->op0->get_kind();
   auto store_candidate = op0_kind == bit_field_ref_K || op0_kind == component_ref_K || op0_kind == indirect_ref_K ||
                          op0_kind == misaligned_indirect_ref_K || op0_kind == mem_ref_K || op0_kind == array_ref_K ||
                          op0_kind == target_mem_ref_K || op0_kind == target_mem_ref461_K ||
                          fun_mem_data.count(ga->op0->index);
   if(op0_kind == realpart_expr_K || op0_kind == imagpart_expr_K)
   {
      const auto op = GetPointerS<const unary_expr>(ga->op0)->op;
      const auto code0 = op->get_kind();
      if(code0 == bit_field_ref_K || code0 == component_ref_K || code0 == indirect_ref_K ||
         code0 == misaligned_indirect_ref_K || code0 == mem_ref_K || code0 == array_ref_K ||
         code0 == target_mem_ref_K || code0 == target_mem_ref461_K)
      {
         store_candidate = true;
      }
      if(code0 == var_decl_K && fun_mem_data.count(op->index))
      {
         store_candidate = true;
      }
   }

   if(ga->op1->get_kind() == view_convert_expr_K)
   {
      const auto op0_type = CGetType(ga->op0);
      const auto vc = GetPointerS<const view_convert_expr>(ga->op1);
      if(op0_type->get_kind() == record_type_K || op0_type->get_kind() == union_type_K)
      {
         store_candidate = true;
      }
      const auto vc_op_type = CGetType(vc->op);
      if(vc_op_type->get_kind() == vector_type_K && op0_type->get_kind() == array_type_K)
      {
         store_candidate = true;
      }
   }
   return store_candidate;
}

bool tree_helper::IsLoad(const tree_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data)
{
   if(tn->get_kind() != gimple_assign_K)
   {
      return false;
   }
   const auto ga = GetPointerS<const gimple_assign>(tn);
   const auto op1_kind = ga->op1->get_kind();
   bool is_a_vector_bitfield = false;
   /// check for bit field ref of vector type
   if(op1_kind == bit_field_ref_K)
   {
      const auto bfr = GetPointerS<const bit_field_ref>(ga->op1);
      if(IsVectorType(bfr->op0))
      {
         is_a_vector_bitfield = true;
      }
   }
   auto load_candidate = (op1_kind == bit_field_ref_K && !is_a_vector_bitfield) || op1_kind == component_ref_K ||
                         op1_kind == indirect_ref_K || op1_kind == misaligned_indirect_ref_K || op1_kind == mem_ref_K ||
                         op1_kind == array_ref_K || op1_kind == target_mem_ref_K || op1_kind == target_mem_ref461_K ||
                         fun_mem_data.count(ga->op1->index);
   if(op1_kind == realpart_expr_K || op1_kind == imagpart_expr_K)
   {
      const auto op = GetPointerS<const unary_expr>(ga->op1)->op;
      const auto code1 = op->get_kind();
      if((code1 == bit_field_ref_K && !is_a_vector_bitfield) || code1 == component_ref_K || code1 == indirect_ref_K ||
         code1 == bit_field_ref_K || code1 == misaligned_indirect_ref_K || code1 == mem_ref_K || code1 == array_ref_K ||
         code1 == target_mem_ref_K || code1 == target_mem_ref461_K)
      {
         load_candidate = true;
      }
      if(code1 == var_decl_K && fun_mem_data.count(op->index))
      {
         load_candidate = true;
      }
   }

   if(op1_kind == view_convert_expr_K)
   {
      const auto vc = GetPointerS<const view_convert_expr>(ga->op1);
      const auto vc_op_type = CGetType(vc->op);
      if(vc_op_type->get_kind() == record_type_K || vc_op_type->get_kind() == union_type_K)
      {
         load_candidate = true;
      }
      const auto op0_type = CGetType(ga->op0);
      if(vc_op_type->get_kind() == array_type_K && op0_type->get_kind() == vector_type_K)
      {
         load_candidate = true;
      }
   }
   return load_candidate;
}

bool tree_helper::IsLut(const tree_nodeConstRef& tn)
{
   const auto ga = GetPointer<const gimple_assign>(tn);
   if(!ga)
   {
      return false;
   }
   const auto op1 = ga->op1;
   return op1->get_kind() == lut_expr_K;
}

bool tree_helper::has_omp_simd(const statement_list* sl)
{
   THROW_ASSERT(sl, "");
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
         const auto gp = GetPointer<const gimple_pragma>(stmt);
         if(gp && gp->scope && GetPointer<const omp_pragma>(gp->scope))
         {
            const auto sp = GetPointer<const omp_simd_pragma>(gp->directive);
            if(sp)
            {
               return true;
            }
         }
      }
   }
   return false;
}
