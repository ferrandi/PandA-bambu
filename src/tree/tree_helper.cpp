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
 * @file tree_helper.cpp
 * @brief This file collects some utility functions.
 *
 *
 * @author Katia Turati <turati@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

/// Header include
#include "tree_helper.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// STL include
#include <algorithm>
#include <utility>

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "var_pp_functor.hpp"

/// Utility include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for STR
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <set>

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

const std::set<std::string> tree_helper::SC_tmpl_class = {"sc_signal", "sc_in", "sc_out", "sc_inout", "sc_port", "sc_export"};

const std::set<std::string> tree_helper::SC_builtin_scalar_type = {"sc_int",   "sc_uint",   "sc_biguint",  "sc_bigint", "sc_lv",
                                                                   "sc_in_rv", "sc_out_rv", "sc_inout_rv", "sc_bv",     "sc_signal_rv"}; /*check "sc_fix","sc_ufix"  "sc_fifo", "sc_buffer", "sc_fixed", "sc_ufixed" */

const std::set<std::string> tree_helper::TLM_SC_builtin_scalar_type = {
    "tlm_blocking_put_if", "tlm_blocking_get_if", "tlm_nonblocking_get_if", "tlm_nonblocking_put_if",
    "tlm_transport_if" // TLM objects
};

int tree_helper::debug_level = 0;

tree_helper::tree_helper() = default;

tree_helper::~tree_helper() = default;

static std::string sign_reduce_bitstring(std::string bitstring, bool bitstring_is_signed)
{
   THROW_ASSERT(not bitstring.empty(), "");
   while(bitstring.size() > 1)
   {
      if(bitstring_is_signed)
      {
         if(bitstring.at(0) == 'X' or (bitstring.at(0) != 'U' and (bitstring.at(0) == bitstring.at(1) or bitstring.at(1) == 'X')))
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
         if(bitstring.at(0) == 'X' or bitstring.at(0) == '0')
         {
            bitstring = bitstring.substr(1);
         }
         else
         {
            break;
         }
      }
   }
   return bitstring;
}

unsigned int tree_helper::size(const tree_managerConstRef tm, unsigned int index)
{
   return Size(tm->get_tree_node_const(index));
}

unsigned int tree_helper::Size(const tree_nodeConstRef t)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Getting size of " + t->get_kind_text() + " " + STR(t->index) + ": " + t->ToString());
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
            const auto type = GET_NODE(sa->type);
            if(type->get_kind() == real_type_K)
            {
               return Size(GET_NODE(sa->type));
            }
            const bool signed_p = type->get_kind() == integer_type_K ? !(GetPointer<integer_type>(type)->unsigned_flag) : (type->get_kind() == enumeral_type_K ? !(GetPointer<enumeral_type>(type)->unsigned_flag) : false);
            const auto bv_test = sign_reduce_bitstring(sa->bit_values, signed_p);
            return_value = static_cast<unsigned int>(bv_test.size());
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
            long long max = get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(sa->max)));
            long long min = get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(sa->min)));
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
               min_it = get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->min)));
               max_it = get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->max)));
               unsigned_p = it->unsigned_flag;
            }
            else
            {
               const enumeral_type* it = GetPointer<enumeral_type>(type);
               min_it = get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->min)));
               max_it = get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->max)));
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
         return_value = static_cast<unsigned int>(get_integer_cst_value(ic));
         break;
      }
      case reference_type_K:
      {
         const auto* ic = GetPointer<const integer_cst>(GET_NODE(GetPointer<const reference_type>(t)->size));
         return_value = static_cast<unsigned int>(get_integer_cst_value(ic));
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
               return_value = static_cast<unsigned int>(get_integer_cst_value(ic));
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
         return_value = 8;
         break;
      }
      case enumeral_type_K:
      {
         const auto* et = GetPointer<const enumeral_type>(t);
         if(et->min && et->max && GET_NODE(et->min)->get_kind() == integer_cst_K && GET_NODE(et->max)->get_kind() == integer_cst_K)
         {
            long long max = get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(et->max)));
            long long min = get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(et->min)));
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
            return_value = static_cast<unsigned int>(get_integer_cst_value(ic));
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
         return_value = static_cast<unsigned int>(get_integer_cst_value(ic));
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
            return_value = static_cast<unsigned int>(get_integer_cst_value(ic));
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Size is " + STR(return_value));
   return return_value;
}

