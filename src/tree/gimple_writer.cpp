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
 * @file GimpleWriter.cpp
 * @brief tree node writer. This class exploiting the visitor design pattern write a tree node according to the gimple format (i.e. the format used by gcc in plain text dump of gimple)
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "gimple_writer.hpp"

#include "custom_map.hpp"                     // for unordered_map<>:...
#include <algorithm>                          // for transform
#include <boost/algorithm/string/replace.hpp> // for replace_all
#include <boost/lexical_cast.hpp>             // for lexical_cast
#include <cctype>                             // for toupper
#include <cstddef>                            // for size_t
#include <string>                             // for string, operator+
#include <utility>                            // for pair, operator!=
#include <vector>                             // for vector, vector<>...

/// Behavior include
#include "basic_block.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

GimpleWriter::GimpleWriter(std::ostream& _os, const bool _use_uid) : os(_os), use_uid(_use_uid), current_node_index(0)
{
}

void GimpleWriter::operator()(const tree_node*, unsigned int&)
{
}

void GimpleWriter::operator()(const WeightedNode* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const tree_reindex* obj, unsigned int&)
{
   current_node_index = obj->index;
   //   obj->actual_tree_node->visit(this);
}

void GimpleWriter::operator()(const attr*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void GimpleWriter::operator()(const srcp*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void GimpleWriter::operator()(const decl_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
   obj->srcp::visit(this);
}

void GimpleWriter::operator()(const expr_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->WeightedNode::visit(this);
   obj->srcp::visit(this);
}

void GimpleWriter::operator()(const gimple_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->WeightedNode::visit(this);
   obj->srcp::visit(this);
}

void GimpleWriter::operator()(const unary_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   switch(obj->get_kind())
   {
      case abs_expr_K:
      {
         os << "ABS_EXPR <";
         obj->op->visit(this);
         os << ">";
         break;
      }
      case addr_expr_K:
      {
         os << "&";
         break;
      }
      case fix_trunc_expr_K:
      case float_expr_K:
      case nop_expr_K:
      case convert_expr_K:
      {
         os << "(";
         obj->type->visit(this);
         os << ") ";
         break;
      }
      case view_convert_expr_K:
      {
         os << "VIEW_CONVERT_EXPR<";
         obj->type->visit(this);
         os << ">(";
         obj->op->visit(this);
         os << ")";
         obj->expr_node::visit(this);
         return;
      }
      case imagpart_expr_K:
      {
         os << "IMAG_EXPR<";
         obj->op->visit(this);
         os << ">";
         break;
      }
      case realpart_expr_K:
      {
         os << "REAL_EXPR<";
         obj->op->visit(this);
         os << ">";
         break;
      }
      case arrow_expr_K:
      case bit_not_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case indirect_ref_K:
      case misaligned_indirect_ref_K:
      case loop_expr_K:
      case negate_expr_K:
      case non_lvalue_expr_K:
      case paren_expr_K:
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
      {
         std::string str = obj->get_kind_text();
         std::transform(str.begin(), str.end(), str.begin(), ::toupper);
         os << "<" << str;
         obj->op->visit(this);
         os << ">";
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
      case CASE_TYPE_NODES:
      default:
         THROW_UNREACHABLE("");
   }
   if(GET_NODE(obj->op)->get_kind() == function_decl_K)
      GetPointer<function_decl>(GET_NODE(obj->op))->name->visit(this);
   else
      obj->op->visit(this);
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const binary_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   switch(obj->get_kind())
   {
      case max_expr_K:
      {
         os << "MAX_EXPR <";
         obj->op0->visit(this);
         os << ", ";
         obj->op1->visit(this);
         os << ">";
         break;
      }
      case min_expr_K:
      {
         os << "MIN_EXPR <";
         obj->op0->visit(this);
         os << ", ";
         obj->op1->visit(this);
         os << ">";
         break;
      }
      case complex_expr_K:
      {
         os << "COMPLEX_EXPR <";
         obj->op0->visit(this);
         os << ", ";
         obj->op1->visit(this);
         os << ">";
         break;
      }
      case mem_ref_K:
      {
         if(GET_NODE(obj->op1)->get_kind() == integer_cst_K)
         {
            const long long offset = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(obj->op1)));
            if(offset == 0)
            {
               os << "*";
               obj->op0->visit(this);
            }
            else
            {
               os << "MEM[(";
               tree_helper::get_type_node(GET_NODE(obj->op1))->visit(this);
               os << ")";
               obj->op0->visit(this);
               os << ") + ";
               obj->op1->visit(this);
               os << "]";
            }
         }
         break;
      }
      case uneq_expr_K:
      {
         obj->op0->visit(this);
         os << " u== ";
         obj->op1->visit(this);
         break;
      }
      case lrotate_expr_K:
      case rrotate_expr_K:
      case vec_pack_trunc_expr_K:
      {
         std::string str = obj->get_kind_text();
         std::transform(str.begin(), str.end(), str.begin(), ::toupper);
         os << "<" << str;
         obj->op0->visit(this);
         os << " , ";
         obj->op1->visit(this);
         os << ">";
         break;
      }
      case assert_expr_K:
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case catch_expr_K:
      case ceil_div_expr_K:
      case ceil_mod_expr_K:
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
      case lshift_expr_K:
      case lt_expr_K:
      case minus_expr_K:
      case modify_expr_K:
      case mult_expr_K:
      case mult_highpart_expr_K:
      case ne_expr_K:
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
      case unge_expr_K:
      case ungt_expr_K:
      case unle_expr_K:
      case unlt_expr_K:
      case widen_sum_expr_K:
      case widen_mult_expr_K:
      case with_size_expr_K:
      case vec_lshift_expr_K:
      case vec_rshift_expr_K:
      case widen_mult_hi_expr_K:
      case widen_mult_lo_expr_K:
      case vec_pack_sat_expr_K:
      case vec_pack_fix_trunc_expr_K:
      case vec_extracteven_expr_K:
      case vec_extractodd_expr_K:
      case vec_interleavehigh_expr_K:
      case vec_interleavelow_expr_K:
      {
         obj->op0->visit(this);
         const std::string op = tree_helper::op_symbol(obj);
         os << " " << op << " ";
         obj->op1->visit(this);
         break;
      }
      case sat_plus_expr_K:
      {
         obj->op0->visit(this);
         os << " satplus ";
         obj->op1->visit(this);
         break;
      }
      case sat_minus_expr_K:
      {
         obj->op0->visit(this);
         os << " satminus ";
         obj->op1->visit(this);
         break;
      }
      case extract_bit_expr_K:
      {
         obj->op0->visit(this);
         const std::string op = "EXTRACT_BIT_EXPR";
         os << " " << op << " ";
         obj->op1->visit(this);
         break;
      }
      case ltgt_expr_K:
      {
         obj->op0->visit(this);
         os << " <!=> ";
         obj->op1->visit(this);
         break;
      }
      case unordered_expr_K:
      {
         os << "UNORDERED_EXPR<";
         obj->op0->visit(this);
         os << ", ";
         obj->op1->visit(this);
         os << ">";
         break;
      }
      case ordered_expr_K:
      {
         os << "ORDERED_EXPR<";
         obj->op0->visit(this);
         os << ", ";
         obj->op1->visit(this);
         os << ">";
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case lut_expr_K:
      {
         os << "LUT<";
         const auto* obj2 = dynamic_cast<const lut_expr*>(obj);
         obj2->op0->visit(this);
         os << ", ";
         obj2->op1->visit(this);
         os << ">";
         break;
      }
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_UNREACHABLE("");
   }
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const ternary_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   switch(obj->get_kind())
   {
      case bit_field_ref_K:
      {
         os << "BIT_FIELD_REF <";
         obj->op0->visit(this);
         os << ", ";
         obj->op1->visit(this);
         os << ", ";
         obj->op2->visit(this);
         os << ">";
         break;
      }
      case component_ref_K:
      {
         const indirect_ref* ir = GetPointer<indirect_ref>(GET_NODE(obj->op0));
         if(ir)
         {
            ir->op->visit(this);
            os << "->";
            obj->op1->visit(this);
         }
         else
         {
            obj->op0->visit(this);
            os << ".";
            obj->op1->visit(this);
         }
         break;
      }
      case cond_expr_K:
      case vec_cond_expr_K:
      {
         obj->op0->visit(this);
         os << " ? ";
         obj->op1->visit(this);
         os << " : ";
         obj->op2->visit(this);
         break;
      }
      case ternary_plus_expr_K:
      {
         obj->op0->visit(this);
         os << " + ";
         obj->op1->visit(this);
         os << " + ";
         obj->op2->visit(this);
         break;
      }
      case ternary_pm_expr_K:
      {
         obj->op0->visit(this);
         os << " + ";
         obj->op1->visit(this);
         os << " - ";
         obj->op2->visit(this);
         break;
      }
      case ternary_mp_expr_K:
      {
         obj->op0->visit(this);
         os << " - ";
         obj->op1->visit(this);
         os << " + ";
         obj->op2->visit(this);
         break;
      }
      case ternary_mm_expr_K:
      {
         obj->op0->visit(this);
         os << " - ";
         obj->op1->visit(this);
         os << " - ";
         obj->op2->visit(this);
         break;
      }
      case bit_ior_concat_expr_K:
      {
         obj->op0->visit(this);
         os << " | ";
         obj->op1->visit(this);
         os << " ( ";
         obj->op2->visit(this);
         os << " ) ";
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case dot_prod_expr_K:
      case identifier_node_K:
      case obj_type_ref_K:
      case save_expr_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case vec_perm_expr_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
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
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      default:
      {
         break;
      }
   }
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const quaternary_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(obj->get_kind() == array_ref_K)
   {
      obj->op0->visit(this);
      os << "[";
      obj->op1->visit(this);
      os << "]";
   }
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const type_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(obj->qual != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
   {
      std::string replace = tree_helper::return_C_qualifiers(obj->qual, true);
      boost::replace_all(replace, "__restrict__ ", " restrict");
      os << replace << " ";
   }
   if(obj->name)
   {
      obj->name->visit(this);
   }
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const memory_tag* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
}

void GimpleWriter::operator()(const cst_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type->visit(this);
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const error_mark* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "error_mark";
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const array_type* obj, unsigned int& mask)
{
   obj->elts->visit(this);
   os << "[";
   /// Computing size
   tree_nodeRef array_length = GET_NODE(obj->size);
   tree_nodeRef array_element = GET_NODE(obj->elts);
   if(array_length->get_kind() == integer_cst_K)
   {
      auto* arr_ic = GetPointer<integer_cst>(array_length);
      auto* tn = GetPointer<type_node>(array_element);
      auto* eln_ic = GetPointer<integer_cst>(GET_NODE(tn->size));
      os << boost::lexical_cast<std::string>(tree_helper::get_integer_cst_value(arr_ic) / tree_helper::get_integer_cst_value(eln_ic));
   }

   os << "]";
   mask = NO_VISIT;
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const gimple_asm* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
}

void GimpleWriter::operator()(const baselink* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const gimple_bind* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const binfo* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const block* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const call_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   const addr_expr* ae = GetPointer<addr_expr>(GET_NODE(obj->fn));
   if(ae)
   {
      const function_decl* fd = GetPointer<function_decl>(GET_NODE(ae->op));
      if(fd)
      {
         fd->name->visit(this);
         os << " (";
         const std::vector<tree_nodeRef>& args = obj->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            if(arg != args.begin())
               os << ", ";
            (*arg)->visit(this);
         }
         os << ")";
      }
   }
   else
   {
      const ssa_name* sn = GetPointer<ssa_name>(GET_NODE(obj->fn));
      if(sn)
      {
         sn->visit(this);
         os << " (";
         const std::vector<tree_nodeRef>& args = obj->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            if(arg != args.begin())
               os << ", ";
            (*arg)->visit(this);
         }
         os << ")";
      }
   }
}

void GimpleWriter::operator()(const aggr_init_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   const addr_expr* ae = GetPointer<addr_expr>(GET_NODE(obj->fn));
   if(ae)
   {
      const function_decl* fd = GetPointer<function_decl>(GET_NODE(ae->op));
      if(fd)
      {
         fd->name->visit(this);
         os << " (";
         const std::vector<tree_nodeRef>& args = obj->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            if(arg != args.begin())
               os << ", ";
            (*arg)->visit(this);
         }
         os << ")";
      }
   }
   else
   {
      const ssa_name* sn = GetPointer<ssa_name>(GET_NODE(obj->fn));
      if(sn)
      {
         sn->visit(this);
         os << " (";
         const std::vector<tree_nodeRef>& args = obj->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            if(arg != args.begin())
               os << ", ";
            (*arg)->visit(this);
         }
         os << ")";
      }
   }
   os << "ctor: " << obj->ctor;
   if(obj->slot)
      obj->slot->visit(this);
}

