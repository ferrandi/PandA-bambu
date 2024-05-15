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
 * @file BehavioralHelper.cpp
 * @brief Helper for reading data from tree_manager
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */
#include "behavioral_helper.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "cdfg_edge_info.hpp"
#include "compiler_constants.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "type_casting.hpp"
#include "var_pp_functor.hpp"

#if HAVE_FROM_PRAGMA_BUILT
#include "pragma_constants.hpp"
#endif

#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_RELEASE.hpp"

#include <boost/algorithm/string/replace.hpp>

#include <cstddef>
#include <string>
#include <vector>

std::map<std::string, unsigned int> BehavioralHelper::used_name;

std::map<unsigned int, std::string> BehavioralHelper::vars_symbol_table;

std::map<unsigned int, std::string> BehavioralHelper::vars_renaming_table;

/// Max length of a row (at the moment checked only during constructor printing)
#define MAX_ROW_LENGTH 128

BehavioralHelper::BehavioralHelper(const application_managerRef _AppM, unsigned int _index, bool _body,
                                   const ParameterConstRef _parameters)
    : AppM(application_managerRef(_AppM.get(), null_deleter())),
      TM(_AppM->get_tree_manager()),
      Param(_parameters),
      debug_level(Param->get_class_debug_level("BehavioralHelper", DEBUG_LEVEL_NONE)),
      function_index(_index),
      function_name(tree_helper::GetFunctionName(TM, TM->GetTreeNode(function_index))),
      body(_body),
      opaque(!_body)
{
}

BehavioralHelper::~BehavioralHelper() = default;

std::tuple<std::string, unsigned int, unsigned int> BehavioralHelper::get_definition(unsigned int index,
                                                                                     bool& is_system) const
{
   THROW_ASSERT(index, "expected a meaningful index");
   return tree_helper::GetSourcePath(TM->GetTreeNode(index), is_system);
}

std::string BehavioralHelper::print_vertex(const OpGraphConstRef g, const vertex v, const var_pp_functorConstRef vppf,
                                           bool dot) const
{
   const auto node = g->CGetOpNodeInfo(v)->node;
   std::string res;
   if(node && node->get_kind() != gimple_nop_K)
   {
      res = PrintNode(node, v, vppf);
      switch(node->get_kind())
      {
         case gimple_assign_K:
         case gimple_call_K:
         case gimple_asm_K:
         case gimple_goto_K:
            res += ";";
            break;
         case binfo_K:
         case block_K:
         case call_expr_K:
         case aggr_init_expr_K:
         case case_label_expr_K:
         case constructor_K:
         case gimple_bind_K:
         case gimple_cond_K:
         case gimple_for_K:
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
         case CASE_PRAGMA_NODES:
         case CASE_QUATERNARY_EXPRESSION:
         case CASE_TERNARY_EXPRESSION:
         case CASE_TYPE_NODES:
         case CASE_UNARY_EXPRESSION:
         default:
            break;
      }
   }
   if(dot)
   {
      boost::replace_all(res, "\\\"", "&quot;");
      std::string ret;
      for(const auto& re : res)
      {
         if(re == '\"')
         {
            ret += "\\\"";
         }
         else if(re != '\n')
         {
            ret += re;
         }
         else
         {
            ret += "\\\n";
         }
      }
      ret += "\\n";
      return ret;
   }
   else if(res != "")
   {
      res += "\n";
   }
   return res;
}