std::string tree_helper::name_type(const tree_managerConstRef tm, int unsigned index)
{
   const tree_nodeRef t = tm->get_tree_node_const(index);
   THROW_ASSERT(GetPointer<type_node>(t) || t->get_kind() == tree_list_K, std::string("expected a type_decl got ") + t->get_kind_text());

   switch(t->get_kind())
   {
      case pointer_type_K:
      {
         return "*" + name_type(tm, GET_INDEX_NODE((GetPointer<pointer_type>(t))->ptd));
      }
      case reference_type_K:
      {
         return "&" + name_type(tm, GET_INDEX_NODE((GetPointer<reference_type>(t))->refd));
      }
      case record_type_K:
      {
         auto* rect = GetPointer<record_type>(t);
         std::string nt;
         if(rect->name)
         {
            if(GET_NODE(rect->name)->get_kind() == type_decl_K)
            {
               auto* td = GetPointer<type_decl>(GET_NODE(rect->name));
               if(GET_NODE(td->name)->get_kind() == identifier_node_K)
               {
                  auto* idn = GetPointer<identifier_node>(GET_NODE(td->name));
                  nt = idn->strg;
               }
               else
               {
                  THROW_ERROR("unexpected record type pattern: " + STR(index));
               }
            }
            else if(GET_NODE(rect->name)->get_kind() == identifier_node_K)
            {
               auto* idn = GetPointer<identifier_node>(GET_NODE(rect->name));
               nt = "struct " + normalized_ID(idn->strg);
            }
            else
            {
               THROW_ERROR("unexpected record type pattern: " + STR(index));
            }
         }
         else
         {
            return "_unnamed_" + STR(index);
         }
         if(SC_tmpl_class.find(nt) == SC_tmpl_class.end())
         {
            return nt;
         }
         else if(rect->tmpl_args) /*the class has template parameters*/
         {
            auto* rtv = GetPointer<tree_vec>(GET_NODE(rect->tmpl_args));
            THROW_ASSERT(rtv->lngt == 1 || nt == "sc_port", "Expected just one element");
            return name_type(tm, GET_INDEX_NODE(rtv->list_of_op[0]));
         }
         else
         {
            THROW_ERROR("Unexpected template parameter pattern");
         }
         return ""; // unreachable code
      }
      case union_type_K:
      {
         auto* unt = GetPointer<union_type>(t);
         std::string nt;
         if(unt->name)
         {
            if(GET_NODE(unt->name)->get_kind() == type_decl_K)
            {
               auto* td = GetPointer<type_decl>(GET_NODE(unt->name));
               if(GET_NODE(td->name)->get_kind() == identifier_node_K)
               {
                  auto* idn = GetPointer<identifier_node>(GET_NODE(td->name));
                  nt = idn->strg;
               }
               else
               {
                  THROW_ERROR("unexpected record type pattern: " + STR(index));
               }
            }
            else if(GET_NODE(unt->name)->get_kind() == identifier_node_K)
            {
               auto* idn = GetPointer<identifier_node>(GET_NODE(unt->name));
               nt = "union " + idn->strg;
            }
            else
            {
               THROW_ERROR("unexpected record type pattern: " + STR(index));
            }
         }
         else
         {
            return "_unnamed_" + STR(index);
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
         auto* at = GetPointer<array_type>(t);
         std::string vec_size_string;
         if(at->domn)
         {
            tree_nodeRef domain = GET_NODE(at->domn);
            auto* it = GetPointer<integer_type>(domain);
            THROW_ASSERT(it, "expected an integer type as array domain");
            if(it->max)
            {
               auto* ic = GetPointer<integer_cst>(GET_NODE(it->max));
               if(ic)
               {
                  long long int vec_size = static_cast<unsigned int>(get_integer_cst_value(ic)) + 1;
                  vec_size_string = "[" + boost::lexical_cast<std::string>(vec_size) + "]";
               }
               else
               {
                  vec_size_string = "[]";
               }
            }
         }
         return name_type(tm, GET_INDEX_NODE(at->elts)) + vec_size_string;
      }
      case enumeral_type_K:
      {
         const enumeral_type* et = GetPointer<enumeral_type>(t);
         if(et->name)
         {
            if(GET_NODE(et->name)->get_kind() == type_decl_K)
            {
               const type_decl* td = GetPointer<type_decl>(GET_NODE(et->name));
               const identifier_node* in = GetPointer<identifier_node>(GET_NODE(td->name));
               return in->strg;
            }
            else if(GET_NODE(et->name)->get_kind() == identifier_node_K)
            {
               const identifier_node* in = GetPointer<identifier_node>(GET_NODE(et->name));
               return "enum " + in->strg;
            }
         }
         return "enum Internal_" + boost::lexical_cast<std::string>(index);
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
         auto* tnode = GetPointer<type_node>(t);
         if(not tnode->name)
         {
            if(t->get_kind() == integer_type_K)
            {
               return "int";
            }
            return boost::lexical_cast<std::string>(index);
         }
         if(GET_NODE(tnode->name)->get_kind() == type_decl_K)
         {
            auto* tdecl = GetPointer<type_decl>(GET_NODE(tnode->name));
            THROW_ASSERT(GET_NODE(tdecl->name)->get_kind() == identifier_node_K, "unexpected type name pattern");
            auto* idn = GetPointer<identifier_node>(GET_NODE(tdecl->name));
            return idn->strg;
         }
         else if(GET_NODE(tnode->name)->get_kind() == identifier_node_K)
         {
            auto* idn = GetPointer<identifier_node>(GET_NODE(tnode->name));
            return idn->strg;
         }
         else
         {
            THROW_UNREACHABLE(std::string("unexpected builtin type pattern ") + t->get_kind_text() + " " + boost::lexical_cast<std::string>(index));
         }
         break;
      }
      case function_type_K:
      {
         std::string retn = name_type(tm, GET_INDEX_NODE(GetPointer<function_type>(t)->retn));
         retn += "(*)(";
         if(GetPointer<function_type>(t)->prms)
         {
            retn += name_type(tm, GET_INDEX_NODE(GetPointer<function_type>(t)->prms));
         }
         retn += ")";
         return retn;
      }
      case method_type_K:
      {
         std::string retn = name_type(tm, GET_INDEX_NODE(GetPointer<method_type>(t)->retn));
         retn += "(*)(";
         retn += name_type(tm, GET_INDEX_NODE(GetPointer<method_type>(t)->prms));
         retn += ")";
         return retn;
      }
      case tree_list_K:
      {
         auto* tl = GetPointer<tree_list>(t);
         std::string retn;
         if(tl->valu)
         {
            retn = name_type(tm, GET_INDEX_NODE(tl->valu));
         }
         std::list<tree_list*> tl_list;
         while(tl->chan)
         {
            tl = GetPointer<tree_list>(GET_NODE(tl->chan));
            tl_list.push_back(tl);
         }
         for(auto valu : tl_list)
         {
            retn += "," + name_type(tm, GET_INDEX_NODE(valu->valu));
         }
         return retn;
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
         THROW_UNREACHABLE(std::string("unexpected type pattern ") + t->get_kind_text() + " " + boost::lexical_cast<std::string>(index));
         return "";
   }
   return "";
}

std::string tree_helper::name_tmpl(const tree_managerConstRef tm, const unsigned int index)
{
   const tree_nodeRef t = tm->get_tree_node_const(index);
   if(t->get_kind() == record_type_K)
   {
      auto* rect = GetPointer<record_type>(t);
      if(rect->tmpl_args) /*the class is a template*/
      {
         for(auto& list_of_fld : rect->list_of_flds)
         {
            if(GET_NODE(list_of_fld)->get_kind() == type_decl_K)
            {
               auto* td = GetPointer<type_decl>(GET_NODE(list_of_fld));
               if(GET_NODE(td->name)->get_kind() == identifier_node_K)
               {
                  auto* idn = GetPointer<identifier_node>(GET_NODE(td->name));
                  return (idn->strg);
               }
            }
         }
      }
   }
   return "";
}

std::string tree_helper::record_name(const tree_managerConstRef tm, const unsigned int index)
{
   const tree_nodeRef t = tm->get_tree_node_const(index);
   if(t->get_kind() == record_type_K)
   {
      auto* rect = GetPointer<record_type>(t);
      if(rect->name)
      {
         if(GET_NODE(rect->name)->get_kind() == type_decl_K)
         {
            auto* td = GetPointer<type_decl>(GET_NODE(rect->name));
            if(GET_NODE(td->name)->get_kind() == identifier_node_K)
            {
               auto* idn = GetPointer<identifier_node>(GET_NODE(td->name));
               return (idn->strg);
            }
         }
      }
   }
   return "";
}

std::string tree_helper::name_function(const tree_managerConstRef tm, const unsigned int index)
{
   const tree_nodeRef t = tm->get_tree_node_const(index);
   if(t->get_kind() == function_decl_K)
   {
      auto* fd = GetPointer<function_decl>(t);
      return print_function_name(tm, fd);
   }
   else
   {
      THROW_ERROR(std::string("Node not yet supported ") + t->get_kind_text());
   }
   return "";
}

void tree_helper::get_mangled_fname(const function_decl* fd, std::string& fname)
{
   if(fd->builtin_flag)
   {
      THROW_ASSERT(fd->name, "unexpected condition");
      THROW_ASSERT(GET_NODE(fd->name)->get_kind() == identifier_node_K, "unexpected condition");
      fname = tree_helper::normalized_ID(GetPointer<identifier_node>(GET_NODE(fd->name))->strg);
   }
   else if(fd->mngl)
   {
      THROW_ASSERT(GET_NODE(fd->mngl)->get_kind() == identifier_node_K, "unexpected condition");
      fname = tree_helper::normalized_ID(GetPointer<identifier_node>(GET_NODE(fd->mngl))->strg);
   }
   else if(fd->name)
   {
      THROW_ASSERT(GET_NODE(fd->name)->get_kind() == identifier_node_K, "unexpected condition");
      fname = tree_helper::normalized_ID(GetPointer<identifier_node>(GET_NODE(fd->name))->strg);
   }
   else
   {
      THROW_ERROR("unexpected condition");
   }
}

std::string tree_helper::print_function_name(const tree_managerConstRef TM, const function_decl* fd)
{
   tree_nodeRef name;
   if(fd->builtin_flag)
   {
      name = GET_NODE(fd->name);
   }
   else if(TM->is_CPP() && TM->is_top_function(fd))
   {
      name = GET_NODE(fd->name);
   }
   else if(fd->mngl)
   {
      name = GET_NODE(fd->mngl);
   }
   else
   {
      name = GET_NODE(fd->name);
   }
   std::string res;
   if(name->get_kind() == identifier_node_K)
   {
      auto* in = GetPointer<identifier_node>(name);
      if(in->operator_flag)
      {
         res = "operator ";
         for(const auto& attr : fd->list_attr)
         {
            if(attr == TreeVocabularyTokenTypes_TokenEnum::TOK_PUBLIC || attr == TreeVocabularyTokenTypes_TokenEnum::TOK_PRIVATE || attr == TreeVocabularyTokenTypes_TokenEnum::TOK_PROTECTED || attr == TreeVocabularyTokenTypes_TokenEnum::TOK_OPERATOR ||
               attr == TreeVocabularyTokenTypes_TokenEnum::TOK_MEMBER)
            {
               continue;
            }
            else if(attr == TreeVocabularyTokenTypes_TokenEnum::TOK_CONVERSION)
            {
               auto* ft = GetPointer<function_type>(GET_NODE(fd->type));
               if(ft)
               {
                  print_type(TM, GET_INDEX_NODE(ft->retn));
               }
               else
               {
                  auto* mt = GetPointer<method_type>(GET_NODE(fd->type));
                  print_type(TM, GET_INDEX_NODE(mt->retn));
               }
            }
            else if(attr == TreeVocabularyTokenTypes_TokenEnum::TOK_RSHIFT)
            {
               res = res + ">>";
            }
            else if(attr == TreeVocabularyTokenTypes_TokenEnum::TOK_LSHIFT)
            {
               res = res + "<<";
            }
            else if(attr == TreeVocabularyTokenTypes_TokenEnum::TOK_ASSIGN)
            {
               res = res + "=";
            }
            else
            {
               res = res + TI_getTokenName(attr);
            }
         }
         res = normalized_ID(res);
      }
      else
      {
         res = normalized_ID(in->strg);
      }
   }
   else
   {
      THROW_ERROR(std::string("Node not yet supported ") + name->get_kind_text());
   }
   //   if(fd && fd->undefined_flag && fd->builtin_flag && res.find("__builtin_") == std::string::npos)
   //      res = "__builtin_" + res;
   if(fd->builtin_flag && fd->body && !TM->is_top_function(fd))
   {
      res = "__internal_" + res;
   }
   return res;
}

std::tuple<std::string, unsigned int, unsigned int> tree_helper::get_definition(const tree_managerConstRef tm, const unsigned int index, bool& is_system)
{
   is_system = false;
   tree_nodeRef node = tm->get_tree_node_const(index);
   std::string include_name;
   unsigned int line_number = 0;
   unsigned int column_number = 0;
   if(GetPointer<type_node>(node))
   {
      auto* tn = GetPointer<type_node>(node);
      if(tn->name && GetPointer<decl_node>(GET_NODE(tn->name)))
      {
         node = GET_NODE(tn->name);
      }
      else
      {
         if(GetPointer<union_type>(node))
         {
            std::vector<tree_nodeRef> list_of_flds = GetPointer<union_type>(node)->list_of_flds;
            if(!list_of_flds.empty())
            {
               tree_nodeRef field = GET_NODE(list_of_flds[0]);
               if(GetPointer<decl_node>(field))
               {
                  auto* dn = GetPointer<decl_node>(field);
                  include_name = dn->include_name;
                  line_number = dn->line_number;
                  column_number = dn->column_number;
                  is_system = dn->operating_system_flag or dn->library_system_flag;
               }
            }
         }
         if(GetPointer<record_type>(node))
         {
            std::vector<tree_nodeRef> list_of_flds = GetPointer<record_type>(node)->list_of_flds;
            if(!list_of_flds.empty())
            {
               tree_nodeRef field = GET_NODE(list_of_flds[0]);
               if(GetPointer<decl_node>(field))
               {
                  auto* dn = GetPointer<decl_node>(field);
                  include_name = dn->include_name;
                  line_number = dn->line_number;
                  column_number = dn->column_number;
                  is_system = dn->operating_system_flag or dn->library_system_flag;
               }
            }
         }
      }
   }
   if(GetPointer<decl_node>(node))
   {
      auto* dn = GetPointer<decl_node>(node);
      include_name = dn->include_name;
      line_number = dn->line_number;
      column_number = dn->column_number;
      is_system = dn->operating_system_flag or dn->library_system_flag;
   }
   return std::tuple<std::string, unsigned int, unsigned int>(include_name, line_number, column_number);
}

void tree_helper::get_used_variables(bool first_level_only, const tree_nodeRef tRI, CustomUnorderedSet<unsigned int>& list_of_variable)
{
   if(!tRI)
   {
      return;
   }
   THROW_ASSERT(tRI->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   tree_nodeRef t = GET_NODE(tRI);
   switch(t->get_kind())
   {
      case result_decl_K: // tree_to_graph considers this object as particular type of variable
      {
         auto* rd = GetPointer<result_decl>(t);
         list_of_variable.insert(GET_INDEX_NODE(tRI));
         if(rd->init)
         {
            get_used_variables(first_level_only, rd->init, list_of_variable);
         }
         break;
      }
      case var_decl_K:
      {
         auto* vd = GetPointer<var_decl>(t);
         list_of_variable.insert(GET_INDEX_NODE(tRI));
         if(vd->init)
         {
            get_used_variables(first_level_only, vd->init, list_of_variable);
         }
         break;
      }
      case ssa_name_K:
      {
         auto* sn = GetPointer<ssa_name>(t);
         get_used_variables(first_level_only, sn->var, list_of_variable);
         list_of_variable.insert(GET_INDEX_NODE(tRI));
         if(sn->var)
         {
            auto* vd = GetPointer<var_decl>(GET_NODE(sn->var));
            if(vd && vd->init)
            {
               get_used_variables(first_level_only, vd->init, list_of_variable);
            }
         }
         break;
      }
      case parm_decl_K:
         list_of_variable.insert(GET_INDEX_NODE(tRI));
         break;
      case function_decl_K:
      {
         auto* fd = GetPointer<function_decl>(t);
         bool expand_p = !first_level_only;
         auto vend = fd->list_of_args.end();
         list_of_variable.insert(GET_INDEX_NODE(tRI));
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
         auto* sl = GetPointer<statement_list>(t);
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
         auto* tv = GetPointer<tree_vec>(t);
         auto end = tv->list_of_op.end();
         for(auto i = tv->list_of_op.begin(); i != end; ++i)
         {
            get_used_variables(first_level_only, *i, list_of_variable);
         }
      }
      break;
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(t);
         get_used_variables(first_level_only, gc->op0, list_of_variable);
      }
      break;
      case gimple_assign_K:
      {
         auto* me = GetPointer<gimple_assign>(t);
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
         auto* re = GetPointer<gimple_return>(t);
         if(re->op)
         {
            get_used_variables(first_level_only, re->op, list_of_variable);
         }
      }
      break;
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointer<unary_expr>(t);
         if(list_of_variable.find(GET_INDEX_NODE(ue->op)) != list_of_variable.end())
         {
            break;
         }
         get_used_variables(first_level_only, ue->op, list_of_variable);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(t);
         get_used_variables(first_level_only, be->op0, list_of_variable);
         get_used_variables(first_level_only, be->op1, list_of_variable);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         auto* tern = GetPointer<ternary_expr>(t);
         get_used_variables(first_level_only, tern->op0, list_of_variable);
         get_used_variables(first_level_only, tern->op1, list_of_variable);
         get_used_variables(first_level_only, tern->op2, list_of_variable);
         break;
      }
      /* quaternary expressions.*/
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* qe = GetPointer<quaternary_expr>(t);
         get_used_variables(first_level_only, qe->op0, list_of_variable);
         get_used_variables(first_level_only, qe->op1, list_of_variable);
         get_used_variables(first_level_only, qe->op2, list_of_variable);
         get_used_variables(first_level_only, qe->op3, list_of_variable);
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(t);
         get_used_variables(first_level_only, le->op0, list_of_variable);
         get_used_variables(first_level_only, le->op1, list_of_variable);
         if(le->op2)
            get_used_variables(first_level_only, le->op2, list_of_variable);
         if(le->op3)
            get_used_variables(first_level_only, le->op3, list_of_variable);
         if(le->op4)
            get_used_variables(first_level_only, le->op4, list_of_variable);
         if(le->op5)
            get_used_variables(first_level_only, le->op5, list_of_variable);
         if(le->op6)
            get_used_variables(first_level_only, le->op6, list_of_variable);
         if(le->op7)
            get_used_variables(first_level_only, le->op7, list_of_variable);
         if(le->op8)
            get_used_variables(first_level_only, le->op8, list_of_variable);
         break;
      }
      case gimple_switch_K:
      {
         auto* s = GetPointer<gimple_switch>(t);
         get_used_variables(first_level_only, s->op0, list_of_variable);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(t);
         for(auto cond : gmwi->list_of_cond)
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
         auto* co = GetPointer<constructor>(t);
         for(auto i = co->list_of_idx_valu.begin(); i != co->list_of_idx_valu.end(); ++i)
         {
            get_used_variables(first_level_only, i->second, list_of_variable);
         }
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         auto* ce = GetPointer<call_expr>(t);
         const std::vector<tree_nodeRef> args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            get_used_variables(first_level_only, *arg, list_of_variable);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointer<gimple_call>(t);
         const std::vector<tree_nodeRef> args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            get_used_variables(first_level_only, *arg, list_of_variable);
         }
         break;
      }
      case gimple_asm_K:
      {
         auto* ae = GetPointer<gimple_asm>(t);
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
         auto* tl = GetPointer<tree_list>(t);
         std::list<tree_list*> tl_list;
         do
         {
            tl_list.push_back(tl);
            tl = tl->chan ? GetPointer<tree_list>(GET_NODE(tl->chan)) : nullptr;
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
         const gimple_for* fe = GetPointer<gimple_for>(t);
         get_used_variables(first_level_only, fe->op0, list_of_variable);
         get_used_variables(first_level_only, fe->op1, list_of_variable);
         get_used_variables(first_level_only, fe->op2, list_of_variable);
         break;
      }
      case gimple_while_K:
      {
         const gimple_while* we = GetPointer<gimple_while>(t);
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

bool tree_helper::look_for_binfo_inheritance(binfo* b, const std::string& bcs)
{
   if(b)
   {
      if(b->type)
      {
         auto* rt = GetPointer<record_type>(GET_NODE(b->type));
         if(rt && rt->get_maybe_name() == bcs)
         {
            return true;
         }
      }
      for(unsigned int i = 0; i < b->get_baseinfo_size(); i++)
      {
         tree_nodeRef binf = b->get_base(i);
         auto* bnf = GetPointer<binfo>(GET_NODE(binf));
         bool found = look_for_binfo_inheritance(bnf, bcs);
         if(found)
         {
            return true;
         }
      }
   }
   return false;
}

tree_nodeRef tree_helper::find_obj_type_ref_function(const tree_nodeRef tn)
{
   const tree_nodeRef curr_tn = GET_NODE(tn);
   unsigned int ind = GET_INDEX_NODE(tn);
   auto* otr = GetPointer<obj_type_ref>(curr_tn);
   THROW_ASSERT(otr, "tree node is not an obj_type_ref");
   THROW_ASSERT(otr->type && otr->op1 && otr->op2, "obj_type_ref has missing fields");
   tree_nodeRef type;
   unsigned int function_type;

   auto* t_pt = GetPointer<pointer_type>(GET_NODE(otr->type));
   THROW_ASSERT(t_pt, "Expected a pointer_type");
   function_type = GET_INDEX_NODE(t_pt->ptd);
   THROW_ASSERT(GetPointer<method_type>(GET_NODE(t_pt->ptd)), "expected a method_type");
   type = GET_NODE(GetPointer<method_type>(GET_NODE(t_pt->ptd))->clas);
#if 0
   var_decl* vd = GetPointer<var_decl>(GET_NODE(otr->op1));
   if(vd)
      type = vd->type;
   else
   {
      parm_decl* pd = GetPointer<parm_decl>(GET_NODE(otr->op1));
      if(pd)
         type = pd->type;
      else
         THROW_ERROR(std::string("not supported case for obj_type_ref(") + boost::lexical_cast<std::string>(ind) + std::string(")"));
   }
#endif
   if(type)
   {
      auto* rt = GetPointer<record_type>(type);
#if 0
      pointer_type* pt = GetPointer<pointer_type>(GET_NODE(type));
      if(pt)
         rt = GetPointer<record_type>(GET_NODE(pt->ptd));
      else
      {
         reference_type * Rt = GetPointer<reference_type>(GET_NODE(type));
         if(Rt)
            rt = GetPointer<record_type>(GET_NODE(Rt->refd));
         else
            THROW_ERROR(std::string("not supported case for obj_type_ref(") + boost::lexical_cast<std::string>(ind) + std::string(")"));
      }
#endif
      if(rt)
      {
         for(auto x = rt->list_of_fncs.begin(); x != rt->list_of_fncs.end(); ++x)
         {
            auto* fd = GetPointer<function_decl>(GET_NODE(*x));
            if(fd && GET_INDEX_NODE(fd->type) == function_type)
            {
               return *x;
            }
         }
      }
      else
      {
         THROW_ERROR(std::string("not supported case for obj_type_ref(") + boost::lexical_cast<std::string>(ind) + std::string(")"));
      }
   }
   else
   {
      THROW_ERROR(std::string("not supported case for obj_type_ref(") + boost::lexical_cast<std::string>(ind) + std::string(")"));
   }
   THROW_ERROR(std::string("obj_type_ref Function not found (") + boost::lexical_cast<std::string>(ind) + std::string(")"));
   return tree_nodeRef();
}

bool tree_helper::is_system(const tree_managerConstRef TM, const unsigned int index)
{
   const tree_nodeRef curr_tn = TM->get_tree_node_const(index);
   if(GetPointer<decl_node>(curr_tn))
   {
      return GetPointer<decl_node>(curr_tn)->operating_system_flag or GetPointer<decl_node>(curr_tn)->library_system_flag;
   }
   if(GetPointer<type_node>(curr_tn))
   {
      return GetPointer<type_node>(curr_tn)->system_flag;
   }
   return false;
}

#if HAVE_BAMBU_BUILT
bool tree_helper::IsInLibbambu(const tree_managerConstRef TM, const unsigned int index)
{
   const tree_nodeConstRef curr_tn = TM->CGetTreeNode(index);
   if(GetPointer<const decl_node>(curr_tn) and GetPointer<const decl_node>(curr_tn)->libbambu_flag)
   {
      return true;
   }
   if(GetPointer<const type_node>(curr_tn) and GetPointer<const type_node>(curr_tn)->libbambu_flag)
   {
      return true;
   }
   return false;
}
#endif

const CustomUnorderedSet<unsigned int> tree_helper::GetTypesToBeDeclaredBefore(const tree_managerConstRef TM, const unsigned int index, const bool without_transformation)
{
   return RecursiveGetTypesToBeDeclared(TM, index, false, without_transformation, true);
}

const CustomUnorderedSet<unsigned int> tree_helper::GetTypesToBeDeclaredAfter(const tree_managerConstRef TM, const unsigned int index, const bool without_transformation)
{
   return RecursiveGetTypesToBeDeclared(TM, index, false, without_transformation, false);
}

const CustomUnorderedSet<unsigned int> tree_helper::RecursiveGetTypesToBeDeclared(const tree_managerConstRef TM, const unsigned int index, const bool recursion, const bool without_transformation, const bool before)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR("-->Getting types to be declared ") + (before ? "before" : "after") + " (" + STR(index) + ") " + STR(TM->CGetTreeNode(index)));
   CustomUnorderedSet<unsigned int> returned_types;
   const tree_nodeRef type = TM->get_tree_node_const(index);
   switch(type->get_kind())
   {
      case pointer_type_K:
      {
         if(before)
         {
            returned_types = RecursiveGetTypesToBeDeclared(TM, get_pointed_type(TM, index), true, without_transformation, true);
         }
         break;
      }
      case reference_type_K:
      {
         if(before)
         {
            auto* rt = GetPointer<reference_type>(type);
            returned_types = RecursiveGetTypesToBeDeclared(TM, GET_INDEX_NODE(rt->refd), true, without_transformation, true);
         }
         break;
      }
      case array_type_K:
      case vector_type_K:
      {
         if(before)
         {
            const type_node* tn = GetPointer<type_node>(type);
            if(recursion and tn->name and GET_NODE(tn->name)->get_kind() == type_decl_K)
            {
               returned_types.insert(index);
            }
            else
            {
               returned_types = RecursiveGetTypesToBeDeclared(TM, GetElements(TM, index), true, without_transformation, before);
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
               returned_types.insert(index);
            }
         }
         else
         {
            const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
            if(rt->unql and (GetPointer<record_type>(GET_NODE(rt->unql))->name or without_transformation))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Record type with named unqualified");
               if(((not before) and (not tree_helper::IsAligned(TM, index))) or (before and tree_helper::IsAligned(TM, index)))
               {
                  returned_types.insert(GET_INDEX_NODE(rt->unql));
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Record type without named unqualified");
               const auto field_types = CGetFieldTypes(TM->CGetTreeNode(index));
               for(const auto& field_type : field_types)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering field type (" + STR(field_type->index) + ") " + STR(field_type));
                  bool pointer_to_unnamed_structure = [&]() {
                     if(not is_a_pointer(TM, field_type->index))
                     {
                        return false;
                     }
                     const auto pointed_type_index = get_pointed_type(TM, field_type->index);
                     const auto pointed_type = TM->CGetTreeNode(pointed_type_index);
                     if(GetPointer<const record_type>(pointed_type) and GET_CONST_NODE(GetPointer<const record_type>(pointed_type)->name)->get_kind() != type_decl_K)
                     {
                        return true;
                     }
                     if(GetPointer<const union_type>(pointed_type) and GET_CONST_NODE(GetPointer<const union_type>(pointed_type)->name)->get_kind() != type_decl_K)
                     {
                        return true;
                     }
                     return false;
                  }();
                  /// Non pointer fields must be declared before structs, pointer fields can be declared after; in some cases they must be declared after (circular dependencies)
                  if(before)
                  {
                     if(not is_a_pointer(TM, field_type->index) or not pointer_to_unnamed_structure)
                     {
                        const CustomUnorderedSet<unsigned int> local_types = RecursiveGetTypesToBeDeclared(TM, field_type->index, true, without_transformation, true);
                        returned_types.insert(local_types.begin(), local_types.end());
                     }
                  }
                  else
                  {
                     if(pointer_to_unnamed_structure)
                     {
                        /// Here true is correct
                        const CustomUnorderedSet<unsigned int> local_types = RecursiveGetTypesToBeDeclared(TM, field_type->index, true, without_transformation, true);
                        returned_types.insert(local_types.begin(), local_types.end());
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered field type (" + STR(field_type->index) + ") " + STR(field_type));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
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
               returned_types.insert(index);
            }
         }
         else
         {
            const union_type* ut = GetPointer<union_type>(TM->get_tree_node_const(index));
            if(ut->unql and (GetPointer<union_type>(GET_NODE(ut->unql))->name or without_transformation))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Union type with named unqualified");
               if(((not before) and (not tree_helper::IsAligned(TM, index))) or (before and tree_helper::IsAligned(TM, index)))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inserting " + STR(ut->unql) + " in the types to be declared");
                  returned_types.insert(GET_INDEX_NODE(ut->unql));
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Union type without named unqualified");
               const auto field_types = CGetFieldTypes(TM->CGetTreeNode(index));
               for(const auto& field_type : field_types)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering field type (" + STR(field_type->index) + ") " + STR(field_type));
                  bool pointer_to_unnamed_structure = [&]() {
                     if(not is_a_pointer(TM, field_type->index))
                     {
                        return false;
                     }
                     const auto pointed_type_index = get_pointed_type(TM, field_type->index);
                     const auto pointed_type = TM->CGetTreeNode(pointed_type_index);
                     if(GetPointer<const record_type>(pointed_type) and GET_CONST_NODE(GetPointer<const record_type>(pointed_type)->name)->get_kind() != type_decl_K)
                     {
                        return true;
                     }
                     if(GetPointer<const union_type>(pointed_type) and GET_CONST_NODE(GetPointer<const union_type>(pointed_type)->name)->get_kind() != type_decl_K)
                     {
                        return true;
                     }
                     return false;
                  }();
                  /// Non pointer fields must be declared before structs, pointer fields can be declared after; in some cases they must be declared after (circular dependencies)
                  if(before)
                  {
                     if(not is_a_pointer(TM, field_type->index) or not pointer_to_unnamed_structure)
                     {
                        const CustomUnorderedSet<unsigned int> local_types = RecursiveGetTypesToBeDeclared(TM, field_type->index, true, without_transformation, true);
                        returned_types.insert(local_types.begin(), local_types.end());
                     }
                  }
                  else
                  {
                     if(pointer_to_unnamed_structure)
                     {
                        /// Here true is correct
                        const CustomUnorderedSet<unsigned int> local_types = RecursiveGetTypesToBeDeclared(TM, field_type->index, true, without_transformation, true);
                        returned_types.insert(local_types.begin(), local_types.end());
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered field type (" + STR(field_type->index) + ") " + STR(field_type));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
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
               returned_types.insert(index);
            }
         }
         else
         {
            const enumeral_type* et = GetPointer<enumeral_type>(TM->get_tree_node_const(index));
            if(et->unql and GetPointer<enumeral_type>(GET_NODE(et->unql))->name)
            {
               if(before)
               {
                  returned_types.insert(GET_INDEX_NODE(et->unql));
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
            const type_node* tn = GetPointer<type_node>(type);
            if(tn->name and GET_NODE(tn->name)->get_kind() == type_decl_K)
            {
               returned_types.insert(index);
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
            const tree_nodeRef return_type = GetFunctionReturnType(TM->get_tree_node_const(index));
            if(return_type)
            {
               const CustomUnorderedSet<unsigned int> return_types = RecursiveGetTypesToBeDeclared(TM, return_type->index, true, without_transformation, true);
               returned_types.insert(return_types.begin(), return_types.end());
            }
            std::list<unsigned int> parameters;
            get_parameter_types(TM, index, parameters);
            std::list<unsigned int>::const_iterator parameter, parameter_end = parameters.end();
            for(parameter = parameters.begin(); parameter != parameter_end; ++parameter)
            {
               const CustomUnorderedSet<unsigned int> local_types = RecursiveGetTypesToBeDeclared(TM, *parameter, true, without_transformation, true);
               returned_types.insert(local_types.begin(), local_types.end());
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
         THROW_UNREACHABLE("Unexpected node: " + boost::lexical_cast<std::string>(index));
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR("<--Got types to be declared ") + (before ? "before" : "after") + " (" + STR(index) + ") " + STR(TM->CGetTreeNode(index)));
   return returned_types;
}

unsigned int tree_helper::GetRealType(const tree_managerConstRef& TM, unsigned int index)
{
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt and rt->unql and (not rt->name or GET_NODE(rt->name)->get_kind() == identifier_node_K))
   {
      return GET_INDEX_NODE(rt->unql);
   }
   const union_type* ut = GetPointer<union_type>(TM->get_tree_node_const(index));
   if(ut and ut->unql and (not ut->name or GET_NODE(ut->name)->get_kind() == identifier_node_K))
   {
      return GET_INDEX_NODE(ut->unql);
   }
   const enumeral_type* et = GetPointer<enumeral_type>(TM->get_tree_node_const(index));
   if(et and et->unql and (not et->name or GET_NODE(et->name)->get_kind() == identifier_node_K))
   {
      return GET_INDEX_NODE(et->unql);
   }
   return index;
}

unsigned int tree_helper::get_type_index(const tree_managerConstRef TM, const unsigned int index, long long int& vec_size, bool& is_a_pointer, bool& is_a_function)
{
   is_a_pointer = false;
   is_a_function = false;
   vec_size = 0;
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   unsigned int type_index;
   if(GetPointer<type_node>(T))
   {
      type_index = index;
      Type = T;
   }
   else
   {
      // decl_node * dn = GetPointer<decl_node>(T);
      // THROW_ASSERT(dn, "expected a declaration object");
      type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index " + boost::lexical_cast<std::string>(index));
      // type_index = GET_INDEX_NODE(dn->type);
   }
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
      auto* at = GetPointer<array_type>(Type);
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

tree_nodeRef tree_helper::GetFunctionReturnType(const tree_nodeRef function)
{
   tree_nodeRef fun_type;
   switch(function->get_kind())
   {
      case(function_decl_K):
      {
         const function_decl* fd = GetPointer<function_decl>(function);
         fun_type = GET_NODE(fd->type);
         break;
      }
      case method_type_K:
      case(function_type_K):
      {
         fun_type = function;
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
         THROW_UNREACHABLE("Not supported tree node type " + function->get_kind_text());
      }
   }
   if(fun_type->get_kind() == function_type_K)
   {
      const function_type* ft = GetPointer<function_type>(fun_type);
      THROW_ASSERT(ft, "NodeId is not related to a valid function type");
      if(GET_NODE(ft->retn)->get_kind() != void_type_K)
      {
         return GET_NODE(ft->retn);
      }
      else
      {
         return tree_nodeRef();
      }
   }
   else if(fun_type->get_kind() == method_type_K)
   {
      const method_type* mt = GetPointer<method_type>(fun_type);
      THROW_ASSERT(mt, "NodeId is not related to a valid function type");
      if(GET_NODE(mt->retn)->get_kind() != void_type_K)
      {
         return GET_NODE(mt->retn);
      }
      else
      {
         return tree_nodeRef();
      }
   }

   else
   {
      return tree_nodeRef();
   }
}

unsigned int tree_helper::get_pointed_type(const tree_managerConstRef TM, const int unsigned index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   switch(T->get_kind())
   {
      case(pointer_type_K):
      {
         const pointer_type* pt = GetPointer<pointer_type>(T);
         return GET_INDEX_NODE(pt->ptd);
      }
      case reference_type_K:
      {
         const reference_type* rt = GetPointer<reference_type>(T);
         return GET_INDEX_NODE(rt->refd);
      }
      case(function_type_K):
      {
         const function_type* ft = GetPointer<function_type>(T);
         return get_pointed_type(TM, GET_INDEX_NODE(ft->retn));
      }
      case(method_type_K):
      {
         const method_type* mt = GetPointer<method_type>(T);
         return get_pointed_type(TM, GET_INDEX_NODE(mt->retn));
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
         THROW_ASSERT(false, "Index " + boost::lexical_cast<std::string>(index) + " does not correspond to a pointer type");
      }
   }
   return 0;
}

const tree_nodeConstRef tree_helper::CGetPointedType(const tree_nodeConstRef pointer)
{
   switch(pointer->get_kind())
   {
      case(pointer_type_K):
      {
         const auto* pt = GetPointer<const pointer_type>(pointer);
         return GET_NODE(pt->ptd);
      }
      case(reference_type_K):
      {
         const auto* pt = GetPointer<const reference_type>(pointer);
         return GET_NODE(pt->refd);
      }
      case(function_type_K):
      {
         const auto* ft = GetPointer<const function_type>(pointer);
         return CGetPointedType(GET_NODE(ft->retn));
      }
      case method_type_K:
      {
         const auto* mt = GetPointer<const method_type>(pointer);
         return CGetPointedType(GET_NODE(mt->retn));
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
         THROW_UNREACHABLE("Index " + boost::lexical_cast<std::string>(pointer->index) + " does not correspond to a pointer type");
      }
   }
   return tree_nodeConstRef();
}

unsigned int tree_helper::GetElements(const tree_managerConstRef TM, const unsigned int index)
{
   return CGetElements(TM->get_tree_node_const(index))->index;
}

const tree_nodeConstRef tree_helper::CGetElements(const tree_nodeConstRef type)
{
   const auto* at = GetPointer<const array_type>(type);
   if(at)
   {
      return GET_CONST_NODE(at->elts);
   }
   const auto* vt = GetPointer<const vector_type>(type);
   if(vt)
   {
      return GET_CONST_NODE(vt->elts);
   }
   THROW_UNREACHABLE("Tree node of type " + type->get_kind_text());
   return tree_nodeConstRef();
}

std::string tree_helper::get_type_name(const tree_managerConstRef TM, const unsigned int index)
{
   unsigned int type_index;
   tree_nodeRef type = get_type_node(TM->get_tree_node_const(index), type_index);
   THROW_ASSERT(GetPointer<type_node>(type), "Node type not type_node");
   auto* tn = GetPointer<type_node>(type);
   if(tn->name)
   {
      tree_nodeRef name;
      if(GET_NODE(tn->name)->get_kind() == type_decl_K)
      {
         name = GetPointer<type_decl>(GET_NODE(tn->name))->name;
         if(!name)
         {
            return "Internal_" + boost::lexical_cast<std::string>(type_index);
         }
      }
      else
      {
         name = tn->name;
      }
      THROW_ASSERT(name && GET_NODE(name)->get_kind() == identifier_node_K, "Not an identifier node:" + STR(index));
      auto* id = GetPointer<identifier_node>(GET_NODE(name));
      return id->strg;
   }
   else
   {
      return "Internal_" + boost::lexical_cast<std::string>(type_index);
   }
}

void tree_helper::get_parameter_types(const tree_managerConstRef TM, const unsigned int index, std::list<unsigned int>& params)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
   }
   THROW_ASSERT(GetPointer<function_type>(Type) || GetPointer<method_type>(Type), "Index " + boost::lexical_cast<std::string>(index) + " does not correspond to a function type");
   if(Type->get_kind() == function_type_K)
   {
      if(GetPointer<function_type>(Type)->prms)
      {
         auto* tl = GetPointer<tree_list>(GET_NODE(GetPointer<function_type>(Type)->prms));
         params.push_back(GET_INDEX_NODE(tl->valu));
         while(tl->chan)
         {
            tl = GetPointer<tree_list>(GET_NODE(tl->chan));
            params.push_back(GET_INDEX_NODE(tl->valu));
         }
      }
   }
   else
   {
      if(GetPointer<method_type>(Type)->prms)
      {
         auto* tl = GetPointer<tree_list>(GET_NODE(GetPointer<method_type>(Type)->prms));
         params.push_back(GET_INDEX_NODE(tl->valu));
         while(tl->chan)
         {
            tl = GetPointer<tree_list>(GET_NODE(tl->chan));
            params.push_back(GET_INDEX_NODE(tl->valu));
         }
      }
   }
}

unsigned int tree_helper::get_type_index(const tree_managerConstRef TM, const unsigned int index)
{
   bool is_a_pointer;
   bool is_a_function;
   long long int vec_size;
   return get_type_index(TM, index, vec_size, is_a_pointer, is_a_function);
}

const std::vector<tree_nodeConstRef> tree_helper::CGetFieldTypes(const tree_nodeConstRef type)
{
   std::vector<tree_nodeConstRef> ret;
   if(type->get_kind() == record_type_K)
   {
      const auto* rt = GetPointer<const record_type>(type);
      for(const auto& list_of_fld : rt->list_of_flds)
      {
         if(GET_CONST_NODE(list_of_fld)->get_kind() == type_decl_K)
         {
            continue;
         }
         if(GET_CONST_NODE(list_of_fld)->get_kind() == function_decl_K)
         {
            continue;
         }
         ret.push_back(CGetType(GET_CONST_NODE(list_of_fld)));
      }
   }
   else if(type->get_kind() == union_type_K)
   {
      const auto* ut = GetPointer<const union_type>(type);
      for(const auto& list_of_fld : ut->list_of_flds)
      {
         ret.push_back(CGetType(GET_CONST_NODE(list_of_fld)));
      }
   }
   else
   {
      THROW_UNREACHABLE("Asking fields type of not a type. Tree node is " + type->ToString());
   }
   return ret;
}

unsigned int tree_helper::get_field_idx(const tree_managerConstRef TM, const unsigned int index, unsigned int idx)
{
   tree_nodeRef node = TM->get_tree_node_const(index);
   THROW_ASSERT(GetPointer<record_type>(node) || GetPointer<union_type>(node), "expected record or union type");
   auto* rt = GetPointer<record_type>(node);
   auto* ut = GetPointer<union_type>(node);
   if(rt)
   {
      THROW_ASSERT(idx < rt->list_of_flds.size(), "unexpected index for list of fields");
      return GET_INDEX_NODE(rt->list_of_flds[idx]);
   }
   else
   {
      THROW_ASSERT(idx < ut->list_of_flds.size(), "unexpected index for list of fields");
      return GET_INDEX_NODE(ut->list_of_flds[idx]);
   }
}

unsigned int tree_helper::local_return_index = 0;

/// FIXME to be removed after complete substitution with GetType
tree_nodeRef tree_helper::get_type_node(const tree_nodeRef& node, unsigned int& return_index)
{
   switch(node->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         auto* ce = GetPointer<call_expr>(node);
         return_index = GET_INDEX_NODE(ce->type);
         return GET_NODE(ce->type);
         /*otherwise fall through*/
      }
      case gimple_asm_K:
      case gimple_bind_K:
      case gimple_return_K:
      case gimple_switch_K:
      case gimple_label_K:
      case gimple_goto_K:
      case gimple_nop_K:
      case gimple_resx_K:
      case gimple_call_K:
      case gimple_cond_K:
      case gimple_multi_way_if_K:
      case gimple_pragma_K:
      {
         return_index = 0;
         return tree_nodeRef();
      }
      case lut_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* en = GetPointer<expr_node>(node);
         THROW_ASSERT(en && en->type, std::string("this NODE does not have a type: ") + node->get_kind_text());
         return_index = GET_INDEX_NODE(en->type);
         return GET_NODE(en->type);
      }
      case gimple_phi_K:
      {
         const auto gp = GetPointer<const gimple_phi>(node);
         return get_type_node(GET_NODE(gp->res), return_index);
      }
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(node);
         return get_type_node(GET_NODE(gm->op0), return_index);
      }
      case integer_cst_K:
      {
         auto* ic = GetPointer<integer_cst>(node);
         return_index = GET_INDEX_NODE(ic->type);
         return GET_NODE(ic->type);
      }
      case real_cst_K:
      {
         auto* rc = GetPointer<real_cst>(node);
         return_index = GET_INDEX_NODE(rc->type);
         return GET_NODE(rc->type);
      }
      case string_cst_K:
      {
         auto* sc = GetPointer<string_cst>(node);
         return_index = sc->type ? GET_INDEX_NODE(sc->type) : 0;
         return sc->type ? GET_NODE(sc->type) : node;
      }
      case vector_cst_K:
      {
         auto* vc = GetPointer<vector_cst>(node);
         return_index = vc->type ? GET_INDEX_NODE(vc->type) : 0;
         return vc->type ? GET_NODE(vc->type) : node;
      }
      case complex_cst_K:
      {
         auto* cc = GetPointer<complex_cst>(node);
         return_index = cc->type ? GET_INDEX_NODE(cc->type) : 0;
         return cc->type ? GET_NODE(cc->type) : node;
      }
      case constructor_K:
      {
         auto* c = GetPointer<constructor>(node);
         if(c->type)
         {
            return_index = GET_INDEX_NODE(c->type);
            return GET_NODE(c->type);
         }
         else
         {
            return_index = 0;
            return tree_nodeRef();
         }
      }
      case CASE_DECL_NODES:
      {
         auto* dn = GetPointer<decl_node>(node);
         return_index = GET_INDEX_NODE(dn->type);
         return GET_NODE(dn->type);
      }
      case ssa_name_K:
      {
         auto* sa = GetPointer<ssa_name>(node);
         if(sa->var)
         {
            return get_type_node(GET_NODE(sa->var), return_index);
         }
         else
         {
            THROW_ASSERT(sa->type, "ssa without type nor var: " + STR(sa->index));
            return_index = GET_INDEX_NODE(sa->type);
            return GET_NODE(sa->type);
         }
      }
      case target_mem_ref_K:
      {
         auto* tm = GetPointer<target_mem_ref>(node);
         return get_type_node(GET_NODE(tm->orig), return_index);
      }
      case target_mem_ref461_K:
      {
         auto* tm = GetPointer<target_mem_ref461>(node);
         return_index = GET_INDEX_NODE(tm->type);
         return GET_NODE(tm->type);
      }
      case gimple_for_K:
      case gimple_while_K:
      {
         const gimple_while* gw = GetPointer<gimple_while>(node);
         return get_type_node(GET_NODE(gw->op0), return_index);
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case gimple_predict_K:
      case identifier_node_K:
      case statement_list_K:
      case target_expr_K:
      case tree_list_K:
      case tree_vec_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      case void_cst_K:
      case error_mark_K:
      default:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, std::string("Node not yet supported ") + node->get_kind_text());
      }
   }
   return node;
}

const tree_nodeConstRef tree_helper::CGetType(const tree_nodeConstRef node)
{
   switch(node->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto* ce = GetPointer<const call_expr>(node);
         return GET_NODE(ce->type);
         /*otherwise fall through*/
      }
      case gimple_asm_K:
      case gimple_bind_K:
      case gimple_return_K:
      case gimple_for_K:
      case gimple_phi_K:
      case gimple_resx_K:
      case gimple_while_K:
      case gimple_switch_K:
      case gimple_label_K:
      case gimple_goto_K:
      case gimple_nop_K:
      case gimple_call_K:
      case gimple_cond_K:
      case gimple_multi_way_if_K:
      case gimple_pragma_K:
      {
         return tree_nodeRef();
      }
      case lut_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto* en = GetPointer<const expr_node>(node);
         THROW_ASSERT(en && en->type, std::string("this NODE does not have a type: ") + node->get_kind_text());
         return GET_NODE(en->type);
      }
      case gimple_assign_K:
      {
         const auto* gm = GetPointer<const gimple_assign>(node);
         return CGetType(GET_NODE(gm->op0));
      }
      case integer_cst_K:
      {
         const auto* ic = GetPointer<const integer_cst>(node);
         return GET_NODE(ic->type);
      }
      case real_cst_K:
      {
         const auto* rc = GetPointer<const real_cst>(node);
         return GET_NODE(rc->type);
      }
      case string_cst_K:
      {
         const auto* sc = GetPointer<const string_cst>(node);
         return sc->type ? GET_NODE(sc->type) : node;
      }
      case vector_cst_K:
      {
         const auto* vc = GetPointer<const vector_cst>(node);
         return vc->type ? GET_NODE(vc->type) : node;
      }
      case complex_cst_K:
      {
         const auto* cc = GetPointer<const complex_cst>(node);
         return cc->type ? GET_NODE(cc->type) : node;
      }
      case constructor_K:
      {
         const auto* c = GetPointer<const constructor>(node);
         if(c->type)
         {
            return GET_NODE(c->type);
         }
         else
         {
            return tree_nodeRef();
         }
      }
      case const_decl_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case result_decl_K:
      case translation_unit_decl_K:
      case using_decl_K:
      case type_decl_K:
      case parm_decl_K:
      case var_decl_K:
      case template_decl_K:
      {
         const auto* dn = GetPointer<const decl_node>(node);
         return GET_NODE(dn->type);
      }
      case ssa_name_K:
      {
         const auto* sa = GetPointer<const ssa_name>(node);
         if(sa->var)
         {
            return CGetType(GET_NODE(sa->var));
         }
         else
         {
            return GET_NODE(sa->type);
         }
      }
      case target_mem_ref_K:
      {
         const auto* tm = GetPointer<const target_mem_ref>(node);
         return CGetType(GET_NODE(tm->orig));
      }
      case target_mem_ref461_K:
      {
         const auto* tm = GetPointer<const target_mem_ref461>(node);
         return GET_NODE(tm->type);
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case gimple_predict_K:
      case identifier_node_K:
      case statement_list_K:
      case target_expr_K:
      case tree_list_K:
      case tree_vec_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      case void_cst_K:
      case error_mark_K:
      default:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, std::string("Node not yet supported ") + node->get_kind_text());
      }
   }
   return node;
}

bool tree_helper::is_an_enum(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
   }
   return Type->get_kind() == enumeral_type_K;
}