void GimpleWriter::operator()(const gimple_call* obj, unsigned int& mask)
{
   obj->gimple_node::visit(this);
   mask = NO_VISIT;
   const addr_expr* ae = GetPointer<addr_expr>(GET_NODE(obj->fn));
   if(ae)
   {
      const function_decl* fd = GetPointer<function_decl>(GET_NODE(ae->op));
      if(fd)
      {
         fd->name->visit(this);
         os << " (";
         const std::vector<tree_nodeRef>& args = obj->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            if(arg != args.begin())
               os << ", ";
            (*arg)->visit(this);
         }
         os << ")";
      }
   }
}

void GimpleWriter::operator()(const case_label_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(obj->default_flag)
      os << "default: ";
   else
      os << "case ";
   if(obj->op0 and obj->op1)
   {
      obj->op0->visit(this);
      os << " ... ";
      obj->op1->visit(this);
   }
   else if(obj->op0)
   {
      obj->op0->visit(this);
      os << ": ";
   }
   if(obj->got)
      obj->got->visit(this);
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const cast_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "cast_expr ";
   if(obj->op)
   {
      obj->op->visit(this);
   }
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const complex_cst* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->cst_node::visit(this);
}

void GimpleWriter::operator()(const complex_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const gimple_cond* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "if (";
   obj->op0->visit(this);
   os << ")";
   obj->gimple_node::visit(this);
}