std::string BehavioralHelper::PrintInit(const tree_nodeConstRef& node, const var_pp_functorConstRef vppf) const
{
   std::string res;
   switch(node->get_kind())
   {
      case constructor_K:
      {
         const auto constr = GetPointerS<const constructor>(node);
         bool designated_initializers_needed = false;
         res += '{';
         auto i = constr->list_of_idx_valu.begin();
         const auto vend = constr->list_of_idx_valu.end();
         /// check if designated initializers are really needed
         const auto firstnode = i != vend ? constr->list_of_idx_valu.front().first : tree_nodeRef();
         if(firstnode && firstnode->get_kind() == field_decl_K)
         {
            const auto fd = GetPointerS<const field_decl>(firstnode);
            const auto scpe = fd->scpe;
            std::vector<tree_nodeRef> field_list;
            if(scpe->get_kind() == record_type_K)
            {
               field_list = GetPointerS<const record_type>(scpe)->list_of_flds;
            }
            else if(scpe->get_kind() == union_type_K)
            {
               field_list = GetPointerS<const union_type>(scpe)->list_of_flds;
            }
            else
            {
               THROW_ERROR("expected a record_type or a union_type");
            }
            auto fli = field_list.begin();
            const auto flend = field_list.end();
            for(; fli != flend && i != vend; ++i, ++fli)
            {
               if(i->first && i->first->index != (*fli)->index)
               {
                  break;
               }
            }
            if(fli != flend && i != vend)
            {
               designated_initializers_needed = true;
            }
         }
         else
         {
            designated_initializers_needed = true;
         }

         auto current_length = res.size();
         for(i = constr->list_of_idx_valu.begin(); i != vend;)
         {
            std::string current;
            if(designated_initializers_needed && i->first && i->first->get_kind() == field_decl_K)
            {
               current += ".";
               current += PrintVariable(i->first->index);
               current += "=";
            }
            auto val = i->second;
            THROW_ASSERT(val, "Something of unexpected happen");
            if(val->get_kind() == addr_expr_K)
            {
               const auto ae = GetPointerS<const addr_expr>(val);
               const auto op = ae->op;
               if(op->get_kind() == function_decl_K)
               {
                  val = ae->op;
                  THROW_ASSERT(val, "Something of unexpected happen");
               }
            }
            if(val->get_kind() == function_decl_K)
            {
               current += tree_helper::print_function_name(TM, GetPointerS<const function_decl>(val));
            }
            else if(val->get_kind() == constructor_K)
            {
               current += PrintInit(i->second, vppf);
            }
            else if(val->get_kind() == var_decl_K)
            {
               current += PrintVariable(i->second->index);
            }
            else if(val->get_kind() == ssa_name_K)
            {
               current += PrintVariable(i->second->index);
            }
            else
            {
               current += PrintNode(i->second, vertex(), vppf);
            }
            ++i;
            if(i != vend)
            {
               current += ", ";
            }
            if((current.size() + current_length) > MAX_ROW_LENGTH)
            {
               current_length = current.size();
               res += "\n" + current;
            }
            else
            {
               current_length += current.size();
               res += current;
            }
         }
         res += '}';
         break;
      }
      case addr_expr_K:
      case nop_expr_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      {
         res = PrintConstant(node, vppf);
         break;
      }
      case pointer_plus_expr_K:
      case plus_expr_K:
      {
         vertex dummy_vertex = NULL_VERTEX;
         res += PrintNode(node, dummy_vertex, vppf);
         break;
      }
      case var_decl_K:
      {
         const auto vd = GetPointerS<const var_decl>(node);
         THROW_ASSERT(vd->init, "expected a initialization value: " + STR(node));
         res += PrintInit(vd->init, vppf);
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case ssa_name_K:
      case statement_list_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
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
      case paren_expr_K:
      case realpart_expr_K:
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
      case mem_ref_K:
      case min_expr_K:
      case minus_expr_K:
      case modify_expr_K:
      case mult_expr_K:
      case mult_highpart_expr_K:
      case ne_expr_K:
      case ordered_expr_K:
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
      case const_decl_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case parm_decl_K:
      case result_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case error_mark_K:
      case using_decl_K:
      case type_decl_K:
      case identifier_node_K:
      case alignof_expr_K:
      case arrow_expr_K:
      case reference_expr_K:
      case abs_expr_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case target_expr_K:
      case extract_bit_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      case frem_expr_K:
      default:
         THROW_ERROR("Currently not supported nodeID " + STR(node));
   }
   return res;
}

std::string BehavioralHelper::print_attributes(unsigned int var, const var_pp_functorConstRef vppf, bool first) const
{
   std::string res;
   if(first)
   {
      res += "__attribute__ (";
   }
   auto tl = GetPointerS<const tree_list>(TM->GetTreeNode(var));
   if(tl->purp)
   {
      res += "(" + PrintVariable(tl->purp->index);
   }
   if(tl->valu && tl->valu->get_kind() == tree_list_K)
   {
      res += print_attributes(tl->valu->index, vppf, false);
   }
   else if(tl->valu && (tl->valu->get_kind() == string_cst_K || tl->valu->get_kind() == integer_cst_K))
   {
      res += "(" + PrintConstant(tl->valu) + ")";
   }
   else if(tl->valu)
   {
      THROW_ERROR("Not yet supported: " + std::string(tl->valu->get_kind_text()));
   }
   if(tl->purp)
   {
      res += ")";
   }
   if(tl->chan)
   {
      res += " " + print_attributes(tl->chan->index, vppf, false);
   }
   if(first)
   {
      res += ")";
   }
   return res;
}

std::string BehavioralHelper::PrintVariable(unsigned int var) const
{
   if(vars_renaming_table.find(var) != vars_renaming_table.end())
   {
      return vars_renaming_table.find(var)->second;
   }
   if(vars_symbol_table[var] != "")
   {
      return vars_symbol_table[var];
   }
   if(var == default_COND)
   {
      return "default";
   }
   const auto var_node = TM->GetTreeNode(var);
   if(var_node->get_kind() == indirect_ref_K)
   {
      const auto ir = GetPointerS<const indirect_ref>(var_node);
      auto pointer = ir->op->index;
      std::string pointer_name = PrintVariable(pointer);
      vars_symbol_table[var] = "*" + pointer_name;
      return vars_symbol_table[var];
   }
   if(var_node->get_kind() == misaligned_indirect_ref_K)
   {
      const auto mir = GetPointerS<const misaligned_indirect_ref>(var_node);
      auto pointer = mir->op->index;
      std::string pointer_name = PrintVariable(pointer);
      vars_symbol_table[var] = "*" + pointer_name;
      return vars_symbol_table[var];
   }
   if(var_node->get_kind() == mem_ref_K)
   {
      const auto mr = GetPointerS<const mem_ref>(var_node);
      const auto offset = tree_helper::GetConstValue(mr->op1);
      const tree_manipulationRef tm(new tree_manipulation(AppM->get_tree_manager(), Param, AppM));
      const auto pointer_type = tm->GetPointerType(mr->type, 8);
      const auto type_string = tree_helper::PrintType(TM, pointer_type);
      if(offset == 0)
      {
         vars_symbol_table[var] = "*((" + type_string + ")(" + PrintVariable(mr->op0->index) + "))";
      }
      else
      {
         vars_symbol_table[var] =
             "*((" + type_string + ")(((unsigned char*)" + PrintVariable(mr->op0->index) + ") + " + STR(offset) + "))";
      }
      return vars_symbol_table[var];
   }
   if(var_node->get_kind() == identifier_node_K)
   {
      const auto in = GetPointerS<const identifier_node>(var_node);
      vars_symbol_table[var] = in->strg;
      return vars_symbol_table[var];
   }
   if(var_node->get_kind() == field_decl_K)
   {
      const auto fd = GetPointerS<const field_decl>(var_node);
      if(fd->name)
      {
         const auto id = GetPointerS<const identifier_node>(fd->name);
         return tree_helper::NormalizeTypename(id->strg);
      }
      else
      {
         return INTERNAL + STR(var);
      }
   }
   if(var_node->get_kind() == function_decl_K)
   {
      const auto fd = GetPointerS<const function_decl>(var_node);
      return tree_helper::print_function_name(TM, fd);
   }
   if(var_node->get_kind() == ssa_name_K)
   {
      const auto sa = GetPointerS<const ssa_name>(var_node);
      std::string name;
      if(sa->var)
      {
         name = PrintVariable(sa->var->index);
      }
      THROW_ASSERT(sa->volatile_flag || sa->CGetDefStmts().size(), sa->ToString() + " has not define statement");
      if(sa->virtual_flag || (!sa->volatile_flag && sa->CGetDefStmt()->get_kind() != gimple_nop_K))
      {
         name += ("_" + STR(sa->vers));
      }
      else
      {
         THROW_ASSERT(sa->var, "the name has to be defined for volatile or parameters");
      }
      // if(sa->min && sa->max) name += "/*[" + PrintConstant(sa->min) + "," + PrintConstant(sa->max) + "]*/";
      return name;
   }
   if(var_node->get_kind() == var_decl_K || var_node->get_kind() == parm_decl_K)
   {
      const auto dn = GetPointerS<const decl_node>(var_node);
      if(dn->name)
      {
         const auto id = GetPointerS<const identifier_node>(dn->name);
         vars_symbol_table[var] = tree_helper::NormalizeTypename(id->strg);
         return vars_symbol_table[var];
      }
   }
   if(is_a_constant(var))
   {
      vars_symbol_table[var] = PrintConstant(var_node);
   }
   if(vars_symbol_table[var] == "")
   {
      vars_symbol_table[var] = INTERNAL + STR(var);
   }
   return vars_symbol_table[var];
}

std::string BehavioralHelper::PrintConstant(const tree_nodeConstRef& node, const var_pp_functorConstRef vppf) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Printing constant " + STR(node));
   THROW_ASSERT(is_a_constant(node->index), std::string("Object is not a constant ") + STR(node));
   if(node->index == default_COND)
   {
      return "default";
   }
   std::string res;
   switch(node->get_kind())
   {
      case integer_cst_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->integer_cst");
         const auto ic = GetPointerS<const integer_cst>(node);
         const auto type = ic->type;
         const auto it = GetPointer<const integer_type>(type);
         const auto unsigned_flag = (it && it->unsigned_flag) || type->get_kind() == pointer_type_K ||
                                    type->get_kind() == reference_type_K || type->get_kind() == boolean_type_K;
         THROW_ASSERT(ic, "");
         auto value = tree_helper::GetConstValue(node, !unsigned_flag);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Value is " + STR(value));
         // TODO: fix for bitwidth greater than 64 bits
         if((it && it->prec == 64) && (value == (static_cast<long long int>(-0x08000000000000000LL))))
         {
            res = "(long long int)-0x08000000000000000";
         }
         else if((it && it->prec == 32) && (value == (static_cast<long int>(-0x080000000L))))
         {
            res = "(long int)-0x080000000";
         }
         else
         {
            if(it && it->unsigned_flag)
            {
               res += STR(value & ((integer_cst_t(1) << it->prec) - 1));
            }
            else
            {
               res += STR(value);
            }
         }
         if(it && it->prec > 32)
         {
            if(it && it->unsigned_flag)
            {
               res += "LLU";
            }
            else
            {
               res += "LL";
            }
         }
         else
         {
            if(unsigned_flag)
            {
               res += "u";
            }
            else if(type->get_kind() == pointer_type_K || type->get_kind() == reference_type_K)
            {
               res += "/*B*/";
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
         break;
      }
      case real_cst_K:
      {
         const auto rc = GetPointerS<const real_cst>(node);
         if(rc->overflow_flag)
         {
            res += " overflow";
         }
         if(tree_helper::IsRealType(rc->type))
         {
            if(GetPointerS<const real_type>(rc->type)->prec == 80) /// long double
            {
               if(strcasecmp(rc->valr.data(), "Inf") == 0)
               {
                  if(rc->valx[0] == '-')
                  {
                     res += "-";
                  }
                  res += "__builtin_infl()";
               }
               else if(strcasecmp(rc->valr.data(), "Nan") == 0)
               {
                  if(rc->valx[0] == '-')
                  {
                     res += "-";
                  }
                  res += "__builtin_nanl(\"\")";
               }
               else
               {
                  res += rc->valr;
                  res += "L";
               }
            }
            else if(GetPointerS<const real_type>(rc->type)->prec == 64) /// double
            {
               if(strcasecmp(rc->valr.data(), "Inf") == 0)
               {
                  if(rc->valx[0] == '-')
                  {
                     res += "-";
                  }
                  res += "__builtin_inf()";
               }
               else if(strcasecmp(rc->valr.data(), "Nan") == 0)
               {
                  if(rc->valx[0] == '-')
                  {
                     res += "-";
                  }
                  res += "__builtin_nan(\"\")";
               }
               else
               {
                  res += rc->valr;
               }
            }
            else if(strcasecmp(rc->valr.data(), "Inf") == 0) /// float
            {
               if(rc->valx[0] == '-')
               {
                  res += "-";
               }
               res += "__builtin_inff()";
            }
            else if(strcasecmp(rc->valr.data(), "Nan") == 0)
            {
               if(rc->valx[0] == '-')
               {
                  res += "-";
               }
               res += "__builtin_nanf(\"\")";
            }
            else
            {
               /// FIXME: float can not be used for imaginary part of complex number
               res += /*"(float)" +*/ rc->valr;
            }
         }
         else
         {
            THROW_ERROR(std::string("Node not yet supported: ") + node->get_kind_text());
         }
         break;
      }
      case complex_cst_K:
      {
         const auto cc = GetPointerS<const complex_cst>(node);
         res += "(";
         res += PrintConstant(cc->real, vppf);
         res += "+";
         res += PrintConstant(cc->imag, vppf);
         res += "*1i)";
         break;
      }
      case string_cst_K:
      {
         const auto sc = GetPointerS<const string_cst>(node);
         if(sc->type)
         {
            const auto at = GetPointer<const array_type>(sc->type);
            THROW_ASSERT(at, "Expected an array type");
            const auto elts = GetPointerS<const integer_type>(at->elts);
            if(elts->prec == 32) // wide char string
            {
               if(elts->unsigned_flag && !tree_helper::IsSystemType(at->elts))
               {
                  THROW_ERROR_CODE(C_EC, "Unsigned wide char not supported");
               }
               res = "L";
            }
         }
         res += "\"" + sc->strg + "\"";
         break;
      }
      case vector_cst_K:
      {
         const auto vc = GetPointerS<const vector_cst>(node);
         THROW_ASSERT(vc->type->get_kind() == vector_type_K, "Vector constant of type " + vc->type->get_kind_text());
         if(GetPointerS<const vector_type>(vc->type)->elts->get_kind() != pointer_type_K)
         {
            res += "(" + tree_helper::PrintType(TM, vc->type, false, true) + ") ";
         }
         res += "{ ";
         for(unsigned int i = 0; i < (vc->list_of_valu).size(); i++) // vector elements
         {
            res += PrintConstant(vc->list_of_valu[i], vppf);
            if(i != vc->list_of_valu.size() - 1)
            { // not the last element element
               res += ", ";
            }
         }
         res += " }";
         break;
      }
      case case_label_expr_K:
      {
         const auto cl = GetPointerS<const case_label_expr>(node);
         if(cl->default_flag)
         {
            res += "default";
         }
         else
         {
            if(cl->op1)
            {
               THROW_ASSERT(GetPointer<integer_cst>(cl->op0),
                            "Case label expression " + STR(node) + " does not use integer_cst");
               THROW_ASSERT(GetPointer<integer_cst>(cl->op1),
                            "Case label expression " + STR(node) + " does not use integer_cst");
               auto low = tree_helper::GetConstValue(cl->op0);
               auto high = tree_helper::GetConstValue(cl->op1);
               while(low < high)
               {
                  res += STR(low) + "u : case ";
                  low++;
               }
               res += STR(low) + "u";
            }
            else
            {
               res += PrintConstant(cl->op0, vppf);
            }
         }
         break;
      }
      case label_decl_K:
      {
         const auto ld = GetPointerS<const label_decl>(node);
         THROW_ASSERT(ld->name, "name expected in a label_decl");
         const auto id = GetPointer<const identifier_node>(ld->name);
         THROW_ASSERT(id, "expected an identifier_node");
         res += id->strg;
         break;
      }
      case nop_expr_K:
      {
         const auto ue = GetPointerS<const unary_expr>(node);
         if(!(ue->op->get_kind() == addr_expr_K &&
              GetPointerS<const addr_expr>(ue->op)->op->get_kind() == label_decl_K))
         {
            res += "(" + tree_helper::PrintType(TM, ue->type) + ") ";
         }
         res += PrintConstant(ue->op, vppf);
         break;
      }
      case addr_expr_K:
      {
         const auto ue = GetPointerS<const unary_expr>(node);
         if(is_a_constant(ue->op->index))
         {
            res += "(&(" + PrintConstant(ue->op) + "))";
         }
         else if(ue->op->get_kind() == function_decl_K)
         {
            res += tree_helper::print_function_name(TM, GetPointerS<const function_decl>(ue->op));
         }
         else
         {
            if(vppf)
            {
               res += "(&(" + (*vppf)(ue->op->index) + "))";
            }
            else
            {
               res += "(&(" + PrintVariable(ue->op->index) + "))";
            }
         }
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case const_decl_K:
      case field_decl_K:
      case function_decl_K:
      case namespace_decl_K:
      case parm_decl_K:
      case result_decl_K:
      case translation_unit_decl_K:
      case using_decl_K:
      case type_decl_K:
      case var_decl_K:
      case template_decl_K:
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
      case negate_expr_K:
      case non_lvalue_expr_K:
      case paren_expr_K:
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
      case void_cst_K:
      default:
         THROW_ERROR("Var object is not a constant " + STR(node) + " " + node->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Printed constant " + res);
   return res;
}

unsigned long long BehavioralHelper::get_size(unsigned int var) const
{
   return tree_helper::Size(TM->GetTreeNode(var));
}

std::string BehavioralHelper::get_function_name() const
{
   return function_name;
}

std::string BehavioralHelper::GetMangledFunctionName() const
{
   return tree_helper::GetMangledFunctionName(GetPointerS<const function_decl>(TM->GetTreeNode(function_index)));
}

unsigned int BehavioralHelper::get_function_index() const
{
   return function_index;
}

unsigned int BehavioralHelper::GetFunctionReturnType(unsigned int function) const
{
   const auto return_type = tree_helper::GetFunctionReturnType(TM->GetTreeNode(function));
   if(return_type)
   {
      return return_type->index;
   }
   else
   {
      return 0;
   }
}

const std::list<unsigned int> BehavioralHelper::get_parameters() const
{
   std::list<unsigned int> parameters;
   const auto fun = TM->GetTreeNode(function_index);
   const auto fd = GetPointerS<const function_decl>(fun);
   const auto& list_of_args = fd->list_of_args;
   if(fd->list_of_args.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Parameter list size: " + STR(list_of_args.size()));
      for(const auto& list_of_arg : list_of_args)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Adding parameter " + STR(list_of_arg->index));
         parameters.push_back(list_of_arg->index);
      }
   }
   else
   {
      const auto ft = GetPointerS<const function_type>(fd->type);
      if(ft->prms)
      {
         auto currentp = ft->prms;
         while(currentp)
         {
            const auto tl = GetPointerS<const tree_list>(currentp);
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Adding parameter " + STR(tl->valu->index));
            if(tl->valu->get_kind() != void_type_K)
            {
               parameters.push_back(tl->valu->index);
            }
            currentp = tl->chan;
         }
      }
   }
   return parameters;
}

std::vector<tree_nodeRef> BehavioralHelper::GetParameters() const
{
   const auto fun = TM->GetTreeNode(function_index);
   const auto fd = GetPointerS<const function_decl>(fun);
   if(fd->list_of_args.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Parameter list size: " + STR(fd->list_of_args.size()));
      return fd->list_of_args;
   }

   std::vector<tree_nodeRef> parameters;
   const auto ft = GetPointerS<const function_type>(fd->type);
   if(ft->prms)
   {
      auto currentp = ft->prms;
      while(currentp)
      {
         const auto tl = GetPointerS<const tree_list>(currentp);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Adding parameter " + STR(tl->valu->index));
         if(tl->valu->get_kind() != void_type_K)
         {
            parameters.push_back(tl->valu);
         }
         currentp = tl->chan;
      }
   }
   return parameters;
}

bool BehavioralHelper::has_implementation() const
{
   return body;
}

void BehavioralHelper::add_initialization(unsigned int var, unsigned int init)
{
   initializations[var] = init;
}

void BehavioralHelper::set_opaque()
{
   opaque = true;
}

void BehavioralHelper::set_not_opaque()
{
   opaque = false;
}

bool BehavioralHelper::get_opaque() const
{
   return opaque;
}

unsigned int BehavioralHelper::get_type(const unsigned int var) const
{
   return tree_helper::get_type_index(TM, var);
}

unsigned int BehavioralHelper::get_pointed_type(const unsigned int type) const
{
   return tree_helper::CGetPointedType(TM->GetTreeNode(type))->index;
}

unsigned int BehavioralHelper::GetElements(const unsigned int type) const
{
   return tree_helper::CGetElements(TM->GetTreeNode(type))->index;
}

std::string BehavioralHelper::PrintVarDeclaration(unsigned int var, var_pp_functorConstRef vppf,
                                                  bool init_has_to_be_printed) const
{
   std::string return_value;
   const auto curr_tn = TM->GetTreeNode(var);
   THROW_ASSERT(GetPointer<const decl_node>(curr_tn) || GetPointer<const ssa_name>(curr_tn),
                "Call pparameter_type_indexrint_var_declaration on node " + STR(var) + " which is of type " +
                    curr_tn->get_kind_text());
   const decl_node* dn = nullptr;
   if(GetPointer<const decl_node>(curr_tn))
   {
      dn = GetPointerS<const decl_node>(curr_tn);
   }
   /// If it is not a decl node (then it is an ssa-name) or it's a not system decl_node
   if(!dn || !(dn->operating_system_flag || dn->library_system_flag) || tree_helper::IsInLibbambu(curr_tn))
   {
      return_value += tree_helper::PrintType(TM, tree_helper::CGetType(curr_tn), false, false, init_has_to_be_printed,
                                             curr_tn, vppf);
      auto attributes = get_attributes(var);
      CustomUnorderedSet<unsigned int> list_of_variables;
      const unsigned int init = GetInit(var, list_of_variables);
      if(attributes)
      {
         return_value += " " + print_attributes(attributes, vppf);
      }
      if(dn && dn->packed_flag)
      {
         return_value += " __attribute__((packed))";
      }
      if(dn && ((dn->orig && (GetPointer<const var_decl>(curr_tn)) && (GetPointer<const var_decl>(dn->orig)) &&
                 (GetPointer<const var_decl>(curr_tn)->algn != GetPointer<const var_decl>(dn->orig)->algn)) ||
                ((GetPointer<const var_decl>(curr_tn)) && (GetPointer<const var_decl>(curr_tn)->algn == 128))))
      {
         return_value += " __attribute__ ((aligned (" + STR(GetPointerS<const var_decl>(curr_tn)->algn / 8) + "))) ";
      }
      if(init && init_has_to_be_printed)
      {
         return_value += " = " + PrintInit(TM->GetTreeNode(init), vppf);
      }
   }
   return return_value;
}

bool BehavioralHelper::is_var_args() const
{
   tree_nodeRef tn = TM->GetTreeNode(function_index);
   THROW_ASSERT(tn->get_kind() == function_decl_K, "function_index is not a function decl");
   tn = GetPointer<function_decl>(tn)->type;
   return GetPointer<function_type>(tn)->varargs_flag;
}

std::string BehavioralHelper::PrintNode(unsigned int _node, vertex v, const var_pp_functorConstRef vppf) const
{
   const auto node = TM->GetTreeNode(_node);
   return PrintNode(node, v, vppf);
}

std::string BehavioralHelper::PrintNode(const tree_nodeConstRef& node, vertex v,
                                        const var_pp_functorConstRef vppf) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Printing node " + STR(node));
   std::string res = "";
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Type is " + std::string(node->get_kind_text()));
   switch(node->get_kind())
   {
         /* Binary arithmetic and logic expressions.  */
      case sat_plus_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         const auto res_size = tree_helper::Size(be->type);
         std::string left, right;
         left = ("(" + PrintNode(be->op0, v, vppf) + ")");
         right = ("(" + PrintNode(be->op1, v, vppf) + ")");
         if(tree_helper::IsSignedIntegerType(be->type))
         {
            const auto op_size = tree_helper::Size(be->op0);
            res += "((((" + left + " & " + STR(1ULL << (op_size - 1)) + "ULL) ^ (" + right + " & " +
                   STR(1ULL << (op_size - 1)) +
                   "ULL)) |"
                   "((" +
                   left + " & " + STR(1ULL << (op_size - 1)) + "ULL) == ((" + left + " + " + right + ") & " +
                   STR(1ULL << (op_size - 1)) +
                   "ULL))) ? "
                   "(" +
                   left + " + " + right +
                   ") "
                   ": ((" +
                   left + " & " + STR(1ULL << (op_size - 1)) + "ULL) ? " +
                   STR(static_cast<long long>(-1ULL << (res_size - 1))) + " : " + STR((1LL << (res_size - 1)) - 1) +
                   "))";
         }
         else
         {
            res += "( (" + left + " + " + right + ") < " + right + " ? " + STR((1ULL << res_size) - 1) +
                   (res_size > 32 ? "ULL" : "") + " : " + left + " + " + right + ")";
         }
         break;
      }
      case sat_minus_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string left, right;
         left = ("(" + PrintNode(be->op0, v, vppf) + ")");
         right = ("(" + PrintNode(be->op1, v, vppf) + ")");
         if(tree_helper::IsSignedIntegerType(be->type))
         {
            const auto res_size = tree_helper::Size(be->type);
            const auto op_size = tree_helper::Size(be->op0);
            res += "((((" + left + " & " + STR(1ULL << (op_size - 1)) + "ULL) ^ (~" + right + " & " +
                   STR(1ULL << (op_size - 1)) +
                   "ULL)) | "
                   "((" +
                   left + " & " + STR(1ULL << (op_size - 1)) + "ULL) == ((" + left + " - " + right + ") & " +
                   STR(1ULL << (op_size - 1)) +
                   "ULL))) ? "
                   "(" +
                   left + " - " + right +
                   ") "
                   ": ((" +
                   left + " & " + STR(1ULL << (op_size - 1)) + "ULL) ? " +
                   STR(static_cast<long long>(-1ULL << (res_size - 1))) + " : " + STR((1LL << (res_size - 1)) - 1) +
                   "))";
         }
         else
         {
            res += "( " + left + " > " + right + " ? " + left + " - " + right + " : 0)";
         }
         break;
      }
      case frem_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string left, right;
         THROW_ASSERT(GetPointer<const real_type>(be->type), "unexpected case");
         left = ("(" + PrintNode(be->op0, v, vppf) + ")");
         right = ("(" + PrintNode(be->op1, v, vppf) + ")");

         const auto rt = GetPointerS<const real_type>(be->type);
         if(rt->prec == 80)
         {
            res += "fmodl(" + left + "," + right + ")";
         }
         else if(rt->prec == 64)
         {
            res += "fmod(" + left + "," + right + ")";
         }
         else if(rt->prec == 32)
         {
            res += "fmodf(" + left + "," + right + ")";
         }
         else
         {
            THROW_ERROR("fmod on a real number with not supported precision");
         }
         break;
      }
      case plus_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         const auto op = tree_helper::op_symbol(node);
         const auto left_op_type = tree_helper::CGetType(be->op0);
         const auto right_op_type = tree_helper::CGetType(be->op1);
         const auto vector = tree_helper::IsVectorType(be->type) &&
                             ((tree_helper::IsVectorType(left_op_type) && left_op_type->index != be->type->index) ||
                              (tree_helper::IsVectorType(right_op_type) && right_op_type->index != be->type->index));
         if(vector)
         {
            const auto element_type = tree_helper::CGetElements(be->type);
            const auto element_size = tree_helper::SizeAlloc(element_type);
            const auto size = tree_helper::SizeAlloc(be->type);
            const auto vector_size = size / element_size;
            res += "(" + tree_helper::PrintType(TM, be->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(be->op0, v, vppf) + ")[" + STR(ind) + "] + (" + PrintNode(be->op1, v, vppf) +
                      ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
         }
         else
         {
            const auto type = tree_helper::CGetType(node);
            unsigned int prec = 0;
            unsigned int algn = 0;
            if(type && type->get_kind() == integer_type_K)
            {
               prec = GetPointerS<const integer_type>(type)->prec;
               algn = GetPointerS<const integer_type>(type)->algn;
            }
            // bitfield type
            if(prec != algn && prec % algn)
            {
               res += "((";
            }
            res += "(" + tree_helper::PrintType(TM, be->type) + ")(";

            if(GetPointer<const decl_node>(be->op0) || GetPointer<const ssa_name>(be->op0))
            {
               res += PrintNode(be->op0, v, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op0, v, vppf) + ")");
            }
            res += std::string(" ") + op + " ";

            if(GetPointer<const decl_node>(be->op1) || GetPointer<const ssa_name>(be->op1))
            {
               res += PrintNode(be->op1, v, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op1, v, vppf) + ")");
            }
            res += ")";
            if(prec != algn && prec % algn)
            {
               res += ")%(1";
               if(prec > 32)
               {
                  res += "LL";
               }
               if(GetPointerS<const integer_type>(type)->unsigned_flag)
               {
                  res += "U";
               }
               res += " << " + STR(prec) + "))";
            }
         }
         break;
      }
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case bit_and_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         const auto left_op_type = tree_helper::CGetType(be->op0);
         const auto right_op_type = tree_helper::CGetType(be->op1);
         const auto op = tree_helper::op_symbol(node);
         const auto vector = tree_helper::IsVectorType(be->type) &&
                             ((tree_helper::IsVectorType(left_op_type) && left_op_type->index != be->type->index) ||
                              (tree_helper::IsVectorType(right_op_type) && right_op_type->index != be->type->index));
         if(vector)
         {
            const auto element_type = tree_helper::CGetElements(be->type);
            const auto element_size = tree_helper::SizeAlloc(element_type);
            const auto size = tree_helper::SizeAlloc(be->type);
            const auto vector_size = size / element_size;
            res += "(" + tree_helper::PrintType(TM, be->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(be->op0, v, vppf) + ")[" + STR(ind) + "] " + op + " (" +
                      PrintNode(be->op1, v, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
         }
         else
         {
            const auto type = tree_helper::CGetType(node);
            bool bit_expression = type && type->get_kind() == pointer_type_K;
            unsigned int prec = 0;
            unsigned int algn = 0;
            if(type && type->get_kind() == integer_type_K)
            {
               prec = GetPointerS<const integer_type>(type)->prec;
               algn = GetPointerS<const integer_type>(type)->algn;
            }
            // bitfield type
            if(prec != algn && prec % algn)
            {
               res += "((";
            }
            if(bit_expression)
            {
               res += "((" + tree_helper::PrintType(TM, tree_helper::CGetType(node)) + ")(((unsigned)(";
            }

            if(GetPointer<const decl_node>(be->op0) || GetPointer<const ssa_name>(be->op0))
            {
               res += PrintNode(be->op0, v, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op0, v, vppf) + ")");
            }
            if(bit_expression)
            {
               res += "))";
            }
            res += std::string(" ") + op + " ";
            if(bit_expression)
            {
               res += "((unsigned)(";
            }

            if(GetPointer<const decl_node>(be->op1) || GetPointer<const ssa_name>(be->op1))
            {
               res += PrintNode(be->op1, v, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op1, v, vppf) + ")");
            }
            if(bit_expression)
            {
               res += "))))";
            }
            if(prec != algn && prec % algn)
            {
               res += ")%(1";
               if(prec > 32)
               {
                  res += "LL";
               }
               if(GetPointerS<const integer_type>(type)->unsigned_flag)
               {
                  res += "U";
               }
               res += " << " + STR(prec) + "))";
            }
         }
         break;
      }
      case vec_rshift_expr_K:
      {
         const auto vre = GetPointerS<const vec_rshift_expr>(node);
         const auto element_type = tree_helper::CGetElements(vre->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vre->type);
         const auto vector_size = size / element_size;
         res += "/*" + vre->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vre->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            for(unsigned int k = ind; k < vector_size; ++k)
            {
               res += "((" + PrintNode(vre->op1, v, vppf) + " >= " + STR(element_size * (k - ind)) + "&&" +
                      PrintNode(vre->op1, v, vppf) + " < " + STR(element_size * (k - ind + 1)) + ") ? ((" +
                      PrintNode(vre->op0, v, vppf) + ")[" + STR(k) + "]) >> (" + PrintNode(vre->op1, v, vppf) + "-" +
                      STR(element_size * (k - ind)) + "): 0)";
               if(k != vector_size - 1)
               {
                  res += "|";
               }
            }

            for(unsigned int k = ind + 1; k < vector_size; ++k)
            {
               res += "| ((" + PrintNode(vre->op1, v, vppf) + " > " + STR(element_size * (k - 1 - ind)) + "&&" +
                      PrintNode(vre->op1, v, vppf) + " < " + STR(element_size * (k - ind)) + ") ? ((" +
                      PrintNode(vre->op0, v, vppf) + ")[" + STR(k) + "]) << (" + STR(element_size * (k - ind)) + "-" +
                      PrintNode(vre->op1, v, vppf) + "): 0)";
            }
            if(ind != vector_size - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case vec_lshift_expr_K:
      {
         const auto vle = GetPointerS<const vec_lshift_expr>(node);
         const auto element_type = tree_helper::CGetElements(vle->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vle->type);
         const auto vector_size = size / element_size;
         res += "/*" + vle->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vle->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            for(unsigned int k = 0; k <= ind; ++k)
            {
               res += "((" + PrintNode(vle->op1, v, vppf) + " >= " + STR(element_size * (k)) + "&&" +
                      PrintNode(vle->op1, v, vppf) + " < " + STR(element_size * (k + 1)) + ") ? ((" +
                      PrintNode(vle->op0, v, vppf) + ")[" + STR(ind - k) + "]) << (" + PrintNode(vle->op1, v, vppf) +
                      "-" + STR(element_size * k) + "): 0)";
               if(k != ind)
               {
                  res += "|";
               }
            }
            for(unsigned int k = 0; k < ind; ++k)
            {
               res += "| ((" + PrintNode(vle->op1, v, vppf) + " > " + STR(element_size * (k)) + "&&" +
                      PrintNode(vle->op1, v, vppf) + " < " + STR(element_size * (k + 1)) + ") ? ((" +
                      PrintNode(vle->op0, v, vppf) + ")[" + STR(ind - k - 1) + "]) >> (" + STR(element_size * (k + 1)) +
                      "-" + PrintNode(vle->op1, v, vppf) + "): 0)";
            }
            if(ind != vector_size - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case mult_highpart_expr_K:
      {
         THROW_UNREACHABLE("Not implemented");
         break;
      }
      case mult_expr_K:
      case minus_expr_K:
      case trunc_div_expr_K:
      case ceil_div_expr_K:
      case floor_div_expr_K:
      case round_div_expr_K:
      case trunc_mod_expr_K:
      case ceil_mod_expr_K:
      case floor_mod_expr_K:
      case round_mod_expr_K:
      case rdiv_expr_K:
      case exact_div_expr_K:
      case lshift_expr_K:
      case rshift_expr_K:
      case truth_andif_expr_K:
      case truth_orif_expr_K:
      case truth_and_expr_K:
      case truth_or_expr_K:
      case truth_xor_expr_K:
      {
         const auto op = tree_helper::op_symbol(node);
         const auto be = GetPointerS<const binary_expr>(node);
         const auto left_op_type = tree_helper::CGetType(be->op0);
         const auto right_op_type = tree_helper::CGetType(be->op1);
         const auto vector = tree_helper::IsVectorType(be->type) &&
                             ((tree_helper::IsVectorType(left_op_type) && left_op_type->index != be->type->index) ||
                              (tree_helper::IsVectorType(right_op_type) && right_op_type->index != be->type->index));
         if(vector)
         {
            const auto element_type = tree_helper::CGetElements(be->type);
            const auto element_size = tree_helper::SizeAlloc(element_type);
            const auto size = tree_helper::SizeAlloc(be->type);
            const auto vector_size = size / element_size;
            res += "(" + tree_helper::PrintType(TM, be->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(be->op0, v, vppf) + ")[" + STR(ind) + "] " + op + " (" +
                      PrintNode(be->op1, v, vppf) + ")";
               if(be->op1->get_kind() != integer_cst_K)
               {
                  res += "[" + STR(ind) + "]";
               }
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
         }
         else
         {
            const auto type = tree_helper::CGetType(node);
            unsigned int prec = 0;
            unsigned int algn = 0;
            if(type && type->get_kind() == integer_type_K)
            {
               prec = GetPointerS<const integer_type>(type)->prec;
               algn = GetPointerS<const integer_type>(type)->algn;
            }
            // bitfield type
            if(prec != algn && prec % algn)
            {
               res += "((";
            }
            if((node->get_kind() == lshift_expr_K || node->get_kind() == rshift_expr_K) &&
               left_op_type->index != be->type->index)
            {
               res += "(" + tree_helper::PrintType(TM, be->type) + ")(";
            }
            if(GetPointer<const decl_node>(be->op0) || GetPointer<const ssa_name>(be->op0))
            {
               res += PrintNode(be->op0, v, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op0, v, vppf) + ")");
            }
            if((node->get_kind() == lshift_expr_K || node->get_kind() == rshift_expr_K) &&
               left_op_type->index != be->type->index)
            {
               res += ")";
            }
            res += std::string(" ") + op + " ";
            if(GetPointer<const decl_node>(be->op1) || GetPointer<const ssa_name>(be->op1))
            {
               res += PrintNode(be->op1, v, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op1, v, vppf) + ")");
            }
            if(prec != algn && prec % algn)
            {
               res += ")%(1";
               if(prec > 32)
               {
                  res += "LL";
               }
               if(GetPointerS<const integer_type>(type)->unsigned_flag)
               {
                  res += "U";
               }
               res += " << " + STR(prec) + "))";
            }
         }
         break;
      }
      case widen_sum_expr_K:
      case widen_mult_expr_K:
      {
         const auto op = tree_helper::op_symbol(node);
         const auto be = GetPointerS<const binary_expr>(node);
         const auto return_type = tree_helper::CGetType(node);

         res += "((" + tree_helper::PrintType(TM, return_type) + ")(" + PrintNode(be->op0, v, vppf) + "))";
         res += std::string(" ") + op + " ";
         res += "((" + tree_helper::PrintType(TM, return_type) + ")(" + PrintNode(be->op1, v, vppf) + "))";
         break;
      }
      case extract_bit_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         res += "(_Bool)(((unsigned long long int)(" + PrintNode(be->op0, v, vppf);
         res += std::string(") >> ");
         res += PrintNode(be->op1, v, vppf) + ") & 1)";
         break;
      }
      case extractvalue_expr_K:
      {
         const auto be = GetPointerS<const extractvalue_expr>(node);
         const auto return_type = tree_helper::CGetType(node);
         res += "(" + tree_helper::PrintType(TM, return_type) + ")(" + PrintNode(be->op0, v, vppf) + " >> " +
                PrintNode(be->op1, v, vppf) + ");";
         break;
      }
      case insertvalue_expr_K:
      {
         const auto te = GetPointerS<const insertvalue_expr>(node);
         unsigned long long op2_size;
         op2_size = tree_helper::Size(te->op2);
         const auto return_type = tree_helper::CGetType(node);
         res += "(" + tree_helper::PrintType(TM, return_type) + ")";
         res +=
             "(((" + PrintNode(te->op0, v, vppf) + " >> (" + STR(op2_size) + " + " + PrintNode(te->op2, v, vppf) + "))";
         res += " << (" + STR(op2_size) + " + " + PrintNode(te->op2, v, vppf) + ")) | (( " +
                PrintNode(te->op1, v, vppf) + " << ";
         res += PrintNode(te->op2, v, vppf) + ") | ((" + PrintNode(te->op0, v, vppf) + " << " +
                PrintNode(te->op2, v, vppf);
         res += ") >> " + PrintNode(te->op2, v, vppf) + ")))";
         break;
      }
      case extractelement_expr_K:
      {
         const auto be = GetPointerS<const extractvalue_expr>(node);
         const auto return_type = tree_helper::CGetType(node);
         res += "((" + tree_helper::PrintType(TM, return_type) + ")(" + PrintNode(be->op0, v, vppf) + "[" +
                PrintNode(be->op1, v, vppf) + "]))";
         break;
      }
      case insertelement_expr_K:
      {
         const auto iee = GetPointerS<const insertelement_expr>(node);
         const auto element_type = tree_helper::CGetElements(iee->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(iee->type);
         const auto vector_size = size / element_size;

         res += "/*" + iee->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, iee->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += "(" + PrintNode(iee->op2, v, vppf) + " == " + STR(ind) + " ? " + PrintNode(iee->op1, v, vppf) +
                   " : " + PrintNode(iee->op0, v, vppf) + "[" + STR(ind) + "])";
            if(ind != vector_size - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case pointer_plus_expr_K:
      {
         const auto ppe = GetPointerS<const pointer_plus_expr>(node);
         const auto op = tree_helper::op_symbol(node);
         const auto binary_op_cast = tree_helper::IsPointerType(node);
         const auto type_node = tree_helper::CGetType(ppe->op0);
         const auto left_op_cast = tree_helper::IsPointerType(ppe->op0);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->");
#ifndef NDEBUG
         if(left_op_cast)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Left part is a pointer");
         }
#endif
         bool do_reverse_pointer_arithmetic = false;
         auto right_op_node = ppe->op1;
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "Starting right op node is " + STR(ppe->op1->index) + " - " + right_op_node->get_kind_text());
         const auto right_cost = right_op_node->get_kind() == integer_cst_K;
         THROW_ASSERT(!tree_helper::IsPointerType(ppe->op1), "expected a right operand different from a pointer");
         THROW_ASSERT(tree_helper::IsPointerType(ppe->type),
                      "expected a pointer type: " + ppe->type->get_kind_text() + " - " + STR(ppe->type));

         /// check possible pointer arithmetic reverse
         unsigned long long deltabit;
         const auto pointed_type = tree_helper::CGetPointedType(tree_helper::CGetType(ppe->op0));
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "Pointed type (" + STR(pointed_type) + ") is " + pointed_type->get_kind_text());
         if(tree_helper::IsVoidType(pointed_type))
         {
            const auto vt = GetPointerS<const void_type>(pointed_type);
            deltabit = vt->algn;
         }
         else
         {
            deltabit = tree_helper::Size(pointed_type);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "deltabit is " + STR(deltabit));
         integer_cst_t pointer_offset = 0;
         std::string right_offset_var;
         if(right_cost)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Offset is constant");
            pointer_offset = tree_helper::GetConstValue(right_op_node, tree_helper::IsSignedIntegerType(right_op_node));
            if(deltabit / 8 == 0)
            {
               do_reverse_pointer_arithmetic = false;
            }
            else if(pointed_type->get_kind() != array_type_K && deltabit && pointer_offset > deltabit &&
                    ((pointer_offset % (deltabit / 8)) == 0))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Arithmetic pointer pattern matched");
               const auto ic = GetPointerS<const integer_cst>(right_op_node);
               const auto it = GetPointer<const integer_type>(ic->type);
               if(it && (it->prec == 32))
               {
                  pointer_offset = static_cast<unsigned int>(pointer_offset / (deltabit / 8));
               }
               else
               {
                  pointer_offset = pointer_offset / (deltabit / 8);
               }
               do_reverse_pointer_arithmetic = true;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "Arithmetic pointer pattern not matched " + STR(pointer_offset) + " vs " +
                                  STR(deltabit / 8));
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Offset is not constant");
            bool exit = pointed_type->get_kind() == array_type_K;
            while(!do_reverse_pointer_arithmetic && !exit)
            {
               switch(right_op_node->get_kind())
               {
                  case ssa_name_K:
                  {
                     if(GetPointerS<const ssa_name>(right_op_node)->CGetDefStmts().empty())
                     {
                        exit = true;
                        break;
                     }
                     const auto rssa = GetPointerS<const ssa_name>(right_op_node);
                     const auto defstmt = rssa->CGetDefStmt();
                     if(defstmt->get_kind() != gimple_assign_K)
                     {
                        exit = true;
                        break;
                     }
                     right_op_node = GetPointerS<const gimple_assign>(defstmt)->op1;
                     INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                    "New op node is " + STR(right_op_node->index) + " - " +
                                        right_op_node->get_kind_text());
                     break;
                  }
                  case mult_highpart_expr_K:
                  {
                     THROW_UNREACHABLE("");
                     break;
                  }
                  case mult_expr_K:
                  {
                     const auto mult = GetPointerS<const mult_expr>(right_op_node);
                     if(mult->op1->get_kind() == integer_cst_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "-->Right part of multiply is an integer constant " + STR(mult->op1->index));
                        const auto size_of_pointer = tree_helper::GetConstValue(mult->op1);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "---Size of pointer is " + STR(size_of_pointer));
                        if(size_of_pointer == (deltabit / 8))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Constant is the size of the pointed");
                           right_offset_var += PrintNode(mult->op0, v, vppf);
                           do_reverse_pointer_arithmetic = true;
                        }
                        else
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                          "-->Constant is not the size of the pointed: " + STR(size_of_pointer) +
                                              " vs " + STR(deltabit / 8));
                           const auto temp1 = tree_helper::CGetType(mult->op1);
                           THROW_ASSERT(GetPointer<const integer_type>(temp1),
                                        "Type of integer cast " + STR(mult->op1->index) + " is not an integer type");
                           const auto it = GetPointerS<const integer_type>(temp1);
                           const auto max_int = tree_helper::GetConstValue(it->max);
                           const auto new_size_of_pointer = max_int + 1 - size_of_pointer;
                           if(new_size_of_pointer == (deltabit / 8))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                             "---Constant is minus the size of the pointed");
                              right_offset_var += "-(" + PrintNode(mult->op0, v, vppf) + ")";
                              do_reverse_pointer_arithmetic = true;
                           }
                           else if(deltabit && (deltabit / 8) && (size_of_pointer % ((deltabit / 8)) == 0))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                             "---Constant is a multiple of the  size of the pointed");
                              right_offset_var += PrintNode(mult->op0, v, vppf);
                              right_offset_var += " * ";
                              right_offset_var += STR(size_of_pointer / ((deltabit / 8)));
                              do_reverse_pointer_arithmetic = true;
                           }
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "<--Right offset variable is " + right_offset_var);
                     }
                     else if(mult->op0->get_kind() == integer_cst_K)
                     {
                        const auto size_of_pointer = tree_helper::GetConstValue(mult->op0);
                        if(size_of_pointer == (deltabit / 8))
                        {
                           right_offset_var += PrintNode(mult->op1, v, vppf);
                           do_reverse_pointer_arithmetic = true;
                        }
                        else
                        {
                           const auto temp1 = tree_helper::CGetType(mult->op0);
                           THROW_ASSERT(GetPointer<const integer_type>(temp1),
                                        "Type of integer cast " + STR(mult->op0->index) + " is not an integer type");
                           const auto it = GetPointerS<const integer_type>(temp1);
                           const auto max_int = tree_helper::GetConstValue(it->max);
                           const auto new_size_of_pointer = max_int + 1 - size_of_pointer;
                           if(new_size_of_pointer == (deltabit / 8))
                           {
                              right_offset_var += "-" + PrintNode(mult->op1, v, vppf);
                              do_reverse_pointer_arithmetic = true;
                           }
                           else if((deltabit / 8) && (size_of_pointer % ((deltabit / 8)) == 0))
                           {
                              right_offset_var += PrintNode(mult->op1, v, vppf);
                              right_offset_var += " * ";
                              right_offset_var += STR(size_of_pointer / ((deltabit / 8)));
                              do_reverse_pointer_arithmetic = true;
                           }
                        }
                     }
                     exit = true;
                     break;
                  }
                  case negate_expr_K:
                  {
                     const auto ne = GetPointerS<const negate_expr>(right_op_node);
                     right_offset_var += "-";
                     right_op_node = ne->op;
                     INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                    "New op node is " + STR(ne->op->index) + " - " + right_op_node->get_kind_text());
                     break;
                  }
                  case paren_expr_K:
                  case nop_expr_K:
                  {
                     const auto ne = GetPointerS<const nop_expr>(right_op_node);
                     right_op_node = ne->op;
                     INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                    "New op node is " + STR(ne->op->index) + " - " + right_op_node->get_kind_text());
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
                  case target_expr_K:
                  case target_mem_ref_K:
                  case target_mem_ref461_K:
                  case tree_list_K:
                  case tree_vec_K:
                  case addr_expr_K:
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
                  case non_lvalue_expr_K:
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
                  case abs_expr_K:
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
                  case mem_ref_K:
                  case min_expr_K:
                  case minus_expr_K:
                  case modify_expr_K:
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
                  {
                     exit = true;
                     break;
                  }
               }
            }
            if(deltabit == 8)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Reversing pointer arithmetic successful");
               do_reverse_pointer_arithmetic = true;
               right_offset_var = PrintNode(ppe->op1, v, vppf);
            }
         }
         bool char_pointer = false;
         if(!do_reverse_pointer_arithmetic && tree_helper::Size(GetPointerS<const pointer_type>(type_node)->ptd) == 8)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "Reversing pointer arithmetic successful because of char pointer");
            do_reverse_pointer_arithmetic = true;
            char_pointer = true;
         }
         if(binary_op_cast && !do_reverse_pointer_arithmetic)
         {
            res += "(" + tree_helper::PrintType(TM, ppe->type) + ")(";
         }
         if((left_op_cast && tree_helper::IsVoidType(pointed_type)) || !do_reverse_pointer_arithmetic)
         {
            res += "((unsigned char*)";
         }
         res += PrintNode(ppe->op0, v, vppf);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "After printing of left part " + res);
         if((left_op_cast && tree_helper::IsVoidType(pointed_type)) || !do_reverse_pointer_arithmetic)
         {
            res += ")";
         }
         res += std::string(" ") + op + " ";
         /*
                  if (!do_reverse_pointer_arithmetic)
                  {
                     TM->increment_unremoved_pointer_plus();
                  }
                  else
                  {
                     TM->increment_removable_pointer_plus();
                  }
         */
         if(do_reverse_pointer_arithmetic && !char_pointer)
         {
            if(right_offset_var != "")
            {
               res += right_offset_var;
            }
            else
            {
               res += STR(pointer_offset);
               const auto type = GetPointerS<const integer_cst>(right_op_node)->type;
               const auto it = GetPointer<const integer_type>(type);
               bool unsigned_flag = (it && it->unsigned_flag) || type->get_kind() == pointer_type_K ||
                                    type->get_kind() == boolean_type_K || type->get_kind() == enumeral_type_K;
               if(unsigned_flag)
               {
                  res += "u";
               }
            }
         }
         else
         {
            if(right_op_node->get_kind() == integer_cst_K)
            {
               res += STR(tree_helper::GetConstValue(right_op_node));
            }
            else
            {
               res += PrintNode(ppe->op1, v, vppf);
            }
         }
         if(binary_op_cast && !do_reverse_pointer_arithmetic)
         {
            res += ")";
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
         break;
      }
      /* Binary relational expressions.  */
      case lt_expr_K:
      case le_expr_K:
      case gt_expr_K:
      case ge_expr_K:
      case eq_expr_K:
      case ne_expr_K:
      {
         const auto binary_op_cast = tree_helper::IsPointerType(node);
         if(binary_op_cast)
         {
            res += "((" + tree_helper::PrintType(TM, tree_helper::CGetType(node)) + ")(";
         }
         const auto op = tree_helper::op_symbol(node);
         const auto be = GetPointerS<const binary_expr>(node);
         const auto& left_op = be->op0;
         const auto& right_op = be->op1;
         const auto left_op_type = tree_helper::CGetType(be->op0);
         const auto right_op_type = tree_helper::CGetType(be->op1);

         bool vector = tree_helper::IsVectorType(be->type) && tree_helper::IsVectorType(right_op_type) &&
                       right_op_type->index != be->type->index;
         if(vector)
         {
            const auto element_type = tree_helper::CGetElements(be->type);
            const auto element_size = tree_helper::SizeAlloc(element_type);
            const auto size = tree_helper::SizeAlloc(be->type);
            const auto vector_size = size / element_size;
            res += "(" + tree_helper::PrintType(TM, be->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(left_op, v, vppf) + ")[" + STR(ind) + "] " + op + " (" +
                      PrintNode(right_op, v, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
            break;
         }
         else
         {
            const auto left_op_cast = tree_helper::IsPointerType(left_op) && tree_helper::IsPointerType(right_op) &&
                                      (tree_helper::Size(left_op) != tree_helper::Size(right_op));
            const auto right_op_cast = tree_helper::IsPointerType(right_op) && tree_helper::IsPointerType(left_op) &&
                                       (tree_helper::Size(left_op) != tree_helper::Size(right_op));
            const auto left_op_bracket = !(GetPointer<decl_node>(be->op0) || GetPointer<ssa_name>(be->op0));
            const auto right_op_bracket = !(GetPointer<decl_node>(be->op1) || GetPointer<ssa_name>(be->op1));

            if(left_op_bracket)
            {
               res += "(";
            }
            if(left_op_cast)
            {
               res += "((unsigned long int)";
            }

            if(tree_helper::IsVectorType(be->type) && tree_helper::IsVectorType(left_op_type) &&
               left_op_type->index != be->type->index)
            {
               res += "(" + tree_helper::PrintType(TM, be->type) + ")(";
            }

            res += PrintNode(left_op, v, vppf);

            if(tree_helper::IsVectorType(be->type) && tree_helper::IsVectorType(left_op_type) &&
               left_op_type->index != be->type->index)
            {
               res += ")";
            }

            if(left_op_cast)
            {
               res += ")";
            }
            if(left_op_bracket)
            {
               res += ")";
            }
            res += std::string(" ") + op + " ";
            if(right_op_bracket)
            {
               res += "(";
            }
            if(right_op_cast)
            {
               res += "((unsigned long int)";
            }

            res += PrintNode(right_op, v, vppf);

            if(right_op_cast)
            {
               res += ")";
            }
            if(right_op_bracket)
            {
               res += ")";
            }
            if(binary_op_cast)
            {
               res += "))";
            }
         }
         break;
      }
      case rrotate_expr_K:
      case lrotate_expr_K:
      {
         const auto binary_op_cast = tree_helper::IsPointerType(node);
         const auto type = tree_helper::CGetType(node);
         if(binary_op_cast)
         {
            res += "((" + tree_helper::PrintType(TM, type) + ")(";
         }
         const auto be = GetPointerS<const binary_expr>(node);
         const auto& left_op = be->op0;
         const auto& right_op = be->op1;
         bool left_op_cast = tree_helper::IsPointerType(left_op);
         bool right_op_cast = tree_helper::IsPointerType(right_op);
         if(left_op_cast)
         {
            res += "((unsigned long int)";
         }
         res += PrintNode(left_op, v, vppf);
         if(left_op_cast)
         {
            res += ")";
         }
         if(node->get_kind() == rrotate_expr_K)
         {
            res += " >> ";
         }
         else
         {
            res += " << ";
         }
         if(right_op_cast)
         {
            res += "((unsigned long int)";
         }
         res += PrintNode(right_op, v, vppf);
         if(right_op_cast)
         {
            res += ")";
         }
         res += " | ";
         if(left_op_cast)
         {
            res += "((unsigned long int)";
         }
         res += PrintNode(left_op, v, vppf);
         if(left_op_cast)
         {
            res += ")";
         }
         if(node->get_kind() == rrotate_expr_K)
         {
            res += " << ";
         }
         else
         {
            res += " >> ";
         }
         res += "(" + STR(GetPointerS<const integer_type>(type)->prec) + "-";
         if(right_op_cast)
         {
            res += "((unsigned long int)";
         }
         res += PrintNode(right_op, v, vppf);
         if(right_op_cast)
         {
            res += ")";
         }
         res += ")";
         if(binary_op_cast)
         {
            res += "))";
         }
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointerS<const lut_expr>(node);
         std::string concat_shift_string;
         if(le->op8)
         {
            THROW_ERROR("not supported");
         }
         if(le->op7)
         {
            THROW_ERROR("not supported");
         }
         if(le->op6)
         {
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op6, v, vppf) + ")<<5) | ";
         }
         if(le->op5)
         {
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op5, v, vppf) + ")<<4) | ";
         }
         if(le->op4)
         {
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op4, v, vppf) + ")<<3) | ";
         }
         if(le->op3)
         {
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op3, v, vppf) + ")<<2) | ";
         }
         if(le->op2)
         {
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op2, v, vppf) + ")<<1) | ";
         }
         concat_shift_string = concat_shift_string + "(" + PrintNode(le->op1, v, vppf) + ")";
         res = res + "(" + PrintNode(le->op0->index, v, vppf) + ">>(" + concat_shift_string + "))&1";
         break;
      }
      case negate_expr_K:
      case bit_not_expr_K:
      case reference_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      {
         const auto ue = GetPointerS<const unary_expr>(node);
         const auto op = tree_helper::op_symbol(node);
         res = res + " " + op + "(" + PrintNode(ue->op, v, vppf) + ")";
         break;
      }
      case truth_not_expr_K:
      {
         const auto te = GetPointerS<const truth_not_expr>(node);
         if(tree_helper::IsVectorType(te->type))
         {
            const auto element_type = tree_helper::CGetElements(te->type);
            const auto element_size = tree_helper::SizeAlloc(element_type);
            const auto size = tree_helper::SizeAlloc(te->type);
            const auto vector_size = size / element_size;
            res += "(" + tree_helper::PrintType(TM, te->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += " !(" + PrintNode(te->op, v, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
         }
         else
         {
            const auto op = tree_helper::op_symbol(node);
            res = res + " " + op + "(" + PrintNode(te->op, v, vppf) + ")";
            break;
         }
         break;
      }
      case realpart_expr_K:
      case imagpart_expr_K:
      {
         const auto op = tree_helper::op_symbol(node);
         const auto ue = GetPointerS<const unary_expr>(node);
         res = res + " " + op + PrintNode(ue->op, v, vppf);
         const auto sa = GetPointer<const ssa_name>(ue->op);
         if(sa && (sa->volatile_flag || (sa->CGetDefStmt()->get_kind() == gimple_nop_K)) &&
            (sa->var && GetPointer<const var_decl>(sa->var)))
         {
            res += " = 0";
         }
         break;
      }
      case addr_expr_K:
      {
         const auto ue = GetPointerS<const addr_expr>(node);
         if(GetPointer<const component_ref>(ue->op) &&
            has_bit_field(GetPointerS<const component_ref>(ue->op)->op1->index))
         {
            THROW_ERROR_CODE(BITFIELD_EC, "Trying to get the address of a bitfield");
         }
         if(ue->op->get_kind() == array_ref_K)
         {
            const auto ar = GetPointerS<const array_ref>(ue->op);
            ///&string[0]
            if(ar->op0->get_kind() == string_cst_K && ar->op1->get_kind() == integer_cst_K &&
               tree_helper::GetConstValue(ar->op1) == 0)
            {
               res += PrintNode(ar->op0, v, vppf);
               break;
            }
         }
         ///&array is printed back as array
         if(ue->op->get_kind() == var_decl_K && (tree_helper::CGetType(ue->op)->get_kind() == array_type_K))
         {
            res += PrintNode(ue->op, v, vppf);
            break;
         }
         if(ue->op->get_kind() == label_decl_K)
         {
            res += "&&" + PrintNode(ue->op, v, vppf);
            break;
         }
         res += "(" + tree_helper::op_symbol(node) + "(" + PrintNode(ue->op, v, vppf) + "))";
         break;
      }
      case function_decl_K:
      {
         const auto fd = GetPointerS<const function_decl>(node);
         res += tree_helper::print_function_name(TM, fd);
         break;
      }
      case fix_trunc_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case float_expr_K:
      case convert_expr_K:
      case nop_expr_K:
      {
         const auto ue = GetPointerS<const unary_expr>(node);
         const auto type = tree_helper::CGetType(node);
         unsigned int prec = 0;
         unsigned int algn = 0;
         if(type && type->get_kind() == integer_type_K)
         {
            prec = GetPointerS<const integer_type>(type)->prec;
            algn = GetPointerS<const integer_type>(type)->algn;
         }

         const auto operand_type = tree_helper::CGetType(ue->op);
         unsigned int operand_prec = 0;
         unsigned int operand_algn = 0;
         if(operand_type && operand_type->get_kind() == integer_type_K)
         {
            operand_prec = GetPointerS<const integer_type>(operand_type)->prec;
            operand_algn = GetPointerS<const integer_type>(operand_type)->algn;
         }

         std::string operand_res = PrintNode(ue->op, v, vppf);
         if(operand_prec != operand_algn && operand_prec % operand_algn &&
            tree_helper::IsSignedIntegerType(operand_type))
         {
            operand_res = "((union {" + tree_helper::PrintType(TM, operand_type) + " orig; " +
                          tree_helper::PrintType(TM, operand_type) + " bitfield : " + STR(operand_prec) + ";}){" +
                          operand_res + "}).bitfield";
         }
         if(type && (type->get_kind() == boolean_type_K))
         {
            res += "(";
            res += operand_res;
            res += ")&1";
         }
         // bitfield type
         else if(prec != algn && prec % algn)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Bitfield");
            bool ui = tree_helper::IsUnsignedIntegerType(operand_type) && tree_helper::IsSignedIntegerType(type);
            if(ui)
            {
               res += "((" + tree_helper::PrintType(TM, type) + ")(";
            }
            res += "(";
            res += operand_res;
            res += ")%(1";
            if(prec > 32)
            {
               res += "LL";
            }
            if(GetPointerS<const integer_type>(type)->unsigned_flag)
            {
               res += "U";
            }
            res += " << " + STR(prec) + ")";
            if(ui)
            {
               res += std::string(" << ") + STR(algn - prec) + ")) >> " + STR(algn - prec);
            }
         }
         else
         {
            res = res + "(" + tree_helper::PrintType(TM, ue->type) + ") (";
            res += operand_res;
            res += ")";
         }
         break;
      }
      case view_convert_expr_K:
      {
         const auto vce = GetPointerS<const view_convert_expr>(node);
         if(GetPointer<const integer_cst>(vce->op))
         {
            res = res + "__panda_union.dest;}";
         }
         else
         {
            if(tree_helper::IsPointerType(vce->type))
            {
               res = res + "((" + tree_helper::PrintType(TM, vce->type) + ") (" + PrintNode(vce->op, v, vppf) + "))";
            }
            else
            {
               res =
                   res + "*((" + tree_helper::PrintType(TM, vce->type) + " * ) &(" + PrintNode(vce->op, v, vppf) + "))";
            }
         }
         break;
      }
      case component_ref_K:
      {
         const auto cr = GetPointerS<const component_ref>(node);
         res = "(" + PrintNode(cr->op0, v, vppf) + ")." + PrintNode(cr->op1, v, vppf);
         break;
      }
      case indirect_ref_K:
      {
         const auto ir = GetPointerS<const indirect_ref>(node);
         res = "*(";
         if(GetPointer<const integer_cst>(ir->op))
         {
            res += "(" + tree_helper::PrintType(TM, tree_helper::CGetType(ir->op)) + ")";
         }
         res += PrintNode(ir->op, v, vppf);
         res += ")";
         break;
      }
      case misaligned_indirect_ref_K:
      {
         const auto mir = GetPointerS<const misaligned_indirect_ref>(node);
         res = "*(" + PrintNode(mir->op, v, vppf) + ")";
         break;
      }
      case mem_ref_K:
      {
         const auto mr = GetPointerS<const mem_ref>(node);
         const auto offset = tree_helper::GetConstValue(mr->op1);
         const tree_manipulationRef tm(new tree_manipulation(AppM->get_tree_manager(), Param, AppM));
         const auto pointer_type = tm->GetPointerType(mr->type, 8);
         const std::string type_string = tree_helper::PrintType(TM, pointer_type);
         if(offset == 0)
         {
            res = "(*((" + type_string + ")(" + PrintNode(mr->op0, v, vppf) + ")))";
         }
         else
         {
            res = "(*((" + type_string + ")(((unsigned char*)" + PrintNode(mr->op0, v, vppf) + ") + " + STR(offset) +
                  ")))";
         }
         break;
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref>(node);
         bool need_plus = false;
         res = "(*((" + tree_helper::PrintType(TM, tmr->type) + "*)(";
         if(tmr->symbol)
         {
            res += "((unsigned char*)" +
                   (tree_helper::IsStructType(tmr->symbol) || tree_helper::IsUnionType(tmr->symbol) ? std::string("&") :
                                                                                                      std::string("")) +
                   PrintNode(tmr->symbol, v, vppf) + ")";
            need_plus = true;
         }
         if(tmr->base)
         {
            if(need_plus)
            {
               res += "+";
            }
            else
            {
               need_plus = true;
            }
            if(tmr->symbol)
            {
               res += "(" + PrintNode(tmr->base, v, vppf) + ")";
            }
            else
            {
               res += "((unsigned char*)" + PrintNode(tmr->base, v, vppf) + ")";
            }
         }
         if(tmr->step)
         {
            if(need_plus)
            {
               res += "+";
            }
            need_plus = false;
            res += PrintNode(tmr->step, v, vppf) + "*";
            THROW_ASSERT(tmr->idx, "idx expected!");
         }
         if(tmr->idx)
         {
            if(need_plus)
            {
               res += "+";
            }
            else
            {
               need_plus = true;
            }
            res += PrintNode(tmr->idx, v, vppf);
         }
         if(tmr->offset)
         {
            if(need_plus)
            {
               res += "+";
            }
            res += PrintNode(tmr->offset, v, vppf);
         }
         res += ")))";
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref461>(node);
         bool need_plus = false;
         bool isFunctionPointer = tree_helper::IsFunctionPointerType(tmr->type);
         res = isFunctionPointer ? "(" : "(*((";
         res += tree_helper::PrintType(TM, tmr->type);
         res += isFunctionPointer ? ")(" : "*)(";
         if(tmr->base)
         {
            need_plus = true;
            res += "((unsigned char*)" +
                   (tree_helper::IsStructType(tmr->base) || tree_helper::IsUnionType(tmr->base) ? std::string("&") :
                                                                                                  std::string("")) +
                   PrintNode(tmr->base, v, vppf) + ")";
         }
         if(tmr->step)
         {
            if(need_plus)
            {
               res += "+";
            }
            need_plus = false;
            res += PrintNode(tmr->step, v, vppf) + "*";
            THROW_ASSERT(tmr->idx, "idx expected!");
         }
         if(tmr->idx)
         {
            if(need_plus)
            {
               res += "+";
            }
            else
            {
               need_plus = true;
            }
            res += PrintNode(tmr->idx, v, vppf);
         }
         if(tmr->idx2)
         {
            if(need_plus)
            {
               res += "+";
            }
            else
            {
               need_plus = true;
            }
            res += PrintNode(tmr->idx2, v, vppf);
         }
         if(tmr->offset)
         {
            if(need_plus)
            {
               res += "+";
            }
            res += PrintNode(tmr->offset, v, vppf);
         }
         res += isFunctionPointer ? ")" : ")))";
         break;
      }
      case array_ref_K:
      {
         const auto ar = GetPointerS<const array_ref>(node);
         const auto base = ar->op0;
         // tree_nodeRef offset = ar->op1;
         if(base->get_kind() == mem_ref_K)
         {
            const auto mr = GetPointerS<const mem_ref>(base);
            const auto offset = tree_helper::GetConstValue(mr->op1);
            std::string type_string = tree_helper::PrintType(TM, mr->type);
            if(mr->type->get_kind() == array_type_K)
            {
               size_t found_square_bracket = type_string.find('[');
               if(found_square_bracket != std::string::npos)
               {
                  type_string.insert(found_square_bracket, "(*)");
               }
               else
               {
                  type_string = type_string + "*";
               }
            }
            else
            {
               type_string = type_string + "*";
            }
            if(offset == 0)
            {
               res = "(*((" + type_string + ")(" + PrintNode(mr->op0, v, vppf) + ")))";
            }
            else
            {
               res = "(*((" + type_string + ")(((unsigned char*)" + PrintNode(mr->op0, v, vppf) + ") + " + STR(offset) +
                     ")))";
            }
         }
         else
         {
            res = "(" + PrintNode(ar->op0, v, vppf) + ")";
         }
         res += "[" + PrintNode(ar->op1, v, vppf) + "]";
         break;
      }
      case bit_field_ref_K:
      {
         const auto bf = GetPointerS<const bit_field_ref>(node);
         const auto bpos = tree_helper::GetConstValue(bf->op2);
         res += "*((" + tree_helper::PrintType(TM, bf->type) + "* ) (((unsigned long int) &(" +
                PrintNode(bf->op0, v, vppf) + ")) + (unsigned long int)" + STR(bpos / 8) + "))";
         if(bpos % 8)
         {
            res += " >> " + STR(bpos % 8);
         }
         break;
      }
      /* post/pre increment and decrement operator should be treated as unary even if they are binary_expr*/
      case postdecrement_expr_K:
      case postincrement_expr_K:
      {
         const auto op = tree_helper::op_symbol(node);
         const auto be = GetPointerS<const binary_expr>(node);
         res += PrintNode(be->op0, v, vppf) + op + PrintNode(be->op1, v, vppf);
         break;
      }
      case min_expr_K:
      {
         const auto me = GetPointerS<const min_expr>(node);
         if(tree_helper::IsVectorType(me->type))
         {
            const auto element_type = tree_helper::CGetElements(me->type);
            const auto element_size = tree_helper::SizeAlloc(element_type);
            const auto size = tree_helper::SizeAlloc(me->type);
            const auto vector_size = size / element_size;
            res += "/*" + me->get_kind_text() + "*/";
            res += "(" + tree_helper::PrintType(TM, me->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(me->op0, v, vppf) + ")[" + STR(ind) + "] < (" + PrintNode(me->op1, v, vppf) +
                      ")[" + STR(ind) + "] ? " + "(" + PrintNode(me->op0, v, vppf) + ")[" + STR(ind) + "]" + " : " +
                      "(" + PrintNode(me->op1, v, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
         }
         else
         {
            std::string op_0 = PrintNode(me->op0, v, vppf), op_1 = PrintNode(me->op1, v, vppf);
            res += op_0 + " < " + op_1 + " ? " + op_0 + " : " + op_1;
         }
         break;
      }
      case max_expr_K:
      {
         const auto me = GetPointerS<const max_expr>(node);
         if(tree_helper::IsVectorType(me->type))
         {
            const auto element_type = tree_helper::CGetElements(me->type);
            const auto element_size = tree_helper::SizeAlloc(element_type);
            const auto size = tree_helper::SizeAlloc(me->type);
            const auto vector_size = size / element_size;
            res += "/*" + me->get_kind_text() + "*/";
            res += "(" + tree_helper::PrintType(TM, me->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(me->op0, v, vppf) + ")[" + STR(ind) + "] > (" + PrintNode(me->op1, v, vppf) +
                      ")[" + STR(ind) + "] ? " + "(" + PrintNode(me->op0, v, vppf) + ")[" + STR(ind) + "]" + " : " +
                      "(" + PrintNode(me->op1, v, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
         }
         else
         {
            std::string op_0 = PrintNode(me->op0, v, vppf), op_1 = PrintNode(me->op1, v, vppf);
            res += op_0 + " > " + op_1 + " ? " + op_0 + " : " + op_1;
         }
         break;
      }
      case unordered_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string op_0 = PrintNode(be->op0, v, vppf), op_1 = PrintNode(be->op1, v, vppf);
         res += "__builtin_isunordered(" + op_0 + "," + op_1 + ")";
         break;
      }
      case ordered_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string op_0 = PrintNode(be->op0, v, vppf), op_1 = PrintNode(be->op1, v, vppf);
         res += "!__builtin_isunordered(" + op_0 + "," + op_1 + ")";
         break;
      }
      case unlt_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string op_0 = PrintNode(be->op0, v, vppf), op_1 = PrintNode(be->op1, v, vppf);
         res += "!__builtin_isgreaterequal(" + op_0 + "," + op_1 + ")";
         break;
      }
      case unle_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string op_0 = PrintNode(be->op0, v, vppf), op_1 = PrintNode(be->op1, v, vppf);
         res += "!__builtin_isgreater(" + op_0 + "," + op_1 + ")";
         break;
      }
      case ungt_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string op_0 = PrintNode(be->op0, v, vppf), op_1 = PrintNode(be->op1, v, vppf);
         res += "!__builtin_islessequal(" + op_0 + "," + op_1 + ")";
         break;
      }
      case unge_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string op_0 = PrintNode(be->op0, v, vppf), op_1 = PrintNode(be->op1, v, vppf);
         res += "!__builtin_isless(" + op_0 + "," + op_1 + ")";
         break;
      }
      case uneq_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string op_0 = PrintNode(be->op0, v, vppf), op_1 = PrintNode(be->op1, v, vppf);
         res += "!__builtin_islessgreater(" + op_0 + "," + op_1 + ")";
         break;
      }
      case ltgt_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         std::string op_0 = PrintNode(be->op0, v, vppf), op_1 = PrintNode(be->op1, v, vppf);
         res += "__builtin_islessgreater(" + op_0 + "," + op_1 + ")";
         break;
      }
      case abs_expr_K:
      {
         const auto ae = GetPointerS<const abs_expr>(node);
         std::string op_0 = PrintNode(ae->op, v, vppf);
         if(GetPointer<const real_type>(ae->type))
         {
            const auto rt = GetPointerS<const real_type>(ae->type);
            if(rt->prec == 80)
            {
               res += "__builtin_fabsl(" + op_0 + ")";
            }
            else if(rt->prec == 64)
            {
               res += "__builtin_fabs(" + op_0 + ")";
            }
            else if(rt->prec == 32)
            {
               res += "__builtin_fabsf(" + op_0 + ")";
            }
            else
            {
               THROW_ERROR("Abs on a real number with not supported precision");
            }
         }
         else
         {
            res += "(" + op_0 + ") >= 0 ? (" + op_0 + ") : -(" + op_0 + ")";
         }
         //            res += "__builtin_llabs(" + op_0 + ")";
         break;
      }
      case complex_expr_K:
      {
         const auto ce = GetPointerS<const complex_expr>(node);
         std::string op_0 = PrintNode(ce->op0, v, vppf), op_1 = PrintNode(ce->op1, v, vppf);
         res += op_0 + "+ 1i*" + op_1;
         break;
      }
      case cond_expr_K:
      {
         const auto ce = GetPointerS<const cond_expr>(node);
         res = PrintNode(ce->op0, v, vppf) + " ? " + PrintNode(ce->op1, v, vppf) + " : " + PrintNode(ce->op2, v, vppf);
         break;
      }
      case ternary_plus_expr_K:
      {
         const auto te = GetPointerS<const ternary_expr>(node);
         res = PrintNode(te->op0, v, vppf) + " + " + PrintNode(te->op1, v, vppf) + " + " + PrintNode(te->op2, v, vppf);
         break;
      }
      case ternary_pm_expr_K:
      {
         const auto te = GetPointerS<const ternary_expr>(node);
         res = PrintNode(te->op0, v, vppf) + " + " + PrintNode(te->op1, v, vppf) + " - " + PrintNode(te->op2, v, vppf);
         break;
      }
      case ternary_mp_expr_K:
      {
         const auto te = GetPointerS<const ternary_expr>(node);
         res = PrintNode(te->op0, v, vppf) + " - " + PrintNode(te->op1, v, vppf) + " + " + PrintNode(te->op2, v, vppf);
         break;
      }
      case ternary_mm_expr_K:
      {
         const auto te = GetPointerS<const ternary_expr>(node);
         res = PrintNode(te->op0, v, vppf) + " - " + PrintNode(te->op1, v, vppf) + " - " + PrintNode(te->op2, v, vppf);
         break;
      }
      case fshl_expr_K:
      case fshr_expr_K:
      {
         const auto te = GetPointerS<const ternary_expr>(node);
         const auto type_size = tree_helper::Size(te->type);
         const auto left_op_cast = tree_helper::IsPointerType(te->op0);
         const auto right_op_cast = tree_helper::IsPointerType(te->op1);
         res += "(";
         if(left_op_cast)
         {
            res += "((unsigned long int)";
         }
         res += PrintNode(te->op0, v, vppf);
         if(left_op_cast)
         {
            res += ")";
         }
         res += " << (";
         if(node->get_kind() == fshl_expr_K)
         {
            res += PrintNode(te->op2, v, vppf);
            res += " % ";
            res += STR(type_size);
         }
         else
         {
            res += STR(type_size);
            res += " - (";
            res += PrintNode(te->op2, v, vppf);
            res += " % ";
            res += STR(type_size);
            res += ")";
         }
         res += ")) | (";

         if(right_op_cast)
         {
            res += "((unsigned long int)";
         }
         res += PrintNode(te->op1, v, vppf);
         if(right_op_cast)
         {
            res += ")";
         }
         res += " >> (";
         if(node->get_kind() == fshr_expr_K)
         {
            res += PrintNode(te->op2, v, vppf);
            res += " % ";
            res += STR(type_size);
         }
         else
         {
            res += STR(type_size);
            res += " - (";
            res += PrintNode(te->op2, v, vppf);
            res += " % ";
            res += STR(type_size);
            res += ")";
         }
         res += "))";
         break;
      }
      case bit_ior_concat_expr_K:
      {
         const auto te = GetPointerS<const ternary_expr>(node);
         res = PrintNode(te->op0, v, vppf) + " | (" + PrintNode(te->op1, v, vppf) + " & ((1ULL<<" +
               PrintNode(te->op2, v, vppf) + ")-1))";
         break;
      }
      case vec_cond_expr_K:
      {
         auto vce = GetPointerS<const vec_cond_expr>(node);
         const auto element_type = tree_helper::CGetElements(vce->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vce->type);
         const auto vector_size = size / element_size;
         res += "/*" + vce->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vce->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += "(" + PrintNode(vce->op0, v, vppf) + ")" +
                   (tree_helper::IsVectorType(vce->op0) ? "[" + STR(ind) + "]" : "") + " ? " + "(" +
                   PrintNode(vce->op1, v, vppf) + ")[" + STR(ind) + "]" + " : " + "(" + PrintNode(vce->op2, v, vppf) +
                   ")[" + STR(ind) + "]";
            if(ind != vector_size - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case vec_perm_expr_K:
      {
         const auto vpe = GetPointerS<const vec_perm_expr>(node);
         const auto element_type = tree_helper::CGetElements(vpe->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vpe->op0);
         const auto vector_size = size / element_size;
         res += "/*" + vpe->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vpe->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += "((((" + PrintNode(vpe->op2, v, vppf) + ")[" + STR(ind) + "])%" + STR(2 * vector_size) + ") < " +
                   STR(vector_size) + ") ? (" + PrintNode(vpe->op0, v, vppf) + ")[(((" + PrintNode(vpe->op2, v, vppf) +
                   ")[" + STR(ind) + "])%" + STR(2 * vector_size) + ")] : (" + PrintNode(vpe->op1, v, vppf) + ")[(((" +
                   PrintNode(vpe->op2, v, vppf) + ")[" + STR(ind) + "])%" + STR(2 * vector_size) + ")-" +
                   STR(vector_size) + "]";
            if(ind != vector_size - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<const gimple_cond>(node);
         for(const auto& pragma : gc->pragmas)
         {
            res += PrintNode(pragma, v, vppf) + "\n";
         }
         res = "if (" + PrintNode(gc->op0, v, vppf) + ")";
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointerS<const gimple_multi_way_if>(node);
         res = "if (";
         bool first = true;
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(first)
            {
               THROW_ASSERT(cond.first, "First condition of multi way if " + STR(node->index) + " is empty");
               res += PrintNode(cond.first, v, vppf);
               first = false;
            }
            else if(cond.first)
            {
               res += " /* else if(" + PrintNode(cond.first, v, vppf) + ")*/";
            }
         }
         res += ")";
         break;
      }
      case gimple_while_K:
      {
         const auto we = GetPointerS<const gimple_while>(node);
         res = "while (" + PrintNode(we->op0, v, vppf) + ")";
         break;
      }
      case gimple_for_K:
      {
         const auto fe = GetPointerS<const gimple_for>(node);
#if !RELEASE
         if(fe->omp_for)
         {
            res = "//#pragma omp " + PrintNode(fe->omp_for, v, vppf) + "\n";
         }
#endif
         res += "for (" + PrintNode(fe->op1, v, vppf) + "; ";
         res += PrintNode(fe->op0, v, vppf) + "; ";
         res += PrintNode(fe->op2, v, vppf);
         res += ")";
         break;
      }
      case gimple_switch_K:
      {
         const auto se = GetPointerS<const gimple_switch>(node);
         res += "switch(" + PrintNode(se->op0, v, vppf) + ")";
         break;
      }
      case gimple_assign_K:
      {
         const auto ms = GetPointerS<const gimple_assign>(node);
         if(!ms->init_assignment && !ms->clobber)
         {
            for(const auto& pragma : ms->pragmas)
            {
               res += PrintNode(pragma, v, vppf) + "\n";
            }
            res = "";
            if(tree_helper::IsArrayType(ms->op0))
            {
               const auto size = tree_helper::SizeAlloc(ms->op0);
               res += "__builtin_memcpy(";
               if(GetPointer<const mem_ref>(ms->op0))
               {
                  res += "&";
               }
               res += PrintNode(ms->op0, v, vppf) + ", ";
               if(ms->op1->get_kind() == view_convert_expr_K)
               {
                  const auto vce = GetPointerS<const view_convert_expr>(ms->op1);
                  res += "&" + PrintNode(vce->op, v, vppf) + ", ";
               }
               else
               {
                  if(GetPointer<const mem_ref>(ms->op1))
                  {
                     res += "&";
                  }
                  res += PrintNode(ms->op1, v, vppf) + ", ";
               }
               res += STR(size / 8) + ")";
               break;
            }
            if((!Param->getOption<bool>(OPT_without_transformation)) &&
               ((tree_helper::IsStructType(ms->op0) || tree_helper::IsUnionType(ms->op0)) &&
                (tree_helper::IsStructType(ms->op1) || tree_helper::IsUnionType(ms->op1)) &&
                tree_helper::GetTypeName(tree_helper::GetRealType(tree_helper::CGetType(ms->op0))) !=
                    tree_helper::GetTypeName(tree_helper::GetRealType(tree_helper::CGetType(ms->op1)))))
            {
               THROW_ERROR_CODE(
                   C_EC,
                   "Implicit struct type definition not supported in gimple assignment " + STR(node->index) + " - " +
                       tree_helper::GetTypeName(tree_helper::GetRealType(tree_helper::CGetType(ms->op0))) + " vs. " +
                       tree_helper::GetTypeName(tree_helper::GetRealType(tree_helper::CGetType(ms->op1))));
            }
            res += PrintNode(ms->op0, v, vppf) + " = ";
            const auto right = ms->op1;
            /// check for type conversion
            switch(right->get_kind())
            {
               case constructor_K:
               {
                  res += "(" + tree_helper::PrintType(TM, tree_helper::CGetType(ms->op0)) + ") ";
                  res += PrintNode(ms->op1, v, vppf);
                  break;
               }
               case vector_cst_K:
               {
                  const auto vc = GetPointerS<const vector_cst>(right);
                  const auto type = tree_helper::CGetType(ms->op0);
                  if(type->index != vc->type->index)
                  {
                     res += "(" + tree_helper::PrintType(TM, type) + ") ";
                  }
                  res += PrintNode(ms->op1, v, vppf);
                  break;
               }
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case complex_cst_K:
               case integer_cst_K:
               case real_cst_K:
               case string_cst_K:
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
               {
                  const auto left_type = tree_helper::CGetType(ms->op0);
                  const auto right_type = tree_helper::CGetType(ms->op1);
                  if(tree_helper::IsVectorType(left_type) && left_type->index != right_type->index)
                  {
                     res += "(" + tree_helper::PrintType(TM, left_type) + ") ";
                  }
                  if((right->get_kind() == rshift_expr_K || right->get_kind() == lshift_expr_K) &&
                     tree_helper::IsPointerType(GetPointerS<const binary_expr>(right)->op0))
                  {
                     res += "(unsigned int)";
                  }
                  res += PrintNode(ms->op1, v, vppf);
                  break;
               }
               case void_cst_K:
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
            const auto vce = GetPointer<const view_convert_expr>(right);
            if(vce && vce->op->get_kind() == integer_cst_K)
            {
               const auto dest_type = tree_helper::CGetType(ms->op1);
               const auto source_type = tree_helper::CGetType(vce->op);
               res = "{union {" + tree_helper::PrintType(TM, dest_type) + " dest; " +
                     tree_helper::PrintType(TM, source_type) +
                     " source;} __panda_union; __panda_union.source = " + PrintNode(vce->op, v, vppf) + "; " + res;
            }
         }
         if(ms->predicate)
         {
            res = "if(" + PrintNode(ms->predicate, v, vppf) + ") " + res;
         }
         if(ms->op1->get_kind() == trunc_div_expr_K || ms->op1->get_kind() == trunc_mod_expr_K)
         {
            const auto tde = GetPointerS<const binary_expr>(ms->op1);
            res = "if(" + PrintNode(tde->op1, v, vppf) + " != 0) " + res;
         }
         break;
      }
      case gimple_nop_K:
      {
         res += "/*gimple_nop*/";
         break;
      }
      case init_expr_K:
      {
         const auto be = GetPointerS<const binary_expr>(node);
         res = PrintNode(be->op0, v, vppf) + " = ";
         const auto right = be->op1;
         /// check for type conversion
         switch(right->get_kind())
         {
            case fix_trunc_expr_K:
            case fix_ceil_expr_K:
            case fix_floor_expr_K:
            case fix_round_expr_K:
            case float_expr_K:
            case convert_expr_K:
            case view_convert_expr_K:
            case nop_expr_K:
            case paren_expr_K:
            {
               const auto ue = GetPointerS<const unary_expr>(right);
               res = res + "(" + tree_helper::PrintType(TM, tree_helper::CGetType(be->op0)) + ") ";
               res += PrintNode(ue->op, v, vppf);
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
            case abs_expr_K:
            case addr_expr_K:
            case alignof_expr_K:
            case arrow_expr_K:
            case bit_not_expr_K:
            case buffer_ref_K:
            case card_expr_K:
            case cleanup_point_expr_K:
            case conj_expr_K:
            case exit_expr_K:
            case imagpart_expr_K:
            case indirect_ref_K:
            case misaligned_indirect_ref_K:
            case loop_expr_K:
            case negate_expr_K:
            case non_lvalue_expr_K:
            case realpart_expr_K:
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
               res += PrintNode(be->op1, v, vppf);
         }
         break;
      }
      case gimple_return_K:
      {
         const auto re = GetPointerS<const gimple_return>(node);
         for(const auto& pragma : re->pragmas)
         {
            res += PrintNode(pragma, v, vppf) + "\n";
         }
         res += "return ";
         if(re->op != nullptr)
         {
            const auto return_node = re->op;
            /// check for type conversion
            switch(return_node->get_kind())
            {
               case fix_trunc_expr_K:
               case fix_ceil_expr_K:
               case fix_floor_expr_K:
               case fix_round_expr_K:
               case float_expr_K:
               case convert_expr_K:
               case nop_expr_K:
               {
                  const auto ue = GetPointerS<const unary_expr>(return_node);
                  res += "(" + tree_helper::PrintType(TM, tree_helper::CGetType(re->op)) + ") (";
                  res += PrintNode(ue->op, v, vppf);
                  res += ")";
                  break;
               }
               case constructor_K:
               {
                  res += "(" + tree_helper::PrintType(TM, tree_helper::CGetType(re->op)) + ") ";
                  res += PrintNode(re->op, v, vppf);
                  break;
               }
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case abs_expr_K:
               case addr_expr_K:
               case alignof_expr_K:
               case arrow_expr_K:
               case bit_not_expr_K:
               case buffer_ref_K:
               case card_expr_K:
               case cleanup_point_expr_K:
               case conj_expr_K:
               case exit_expr_K:
               case imagpart_expr_K:
               case indirect_ref_K:
               case misaligned_indirect_ref_K:
               case loop_expr_K:
               case negate_expr_K:
               case non_lvalue_expr_K:
               case realpart_expr_K:
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
               case view_convert_expr_K:
               case error_mark_K:
               case paren_expr_K:
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
               {
                  res += PrintNode(re->op, v, vppf);
                  break;
               }
            }
         }
         res += ";";
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<const call_expr>(node);
         const function_decl* fd = nullptr;
         const auto op0 = ce->fn;
         bool is_va_start_end = false;

         // sizeof workaround
         bool is_sizeof = false;
         auto op0_kind = op0->get_kind();

         switch(op0_kind)
         {
            case addr_expr_K:
            {
               const auto ue = GetPointerS<const unary_expr>(op0);
               const auto fn = ue->op;
               THROW_ASSERT(fn->get_kind() == function_decl_K,
                            "tree node not currently supported " + fn->get_kind_text());
               fd = GetPointerS<const function_decl>(fn);
               ///__builtin_va_start should be ad-hoc managed
               std::string fname = tree_helper::print_function_name(TM, fd);
               if(fname == "__builtin_va_start" || fname == "__builtin_va_end" || fname == "__builtin_va_copy")
               {
                  is_va_start_end = true;
               }
               if(fname == "__builtin_constant_p")
               {
                  THROW_ERROR_CODE(C_EC, "Not supported function " + fname);
               }
               if(fname == STR_CST_string_sizeof)
               {
                  is_sizeof = true;
                  res += "sizeof";
               }
               else
               {
                  res += fname;
               }
               break;
            }
            case ssa_name_K:
            {
               res += "(*" + PrintNode(ce->fn, v, vppf) + ")";
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
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case tree_list_K:
            case tree_vec_K:
            case CASE_TERNARY_EXPRESSION:
            {
               if(op0_kind == obj_type_ref_K)
               {
                  const auto fn = tree_helper::find_obj_type_ref_function(ce->fn);
                  THROW_ASSERT(fn->get_kind(), "tree node not currently supported " + fn->get_kind_text());
                  const auto local_fd = GetPointerS<const function_decl>(fn);
                  if(tree_helper::GetFunctionReturnType(fn))
                  {
                     res += PrintNode(fn, v, vppf) + " = ";
                  }
                  res += tree_helper::print_function_name(TM, local_fd);
               }
               else
               {
                  THROW_ERROR(std::string("tree node not currently supported ") + op0->get_kind_text());
               }
               break;
            }
            case target_expr_K:
            case error_mark_K:
            case paren_expr_K:
            case lut_expr_K:
            case CASE_BINARY_EXPRESSION:
            case CASE_CPP_NODES:
            case CASE_CST_NODES:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_GIMPLE_NODES:
            case CASE_NON_ADDR_UNARY_EXPRESSION:
            case CASE_PRAGMA_NODES:
            case CASE_QUATERNARY_EXPRESSION:
            case CASE_TYPE_NODES:
            default:
            {
               THROW_ERROR(std::string("tree node not currently supported ") + op0->get_kind_text());
            }
         }
         /// Print parameters.
         res += "(";
         if(is_va_start_end)
         {
            THROW_ASSERT(ce->args.size(), "va_start or va_end have to have arguments");
            const auto par1 = ce->args[0];
            // print the first removing the address
            if(GetPointer<const addr_expr>(par1))
            {
               res += PrintNode(GetPointerS<const addr_expr>(par1)->op, v, vppf);
            }
            else if(GetPointer<const var_decl>(par1) || GetPointer<const ssa_name>(par1))
            {
               res += "*(" + PrintNode(par1, v, vppf) + ")";
            }
            else
            {
               THROW_ERROR("expected an address or a variable " + STR(par1->index));
            }
            for(size_t arg_index = 1; arg_index < ce->args.size(); arg_index++)
            {
               res += ", ";
               res += PrintNode(ce->args[arg_index], v, vppf);
            }
         }
         else if(is_sizeof)
         {
            THROW_ASSERT(ce->args.size() == 1, "Wrong number of arguments: " + STR(ce->args.size()));
            std::string argument = PrintNode(ce->args[0], v, vppf);
            const auto arg1 = ce->args[0];
            THROW_ASSERT(GetPointer<const addr_expr>(arg1),
                         "Argument is not an addr_expr but a " + std::string(arg1->get_kind_text()));
            const auto ae = GetPointer<const addr_expr>(arg1);
            /// This pattern is for gcc 4.5
            if(GetPointer<const array_ref>(ae->op))
            {
               THROW_ASSERT(GetPointer<const array_ref>(ae->op),
                            "Argument is not an array ref but a " + std::string(ae->op->get_kind_text()));
#if HAVE_ASSERTS
               const auto ar = GetPointerS<const array_ref>(ae->op);
#endif
               THROW_ASSERT(GetPointer<const string_cst>(ar->op0),
                            "Argument is not a string cast but a " + std::string(ar->op0->get_kind_text()));
               std::string unquoted_argument = argument.substr(1, argument.size() - 2);
               res += unquoted_argument;
            }
            else
            /// This pattern is for gcc 4.6
            {
               THROW_ASSERT(GetPointer<const string_cst>(ae->op),
                            "Argument is not a string cast but a " + std::string(ae->op->get_kind_text()));
               std::string unquoted_argument = argument.substr(3, argument.size() - 6);
               res += unquoted_argument;
            }
         }
         else
         {
            if(ce->args.size())
            {
               const auto& actual_args = ce->args;
               std::vector<tree_nodeRef> formal_args;
               if(fd)
               {
                  formal_args = fd->list_of_args;
               }
               std::vector<tree_nodeRef>::const_iterator actual_arg, actual_arg_end = actual_args.end();
               std::vector<tree_nodeRef>::const_iterator formal_arg, formal_arg_end = formal_args.end();
               for(actual_arg = actual_args.begin(), formal_arg = formal_args.begin(); actual_arg != actual_arg_end;
                   ++actual_arg)
               {
                  if(formal_arg != formal_arg_end &&
                     (tree_helper::IsStructType(*actual_arg) || tree_helper::IsUnionType(*actual_arg)) &&
                     (tree_helper::IsStructType(*formal_arg) || tree_helper::IsUnionType(*formal_arg)) &&
                     (tree_helper::GetRealType(tree_helper::CGetType(*actual_arg))->index !=
                      tree_helper::GetRealType(tree_helper::CGetType(*formal_arg))->index))
                  {
                     THROW_ERROR_CODE(C_EC, "Implicit struct type definition not supported in gimple assignment " +
                                                STR(node->index));
                  }
                  if(actual_arg != actual_args.begin())
                  {
                     res += ", ";
                  }
                  res += PrintNode(*actual_arg, v, vppf);
                  if(formal_arg != formal_arg_end)
                  {
                     ++formal_arg;
                  }
               }
            }
         }
         res += ")";
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointerS<const gimple_call>(node);
         const function_decl* fd = nullptr;
         for(const auto& pragma : ce->pragmas)
         {
            res += PrintNode(pragma, v, vppf) + "\n";
         }
         const auto op0 = ce->fn;
         bool is_va_start_end = false;

         // sizeof workaround
         bool is_sizeof = false;
         auto op0_kind = op0->get_kind();

         switch(op0_kind)
         {
            case addr_expr_K:
            {
               const auto ue = GetPointerS<const unary_expr>(op0);
               const auto fn = ue->op;
               THROW_ASSERT(fn->get_kind() == function_decl_K,
                            "tree node not currently supported " + fn->get_kind_text());
               fd = GetPointerS<const function_decl>(fn);
               ///__builtin_va_start should be ad-hoc managed
               std::string fname = tree_helper::print_function_name(TM, fd);
               if(fname == "__builtin_va_start" || fname == "__builtin_va_end" || fname == "__builtin_va_copy")
               {
                  is_va_start_end = true;
               }
               if(fname == "__builtin_constant_p")
               {
                  THROW_ERROR_CODE(C_EC, "Not supported function " + fname);
               }
               if(fname == STR_CST_string_sizeof)
               {
                  is_sizeof = true;
                  res += "sizeof";
               }
               else
               {
                  res += fname;
               }
               break;
            }
            case ssa_name_K:
            {
               res += "(*" + PrintNode(ce->fn, v, vppf) + ")";
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
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case tree_list_K:
            case tree_vec_K:
            case CASE_TERNARY_EXPRESSION:
            {
               if(op0_kind == obj_type_ref_K)
               {
                  const auto fn = tree_helper::find_obj_type_ref_function(ce->fn);
                  THROW_ASSERT(fn->get_kind() == function_decl_K,
                               "tree node not currently supported " + fn->get_kind_text());
                  const auto local_fd = GetPointerS<const function_decl>(fn);
                  if(tree_helper::GetFunctionReturnType(fn))
                  {
                     res += PrintNode(fn, v, vppf) + " = ";
                  }
                  res += tree_helper::print_function_name(TM, local_fd);
               }
               else
               {
                  THROW_ERROR(std::string("tree node not currently supported ") + op0->get_kind_text());
               }
               break;
            }
            case target_expr_K:
            case error_mark_K:
            case paren_expr_K:
            case lut_expr_K:
            case CASE_BINARY_EXPRESSION:
            case CASE_CPP_NODES:
            case CASE_CST_NODES:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_GIMPLE_NODES:
            case CASE_NON_ADDR_UNARY_EXPRESSION:
            case CASE_PRAGMA_NODES:
            case CASE_QUATERNARY_EXPRESSION:
            case CASE_TYPE_NODES:
            default:
            {
               THROW_ERROR(std::string("tree node not currently supported ") + op0->get_kind_text());
            }
         }
         /// Print parameters.
         res += "(";
         if(is_va_start_end)
         {
            THROW_ASSERT(ce->args.size(), "va_start or va_end have to have arguments");
            const auto par1 = ce->args[0];
            // print the first removing the address
            if(GetPointer<const addr_expr>(par1))
            {
               res += PrintNode(GetPointerS<const addr_expr>(par1)->op, v, vppf);
            }
            else if(GetPointer<const var_decl>(par1) || GetPointer<const ssa_name>(par1))
            {
               res += "*(" + PrintNode(par1, v, vppf) + ")";
            }
            else
            {
               THROW_ERROR("expected an address or a variable " + STR(par1));
            }
            for(size_t arg_index = 1; arg_index < ce->args.size(); arg_index++)
            {
               res += ", ";
               res += PrintNode(ce->args[arg_index], v, vppf);
            }
         }
         else if(is_sizeof)
         {
            THROW_ASSERT(ce->args.size() == 1, "Wrong number of arguments: " + STR(ce->args.size()));
            const auto arg1 = ce->args[0];
#ifndef NDEBUG
            THROW_ASSERT(GetPointer<const addr_expr>(arg1),
                         "Argument is not an addr_expr but a " + std::string(arg1->get_kind_text()));
            const auto ae = GetPointerS<const addr_expr>(arg1);
            if(GetPointer<const array_ref>(ae->op))
            {
               THROW_ASSERT(GetPointer<const array_ref>(ae->op),
                            "Argument is not an array ref but a " + std::string(ae->op->get_kind_text()));
               const auto ar = GetPointer<const array_ref>(ae->op);
               THROW_ASSERT(GetPointer<const string_cst>(ar->op0),
                            "Argument is not a string cast but a " + std::string(ar->op0->get_kind_text()));
            }
            else
            {
               THROW_ASSERT(GetPointer<const string_cst>(ae->op),
                            "Argument is not a string cast but a " + std::string(ae->op->get_kind_text()));
            }
#endif
            const auto argument = PrintNode(ce->args[0], v, vppf);
            const auto unquoted_argument = argument.substr(1, argument.size() - 2);
            res += unquoted_argument;
         }
         else
         {
            if(ce->args.size())
            {
               const auto& actual_args = ce->args;
               std::vector<tree_nodeRef> formal_args;
               if(fd)
               {
                  formal_args = fd->list_of_args;
               }
               std::vector<tree_nodeRef>::const_iterator actual_arg, actual_arg_end = actual_args.end();
               std::vector<tree_nodeRef>::const_iterator formal_arg, formal_arg_end = formal_args.end();
               for(actual_arg = actual_args.begin(), formal_arg = formal_args.begin(); actual_arg != actual_arg_end;
                   ++actual_arg)
               {
                  if(formal_arg != formal_arg_end &&
                     (tree_helper::IsStructType(*actual_arg) || tree_helper::IsUnionType(*actual_arg)) &&
                     (tree_helper::IsStructType(*formal_arg) || tree_helper::IsUnionType(*formal_arg)) &&
                     (tree_helper::GetRealType(tree_helper::CGetType(*actual_arg))->index !=
                      tree_helper::GetRealType(tree_helper::CGetType(*formal_arg))->index))
                  {
                     THROW_ERROR_CODE(C_EC, "Implicit struct type definition not supported in gimple assignment " +
                                                STR(node->index));
                  }
                  if(actual_arg != actual_args.begin())
                  {
                     res += ", ";
                  }
                  res += PrintNode(*actual_arg, v, vppf);
                  if(formal_arg != formal_arg_end)
                  {
                     ++formal_arg;
                  }
               }
            }
         }
         res += ")";
         break;
      }
      case gimple_resx_K:
      {
         res += "__gimple_resx()";
         break;
      }
      case gimple_asm_K:
      {
         const auto ae = GetPointerS<const gimple_asm>(node);
         for(const auto& pragma : ae->pragmas)
         {
            res += PrintNode(pragma, v, vppf);
         }
         res += "asm";
         if(ae->volatile_flag)
         {
            res += " __volatile__ ";
         }
         res += "(\"" + ae->str + "\"";
         if(ae->out)
         {
            res += ":";
            auto tl = GetPointerS<const tree_list>(ae->out);
            std::string out_string;
            do
            {
               auto tl_purp = GetPointerS<const tree_list>(tl->purp);
               out_string = "";
               do
               {
                  out_string += PrintConstant(tl_purp->valu, vppf);
                  if(tl_purp->chan)
                  {
                     tl_purp = GetPointer<const tree_list>(tl_purp->chan);
                  }
                  else
                  {
                     tl_purp = nullptr;
                  }
               } while(tl_purp);
               if(out_string == "\"=\"")
               {
                  out_string = "\"=r\"";
               }
               res += out_string;
               res += "(" + PrintNode(tl->valu, v, vppf) + ")";
               if(tl->chan)
               {
                  res += ",";
                  tl = GetPointer<const tree_list>(tl->chan);
               }
               else
               {
                  tl = nullptr;
               }
            } while(tl);
         }
         else if(ae->in || ae->clob)
         {
            res += ":";
         }
         if(ae->in)
         {
            res += ":";
            auto tl = GetPointerS<const tree_list>(ae->in);
            std::string in_string;
            do
            {
               auto tl_purp = GetPointer<const tree_list>(tl->purp);
               in_string = "";
               do
               {
                  in_string += PrintConstant(tl_purp->valu, vppf);
                  if(tl_purp->chan)
                  {
                     tl_purp = GetPointer<const tree_list>(tl_purp->chan);
                  }
                  else
                  {
                     tl_purp = nullptr;
                  }
               } while(tl_purp);
               if(in_string == "\"\"")
               {
                  in_string = "\"r\"";
               }
               res += in_string;
               res += "(" + PrintNode(tl->valu, v, vppf) + ")";
               if(tl->chan)
               {
                  res += ",";
                  tl = GetPointer<const tree_list>(tl->chan);
               }
               else
               {
                  tl = nullptr;
               }
            } while(tl);
         }
         else if(ae->clob)
         {
            res += ":";
         }
         if(ae->clob)
         {
            res += ":";
            auto tl = GetPointerS<const tree_list>(ae->clob);
            do
            {
               res += PrintNode(tl->valu, v, vppf);
               if(tl->chan)
               {
                  res += " ";
                  tl = GetPointer<const tree_list>(tl->chan);
               }
               else
               {
                  tl = nullptr;
               }
            } while(tl);
         }
         res += ")";
         break;
      }
      case gimple_phi_K:
      {
         const auto pn = GetPointerS<const gimple_phi>(node);
         res += "/* " + PrintNode(pn->res, v, vppf) + " = gimple_phi(";
         for(const auto& def_edge : pn->CGetDefEdgesList())
         {
            if(def_edge != pn->CGetDefEdgesList().front())
            {
               res += ", ";
            }
            res += "<" + PrintNode(def_edge.first, v, vppf) + ", BB" + STR(def_edge.second) + ">";
         }
         res += ") */";
         break;
      }
      case ssa_name_K:
      {
         res += (*vppf)(node->index);
         // res += PrintNode(sn->var, v, vppf);
         /*if (!sn->volatile_flag || sn->CGetDefStmt()->get_kind() == gimple_nop_K)
          res += "_" + STR(sn->vers);*/
         break;
      }
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      {
         res = PrintConstant(node, vppf);
         break;
      }
      case var_decl_K:
      case result_decl_K:
      case parm_decl_K:
      {
         res = (*vppf)(node->index);
         break;
      }
      case field_decl_K:
      {
         res = PrintVariable(node->index);
         break;
      }
      case label_decl_K:
      {
         const auto ld = GetPointerS<const label_decl>(node);
         if(ld->name)
         {
            const auto id = GetPointerS<const identifier_node>(ld->name);
            res = id->strg;
         }
         else
         {
            res = "_unnamed_label_" + STR(node->index);
         }
         break;
      }
      case gimple_label_K:
      {
         const auto le = GetPointerS<const gimple_label>(node);
         if(!GetPointerS<const label_decl>(le->op)->artificial_flag)
         {
            res += PrintNode(le->op, v, vppf);
            res += ":";
         }
         break;
      }
      case gimple_goto_K:
      {
         const auto ge = GetPointerS<const gimple_goto>(node);
         const auto is_a_label = GetPointer<const label_decl>(ge->op) != nullptr;
         res += (is_a_label ? "goto " : "goto *") + PrintNode(ge->op, v, vppf);
         break;
      }
      case constructor_K:
      {
         res += PrintInit(node, vppf);
         break;
      }
      case with_size_expr_K:
      {
         const auto wse = GetPointerS<const with_size_expr>(node);
         res += PrintNode(wse->op0, v, vppf);
         break;
      }
      case gimple_predict_K:
      case null_node_K:
      {
         break;
      }
      case gimple_pragma_K:
      {
         const auto pn = GetPointerS<const gimple_pragma>(node);
         if(pn->is_block && !pn->is_opening)
         {
            res += "\n}";
         }
         else
         {
            res += "#pragma ";
            if(!pn->scope)
            {
               res += pn->line;
            }
            else
            {
               res += PrintNode(pn->scope, v, vppf);
               res += " ";
               res += PrintNode(pn->directive, v, vppf);
               if(pn->is_block)
               {
                  res += "\n{";
               }
            }
         }
         break;
      }
      case omp_pragma_K:
      {
         res += "omp";
         break;
      }
      case omp_atomic_pragma_K:
      {
         res += " atomic";
         break;
      }
      case omp_for_pragma_K:
      {
         const auto fp = GetPointerS<const omp_for_pragma>(node);
         res += "for ";
         /// now print clauses
         for(const auto& clause : fp->clauses)
         {
            res += " " + clause.first + "(" + clause.second + ")";
         }
         break;
      }
      case omp_parallel_pragma_K:
      {
         const auto pn = GetPointerS<const omp_parallel_pragma>(node);
         if(!pn->is_shortcut)
         {
            res += "parallel";
         }
         /// now print clauses
         for(const auto& clause : pn->clauses)
         {
            res += " " + clause.first + "(" + clause.second + ")";
         }
         break;
      }
      case omp_sections_pragma_K:
      {
         const auto pn = GetPointerS<const omp_sections_pragma>(node);
         if(!pn->is_shortcut)
         {
            res += "sections";
         }
         break;
      }
      case omp_parallel_sections_pragma_K:
      {
         const auto pn = GetPointerS<const omp_parallel_sections_pragma>(node);
         res += "parallel sections";
         res += PrintNode(pn->op0, v, vppf);
         res += " ";
         res += PrintNode(pn->op1, v, vppf);
         break;
      }
      case omp_section_pragma_K:
      {
         res += "section";
         break;
      }
      case omp_declare_simd_pragma_K:
      {
         const auto fp = GetPointerS<const omp_declare_simd_pragma>(node);
         res += "declare simd ";
         /// now print clauses
         for(const auto& clause : fp->clauses)
         {
            res += " " + clause.first + "(" + clause.second + ")";
         }
         break;
      }
      case omp_simd_pragma_K:
      {
         const auto fp = GetPointerS<const omp_simd_pragma>(node);
         res += "simd ";
         /// now print clauses
         for(const auto& clause : fp->clauses)
         {
            res += " " + clause.first + "(" + clause.second + ")";
         }
         break;
      }
      case omp_critical_pragma_K:
      {
         res += "critical";
         const auto ocp = GetPointerS<const omp_critical_pragma>(node);
         for(const auto& clause : ocp->clauses)
         {
            res += " " + clause.first + "(" + clause.second + ")";
         }
         break;
      }
      case omp_target_pragma_K:
      {
         res += "target";
         const auto otp = GetPointerS<const omp_target_pragma>(node);
         for(const auto& clause : otp->clauses)
         {
            res += " " + clause.first + "(" + clause.second + ")";
         }
         break;
      }
      case omp_task_pragma_K:
      {
         res += "task";
         const auto otp = GetPointerS<const omp_task_pragma>(node);
         for(const auto& clause : otp->clauses)
         {
            res += " " + clause.first + "(" + clause.second + ")";
         }
         break;
      }
#if HAVE_FROM_PRAGMA_BUILT
      case map_pragma_K:
      {
         res += STR_CST_pragma_keyword_map;
         break;
      }
      case call_hw_pragma_K:
      {
         res += STR_CST_pragma_keyword_call_hw " ";
         const auto ch = GetPointerS<const call_hw_pragma>(node);
         res += ch->HW_component;
         if(ch->ID_implementation.size())
         {
            res += " " + STR(ch->ID_implementation);
         }
         break;
      }
      case call_point_hw_pragma_K:
      {
         res += STR_CST_pragma_keyword_call_point_hw " ";
         const auto ch = GetPointerS<const call_point_hw_pragma>(node);
         res += ch->HW_component;
         if(ch->ID_implementation.size())
         {
            res += " " + STR(ch->ID_implementation);
         }
         if(ch->recursive)
         {
            res += " " + std::string(STR_CST_pragma_keyword_recursive);
         }
         break;
      }
#else
      case map_pragma_K:
      case call_hw_pragma_K:
      case call_point_hw_pragma_K:
      {
         THROW_UNREACHABLE("Mapping pragmas can not be printed in this version");
         break;
      }
#endif
      case issue_pragma_K:
      {
         res = "issue";
         break;
      }
      case profiling_pragma_K:
      {
         res = "profiling";
         break;
      }
      case blackbox_pragma_K:
      {
         res = "blackbox";
         break;
      }
      case statistical_profiling_K:
      {
         res = "profiling";
         break;
      }
      case assert_expr_K:
      {
         const auto ae = GetPointerS<const assert_expr>(node);
         res += PrintNode(ae->op0, v, vppf) + "/* " + PrintNode(ae->op1, v, vppf) + "*/";
         break;
      }
      case reduc_max_expr_K:
      {
         const auto rme = GetPointerS<const reduc_max_expr>(node);
         res += "/*reduc_max_expr*/" + PrintNode(rme->op, v, vppf);
         break;
      }
      case reduc_min_expr_K:
      {
         const auto rme = GetPointerS<const reduc_min_expr>(node);
         res += "/*reduc_min_expr*/" + PrintNode(rme->op, v, vppf);
         break;
      }
      case reduc_plus_expr_K:
      {
         const auto rpe = GetPointerS<const reduc_plus_expr>(node);
         res += "/*reduc_plus_expr*/";
         res += "(" + tree_helper::PrintType(TM, rpe->type) + ") ";
         res += "{";
         const auto size = tree_helper::SizeAlloc(rpe->type);
         const auto element_type = tree_helper::CGetElements(rpe->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto vector_size = size / element_size;
         res += PrintNode(rpe->op, v, vppf) + "[0]";
         for(unsigned int ind = 1; ind < vector_size; ++ind)
         {
            res += "+" + PrintNode(rpe->op, v, vppf) + "[" + STR(ind) + "]";
         }
         for(unsigned int ind = 1; ind < vector_size; ++ind)
         {
            res += ", 0";
         }
         res += "}";
         break;
      }
      case vec_unpack_hi_expr_K:
      {
         const auto vuh = GetPointerS<const vec_unpack_hi_expr>(node);
         const auto op = vuh->op;
         const auto element_type = tree_helper::CGetElements(vuh->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vuh->type);
         const auto vector_size = size / element_size;
         res += "/*" + vuh->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vuh->type) + ") ";
         res += "{";
         if(op->get_kind() == vector_cst_K)
         {
            const auto vc = GetPointerS<const vector_cst>(op);
            for(auto i = static_cast<unsigned int>(vc->list_of_valu.size() / 2); i < vc->list_of_valu.size();
                i++) // vector elements
            {
               res += "((" + tree_helper::PrintType(TM, element_type) + ") (";
               res += PrintConstant(vc->list_of_valu[i], vppf);
               res += "))";
               if(i != (vc->list_of_valu).size() - 1)
               { // not the last element element
                  res += ", ";
               }
            }
         }
         else
         {
            for(auto ind = vector_size; ind < 2 * vector_size; ++ind)
            {
               res += "((" + tree_helper::PrintType(TM, element_type) + ") (" + PrintNode(vuh->op, v, vppf) + "[" +
                      STR(ind) + "]))";
               if(ind != 2 * vector_size - 1)
               {
                  res += ", ";
               }
            }
         }
         res += "}";
         break;
      }
      case vec_unpack_lo_expr_K:
      {
         const auto vul = GetPointerS<const vec_unpack_lo_expr>(node);
         const auto op = vul->op;
         const auto element_type = tree_helper::CGetElements(vul->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vul->type);
         const auto vector_size = size / element_size;
         res += "/*" + vul->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vul->type) + ") ";
         res += "{";
         if(op->get_kind() == vector_cst_K)
         {
            const auto vc = GetPointerS<const vector_cst>(op);
            for(unsigned int i = 0; i < vc->list_of_valu.size() / 2; i++) // vector elements
            {
               res += "((" + tree_helper::PrintType(TM, element_type) + ") (";
               res += PrintConstant(vc->list_of_valu[i], vppf);
               res += "))";
               if(i != (vc->list_of_valu).size() / 2 - 1)
               { // not the last element element
                  res += ", ";
               }
            }
         }
         else
         {
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "((" + tree_helper::PrintType(TM, element_type) + ") (" + PrintNode(vul->op, v, vppf) + "[" +
                      STR(ind) + "]))";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
         }
         res += "}";
         break;
      }
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      {
         const auto vie = GetPointerS<const unary_expr>(node);
         res += "/*" + vie->get_kind_text() + "*/" + PrintNode(vie->op, v, vppf);
         break;
      }
      case paren_expr_K:
      {
         const auto vie = GetPointerS<const unary_expr>(node);
         res += "(" + PrintNode(vie->op, v, vppf) + ")";
         break;
      }

      case vec_pack_trunc_expr_K:
      {
         const auto vpt = GetPointerS<const vec_pack_trunc_expr>(node);
         const auto op0 = vpt->op0;
         const auto op1 = vpt->op1;
         const auto element_type = tree_helper::CGetElements(vpt->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vpt->type);
         const auto vector_size = size / element_size;
         res += "/*" + vpt->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vpt->type) + ") ";
         res += "{";
         if(op0->get_kind() == vector_cst_K)
         {
            const auto vc = GetPointerS<const vector_cst>(op0);
            for(const auto& i : vc->list_of_valu) // vector elements
            {
               res += "((" + tree_helper::PrintType(TM, element_type) + ") (";
               res += PrintConstant(i, vppf);
               res += ")), ";
            }
         }
         else
         {
            res += "((" + tree_helper::PrintType(TM, element_type) + ") (" + PrintNode(vpt->op0, v, vppf) + "[0])), ";
            for(unsigned int ind = 1; ind < vector_size / 2; ++ind)
            {
               res += "((" + tree_helper::PrintType(TM, element_type) + ") (" + PrintNode(vpt->op0, v, vppf) + "[" +
                      STR(ind) + "])), ";
            }
         }

         if(op1->get_kind() == vector_cst_K)
         {
            const auto vc = GetPointerS<const vector_cst>(op1);
            for(unsigned int i = 0; i < (vc->list_of_valu).size(); i++) // vector elements
            {
               res += "((" + tree_helper::PrintType(TM, element_type) + ") (";
               res += PrintConstant(vc->list_of_valu[i], vppf);
               res += "))";
               if(i != (vc->list_of_valu).size() - 1)
               { // not the last element element
                  res += ", ";
               }
            }
         }
         else
         {
            res += "((" + tree_helper::PrintType(TM, element_type) + ") (" + PrintNode(vpt->op1, v, vppf) + "[0]))";
            for(unsigned int ind = 1; ind < vector_size / 2; ++ind)
            {
               res += ", ((" + tree_helper::PrintType(TM, element_type) + ") (" + PrintNode(vpt->op1, v, vppf) + "[" +
                      STR(ind) + "]))";
            }
         }
         res += "}";
         break;
      }
      case dot_prod_expr_K:
      {
         const auto dpe = GetPointerS<const ternary_expr>(node);
         const auto two_op_type = tree_helper::CGetType(dpe->op2);
         const auto element_type = tree_helper::CGetElements(two_op_type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(two_op_type);
         const auto vector_size = size / element_size;

         res += "/*" + dpe->get_kind_text() + "*/";
         if(tree_helper::IsVectorType(dpe->type) && tree_helper::IsVectorType(two_op_type) &&
            two_op_type->index != dpe->type->index)
         {
            res += "(" + tree_helper::PrintType(TM, dpe->type) + ")(";
         }
         res += "(" + tree_helper::PrintType(TM, two_op_type) + ")";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += "(" + PrintNode(dpe->op0, v, vppf) + "[" + STR(2 * ind) + "]" + " * " +
                   PrintNode(dpe->op1, v, vppf) + "[" + STR(2 * ind) + "]" + ")";
            res += "+(" + PrintNode(dpe->op0, v, vppf) + "[" + STR(2 * ind + 1) + "]" + " * " +
                   PrintNode(dpe->op1, v, vppf) + "[" + STR(2 * ind + 1) + "]" + ")";
            if(ind != (vector_size - 1))
            {
               res += ", ";
            }
         }
         res += "}";
         res += " + " + PrintNode(dpe->op2, v, vppf);
         if(tree_helper::IsVectorType(dpe->type) && tree_helper::IsVectorType(two_op_type) &&
            two_op_type->index != dpe->type->index)
         {
            res += ")";
         }
         break;
      }
      case widen_mult_hi_expr_K:
      {
         const auto wmhe = GetPointerS<const widen_mult_hi_expr>(node);
         const auto element_type = tree_helper::CGetElements(wmhe->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(wmhe->type);
         const auto vector_size = size / element_size;

         res += "/*" + wmhe->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, wmhe->type) + ") ";
         res += "{";
         for(auto ind = vector_size; ind < vector_size * 2; ++ind)
         {
            res += PrintNode(wmhe->op0, v, vppf) + "[" + STR(ind) + "]";
            res += " * ";
            res += PrintNode(wmhe->op1, v, vppf) + "[" + STR(ind) + "]";
            if(ind != (vector_size * 2 - 1))
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case widen_mult_lo_expr_K:
      {
         const auto wmle = GetPointerS<const widen_mult_lo_expr>(node);
         const auto element_type = tree_helper::CGetElements(wmle->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(wmle->type);
         const auto vector_size = size / element_size;

         res += "/*" + wmle->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, wmle->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += PrintNode(wmle->op0, v, vppf) + "[" + STR(ind) + "]";
            res += " * ";
            res += PrintNode(wmle->op1, v, vppf) + "[" + STR(ind) + "]";
            if(ind != (vector_size - 1))
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case vec_pack_sat_expr_K:
      case vec_pack_fix_trunc_expr_K:
      {
         const auto vie = GetPointerS<const binary_expr>(node);
         res += "/*" + vie->get_kind_text() + "*/" + PrintNode(vie->op0, v, vppf) + " /**/ " +
                PrintNode(vie->op1, v, vppf);
         break;
      }
      case vec_extracteven_expr_K:
      {
         const auto vee = GetPointerS<const vec_extracteven_expr>(node);
         const auto element_type = tree_helper::CGetElements(vee->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vee->type);
         const auto vector_size = size / element_size;

         res += "/*" + vee->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vee->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ind += 2)
         {
            res += PrintNode(vee->op0, v, vppf) + "[" + STR(ind) + "]";
            res += ", ";
         }
         for(unsigned int ind = 0; ind < vector_size; ind += 2)
         {
            res += PrintNode(vee->op1, v, vppf) + "[" + STR(ind) + "]";
            if(ind != vector_size - 2)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case vec_extractodd_expr_K:
      {
         const auto vee = GetPointerS<const vec_extractodd_expr>(node);
         const auto element_type = tree_helper::CGetElements(vee->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vee->type);
         const auto vector_size = size / element_size;

         res += "/*" + vee->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vee->type) + ") ";
         res += "{";
         for(unsigned int ind = 1; ind < vector_size; ind += 2)
         {
            res += PrintNode(vee->op0, v, vppf) + "[" + STR(ind) + "]";
            res += ", ";
         }
         for(unsigned int ind = 1; ind < vector_size; ind += 2)
         {
            res += PrintNode(vee->op1, v, vppf) + "[" + STR(ind) + "]";
            if(ind != vector_size - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case vec_interleavehigh_expr_K:
      {
         const auto vie = GetPointerS<const vec_interleavehigh_expr>(node);
         const auto element_type = tree_helper::CGetElements(vie->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vie->type);
         const auto vector_size = size / element_size;

         res += "/*" + vie->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vie->type) + ") ";
         res += "{";
         for(auto ind = vector_size / 2; ind < vector_size; ++ind)
         {
            res += PrintNode(vie->op0, v, vppf) + "[" + STR(ind) + "]";
            res += ", ";
            res += PrintNode(vie->op1, v, vppf) + "[" + STR(ind) + "]";
            if(ind != vector_size - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case vec_interleavelow_expr_K:
      {
         const auto vie = GetPointerS<const vec_interleavelow_expr>(node);
         const auto element_type = tree_helper::CGetElements(vie->type);
         const auto element_size = tree_helper::SizeAlloc(element_type);
         const auto size = tree_helper::SizeAlloc(vie->type);
         const auto vector_size = size / element_size;

         res += "/*" + vie->get_kind_text() + "*/";
         res += "(" + tree_helper::PrintType(TM, vie->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size / 2; ++ind)
         {
            res += PrintNode(vie->op0, v, vppf) + "[" + STR(ind) + "]";
            res += ", ";
            res += PrintNode(vie->op1, v, vppf) + "[" + STR(ind) + "]";
            if(ind != (vector_size / 2) - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case case_label_expr_K:
      {
         res = PrintConstant(node, vppf);
         break;
      }
      case array_range_ref_K:
      case catch_expr_K:
      case compound_expr_K:
      case eh_filter_expr_K:
      case fdesc_expr_K:
      case goto_subroutine_K:
      case in_expr_K:
      case modify_expr_K:
      case range_expr_K:
      case set_le_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case const_decl_K:
      case namespace_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case type_decl_K:
      case gimple_bind_K:
      case binfo_K:
      case block_K:
      case identifier_node_K:
      case statement_list_K:
      case tree_list_K:
      case tree_vec_K:
      case target_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case alignof_expr_K:
      case arrow_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case exit_expr_K:
      case loop_expr_K:
      case non_lvalue_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case error_mark_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
         THROW_ERROR(std::string("tree node not currently supported ") + node->get_kind_text());
         break;
      default:
         THROW_UNREACHABLE("");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Printed node " + STR(node->index) + " res = " + res);

   return res;
}

std::string BehavioralHelper::print_type_declaration(unsigned int type) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Printing type declaration " + STR(type));
   std::string res;
   const auto node_type = TM->GetTreeNode(type);
   switch(node_type->get_kind())
   {
      case record_type_K:
      {
         const auto rt = GetPointerS<const record_type>(node_type);
         THROW_ASSERT(tree_helper::GetRealType(TM, type) == type, "Printing declaration of fake type " + STR(type));
         if(rt->unql)
         {
            res += "typedef ";
         }
         res += tree_helper::return_C_qualifiers(rt->qual, false);
         if(!rt->unql)
         {
            res += tree_helper::PrintType(TM, node_type) + " ";
         }
         else
         {
            res += "struct ";
         }
         if(!rt->unql ||
            (!GetPointerS<const record_type>(rt->unql)->name && !Param->getOption<bool>(OPT_without_transformation)))
         {
            /// Print the contents of the structure
            res += "\n{\n";
            null_deleter null_del;
            const var_pp_functorConstRef std_vpf(new std_var_pp_functor(BehavioralHelperConstRef(this, null_del)));
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Printing content of the structure");
            for(auto& list_of_fld : rt->list_of_flds)
            {
               auto field = list_of_fld->index;
               const auto fld_node = TM->GetTreeNode(field);
               if(fld_node->get_kind() == type_decl_K)
               {
                  continue;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Printing field " + STR(field));
               const auto fd = GetPointerS<const field_decl>(fld_node);
               const auto field_type = tree_helper::CGetType(fld_node);
               if(has_bit_field(field))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Bit field");
                  res += tree_helper::PrintType(TM, field_type) + " ";
                  res += (*std_vpf)(field);
                  res += " : ";
                  res += PrintConstant(fd->size);
               }
               else
               {
                  res += tree_helper::PrintType(TM, field_type, false, false, false, fld_node, std_vpf);
               }
               if(fd && fd->packed_flag)
               {
                  res += " __attribute__((packed))";
               }
               res += ";\n";
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
            res += '}';
            if(rt->packed_flag)
            {
               res += " __attribute__((packed))";
            }
            res += " ";
         }
         if(rt->unql)
         {
            if(rt->name || Param->getOption<bool>(OPT_without_transformation))
            {
               const auto rt_unqal = GetPointerS<const record_type>(rt->unql);
               if(rt_unqal->name)
               {
                  res += tree_helper::PrintType(TM, rt_unqal->name) + " ";
               }
               else if(Param->getOption<bool>(OPT_without_transformation))
               {
                  res += "Internal_" + STR(rt->unql->index) + " ";
               }
               res += tree_helper::PrintType(TM, rt->name);
            }
            if(rt->algn != GetPointerS<const record_type>(rt->unql)->algn)
            {
               res += " __attribute__ ((aligned (" + STR(rt->algn / 8) + ")))";
            }
         }
         break;
      }
      case union_type_K:
      {
         const auto ut = GetPointerS<const union_type>(node_type);
         THROW_ASSERT(tree_helper::GetRealType(TM, type) == type,
                      "Printing declaration of fake type " + STR(node_type));
         if(ut->unql)
         {
            res += "typedef ";
         }
         res += tree_helper::return_C_qualifiers(ut->qual, false);
         if(!ut->unql)
         {
            res += tree_helper::PrintType(TM, node_type) + " ";
         }
         else
         {
            res += "union ";
         }
         if(!ut->unql ||
            (!GetPointerS<const union_type>(ut->unql)->name && !Param->getOption<bool>(OPT_without_transformation)))
         {
            /// Print the contents of the structure
            res += "\n{\n";
            null_deleter null_del;
            const var_pp_functorConstRef std_vpf(new std_var_pp_functor(BehavioralHelperConstRef(this, null_del)));
            for(const auto& list_of_fld : ut->list_of_flds)
            {
               res += tree_helper::PrintType(TM, tree_helper::CGetType(list_of_fld), false, false, false, list_of_fld,
                                             std_vpf);
               res += ";\n";
            }
            res += '}';
            if(ut->packed_flag)
            {
               res += " __attribute__((packed))";
            }
            res += " ";
         }
         if(ut->unql)
         {
            if(ut->name || Param->getOption<bool>(OPT_without_transformation))
            {
               const auto uut = GetPointerS<const union_type>(ut->unql);
               if(uut->name)
               {
                  res += tree_helper::PrintType(TM, uut->name) + " ";
               }
               else if(Param->getOption<bool>(OPT_without_transformation))
               {
                  res += "Internal_" + STR(ut->unql->index) + " ";
               }
               else
               {
                  THROW_UNREACHABLE("");
               }
               res += tree_helper::PrintType(TM, ut->name);
            }
            if(ut->algn != GetPointerS<const union_type>(ut->unql)->algn)
            {
               res += " __attribute__ ((aligned (" + STR(ut->algn / 8) + "))) ";
            }
         }
         break;
      }
      case enumeral_type_K:
      {
         const auto et = GetPointerS<const enumeral_type>(node_type);
         if(et->unql)
         {
            res += "typedef ";
         }
         res += tree_helper::return_C_qualifiers(et->qual, false);
         res += "enum ";
         if(!et->unql)
         {
            if(et->packed_flag)
            {
               res += " __attribute__((packed)) ";
            }
            if(et->name)
            {
               res += tree_helper::PrintType(TM, et->name) + " ";
            }
            else
            {
               res += "Internal_" + STR(type) + " ";
            }
         }
         if(!et->unql || !GetPointerS<const enumeral_type>(et->unql)->name)
         {
            res += "{";
            auto tl = GetPointer<tree_list>(et->csts);
            while(tl)
            {
               res += tree_helper::PrintType(TM, tl->purp);
               if(tl->valu)
               {
                  res += " = " + PrintConstant(tl->valu);
               }
               if(tl->chan)
               {
                  res += ",";
                  tl = GetPointer<tree_list>(tl->chan);
               }
               else
               {
                  tl = nullptr;
               }
            }
            res += "}";
         }
         if(et->unql && et->name)
         {
            const auto et_unql = GetPointerS<const enumeral_type>(et->unql);
            if(et_unql->name)
            {
               res += tree_helper::PrintType(TM, et_unql->name) + " ";
            }
            res += tree_helper::PrintType(TM, et->name);
         }
         break;
      }
      case array_type_K:
      {
         const auto at = GetPointerS<const array_type>(node_type);
         /// Compute the dimensions
         if(!at->size)
         {
            THROW_ERROR_CODE(C_EC, "Declaration of array type without size");
         }
         const auto array_length = at->size;
         const auto array_t = at->elts;
         if(array_length->get_kind() != integer_cst_K)
         {
            THROW_ERROR_CODE(C_EC, "Declaration of array type without fixed size");
         }
         const auto tn = GetPointerS<const type_node>(array_t);

         res += "typedef ";
         res += tree_helper::PrintType(TM, at->elts);
         res += " ";
         THROW_ASSERT(at->name, "Trying to declare array without name " + STR(type));
         res += tree_helper::PrintType(TM, at->name);
         res += "[";
         res += STR(tree_helper::GetConstValue(at->size) / tree_helper::GetConstValue(tn->size));
         res += "]";
         break;
      }
      /// NOTE: this case cannot be moved because of break absence
      case pointer_type_K:
      {
         const auto pt = GetPointerS<const pointer_type>(node_type);
         if(pt->unql && pt->ptd->get_kind() == function_type_K)
         {
            const auto ft = GetPointerS<const function_type>(pt->ptd);
            res += "typedef ";
            res += tree_helper::PrintType(TM, ft->retn);
            res += " (* ";
            res += tree_helper::return_C_qualifiers(GetPointerS<const type_node>(node_type)->qual, false);
            if(GetPointerS<const type_node>(node_type)->name)
            {
               res += tree_helper::PrintType(TM, GetPointerS<const type_node>(node_type)->name);
            }
            else
            {
               res += "Internal_" + STR(type);
            }
            res += ") (";
            if(ft->prms)
            {
               res += tree_helper::PrintType(TM, ft->prms);
            }
            res += ")";
            return res;
         }
         // in this point break has not been forgotten. Pointer to not function type could be treated normally
#if(__GNUC__ >= 7)
         [[gnu::fallthrough]];
#endif
      }
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
      case typename_type_K:
      case type_argument_pack_K:
      case vector_type_K:
      case void_type_K:
      {
         const auto nt = GetPointerS<const type_node>(node_type);
         if(nt->unql)
         {
            res += "typedef ";
            res += tree_helper::return_C_qualifiers(nt->qual, false);
            res += tree_helper::PrintType(TM, nt->unql);
            res += " ";
            /*            if(nt->algn != 8)
             res += "__attribute__ ((aligned (" + STR(nt->algn/8) + "))) ";*/
            if(nt->name)
            {
               res += tree_helper::PrintType(TM, nt->name);
            }
            else
            {
               res += "Internal_" + STR(type);
            }
         }
         else
         {
            THROW_ASSERT(node_type->get_kind() == integer_cst_K,
                         "Expected an integer, got a " + node_type->get_kind_text());
            THROW_ASSERT(nt->name, "Expected a typedef declaration with a name " + STR(type));
            /* #ifndef NDEBUG
                        identifier_node * id = GetPointerS<const identifier_node>(bt->name);
                        THROW_ASSERT(id && id->strg == "bit_size_type", "Expected bit_size_type " + STR(type));
            #endif */
            res += "typedef long long int bit_size_type";
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
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Printed type declaration " + STR(type));
   return res;
}

bool BehavioralHelper::is_bool(unsigned int index) const
{
   return tree_helper::is_bool(TM, get_type(index));
}

bool BehavioralHelper::is_natural(unsigned int index) const
{
   return tree_helper::is_natural(TM, index);
}

bool BehavioralHelper::is_int(unsigned int index) const
{
   return tree_helper::is_int(TM, get_type(index));
}

bool BehavioralHelper::is_an_enum(unsigned int index) const
{
   return tree_helper::is_an_enum(TM, get_type(index));
}
bool BehavioralHelper::is_unsigned(unsigned int index) const
{
   return tree_helper::is_unsigned(TM, get_type(index));
}

bool BehavioralHelper::is_real(unsigned int index) const
{
   return tree_helper::is_real(TM, get_type(index));
}

bool BehavioralHelper::is_a_complex(unsigned int index) const
{
   return tree_helper::is_a_complex(TM, get_type(index));
}

bool BehavioralHelper::is_a_struct(unsigned int variable) const
{
   return tree_helper::is_a_struct(TM, get_type(variable));
}

bool BehavioralHelper::is_an_union(unsigned int variable) const
{
   return tree_helper::is_an_union(TM, get_type(variable));
}

bool BehavioralHelper::is_an_array(unsigned int variable) const
{
   return tree_helper::is_an_array(TM, get_type(variable));
}

bool BehavioralHelper::is_a_vector(unsigned int variable) const
{
   return tree_helper::is_a_vector(TM, get_type(variable));
}

bool BehavioralHelper::is_a_pointer(unsigned int variable) const
{
   return tree_helper::is_a_pointer(TM, variable);
}

bool BehavioralHelper::is_an_indirect_ref(unsigned int variable) const
{
   const auto temp = TM->GetTreeNode(variable);
   if(temp->get_kind() == indirect_ref_K || temp->get_kind() == misaligned_indirect_ref_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_an_array_ref(unsigned int variable) const
{
   const auto temp = TM->GetTreeNode(variable);
   if(temp->get_kind() == array_ref_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_a_component_ref(unsigned int variable) const
{
   const auto temp = TM->GetTreeNode(variable);
   if(temp->get_kind() == component_ref_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_an_addr_expr(unsigned int variable) const
{
   const auto temp = TM->GetTreeNode(variable);
   if(temp->get_kind() == addr_expr_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_a_mem_ref(unsigned int variable) const
{
   const auto temp = TM->GetTreeNode(variable);
   if(temp->get_kind() == mem_ref_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_static(unsigned int decl) const
{
   return tree_helper::is_static(TM, decl);
}

bool BehavioralHelper::is_extern(unsigned int decl) const
{
   return tree_helper::is_extern(TM, decl);
}

bool BehavioralHelper::is_a_constant(unsigned int obj) const
{
   const auto node = TM->GetTreeNode(obj);
   switch(node->get_kind())
   {
      case addr_expr_K:
         return true;
      case paren_expr_K:
      case nop_expr_K:
      {
         const auto ue = GetPointerS<const unary_expr>(node);
         return is_a_constant(ue->op->index);
      }
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case case_label_expr_K:
      case label_decl_K:
         return true;
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case constructor_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case const_decl_K:
      case field_decl_K:
      case function_decl_K:
      case namespace_decl_K:
      case parm_decl_K:
      case result_decl_K:
      case translation_unit_decl_K:
      case template_decl_K:
      case using_decl_K:
      case type_decl_K:
      case var_decl_K:
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
      case negate_expr_K:
      case non_lvalue_expr_K:
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
      case target_expr_K:
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
      default:
         return false;
   }
}

bool BehavioralHelper::is_a_result_decl(unsigned int obj) const
{
   const tree_nodeRef node = TM->GetTreeNode(obj);
   if(node->get_kind() == result_decl_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_a_realpart_expr(unsigned int obj) const
{
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   if(temp->get_kind() == realpart_expr_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_a_imagpart_expr(unsigned int obj) const
{
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   if(temp->get_kind() == imagpart_expr_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_operating_system_function(const unsigned int obj) const
{
   const tree_nodeRef curr_tn = TM->GetTreeNode(obj);
   const function_decl* fd = GetPointer<function_decl>(curr_tn);
   if(!fd)
   {
      return false;
   }
   return fd->operating_system_flag;
}

unsigned int BehavioralHelper::get_indirect_ref_var(unsigned int obj) const
{
   THROW_ASSERT(is_an_indirect_ref(obj), "obj assumed to be an inderect_ref object");
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   auto ir = GetPointer<indirect_ref>(temp);
   if(ir)
   {
      return ir->op->index;
   }
   auto mir = GetPointer<misaligned_indirect_ref>(temp);
   return mir->op->index;
}

unsigned int BehavioralHelper::get_array_ref_array(unsigned int obj) const
{
   THROW_ASSERT(is_an_array_ref(obj), "obj assumed to be an array_ref object");
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   auto ar = GetPointer<array_ref>(temp);
   return ar->op0->index;
}

unsigned int BehavioralHelper::get_array_ref_index(unsigned int obj) const
{
   THROW_ASSERT(is_an_array_ref(obj), "obj assumed to be an array_ref object");
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   auto ar = GetPointer<array_ref>(temp);
   return ar->op1->index;
}

unsigned int BehavioralHelper::get_component_ref_record(unsigned int obj) const
{
   THROW_ASSERT(is_a_component_ref(obj), "obj assumed to be a component_ref object");
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   auto cr = GetPointer<component_ref>(temp);
   return cr->op0->index;
}

unsigned int BehavioralHelper::get_component_ref_field(unsigned int obj) const
{
   THROW_ASSERT(is_a_component_ref(obj), "obj assumed to be a component_ref object");
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   auto cr = GetPointer<component_ref>(temp);
   return cr->op1->index;
}

unsigned int BehavioralHelper::get_mem_ref_base(unsigned int obj) const
{
   THROW_ASSERT(is_a_mem_ref(obj), "obj assumed to be a mem_ref object");
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   auto mr = GetPointer<mem_ref>(temp);
   return mr->op0->index;
}

unsigned int BehavioralHelper::get_mem_ref_offset(unsigned int obj) const
{
   THROW_ASSERT(is_a_mem_ref(obj), "obj assumed to be a mem_ref object");
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   auto mr = GetPointer<mem_ref>(temp);
   return mr->op1->index;
}

unsigned int BehavioralHelper::get_operand_from_unary_expr(unsigned int obj) const
{
   THROW_ASSERT(is_an_addr_expr(obj) || is_a_realpart_expr(obj) || is_a_imagpart_expr(obj),
                "obj assumed to be an addr_expr, a realpart_expr or an imagpart_expr object. obj is " + STR(obj));
   const tree_nodeRef temp = TM->GetTreeNode(obj);
   auto ue = GetPointer<unary_expr>(temp);
   return ue->op->index;
}

unsigned int BehavioralHelper::GetVarFromSsa(unsigned int index) const
{
   const tree_nodeRef temp = TM->GetTreeNode(index);
   auto sn = GetPointer<ssa_name>(temp);
   if(sn)
   {
      return sn->var->index;
   }
   else
   {
      return index;
   }
}

unsigned int BehavioralHelper::get_intermediate_var(unsigned int obj) const
{
   const tree_nodeRef node = TM->GetTreeNode(obj);
   switch(node->get_kind())
   {
      case modify_expr_K:
      case init_expr_K:
      {
         auto be = GetPointer<binary_expr>(node);
         const tree_nodeRef right = be->op1;
         /// check for type conversion
         switch(right->get_kind())
         {
               // case fix_trunc_expr_K:
            case fix_ceil_expr_K:
            case fix_floor_expr_K:
            case fix_round_expr_K:
            case float_expr_K:
            case convert_expr_K:
            case view_convert_expr_K:
            case nop_expr_K:
            case realpart_expr_K:
            case imagpart_expr_K:
            case paren_expr_K:
            {
               auto ue = GetPointer<unary_expr>(right);
               return ue->op->index;
            }
            case binfo_K:
            case block_K:
            case call_expr_K:
            case aggr_init_expr_K:
            case case_label_expr_K:
            case constructor_K:
            case fix_trunc_expr_K:
            case identifier_node_K:
            case ssa_name_K:
            case statement_list_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case tree_list_K:
            case tree_vec_K:
            case abs_expr_K:
            case addr_expr_K:
            case alignof_expr_K:
            case arrow_expr_K:
            case bit_not_expr_K:
            case buffer_ref_K:
            case card_expr_K:
            case cleanup_point_expr_K:
            case conj_expr_K:
            case exit_expr_K:
            case indirect_ref_K:
            case misaligned_indirect_ref_K:
            case loop_expr_K:
            case negate_expr_K:
            case non_lvalue_expr_K:
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
            case target_expr_K:
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
               return be->op1->index;
         }
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
      case target_mem_ref_K:
      case target_mem_ref461_K:
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
      case le_expr_K:
      case lrotate_expr_K:
      case lshift_expr_K:
      case lt_expr_K:
      case max_expr_K:
      case mem_ref_K:
      case min_expr_K:
      case minus_expr_K:
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
      case lut_expr_K:
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
      case target_expr_K:
      case error_mark_K:
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
      case CASE_UNARY_EXPRESSION:
      default:
         return 0;
   }
}

TreeNodeConstSet BehavioralHelper::GetParameterTypes() const
{
   TreeNodeConstSet ret;
   const auto node = TM->GetTreeNode(function_index);
   const auto fd = GetPointer<const function_decl>(node);
   if(!fd)
   {
      return ret;
   }
   for(const auto& arg : fd->list_of_args)
   {
      const auto pd = GetPointerS<const parm_decl>(arg);
      ret.insert(pd->type);
   }
   const auto ft = GetPointerS<const function_type>(fd->type);
   if(!ft || !ft->prms)
   {
      return ret;
   }
   auto tl = GetPointer<const tree_list>(ft->prms);
   while(tl)
   {
      ret.insert(tl->valu);
      if(tl->chan)
      {
         tl = GetPointerS<const tree_list>(tl->chan);
      }
      else
      {
         break;
      }
   }
   return ret;
}

unsigned int BehavioralHelper::is_named_pointer(const unsigned int index) const
{
   THROW_ASSERT(index, "this index does not exist: " + STR(index));
   const auto type = TM->GetTreeNode(index);
   THROW_ASSERT(type, "this index does not exist: " + STR(type));
   const auto Type_node = tree_helper::CGetType(type);
   THROW_ASSERT(Type_node, "this index does not exist: " + STR(type));
   if(Type_node->get_kind() == pointer_type_K)
   {
      const auto pt = GetPointerS<const pointer_type>(Type_node);
      if(pt->name)
      {
         return type->index;
      }
      else
      {
         return is_named_pointer(pt->ptd->index);
      }
   }
   else
   {
      return 0;
   }
}

bool BehavioralHelper::is_va_start_call(unsigned int stm) const
{
   if(is_var_args())
   {
      const tree_nodeRef node = TM->GetTreeNode(stm);
      if(node->get_kind() == gimple_call_K)
      {
         auto ce = GetPointer<gimple_call>(node);
         tree_nodeRef cefn = ce->fn;
         if(cefn->get_kind() == addr_expr_K)
         {
            auto ue = GetPointer<unary_expr>(cefn);
            auto fd = GetPointer<function_decl>(ue->op);
            if(fd && tree_helper::print_function_name(TM, fd) == "__builtin_va_start")
            {
               return true;
            }
            else
            {
               return false;
            }
         }
         else
         {
            return false;
         }
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::has_bit_field(unsigned int variable) const
{
   const tree_nodeRef node = TM->GetTreeNode(variable);
   if(node->get_kind() == field_decl_K)
   {
      auto fd = GetPointer<field_decl>(node);
      if((fd->list_attr.find(TreeVocabularyTokenTypes_TokenEnum::TOK_BITFIELD)) != fd->list_attr.end())
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }
}

unsigned int BehavioralHelper::get_attributes(unsigned int var) const
{
   const tree_nodeRef node = TM->GetTreeNode(var);
   switch(node->get_kind())
   {
      case parm_decl_K:
      case ssa_name_K:
      {
         /*ssa_name * sn = GetPointer<ssa_name>(node);
          return get_attributes(sn->var->index);*/
         return 0;
      }
      case function_decl_K:
      case result_decl_K:
      case var_decl_K:
      {
         THROW_ASSERT(GetPointer<decl_node>(node), "get_attributes is only for decl_node: " + STR(var));
         return GetPointer<decl_node>(node)->attributes ? GetPointer<decl_node>(node)->attributes->index : 0;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case identifier_node_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case const_decl_K:
      case field_decl_K:
      case label_decl_K:
      case namespace_decl_K:
      case translation_unit_decl_K:
      case using_decl_K:
      case type_decl_K:
      case template_decl_K:
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
         THROW_ERROR("Not supported: " + std::string(node->get_kind_text()));
   }
   return 0;
}

unsigned int BehavioralHelper::GetInit(unsigned int var, CustomUnorderedSet<unsigned int>& list_of_variables) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Get init of " + PrintVariable(var));
   if(initializations.find(var) != initializations.end())
   {
      unsigned int init = var;
      while(initializations.find(init) != initializations.end())
      {
         init = initializations.find(init)->second;
      }
      if(TM->GetTreeNode(init)->get_kind() == var_decl_K)
      {
         list_of_variables.insert(init);
         const auto init_of_init = GetInit(init, list_of_variables);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Init is " + STR(init_of_init));
         return init_of_init;
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Init is " + STR(init));
         return init;
      }
   }
   const tree_nodeRef node = TM->GetTreeNode(var);
   switch(node->get_kind())
   {
      case ssa_name_K:
      {
         auto sn = GetPointer<ssa_name>(node);
         if(!sn->var)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Without init");
            return 0;
         }
         const unsigned ssa_init = GetInit(sn->var->index, list_of_variables);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Init is " + STR(ssa_init));
         return ssa_init;
      }
      case var_decl_K:
      {
         auto vd = GetPointer<var_decl>(node);
         if(vd->init)
         {
            tree_helper::get_used_variables(true, vd->init, list_of_variables);
            const unsigned var_init = vd->init->index;
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Init is " + STR(var_init));
            return var_init;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Without init");
            return 0;
         }
      }
      case constructor_K:
      {
         auto co = GetPointerS<const constructor>(TM->GetTreeNode(var));
         auto vend = co->list_of_idx_valu.end();
         for(auto i = co->list_of_idx_valu.begin(); i != vend; ++i)
         {
            tree_helper::get_used_variables(true, i->second, list_of_variables);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Init is " + STR(var));
         return var;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case identifier_node_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
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
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Without init");
         return 0;
      }
      default:
         THROW_UNREACHABLE("get_init: Tree node not yet supported " + STR(var) + " " + node->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Without init");
   return 0;
}

std::string BehavioralHelper::print_phinode_res(unsigned int phi_node_id, vertex v,
                                                const var_pp_functorConstRef vppf) const
{
   const auto node = TM->GetTreeNode(phi_node_id);
   const auto phi = GetPointer<const gimple_phi>(node);
   THROW_ASSERT(phi, "NodeId is not related to a gimple_phi");
   return PrintNode(phi->res, v, vppf);
}

unsigned int BehavioralHelper::start_with_a_label(const blocRef& block) const
{
   if(block->CGetStmtList().empty())
   {
      return 0;
   }
   const auto& first_stmt = block->CGetStmtList().front();
   const auto le = GetPointer<const gimple_label>(first_stmt);
   if(le && le->op && le->op->get_kind() == label_decl_K)
   {
      const auto ld = GetPointerS<const label_decl>(le->op);
      if(ld->name)
      {
         return first_stmt->index;
      }
      else
      {
         return 0;
      }
   }
   else
   {
      return 0;
   }
}

const std::string BehavioralHelper::get_label_name(unsigned int label_expr_nid) const
{
   tree_nodeRef tn = TM->GetTreeNode(label_expr_nid);
   auto le = GetPointer<gimple_label>(tn);
   THROW_ASSERT(le->op && le->op->get_kind() == label_decl_K, "label decl expected");
   auto ld = GetPointer<label_decl>(le->op);
   THROW_ASSERT(ld->name && ld->name->get_kind() == identifier_node_K, "identifier_node expected");
   auto id = GetPointer<identifier_node>(ld->name);
   return id->strg;
}

unsigned int BehavioralHelper::end_with_a_cond_or_goto(const blocRef& block) const
{
   if(block->CGetStmtList().empty())
   {
      return 0;
   }
   tree_nodeRef last = block->CGetStmtList().back();
   auto gc = GetPointer<gimple_cond>(last);
   if(gc)
   {
      return last->index;
   }
   auto se = GetPointer<gimple_switch>(last);
   if(se)
   {
      return last->index;
   }
   auto ge = GetPointer<gimple_goto>(last);
   if(ge)
   {
      return last->index;
   }
   auto gmwi = GetPointer<gimple_multi_way_if>(last);
   if(gmwi)
   {
      return last->index;
   }
   return 0;
}

std::string BehavioralHelper::print_forward_declaration(unsigned int type) const
{
   std::string res;
   const auto node_type = TM->GetTreeNode(type);
   switch(node_type->get_kind())
   {
      case record_type_K:
      {
         const auto rt = GetPointerS<const record_type>(node_type);
         if(rt->name)
         {
            res += "struct " + tree_helper::PrintType(TM, rt->name);
         }
         else
         {
            res += "struct Internal_" + STR(type);
         }
         break;
      }
      case union_type_K:
      {
         const auto rt = GetPointerS<const union_type>(node_type);
         if(rt->name)
         {
            res += "union " + tree_helper::PrintType(TM, rt->name);
         }
         else
         {
            res += "union Internal_" + STR(type);
         }
         break;
      }
      case enumeral_type_K:
      {
         const auto rt = GetPointerS<const enumeral_type>(node_type);
         if(rt->name)
         {
            res += "enum " + tree_helper::PrintType(TM, rt->name);
         }
         else
         {
            res += "enum Internal_" + STR(type);
         }
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
      case constructor_K:
      case function_type_K:
      case identifier_node_K:
      case integer_type_K:
      case lang_type_K:
      case method_type_K:
      case offset_type_K:
      case pointer_type_K:
      case qual_union_type_K:
      case real_type_K:
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
         THROW_ERROR("Not yet supported " + std::string(node_type->get_kind_text()));
      }
   }
   return res;
}

bool BehavioralHelper::is_empty_return(unsigned int index) const
{
   const auto retNode = TM->GetTreeNode(index);
   const auto re = GetPointer<const gimple_return>(retNode);
   THROW_ASSERT(re, "Expected a return statement");
   return !re->op;
}

unsigned int BehavioralHelper::GetUnqualified(const unsigned int index) const
{
   return tree_helper::GetUnqualified(TM, index);
}

std::string BehavioralHelper::print_type(unsigned int type, bool global, bool print_qualifiers, bool print_storage,
                                         unsigned int var, const var_pp_functorConstRef vppf, const std::string& prefix,
                                         const std::string& tail) const
{
   return tree_helper::PrintType(TM, TM->GetTreeNode(type), global, print_qualifiers, print_storage,
                                 var ? TM->GetTreeNode(var) : nullptr, vppf, prefix, tail);
}

void BehavioralHelper::rename_a_variable(unsigned int var, const std::string& new_name)
{
   vars_renaming_table[var] = new_name;
}

void BehavioralHelper::clear_renaming_table()
{
   vars_renaming_table.clear();
}

void BehavioralHelper::GetTypecast(const tree_nodeConstRef& tn, TreeNodeConstSet& types) const
{
   type_casting Visitor(types);
   tn->visit(&Visitor);
}

bool BehavioralHelper::IsDefaultSsaName(const unsigned int ssa_name_index) const
{
   const auto sn = GetPointer<const ssa_name>(TM->GetTreeNode(ssa_name_index));
   return sn && sn->default_flag;
}

#if HAVE_FROM_PRAGMA_BUILT
size_t BehavioralHelper::GetOmpForDegree() const
{
   const auto fd = GetPointerS<const function_decl>(TM->GetTreeNode(function_index));
   return fd->omp_for_wrapper;
}

bool BehavioralHelper::IsOmpFunctionAtomic() const
{
   const auto fd = GetPointerS<const function_decl>(TM->GetTreeNode(function_index));
   return fd->omp_atomic;
}

bool BehavioralHelper::IsOmpBodyLoop() const
{
   const auto fd = GetPointerS<const function_decl>(TM->GetTreeNode(function_index));
   return fd->omp_body_loop;
}
#endif

#if HAVE_FROM_PRAGMA_BUILT
bool BehavioralHelper::IsOmpAtomic() const
{
   const auto fd = GetPointerS<const function_decl>(TM->GetTreeNode(function_index));
   return fd->omp_atomic;
}
#endif

bool BehavioralHelper::function_has_to_be_printed(unsigned int f_id) const
{
   if(function_name == "__builtin_cond_expr32")
   {
      return false;
   }
   if(tree_helper::IsInLibbambu(TM, f_id))
   {
      return true;
   }
   return !tree_helper::is_system(TM, f_id);
}

std::string BehavioralHelper::get_asm_string(const unsigned int node_index) const
{
   return tree_helper::get_asm_string(TM, node_index);
}

bool BehavioralHelper::CanBeSpeculated(const unsigned int node_index) const
{
   if(node_index == ENTRY_ID || node_index == EXIT_ID)
   {
      return false;
   }
   const auto tn = TM->GetTreeNode(node_index);
   const auto ga = GetPointer<const gimple_assign>(tn);
   /// This check must be done before check of load or store since predicated load nd predicated store can be speculated
   if(ga && ga->predicate && ga->predicate->get_kind() != integer_cst_K)
   {
      return true;
   }
   if(IsStore(node_index))
   {
      return false;
   }
   /// Load cannot be speculated since load from not mapped addresses would not end
   if(IsLoad(node_index))
   {
      return false;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Checking if " + STR(tn) + " can be speculated");
   switch(tn->get_kind())
   {
      case gimple_nop_K:
      case gimple_phi_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Yes because is a " + tn->get_kind_text());
         return true;
      }
      case(gimple_assign_K):
      {
         THROW_ASSERT(ga && ga->op1, "unexpected condition"); // to silence the clang static analyzer
         switch(ga->op1->get_kind())
         {
            case call_expr_K:
            case aggr_init_expr_K:
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "<--No because of " + ga->op1->get_kind_text() + " in right part of gimple_assign");
               return false;
            }
            case CASE_CST_NODES:
            case CASE_DECL_NODES:
            case CASE_QUATERNARY_EXPRESSION:
            case CASE_TERNARY_EXPRESSION:
            case CASE_UNARY_EXPRESSION:
            case ssa_name_K:
            case assert_expr_K:
            case bit_and_expr_K:
            case bit_ior_expr_K:
            case bit_xor_expr_K:
            case catch_expr_K:
            case complex_expr_K:
            case compound_expr_K:
            case constructor_K:
            case eh_filter_expr_K:
            case eq_expr_K:
            case fdesc_expr_K:
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
            case mem_ref_K:
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
            case rrotate_expr_K:
            case rshift_expr_K:
            case set_le_expr_K:
            case trunc_div_expr_K:
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
            case ceil_div_expr_K:
            case ceil_mod_expr_K:
            case exact_div_expr_K:
            case floor_div_expr_K:
            case floor_mod_expr_K:
            case rdiv_expr_K:
            case round_div_expr_K:
            case round_mod_expr_K:
            case trunc_mod_expr_K:
            case target_expr_K:
            case extract_bit_expr_K:
            case sat_plus_expr_K:
            case sat_minus_expr_K:
            case extractvalue_expr_K:
            case extractelement_expr_K:
            case frem_expr_K:
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "<--Yes because it is a gimple_assign with " + ga->op1->get_kind_text() +
                                  " in right part of assignment");
               return true;
            }
            case binfo_K:
            case block_K:
            case CASE_GIMPLE_NODES:
            case case_label_expr_K:
            case identifier_node_K:
            case statement_list_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case tree_list_K:
            case tree_vec_K:
            case error_mark_K:
            case CASE_CPP_NODES:
            case CASE_FAKE_NODES:
            case CASE_PRAGMA_NODES:
            case CASE_TYPE_NODES:
            {
               THROW_UNREACHABLE(ga->op1->get_kind_text() + " - " + STR(tn));
               break;
            }
            default:
               THROW_UNREACHABLE("");
         }
         return true;
      }
      case gimple_asm_K:
      /// Call functions cannot be speculated because of possible effects not caught by virtuals in particular corner
      /// case (gcc-4.6 -O0 gcc_regression_simple/20070424-1.c)
      case gimple_call_K:
      case gimple_cond_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_multi_way_if_K:
      case gimple_pragma_K:
      case gimple_return_K:
      case gimple_switch_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--No because is a " + tn->get_kind_text());
         return false;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_while_K:
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
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
      {
         THROW_UNREACHABLE(tn->get_kind_text());
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   THROW_UNREACHABLE("");
   return false;
}

bool BehavioralHelper::CanBeMoved(const unsigned int node_index) const
{
   // entry and exit nodes can never be moved
   if(node_index == ENTRY_ID || node_index == EXIT_ID)
   {
      return false;
   }
   THROW_ASSERT(node_index, "unexpected condition");
   const auto tn = TM->GetTreeNode(node_index);
   const auto gn = GetPointer<const gimple_node>(tn);
   THROW_ASSERT(gn, "unexpected condition: node " + STR(tn) + " is not a gimple_node");
   /*
    * artificial gimple_node can never be moved because they are created to
    * handle specific situations, like for example handling functions returnin
    * structs by value or accepting structs passed by value as parameters
    */
   if(gn->artificial)
   {
      return false;
   }
   const auto gc = GetPointer<const gimple_call>(tn);
   if(gc && gc->fn->get_kind() == addr_expr_K)
   {
      // the node is a gimple_call to a function (no function pointers
      const auto addr_node = gc->fn;
      const auto ae = GetPointerS<const addr_expr>(addr_node);
      THROW_ASSERT(ae->op->get_kind() == function_decl_K,
                   "node  " + STR(ae->op) + " is not function_decl but " + ae->op->get_kind_text());
      auto called_id = ae->op->index;
      const std::string fu_name = tree_helper::name_function(TM, called_id);
      /*
       * __builtin_bambu_time_start() and __builtin_bambu_time_stop() can never
       * be moved, even if they have not the artificial flag.
       * the reason is that they must stay exactly where they are placed in
       * order to work properly to compute the number of simulation cycles
       */
      if(fu_name == "__builtin_bambu_time_start" || fu_name == "__builtin_bambu_time_stop")
      {
         return false;
      }
   }
   return true;
}

bool BehavioralHelper::IsStore(const unsigned int statement_index) const
{
   const auto& fun_mem_data = AppM->CGetFunctionBehavior(function_index)->get_function_mem();
   return tree_helper::IsStore(TM->GetTreeNode(statement_index), fun_mem_data);
}

bool BehavioralHelper::IsLoad(const unsigned int statement_index) const
{
   const auto& fun_mem_data = AppM->CGetFunctionBehavior(function_index)->get_function_mem();
   return tree_helper::IsLoad(TM->GetTreeNode(statement_index), fun_mem_data);
}

bool BehavioralHelper::IsLut(const unsigned int statement_index) const
{
   return tree_helper::IsLut(TM->GetTreeNode(statement_index));
}

void BehavioralHelper::InvaildateVariableName(const unsigned int index)
{
   if(vars_symbol_table.find(index) != vars_symbol_table.end())
   {
      vars_symbol_table.erase(vars_symbol_table.find(index));
   }
}