bool tree_helper::is_a_struct(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      // decl_node * dn = GetPointer<decl_node>(T);
      // THROW_ASSERT(dn, "expected a declaration object");
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
      // type_index = GET_INDEX_NODE(dn->type);
   }
   return Type->get_kind() == record_type_K;
}

bool tree_helper::is_an_union(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      // decl_node * dn = GetPointer<decl_node>(T);
      // THROW_ASSERT(dn, "expected a declaration object");
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
      // type_index = GET_INDEX_NODE(dn->type);
   }
   return Type->get_kind() == union_type_K;
}

bool tree_helper::is_a_complex(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      // decl_node * dn = GetPointer<decl_node>(T);
      // THROW_ASSERT(dn, "expected a declaration object");
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
      // type_index = GET_INDEX_NODE(dn->type);
   }
   return Type->get_kind() == complex_type_K;
}

// static void getBuiltinFieldTypes(const tree_nodeConstRef& type, std::list<tree_nodeConstRef> &listOfTypes, CustomUnorderedSet<unsigned int> &already_visited)
//{
//   if(already_visited.find(type->index) != already_visited.end())
//      return;
//   already_visited.insert(type->index);
//   if(type->get_kind() == record_type_K)
//   {
//      const auto* rt = GetPointer<const record_type>(type);
//      for(const auto& fld : rt->list_of_flds)
//      {
//         if(GET_CONST_NODE(fld)->get_kind() == type_decl_K)
//         {
//            continue;
//         }
//         if(GET_CONST_NODE(fld)->get_kind() == function_decl_K)
//         {
//            continue;
//         }
//         auto fdType = tree_helper::CGetType(GET_CONST_NODE(fld));
//         if(fdType->get_kind() == record_type_K || fdType->get_kind() == union_type_K || fdType->get_kind() == array_type_K || fdType->get_kind() == vector_type_K)
//            getBuiltinFieldTypes(fdType, listOfTypes, already_visited);
//         else
//            listOfTypes.push_back(fdType);
//      }
//   }
//   else if(type->get_kind() == union_type_K)
//   {
//      const auto* ut = GetPointer<const union_type>(type);
//      for(const auto& fld : ut->list_of_flds)
//      {
//         auto fdType = tree_helper::CGetType(GET_CONST_NODE(fld));
//         if(fdType->get_kind() == record_type_K || fdType->get_kind() == union_type_K || fdType->get_kind() == array_type_K || fdType->get_kind() == vector_type_K)
//            getBuiltinFieldTypes(fdType, listOfTypes, already_visited);
//         else
//            listOfTypes.push_back(fdType);
//      }
//   }
//   else if(type->get_kind() == array_type_K)
//   {
//      auto* at = GetPointer<const array_type>(type);
//      THROW_ASSERT(at->elts, "elements type expected");
//      getBuiltinFieldTypes(GET_NODE(at->elts), listOfTypes, already_visited);
//   }
//   else if(type->get_kind() == vector_type_K)
//   {
//      auto* vt = GetPointer<const vector_type>(type);
//      THROW_ASSERT(vt->elts, "elements type expected");
//      getBuiltinFieldTypes(GET_NODE(vt->elts), listOfTypes, already_visited);
//   }
//   else
//      listOfTypes.push_back(type);
//}

// static bool same_size_fields(const tree_nodeConstRef& t)
//{
//   std::list<tree_nodeConstRef> listOfTypes;
//   CustomUnorderedSet<unsigned int> already_visited;
//   getBuiltinFieldTypes(t, listOfTypes, already_visited);
//   auto sizeFlds = 0u;
//   for(auto fldType : listOfTypes)
//   {
//      if(!sizeFlds)
//         sizeFlds = tree_helper::Size(fldType);
//      else if(sizeFlds != tree_helper::Size(fldType))
//         return false;
//   }
//   return true;
//}

bool tree_helper::is_an_array(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      // decl_node * dn = GetPointer<decl_node>(T);
      // THROW_ASSERT(dn, "expected a declaration object");
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
      // type_index = GET_INDEX_NODE(dn->type);
   }
   if(Type->get_kind() == array_type_K)
   {
      return true;
   }
   else if(Type->get_kind() == record_type_K)
   {
      auto* rt = GetPointer<record_type>(Type);
      if(rt->list_of_flds.size() != 1)
      {
         return false;
      }
      auto fd = GET_NODE(rt->list_of_flds[0]);
      THROW_ASSERT(fd->get_kind() == field_decl_K, "expected a field_decl");
      auto at_node = GET_NODE(GetPointer<field_decl>(fd)->type);
      if(at_node->get_kind() == array_type_K)
      {
         return true;
      }
      else if(at_node->get_kind() == record_type_K)
      {
         return is_an_array(TM, GET_INDEX_NODE(GetPointer<field_decl>(fd)->type));
      }
      else
      {
         return false;
      }
      //      return same_size_fields(Type);
   }
   return false;
}

bool tree_helper::is_a_pointer(const tree_managerConstRef& TM, const unsigned int index)
{
   long long int vec_size;
   bool ret;
   bool is_a_function;
   tree_helper::get_type_index(TM, index, vec_size, ret, is_a_function);
   return ret;
}

bool tree_helper::is_a_function(const tree_managerConstRef& TM, const unsigned int index)
{
   long long int vec_size;
   bool is_a_pointer;
   bool ret;
   tree_helper::get_type_index(TM, index, vec_size, is_a_pointer, ret);
   return ret;
}

bool tree_helper::is_a_vector(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
   }
   return Type->get_kind() == vector_type_K;
}

bool tree_helper::is_a_misaligned_vector(const tree_managerConstRef& TM, const unsigned int index)
{
   if(!is_a_vector(TM, index))
   {
      return false;
   }
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));

   if(GetPointer<misaligned_indirect_ref>(T))
   {
      return true;
   }

   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
   }
   auto* vt = GetPointer<vector_type>(Type);
   THROW_ASSERT(vt, "expected a vector type");
   return vt->algn != tree_helper::Size(Type);
}

bool tree_helper::is_an_addr_expr(const tree_managerConstRef& TM, const unsigned int index)
{
   return (TM->get_tree_node_const(index)->get_kind() == addr_expr_K);
}

bool tree_helper::HasToBeDeclared(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef type = TM->get_tree_node_const(index);
   THROW_ASSERT(GetPointer<type_node>(type), "Tree node " + boost::lexical_cast<std::string>(index) + " is not a type_node but " + type->get_kind_text());
   if(GetPointer<type_node>(type)->name)
   {
      tree_nodeRef name = GET_NODE(GetPointer<type_node>(type)->name);
      if(name->get_kind() == type_decl_K)
      {
         auto* td = GetPointer<type_decl>(name);
         if(td->include_name == "<built-in>")
         {
            return false;
         }
         if(GetPointer<complex_type>(type))
         {
            std::string name1 = tree_helper::name_type(TM, index);
            std::vector<std::string> splitted = SplitString(name1, " ");
            if(splitted.size() > 1 and (splitted[0] == "_Complex" or splitted[0] == "__complex__" or splitted[0] == "complex"))
            {
               return false;
            }
         }
         return true;
      }
   }
   return type->get_kind() == record_type_K or type->get_kind() == union_type_K or type->get_kind() == enumeral_type_K;
}

bool tree_helper::is_function_type(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
   }
   return GetPointer<function_type>(Type) != nullptr;
}

bool tree_helper::is_function_pointer_type(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
   }
   if(GetPointer<pointer_type>(Type))
   {
      Type = GET_NODE(GetPointer<pointer_type>(Type)->ptd);
      if(GetPointer<function_type>(Type))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_bool(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   unsigned int type_index;
   if(GetPointer<type_node>(T))
   {
      type_index = index;
      Type = T;
   }
   else
   {
      // decl_node * dn = GetPointer<decl_node>(T);
      // THROW_ASSERT(dn, "expected a declaration object");
      type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
      // type_index = GET_INDEX_NODE(dn->type);
   }
   if(GetPointer<boolean_type>(Type))
   {
      return true;
   }
   std::string type_name = name_type(TM, type_index);
   return type_name == "sc_logic" || type_name == "sc_in_resolved" || type_name == "sc_inout_resolved" || type_name == "sc_out_resolved" || type_name == "sc_in_clk" || type_name == "sc_inout_clk" || type_name == "sc_out_clk" || type_name == "sc_bit" ||
          type_name == "sc_clock";
}

bool tree_helper::is_a_void(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
   }
   return GetPointer<void_type>(Type) != nullptr;
}

bool tree_helper::is_natural(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef var = TM->get_tree_node_const(index);
   THROW_ASSERT(var, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   if(GetPointer<ssa_name>(var) && GetPointer<ssa_name>(var)->min)
   {
      tree_nodeRef minimum = GET_NODE(GetPointer<ssa_name>(var)->min);
      THROW_ASSERT(minimum->get_kind() == integer_cst_K, "expected an integer const: " + boost::lexical_cast<std::string>(index));
      long long int min_value = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(minimum));
      return min_value >= 0;
   }
   else
   {
      return false;
   }
}

bool tree_helper::is_int(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   unsigned int type_index;
   if(GetPointer<type_node>(T))
   {
      type_index = index;
      Type = T;
   }
   else
   {
      // decl_node * dn = GetPointer<decl_node>(T);
      // THROW_ASSERT(dn, "expected a declaration object");
      type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
      // type_index = GET_INDEX_NODE(dn->type);
   }
   if(GetPointer<enumeral_type>(Type) && !GetPointer<enumeral_type>(Type)->unsigned_flag)
   {
      return true;
   }
   if(GetPointer<integer_type>(Type) and not GetPointer<integer_type>(Type)->unsigned_flag)
   {
      return true;
   }
   std::string type_name = name_type(TM, type_index);
   return type_name == std::string("sc_int");
}