void GimpleWriter::operator()(const const_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
}

void GimpleWriter::operator()(const constructor* obj, unsigned int& mask)
{
   os << "{";
   os << "}";
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const enumeral_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const expr_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const field_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(obj->name)
      obj->name->visit(this);
   obj->decl_node::visit(this);
   obj->attr::visit(this);
}

void GimpleWriter::operator()(const function_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << std::endl;
   os << ";; Function ";
   const tree_nodeRef name = obj->name;
   name->visit(this);
   os << " (";
   name->visit(this);
   os << ")" << std::endl << std::endl;
   name->visit(this);
   os << " (";
   const std::vector<tree_nodeRef>& list_of_args = obj->list_of_args;
   std::vector<tree_nodeRef>::const_iterator arg, arg_end = list_of_args.end();
   for(arg = list_of_args.begin(); arg != arg_end; ++arg)
   {
      if(arg != list_of_args.begin())
         os << ", ";
      const parm_decl* pd = GetPointer<parm_decl>(GET_NODE(*arg));
      pd->type->visit(this);
      os << " ";
      pd->name->visit(this);
   }

   os << ")" << std::endl;
   if(obj->body)
   {
      os << "{" << std::endl;
      obj->body->visit(this);
      os << "}" << std::endl << std::endl << std::endl;
   }
   obj->decl_node::visit(this);
   obj->attr::visit(this);
}