bool tree_helper::is_real(const tree_managerConstRef& TM, const unsigned int index)
{
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
   }
   return GetPointer<real_type>(Type) != nullptr;
}

bool tree_helper::is_unsigned(const tree_managerConstRef& TM, const unsigned int index)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->is_unsigned " + boost::lexical_cast<std::string>(index));
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   unsigned int type_index;
   if(GetPointer<type_node>(T))
   {
      type_index = index;
      Type = T;
   }
   else
   {
      type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
      // type_index = GET_INDEX_NODE(dn->type);
   }
   if(GetPointer<enumeral_type>(Type) and GetPointer<enumeral_type>(Type)->unsigned_flag)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--yes");
      return true;
   }

   if(GetPointer<integer_type>(Type) and GetPointer<integer_type>(Type)->unsigned_flag)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--yes");
      return true;
   }

   std::string type_name = name_type(TM, type_index);
   if(type_name == std::string("sc_uint") || type_name == std::string("sc_lv") || type_name == std::string("sc_in_rv") || type_name == std::string("sc_out_rv") || type_name == std::string("sc_inout_rv") || type_name == std::string("sc_bv") ||
      type_name == std::string("sc_signal_rv"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--yes");
      return true;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--no");
   return false;
}

bool tree_helper::is_scalar(const tree_managerConstRef& TM, const unsigned int var)
{
   return tree_helper::is_int(TM, var) || tree_helper::is_real(TM, var) || tree_helper::is_unsigned(TM, var) || tree_helper::is_bool(TM, var);
}

bool tree_helper::is_module(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string mod_st = "sc_module";
   const std::string mod_name_st = "sc_module_name";
   const std::string ifc_st = "sc_interface";
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt && rt->binf)
   {
      auto* bi = GetPointer<binfo>(GET_NODE(rt->binf));
      if(bi && look_for_binfo_inheritance(bi, mod_st) && !look_for_binfo_inheritance(bi, mod_name_st) && !look_for_binfo_inheritance(bi, ifc_st))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_builtin_channel(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(GetPointer<record_type>(TM->get_tree_node_const(index)), "a record type is expected");
   std::string rec_name = tree_helper::record_name(TM, index);
   return rec_name == "sc_fifo" || rec_name == "tlm_fifo" || rec_name == "sc_mutex" || rec_name == "sc_semaphore";
}

bool tree_helper::is_channel(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string mod_st = "sc_module";
   const std::string ifc_st = "sc_interface";
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt && rt->binf)
   {
      auto* bi = GetPointer<binfo>(GET_NODE(rt->binf));
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

#if 0
rt->get_maybe_name().find("sc_signal<") == 0 ||
                                rt->get_maybe_name().find("sc_signal_resolved") == 0  ||  // inherit from sc_signal
                                rt->get_maybe_name().find("sc_signal_rv<") == 0  || // inherit from sc_signal
                                rt->get_maybe_name().find("sc_buffer") == 0)  // inherit from sc_signal
#endif
bool tree_helper::is_signal(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string ifc_st = "sc_interface";
   const std::string pch_st = "sc_prim_channel";
   const std::string sig_st = "sc_signal";
   const std::string clock_st = "sc_clock";
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt && rt->binf)
   {
      auto* bi = GetPointer<binfo>(GET_NODE(rt->binf));
      if(bi && look_for_binfo_inheritance(bi, pch_st) && ((look_for_binfo_inheritance(bi, sig_st) && look_for_binfo_inheritance(bi, ifc_st))) && rt->get_maybe_name() != clock_st)
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
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt && rt->binf)
   {
      auto* bi = GetPointer<binfo>(GET_NODE(rt->binf));
      if(bi && (tree_helper::look_for_binfo_inheritance(bi, ifc_st) && tree_helper::look_for_binfo_inheritance(bi, pch_st) && rt->get_maybe_name() == clock_st))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_SC_BIND_PROXY_NIL(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef curr_tn = TM->get_tree_node_const(index);
   if(curr_tn->get_kind() == addr_expr_K)
   {
      curr_tn = GET_NODE(GetPointer<addr_expr>(curr_tn)->op);
   }
   if(curr_tn->get_kind() == var_decl_K)
   {
      auto* vd = GetPointer<var_decl>(curr_tn);
      if(vd->name && GET_NODE(vd->name)->get_kind() == identifier_node_K)
      {
         auto* id = GetPointer<identifier_node>(GET_NODE(vd->name));
         std::string strg = id->strg;
         return strg.find("SC_BIND_PROXY_NIL") != std::string::npos;
      }
   }
   return false;
}

#if 0
rt->get_maybe_name().find("sc_in<") == 0 || //inherit from sc_port
                                rt->get_maybe_name().find("sc_in_rv<") == 0 || //inherit from sc_in
                                rt->get_maybe_name().find("sc_in_resolved") == 0 || //inherit from sc_in
                                rt->get_maybe_name().find("sc_in_clk") == 0 || // equal to sc_in<bool>
                                rt->get_maybe_name().find("sc_out<") == 0 || //inherit from sc_port
                                rt->get_maybe_name().find("sc_out_rv<") == 0 || //inherit from sc_inout_rv
                                rt->get_maybe_name().find("sc_out_resolved") == 0 || //inherit from sc_inout_resolved
                                rt->get_maybe_name().find("sc_out_clk") == 0 || //equal to sc_out<bool>
                                rt->get_maybe_name().find("sc_inout<") == 0 || //inherit from sc_port
                                rt->get_maybe_name().find("sc_inout_rv<") == 0 || //inherit from sc_inout
                                rt->get_maybe_name().find("sc_inout_resolved") == 0 || //inherit from sc_inout
                                rt->get_maybe_name().find("sc_inout_clk") == 0 || //equal to sc_inout<bool>
                                rt->get_maybe_name().find("sc_fifo_in<") == 0 || //inherit from sc_port
                                rt->get_maybe_name().find("sc_fifo_out<") == 0 //inherit from sc_port
#endif
bool tree_helper::is_port(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   const std::string port_st = "sc_port";
   const std::string sc_export_st = "sc_export";
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt && rt->binf)
   {
      auto* bi = GetPointer<binfo>(GET_NODE(rt->binf));
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
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt && rt->binf)
   {
      auto* bi = GetPointer<binfo>(GET_NODE(rt->binf));
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
   const std::string sc_out_st = "sc_out"; // several out port are actually inout port (e.g., sc_out_resolved and sc_out_rv)
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt && rt->binf)
   {
      auto* bi = GetPointer<binfo>(GET_NODE(rt->binf));
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
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt && rt->binf)
   {
      auto* bi = GetPointer<binfo>(GET_NODE(rt->binf));
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
   const record_type* rt = GetPointer<record_type>(TM->get_tree_node_const(index));
   if(rt && rt->binf)
   {
      auto* bi = GetPointer<binfo>(GET_NODE(rt->binf));
      if(bi && look_for_binfo_inheritance(bi, event_st))
      {
         return true;
      }
   }
   return false;
}

bool tree_helper::is_a_variable(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   switch(node->get_kind())
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
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "tree_helper::is_a_variable - variable is not supported: " + std::string(node->get_kind_text()));
   }
   return true;
}

static unsigned int check_for_simple_pointer_arithmetic(const tree_nodeRef& node)
{
   switch(GET_NODE(node)->get_kind())
   {
      case gimple_assign_K:
      {
         auto* ga = GetPointer<gimple_assign>(GET_NODE(node));
         if(ga->temporary_address)
         {
            auto* ae = GetPointer<addr_expr>(GET_NODE(ga->op1));
            if(ae)
            {
               return check_for_simple_pointer_arithmetic(ae->op);
            }
            else
            {
               auto* ppe = GetPointer<pointer_plus_expr>(GET_NODE(ga->op1));
               if(ppe)
               {
                  return check_for_simple_pointer_arithmetic(ppe->op0);
               }
               else
               {
                  auto* ne = GetPointer<nop_expr>(GET_NODE(ga->op1));
                  if(ne)
                  {
                     return check_for_simple_pointer_arithmetic(ne->op);
                  }
                  else
                  {
                     auto* vce = GetPointer<view_convert_expr>(GET_NODE(ga->op1));
                     if(vce)
                     {
                        return check_for_simple_pointer_arithmetic(vce->op);
                     }
                     else
                     {
                        return check_for_simple_pointer_arithmetic(ga->op1);
                     }
                  }
               }
            }
         }
         else if(GetPointer<pointer_plus_expr>(GET_NODE(ga->op1)))
         {
            auto* ppe = GetPointer<pointer_plus_expr>(GET_NODE(ga->op1));
            return check_for_simple_pointer_arithmetic(ppe->op0);
         }
         else if(GetPointer<nop_expr>(GET_NODE(ga->op1)))
         {
            auto* ne = GetPointer<nop_expr>(GET_NODE(ga->op1));
            return check_for_simple_pointer_arithmetic(ne->op);
         }
         else if(GetPointer<view_convert_expr>(GET_NODE(ga->op1)))
         {
            auto* vce = GetPointer<view_convert_expr>(GET_NODE(ga->op1));
            return check_for_simple_pointer_arithmetic(vce->op);
         }
         else
         {
            return 0;
         }
      }
      case mem_ref_K:
      {
         auto* mr = GetPointer<mem_ref>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(mr->op0);
      }
      case target_mem_ref461_K:
      {
         auto* tmr = GetPointer<target_mem_ref461>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(tmr->base);
      }
      case component_ref_K:
      {
         auto* cr = GetPointer<component_ref>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(cr->op0);
      }
      case realpart_expr_K:
      {
         auto* rpe = GetPointer<realpart_expr>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(rpe->op);
      }
      case imagpart_expr_K:
      {
         auto* rpe = GetPointer<imagpart_expr>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(rpe->op);
      }
      case bit_field_ref_K:
      {
         auto* bfr = GetPointer<bit_field_ref>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(bfr->op0);
      }
      case pointer_plus_expr_K:
      {
         auto* ppe = GetPointer<pointer_plus_expr>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(ppe->op0);
      }
      case view_convert_expr_K:
      {
         auto* vce = GetPointer<view_convert_expr>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(vce->op);
      }
      case addr_expr_K:
      {
         auto* ae = GetPointer<addr_expr>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(ae->op);
      }
      case array_ref_K:
      {
         auto* ar = GetPointer<array_ref>(GET_NODE(node));
         return check_for_simple_pointer_arithmetic(ar->op0);
      }
      case parm_decl_K:
      case var_decl_K:
      case ssa_name_K:
         return GET_INDEX_NODE(node);

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
      case bit_ior_concat_expr_K:
      case abs_expr_K:
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
      {
         return 0;
      }
      default:
         THROW_UNREACHABLE("");
         return 0;
   }
   return 0;
}

unsigned int tree_helper::get_base_index(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   switch(node->get_kind())
   {
      case ssa_name_K:
      {
         auto* sn = GetPointer<ssa_name>(node);
         if(sn->use_set->is_a_singleton())
         {
            if(GET_NODE(sn->use_set->variables.front())->get_kind() == function_decl_K)
            {
               return sn->use_set->variables.front()->index;
            }
            else
            {
               return get_base_index(TM, GET_INDEX_NODE(sn->use_set->variables.front()));
            }
         }
         unsigned int base_index = sn->CGetDefStmts().size() == 1 ? check_for_simple_pointer_arithmetic(sn->CGetDefStmt()) : 0;
         if(base_index)
         {
            return get_base_index(TM, base_index);
         }

         if(sn->var)
         {
            if(GET_NODE(sn->var)->get_kind() == function_decl_K)
            {
               return sn->var->index;
            }
            else
            {
               return get_base_index(TM, sn->var->index);
            }
         }
         else
         {
            return index;
         }
      }
      case result_decl_K:
      case parm_decl_K:
      case var_decl_K:
      case string_cst_K:
      case integer_cst_K:
      {
         return index;
      }
      case indirect_ref_K:
      {
         auto* ir = GetPointer<indirect_ref>(node);
         return get_base_index(TM, GET_INDEX_NODE(ir->op));
      }
      case misaligned_indirect_ref_K:
      {
         auto* mir = GetPointer<misaligned_indirect_ref>(node);
         return get_base_index(TM, GET_INDEX_NODE(mir->op));
      }
      case mem_ref_K:
      {
         auto* mr = GetPointer<mem_ref>(node);
         return get_base_index(TM, GET_INDEX_NODE(mr->op0));
      }
      case array_ref_K:
      {
         auto* ar = GetPointer<array_ref>(node);
         return get_base_index(TM, GET_INDEX_NODE(ar->op0));
      }
      case component_ref_K:
      {
         auto* cr = GetPointer<component_ref>(node);
         return get_base_index(TM, GET_INDEX_NODE(cr->op0));
      }
      case realpart_expr_K:
      {
         auto* rpe = GetPointer<realpart_expr>(node);
         return get_base_index(TM, GET_INDEX_NODE(rpe->op));
      }
      case imagpart_expr_K:
      {
         auto* rpe = GetPointer<imagpart_expr>(node);
         return get_base_index(TM, GET_INDEX_NODE(rpe->op));
      }
      case bit_field_ref_K:
      {
         auto* bfr = GetPointer<bit_field_ref>(node);
         return get_base_index(TM, GET_INDEX_NODE(bfr->op0));
      }
      case target_mem_ref_K:
      {
         auto* tmr = GetPointer<target_mem_ref>(node);
         if(tmr->symbol)
         {
            return get_base_index(TM, GET_INDEX_NODE(tmr->symbol));
         }
         else if(tmr->base)
         {
            return get_base_index(TM, GET_INDEX_NODE(tmr->base));
         }
         else if(tmr->idx)
         {
            return get_base_index(TM, GET_INDEX_NODE(tmr->idx));
         }
         else
         {
            THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "tree_helper::get_base_index::target_mem_ref_K - variable type pattern not supported: " + STR(index));
         }
         break;
      }
      case target_mem_ref461_K:
      {
         auto* tmr = GetPointer<target_mem_ref461>(node);
         if(tmr->base)
         {
            return get_base_index(TM, GET_INDEX_NODE(tmr->base));
         }
         else if(tmr->idx)
         {
            return get_base_index(TM, GET_INDEX_NODE(tmr->idx));
         }
         else if(tmr->idx2)
         {
            return get_base_index(TM, GET_INDEX_NODE(tmr->idx2));
         }
         else
         {
            THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "tree_helper::get_base_index::target_mem_ref461_K - variable type pattern not supported: " + STR(index));
         }
         break;
      }
      case addr_expr_K:
      {
         auto* ae = GetPointer<addr_expr>(node);
         tree_nodeRef addr_expr_op = GET_NODE(ae->op);

         switch(addr_expr_op->get_kind())
         {
            case ssa_name_K:
            case var_decl_K:
            case parm_decl_K:
            case string_cst_K:
            case result_decl_K:
            {
               return GET_INDEX_NODE(ae->op);
            }
            case array_ref_K:
            {
               auto* ar = GetPointer<array_ref>(addr_expr_op);
               tree_nodeRef idx = GET_NODE(ar->op1);
               if(idx->get_kind() == integer_cst_K and get_integer_cst_value(GetPointer<integer_cst>(idx)) == 0)
               {
                  switch(GET_NODE(ar->op0)->get_kind())
                  {
                     case ssa_name_K:
                     case var_decl_K:
                     case parm_decl_K:
                     case string_cst_K:
                     {
                        return GET_INDEX_NODE(ar->op0);
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
                        THROW_ERROR("addr_expr-array_ref[0] pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" + STR(index));
                  }
               }
               else
               {
                  return index;
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
               THROW_ERROR("addr_expr pattern not supported: " + std::string(addr_expr_op->get_kind_text()) + " @" + STR(index));
         }
         break;
      }
      case view_convert_expr_K:
      {
         auto* vc = GetPointer<view_convert_expr>(node);
         tree_nodeRef vc_expr_op = GET_NODE(vc->op);

         switch(vc_expr_op->get_kind())
         {
            case ssa_name_K:
            {
               auto* sn = GetPointer<ssa_name>(GET_NODE(vc->op));
               if(!sn->var)
               {
                  return 0;
               }
               auto* pd = GetPointer<parm_decl>(GET_NODE(sn->var));
               if(pd)
               {
                  return GET_INDEX_NODE(sn->var);
               }
               else
               {
                  THROW_ERROR("view_convert_expr pattern currently not supported: " + GET_NODE(vc->op)->get_kind_text() + " @" + STR(index));
               }
               break;
            }
            case var_decl_K:
            {
               return GET_INDEX_NODE(vc->op);
            }
            case integer_cst_K:
            {
               return index;
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
               THROW_ERROR("view_convert_expr pattern not supported: " + std::string(vc_expr_op->get_kind_text()) + " @" + STR(index));
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
      case bit_ior_concat_expr_K:
      case abs_expr_K:
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
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "tree_helper::get_base_index - variable type is not supported: " + STR(index) + "-" + std::string(node->get_kind_text()));
   }
   return 0;
}

bool tree_helper::is_fully_resolved(const tree_managerConstRef& TM, const unsigned int index, CustomOrderedSet<unsigned int>& res_set)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   switch(node->get_kind())
   {
      case ssa_name_K:
      {
         auto* sn = GetPointer<ssa_name>(node);
         if(sn->use_set->is_fully_resolved())
         {
            const auto v_it_end = sn->use_set->variables.end();
            for(auto v_it = sn->use_set->variables.begin(); v_it != v_it_end; ++v_it)
            {
               res_set.insert(GET_INDEX_NODE(*v_it));
            }
            return true;
         }
         else
         {
            unsigned int base_index = sn->CGetDefStmts().size() == 1 ? check_for_simple_pointer_arithmetic(sn->CGetDefStmt()) : 0;
            if(base_index)
            {
               return is_fully_resolved(TM, base_index, res_set);
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
         auto* ir = GetPointer<indirect_ref>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(ir->op), res_set);
      }
      case misaligned_indirect_ref_K:
      {
         auto* mir = GetPointer<misaligned_indirect_ref>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(mir->op), res_set);
      }
      case mem_ref_K:
      {
         auto* mr = GetPointer<mem_ref>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(mr->op0), res_set);
      }
      case array_ref_K:
      {
         auto* ar = GetPointer<array_ref>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(ar->op0), res_set);
      }
      case component_ref_K:
      {
         auto* cr = GetPointer<component_ref>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(cr->op0), res_set);
      }
      case realpart_expr_K:
      {
         auto* rpe = GetPointer<realpart_expr>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(rpe->op), res_set);
      }
      case imagpart_expr_K:
      {
         auto* rpe = GetPointer<imagpart_expr>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(rpe->op), res_set);
      }
      case bit_field_ref_K:
      {
         auto* bfr = GetPointer<bit_field_ref>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(bfr->op0), res_set);
      }
      case target_mem_ref_K:
      {
         auto* tmr = GetPointer<target_mem_ref>(node);
         if(tmr->symbol)
         {
            return is_fully_resolved(TM, GET_INDEX_NODE(tmr->symbol), res_set);
         }
         else if(tmr->base)
         {
            return is_fully_resolved(TM, GET_INDEX_NODE(tmr->base), res_set);
         }
         else
         {
            return false;
         }
      }
      case target_mem_ref461_K:
      {
         auto* tmr = GetPointer<target_mem_ref461>(node);
         if(tmr->base)
         {
            return is_fully_resolved(TM, GET_INDEX_NODE(tmr->base), res_set);
         }
         else
         {
            return false;
         }
      }
      case addr_expr_K:
      {
         auto* ae = GetPointer<addr_expr>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(ae->op), res_set);
      }
      case view_convert_expr_K:
      {
         auto* vc = GetPointer<view_convert_expr>(node);
         return is_fully_resolved(TM, GET_INDEX_NODE(vc->op), res_set);
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
      case bit_ior_concat_expr_K:
      case abs_expr_K:
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
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_TYPE_NODES:
      case paren_expr_K:
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "tree_helper::get_base_index - variable type is not supported: " + STR(index) + "-" + std::string(node->get_kind_text()));
   }
   return false;
}

bool tree_helper::is_volatile(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   auto* sa = GetPointer<ssa_name>(node);
   if(!sa)
   {
      // variable or indirect ref
      tree_nodeRef n = get_type_node(node);
      auto* tn = GetPointer<type_node>(n);
      return tn->qual == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_V || tn->qual == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_VR || tn->qual == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CVR;
   }
   return sa->volatile_flag;
}

bool tree_helper::is_parameter(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   if(GetPointer<parm_decl>(node))
   {
      return true;
   }
   auto* sn = GetPointer<ssa_name>(node);
   if(!sn)
   {
      return false;
   }
   return GET_NODE(sn->CGetDefStmt())->get_kind() == gimple_nop_K && GET_NODE(sn->var)->get_kind() == parm_decl_K;
}

bool tree_helper::is_ssa_name(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   auto* sn = GetPointer<ssa_name>(node);
   return sn != nullptr;
}

bool tree_helper::is_virtual(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   auto* sn = GetPointer<ssa_name>(node);
   if(!sn)
   {
      return false;
   }
   return sn->virtual_flag;
}