void GimpleWriter::operator()(const function_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const gimple_assign* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(obj->predicate)
   {
      os << "if(";
      obj->predicate->visit(this);
      os << ") ";
   }
   obj->op0->visit(this);
   os << " = ";
   obj->op1->visit(this);
   if(obj->clobber)
      os << "clobber";
   if(obj->temporary_address)
      os << "addr";
   obj->gimple_node::visit(this);
}

void GimpleWriter::operator()(const gimple_goto* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
}

void GimpleWriter::operator()(const handler* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const identifier_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << obj->strg;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const integer_cst* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << tree_helper::get_integer_cst_value(obj);
   obj->cst_node::visit(this);
}

void GimpleWriter::operator()(const integer_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const gimple_label* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->op->visit(this);
   os << ":";
   obj->gimple_node::visit(this);
}

void GimpleWriter::operator()(const method_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->function_type::visit(this);
}

void GimpleWriter::operator()(const namespace_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
}

void GimpleWriter::operator()(const overload* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const parm_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->name->visit(this);
   obj->decl_node::visit(this);
}

void GimpleWriter::operator()(const gimple_phi* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "  # ";
   obj->res->visit(this);
   os << " = PHI <";
   for(const auto& def_edge : obj->CGetDefEdgesList())
   {
      if(def_edge != obj->CGetDefEdgesList().front())
         os << ", ";
      def_edge.first->visit(this);
      os << "(" << def_edge.second << ")";
   }
   os << ">";
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const pointer_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(not obj->name)
   {
      obj->ptd->visit(this);
      os << " *";
   }
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const real_cst* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << obj->valr;
   obj->cst_node::visit(this);
}