bool tree_helper::is_static(const tree_managerConstRef& TM, const unsigned int index)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   auto* vd = GetPointer<var_decl>(node);
   if(!vd)
   {
      auto* fd = GetPointer<function_decl>(node);
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
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   auto* vd = GetPointer<var_decl>(node);
   if(!vd)
   {
      auto* fd = GetPointer<function_decl>(node);
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
   const tree_nodeRef T = TM->get_tree_node_const(index);
   THROW_ASSERT(T, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   tree_nodeRef Type;
   if(GetPointer<type_node>(T))
   {
      Type = T;
   }
   else
   {
      unsigned int type_index = 0;
      Type = get_type_node(T, type_index);
      THROW_ASSERT(type_index > 0, "expected a type index");
   }
   const auto quals = GetPointer<type_node>(Type)->qual;
   return quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN and (quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_C || quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CV || quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CVR);
}

std::string tree_helper::return_qualifier_prefix(const TreeVocabularyTokenTypes_TokenEnum quals)
{
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_R)
   {
      return "r_";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_V)
   {
      return "v_";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_VR)
   {
      return "vr_";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_C)
   {
      return "c_";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CR)
   {
      return "cr_";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CV)
   {
      return "cv_";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CVR)
   {
      return "cvr_";
   }
   THROW_UNREACHABLE("not supported qualifier " + STR(static_cast<unsigned int>(quals)));
   return "";
}

std::string tree_helper::return_C_qualifiers(const TreeVocabularyTokenTypes_TokenEnum quals, bool real_const)
{
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_R)
   {
      return "__restrict__ ";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_V)
   {
      return "volatile ";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_VR)
   {
      return "volatile __restrict__ ";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_C)
   {
      return real_const ? "const " : "/*const*/ ";
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CR)
   {
      return (real_const ? "const" : "/*const*/") + std::string(" __restrict__ ");
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CV)
   {
      return (real_const ? "const" : "/*const*/") + std::string(" volatile ");
   }
   if(quals == TreeVocabularyTokenTypes_TokenEnum::TOK_QUAL_CVR)
   {
      return (real_const ? "const" : "/*const*/") + std::string(" volatile __restrict__ ");
   }
   THROW_UNREACHABLE("not supported qualifier " + boost::lexical_cast<std::string>(static_cast<unsigned int>(quals)));
   return "";
}

long long tree_helper::get_integer_cst_value(const integer_cst* ic)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Getting integer const value");
   THROW_ASSERT(ic != nullptr, "unexpected condition");
   THROW_ASSERT(ic->type, "Something wrong");
   tree_nodeRef type = GET_NODE(ic->type);
   THROW_ASSERT(GetPointer<integer_type>(type) or type->get_kind() == pointer_type_K or type->get_kind() == reference_type_K or type->get_kind() == boolean_type_K or type->get_kind() == enumeral_type_K,
                "Expected a integer_type, pointer_type, reference_type, boolean_type, enumeral_type. Found: " + STR(GET_INDEX_NODE(ic->type)) + " " + type->get_kind_text());
   long long result = 0;
   /// If high is null or if high is a sign extension or low is enough to express an int (64 bit system)

   result = ic->value;
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Constant is " + boost::lexical_cast<std::string>(result));
   return result;
}

unsigned int tree_helper::get_array_var(const tree_managerConstRef& TM, const unsigned int index, bool is_written, bool& two_dim_p)
{
   THROW_ASSERT(index > 0, "expected positive non zero numbers");
   tree_nodeRef node = TM->get_tree_node_const(index);
   auto* gms = GetPointer<gimple_assign>(node);
   array_ref* ar;
   if(is_written)
   {
      ar = GetPointer<array_ref>(GET_NODE(gms->op0));
   }
   else
   {
      ar = GetPointer<array_ref>(GET_NODE(gms->op1));
   }
   two_dim_p = false;
   if(ar)
   {
      if(GetPointer<array_ref>(GET_NODE(ar->op0)))
      {
         ar = GetPointer<array_ref>(GET_NODE(ar->op0));
         if(GetPointer<array_ref>(GET_NODE(ar->op0)))
         {
            THROW_ERROR("n-dimension array not yet supported (n > 2)");
         }
         two_dim_p = true;
         return GET_INDEX_NODE(ar->op0);
      }
      else
      {
         return GET_INDEX_NODE(ar->op0);
      }
   }
   target_mem_ref* tmr;
   if(is_written)
   {
      tmr = GetPointer<target_mem_ref>(GET_NODE(gms->op0));
   }
   else
   {
      tmr = GetPointer<target_mem_ref>(GET_NODE(gms->op1));
   }
   if(tmr)
   {
      if(tmr->symbol)
      {
         return GET_INDEX_NODE(tmr->symbol);
      }
      else if(tmr->base)
      {
         return GET_INDEX_NODE(tmr->base);
      }
      else
      {
         THROW_ERROR("Unexpected pattern");
      }
   }
   target_mem_ref461* tmr461;
   if(is_written)
   {
      tmr461 = GetPointer<target_mem_ref461>(GET_NODE(gms->op0));
   }
   else
   {
      tmr461 = GetPointer<target_mem_ref461>(GET_NODE(gms->op1));
   }
   if(tmr461)
   {
      if(tmr461->base)
      {
         return GET_INDEX_NODE(tmr461->base);
      }
      else
      {
         THROW_ERROR("Unexpected pattern");
      }
   }
   auto* ae = GetPointer<addr_expr>(GET_NODE(gms->op1));
   if(ae)
   {
      ar = GetPointer<array_ref>(GET_NODE(ae->op));
      if(ar)
      {
         if(GetPointer<array_ref>(GET_NODE(ar->op0)))
         {
            ar = GetPointer<array_ref>(GET_NODE(ar->op0));
            if(GetPointer<array_ref>(GET_NODE(ar->op0)))
            {
               THROW_ERROR("n-dimension array not yet supported (n > 2)");
            }
            two_dim_p = true;
            return GET_INDEX_NODE(ar->op0);
         }
         else
         {
            return GET_INDEX_NODE(ar->op0);
         }
      }
      else
      {
         THROW_ERROR("Unexpected pattern " + boost::lexical_cast<std::string>(index));
         return 0;
      }
   }
   auto* ne = GetPointer<nop_expr>(GET_NODE(gms->op1));
   if(ne)
   {
      ae = GetPointer<addr_expr>(GET_NODE(ne->op));
      if(ae)
      {
         ar = GetPointer<array_ref>(GET_NODE(ae->op));
         if(ar)
         {
            if(GetPointer<array_ref>(GET_NODE(ar->op0)))
            {
               ar = GetPointer<array_ref>(GET_NODE(ar->op0));
               if(GetPointer<array_ref>(GET_NODE(ar->op0)))
               {
                  THROW_ERROR("n-dimension array not yet supported (n > 2)");
               }
               two_dim_p = true;
               return GET_INDEX_NODE(ar->op0);
            }
            else
            {
               return GET_INDEX_NODE(ar->op0);
            }
         }
         auto* vd = GetPointer<var_decl>(GET_NODE(ae->op));
         if(vd)
         {
            return GET_INDEX_NODE(ae->op);
         }
         else
         {
            THROW_ERROR("Unexpected pattern " + boost::lexical_cast<std::string>(index));
            return 0;
         }
      }
   }
   THROW_ERROR("Unexpected pattern " + boost::lexical_cast<std::string>(index));
   return 0;
}

bool tree_helper::is_concat_bit_ior_expr(const tree_managerConstRef& TM, const unsigned int index)
{
   tree_nodeRef node = TM->get_tree_node_const(index);
   auto* ga = GetPointer<gimple_assign>(node);
   if(ga)
   {
      auto* bie = GetPointer<bit_ior_expr>(GET_NODE(ga->op1));
      if(bie)
      {
         tree_nodeRef op0 = GET_NODE(bie->op0);
         tree_nodeRef op1 = GET_NODE(bie->op1);
         if(op0->get_kind() == ssa_name_K && op1->get_kind() == ssa_name_K)
         {
            auto* op0_ssa = GetPointer<ssa_name>(op0);
            auto* op1_ssa = GetPointer<ssa_name>(op1);
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
   tree_nodeRef node = TM->get_tree_node_const(index);
   auto* ga = GetPointer<gimple_assign>(node);
   if(ga)
   {
      auto* ppe = GetPointer<pointer_plus_expr>(GET_NODE(ga->op1));
      if(ppe)
      {
         tree_nodeRef op1 = GET_NODE(ppe->op1);
         if(op1->get_kind() == integer_cst_K)
         {
            tree_nodeRef op0 = GET_NODE(ppe->op0);
            if(op0->get_kind() == addr_expr_K)
            {
               return true;
            }
            else if(op0->get_kind() == ssa_name_K)
            {
               auto temp_def = GET_NODE(GetPointer<const ssa_name>(op0)->CGetDefStmt());
               auto* ga_def = GetPointer<gimple_assign>(temp_def);
               if(ga_def)
               {
                  if(GET_NODE(ga_def->op1)->get_kind() == addr_expr_K)
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
   tree_nodeRef node = TM->get_tree_node_const(index);
   switch(node->get_kind())
   {
      case string_cst_K:
      case real_cst_K:
      case complex_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case integer_cst_K:
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

std::string tree_helper::op_symbol(const tree_nodeRef& op)
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
            const auto* ae = dynamic_cast<const addr_expr*>(op);
            tree_nodeRef tn = GET_NODE(ae->op);
            if(GetPointer<array_ref>(tn))
            {
               auto* ar = GetPointer<array_ref>(tn);
               tree_nodeRef base = GET_NODE(ar->op0);
               tree_nodeRef offset = GET_NODE(ar->op1);
               if(base->get_kind() == string_cst_K and offset->get_kind() == integer_cst_K and !tree_helper::get_integer_cst_value(GetPointer<integer_cst>(offset)))
               {
                  return "";
               }
            }
            if(GetPointer<string_cst>(tn))
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
         THROW_ERROR(std::string("op_symbol not yet supported ") + op->get_kind_text() + " in node " + boost::lexical_cast<std::string>(op->index));
         return "";
   }
}

unsigned int tree_helper::get_array_data_bitsize(const tree_managerConstRef& TM, const unsigned int index)
{
   tree_nodeRef node = TM->get_tree_node_const(index);
   if(node->get_kind() == record_type_K)
   {
      auto* rt = GetPointer<record_type>(node);
      auto fd = GET_NODE(rt->list_of_flds[0]);
      THROW_ASSERT(fd->get_kind() == field_decl_K, "expected a field_decl");
      auto at_index = GET_INDEX_NODE(GetPointer<field_decl>(fd)->type);
      return get_array_data_bitsize(TM, at_index);
   }
   THROW_ASSERT(node->get_kind() == array_type_K, "array_type expected: @" + STR(index));
   auto* at = GetPointer<array_type>(node);
   THROW_ASSERT(at->elts, "elements type expected");
   tree_nodeRef elts = GET_NODE(at->elts);
   unsigned int return_value;
   if(elts->get_kind() == array_type_K)
   {
      return_value = get_array_data_bitsize(TM, GET_INDEX_NODE(at->elts));
   }
   else
   {
      auto type_id = get_type_index(TM, GET_INDEX_NODE(at->elts));
      return_value = size(TM, type_id);
      auto* fd = GetPointer<field_decl>(TM->get_tree_node_const(type_id));
      if(!fd or !fd->is_bitfield())
      {
         return_value = std::max(8u, return_value);
      }
   }
   return return_value;
}

void tree_helper::get_array_dim_and_bitsize(const tree_managerConstRef& TM, const unsigned int index, std::vector<unsigned int>& dims, unsigned int& elts_bitsize)
{
   tree_nodeRef node = TM->get_tree_node_const(index);
   if(node->get_kind() == record_type_K)
   {
      auto* rt = GetPointer<record_type>(node);
      auto fd = GET_NODE(rt->list_of_flds[0]);
      THROW_ASSERT(fd->get_kind() == field_decl_K, "expected a field_decl");
      auto at_index = GET_INDEX_NODE(GetPointer<field_decl>(fd)->type);
      get_array_dim_and_bitsize(TM, at_index, dims, elts_bitsize);
      return;
   }
   THROW_ASSERT(node->get_kind() == array_type_K, "array_type expected: @" + STR(index));
   auto* at = GetPointer<array_type>(node);
   if(!at->domn)
   {
      dims.push_back(1); // at least one element is expected
   }
   else
   {
      tree_nodeRef domn = GET_NODE(at->domn);
      THROW_ASSERT(domn->get_kind() == integer_type_K, "expected an integer type as domain");
      auto* it = GetPointer<integer_type>(domn);
      unsigned int min_value = 0;
      unsigned int max_value = 0;
      if(it->min)
      {
         min_value = static_cast<unsigned int>(get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->min))));
      }
      if(it->max)
      {
         max_value = static_cast<unsigned int>(get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->max))));
      }
      unsigned int range_domain = max_value - min_value + 1;
      dims.push_back(range_domain);
   }
   THROW_ASSERT(at->elts, "elements type expected");
   tree_nodeRef elts = GET_NODE(at->elts);
   if(elts->get_kind() == array_type_K)
   {
      get_array_dim_and_bitsize(TM, GET_INDEX_NODE(at->elts), dims, elts_bitsize);
   }
   else
   {
      auto type_id = get_type_index(TM, GET_INDEX_NODE(at->elts));
      elts_bitsize = size(TM, type_id);
      auto* fd = GetPointer<field_decl>(TM->get_tree_node_const(type_id));
      if(!fd or !fd->is_bitfield())
      {
         elts_bitsize = std::max(8u, elts_bitsize);
      }
   }
}

void tree_helper::get_array_dimensions(const tree_managerConstRef& TM, const unsigned int index, std::vector<unsigned int>& dims)
{
   tree_nodeRef node = TM->get_tree_node_const(index);
   if(node->get_kind() == record_type_K)
   {
      auto* rt = GetPointer<record_type>(node);
      auto fd = GET_NODE(rt->list_of_flds[0]);
      THROW_ASSERT(fd->get_kind() == field_decl_K, "expected a field_decl");
      auto at_index = GET_INDEX_NODE(GetPointer<field_decl>(fd)->type);
      get_array_dimensions(TM, at_index, dims);
      return;
   }
   THROW_ASSERT(node->get_kind() == array_type_K, "array_type expected: @" + STR(index));
   auto* at = GetPointer<array_type>(node);
   tree_nodeRef domn = GET_NODE(at->domn);
   THROW_ASSERT(domn->get_kind() == integer_type_K, "expected an integer type as domain");
   auto* it = GetPointer<integer_type>(domn);
   unsigned int min_value = 0;
   unsigned int max_value = 0;
   if(it->min && GET_NODE(it->min)->get_kind() == integer_cst_K && it->max && GET_NODE(it->max)->get_kind() == integer_cst_K)
   {
      if(it->min)
      {
         min_value = static_cast<unsigned int>(get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->min))));
      }
      if(it->max)
      {
         max_value = static_cast<unsigned int>(get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->max))));
      }
      unsigned int range_domain = max_value - min_value + 1;
      dims.push_back(range_domain);
   }
   else
      dims.push_back(0); // variable size array may fall in this case
   THROW_ASSERT(at->elts, "elements type expected");
   tree_nodeRef elts = GET_NODE(at->elts);
   if(elts->get_kind() == array_type_K)
   {
      get_array_dimensions(TM, GET_INDEX_NODE(at->elts), dims);
   }
}

unsigned int tree_helper::get_array_num_elements(const tree_managerConstRef& TM, const unsigned int index)
{
   unsigned int num_elements = 1;
   std::vector<unsigned int> array_dimensions;

   get_array_dimensions(TM, index, array_dimensions);

   for(auto i : array_dimensions)
   {
      num_elements *= i;
   }

   return num_elements;
}

void tree_helper::extract_array_indexes(const tree_managerConstRef& TM, const unsigned int index, std::vector<unsigned int>& indexes, std::vector<unsigned int>& size_indexes, unsigned int& base_object)
{
   tree_nodeRef node = TM->get_tree_node_const(index);
   THROW_ASSERT(node->get_kind() == array_ref_K, "array_ref expected: @" + STR(index));
   auto* ar = GetPointer<array_ref>(node);
   base_object = GET_INDEX_NODE(ar->op0);
   if(GET_NODE(ar->op0)->get_kind() == array_ref_K)
   {
      auto* nested_ar = GetPointer<array_ref>(GET_NODE(ar->op0));
      auto* at = GetPointer<array_type>(GET_NODE(nested_ar->type));
      tree_nodeRef domn = GET_NODE(at->domn);
      THROW_ASSERT(domn->get_kind() == integer_type_K, "expected an integer type as domain");
      auto* it = GetPointer<integer_type>(domn);
      unsigned int min_value = 0;
      unsigned int max_value = 0;
      if(it->min)
      {
         min_value = static_cast<unsigned int>(get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->min))));
      }
      if(it->max)
      {
         max_value = static_cast<unsigned int>(get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->max))));
      }
      unsigned int range_domain = max_value - min_value + 1;
      size_indexes.push_back(range_domain);
      extract_array_indexes(TM, base_object, indexes, size_indexes, base_object);
      indexes.push_back(GET_INDEX_NODE(nested_ar->op1));
   }
}

unsigned int tree_helper::GetUnqualified(const tree_managerConstRef& TM, unsigned int type)
{
   const tree_nodeRef typeNode = TM->get_tree_node_const(type);
   if(GetPointer<type_node>(typeNode) and GetPointer<type_node>(typeNode)->unql)
   {
      return GET_INDEX_NODE(GetPointer<type_node>(typeNode)->unql);
   }
   return 0;
}

bool tree_helper::IsAligned(const tree_managerConstRef& TM, unsigned int type)
{
   const tree_nodeRef node = TM->get_tree_node_const(type);
   const type_node* tn = GetPointer<type_node>(node);
   THROW_ASSERT(tn, "Tree node " + boost::lexical_cast<std::string>(type) + " is of type " + node->get_kind_text());
   return tn->unql and tn->algn != GetPointer<type_node>(GET_NODE(tn->unql))->algn;
}

unsigned int tree_helper::get_var_alignment(const tree_managerConstRef& TM, unsigned int var)
{
   const tree_nodeRef varnode = TM->get_tree_node_const(var);
   const auto* vd = GetPointer<var_decl>(varnode);
   if(vd)
      return vd->algn < 8 ? 1 : (vd->algn / 8);
   return 1;
}

std::string tree_helper::normalized_ID(const std::string& id)
{
   std::string strg = id;
   for(char& i : strg)
   {
      if(i == '*')
      {
         i = '_';
      }
      else if(i == '$')
      {
         i = '_';
      }
      else if(i == '.')
      {
         i = '_';
      }
      else if(i == ':')
      {
         i = '_';
      }
   }
   return strg;
}