void GimpleWriter::operator()(const real_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const record_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(not obj->name or GET_CONST_NODE(obj->name)->get_kind() != type_decl_K)
      os << "struct ";
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const reference_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const result_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "<retval>";
   obj->decl_node::visit(this);
}

void GimpleWriter::operator()(const gimple_return* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "return";
   if(obj->op)
   {
      os << " ";
      obj->op->visit(this);
   }
   obj->gimple_node::visit(this);
}

void GimpleWriter::operator()(const return_stmt* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const scope_ref* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const ssa_name* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(obj->type)
   {
      os << "(";
      obj->type->visit(this);
      os << ") ";
   }
   os << " ";
   if(obj->var)
      obj->var->visit(this);
   os << "_" << obj->vers;
   if(obj->orig_vers)
      os << "_[" << obj->orig_vers << "]";
   if(obj->default_flag)
      os << "(D)";
   //   if(obj->min) obj->min->visit(this);
   //   if(obj->max) obj->max->visit(this);
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const statement_list* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   const std::map<unsigned int, blocRef>& list_of_block = obj->list_of_bloc;
   std::map<unsigned int, blocRef>::const_iterator block, block_end = list_of_block.end();
   for(block = list_of_block.begin(); block != block_end; ++block)
   {
      if(block->first == BB_ENTRY or block->first == BB_EXIT)
      {
         continue;
      }
      if(block->second->CGetStmtList().empty() or GET_NODE(block->second->CGetStmtList().front())->get_kind() != gimple_label_K)
      {
         os << "<bb " << block->first << ">:" << std::endl;
      }
      for(const auto& phi : block->second->CGetPhiList())
      {
         phi->visit(this);
         os << std::endl;
      }
      for(const auto& stmt : block->second->CGetStmtList())
      {
         const tree_nodeConstRef statement = GET_NODE(stmt);
         /// We print only MEMUSE and MEMDEF as VUSE and VDEF like gcc -fdump-tree-all
         if(GetPointer<const gimple_node>(statement) and GetPointer<const gimple_node>(statement)->memuse)
         {
            os << "VUSE ";
            GetPointer<const gimple_node>(statement)->memuse->visit(this);
         }
         if(GetPointer<const gimple_node>(statement) and GetPointer<const gimple_node>(statement)->memdef)
         {
            os << "VDEF ";
            GetPointer<const gimple_node>(statement)->memdef->visit(this);
         }
         stmt->visit(this);
         if(statement->get_kind() != gimple_cond_K and statement->get_kind() != gimple_label_K and statement->get_kind() != gimple_switch_K and statement->get_kind() != gimple_pragma_K)
            os << ";";
         os << std::endl;
      }
      if(block->second->true_edge and block->second->false_edge)
      {
         os << "    goto <bb " << block->second->true_edge << ">";
         const blocRef next_true = list_of_block.find(block->second->true_edge)->second;
         if(next_true->CGetStmtList().size() and GET_NODE(next_true->CGetStmtList().front())->get_kind() == gimple_label_K)
         {
            const gimple_label* le = GetPointer<gimple_label>(GET_NODE(next_true->CGetStmtList().front()));
            os << " (";
            le->op->visit(this);
            os << ")";
         }
         os << ";" << std::endl;
         os << "  else" << std::endl;
         os << "    goto <bb " << block->second->false_edge << ">";
         const blocRef next_false = list_of_block.find(block->second->false_edge)->second;
         if(next_false->CGetStmtList().size() and GET_NODE(next_false->CGetStmtList().back())->get_kind() == gimple_label_K)
         {
            const gimple_label* le = GetPointer<gimple_label>(GET_NODE(next_false->CGetStmtList().front()));
            os << " (";
            le->op->visit(this);
            os << ")";
         }
         os << ";" << std::endl;
      }
      else if(block->second->list_of_succ.size() == 1)
      {
         const unsigned int succ_index = *(block->second->list_of_succ.begin());
         if(succ_index != block->second->number + 1 and succ_index != BB_EXIT)
         {
            os << "  goto <bb " << succ_index << ">";
            const blocRef next = list_of_block.find(succ_index)->second;
            if(next->CGetStmtList().size() and GET_NODE(next->CGetStmtList().back())->get_kind() == gimple_label_K)
            {
               const gimple_label* le = GetPointer<gimple_label>(GET_NODE(next->CGetStmtList().front()));
               os << " (";
               le->op->visit(this);
               os << ")";
            }
            os << ";" << std::endl;
         }
      }
      os << std::endl;
   }
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const string_cst* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "\"" << obj->strg << "\"";
   obj->cst_node::visit(this);
}