std::string tree_helper::print_type(const tree_managerConstRef& TM, unsigned int original_type, bool global, bool print_qualifiers, bool print_storage, unsigned int var, const var_pp_functorConstRef& vppf, const std::string& prefix,
                                    const std::string& tail)
{
   bool skip_var_printing = false;
   const unsigned int type = tree_helper::GetRealType(TM, original_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Printing type " + boost::lexical_cast<std::string>(original_type) + "(" + boost::lexical_cast<std::string>(type) + ") - Var " + boost::lexical_cast<std::string>(var));
   std::string res;
   const tree_nodeRef node_type = TM->get_tree_node_const(type);
   if(var)
   {
      const tree_nodeRef node_var = TM->get_tree_node_const(var);
      if(node_var->get_kind() == var_decl_K)
      {
         auto* vd = GetPointer<var_decl>(node_var);
         if((vd->extern_flag) and print_storage)
         {
            res += "extern ";
         }
         if((vd->static_flag || vd->static_static_flag) and print_storage)
         {
            res += "static ";
         }
      }
   }
   switch(node_type->get_kind())
   {
      case function_decl_K:
      {
         auto* fd = GetPointer<function_decl>(node_type);
         std::string function_name = tree_helper::print_function_name(TM, fd);
         if(fd->undefined_flag)
         {
            res = "extern ";
         }
         else if(!fd->static_flag && TM->is_CPP() && !fd->mngl && function_name != "main")
         {
            res = "\n#ifdef __cplusplus\n  extern \"C\"\n#else\n  extern\n#endif\n";
         }
         if(fd->static_flag)
         {
            res = "static ";
         }
         auto* dn = GetPointer<decl_node>(node_type);
         THROW_ASSERT(dn, "expected a declaration node");
         tree_nodeRef ftype = GET_NODE(dn->type);
         /* Print type declaration.  */
         if(ftype->get_kind() == function_type_K || ftype->get_kind() == method_type_K)
         {
            auto* ft = GetPointer<function_type>(ftype);
            res += print_type(TM, GET_INDEX_NODE(ft->retn), global);
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
               unsigned int argom = GET_INDEX_NODE(fd->list_of_args[i]);
               res = res + print_type(TM, get_type_index(TM, argom), global, print_qualifiers, print_storage, argom, vppf);
            }
         }
         else if(TM->get_implementation_node(type) and TM->get_implementation_node(type) != type)
         {
            skip_var_printing = true;
            res = print_type(TM, TM->get_implementation_node(type), global, print_qualifiers, print_storage, 0, vppf);
            break;
         }
         else if(GetPointer<function_type>(ftype)->prms)
         {
            if(ftype->get_kind() == function_type_K)
            {
               auto* ft = GetPointer<function_type>(ftype);
               res += print_type(TM, GET_INDEX_NODE(ft->prms), global);
            }
            else if(ftype->get_kind() == method_type_K)
            {
               auto* mt = GetPointer<method_type>(ftype);
               res += print_type(TM, GET_INDEX_NODE(mt->prms), global);
            }
            else
            {
               THROW_ERROR(std::string("tree node not currently supported ") + node_type->get_kind_text());
            }
         }
         if((!fd->list_of_args.empty() || GetPointer<function_type>(ftype)->prms) && GetPointer<function_type>(ftype)->varargs_flag)
         {
            res += ", ... ";
         }
         res += ")";
         break;
      }
      case type_decl_K:
      {
         auto* td = GetPointer<type_decl>(node_type);
         if(td->name)
         {
            tree_nodeRef name = GET_NODE(td->name);
            if(name->get_kind() == identifier_node_K)
            {
               auto* in = GetPointer<identifier_node>(name);
               /// patch for unsigned char
               std::string typename_value = tree_helper::normalized_ID(in->strg);
               if(typename_value == "char" && GetPointer<integer_type>(GET_NODE(td->type)) && GetPointer<integer_type>(GET_NODE(td->type))->unsigned_flag)
               {
                  res += "unsigned " + typename_value;
                  /// patch for va_list
               }
               else if(typename_value == "__va_list_tag")
               {
                  res += "va_list";
               }
               else if(GET_NODE(td->type)->get_kind() == complex_type_K)
               {
                  std::vector<std::string> splitted = SplitString(typename_value, " ");
                  if((splitted[0] == "_Complex" || splitted[0] == "__complex__" || splitted[0] == "complex"))
                  {
                     res += "__complex__";
                     for(unsigned int ci = 1; ci < splitted.size(); ci++)
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
         auto* in = GetPointer<identifier_node>(node_type);
         res += tree_helper::normalized_ID(in->strg);
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
         auto* tn = GetPointer<type_node>(node_type);
         const auto quals = tn->qual;
         /* const internally are not considered as constant...*/
         if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
         {
            tree_nodeRef node;
            if(var)
            {
               node = TM->get_tree_node_const(var);
            }
            if(var > 0 && !tn->name)
            {
               /// Global variables or parameters
               if((GetPointer<var_decl>(node) and (!(GetPointer<var_decl>(node)->scpe) or GET_NODE(GetPointer<var_decl>(node)->scpe)->get_kind() == translation_unit_decl_K)))
               {
                  res += tree_helper::return_C_qualifiers(quals, true);
               }
               else
               {
                  res += tree_helper::return_C_qualifiers(quals, false);
               }
            }
            else if(global or print_qualifiers)
            {
               res += tree_helper::return_C_qualifiers(quals, true);
            }
         }
         if(tn->name && (GET_NODE(tn->name)->get_kind() != type_decl_K || !tn->system_flag))
         {
            tree_nodeRef name = GET_NODE(tn->name);
            if(name->get_kind() == identifier_node_K)
            {
               auto* in = GetPointer<identifier_node>(name);
               res += tree_helper::normalized_ID(in->strg);
            }
            else if(name->get_kind() == type_decl_K)
            {
               res += print_type(TM, GET_INDEX_NODE(tn->name), global);
            }
            else
            {
               THROW_ERROR(std::string("Node not yet supported: ") + node_type->get_kind_text()); // p_string(buffer, get_unnamed(node));
            }
         }
         else if(node_type->get_kind() == complex_type_K)
         {
            const auto ct = GetPointer<const complex_type>(node_type);
            if(ct->unql)
            {
               res += print_type(TM, GET_INDEX_NODE(ct->unql), global);
            }
            else
            {
               res += "_Complex ";
               if(GetPointer<complex_type>(node_type)->unsigned_flag)
               {
                  res += "unsigned ";
               }
               if(GetPointer<complex_type>(node_type)->real_flag)
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
                     THROW_ERROR(std::string("Complex Real type not yet supported ") + boost::lexical_cast<std::string>(original_type));
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
                     THROW_ERROR(std::string("Node not yet supported: ") + node_type->get_kind_text() + boost::lexical_cast<std::string>(var));
                  }
               }
            }
         }
         else if(node_type->get_kind() == vector_type_K)
         {
            auto* vt = GetPointer<vector_type>(node_type);
            if(vt->unql)
            {
               res += print_type(TM, GET_INDEX_NODE(vt->unql), global);
            }
            else
            {
               /// Ad hoc management of vector of bool
               if(GET_CONST_NODE(vt->elts)->get_kind() == boolean_type_K)
               {
                  res += "int __attribute__((vector_size(" + STR(Size(node_type) * 4) + ")))";
               }
               else
               {
                  // THROW_ERROR(std::string("Node not yet supported:<unnamed type> ") + node_type->get_kind_text()+boost::lexical_cast<std::string>(type));
                  THROW_ASSERT(vt->elts, "expected the type of the elements of the vector");
                  res += print_type(TM, GET_INDEX_NODE(vt->elts), global);
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
                  auto* it = GetPointer<integer_type>(node_type);
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
                     THROW_ERROR(std::string("Node not yet supported: ") + node_type->get_kind_text() + STR(tn->algn));
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
                  auto* rt = GetPointer<real_type>(node_type);
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
                     THROW_ERROR(std::string("Real type not yet supported ") + boost::lexical_cast<std::string>(original_type));
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
                  THROW_ERROR(std::string("Node not yet supported ") + node_type->get_kind_text() + " " + boost::lexical_cast<std::string>(original_type));
            }
         }
         break;
      }
      case pointer_type_K:
      case reference_type_K:
      {
         unsigned int ptd_type;
         if(node_type->get_kind() == pointer_type_K)
         {
            auto* tree_type = GetPointer<pointer_type>(node_type);
            if(tree_type->name && GET_NODE(tree_type->name)->get_kind() == type_decl_K)
            {
               auto* td = GetPointer<type_decl>(GET_NODE(tree_type->name));
               if(td->name && GET_NODE(td->name)->get_kind() == identifier_node_K)
               {
                  auto* id = GetPointer<identifier_node>(GET_NODE(td->name));
                  if(id->strg == "va_list")
                  {
                     res = "va_list";
                     break;
                  }
               }
            }
            if(tree_type->ptd && GET_NODE(tree_type->ptd)->get_kind() == record_type_K && GetPointer<record_type>(GET_NODE(tree_type->ptd))->name && GET_NODE(GetPointer<record_type>(GET_NODE(tree_type->ptd))->name)->get_kind() == type_decl_K)
            {
               auto* td = GetPointer<type_decl>(GET_NODE(GetPointer<record_type>(GET_NODE(tree_type->ptd))->name));
               if(td->name && GET_NODE(td->name)->get_kind() == identifier_node_K)
               {
                  auto* id = GetPointer<identifier_node>(GET_NODE(td->name));
                  if(id->strg == "va_list" || id->strg == "__va_list_tag")
                  {
                     res = "va_list";
                     break;
                  }
               }
            }
            res = "*";
            const auto quals = tree_type->qual;
            /* const internally are not considered as constant...*/
            if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            {
               tree_nodeRef node;
               if(var)
               {
                  node = TM->get_tree_node_const(var);
               }
               if(node and print_qualifiers)
               {
                  res += tree_helper::return_C_qualifiers(quals, true);
               }
               else
               {
                  res += tree_helper::return_C_qualifiers(quals, false);
               }
            }
            ptd_type = GET_INDEX_NODE(tree_type->ptd);
            res = print_type(TM, ptd_type, global, print_qualifiers, print_storage, var, vppf, prefix + res, tail);
            skip_var_printing = true;
            break;
         }
         else
         {
            res = "/*&*/*"; /// references are translated as pointer type objects
            auto* tree_type = GetPointer<reference_type>(node_type);
            ptd_type = GET_INDEX_NODE(tree_type->refd);
            res = print_type(TM, ptd_type, global, print_qualifiers, print_storage, var, vppf, prefix + res, tail);
            skip_var_printing = true;
            break;
         }
         // res +=print_type(TM, ptd_type) + " " + res;
         break;
      }
      case function_type_K:
      {
         auto* ft = GetPointer<function_type>(node_type);
         res += print_type(TM, GET_INDEX_NODE(ft->retn), global, true);
         res += "(" + prefix;
         if(var)
         {
            THROW_ASSERT(vppf, "expected a functor");
            res += " " + (*vppf)(var);
         }
         res += tail + ")(";
         if(ft->prms)
         {
            res += print_type(TM, GET_INDEX_NODE(ft->prms), global, true);
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
         auto* mt = GetPointer<method_type>(node_type);
         res += print_type(TM, GET_INDEX_NODE(mt->clas), global);
         res += "::";
         res += print_type(TM, GET_INDEX_NODE(mt->retn), global);
         res += "(" + prefix;
         if(var)
         {
            THROW_ASSERT(vppf, "expected a functor");
            res += " " + (*vppf)(var);
         }
         res += tail + ")(";
         if(mt->prms)
         {
            res += print_type(TM, GET_INDEX_NODE(mt->prms), global);
         }
         res += ")";
         skip_var_printing = true;
         break;
      }
      case array_type_K:
      {
         const array_type* at = GetPointer<array_type>(node_type);
         if(at->name)
         {
            res += print_type(TM, GET_INDEX_NODE(at->name));
         }
         else
         {
            std::string local_prefix;
            std::string local_tail;
            /* Print array's type */
            /// Compute the dimensions
            if(at->size)
            {
               tree_nodeRef array_length = GET_NODE(at->size);
               tree_nodeRef array_t = GET_NODE(at->elts);
               if(array_length->get_kind() == integer_cst_K)
               {
                  auto* arr_ic = GetPointer<integer_cst>(array_length);
                  auto* tn = GetPointer<type_node>(array_t);
                  auto* eln_ic = GetPointer<integer_cst>(GET_NODE(tn->size));
                  local_tail += "[";
                  local_tail += boost::lexical_cast<std::string>(tree_helper::get_integer_cst_value(arr_ic) / tree_helper::get_integer_cst_value(eln_ic));
                  local_tail += "]";
               }
               else if(array_length->get_kind() == var_decl_K)
               {
                  local_tail += "[";
                  local_tail += "]";
               }
               else
               {
                  THROW_ERROR("array print_type not supported " + STR(GET_INDEX_NODE(at->size)));
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
               local_prefix += " " + (*vppf)(var);
            }
            if(!prefix.empty())
            {
               local_tail = ")" + local_tail;
            }

            res += print_type(TM, GET_INDEX_NODE(at->elts), global, print_qualifiers, print_storage, 0, var_pp_functorConstRef(), "", local_prefix + tail + local_tail);
            /// add alignment
            if(var && TM->get_tree_node_const(var)->get_kind() == field_decl_K)
            {
               unsigned int type_align = at->algn;
               const tree_nodeRef node_var = TM->get_tree_node_const(var);
               unsigned int var_align;
               bool is_a_pointerP = false;
               bool is_static = false;
               switch(node_var->get_kind())
               {
                  case field_decl_K:
                     var_align = GetPointer<field_decl>(node_var)->algn;
                     is_a_pointerP = GET_NODE(GetPointer<field_decl>(node_var)->type)->get_kind() == pointer_type_K || GET_NODE(GetPointer<field_decl>(node_var)->type)->get_kind() == reference_type_K;
                     break;
                  case parm_decl_K:
                     // var_align = GetPointer<parm_decl>(node_var)->algn;
                     var_align = type_align;
                     break;
                  case result_decl_K:
                     // var_align = GetPointer<result_decl>(node_var)->algn;
                     var_align = type_align;
                     break;
                  case var_decl_K:
                     var_align = GetPointer<var_decl>(node_var)->algn;
                     is_a_pointerP = GET_NODE(GetPointer<var_decl>(node_var)->type)->get_kind() == pointer_type_K || GET_NODE(GetPointer<var_decl>(node_var)->type)->get_kind() == reference_type_K;
                     is_static = GetPointer<var_decl>(node_var)->static_flag;
                     break;
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
                  res += " __attribute__((aligned(" + boost::lexical_cast<std::string>(var_align / 8) + ")))";
               }
            }
            skip_var_printing = true;
         }
         break;
      }
      case component_ref_K:
      {
         auto* cr = GetPointer<component_ref>(node_type);
         res += print_type(TM, GET_INDEX_NODE(cr->type), global);
         break;
      }
      case enumeral_type_K:
      {
         auto* et = GetPointer<enumeral_type>(node_type);
         if(et->name and (GET_NODE(et->name)->get_kind() == type_decl_K or et->unql))
         {
            res += print_type(TM, GET_INDEX_NODE(et->name), global);
         }
         else if(et->name)
         {
            res += "enum " + print_type(TM, GET_INDEX_NODE(et->name), global);
         }
         else if(et->unql)
         {
            res += "Internal_" + boost::lexical_cast<std::string>(type);
         }
         else
         {
            res += "enum Internal_" + boost::lexical_cast<std::string>(type);
         }
         break;
      }
      case record_type_K:
      {
         auto* rt = GetPointer<record_type>(node_type);
         const auto quals = rt->qual;
         if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN and print_qualifiers)
         {
            res += tree_helper::return_C_qualifiers(quals, true);
         }
         if(rt->name and (GET_NODE(rt->name)->get_kind() == type_decl_K or (rt->unql && !rt->system_flag)))
         {
            res += print_type(TM, GET_INDEX_NODE(rt->name), global);
         }
         else if(rt->name)
         {
            std::string struct_name = print_type(TM, GET_INDEX_NODE(rt->name), global);
            if(struct_name == "_IO_FILE")
            {
               res += "FILE";
            }
            else
            {
               res += "struct " + normalized_ID(struct_name);
            }
         }
         else if(rt->unql)
         {
            res += "Internal_" + boost::lexical_cast<std::string>(type);
         }
         else
         {
            res += "struct Internal_" + boost::lexical_cast<std::string>(type);
         }
         break;
      }
      case union_type_K:
      {
         auto* ut = GetPointer<union_type>(node_type);
         auto const quals = ut->qual;
         if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN and print_qualifiers)
         {
            res += tree_helper::return_C_qualifiers(quals, true);
         }
         if(ut->name and (GET_NODE(ut->name)->get_kind() == type_decl_K or (ut->unql && !ut->system_flag)))
         {
            res += print_type(TM, GET_INDEX_NODE(ut->name), global);
         }
         else if(ut->name)
         {
            res += "union " + print_type(TM, GET_INDEX_NODE(ut->name), global);
         }
         else if(ut->unql)
         {
            res += "Internal_" + boost::lexical_cast<std::string>(type);
         }
         else
         {
            res += "union Internal_" + boost::lexical_cast<std::string>(type);
         }
         break;
      }
      case tree_list_K:
      {
         THROW_ASSERT(var == 0, "Received something of unexpected");
         auto* lnode = GetPointer<tree_list>(node_type);
         res += print_type(TM, GET_INDEX_NODE(lnode->valu), global, print_qualifiers);
         /// tree_list are used for parameters declaration: in that case void_type has to be removed from the last type parameter
         std::list<tree_nodeRef> prmtrs;
         while(lnode->chan)
         {
            lnode = GetPointer<tree_list>(GET_NODE(lnode->chan));
            if(!GetPointer<void_type>(GET_NODE(lnode->valu)))
               prmtrs.push_back(lnode->valu);
         }
         for(auto valu : prmtrs)
            res += "," + print_type(TM, GET_INDEX_NODE(valu), global, print_qualifiers);
         break;
      }
      case template_type_parm_K:
      {
         auto* ttp = GetPointer<template_type_parm>(node_type);
         res += print_type(TM, GET_INDEX_NODE(ttp->name), global, print_qualifiers);
         break;
      }
      case typename_type_K:
      {
         auto* tt = GetPointer<typename_type>(node_type);
         res += print_type(TM, GET_INDEX_NODE(tt->name), global, print_qualifiers);
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
      case parm_decl_K:
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
      case template_decl_K:
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
      case bit_ior_concat_expr_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_UNREACHABLE("Type not yet supported " + boost::lexical_cast<std::string>(original_type) + " " + node_type->get_kind_text() + " " + boost::lexical_cast<std::string>(var));
   }
   if(!skip_var_printing)
   {
      res += prefix;
      if(var)
      {
         THROW_ASSERT(vppf, "expected a functor");
         res += " " + (*vppf)(var);
      }
      res += tail;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Printed type " + boost::lexical_cast<std::string>(original_type) + ": " + res);
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
   tree_nodeRef curr_tn;
   if(var->get_kind() == tree_reindex_K)
   {
      curr_tn = GET_NODE(var);
   }
   else
   {
      curr_tn = var;
   }
   THROW_ASSERT(GetPointer<decl_node>(curr_tn), "Checking type of not a decl_node");
   auto* dn = GetPointer<decl_node>(curr_tn);
   std::string include_name = dn->include_name;
   auto it_end = headers.end();
   for(auto it = headers.begin(); it != it_end; ++it)
   {
      if(include_name.find(*it) != std::string::npos && dn->type)
      {
         if(GetPointer<type_node>(GET_NODE(dn->type)))
         {
            lib_types.insert(GET_NODE(dn->type));
            auto* tn = GetPointer<type_node>(GET_NODE(dn->type));
            if(tn && tn->unql)
            {
               lib_types.insert(GET_NODE(tn->unql));
            }
         }
      }
   }
}

bool FunctionExpander::operator()(const tree_nodeRef& tn) const
{
   tree_nodeRef curr_tn;
   if(tn->get_kind() == tree_reindex_K)
   {
      curr_tn = GET_NODE(tn);
   }
   else
   {
      curr_tn = tn;
   }
   THROW_ASSERT(GetPointer<type_node>(curr_tn) || GetPointer<function_decl>(curr_tn), "tn is not a node of type type_node nor function_decl");
   if(lib_types.find(curr_tn) != lib_types.end())
   {
      return false;
   }
   if(curr_tn->get_kind() == record_type_K)
   {
      auto* rt = GetPointer<record_type>(curr_tn);
      if(rt->ptrmem_flag)
      {
         return false;
      }
   }
   auto* type = GetPointer<type_node>(curr_tn);
   if(type && type->name)
   {
      auto* td = GetPointer<type_decl>(GET_NODE(type->name));
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

unsigned int tree_helper::get_formal_ith(const tree_managerConstRef& TM, unsigned int index_obj, unsigned int parm_index)
{
   const tree_nodeRef t = TM->get_tree_node_const(index_obj);
   if(t->get_kind() == gimple_call_K)
   {
      auto* gc = GetPointer<gimple_call>(t);

      unsigned int fn_type_index;
      const tree_nodeRef fn_type = get_type_node(GET_NODE(gc->fn), fn_type_index);
      if(fn_type->get_kind() == pointer_type_K)
      {
         auto* pt = GetPointer<pointer_type>(fn_type);
         THROW_ASSERT(pt->ptd, "unexpected pattern");
         auto* ft = GetPointer<function_type>(GET_NODE(pt->ptd));
         if(ft->varargs_flag)
         {
            return 0;
         }
         else if(ft->prms)
         {
            auto* tl = GetPointer<tree_list>(GET_NODE(ft->prms));
            unsigned int ith = 0;
            if(parm_index == ith)
            {
               return GET_INDEX_NODE(tl->valu);
            }
            while(tl->chan)
            {
               ++ith;
               tl = GetPointer<tree_list>(GET_NODE(tl->chan));
               if(parm_index == ith)
               {
                  return GET_INDEX_NODE(tl->valu);
               }
            }
            THROW_ERROR("unexpected pattern");
            return 0;
         }
         else
         {
            const tree_nodeRef fn_node = GET_NODE(gc->fn);
            /// parameters are not available through function_type but only through function_decl
            THROW_ASSERT(fn_node->get_kind() == addr_expr_K, "Unexpected pattern");
            auto* ue = GetPointer<unary_expr>(fn_node);
            tree_nodeRef fn = GET_NODE(ue->op);
            THROW_ASSERT(fn->get_kind() == function_decl_K, "Unexpected pattern");
            auto* fd = GetPointer<function_decl>(fn);
            return get_formal_ith(TM, fd->index, parm_index);
         }
      }
      else
      {
         THROW_ERROR("unexpected pattern");
         return 0;
      }
   }
   else if(t->get_kind() == gimple_assign_K)
   {
      auto* ga = GetPointer<gimple_assign>(t);
      return get_formal_ith(TM, GET_INDEX_NODE(ga->op1), parm_index);
   }
   else if(t->get_kind() == call_expr_K || t->get_kind() == aggr_init_expr_K)
   {
      auto* ce = GetPointer<call_expr>(t);
      unsigned int fn_type_index;
      const tree_nodeRef fn_type = get_type_node(GET_NODE(ce->fn), fn_type_index);
      if(fn_type->get_kind() == pointer_type_K)
      {
         auto* pt = GetPointer<pointer_type>(fn_type);
         THROW_ASSERT(pt->ptd, "unexpected pattern");
         auto* ft = GetPointer<function_type>(GET_NODE(pt->ptd));
         if(ft->varargs_flag)
         {
            return 0;
         }
         else if(ft->prms)
         {
            auto* tl = GetPointer<tree_list>(GET_NODE(ft->prms));
            unsigned int ith = 0;
            if(parm_index == ith)
            {
               return GET_INDEX_NODE(tl->valu);
            }
            while(tl->chan)
            {
               ++ith;
               tl = GetPointer<tree_list>(GET_NODE(tl->chan));
               if(parm_index == ith)
               {
                  return GET_INDEX_NODE(tl->valu);
               }
            }
            THROW_ERROR("unexpected pattern");
            return 0;
         }
         else
         {
            const tree_nodeRef fn_node = GET_NODE(ce->fn);
            /// parameters are not available through function_type but only through function_decl
            THROW_ASSERT(fn_node->get_kind() == addr_expr_K, "Unexpected pattern");
            auto* ue = GetPointer<unary_expr>(fn_node);
            tree_nodeRef fn = GET_NODE(ue->op);
            THROW_ASSERT(fn->get_kind(), "Unexpected pattern");
            auto* fd = GetPointer<function_decl>(fn);
            return get_formal_ith(TM, fd->index, parm_index);
         }
      }
      else
      {
         THROW_ERROR("unexpected pattern");
         return 0;
      }
   }
   else if(t->get_kind() == function_decl_K)
   {
      auto* fd = GetPointer<function_decl>(t);
      unsigned int ith = 0;
      for(const auto& i : fd->list_of_args)
      {
         if(parm_index == ith)
         {
            return get_type_index(TM, GET_INDEX_NODE(i));
         }
         ++ith;
      }
   }
   return 0;
}

bool tree_helper::is_packed(const tree_managerConstRef& TreeM, unsigned int node_index)
{
   const tree_nodeRef node = TreeM->get_tree_node_const(node_index);
   THROW_ASSERT(GetPointer<decl_node>(node), "unexpected pattern" + node->get_kind_text());
   if(GetPointer<decl_node>(node)->packed_flag)
   {
      return true;
   }
   tree_nodeRef node_type = GET_NODE(GetPointer<decl_node>(node)->type);
   if(GetPointer<type_decl>(node_type))
   {
      node_type = GET_NODE(GetPointer<type_decl>(node_type)->type);
   }
   switch(node_type->get_kind())
   {
      case record_type_K:
      {
         auto* rt = GetPointer<record_type>(node_type);
         if(rt->unql)
         {
            rt = GetPointer<record_type>(GET_NODE(rt->unql));
         }
         THROW_ASSERT(not rt->unql, "unexpected pattern");
         if(rt->packed_flag)
         {
            return true;
         }
         for(auto& list_of_fld : rt->list_of_flds)
         {
            const field_decl* fd = GetPointer<field_decl>(GET_NODE(list_of_fld));
            if(fd && fd->packed_flag)
            {
               return true;
            }
         }
         break;
      }
      case union_type_K:
      {
         auto* ut = GetPointer<union_type>(node_type);
         if(ut->unql)
         {
            ut = GetPointer<union_type>(GET_NODE(ut->unql));
         }
         THROW_ASSERT(not ut->unql, "unexpected pattern");
         if(ut->packed_flag)
         {
            return true;
         }

         /// Print the contents of the structure
         for(auto& list_of_fld : ut->list_of_flds)
         {
            const field_decl* fd = GetPointer<field_decl>(GET_NODE(list_of_fld));

            if(fd->packed_flag)
            {
               return true;
            }
         }
         break;
      }
      case enumeral_type_K:
      {
         const enumeral_type* et = GetPointer<enumeral_type>(node_type);
         if(et->unql)
         {
            et = GetPointer<enumeral_type>(GET_NODE(et->unql));
         }
         THROW_ASSERT(not et->unql, "unexpected pattern");
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
   const tree_nodeRef t = TreeM->get_tree_node_const(node_index);
   bool res = false;
   switch(t->get_kind())
   {
      case mem_ref_K:
      {
         auto* mr = GetPointer<mem_ref>(t);
         return is_packed_access(TreeM, GET_INDEX_NODE(mr->op0));
      }
      case target_mem_ref461_K:
      {
         auto* tmr = GetPointer<target_mem_ref461>(t);
         return is_packed_access(TreeM, GET_INDEX_NODE(tmr->base));
      }
      case component_ref_K:
      {
         auto* cr = GetPointer<component_ref>(t);
         tree_nodeRef fd = GET_NODE(cr->op1);
         if(GetPointer<field_decl>(fd) && GetPointer<field_decl>(fd)->packed_flag)
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
         auto* ae = GetPointer<addr_expr>(t);
         return is_packed_access(TreeM, GET_INDEX_NODE(ae->op));
      }
      case ssa_name_K:
      {
         auto* sn = GetPointer<ssa_name>(t);
         auto def_stmt = sn->CGetDefStmt();
         if(GET_NODE(def_stmt)->get_kind() == gimple_assign_K)
         {
            auto ga = GetPointer<gimple_assign>(GET_NODE(def_stmt));
            if(ga->temporary_address)
            {
               return is_packed_access(TreeM, GET_INDEX_NODE(ga->op1));
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
      case bit_ior_concat_expr_K:
      case abs_expr_K:
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
      {
         res = false;
         break;
      }
      default:
         THROW_ERROR("elements not yet supported: " + t->get_kind_text());
   }

   return res;
}

void tree_helper::accessed_greatest_bitsize(const tree_managerConstRef& TreeM, const tree_nodeRef& type_node, unsigned int type_index, unsigned int& bitsize)
{
   switch(type_node->get_kind())
   {
      case array_type_K:
      {
         auto* atype = GetPointer<array_type>(type_node);
         accessed_greatest_bitsize(TreeM, GET_NODE(atype->elts), GET_INDEX_NODE(atype->elts), bitsize);
         break;
      }
      case record_type_K:
      {
         auto* rt = GetPointer<record_type>(type_node);
         std::vector<tree_nodeRef> field_list = rt->list_of_flds;
         auto flend = field_list.end();
         for(auto fli = field_list.begin(); fli != flend; ++fli)
         {
            if(GET_NODE(*fli)->get_kind() == type_decl_K)
            {
               continue;
            }
            if(GET_NODE(*fli)->get_kind() == const_decl_K)
            {
               continue;
            }
            if(GET_NODE(*fli)->get_kind() == template_decl_K)
            {
               continue;
            }
            if(GET_NODE(*fli)->get_kind() == function_decl_K)
            {
               continue;
            }
            if(GET_NODE(*fli)->get_kind() == var_decl_K)
            {
               accessed_greatest_bitsize(TreeM, GET_NODE(GetPointer<var_decl>(GET_NODE(*fli))->type), GET_INDEX_NODE(GetPointer<var_decl>(GET_NODE(*fli))->type), bitsize);
            }
            else
            {
               accessed_greatest_bitsize(TreeM, GET_NODE(*fli), GET_INDEX_NODE(*fli), bitsize);
            }
         }
         break;
      }
      case union_type_K:
      {
         auto* ut = GetPointer<union_type>(type_node);
         std::vector<tree_nodeRef> field_list = ut->list_of_flds;
         auto flend = field_list.end();
         for(auto fli = field_list.begin(); fli != flend; ++fli)
         {
            accessed_greatest_bitsize(TreeM, GET_NODE(*fli), GET_INDEX_NODE(*fli), bitsize);
         }
         break;
      }
      case field_decl_K:
      {
         unsigned int fd_type_index;
         tree_nodeRef fd_type_node = get_type_node(type_node, fd_type_index);
         accessed_greatest_bitsize(TreeM, fd_type_node, fd_type_index, bitsize);
         break;
      }
      case complex_type_K:
      {
         bitsize = std::max(bitsize, size(TreeM, type_index) / 2); /// it is composed by two identical parts
         break;
      }
      case real_type_K:
      case integer_type_K:
      case enumeral_type_K:
      case pointer_type_K:
      case reference_type_K:
      case void_type_K:
      case vector_type_K:
      {
         bitsize = std::max(bitsize, size(TreeM, type_index));
         break;
      }
      case function_decl_K:
      case function_type_K:
      case method_type_K:
      {
         bitsize = 32;
         break;
      }
      case boolean_type_K:
      {
         bitsize = 8;
         break;
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
}

void tree_helper::accessed_minimum_bitsize(const tree_managerConstRef& TreeM, const tree_nodeRef& type_node, unsigned int type_index, unsigned int& bitsize)
{
   switch(type_node->get_kind())
   {
      case array_type_K:
      {
         auto* atype = GetPointer<array_type>(type_node);
         accessed_minimum_bitsize(TreeM, GET_NODE(atype->elts), GET_INDEX_NODE(atype->elts), bitsize);
         break;
      }
      case record_type_K:
      {
         auto* rt = GetPointer<record_type>(type_node);
         std::vector<tree_nodeRef> field_list = rt->list_of_flds;
         auto flend = field_list.end();
         for(auto fli = field_list.begin(); fli != flend; ++fli)
         {
            if(GET_NODE(*fli)->get_kind() == type_decl_K)
            {
               continue;
            }
            if(GET_NODE(*fli)->get_kind() == const_decl_K)
            {
               continue;
            }
            if(GET_NODE(*fli)->get_kind() == template_decl_K)
            {
               continue;
            }
            if(GET_NODE(*fli)->get_kind() == function_decl_K)
            {
               continue;
            }
            if(GET_NODE(*fli)->get_kind() == var_decl_K)
            {
               accessed_minimum_bitsize(TreeM, GET_NODE(GetPointer<var_decl>(GET_NODE(*fli))->type), GET_INDEX_NODE(GetPointer<var_decl>(GET_NODE(*fli))->type), bitsize);
            }
            else
            {
               accessed_minimum_bitsize(TreeM, GET_NODE(*fli), GET_INDEX_NODE(*fli), bitsize);
            }
         }
         break;
      }
      case union_type_K:
      {
         auto* ut = GetPointer<union_type>(type_node);
         std::vector<tree_nodeRef> field_list = ut->list_of_flds;
         auto flend = field_list.end();
         for(auto fli = field_list.begin(); fli != flend; ++fli)
         {
            accessed_minimum_bitsize(TreeM, GET_NODE(*fli), GET_INDEX_NODE(*fli), bitsize);
         }
         break;
      }
      case field_decl_K:
      {
         unsigned int fd_type_index;
         tree_nodeRef fd_type_node = get_type_node(type_node, fd_type_index);
         accessed_minimum_bitsize(TreeM, fd_type_node, fd_type_index, bitsize);
         break;
      }
      case complex_type_K:
      {
         bitsize = std::min(bitsize, size(TreeM, type_index) / 2); /// it is composed by two identical parts
         break;
      }
      case real_type_K:
      case integer_type_K:
      case enumeral_type_K:
      case pointer_type_K:
      case reference_type_K:
      case void_type_K:
      case vector_type_K:
      {
         bitsize = std::min(bitsize, size(TreeM, type_index));
         break;
      }
      case boolean_type_K:
      {
         bitsize = 8;
         break;
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
}

size_t tree_helper::AllocatedMemorySize(const tree_nodeConstRef& parameter)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + parameter->ToString());
   switch(parameter->get_kind())
   {
      case(addr_expr_K):
      {
         const auto* ae = GetPointer<const addr_expr>(parameter);
         /// Note that this part can not be transfromed in recursion because size of array ref corresponds to the size of the element itself
         const tree_nodeRef addr_expr_argument = GET_NODE(ae->op);
         switch(addr_expr_argument->get_kind())
         {
            case(array_ref_K):
            {
               const array_ref* ar = GetPointer<array_ref>(addr_expr_argument);
               const size_t byte_parameter_size = AllocatedMemorySize(GET_NODE(ar->op0));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(byte_parameter_size));
               return byte_parameter_size;
            }
            case(component_ref_K):
            case(mem_ref_K):
            case(parm_decl_K):
            case(string_cst_K):
            case(var_decl_K):
            {
               const size_t byte_parameter_size = AllocatedMemorySize(addr_expr_argument);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(byte_parameter_size));
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
         const auto* at = GetPointer<const array_type>(parameter);
         /// This call check if we can perform deep copy of the single element
         AllocatedMemorySize(GET_NODE(at->elts));
         const size_t bit_parameter_size = tree_helper::Size(parameter);
         /// Round to upper multiple word size
         const size_t byte_parameter_size = bit_parameter_size / 8;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(byte_parameter_size));
         return byte_parameter_size;
      }
      case(record_type_K):
      {
         size_t fields_pointed_size = 0;
         const auto* rt = GetPointer<const record_type>(parameter);
         const std::vector<tree_nodeRef>& list_of_fields = rt->list_of_flds;
         /// This calls check if we can perform deep copy of the single element
         std::vector<tree_nodeRef>::const_iterator field, field_end = list_of_fields.end();
         for(field = list_of_fields.begin(); field != field_end; ++field)
         {
            if(GET_NODE(*field)->get_kind() == type_decl_K)
            {
               continue;
            }
            if(GET_NODE(*field)->get_kind() == function_decl_K)
            {
               continue;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing field " + (*field)->ToString());
            AllocatedMemorySize(tree_helper::get_type_node(GET_NODE(*field)));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed field " + (*field)->ToString());
         }
         const size_t bit_parameter_size = tree_helper::Size(parameter) + fields_pointed_size;
         /// Round to upper multiple word size
         const size_t byte_parameter_size = bit_parameter_size / 8;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(byte_parameter_size));
         return byte_parameter_size;
      }
      case(component_ref_K):
      {
         const auto* cr = GetPointer<const component_ref>(parameter);
         if(GetPointer<const union_type>(get_type_node(GET_NODE(cr->op0))))
         {
            THROW_ERROR("Offloading fields of union is not supported");
         }
         const size_t byte_parameter_size = AllocatedMemorySize(GET_NODE(cr->op1));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(byte_parameter_size));
         return byte_parameter_size;
      }
      case(field_decl_K):
      {
         const size_t byte_parameter_size = AllocatedMemorySize(tree_helper::CGetType(parameter));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(byte_parameter_size));
         return byte_parameter_size;
      }
      case(enumeral_type_K):
      case(integer_type_K):
      case(real_type_K):
      case(string_cst_K):
      {
         const size_t bit_parameter_size = tree_helper::Size(parameter);
         /// Round to upper multiple word size
         const size_t byte_parameter_size = bit_parameter_size / 8;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(byte_parameter_size));
         return byte_parameter_size;
      }
      case(mem_ref_K):
      {
         const auto* mr = GetPointer<const mem_ref>(parameter);
         const size_t byte_parameter_size = AllocatedMemorySize(GET_NODE(mr->op0));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(byte_parameter_size));
         return byte_parameter_size;
      }
      case(parm_decl_K):
      case(ssa_name_K):
      case(var_decl_K):
      {
         const auto* sn = GetPointer<const ssa_name>(parameter);
         if(sn and (GET_NODE(sn->var)->get_kind() == parm_decl_K) and sn->CGetDefStmts().empty())
         {
            return AllocatedMemorySize(GET_NODE(sn->var));
         }

         const tree_nodeConstRef type = tree_helper::CGetType(parameter);
         if(type->get_kind() == pointer_type_K)
         {
            const size_t point_to_size = GetPointer<const parm_decl>(parameter) ? GetPointer<const parm_decl>(parameter)->point_to_information->point_to_size[PointToInformation::default_key] / 8 :
                                                                                  (GetPointer<const ssa_name>(parameter) ? GetPointer<const ssa_name>(parameter)->point_to_information->point_to_size[PointToInformation::default_key] / 8 :
                                                                                                                           GetPointer<const var_decl>(parameter)->point_to_information->point_to_size[PointToInformation::default_key] / 8);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(point_to_size / 8));
            return point_to_size;
         }
         else
         {
            const size_t byte_parameter_size = AllocatedMemorySize(tree_helper::CGetType(parameter));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + parameter->ToString() + " - Size is " + boost::lexical_cast<std::string>(byte_parameter_size));
            return byte_parameter_size;
         }
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
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      default:
      {
         THROW_UNREACHABLE("Unsupported tree node type " + parameter->get_kind_text() + " (" + parameter->ToString() + ")");
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
         return CountPointers(tree_helper::CGetType(tn));
      }
      case reference_type_K:
      case pointer_type_K:
      {
         return 1;
      }
      case record_type_K:
      {
         const auto* rt = GetPointer<const record_type>(tn);
         const std::vector<tree_nodeRef> list_of_fields = rt->list_of_flds;
         std::vector<tree_nodeRef>::const_iterator field, field_end = list_of_fields.end();
         for(field = list_of_fields.begin(); field != field_end; ++field)
         {
            if(GET_NODE(*field)->get_kind() == type_decl_K)
            {
               continue;
            }
            if(GET_NODE(*field)->get_kind() == function_decl_K)
            {
               continue;
            }
            counter += CountPointers(GET_NODE(*field));
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

unsigned int tree_helper::get_multi_way_if_pos(const tree_managerConstRef& TM, unsigned int node_id, unsigned int looked_for_cond)
{
   const tree_nodeRef t = TM->get_tree_node_const(node_id);
   auto* gmwi = GetPointer<gimple_multi_way_if>(t);
   unsigned int pos = 0;
   for(auto const& cond : gmwi->list_of_cond)
   {
      if(cond.first and cond.first->index == looked_for_cond)
      {
         return pos;
      }
      pos++;
   }
   THROW_ERROR("cond not found in gimple_multi_way_if");
   return pos;
}

void tree_helper::compute_ssa_uses_rec_ptr(const tree_nodeRef& curr_tn, CustomOrderedSet<ssa_name*>& ssa_uses)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->compute_ssa_uses_rec_ptr " + curr_tn->ToString());
   auto* gn = GetPointer<gimple_node>(curr_tn);
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
      case tree_reindex_K:
      {
         compute_ssa_uses_rec_ptr(GET_NODE(curr_tn), ssa_uses);
         break;
      }
      case gimple_return_K:
      {
         auto* re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
         {
            compute_ssa_uses_rec_ptr(re->op, ssa_uses);
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* me = GetPointer<gimple_assign>(curr_tn);
         if(GET_NODE(me->op0)->get_kind() != ssa_name_K)
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
         auto* ce = GetPointer<call_expr>(curr_tn);
         compute_ssa_uses_rec_ptr(ce->fn, ssa_uses);
         std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            compute_ssa_uses_rec_ptr(*arg, ssa_uses);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointer<gimple_call>(curr_tn);
         compute_ssa_uses_rec_ptr(ce->fn, ssa_uses);
         std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            compute_ssa_uses_rec_ptr(*arg, ssa_uses);
         }
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(curr_tn);
         compute_ssa_uses_rec_ptr(gc->op0, ssa_uses);
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointer<unary_expr>(curr_tn);
         compute_ssa_uses_rec_ptr(ue->op, ssa_uses);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(curr_tn);
         compute_ssa_uses_rec_ptr(be->op0, ssa_uses);
         compute_ssa_uses_rec_ptr(be->op1, ssa_uses);
         break;
      }
      /*ternary expressions*/
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(curr_tn);
         compute_ssa_uses_rec_ptr(se->op0, ssa_uses);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         auto* te = GetPointer<ternary_expr>(curr_tn);
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
         auto* qe = GetPointer<quaternary_expr>(curr_tn);
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
         auto* le = GetPointer<lut_expr>(curr_tn);
         compute_ssa_uses_rec_ptr(le->op0, ssa_uses);
         compute_ssa_uses_rec_ptr(le->op1, ssa_uses);
         if(le->op2)
            compute_ssa_uses_rec_ptr(le->op2, ssa_uses);
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
         auto* c = GetPointer<constructor>(curr_tn);
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = c->list_of_idx_valu;
         auto vend = list_of_idx_valu.end();
         for(auto i = list_of_idx_valu.begin(); i != vend; ++i)
         {
            compute_ssa_uses_rec_ptr(i->second, ssa_uses);
         }
         break;
      }
      case var_decl_K:
      {
         /// var decl performs an assignment when init is not null
         // var_decl * vd = GetPointer<var_decl>(curr_tn);
         // if(vd->init)
         //   compute_ssa_uses_rec_ptr(vd->init, ssa_uses);
         break;
      }
      case gimple_asm_K:
      {
         auto* ae = GetPointer<gimple_asm>(curr_tn);
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
         auto* ge = GetPointer<gimple_goto>(curr_tn);
         compute_ssa_uses_rec_ptr(ge->op, ssa_uses);
         break;
      }
      case tree_list_K:
      {
         auto* tl = GetPointer<tree_list>(curr_tn);
         std::list<tree_list*> tl_list;
         do
         {
            tl_list.push_back(tl);
            tl = tl->chan ? GetPointer<tree_list>(GET_NODE(tl->chan)) : nullptr;
         } while(tl);
         for(auto tl_current0 : tl_list)
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
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(auto cond : gmwi->list_of_cond)
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
         auto* tmr = GetPointer<target_mem_ref>(curr_tn);
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
         auto* tmr = GetPointer<target_mem_ref461>(curr_tn);
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
         ssa_uses.insert(GetPointer<ssa_name>(curr_tn));
         break;
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case CASE_CPP_NODES:
      case const_decl_K:
      case last_tree_K:
      case none_K:
      case placeholder_expr_K:
      case gimple_for_K:
      case gimple_bind_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_pragma_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case translation_unit_decl_K:
      case using_decl_K:
      case tree_vec_K:
      case type_decl_K:
      case CASE_TYPE_NODES:
      case error_mark_K:
      case gimple_while_K:
      case template_decl_K:
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

void tree_helper::ComputeSsaUses(const tree_nodeRef tn, TreeNodeMap<size_t>& ssa_uses)
{
   THROW_ASSERT(tn->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   const auto curr_tn = GET_NODE(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Computing ssa uses in " + curr_tn->ToString() + " (" + curr_tn->get_kind_text() + ")");
   const auto* gn = GetPointer<const gimple_node>(curr_tn);
   if(gn)
   {
      if(gn->memuse)
      {
         ComputeSsaUses(gn->memuse, ssa_uses);
      }
      if(!gn->vuses.empty())
      {
         for(const auto& vuse : gn->vuses)
         {
            ComputeSsaUses(vuse, ssa_uses);
         }
      }
   }

   switch(curr_tn->get_kind())
   {
      case gimple_return_K:
      {
         auto* re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
         {
            ComputeSsaUses(re->op, ssa_uses);
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* me = GetPointer<gimple_assign>(curr_tn);
         if(GET_NODE(me->op0)->get_kind() != ssa_name_K)
         {
            ComputeSsaUses(me->op0, ssa_uses);
         }
         ComputeSsaUses(me->op1, ssa_uses);
         if(me->predicate)
         {
            ComputeSsaUses(me->predicate, ssa_uses);
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
         auto* ce = GetPointer<call_expr>(curr_tn);
         ComputeSsaUses(ce->fn, ssa_uses);
         std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            ComputeSsaUses(*arg, ssa_uses);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointer<gimple_call>(curr_tn);
         ComputeSsaUses(ce->fn, ssa_uses);
         std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            ComputeSsaUses(*arg, ssa_uses);
         }
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(curr_tn);
         ComputeSsaUses(gc->op0, ssa_uses);
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointer<unary_expr>(curr_tn);
         if(GET_NODE(ue->op)->get_kind() != function_decl_K)
         {
            ComputeSsaUses(ue->op, ssa_uses);
         }
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(curr_tn);
         ComputeSsaUses(be->op0, ssa_uses);
         ComputeSsaUses(be->op1, ssa_uses);
         break;
      }
      /*ternary expressions*/
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(curr_tn);
         ComputeSsaUses(se->op0, ssa_uses);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         auto* te = GetPointer<ternary_expr>(curr_tn);
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
         auto* qe = GetPointer<quaternary_expr>(curr_tn);
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
         auto* le = GetPointer<lut_expr>(curr_tn);
         ComputeSsaUses(le->op0, ssa_uses);
         ComputeSsaUses(le->op1, ssa_uses);
         if(le->op2)
            ComputeSsaUses(le->op2, ssa_uses);
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
         auto* c = GetPointer<constructor>(curr_tn);
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = c->list_of_idx_valu;
         const auto vend = list_of_idx_valu.end();
         for(auto i = list_of_idx_valu.begin(); i != vend; ++i)
         {
            ComputeSsaUses(i->second, ssa_uses);
         }
         break;
      }
      case var_decl_K:
      {
         /// var decl performs an assignment when init is not null
         // var_decl * vd = GetPointer<var_decl>(curr_tn);
         // if(vd->init)
         //   ComputeSsaUses(vd->init, ssa_uses);
         break;
      }
      case gimple_asm_K:
      {
         auto* ae = GetPointer<gimple_asm>(curr_tn);
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
         auto* ge = GetPointer<gimple_goto>(curr_tn);
         ComputeSsaUses(ge->op, ssa_uses);
         break;
      }
      case tree_list_K:
      {
         auto* tl = GetPointer<tree_list>(curr_tn);
         std::list<tree_list*> tl_list;
         do
         {
            tl_list.push_back(tl);
            tl = tl->chan ? GetPointer<tree_list>(GET_NODE(tl->chan)) : nullptr;
         } while(tl);
         for(auto tl_current0 : tl_list)
         {
            if(tl_current0->purp)
            {
               ComputeSsaUses(tl_current0->purp, ssa_uses);
            }
            if(tl_current0->valu)
            {
               ComputeSsaUses(tl_current0->valu, ssa_uses);
            }
         }
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(auto& cond : gmwi->list_of_cond)
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
         auto* gp = GetPointer<gimple_phi>(curr_tn);
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
         auto* tmr = GetPointer<target_mem_ref>(curr_tn);
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
         auto* tmr = GetPointer<target_mem_ref461>(curr_tn);
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
         THROW_UNREACHABLE("Node is " + curr_tn->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--computed_ssa_uses_rec_ref " + boost::lexical_cast<std::string>(GET_INDEX_NODE(tn)));
}

bool tree_helper::is_a_nop_function_decl(function_decl* fd)
{
   if(fd->body)
   {
      auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
      if(not sl->list_of_stmt.empty())
      {
         return false;
      }
      else if(not sl->list_of_bloc.empty())
      {
         size_t bb_number = sl->list_of_bloc.size();
         if(sl->list_of_bloc.find(bloc::ENTRY_BLOCK_ID) != sl->list_of_bloc.end())
         {
            --bb_number;
         }
         if(sl->list_of_bloc.find(bloc::EXIT_BLOCK_ID) != sl->list_of_bloc.end())
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
         for(auto lob_it = sl->list_of_bloc.begin(); lob_it != sl->list_of_bloc.end(); ++lob_it)
         {
            if(lob_it->first != bloc::ENTRY_BLOCK_ID && lob_it->first != bloc::EXIT_BLOCK_ID)
            {
               single_bb = lob_it->second;
            }
         }
         THROW_ASSERT(single_bb, "unexpected condition");
         if(!single_bb->CGetStmtList().empty())
         {
            size_t stmt_number = single_bb->CGetStmtList().size();
            if(stmt_number > 1)
            {
               return false;
            }
            tree_nodeRef single_stmt = single_bb->CGetStmtList().front();
            auto* gr = GetPointer<gimple_return>(GET_NODE(single_stmt));
            if(gr)
            {
               return !static_cast<bool>(gr->op);
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

void tree_helper::get_required_values(const tree_managerConstRef& TM, std::vector<std::tuple<unsigned int, unsigned int>>& required, const tree_nodeRef& tn, unsigned int index)
{
   switch(tn->get_kind())
   {
      case constructor_K:
      {
         auto* co = GetPointer<constructor>(tn);
         if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(co->type)))
         {
            auto vend = co->list_of_idx_valu.end();
            for(auto i = co->list_of_idx_valu.begin(); i != vend; ++i)
            {
               required.emplace_back(GET_INDEX_NODE(i->second), 0);
            }
         }
         else
         {
            required.emplace_back(index, 0);
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
         required.emplace_back(index, 0);
         break;
      }
      case gimple_while_K:
      {
         const auto* we = GetPointer<const gimple_while>(tn);
         get_required_values(TM, required, GET_NODE(we->op0), GET_INDEX_NODE(we->op0));
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(tn);
         get_required_values(TM, required, GET_NODE(be->op0), GET_INDEX_NODE(be->op0));
         if(tn->get_kind() != assert_expr_K)
         {
            get_required_values(TM, required, GET_NODE(be->op1), GET_INDEX_NODE(be->op1));
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointer<unary_expr>(tn);
         if(tn->get_kind() == addr_expr_K /*|| tn->get_kind() == imagpart_expr_K || tn->get_kind() == realpart_expr_K*/)
         {
            required.emplace_back(index, 0);
         }
         else
         {
            get_required_values(TM, required, GET_NODE(ue->op), GET_INDEX_NODE(ue->op));
         }
         break;
      }
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      case bit_ior_concat_expr_K:
      case cond_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      {
         auto* te = GetPointer<ternary_expr>(tn);
         get_required_values(TM, required, GET_NODE(te->op0), GET_INDEX_NODE(te->op0));
         get_required_values(TM, required, GET_NODE(te->op1), GET_INDEX_NODE(te->op1));
         get_required_values(TM, required, GET_NODE(te->op2), GET_INDEX_NODE(te->op2));
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(tn);
         get_required_values(TM, required, GET_NODE(le->op0), GET_INDEX_NODE(le->op0));
         get_required_values(TM, required, GET_NODE(le->op1), GET_INDEX_NODE(le->op1));
         if(le->op2)
            get_required_values(TM, required, GET_NODE(le->op2), GET_INDEX_NODE(le->op2));
         if(le->op3)
            get_required_values(TM, required, GET_NODE(le->op3), GET_INDEX_NODE(le->op3));
         if(le->op4)
            get_required_values(TM, required, GET_NODE(le->op4), GET_INDEX_NODE(le->op4));
         if(le->op5)
            get_required_values(TM, required, GET_NODE(le->op5), GET_INDEX_NODE(le->op5));
         if(le->op6)
            get_required_values(TM, required, GET_NODE(le->op6), GET_INDEX_NODE(le->op6));
         if(le->op7)
            get_required_values(TM, required, GET_NODE(le->op7), GET_INDEX_NODE(le->op7));
         if(le->op8)
            get_required_values(TM, required, GET_NODE(le->op8), GET_INDEX_NODE(le->op8));
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(tn);
         get_required_values(TM, required, GET_NODE(gc->op0), GET_INDEX_NODE(gc->op0));
         break;
      }
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(tn);
         get_required_values(TM, required, GET_NODE(se->op0), GET_INDEX_NODE(se->op0));
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(tn);
         for(auto cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               get_required_values(TM, required, GET_NODE(cond.first), cond.first->index);
            }
         }
         break;
      }
      case array_ref_K:
      {
         auto* ar = GetPointer<array_ref>(tn);
         required.emplace_back(GET_INDEX_NODE(ar->op0), 0);
         get_required_values(TM, required, GET_NODE(ar->op1), GET_INDEX_NODE(ar->op1));
         break;
      }
      case target_mem_ref_K:
      {
         auto* tmr = GetPointer<target_mem_ref>(tn);
         if(tmr->symbol)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->symbol), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->base)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->base), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->idx)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->idx), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->step)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->step), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->offset)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->offset), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         break;
      }
      case target_mem_ref461_K:
      {
         auto* tmr = GetPointer<target_mem_ref461>(tn);
         if(tmr->base)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->base), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->idx)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->idx), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->step)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->step), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->idx2)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->idx2), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         if(tmr->offset)
         {
            required.emplace_back(GET_INDEX_NODE(tmr->offset), 0);
         }
         else
         {
            required.emplace_back(0, 0);
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(tn);
         if(!gm->init_assignment && !gm->clobber)
         {
            tree_nodeRef op0 = GET_NODE(gm->op0);
            tree_nodeRef op1 = GET_NODE(gm->op1);

            if(op0->get_kind() == component_ref_K || op0->get_kind() == indirect_ref_K || op0->get_kind() == misaligned_indirect_ref_K || op0->get_kind() == mem_ref_K || op0->get_kind() == array_ref_K || op0->get_kind() == target_mem_ref_K ||
               op0->get_kind() == target_mem_ref461_K || op0->get_kind() == bit_field_ref_K)
            {
               get_required_values(TM, required, op1, GET_INDEX_NODE(gm->op1));
               get_required_values(TM, required, op0, GET_INDEX_NODE(gm->op0));
            }
            else
            {
               bool is_a_vector_bitfield = false;
               if(op1->get_kind() == bit_field_ref_K)
               {
                  auto* bfr = GetPointer<bit_field_ref>(op1);
                  if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(bfr->op0)))
                  {
                     is_a_vector_bitfield = true;
                  }
               }
               if(op1->get_kind() == component_ref_K || op1->get_kind() == indirect_ref_K || op1->get_kind() == misaligned_indirect_ref_K || op1->get_kind() == mem_ref_K || op1->get_kind() == array_ref_K || op1->get_kind() == target_mem_ref_K ||
                  op1->get_kind() == target_mem_ref461_K || (op1->get_kind() == bit_field_ref_K && !is_a_vector_bitfield))
               {
                  required.emplace_back(0, 0);
               }
               get_required_values(TM, required, op1, GET_INDEX_NODE(gm->op1));
            }
            if(gm->predicate)
            {
               get_required_values(TM, required, GET_NODE(gm->predicate), gm->predicate->index);
            }
         }
         break;
      }
      case gimple_return_K:
      {
         auto* rt = GetPointer<gimple_return>(tn);
         if(rt->op)
         {
            get_required_values(TM, required, GET_NODE(rt->op), GET_INDEX_NODE(rt->op));
         }
         break;
      }
      case gimple_phi_K:
      {
         auto* gp = GetPointer<gimple_phi>(tn);
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            required.emplace_back(GET_INDEX_NODE(def_edge.first), 0);
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
         auto* ce = GetPointer<call_expr>(tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            required.emplace_back(GET_INDEX_NODE(*arg), 0);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointer<gimple_call>(tn);
         function_decl* fd = nullptr;
         tree_nodeRef temp_node = GET_NODE(ce->fn);
         if(temp_node->get_kind() == addr_expr_K)
         {
            auto* ue = GetPointer<unary_expr>(temp_node);
            temp_node = ue->op;
            fd = GetPointer<function_decl>(GET_NODE(temp_node));
         }
         else if(temp_node->get_kind() == obj_type_ref_K)
         {
            temp_node = tree_helper::find_obj_type_ref_function(ce->fn);
            fd = GetPointer<function_decl>(GET_NODE(temp_node));
         }
         if(!fd || !tree_helper::is_a_nop_function_decl(fd))
         {
            const std::vector<tree_nodeRef>& args = ce->args;
            std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
            for(arg = args.begin(); arg != arg_end; ++arg)
            {
               required.emplace_back(GET_INDEX_NODE(*arg), 0);
            }
         }
         break;
      }
      case tree_list_K:
      {
         auto* tl = GetPointer<tree_list>(tn);
         std::list<tree_list*> tl_list;
         do
         {
            tl_list.push_back(tl);
            tl = tl->chan ? GetPointer<tree_list>(GET_NODE(tl->chan)) : nullptr;
         } while(tl);
         for(auto tl_current0 : tl_list)
            required.emplace_back(GET_INDEX_NODE(tl_current0->valu), 0);
         break;
      }
      case component_ref_K:
      {
         auto* cr = GetPointer<component_ref>(tn);
         required.emplace_back(GET_INDEX_NODE(cr->op0), 0);
         get_required_values(TM, required, GET_NODE(cr->op1), GET_INDEX_NODE(cr->op1));
         break;
      }
      case bit_field_ref_K:
      {
         auto* bfr = GetPointer<bit_field_ref>(tn);
         required.emplace_back(GET_INDEX_NODE(bfr->op0), 0);
         get_required_values(TM, required, GET_NODE(bfr->op1), GET_INDEX_NODE(bfr->op1));
         get_required_values(TM, required, GET_NODE(bfr->op2), GET_INDEX_NODE(bfr->op2));
         break;
      }
      case field_decl_K:
      {
         auto* fd = GetPointer<field_decl>(tn);
         auto* ic = GetPointer<integer_cst>(GET_NODE(fd->bpos));
         THROW_ASSERT(ic, "non-constant field offset (variable lenght object) currently not supported: " + STR(GET_INDEX_NODE(fd->bpos)));
         auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
         required.emplace_back(0, static_cast<unsigned int>(ull_value / 8)); /// bpos has an offset in bits
         if(ull_value % 8 != 0)
         {
            THROW_ERROR("bitfields are not yet supported: " + fd->ToString());
         }
         break;
      }
      case gimple_asm_K:
      {
         auto* ga = GetPointer<gimple_asm>(tn);
         if(ga->in)
         {
            get_required_values(TM, required, GET_NODE(ga->in), GET_INDEX_NODE(ga->in));
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
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
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
      case tree_reindex_K:
      {
         if(GET_CONST_NODE(statement))
         {
            return LastStatement(GET_CONST_NODE(statement));
         }
         else
         {
            return false;
         }
      }
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
      case last_tree_K:
      case none_K:
      case placeholder_expr_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      {
         THROW_UNREACHABLE("Unexpected statement: " + statement->ToString());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return true;
}

size_t tree_helper::GetFunctionSize(const tree_managerConstRef& TM, const unsigned int function_index)
{
   const auto tn = TM->get_tree_node_const(function_index);
   const auto fd = GetPointer<const function_decl>(tn);
   THROW_ASSERT(fd, tn->ToString());
   THROW_ASSERT(fd->body, "Function " + fd->ToString() + " is without body");
   const auto sl = GetPointer<const statement_list>(GET_NODE(fd->body));
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
   const auto tn = TM->get_tree_node_const(node_index);
   const auto gasm = GetPointer<const gimple_asm>(tn);
   THROW_ASSERT(gasm, tn->ToString());
   return gasm->str;
}

bool tree_helper::IsStore(const tree_managerConstRef& TM, const tree_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data)
{
   if(tn->get_kind() == tree_reindex_K)
   {
      return IsStore(TM, GET_CONST_NODE(tn), fun_mem_data);
   }
   const auto ga = GetPointer<const gimple_assign>(tn);
   if(not ga)
   {
      return false;
   }
   const auto op0 = GET_NODE(ga->op0);
   const auto op1 = GET_NODE(ga->op1);
   bool store_candidate = op0->get_kind() == bit_field_ref_K || op0->get_kind() == component_ref_K || op0->get_kind() == indirect_ref_K || op0->get_kind() == misaligned_indirect_ref_K || op0->get_kind() == mem_ref_K || op0->get_kind() == array_ref_K ||
                          op0->get_kind() == target_mem_ref_K || op0->get_kind() == target_mem_ref461_K or fun_mem_data.find(op0->index) != fun_mem_data.end();
   if(op0->get_kind() == realpart_expr_K || op0->get_kind() == imagpart_expr_K)
   {
      enum kind code0 = GET_NODE(GetPointer<unary_expr>(op0)->op)->get_kind();
      if(code0 == bit_field_ref_K || code0 == component_ref_K || code0 == indirect_ref_K || code0 == misaligned_indirect_ref_K || code0 == mem_ref_K || code0 == array_ref_K || code0 == target_mem_ref_K || code0 == target_mem_ref461_K)
      {
         store_candidate = true;
      }
      if(code0 == var_decl_K && fun_mem_data.find(GET_INDEX_NODE(GetPointer<unary_expr>(op0)->op)) != fun_mem_data.end())
      {
         store_candidate = true;
      }
   }

   if(op1->get_kind() == view_convert_expr_K)
   {
      const auto op0_type = CGetType(op0);
      auto* vc = GetPointer<view_convert_expr>(op1);
      tree_nodeRef vc_op_type = tree_helper::get_type_node(GET_NODE(vc->op));
      if(op0_type->get_kind() == record_type_K || op0_type->get_kind() == union_type_K)
      {
         store_candidate = true;
      }

      if(vc_op_type->get_kind() == vector_type_K && op0_type->get_kind() == array_type_K)
      {
         store_candidate = true;
      }
   }
   return store_candidate;
}

bool tree_helper::IsLoad(const tree_managerConstRef& TM, const tree_nodeConstRef& tn, const CustomOrderedSet<unsigned int>& fun_mem_data)
{
   if(tn->get_kind() == tree_reindex_K)
   {
      return IsLoad(TM, GET_CONST_NODE(tn), fun_mem_data);
   }
   const auto ga = GetPointer<const gimple_assign>(tn);
   if(not ga)
   {
      return false;
   }
   const auto op0 = GET_NODE(ga->op0);
   const auto op1 = GET_NODE(ga->op1);
   bool is_a_vector_bitfield = false;
   /// check for bit field ref of vector type
   if(op1->get_kind() == bit_field_ref_K)
   {
      auto* bfr = GetPointer<bit_field_ref>(op1);
      if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(bfr->op0)))
      {
         is_a_vector_bitfield = true;
      }
   }
   bool load_candidate = (op1->get_kind() == bit_field_ref_K && !is_a_vector_bitfield) || op1->get_kind() == component_ref_K || op1->get_kind() == indirect_ref_K || op1->get_kind() == misaligned_indirect_ref_K || op1->get_kind() == mem_ref_K ||
                         op1->get_kind() == array_ref_K || op1->get_kind() == target_mem_ref_K || op1->get_kind() == target_mem_ref461_K or fun_mem_data.find(op1->index) != fun_mem_data.end();
   if(op1->get_kind() == realpart_expr_K || op1->get_kind() == imagpart_expr_K)
   {
      enum kind code1 = GET_NODE(GetPointer<unary_expr>(op1)->op)->get_kind();
      if((code1 == bit_field_ref_K && !is_a_vector_bitfield) || code1 == component_ref_K || code1 == indirect_ref_K || code1 == bit_field_ref_K || code1 == misaligned_indirect_ref_K || code1 == mem_ref_K || code1 == array_ref_K ||
         code1 == target_mem_ref_K || code1 == target_mem_ref461_K)
      {
         load_candidate = true;
      }
      if(code1 == var_decl_K && fun_mem_data.find(GET_INDEX_NODE(GetPointer<unary_expr>(op1)->op)) != fun_mem_data.end())
      {
         load_candidate = true;
      }
   }

   if(op1->get_kind() == view_convert_expr_K)
   {
      auto* vc = GetPointer<view_convert_expr>(op1);
      tree_nodeRef vc_op_type = tree_helper::get_type_node(GET_NODE(vc->op));
      if(vc_op_type->get_kind() == record_type_K || vc_op_type->get_kind() == union_type_K)
      {
         load_candidate = true;
      }
      const auto op0_type = CGetType(op0);

      if(vc_op_type->get_kind() == array_type_K && op0_type->get_kind() == vector_type_K)
      {
         load_candidate = true;
      }
   }
   return load_candidate;
}

bool tree_helper::IsLut(const tree_managerConstRef& TM, const tree_nodeConstRef& tn)
{
   if(tn->get_kind() == tree_reindex_K)
   {
      return IsLut(TM, GET_CONST_NODE(tn));
   }
   const auto ga = GetPointer<const gimple_assign>(tn);
   if(not ga)
   {
      return false;
   }
   const auto op1 = GET_NODE(ga->op1);
   return op1->get_kind() == lut_expr_K;
}