void GimpleWriter::operator()(const gimple_switch* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "switch (";
   obj->op0->visit(this);
   os << ") <";
   obj->op1->visit(this);
   os << ">";
   obj->gimple_node::visit(this);
}

void GimpleWriter::operator()(const target_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "target_expr (";
   obj->decl->visit(this);
   obj->init->visit(this);
   obj->clnp->visit(this);
   obj->expr_node::visit(this);
   os << ")";
}
void GimpleWriter::operator()(const lut_expr* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "lut_expr (";
   obj->op0->visit(this);
   obj->op1->visit(this);
   if(obj->op2)
      obj->op2->visit(this);
   if(obj->op3)
      obj->op3->visit(this);
   if(obj->op4)
      obj->op4->visit(this);
   if(obj->op5)
      obj->op5->visit(this);
   if(obj->op6)
      obj->op6->visit(this);
   if(obj->op7)
      obj->op7->visit(this);
   if(obj->op8)
      obj->op8->visit(this);
   obj->expr_node::visit(this);
   os << ")";
}

void GimpleWriter::operator()(const template_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->decl_node::visit(this);
}

void GimpleWriter::operator()(const template_parm_index* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type->visit(this);
   obj->decl->visit(this);
   if(obj->constant_flag)
      os << "_C";
   if(obj->readonly_flag)
      os << "_R";
   os << "_" << obj->idx;
   os << "_" << obj->level;
   os << "_" << obj->orig_level;
}

void GimpleWriter::operator()(const tree_list* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->valu->visit(this);
   if(obj->chan)
   {
      os << ", ";
      obj->chan->visit(this);
   }
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const tree_vec* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   std::vector<tree_nodeRef>::const_iterator op, op_end = obj->list_of_op.end();
   for(op = obj->list_of_op.begin(); op != op_end; ++op)
   {
      if(op != obj->list_of_op.begin())
         os << ", ";
      (*op)->visit(this);
   }
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const try_block* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const type_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->name->visit(this);
   obj->decl_node::visit(this);
}

void GimpleWriter::operator()(const union_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const var_decl* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(obj->name)
      obj->name->visit(this);
   else
   {
      if(this->use_uid)
      {
         os << "D." << obj->uid;
      }
      else
      {
         os << "internal_" << current_node_index;
      }
   }
   obj->decl_node::visit(this);
   obj->attr::visit(this);
}

void GimpleWriter::operator()(const vector_cst* obj, unsigned int& mask)
{
   os << "{ ";
   size_t vector_size = obj->list_of_valu.size();
   for(size_t i = 0; i < vector_size; i++)
   {
      obj->list_of_valu[i]->visit(this);
      if(i != (obj->list_of_valu).size() - 1)
         os << ", ";
   }
   os << " }";

   mask = NO_VISIT;
   obj->cst_node::visit(this);
}

void GimpleWriter::operator()(const type_argument_pack* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(not obj->unql and not obj->name)
   {
      os << "type_argument_pack ";
      obj->arg->visit(this);
      mask = NO_VISIT;
      obj->type_node::visit(this);
   }
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const nontype_argument_pack* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "nontype_argument_pack ";
   obj->arg->visit(this);
   mask = NO_VISIT;
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const type_pack_expansion* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "type_pack_expansion ";
   if(obj->op)
      obj->op->visit(this);
   if(obj->param_packs)
      obj->param_packs->visit(this);
   if(obj->arg)
      obj->arg->visit(this);
   mask = NO_VISIT;
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const expr_pack_expansion* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "expr_pack_expansion ";
   if(obj->op)
      obj->op->visit(this);
   if(obj->param_packs)
      obj->param_packs->visit(this);
   if(obj->arg)
      obj->arg->visit(this);
   mask = NO_VISIT;
   obj->expr_node::visit(this);
}

void GimpleWriter::operator()(const vector_type* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   if(not obj->unql and not obj->name)
   {
      os << "vector ";
      obj->elts->visit(this);
      mask = NO_VISIT;
      obj->type_node::visit(this);
   }
   obj->type_node::visit(this);
}

void GimpleWriter::operator()(const target_mem_ref* obj, unsigned int& mask)
{
   os << "MEM[";
   if(obj->symbol)
   {
      os << "symbol:";
      obj->symbol->visit(this);
   }

   if(obj->idx)
   {
      os << ", index:";
      obj->idx->visit(this);
   }

   if(obj->step)
   {
      os << ", step:";
      obj->step->visit(this);
   }

   if(obj->offset)
   {
      os << ", index:";
      obj->offset->visit(this);
   }
   os << "]";
   mask = NO_VISIT;
   obj->WeightedNode::visit(this);
}

void GimpleWriter::operator()(const target_mem_ref461* obj, unsigned int& mask)
{
   os << "MEM[";
   if(obj->base)
   {
      os << "symbol:";
      obj->base->visit(this);
   }

   if(obj->idx)
   {
      os << ", index:";
      obj->idx->visit(this);
   }

   if(obj->idx2)
   {
      os << ", index:";
      obj->idx2->visit(this);
   }

   if(obj->step)
   {
      os << ", step:";
      obj->step->visit(this);
   }

   if(obj->offset)
   {
      os << ", offset:";
      obj->offset->visit(this);
   }
   os << "]";
   mask = NO_VISIT;
   obj->WeightedNode::visit(this);
}
void GimpleWriter::operator()(const bloc*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void GimpleWriter::operator()(const null_node* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const gimple_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->gimple_node::visit(this);
   os << "#pragma ";
   THROW_ASSERT(obj->scope, "Printing a gimple pragma without scope");
   obj->scope->visit(this);
   obj->directive->visit(this);
}

void GimpleWriter::operator()(const omp_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "omp";
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const omp_parallel_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   os << " parallel";
}

void GimpleWriter::operator()(const omp_sections_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   os << " sections";
}

void GimpleWriter::operator()(const omp_parallel_sections_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->omp_pragma::visit(this);
}

void GimpleWriter::operator()(const omp_section_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   os << " section";
}

void GimpleWriter::operator()(const omp_for_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << " parallel for";
   const CustomUnorderedMapUnstable<std::string, std::string>& clauses = obj->clauses;
   CustomUnorderedMapUnstable<std::string, std::string>::const_iterator clause, clause_end = clauses.end();
   for(clause = clauses.begin(); clause != clause_end; ++clause)
   {
      os << " " + clause->first + (clause->second != "" ? "(" + clause->second + ")" : "");
   }
}

void GimpleWriter::operator()(const omp_simd_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << " simd";
   const CustomUnorderedMapUnstable<std::string, std::string>& clauses = obj->clauses;
   CustomUnorderedMapUnstable<std::string, std::string>::const_iterator clause, clause_end = clauses.end();
   for(clause = clauses.begin(); clause != clause_end; ++clause)
   {
      os << " " + clause->first + (clause->second != "" ? "(" + clause->second + ")" : "");
   }
}

void GimpleWriter::operator()(const omp_declare_simd_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << " declare simd";
   const CustomUnorderedMapUnstable<std::string, std::string>& clauses = obj->clauses;
   CustomUnorderedMapUnstable<std::string, std::string>::const_iterator clause, clause_end = clauses.end();
   for(clause = clauses.begin(); clause != clause_end; ++clause)
   {
      os << " " + clause->first + (clause->second != "" ? "(" + clause->second + ")" : "");
   }
}

void GimpleWriter::operator()(const omp_target_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << " target";
   const CustomUnorderedMapUnstable<std::string, std::string>& clauses = obj->clauses;
   CustomUnorderedMapUnstable<std::string, std::string>::const_iterator clause, clause_end = clauses.end();
   for(clause = clauses.begin(); clause != clause_end; ++clause)
   {
      os << " " + clause->first + (clause->second != "" ? "(" + clause->second + ")" : "");
   }
}

void GimpleWriter::operator()(const omp_critical_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << " critical";
   const CustomUnorderedMapUnstable<std::string, std::string>& clauses = obj->clauses;
   CustomUnorderedMapUnstable<std::string, std::string>::const_iterator clause, clause_end = clauses.end();
   for(clause = clauses.begin(); clause != clause_end; ++clause)
   {
      os << " " + clause->first + (clause->second != "" ? "(" + clause->second + ")" : "");
   }
}

void GimpleWriter::operator()(const omp_task_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << " task";
   const CustomUnorderedMapUnstable<std::string, std::string>& clauses = obj->clauses;
   CustomUnorderedMapUnstable<std::string, std::string>::const_iterator clause, clause_end = clauses.end();
   for(clause = clauses.begin(); clause != clause_end; ++clause)
   {
      os << " " + clause->first + (clause->second != "" ? "(" + clause->second + ")" : "");
   }
}

void GimpleWriter::operator()(const map_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const call_hw_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const call_point_hw_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const issue_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const blackbox_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->issue_pragma::visit(this);
}

void GimpleWriter::operator()(const profiling_pragma* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->tree_node::visit(this);
}

void GimpleWriter::operator()(const statistical_profiling* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   obj->profiling_pragma::visit(this);
}

void GimpleWriter::operator()(const gimple_while* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "while (";
   obj->op0->visit(this);
   os << ")";
   obj->gimple_node::visit(this);
}

void GimpleWriter::operator()(const gimple_for* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "for (";
   obj->op1->visit(this);
   os << "; ";
   obj->op0->visit(this);
   os << "; ";
   obj->op2->visit(this);
   os << ")";
   obj->gimple_node::visit(this);
}

void GimpleWriter::operator()(const gimple_multi_way_if* obj, unsigned int& mask)
{
   mask = NO_VISIT;
   os << "multi_way_if (";
   for(const auto& cond : obj->list_of_cond)
   {
      if(cond.first)
         cond.first->visit(this);
      os << ":" << cond.second << " ";
   }
   os << ")";
   obj->gimple_node::visit(this);
}
