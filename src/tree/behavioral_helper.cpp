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
 * @file BehavioralHelper.cpp
 * @brief Helper for reading data from tree_manager
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

/// Autoheader include
#include "config_HAVE_ASSERTS.hpp"               // for HAVE_ASSERTS
#include "config_HAVE_CODE_ESTIMATION_BUILT.hpp" // for HAVE_CODE_ESTIMATIO...
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"     // for HAVE_FROM_PRAGMA_BUILT
#include "config_HAVE_RTL_BUILT.hpp"             // for HAVE_RTL_BUILT
#include "config_HAVE_SPARC_COMPILER.hpp"        // for HAVE_SPARC_COMPILER
#include "config_RELEASE.hpp"                    // for RELEASE

/// Header include
#include "behavioral_helper.hpp"

#include <boost/algorithm/string/replace.hpp> // for replace_all
#include <cstddef>                            // for size_t

#include "custom_map.hpp" // for unordered_map, unor...
#include <string>         // for operator+, string
#include <vector>         // for vector, allocator

/// Behavior include
#include "application_manager.hpp"
#include "cdfg_edge_info.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// Constant include
#if HAVE_FROM_PRAGMA_BUILT
#include "pragma_constants.hpp"
#endif
#include "treegcc_constants.hpp"

/// Parameter include
#include "Parameter.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

#if HAVE_RTL_BUILT
/// RTL include
#include "rtl_node.hpp"
#endif
#if HAVE_CODE_ESTIMATION_BUILT
#include "weight_information.hpp"
#endif

/// Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_reindex.hpp"
#if HAVE_CODE_ESTIMATION_BUILT
#include "weight_information.hpp"
#endif

/// Utility include
#include "boost/lexical_cast.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for STR
#include "var_pp_functor.hpp"

#include "type_casting.hpp"

/// wrapper/treegcc include
#include "gcc_wrapper.hpp"

std::map<std::string, unsigned int> BehavioralHelper::used_name;

std::map<unsigned int, std::string> BehavioralHelper::vars_symbol_table;

std::map<unsigned int, std::string> BehavioralHelper::vars_renaming_table;

/// Max length of a row (at the moment checked only during constructor printing)
#define MAX_ROW_LENGTH 128

BehavioralHelper::BehavioralHelper(const application_managerRef _AppM, unsigned int _index, bool _body, const ParameterConstRef _parameters)
    : AppM(application_managerRef(_AppM.get(), null_deleter())),
      TM(_AppM->get_tree_manager()),
      Param(_parameters),
      debug_level(Param->get_class_debug_level("BehavioralHelper", DEBUG_LEVEL_NONE)),
      function_index(_index),
      function_name(tree_helper::name_function(TM, function_index)),
      body(_body),
      opaque(!_body)
{
}

BehavioralHelper::~BehavioralHelper() = default;

std::tuple<std::string, unsigned int, unsigned int> BehavioralHelper::get_definition(unsigned int index, bool& is_system) const
{
   THROW_ASSERT(index, "expected a meaningful index");
   return tree_helper::get_definition(TM, index, is_system);
}

std::string BehavioralHelper::print_vertex(const OpGraphConstRef g, const vertex v, const var_pp_functorConstRef vppf, bool dot) const
{
   const unsigned int node_id = g->CGetOpNodeInfo(v)->GetNodeId();
   std::string res;
   if(node_id != ENTRY_ID and node_id != EXIT_ID and TM->get_tree_node_const(node_id)->get_kind() != gimple_nop_K)
   {
      res = print_node(node_id, v, vppf);
      const tree_nodeRef node = TM->get_tree_node_const(node_id);

      switch(node->get_kind())
      {
         case(gimple_assign_K):
         case(gimple_call_K):
         case(gimple_asm_K):
         case(gimple_goto_K):
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
      for(char re : res)
      {
         if(re == '\"')
            ret += "\\\"";
         else if(re != '\n')
            ret += re;
         else
            ret += "\\\n";
      }
      ret += "\\n";
#if HAVE_RTL_BUILT && HAVE_CODE_ESTIMATION_BUILT
      if(node_id and TM->get_tree_node_const(node_id)->get_kind() != gimple_nop_K)
      {
         const tree_nodeRef node = TM->get_tree_node_const(node_id);
         if(GetPointer<WeightedNode>(node))
         {
            const WeightedNode* wn = GetPointer<WeightedNode>(node);
            std::list<std::pair<enum rtl_kind, enum mode_kind>>& filtered_rtl_nodes = wn->weight_information->filtered_rtl_nodes;
            std::list<std::pair<enum rtl_kind, enum mode_kind>>::const_iterator rn, rn_end = filtered_rtl_nodes.end();
            for(rn = filtered_rtl_nodes.begin(); rn != rn_end; rn++)
            {
               res += rtl_node::GetString(rn->first) + ":" + rtl_node::GetString(rn->second) + "\\n";
            }
         }
      }
#endif
      return ret;
   }
   else if(res != "")
   {
      res += "\n";
   }
   return res;
}

std::string BehavioralHelper::print_init(unsigned int var, const var_pp_functorConstRef vppf) const
{
   const tree_nodeRef node = TM->get_tree_node_const(var);
   std::string res;
   switch(node->get_kind())
   {
      case constructor_K:
      {
         auto* constr = GetPointer<constructor>(node);
         bool designated_initializers_needed = false;
         res += '{';
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator i = constr->list_of_idx_valu.begin();
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator vend = constr->list_of_idx_valu.end();
         /// check if designated initializers are really needed
         tree_nodeRef firstnode = i != vend ? constr->list_of_idx_valu.begin()->first : tree_nodeRef();
         if(firstnode && GET_NODE(firstnode)->get_kind() == field_decl_K)
         {
            auto* fd = GetPointer<field_decl>(GET_NODE(firstnode));
            tree_nodeRef scpe = GET_NODE(fd->scpe);
            std::vector<tree_nodeRef> field_list;
            if(scpe->get_kind() == record_type_K)
               field_list = GetPointer<record_type>(scpe)->list_of_flds;
            else if(scpe->get_kind() == union_type_K)
               field_list = GetPointer<union_type>(scpe)->list_of_flds;
            else
               THROW_ERROR("expected a record_type or a union_type");
            std::vector<tree_nodeRef>::const_iterator flend = field_list.end();
            std::vector<tree_nodeRef>::const_iterator fli = field_list.begin();
            for(; fli != flend && i != vend; ++i, ++fli)
            {
               if(i->first && GET_INDEX_NODE(i->first) != GET_INDEX_NODE(*fli))
                  break;
            }
            if(fli != flend && i != vend)
               designated_initializers_needed = true;
         }
         else
            designated_initializers_needed = true;

         size_t current_length = res.size();
         for(i = constr->list_of_idx_valu.begin(); i != vend;)
         {
            std::string current;
            if(designated_initializers_needed && i->first && GET_NODE(i->first)->get_kind() == field_decl_K)
            {
               current += ".";
               current += PrintVariable(GET_INDEX_NODE(i->first));
               current += "=";
            }
            tree_nodeRef val = GET_NODE(i->second);
            THROW_ASSERT(val, "Something of unexpected happen");
            if(val->get_kind() == addr_expr_K)
            {
               auto* ae = GetPointer<addr_expr>(val);
               tree_nodeRef op = GET_NODE(ae->op);
               if(op->get_kind() == function_decl_K)
               {
                  val = GET_NODE(ae->op);
                  THROW_ASSERT(val, "Something of unexpected happen");
               }
            }
            if(val->get_kind() == function_decl_K)
            {
               current += tree_helper::print_function_name(TM, GetPointer<function_decl>(val));
            }
            else if(val->get_kind() == constructor_K)
            {
               current += print_init(GET_INDEX_NODE(i->second), vppf);
            }
            else if(val->get_kind() == var_decl_K)
            {
               current += PrintVariable(GET_INDEX_NODE(i->second));
            }
            else if(val->get_kind() == ssa_name_K)
            {
               current += PrintVariable(GET_INDEX_NODE(i->second));
            }
            else
            {
               current += print_node(GET_INDEX_NODE(i->second), vertex(), vppf);
            }
            ++i;
            if(i != vend)
               current += ", ";
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
         res = print_constant(var, vppf);
         break;
      }
      case pointer_plus_expr_K:
      case plus_expr_K:
      {
         vertex dummy_vertex = NULL_VERTEX;
         res += print_node(var, dummy_vertex, vppf);
         break;
      }
      case var_decl_K:
      {
         auto* vd = GetPointer<var_decl>(node);
         THROW_ASSERT(vd->init, "expected a initialization value: " + STR(var));
         res += print_init(GET_INDEX_NODE(vd->init), vppf);
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
      default:
         THROW_ERROR("Currently not supported nodeID " + boost::lexical_cast<std::string>(var));
   }
   return res;
}

std::string BehavioralHelper::print_attributes(unsigned int var, const var_pp_functorConstRef vppf, bool first) const
{
   std::string res;
   if(first)
      res += "__attribute__ (";
   auto* tl = GetPointer<tree_list>(TM->get_tree_node_const(var));
   if(tl->purp)
      res += "(" + PrintVariable(GET_INDEX_NODE(tl->purp));
   if(tl->valu && GET_NODE(tl->valu)->get_kind() == tree_list_K)
   {
      res += print_attributes(GET_INDEX_NODE(tl->valu), vppf, false);
   }
   else if(tl->valu && (GET_NODE(tl->valu)->get_kind() == string_cst_K || GET_NODE(tl->valu)->get_kind() == integer_cst_K))
   {
      res += "(" + print_constant(GET_INDEX_NODE(tl->valu)) + ")";
   }
   else if(tl->valu)
      THROW_ERROR("Not yet supported: " + std::string(GET_NODE(tl->valu)->get_kind_text()));
   if(tl->purp)
      res += ")";
   if(tl->chan)
   {
      res += " " + print_attributes(GET_INDEX_NODE(tl->chan), vppf, false);
   }
   if(first)
      res += ")";
   return res;
}

std::string BehavioralHelper::PrintVariable(unsigned int var) const
{
   if(vars_renaming_table.find(var) != vars_renaming_table.end())
      return vars_renaming_table.find(var)->second;
   if(vars_symbol_table[var] != "")
   {
      return vars_symbol_table[var];
   }
   if(var == default_COND)
      return "default";
   const tree_nodeRef temp = TM->get_tree_node_const(var);
   if(temp->get_kind() == indirect_ref_K)
   {
      auto* ir = GetPointer<indirect_ref>(temp);
      unsigned int pointer = GET_INDEX_NODE(ir->op);
      std::string pointer_name = PrintVariable(pointer);
      vars_symbol_table[var] = "*" + pointer_name;
      return vars_symbol_table[var];
   }
   if(temp->get_kind() == misaligned_indirect_ref_K)
   {
      auto* mir = GetPointer<misaligned_indirect_ref>(temp);
      unsigned int pointer = GET_INDEX_NODE(mir->op);
      std::string pointer_name = PrintVariable(pointer);
      vars_symbol_table[var] = "*" + pointer_name;
      return vars_symbol_table[var];
   }
   if(temp->get_kind() == mem_ref_K)
   {
      auto* mr = GetPointer<mem_ref>(temp);
      long long int offset = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(mr->op1)));
      tree_managerRef temp_TM(const_cast<tree_manager*>(TM.get()), null_deleter());
      const tree_manipulationRef tm(new tree_manipulation(temp_TM, Param));
      const unsigned int pointer_type = tm->create_pointer_type(mr->type, 8)->index;
      const std::string type_string = tree_helper::print_type(TM, pointer_type);
      if(offset == 0)
         vars_symbol_table[var] = "*((" + type_string + ")(" + PrintVariable(GET_INDEX_NODE(mr->op0)) + "))";
      else
         vars_symbol_table[var] = "*((" + type_string + ")(((unsigned char*)" + PrintVariable(GET_INDEX_NODE(mr->op0)) + ") + " + boost::lexical_cast<std::string>(offset) + "))";
      return vars_symbol_table[var];
   }
   if(temp->get_kind() == identifier_node_K)
   {
      auto* in = GetPointer<identifier_node>(temp);
      vars_symbol_table[var] = in->strg;
      return vars_symbol_table[var];
   }
   if(temp->get_kind() == field_decl_K)
   {
      auto* fd = GetPointer<field_decl>(temp);
      if(fd->name)
      {
         auto* id = GetPointer<identifier_node>(GET_NODE(fd->name));
         return tree_helper::normalized_ID(id->strg);
      }
      else
      {
         return INTERNAL + boost::lexical_cast<std::string>(var);
      }
   }
   if(temp->get_kind() == function_decl_K)
   {
      auto* fd = GetPointer<function_decl>(temp);
      return tree_helper::print_function_name(TM, fd);
   }
   if(temp->get_kind() == ssa_name_K)
   {
      auto* sa = GetPointer<ssa_name>(temp);
      std::string name;
      if(sa->var)
      {
         unsigned int ssa_index = GET_INDEX_NODE(sa->var);
         name = PrintVariable(ssa_index);
      }
      THROW_ASSERT(sa->volatile_flag or sa->CGetDefStmts().size(), sa->ToString() + " has not define statement");
      if(sa->virtual_flag || (!sa->volatile_flag && GET_NODE(sa->CGetDefStmt())->get_kind() != gimple_nop_K))
      {
         name += ("_" + boost::lexical_cast<std::string>(sa->vers));
      }
      else
      {
         THROW_ASSERT(sa->var, "the name has to be defined for volatile or parameters");
      }
      // if(sa->min && sa->max) name += "/*[" + print_constant(GET_INDEX_NODE(sa->min)) + "," + print_constant(GET_INDEX_NODE(sa->max)) + "]*/";
      return name;
   }
   if(temp->get_kind() == var_decl_K || temp->get_kind() == parm_decl_K)
   {
      auto* dn = GetPointer<decl_node>(temp);
      if(dn->name)
      {
         auto* id = GetPointer<identifier_node>(GET_NODE(dn->name));
         vars_symbol_table[var] = tree_helper::normalized_ID(id->strg);
         return vars_symbol_table[var];
      }
   }
   if(is_a_constant(var))
      vars_symbol_table[var] = print_constant(var);
   if(vars_symbol_table[var] == "")
      vars_symbol_table[var] = INTERNAL + boost::lexical_cast<std::string>(var);
   return vars_symbol_table[var];
}

std::string BehavioralHelper::print_constant(unsigned int var, const var_pp_functorConstRef vppf) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Printing constant " + boost::lexical_cast<std::string>(var));
   THROW_ASSERT(is_a_constant(var), std::string("Object is not a constant ") + boost::lexical_cast<std::string>(var));
   if(var == default_COND)
      return "default";
   std::string res;
   const tree_nodeRef node = TM->get_tree_node_const(var);
   switch(node->get_kind())
   {
      case integer_cst_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->integer_cst");
         auto* ic = GetPointer<integer_cst>(node);
         tree_nodeRef type = GET_NODE(ic->type);
         auto* it = GetPointer<integer_type>(type);
         bool unsigned_flag = (it && it->unsigned_flag) || type->get_kind() == pointer_type_K || type->get_kind() == reference_type_K || type->get_kind() == boolean_type_K;
#if 0
         ///check if the IR type is consistent with the type name
         if (it)
         {
            std::string predicted_type;
            if (it->unsigned_flag)
               predicted_type += "unsigned ";
            if (it->algn == 8)
               predicted_type += "char";
            else if (it->algn == 16)
               predicted_type += "short";
            else if (it->algn == 32)
               predicted_type += "int";
            else if (it->algn == 64)
               predicted_type += "long long int";
            std::string actual_type = tree_helper::print_type(TM, GET_INDEX_NODE(ic->type));
            if (predicted_type != actual_type && actual_type != "bit_size_type")
               res = "(" + actual_type + ")";
         }
#endif
         THROW_ASSERT(ic, "");
         long long value = tree_helper::get_integer_cst_value(ic);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Value is " + boost::lexical_cast<std::string>(value));
         if((it && it->prec == 64) && (value == (static_cast<long long int>(-0x08000000000000000LL))))
            res = "(long long int)-0x08000000000000000";
         else if((it && it->prec == 32) && (value == (static_cast<long int>(-0x080000000L))))
            res = "(long int)-0x080000000";
         else
         {
            if(it && it->unsigned_flag)
            {
               if(it && it->prec < 64)
                  res += boost::lexical_cast<std::string>(static_cast<unsigned long long>(value) & ((1ull << it->prec) - 1));
               else
                  res += boost::lexical_cast<std::string>(static_cast<unsigned long long>(value));
            }
            else
               res += boost::lexical_cast<std::string>(value);
         }
         if(it && it->prec > 32)
         {
            if(it && it->unsigned_flag)
               res += "LLU";
            else
               res += "LL";
         }
         else
         {
            if(unsigned_flag)
               res += "u";
            else if(type->get_kind() == pointer_type_K || type->get_kind() == reference_type_K)
               res += "/*B*/";
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         break;
      }
      case real_cst_K:
      {
         auto* rc = GetPointer<real_cst>(node);
         if(rc->overflow_flag)
            res += " overflow";
         tree_nodeRef type = GET_NODE(rc->type);
         if(type->get_kind() == real_type_K)
         {
            if(GetPointer<real_type>(type)->prec == 80) /// long double
            {
               if(strcasecmp(rc->valr.data(), "Inf") == 0)
               {
                  if(rc->valx[0] == '-')
                     res += "-";
                  res += "__builtin_infl()";
               }
               else if(strcasecmp(rc->valr.data(), "Nan") == 0)
               {
                  if(rc->valx[0] == '-')
                     res += "-";
                  res += "__builtin_nanl(\"\")";
               }
               else
               {
                  res += rc->valr;
                  res += "L";
               }
            }
            else if(GetPointer<real_type>(type)->prec == 64) /// double
            {
               if(strcasecmp(rc->valr.data(), "Inf") == 0)
               {
                  if(rc->valx[0] == '-')
                     res += "-";
                  res += "__builtin_inf()";
               }
               else if(strcasecmp(rc->valr.data(), "Nan") == 0)
               {
                  if(rc->valx[0] == '-')
                     res += "-";
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
                  res += "-";
               res += "__builtin_inff()";
            }
            else if(strcasecmp(rc->valr.data(), "Nan") == 0)
            {
               if(rc->valx[0] == '-')
                  res += "-";
               res += "__builtin_nanf(\"\")";
            }
            else
               /// FIXME: float can not be used for imaginary part of complex number
               res += /*"(float)" +*/ rc->valr;
         }
         else
            THROW_ERROR(std::string("Node not yet supported: ") + node->get_kind_text());
         break;
      }
      case complex_cst_K:
      {
         auto* cc = GetPointer<complex_cst>(node);

         res += print_constant(GET_INDEX_NODE(cc->real), vppf);
         res += "+";
         res += print_constant(GET_INDEX_NODE(cc->imag), vppf);
         res += "*1i";
         break;
      }
      case string_cst_K:
      {
         auto* sc = GetPointer<string_cst>(node);
         if(sc->type)
         {
            auto* at = GetPointer<array_type>(GET_NODE(sc->type));
            THROW_ASSERT(at, "Expected an array type");
            auto* elts = GetPointer<integer_type>(GET_NODE(at->elts));
            if(elts->prec == 32) // wide char string
            {
               if(elts->unsigned_flag and not tree_helper::is_system(TM, GET_INDEX_NODE(at->elts)))
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
         auto* vc = GetPointer<vector_cst>(node);
         THROW_ASSERT(GET_NODE(vc->type)->get_kind() == vector_type_K, "Vector constant of type " + GET_NODE(vc->type)->get_kind_text());
         if(GET_NODE(GetPointer<const vector_type>(GET_NODE(vc->type))->elts)->get_kind() != pointer_type_K)
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vc->type), false, true) + ") ";
         res += "{ ";
         for(unsigned int i = 0; i < (vc->list_of_valu).size(); i++) // vector elements
         {
            res += print_constant(GET_INDEX_NODE(vc->list_of_valu[i]), vppf);
            if(i != (vc->list_of_valu).size() - 1) // not the last element element
               res += ", ";
         }
         res += " }";
         break;
      }
      case case_label_expr_K:
      {
         auto* cl = GetPointer<case_label_expr>(node);
         if(cl->default_flag)
            res += "default";
         else
         {
            if(cl->op1)
            {
               THROW_ASSERT(GetPointer<integer_cst>(GET_NODE(cl->op0)), "Case label expression " + boost::lexical_cast<std::string>(var) + " does not use integer_cst");
               THROW_ASSERT(GetPointer<integer_cst>(GET_NODE(cl->op1)), "Case label expression " + boost::lexical_cast<std::string>(var) + " does not use integer_cst");
               long long low = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(cl->op0)));
               long long high = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(cl->op1)));
               while(low < high)
               {
                  res += boost::lexical_cast<std::string>(low) + "u : case ";
                  low++;
               }
               res += boost::lexical_cast<std::string>(low) + "u";
            }
            else
               res += print_constant(GET_INDEX_NODE(cl->op0), vppf);
         }
         break;
      }
      case label_decl_K:
      {
         auto* ld = GetPointer<label_decl>(node);
         THROW_ASSERT(ld->name, "name expected in a label_decl");
         auto* id = GetPointer<identifier_node>(GET_NODE(ld->name));
         THROW_ASSERT(id, "expected an identifier_node");
         res += id->strg;
         break;
      }
      case nop_expr_K:
      {
         auto* ue = GetPointer<unary_expr>(node);
         if(!(GET_NODE(ue->op)->get_kind() == addr_expr_K && GET_NODE(GetPointer<addr_expr>(GET_NODE(ue->op))->op)->get_kind() == label_decl_K))
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(ue->type)) + ") ";
         res += print_constant(GET_INDEX_NODE(ue->op), vppf);
         break;
      }
      case addr_expr_K:
      {
         auto* ue = GetPointer<unary_expr>(node);
         if(is_a_constant(GET_INDEX_NODE(ue->op)))
         {
            res += "(&(" + print_constant(GET_INDEX_NODE(ue->op)) + "))";
         }
         else if(GET_NODE(ue->op)->get_kind() == function_decl_K)
            res += tree_helper::print_function_name(TM, GetPointer<function_decl>(GET_NODE(ue->op)));
         else
         {
            if(vppf)
               res += "(&(" + (*vppf)(GET_INDEX_NODE(ue->op)) + "))";
            else
               res += "(&(" + PrintVariable(GET_INDEX_NODE(ue->op)) + "))";
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
         THROW_ERROR("Var object is not a constant " + boost::lexical_cast<std::string>(var) + " " + node->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Printed constant " + res);
   return res;
}

unsigned int BehavioralHelper::get_size(unsigned int var) const
{
   return tree_helper::size(TM, var);
}

std::string BehavioralHelper::get_function_name() const
{
   return function_name;
}

unsigned int BehavioralHelper::get_function_index() const
{
   return function_index;
}

unsigned int BehavioralHelper::GetFunctionReturnType(unsigned int function) const
{
   const tree_nodeRef return_type = tree_helper::GetFunctionReturnType(TM->get_tree_node_const(function));
   if(return_type)
      return return_type->index;
   else
      return 0;
}

const std::list<unsigned int> BehavioralHelper::get_parameters() const
{
   std::list<unsigned int> parameters;
   tree_nodeRef fun = TM->get_tree_node_const(function_index);
   auto* fd = GetPointer<function_decl>(fun);
   const std::vector<tree_nodeRef>& list_of_args = fd->list_of_args;
   if(fd->list_of_args.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter list size: " + STR(list_of_args.size()));
      for(const auto& list_of_arg : list_of_args)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding parameter " + STR(GET_INDEX_NODE(list_of_arg)));
         parameters.push_back(GET_INDEX_NODE(list_of_arg));
      }
   }
   else
   {
      auto* ft = GetPointer<function_type>(GET_NODE(fd->type));
      if(ft->prms)
      {
         tree_nodeRef currentp = ft->prms;
         while(currentp)
         {
            auto* tl = GetPointer<tree_list>(GET_NODE(currentp));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding parameter " + STR(GET_INDEX_NODE(tl->valu)));
            if(GET_NODE(tl->valu)->get_kind() != void_type_K)
               parameters.push_back(GET_INDEX_NODE(tl->valu));
            currentp = tl->chan;
         }
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
   return tree_helper::get_pointed_type(TM, type);
}

unsigned int BehavioralHelper::GetElements(const unsigned int type) const
{
   return tree_helper::GetElements(TM, type);
}

bool BehavioralHelper::isCallExpression(unsigned int nodeID) const
{
   // get the tree node associated with the provided ID
   const tree_nodeRef node = TM->get_tree_node_const(nodeID);
   if(node->get_kind() == call_expr_K || node->get_kind() == aggr_init_expr_K)
      return true;
   else
      return false;
}

unsigned int BehavioralHelper::getCallExpressionIndex(std::string launch_code) const
{
   std::string temp = launch_code;
   size_t pos = temp.find('(');
   temp.erase(pos);

   // now 'temp' variable contains the called-function name
   unsigned int index = TM->function_index(temp);
   return index;
}

std::string BehavioralHelper::PrintVarDeclaration(unsigned int var, var_pp_functorConstRef vppf, bool init_has_to_be_printed) const
{
   std::string return_value;
   const tree_nodeRef& curr_tn = TM->get_tree_node_const(var);
   THROW_ASSERT(GetPointer<decl_node>(curr_tn) || GetPointer<ssa_name>(curr_tn), "Call pparameter_type_indexrint_var_declaration on node " + boost::lexical_cast<std::string>(var) + " which is of type " + curr_tn->get_kind_text());
   decl_node* dn = nullptr;
   if(GetPointer<decl_node>(curr_tn))
      dn = GetPointer<decl_node>(curr_tn);
   /// If it is not a decl node (then it is an ssa-name) or it's a not system decl_node
   if(!dn or !(dn->operating_system_flag or dn->library_system_flag)
#if HAVE_BAMBU_BUILT
      or tree_helper::IsInLibbambu(TM, var)
#endif
   )
   {
      return_value += tree_helper::print_type(TM, get_type(var), false, false, init_has_to_be_printed, var, vppf);
      unsigned int attributes = get_attributes(var);
      CustomUnorderedSet<unsigned int> list_of_variables;
      const unsigned int init = GetInit(var, list_of_variables);
      if(attributes)
         return_value += " " + print_attributes(attributes, vppf);
      if(dn and dn->packed_flag)
         return_value += " __attribute__((packed))";
      if(dn and ((dn->orig and GetPointer<var_decl>(curr_tn) and GetPointer<var_decl>(GET_NODE(dn->orig)) and GetPointer<var_decl>(curr_tn)->algn != GetPointer<var_decl>(GET_NODE(dn->orig))->algn) ||
                 (GetPointer<var_decl>(curr_tn) and GetPointer<var_decl>(curr_tn)->algn == 128)))
      {
         return_value += " __attribute__ ((aligned (" + boost::lexical_cast<std::string>(GetPointer<var_decl>(curr_tn)->algn / 8) + "))) ";
      }
      if(init && init_has_to_be_printed)
         return_value += " = " + print_init(init, vppf);
   }
   return return_value;
}

unsigned int BehavioralHelper::is_coming_from_phi_node(unsigned int nodeID) const
{
   tree_nodeRef tn = TM->get_tree_node_const(nodeID);
   auto* gms = GetPointer<gimple_assign>(tn);
   if(gms && gms->orig)
      return GET_INDEX_NODE(gms->orig);
   else
      return 0;
}

bool BehavioralHelper::is_var_args() const
{
   tree_nodeRef tn = TM->get_tree_node_const(function_index);
   THROW_ASSERT(tn->get_kind() == function_decl_K, "function_index is not a function decl");
   tn = GET_NODE(GetPointer<function_decl>(tn)->type);
   return GetPointer<function_type>(tn)->varargs_flag;
}

std::string BehavioralHelper::print_node(unsigned int index, vertex v, const var_pp_functorConstRef vppf) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Printing node " + boost::lexical_cast<std::string>(index));
   std::string res = "";
   const tree_nodeRef node = TM->get_tree_node_const(index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Type is " + std::string(node->get_kind_text()));
   switch(node->get_kind())
   {
         /* Binary arithmetic and logic expressions.  */
      case plus_expr_K:
      {
         const std::string op = tree_helper::op_symbol(node);
         auto* be = GetPointer<binary_expr>(node);
         unsigned int left_op = GET_INDEX_NODE(be->op0);
         unsigned int right_op = GET_INDEX_NODE(be->op1);
         unsigned int left_op_type_index = 0;
         tree_helper::get_type_node(GET_NODE(be->op0), left_op_type_index);
         unsigned int right_op_type_index = 0;
         tree_helper::get_type_node(GET_NODE(be->op1), right_op_type_index);
         bool vector = tree_helper::is_a_vector(TM, GET_INDEX_NODE(be->type)) and
                       ((tree_helper::is_a_vector(TM, left_op_type_index) and left_op_type_index != GET_INDEX_NODE(be->type)) or (tree_helper::is_a_vector(TM, right_op_type_index) && right_op_type_index != GET_INDEX_NODE(be->type)));
         if(vector)
         {
            const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(be->type));
            const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
            const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(be->type)));
            const unsigned int vector_size = size / element_size;
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(be->type)) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + print_node(GET_INDEX_NODE(be->op0), v, vppf) + ")[" + STR(ind) + "] + (" + print_node(GET_INDEX_NODE(be->op1), v, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
                  res += ", ";
            }
            res += "}";
         }
         else
         {
            unsigned int prec = 0;
            unsigned int return_index;
            const tree_nodeRef type = tree_helper::get_type_node(node, return_index);
            if(type && (type->get_kind() == integer_type_K))
               prec = GetPointer<integer_type>(type)->prec;
            unsigned int algn = 0;
            if(type && (type->get_kind() == integer_type_K))
               algn = GetPointer<integer_type>(type)->algn;
            // bitfield type
            if(prec != algn && prec % algn)
            {
               res += "((";
            }
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(be->type)) + ")(";

            if(GetPointer<decl_node>(GET_NODE(be->op0)) || GetPointer<ssa_name>(GET_NODE(be->op0)))
               res += print_node(left_op, v, vppf);
            else
               res += ("(" + print_node(left_op, v, vppf) + ")");
            res += std::string(" ") + op + " ";

            if(GetPointer<decl_node>(GET_NODE(be->op1)) || GetPointer<ssa_name>(GET_NODE(be->op1)))
               res += print_node(right_op, v, vppf);
            else
               res += ("(" + print_node(right_op, v, vppf) + ")");
            res += ")";
            if(prec != algn && prec % algn)
            {
               res += ")%(1";
               if(prec > 32)
                  res += "LL";
               if(GetPointer<integer_type>(type)->unsigned_flag)
                  res += "U";
               res += " << " + boost::lexical_cast<std::string>(prec) + "))";
            }
         }
         break;
      }
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case bit_and_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         unsigned int left_op_type_index = 0;
         tree_helper::get_type_node(GET_NODE(be->op0), left_op_type_index);
         unsigned int right_op_type_index = 0;
         tree_helper::get_type_node(GET_NODE(be->op1), right_op_type_index);
         const std::string op = tree_helper::op_symbol(node);
         bool vector = tree_helper::is_a_vector(TM, GET_INDEX_NODE(be->type)) and
                       ((tree_helper::is_a_vector(TM, left_op_type_index) and left_op_type_index != GET_INDEX_NODE(be->type)) or (tree_helper::is_a_vector(TM, right_op_type_index) && right_op_type_index != GET_INDEX_NODE(be->type)));
         if(vector)
         {
            const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(be->type));
            const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
            const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(be->type)));
            const unsigned int vector_size = size / element_size;
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(be->type)) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + print_node(GET_INDEX_NODE(be->op0), v, vppf) + ")[" + STR(ind) + "] " + op + " (" + print_node(GET_INDEX_NODE(be->op1), v, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
                  res += ", ";
            }
            res += "}";
         }
         else
         {
            unsigned int left_op = GET_INDEX_NODE(be->op0);
            unsigned int right_op = GET_INDEX_NODE(be->op1);
            unsigned int prec = 0;
            unsigned int return_index;
            const tree_nodeRef type = tree_helper::get_type_node(node, return_index);
            bool bit_expression = type && type->get_kind() == pointer_type_K;

            if(type && (type->get_kind() == integer_type_K))
               prec = GetPointer<integer_type>(type)->prec;
            unsigned int algn = 0;
            if(type && (type->get_kind() == integer_type_K))
               algn = GetPointer<integer_type>(type)->algn;
            // bitfield type
            if(prec != algn && prec % algn)
            {
               res += "((";
            }
            if(bit_expression)
               res += "((" + tree_helper::print_type(TM, get_type(index)) + ")(((unsigned)(";

            if(GetPointer<decl_node>(GET_NODE(be->op0)) || GetPointer<ssa_name>(GET_NODE(be->op0)))
               res += print_node(left_op, v, vppf);
            else
               res += ("(" + print_node(left_op, v, vppf) + ")");
            if(bit_expression)
               res += "))";
            res += std::string(" ") + op + " ";
            if(bit_expression)
               res += "((unsigned)(";

            if(GetPointer<decl_node>(GET_NODE(be->op1)) || GetPointer<ssa_name>(GET_NODE(be->op1)))
               res += print_node(right_op, v, vppf);
            else
               res += ("(" + print_node(right_op, v, vppf) + ")");
            if(bit_expression)
               res += "))))";
            if(prec != algn && prec % algn)
            {
               res += ")%(1";
               if(prec > 32)
                  res += "LL";
               if(GetPointer<integer_type>(type)->unsigned_flag)
                  res += "U";
               res += " << " + boost::lexical_cast<std::string>(prec) + "))";
            }
         }
         break;
      }
      case vec_rshift_expr_K:
      {
         auto* vre = GetPointer<vec_rshift_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vre->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vre->type)));
         const unsigned int vector_size = size / element_size;
         res += "/*" + vre->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vre->type)) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            for(unsigned int k = ind; k < vector_size; ++k)
            {
               res += "((" + print_node(GET_INDEX_NODE(vre->op1), v, vppf) + " >= " + STR(element_size * (k - ind)) + "&&" + print_node(GET_INDEX_NODE(vre->op1), v, vppf) + " < " + STR(element_size * (k - ind + 1)) + ") ? ((" +
                      print_node(GET_INDEX_NODE(vre->op0), v, vppf) + ")[" + boost::lexical_cast<std::string>(k) + "]) >> (" + print_node(GET_INDEX_NODE(vre->op1), v, vppf) + "-" + STR(element_size * (k - ind)) + "): 0)";
               if(k != vector_size - 1)
                  res += "|";
            }

            for(unsigned int k = ind + 1; k < vector_size; ++k)
            {
               res += "| ((" + print_node(GET_INDEX_NODE(vre->op1), v, vppf) + " > " + STR(element_size * (k - 1 - ind)) + "&&" + print_node(GET_INDEX_NODE(vre->op1), v, vppf) + " < " + STR(element_size * (k - ind)) + ") ? ((" +
                      print_node(GET_INDEX_NODE(vre->op0), v, vppf) + ")[" + boost::lexical_cast<std::string>(k) + "]) << (" + STR(element_size * (k - ind)) + "-" + print_node(GET_INDEX_NODE(vre->op1), v, vppf) + "): 0)";
            }
            if(ind != vector_size - 1)
               res += ", ";
         }
         res += "}";
         break;
      }
      case vec_lshift_expr_K:
      {
         auto* vle = GetPointer<vec_lshift_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vle->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vle->type)));
         const unsigned int vector_size = size / element_size;
         res += "/*" + vle->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vle->type)) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            for(unsigned int k = 0; k <= ind; ++k)
            {
               res += "((" + print_node(GET_INDEX_NODE(vle->op1), v, vppf) + " >= " + STR(element_size * (k)) + "&&" + print_node(GET_INDEX_NODE(vle->op1), v, vppf) + " < " + STR(element_size * (k + 1)) + ") ? ((" +
                      print_node(GET_INDEX_NODE(vle->op0), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind - k) + "]) << (" + print_node(GET_INDEX_NODE(vle->op1), v, vppf) + "-" + STR(element_size * k) + "): 0)";
               if(k != ind)
                  res += "|";
            }
            for(unsigned int k = 0; k < ind; ++k)
            {
               res += "| ((" + print_node(GET_INDEX_NODE(vle->op1), v, vppf) + " > " + STR(element_size * (k)) + "&&" + print_node(GET_INDEX_NODE(vle->op1), v, vppf) + " < " + STR(element_size * (k + 1)) + ") ? ((" +
                      print_node(GET_INDEX_NODE(vle->op0), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind - k - 1) + "]) >> (" + STR(element_size * (k + 1)) + "-" + print_node(GET_INDEX_NODE(vle->op1), v, vppf) + "): 0)";
            }
            if(ind != vector_size - 1)
               res += ", ";
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
         const std::string op = tree_helper::op_symbol(node);
         auto* be = GetPointer<binary_expr>(node);
         unsigned int left_op = GET_INDEX_NODE(be->op0);
         unsigned int right_op = GET_INDEX_NODE(be->op1);
         unsigned int left_op_type_index = 0;
         tree_helper::get_type_node(GET_NODE(be->op0), left_op_type_index);
         unsigned int right_op_type_index = 0;
         tree_helper::get_type_node(GET_NODE(be->op1), right_op_type_index);
         bool vector = tree_helper::is_a_vector(TM, GET_INDEX_NODE(be->type)) and
                       ((tree_helper::is_a_vector(TM, left_op_type_index) and left_op_type_index != GET_INDEX_NODE(be->type)) or (tree_helper::is_a_vector(TM, right_op_type_index) && right_op_type_index != GET_INDEX_NODE(be->type)));
         if(vector)
         {
            const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(be->type));
            const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
            const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(be->type)));
            const unsigned int vector_size = size / element_size;
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(be->type)) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + print_node(GET_INDEX_NODE(be->op0), v, vppf) + ")[" + STR(ind) + "] " + op + " (" + print_node(GET_INDEX_NODE(be->op1), v, vppf) + ")";
               if(GET_CONST_NODE(be->op1)->get_kind() != integer_cst_K)
                  res += "[" + STR(ind) + "]";
               if(ind != vector_size - 1)
                  res += ", ";
            }
            res += "}";
         }
         else
         {
            unsigned int prec = 0;
            unsigned int return_index;
            const tree_nodeRef type = tree_helper::get_type_node(node, return_index);
            if(type && (type->get_kind() == integer_type_K))
               prec = GetPointer<integer_type>(type)->prec;
            unsigned int algn = 0;
            if(type && (type->get_kind() == integer_type_K))
               algn = GetPointer<integer_type>(type)->algn;
            // bitfield type
            if(prec != algn && prec % algn)
            {
               res += "((";
            }
            if((node->get_kind() == lshift_expr_K or node->get_kind() == rshift_expr_K) and left_op_type_index != GET_INDEX_NODE(be->type))
               res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(be->type)) + ")(";
            if(GetPointer<decl_node>(GET_NODE(be->op0)) || GetPointer<ssa_name>(GET_NODE(be->op0)))
               res += print_node(left_op, v, vppf);
            else
               res += ("(" + print_node(left_op, v, vppf) + ")");
            if((node->get_kind() == lshift_expr_K or node->get_kind() == rshift_expr_K) and left_op_type_index != GET_INDEX_NODE(be->type))
               res += ")";
            res += std::string(" ") + op + " ";
            if(GetPointer<decl_node>(GET_NODE(be->op1)) || GetPointer<ssa_name>(GET_NODE(be->op1)))
               res += print_node(right_op, v, vppf);
            else
               res += ("(" + print_node(right_op, v, vppf) + ")");
            if(prec != algn && prec % algn)
            {
               res += ")%(1";
               if(prec > 32)
                  res += "LL";
               if(GetPointer<integer_type>(type)->unsigned_flag)
                  res += "U";
               res += " << " + boost::lexical_cast<std::string>(prec) + "))";
            }
         }
         break;
      }
      case widen_sum_expr_K:
      case widen_mult_expr_K:
      {
         const std::string op = tree_helper::op_symbol(node);
         auto* be = GetPointer<binary_expr>(node);
         unsigned int left_op = GET_INDEX_NODE(be->op0);
         unsigned int right_op = GET_INDEX_NODE(be->op1);
         unsigned int return_index;
         tree_helper::get_type_node(node, return_index);

         res += "((" + tree_helper::print_type(TM, return_index) + ")(" + print_node(left_op, v, vppf) + "))";
         res += std::string(" ") + op + " ";
         res += "((" + tree_helper::print_type(TM, return_index) + ")(" + print_node(right_op, v, vppf) + "))";
         break;
      }
      case extract_bit_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         unsigned int left_op = GET_INDEX_NODE(be->op0);
         unsigned int right_op = GET_INDEX_NODE(be->op1);
         res += "(_Bool)(((unsigned long long int)(" + print_node(left_op, v, vppf);
         res += std::string(") >> ");
         res += print_node(right_op, v, vppf) + ") & 1)";
         break;
      }
      case pointer_plus_expr_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         bool binary_op_cast = is_a_pointer(index);
         const std::string op = tree_helper::op_symbol(node);
         auto* ppe = GetPointer<pointer_plus_expr>(node);
         unsigned int temp = 0;
         tree_nodeRef type_node = tree_helper::get_type_node(GET_NODE(ppe->op0), temp);
         unsigned int left_op = GET_INDEX_NODE(ppe->op0);
         unsigned int right_op = GET_INDEX_NODE(ppe->op1);
         const bool left_op_cast = is_a_pointer(left_op);
#ifndef NDEBUG
         if(left_op_cast)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Left part is a pointer");
         }
#endif
#if HAVE_ASSERTS
         bool right_op_cast = is_a_pointer(right_op);
#endif
         bool do_reverse_pointer_arithmetic = false;
         tree_nodeRef right_op_node = GET_NODE(ppe->op1);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting right op node is " + boost::lexical_cast<std::string>(GET_INDEX_NODE(ppe->op1)) + " - " + right_op_node->get_kind_text());
         bool right_cost = right_op_node->get_kind() == integer_cst_K;
         THROW_ASSERT(!right_op_cast, "expected a right operand different from a pointer");
         THROW_ASSERT(GET_NODE(ppe->type)->get_kind() == pointer_type_K, "expected a pointer type");

         /// check possible pointer arithmetic reverse
         long long int deltabit;
         unsigned int pointed_type_index = get_pointed_type(tree_helper::get_type_index(TM, left_op));
         tree_nodeRef pointed_type = TM->get_tree_node_const(pointed_type_index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Pointed type (" + boost::lexical_cast<std::string>(get_pointed_type(tree_helper::get_type_index(TM, left_op))) + ") is " + pointed_type->get_kind_text());
         if(pointed_type->get_kind() == void_type_K)
         {
            auto* vt = GetPointer<void_type>(pointed_type);
            deltabit = vt->algn;
         }
         else
         {
            deltabit = tree_helper::size(TM, pointed_type_index);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "deltabit is " + STR(deltabit));
         long long int pointer_offset = 0;
         std::string right_offset_var;
         if(right_cost)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Offset is constant");
            pointer_offset = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(right_op_node));
            if(deltabit / 8 == 0)
               do_reverse_pointer_arithmetic = false;
            else if(pointed_type->get_kind() != array_type_K && deltabit and pointer_offset > deltabit && ((pointer_offset % (deltabit / 8)) == 0))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Arithmetic pointer pattern matched");
               auto* ic = GetPointer<integer_cst>(right_op_node);
               tree_nodeRef ic_type = GET_NODE(ic->type);
               auto* it = GetPointer<integer_type>(ic_type);
               if(it && (it->prec == 32))
               {
                  pointer_offset = static_cast<int>(pointer_offset) / static_cast<int>(deltabit / 8);
                  pointer_offset = static_cast<unsigned int>(pointer_offset);
               }
               else
               {
                  pointer_offset = pointer_offset / (deltabit / 8);
               }
               do_reverse_pointer_arithmetic = true;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Arithmetic pointer pattern not matched " + boost::lexical_cast<std::string>(pointer_offset) + " vs " + boost::lexical_cast<std::string>(deltabit / 8));
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Offset is not constant");
            bool exit = pointed_type->get_kind() == array_type_K;
            while(!do_reverse_pointer_arithmetic and !exit)
            {
               switch(right_op_node->get_kind())
               {
                  case(ssa_name_K):
                  {
                     if(GetPointer<ssa_name>(right_op_node)->CGetDefStmts().size() == 0)
                     {
                        exit = true;
                        break;
                     }
                     auto* rssa = GetPointer<ssa_name>(right_op_node);
                     tree_nodeRef defstmt = GET_NODE(rssa->CGetDefStmt());
                     if(defstmt->get_kind() != gimple_assign_K)
                     {
                        exit = true;
                        break;
                     }
                     right_op_node = GET_NODE(GetPointer<gimple_assign>(defstmt)->op1);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "New op node is " + boost::lexical_cast<std::string>(GET_INDEX_NODE(GetPointer<gimple_assign>(defstmt)->op1)) + " - " + right_op_node->get_kind_text());
                     break;
                  }
                  case mult_highpart_expr_K:
                  {
                     THROW_UNREACHABLE("");
                     break;
                  }
                  case(mult_expr_K):
                  {
                     auto* mult = GetPointer<mult_expr>(right_op_node);
                     if(GET_NODE(mult->op1)->get_kind() == integer_cst_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Right part of multiply is an integer constant " + boost::lexical_cast<std::string>(GET_INDEX_NODE(mult->op1)));
                        long long size_of_pointer = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(mult->op1)));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Size of pointer is " + boost::lexical_cast<std::string>(size_of_pointer));
                        if(size_of_pointer == (deltabit / 8))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Constant is the size of the pointed");
                           right_offset_var += print_node(GET_INDEX_NODE(mult->op0), v, vppf);
                           do_reverse_pointer_arithmetic = true;
                        }
                        else
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Constant is not the size of the pointed: " + boost::lexical_cast<std::string>(size_of_pointer) + " vs " + boost::lexical_cast<std::string>(deltabit / 8));
                           unsigned int integer_type_index = 0;
                           tree_nodeRef temp1 = tree_helper::get_type_node(GET_NODE(mult->op1), integer_type_index);
                           THROW_ASSERT(GetPointer<integer_type>(temp1), "Type of integer cast " + boost::lexical_cast<std::string>(GET_INDEX_NODE(mult->op1)) + " is not an integer type");
                           const integer_type* it = GetPointer<integer_type>(temp1);
                           auto max_int = static_cast<unsigned long long>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->max))));
                           long long new_size_of_pointer = static_cast<long long>(max_int) + 1 - size_of_pointer;
                           if(new_size_of_pointer == (deltabit / 8))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Constant is minus the size of the pointed");
                              right_offset_var += "-(" + print_node(GET_INDEX_NODE(mult->op0), v, vppf) + ")";
                              do_reverse_pointer_arithmetic = true;
                           }
                           else if(deltabit and (deltabit / 8) and (size_of_pointer % ((deltabit / 8)) == 0))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Constant is a multiple of the  size of the pointed");
                              right_offset_var += print_node(GET_INDEX_NODE(mult->op0), v, vppf);
                              right_offset_var += " * ";
                              right_offset_var += boost::lexical_cast<std::string>(size_of_pointer / ((deltabit / 8)));
                              do_reverse_pointer_arithmetic = true;
                           }
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Right offset variable is " + right_offset_var);
                     }
                     else if(GET_NODE(mult->op0)->get_kind() == integer_cst_K)
                     {
                        long long size_of_pointer = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(mult->op0)));
                        if(size_of_pointer == (deltabit / 8))
                        {
                           right_offset_var += print_node(GET_INDEX_NODE(mult->op1), v, vppf);
                           do_reverse_pointer_arithmetic = true;
                        }
                        else
                        {
                           unsigned int integer_type_index = 0;
                           tree_nodeRef temp1 = tree_helper::get_type_node(GET_NODE(mult->op0), integer_type_index);
                           THROW_ASSERT(GetPointer<integer_type>(temp1), "Type of integer cast " + boost::lexical_cast<std::string>(GET_INDEX_NODE(mult->op0)) + " is not an integer type");
                           const integer_type* it = GetPointer<integer_type>(temp1);
                           auto max_int = static_cast<unsigned long long>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(it->max))));
                           long long new_size_of_pointer = static_cast<long long>(max_int) + 1 - size_of_pointer;
                           if(new_size_of_pointer == (deltabit / 8))
                           {
                              right_offset_var += "-" + print_node(GET_INDEX_NODE(mult->op1), v, vppf);
                              do_reverse_pointer_arithmetic = true;
                           }
                           else if((deltabit / 8) and (size_of_pointer % ((deltabit / 8)) == 0))
                           {
                              right_offset_var += print_node(GET_INDEX_NODE(mult->op1), v, vppf);
                              right_offset_var += " * ";
                              right_offset_var += boost::lexical_cast<std::string>(size_of_pointer / ((deltabit / 8)));
                              do_reverse_pointer_arithmetic = true;
                           }
                        }
                     }
                     exit = true;
                     break;
                  }
                  case(negate_expr_K):
                  {
                     right_offset_var += "-";
                     auto* ne = GetPointer<negate_expr>(right_op_node);
                     right_op_node = GET_NODE(ne->op);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "New op node is " + boost::lexical_cast<std::string>(GET_INDEX_NODE(ne->op)) + " - " + right_op_node->get_kind_text());
                     break;
                  }
                  case paren_expr_K:
                  case(nop_expr_K):
                  {
                     auto* ne = GetPointer<nop_expr>(right_op_node);
                     right_op_node = GET_NODE(ne->op);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "New op node is " + boost::lexical_cast<std::string>(GET_INDEX_NODE(ne->op)) + " - " + right_op_node->get_kind_text());
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
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Reversing pointer arithmetic sucessful");
               do_reverse_pointer_arithmetic = true;
               right_offset_var = print_node(right_op, v, vppf);
            }
         }
         bool char_pointer = false;
         if(!do_reverse_pointer_arithmetic and get_size(GET_INDEX_NODE(GetPointer<pointer_type>(type_node)->ptd)) == 8)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Reversing pointer arithmetic sucessful because of char pointer");
            do_reverse_pointer_arithmetic = true;
            char_pointer = true;
         }
         if(binary_op_cast && !do_reverse_pointer_arithmetic)
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(ppe->type)) + ")(";
         if((left_op_cast and (pointed_type->get_kind() == void_type_K)) or not do_reverse_pointer_arithmetic)
         {
            res += "((unsigned char*)";
         }
         res += print_node(left_op, v, vppf);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "After printing of left part " + res);
         if((left_op_cast and (pointed_type->get_kind() == void_type_K)) or not do_reverse_pointer_arithmetic)
            res += ")";
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
         if(do_reverse_pointer_arithmetic and !char_pointer)
         {
            if(right_offset_var != "")
               res += right_offset_var;
            else
            {
               res += boost::lexical_cast<std::string>(pointer_offset);
               tree_nodeRef type = GET_NODE(GetPointer<integer_cst>(right_op_node)->type);
               auto* it = GetPointer<integer_type>(type);
               bool unsigned_flag = (it && it->unsigned_flag) || type->get_kind() == pointer_type_K || type->get_kind() == boolean_type_K || type->get_kind() == enumeral_type_K;
               if(unsigned_flag)
                  res += "u";
            }
         }
         else
         {
            if(right_op_node->get_kind() == integer_cst_K)
            {
               res += boost::lexical_cast<std::string>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(right_op_node)));
            }
            else
            {
               res += print_node(right_op, v, vppf);
            }
         }
         if(binary_op_cast and not do_reverse_pointer_arithmetic)
            res += ")";
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
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
         bool binary_op_cast = is_a_pointer(index);
         if(binary_op_cast)
            res += "((" + tree_helper::print_type(TM, get_type(index)) + ")(";
         const std::string op = tree_helper::op_symbol(node);
         auto* be = GetPointer<binary_expr>(node);
         unsigned int left_op = GET_INDEX_NODE(be->op0);
         unsigned int right_op = GET_INDEX_NODE(be->op1);

         unsigned int left_op_type_index = 0;
         tree_helper::get_type_node(GET_NODE(be->op0), left_op_type_index);
         unsigned int right_op_type_index = 0;
         tree_helper::get_type_node(GET_NODE(be->op1), right_op_type_index);

         bool vector = tree_helper::is_a_vector(TM, GET_INDEX_NODE(be->type)) && tree_helper::is_a_vector(TM, right_op_type_index) && right_op_type_index != GET_INDEX_NODE(be->type);
         if(vector)
         {
            const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(be->type));
            const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
            const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(be->type)));
            const unsigned int vector_size = size / element_size;
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(be->type)) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + print_node(GET_INDEX_NODE(be->op0), v, vppf) + ")[" + STR(ind) + "] " + op + " (" + print_node(GET_INDEX_NODE(be->op1), v, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
                  res += ", ";
            }
            res += "}";
            break;
         }
         else
         {
            bool left_op_cast = is_a_pointer(left_op) and is_a_pointer(right_op) and (tree_helper::size(TM, left_op) != tree_helper::size(TM, right_op));
            bool right_op_cast = is_a_pointer(right_op) and is_a_pointer(left_op) and (tree_helper::size(TM, left_op) != tree_helper::size(TM, right_op));
            bool left_op_bracket = !(GetPointer<decl_node>(GET_NODE(be->op0)) || GetPointer<ssa_name>(GET_NODE(be->op0)));
            bool right_op_bracket = !(GetPointer<decl_node>(GET_NODE(be->op1)) || GetPointer<ssa_name>(GET_NODE(be->op1)));

            if(left_op_bracket)
               res += "(";
            if(left_op_cast)
               res += "((unsigned long int)";

            if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(be->type)) && tree_helper::is_a_vector(TM, left_op_type_index) && left_op_type_index != GET_INDEX_NODE(be->type))
               res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(be->type)) + ")(";

            res += print_node(left_op, v, vppf);

            if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(be->type)) && tree_helper::is_a_vector(TM, left_op_type_index) && left_op_type_index != GET_INDEX_NODE(be->type))
               res += ")";

            if(left_op_cast)
               res += ")";
            if(left_op_bracket)
               res += ")";
            res += std::string(" ") + op + " ";
            if(right_op_bracket)
               res += "(";
            if(right_op_cast)
               res += "((unsigned long int)";

            res += print_node(right_op, v, vppf);

            if(right_op_cast)
               res += ")";
            if(right_op_bracket)
               res += ")";
            if(binary_op_cast)
               res += "))";
         }
         break;
      }
      case rrotate_expr_K:
      case lrotate_expr_K:
      {
         bool binary_op_cast = is_a_pointer(index);
         unsigned int type_index = get_type(index);
         const tree_nodeRef type = TM->get_tree_node_const(type_index);
         if(binary_op_cast)
            res += "((" + tree_helper::print_type(TM, type_index) + ")(";
         auto* be = GetPointer<binary_expr>(node);
         unsigned int left_op = GET_INDEX_NODE(be->op0);
         bool left_op_cast = is_a_pointer(left_op);
         unsigned int right_op = GET_INDEX_NODE(be->op1);
         bool right_op_cast = is_a_pointer(right_op);
         if(left_op_cast)
            res += "((unsigned long int)";
         res += print_node(left_op, v, vppf);
         if(left_op_cast)
            res += ")";
         if(node->get_kind() == rrotate_expr_K)
            res += " >> ";
         else
            res += " << ";
         if(right_op_cast)
            res += "((unsigned long int)";
         res += print_node(right_op, v, vppf);
         if(right_op_cast)
            res += ")";
         res += " | ";
         if(left_op_cast)
            res += "((unsigned long int)";
         res += print_node(left_op, v, vppf);
         if(left_op_cast)
            res += ")";
         if(node->get_kind() == rrotate_expr_K)
            res += " << ";
         else
            res += " >> ";
         res += "(" + boost::lexical_cast<std::string>(GetPointer<integer_type>(type)->prec) + "-";
         if(right_op_cast)
            res += "((unsigned long int)";
         res += print_node(right_op, v, vppf);
         if(right_op_cast)
            res += ")";
         res += ")";
         if(binary_op_cast)
            res += "))";
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(node);
         std::string concat_shift_string;
         if(le->op8)
            THROW_ERROR("not supported");
         if(le->op7)
            THROW_ERROR("not supported");
         if(le->op6)
            concat_shift_string = concat_shift_string + "((" + print_node(GET_INDEX_NODE(le->op6), v, vppf) + ")<<5) | ";
         if(le->op5)
            concat_shift_string = concat_shift_string + "((" + print_node(GET_INDEX_NODE(le->op5), v, vppf) + ")<<4) | ";
         if(le->op4)
            concat_shift_string = concat_shift_string + "((" + print_node(GET_INDEX_NODE(le->op4), v, vppf) + ")<<3) | ";
         if(le->op3)
            concat_shift_string = concat_shift_string + "((" + print_node(GET_INDEX_NODE(le->op3), v, vppf) + ")<<2) | ";
         if(le->op2)
            concat_shift_string = concat_shift_string + "((" + print_node(GET_INDEX_NODE(le->op2), v, vppf) + ")<<1) | ";
         concat_shift_string = concat_shift_string + "(" + print_node(GET_INDEX_NODE(le->op1), v, vppf) + ")";
         res = res + "(" + print_node(le->op0->index, v, vppf) + ">>(" + concat_shift_string + "))&1";
         break;
      }
      case negate_expr_K:
      case bit_not_expr_K:
      case reference_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      {
         const std::string op = tree_helper::op_symbol(node);
         auto* ue = GetPointer<unary_expr>(node);
         res = res + " " + op + "(" + print_node(GET_INDEX_NODE(ue->op), v, vppf) + ")";
         break;
      }
      case truth_not_expr_K:
      {
         const auto* te = GetPointer<truth_not_expr>(node);
         if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(te->type)))
         {
            const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(te->type));
            const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
            const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(te->type)));
            const unsigned int vector_size = size / element_size;
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(te->type)) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += " !(" + print_node(GET_INDEX_NODE(te->op), v, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
                  res += ", ";
            }
            res += "}";
         }
         else
         {
            const std::string op = tree_helper::op_symbol(node);
            res = res + " " + op + "(" + print_node(GET_INDEX_NODE(te->op), v, vppf) + ")";
            break;
         }
         break;
      }
      case realpart_expr_K:
      case imagpart_expr_K:
      {
         const std::string op = tree_helper::op_symbol(node);
         auto* ue = GetPointer<unary_expr>(node);
         res = res + " " + op + print_node(GET_INDEX_NODE(ue->op), v, vppf);
         auto* sa = GetPointer<ssa_name>(GET_NODE(ue->op));
         if(sa and (sa->volatile_flag || GET_NODE(sa->CGetDefStmt())->get_kind() == gimple_nop_K) and sa->var and GetPointer<var_decl>(GET_NODE(sa->var)))
         {
            res += " = 0";
         }
         break;
      }
      case addr_expr_K:
      {
         auto* ue = GetPointer<addr_expr>(node);
         if(GetPointer<component_ref>(GET_NODE(ue->op)) and has_bit_field(GET_INDEX_NODE(GetPointer<component_ref>(GET_NODE(ue->op))->op1)))
         {
            THROW_ERROR_CODE(BITFIELD_EC, "Trying to get the address of a bitfield");
         }
         if(GET_NODE(ue->op)->get_kind() == array_ref_K)
         {
            const array_ref* ar = GetPointer<array_ref>(GET_NODE(ue->op));
            ///&string[0]
            if(GET_NODE(ar->op0)->get_kind() == string_cst_K and GET_NODE(ar->op1)->get_kind() == integer_cst_K and tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(ar->op1))) == 0)
            {
               res += print_node(GET_INDEX_NODE(ar->op0), v, vppf);
               break;
            }
         }
         ///&array is printed back as array
         if(GET_NODE(ue->op)->get_kind() == var_decl_K and tree_helper::CGetType(GET_NODE(ue->op))->get_kind() == array_type_K)
         {
            res += print_node(GET_INDEX_NODE(ue->op), v, vppf);
            break;
         }
         if(GET_NODE(ue->op)->get_kind() == label_decl_K)
         {
            res += "&&" + print_node(GET_INDEX_NODE(ue->op), v, vppf);
            break;
         }
         res += "(" + tree_helper::op_symbol(node) + "(" + print_node(GET_INDEX_NODE(ue->op), v, vppf) + "))";
         break;
      }
      case function_decl_K:
      {
         auto* fd = GetPointer<function_decl>(node);
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
         auto* ue = GetPointer<unary_expr>(node);
         unsigned int return_index = 0;
         const tree_nodeRef type = tree_helper::get_type_node(node, return_index);
         unsigned int prec = 0;
         if(type && (type->get_kind() == integer_type_K))
            prec = GetPointer<integer_type>(type)->prec;
         unsigned int algn = 0;
         if(type && (type->get_kind() == integer_type_K))
            algn = GetPointer<integer_type>(type)->algn;

         unsigned int operand_type_index;
         const tree_nodeRef operand_type = tree_helper::get_type_node(GET_NODE(ue->op), operand_type_index);
         unsigned int operand_prec = 0;
         if(operand_type && (operand_type->get_kind() == integer_type_K))
            operand_prec = GetPointer<integer_type>(operand_type)->prec;
         unsigned int operand_algn = 0;
         if(operand_type && (operand_type->get_kind() == integer_type_K))
            operand_algn = GetPointer<integer_type>(operand_type)->algn;

         std::string operand_res = print_node(GET_INDEX_NODE(ue->op), v, vppf);
         if(operand_prec != operand_algn && operand_prec % operand_algn && !GetPointer<integer_type>(operand_type)->unsigned_flag)
         {
            operand_res = "((union {" + tree_helper::print_type(TM, operand_type_index) + " orig; " + tree_helper::print_type(TM, operand_type_index) + " bitfield : " + STR(operand_prec) + ";}){" + operand_res + "}).bitfield";
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
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bitfield");
            bool ui = (GetPointer<integer_type>(operand_type) and GetPointer<integer_type>(operand_type)->unsigned_flag) and (GetPointer<integer_type>(type) and not GetPointer<integer_type>(type)->unsigned_flag);
            if(ui)
            {
               res += "((" + tree_helper::print_type(TM, type->index) + ")(";
            }
            res += "(";
            res += operand_res;
            res += ")%(1";
            if(prec > 32)
               res += "LL";
            if(GetPointer<integer_type>(type)->unsigned_flag)
               res += "U";
            res += " << " + boost::lexical_cast<std::string>(prec) + ")";
            if(ui)
            {
               res += std::string(" << ") + STR(algn - prec) + ")) >> " + STR(algn - prec);
            }
         }
         else
         {
            res = res + "(" + tree_helper::print_type(TM, GET_INDEX_NODE(ue->type)) + ") (";
            res += operand_res;
            res += ")";
         }
         break;
      }
      case view_convert_expr_K:
      {
         const view_convert_expr* vce = GetPointer<view_convert_expr>(node);
         if(GetPointer<integer_cst>(GET_NODE(vce->op)))
         {
            res = res + "__panda_union.dest;}";
         }
         else
         {
            if(tree_helper::is_a_pointer(TM, vce->type->index))
               res = res + "((" + tree_helper::print_type(TM, vce->type->index) + ") (" + print_node(vce->op->index, v, vppf) + "))";
            else
               res = res + "*((" + tree_helper::print_type(TM, vce->type->index) + " * ) &(" + print_node(vce->op->index, v, vppf) + "))";
         }
         break;
      }
      case component_ref_K:
      {
         auto* cr = GetPointer<component_ref>(node);
         res = "(" + print_node(GET_INDEX_NODE(cr->op0), v, vppf) + ")." + print_node(GET_INDEX_NODE(cr->op1), v, vppf);
         break;
      }
      case indirect_ref_K:
      {
         auto* ir = GetPointer<indirect_ref>(node);
         res = "*(";
         if(GetPointer<integer_cst>(GET_NODE(ir->op)))
            res += "(" + tree_helper::print_type(TM, get_type(GET_INDEX_NODE(ir->op))) + ")";
         res += print_node(GET_INDEX_NODE(ir->op), v, vppf);
         res += ")";
         break;
      }
      case misaligned_indirect_ref_K:
      {
         auto* mir = GetPointer<misaligned_indirect_ref>(node);
         res = "*(" + print_node(GET_INDEX_NODE(mir->op), v, vppf) + ")";
         break;
      }
      case mem_ref_K:
      {
         auto* mr = GetPointer<mem_ref>(node);
         long long int offset = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(mr->op1)));
         tree_managerRef temp_TM(const_cast<tree_manager*>(TM.get()), null_deleter());
         const tree_manipulationRef tm(new tree_manipulation(temp_TM, Param));
         const unsigned int pointer_type = tm->create_pointer_type(mr->type, 8)->index;
         const std::string type_string = tree_helper::print_type(TM, pointer_type);
         if(offset == 0)
         {
            res = "(*((" + type_string + ")(" + print_node(GET_INDEX_NODE(mr->op0), v, vppf) + ")))";
         }
         else
            res = "(*((" + type_string + ")(((unsigned char*)" + print_node(GET_INDEX_NODE(mr->op0), v, vppf) + ") + " + boost::lexical_cast<std::string>(offset) + ")))";
         break;
      }
      case target_mem_ref_K:
      {
         auto* tmr = GetPointer<target_mem_ref>(node);
         bool need_plus = false;
         res = "(*((" + tree_helper::print_type(TM, GET_INDEX_NODE(tmr->type)) + "*)(";
         if(tmr->symbol)
         {
            res += "((unsigned char*)" + (tree_helper::is_a_struct(TM, GET_INDEX_NODE(tmr->symbol)) || tree_helper::is_an_union(TM, GET_INDEX_NODE(tmr->symbol)) ? std::string("&") : std::string("")) + print_node(GET_INDEX_NODE(tmr->symbol), v, vppf) + ")";
            need_plus = true;
         }
         if(tmr->base)
         {
            if(need_plus)
               res += "+";
            else
               need_plus = true;
            if(tmr->symbol)
               res += "(" + print_node(GET_INDEX_NODE(tmr->base), v, vppf) + ")";
            else
               res += "((unsigned char*)" + print_node(GET_INDEX_NODE(tmr->base), v, vppf) + ")";
         }
         if(tmr->step)
         {
            if(need_plus)
               res += "+";
            need_plus = false;
            res += print_node(GET_INDEX_NODE(tmr->step), v, vppf) + "*";
            THROW_ASSERT(tmr->idx, "idx expected!");
         }
         if(tmr->idx)
         {
            if(need_plus)
               res += "+";
            else
               need_plus = true;
            res += print_node(GET_INDEX_NODE(tmr->idx), v, vppf);
         }
         if(tmr->offset)
         {
            if(need_plus)
               res += "+";
            res += print_node(GET_INDEX_NODE(tmr->offset), v, vppf);
         }
         res += ")))";
         break;
      }
      case target_mem_ref461_K:
      {
         auto* tmr = GetPointer<target_mem_ref461>(node);
         bool need_plus = false;
         bool isFunctionPointer = tree_helper::is_function_pointer_type(TM, GET_INDEX_NODE(tmr->type));
         res = isFunctionPointer ? "(" : "(*((";
         res += tree_helper::print_type(TM, GET_INDEX_NODE(tmr->type));
         res += isFunctionPointer ? ")(" : "*)(";
         if(tmr->base)
         {
            need_plus = true;
            res += "((unsigned char*)" + (tree_helper::is_a_struct(TM, GET_INDEX_NODE(tmr->base)) || tree_helper::is_an_union(TM, GET_INDEX_NODE(tmr->base)) ? std::string("&") : std::string("")) + print_node(GET_INDEX_NODE(tmr->base), v, vppf) + ")";
         }
         if(tmr->step)
         {
            if(need_plus)
               res += "+";
            need_plus = false;
            res += print_node(GET_INDEX_NODE(tmr->step), v, vppf) + "*";
            THROW_ASSERT(tmr->idx, "idx expected!");
         }
         if(tmr->idx)
         {
            if(need_plus)
               res += "+";
            else
               need_plus = true;
            res += print_node(GET_INDEX_NODE(tmr->idx), v, vppf);
         }
         if(tmr->idx2)
         {
            if(need_plus)
               res += "+";
            else
               need_plus = true;
            res += print_node(GET_INDEX_NODE(tmr->idx2), v, vppf);
         }
         if(tmr->offset)
         {
            if(need_plus)
               res += "+";
            res += print_node(GET_INDEX_NODE(tmr->offset), v, vppf);
         }
         res += isFunctionPointer ? ")" : ")))";
         break;
      }
      case array_ref_K:
      {
         auto* ar = GetPointer<array_ref>(node);
         tree_nodeRef base = GET_NODE(ar->op0);
         // tree_nodeRef offset = GET_NODE(ar->op1);
         if(base->get_kind() == mem_ref_K)
         {
            auto* mr = GetPointer<mem_ref>(base);
            long long int offset = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(mr->op1)));
            unsigned int type = GET_INDEX_NODE(mr->type);
            std::string type_string = tree_helper::print_type(TM, type);
            if(is_an_array(type))
            {
               size_t found_square_bracket = type_string.find("[");
               if(found_square_bracket != std::string::npos)
                  type_string.insert(found_square_bracket, "(*)");
               else
                  type_string = type_string + "*";
            }
            else
               type_string = type_string + "*";
            if(offset == 0)
               res = "(*((" + type_string + ")(" + print_node(GET_INDEX_NODE(mr->op0), v, vppf) + ")))";
            else
               res = "(*((" + type_string + ")(((unsigned char*)" + print_node(GET_INDEX_NODE(mr->op0), v, vppf) + ") + " + boost::lexical_cast<std::string>(offset) + ")))";
         }
         else
            res = "(" + print_node(GET_INDEX_NODE(ar->op0), v, vppf) + ")";
         res += "[" + print_node(GET_INDEX_NODE(ar->op1), v, vppf) + "]";
         break;
      }
      case bit_field_ref_K:
      {
#if HAVE_SPARC_COMPILER
         if(Param->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_SPARC_GCC)
         {
            THROW_ERROR_CODE(BITFIELD_EC, "Bitfield not supported by sparc cross compiler");
         }
#endif
         auto* bf = GetPointer<bit_field_ref>(node);

         long long int bpos = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(bf->op2)));
         res += "*((" + tree_helper::print_type(TM, GET_INDEX_NODE(bf->type)) + "* ) (((unsigned long int) &(" + print_node(GET_INDEX_NODE(bf->op0), v, vppf) + ")) + (unsigned long int)" + STR(bpos / 8) + "))";
         if(bpos % 8)
            res += " >> " + boost::lexical_cast<std::string>(bpos % 8);
         break;
      }
      /* post/pre increment and decrement operator should be treated as unary even if they are binary_expr*/
      case postdecrement_expr_K:
      case postincrement_expr_K:
      {
         const std::string op = tree_helper::op_symbol(node);
         auto* be = GetPointer<binary_expr>(node);
         res += print_node(GET_INDEX_NODE(be->op0), v, vppf) + op + print_node(GET_INDEX_NODE(be->op1), v, vppf);
         break;
      }
      case min_expr_K:
      {
         auto* me = GetPointer<min_expr>(node);
         if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(me->type)))
         {
            const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(me->type));
            const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
            const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(me->type)));
            const unsigned int vector_size = size / element_size;
            res += "/*" + me->get_kind_text() + "*/";
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(me->type)) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + print_node(GET_INDEX_NODE(me->op0), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "] < (" + print_node(GET_INDEX_NODE(me->op1), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "] ? " + "(" +
                      print_node(GET_INDEX_NODE(me->op0), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "]" + " : " + "(" + print_node(GET_INDEX_NODE(me->op1), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "]";
               if(ind != vector_size - 1)
                  res += ", ";
            }
            res += "}";
         }
         else
         {
            std::string op_0 = print_node(GET_INDEX_NODE(me->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(me->op1), v, vppf);
            res += op_0 + " < " + op_1 + " ? " + op_0 + " : " + op_1;
         }
         break;
      }
      case max_expr_K:
      {
         auto* me = GetPointer<max_expr>(node);
         if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(me->type)))
         {
            const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(me->type));
            const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
            const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(me->type)));
            const unsigned int vector_size = size / element_size;
            res += "/*" + me->get_kind_text() + "*/";
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(me->type)) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + print_node(GET_INDEX_NODE(me->op0), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "] > (" + print_node(GET_INDEX_NODE(me->op1), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "] ? " + "(" +
                      print_node(GET_INDEX_NODE(me->op0), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "]" + " : " + "(" + print_node(GET_INDEX_NODE(me->op1), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "]";
               if(ind != vector_size - 1)
                  res += ", ";
            }
            res += "}";
         }
         else
         {
            std::string op_0 = print_node(GET_INDEX_NODE(me->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(me->op1), v, vppf);
            res += op_0 + " > " + op_1 + " ? " + op_0 + " : " + op_1;
         }
         break;
      }
      case unordered_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         std::string op_0 = print_node(GET_INDEX_NODE(be->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(be->op1), v, vppf);
         res += "__builtin_isunordered(" + op_0 + "," + op_1 + ")";
         break;
      }
      case ordered_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         std::string op_0 = print_node(GET_INDEX_NODE(be->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(be->op1), v, vppf);
         res += "!__builtin_isunordered(" + op_0 + "," + op_1 + ")";
         break;
      }
      case unlt_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         std::string op_0 = print_node(GET_INDEX_NODE(be->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(be->op1), v, vppf);
         res += "!__builtin_isgreaterequal(" + op_0 + "," + op_1 + ")";
         break;
      }
      case unle_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         std::string op_0 = print_node(GET_INDEX_NODE(be->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(be->op1), v, vppf);
         res += "!__builtin_isgreater(" + op_0 + "," + op_1 + ")";
         break;
      }
      case ungt_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         std::string op_0 = print_node(GET_INDEX_NODE(be->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(be->op1), v, vppf);
         res += "!__builtin_islessequal(" + op_0 + "," + op_1 + ")";
         break;
      }
      case unge_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         std::string op_0 = print_node(GET_INDEX_NODE(be->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(be->op1), v, vppf);
         res += "!__builtin_isless(" + op_0 + "," + op_1 + ")";
         break;
      }
      case uneq_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         std::string op_0 = print_node(GET_INDEX_NODE(be->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(be->op1), v, vppf);
         res += "!__builtin_islessgreater(" + op_0 + "," + op_1 + ")";
         break;
      }
      case ltgt_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         std::string op_0 = print_node(GET_INDEX_NODE(be->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(be->op1), v, vppf);
         res += "__builtin_islessgreater(" + op_0 + "," + op_1 + ")";
         break;
      }
      case abs_expr_K:
      {
         auto* ae = GetPointer<abs_expr>(node);
         tree_nodeRef type = GET_NODE(ae->type);
         std::string op_0 = print_node(GET_INDEX_NODE(ae->op), v, vppf);
         if(GetPointer<real_type>(type))
         {
            auto* rt = GetPointer<real_type>(type);
            if(rt->prec == 80)
               res += "__builtin_fabsl(" + op_0 + ")";
            else if(rt->prec == 64)
               res += "__builtin_fabs(" + op_0 + ")";
            else if(rt->prec == 32)
               res += "__builtin_fabsf(" + op_0 + ")";
            else
               THROW_ERROR("Abs on a real number with not supported precision");
         }
         else
            res += "(" + op_0 + ") >= 0 ? (" + op_0 + ") : -(" + op_0 + ")";
         //            res += "__builtin_llabs(" + op_0 + ")";
         break;
      }
      case complex_expr_K:
      {
         auto* ce = GetPointer<complex_expr>(node);
         std::string op_0 = print_node(GET_INDEX_NODE(ce->op0), v, vppf), op_1 = print_node(GET_INDEX_NODE(ce->op1), v, vppf);
         res += op_0 + "+ 1i*" + op_1;
         break;
      }
      case cond_expr_K:
      {
         auto* ce = GetPointer<cond_expr>(node);
         res = print_node(GET_INDEX_NODE(ce->op0), v, vppf) + " ? " + print_node(GET_INDEX_NODE(ce->op1), v, vppf) + " : " + print_node(GET_INDEX_NODE(ce->op2), v, vppf);
         break;
      }
      case ternary_plus_expr_K:
      {
         auto* te = GetPointer<ternary_expr>(node);
         res = print_node(GET_INDEX_NODE(te->op0), v, vppf) + " + " + print_node(GET_INDEX_NODE(te->op1), v, vppf) + " + " + print_node(GET_INDEX_NODE(te->op2), v, vppf);
         break;
      }
      case ternary_pm_expr_K:
      {
         auto* te = GetPointer<ternary_expr>(node);
         res = print_node(GET_INDEX_NODE(te->op0), v, vppf) + " + " + print_node(GET_INDEX_NODE(te->op1), v, vppf) + " - " + print_node(GET_INDEX_NODE(te->op2), v, vppf);
         break;
      }
      case ternary_mp_expr_K:
      {
         auto* te = GetPointer<ternary_expr>(node);
         res = print_node(GET_INDEX_NODE(te->op0), v, vppf) + " - " + print_node(GET_INDEX_NODE(te->op1), v, vppf) + " + " + print_node(GET_INDEX_NODE(te->op2), v, vppf);
         break;
      }
      case ternary_mm_expr_K:
      {
         auto* te = GetPointer<ternary_expr>(node);
         res = print_node(GET_INDEX_NODE(te->op0), v, vppf) + " - " + print_node(GET_INDEX_NODE(te->op1), v, vppf) + " - " + print_node(GET_INDEX_NODE(te->op2), v, vppf);
         break;
      }
      case bit_ior_concat_expr_K:
      {
         auto* te = GetPointer<ternary_expr>(node);
         res = print_node(GET_INDEX_NODE(te->op0), v, vppf) + " | (" + print_node(GET_INDEX_NODE(te->op1), v, vppf) + " & ((1ULL<<" + print_node(GET_INDEX_NODE(te->op2), v, vppf) + ")-1))";
         break;
      }
      case vec_cond_expr_K:
      {
         auto* vce = GetPointer<vec_cond_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vce->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vce->type)));
         const unsigned int vector_size = size / element_size;
         res += "/*" + vce->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vce->type)) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += "(" + print_node(GET_INDEX_NODE(vce->op0), v, vppf) + ")" + (tree_helper::is_a_vector(TM, vce->op0->index) ? "[" + boost::lexical_cast<std::string>(ind) + "]" : "") + " ? " + "(" + print_node(GET_INDEX_NODE(vce->op1), v, vppf) + ")[" +
                   boost::lexical_cast<std::string>(ind) + "]" + " : " + "(" + print_node(GET_INDEX_NODE(vce->op2), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "]";
            if(ind != vector_size - 1)
               res += ", ";
         }
         res += "}";
         break;
      }
      case vec_perm_expr_K:
      {
         auto* vpe = GetPointer<vec_perm_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vpe->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vpe->type)));
         const unsigned int vector_size = size / element_size;
         res += "/*" + vpe->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vpe->type)) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += "((((" + print_node(GET_INDEX_NODE(vpe->op2), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "])%" + boost::lexical_cast<std::string>(2 * vector_size) + ") < " + boost::lexical_cast<std::string>(vector_size) + ") ? (" +
                   print_node(GET_INDEX_NODE(vpe->op0), v, vppf) + ")[(((" + print_node(GET_INDEX_NODE(vpe->op2), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "])%" + boost::lexical_cast<std::string>(2 * vector_size) + ")] : (" +
                   print_node(GET_INDEX_NODE(vpe->op1), v, vppf) + ")[(((" + print_node(GET_INDEX_NODE(vpe->op2), v, vppf) + ")[" + boost::lexical_cast<std::string>(ind) + "])%" + boost::lexical_cast<std::string>(2 * vector_size) + ")-" +
                   STR(vector_size) + "]";
            if(ind != vector_size - 1)
               res += ", ";
         }
         res += "}";
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(node);
         std::vector<tree_nodeRef> pragmas = gc->pragmas;
         std::vector<tree_nodeRef>::const_iterator pragma, pragma_end = pragmas.end();
         for(pragma = pragmas.begin(); pragma != pragma_end; ++pragma)
         {
            res += print_node(GET_INDEX_NODE(*pragma), v, vppf) + "\n";
         }
         res = "if (" + print_node(GET_INDEX_NODE(gc->op0), v, vppf) + ")";
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(node);
         res = "if (";
         bool first = true;
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(first)
            {
               THROW_ASSERT(cond.first, "First condition of multi way if " + STR(index) + " is empty");
               res += print_node(cond.first->index, v, vppf);
               first = false;
            }
            else if(cond.first)
               res += " /* else if(" + print_node(cond.first->index, v, vppf) + ")*/";
         }
         res += ")";
         break;
      }
      case gimple_while_K:
      {
         const auto* we = GetPointer<const gimple_while>(node);
         res = "while (" + print_node(GET_INDEX_NODE(we->op0), v, vppf) + ")";
         break;
      }
      case gimple_for_K:
      {
         const auto* fe = GetPointer<const gimple_for>(node);
#if !RELEASE
         if(fe->omp_for)
         {
            res = "//#pragma omp " + print_node(GET_INDEX_NODE(fe->omp_for), v, vppf) + "\n";
         }
#endif
         res += "for (" + print_node(GET_INDEX_NODE(fe->op1), v, vppf) + "; ";
         res += print_node(GET_INDEX_NODE(fe->op0), v, vppf) + "; ";
         res += print_node(GET_INDEX_NODE(fe->op2), v, vppf);
         res += ")";
         break;
      }
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(node);
         res += "switch(" + print_node(GET_INDEX_NODE(se->op0), v, vppf) + ")";
         break;
      }
      case gimple_assign_K:
      {
         auto* ms = GetPointer<gimple_assign>(node);
         if(!ms->init_assignment && !ms->clobber)
         {
            std::vector<tree_nodeRef> pragmas = ms->pragmas;
            std::vector<tree_nodeRef>::const_iterator pragma, pragma_end = pragmas.end();
            for(pragma = pragmas.begin(); pragma != pragma_end; ++pragma)
            {
               res += print_node(GET_INDEX_NODE(*pragma), v, vppf) + "\n";
            }
            res = "";
            if(is_an_array(GET_INDEX_NODE(ms->op0)))
            {
               long long size = tree_helper::size(TM, GET_INDEX_NODE(ms->op0));
               res += "__builtin_memcpy(";
               if(GetPointer<mem_ref>(GET_NODE(ms->op0)))
                  res += "&";
               res += print_node(GET_INDEX_NODE(ms->op0), v, vppf) + ", ";
               if(GET_NODE(ms->op1)->get_kind() == view_convert_expr_K)
               {
                  const view_convert_expr* vce = GetPointer<view_convert_expr>(GET_NODE(ms->op1));
                  res += "&" + print_node(GET_INDEX_NODE(vce->op), v, vppf) + ", ";
               }
               else
               {
                  if(GetPointer<mem_ref>(GET_NODE(ms->op1)))
                     res += "&";
                  res += print_node(GET_INDEX_NODE(ms->op1), v, vppf) + ", ";
               }
               res += boost::lexical_cast<std::string>(size / 8) + ")";
               break;
            }
            if(not Param->getOption<bool>(OPT_without_transformation) and
               ((is_a_struct(GET_INDEX_NODE(ms->op0)) or is_an_union(GET_INDEX_NODE(ms->op0))) and (is_a_struct(GET_INDEX_NODE(ms->op1)) or is_an_union(GET_INDEX_NODE(ms->op1))) and
                tree_helper::name_type(TM, tree_helper::GetRealType(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(ms->op0)))) != tree_helper::name_type(TM, tree_helper::GetRealType(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(ms->op1))))))
            {
               THROW_ERROR_CODE(C_EC, "Implicit struct type defintion not supported in gimple assignment " + boost::lexical_cast<std::string>(index) + " - " +
                                          tree_helper::name_type(TM, tree_helper::GetRealType(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(ms->op0)))) + " vs. " +
                                          tree_helper::name_type(TM, tree_helper::GetRealType(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(ms->op1)))));
            }
            res += print_node(GET_INDEX_NODE(ms->op0), v, vppf) + " = ";
            const tree_nodeRef right = GET_NODE(ms->op1);
            /// check for type conversion
            switch(right->get_kind())
            {
               case constructor_K:
               {
                  res += "(" + tree_helper::print_type(TM, get_type(GET_INDEX_NODE(ms->op0))) + ") ";
                  res += print_node(GET_INDEX_NODE(ms->op1), v, vppf);
                  break;
               }
               case vector_cst_K:
               {
                  auto* vc = GetPointer<vector_cst>(right);
                  if(get_type(GET_INDEX_NODE(ms->op0)) != GET_INDEX_NODE(vc->type))
                     res += "(" + tree_helper::print_type(TM, get_type(GET_INDEX_NODE(ms->op0))) + ") ";
                  res += print_node(GET_INDEX_NODE(ms->op1), v, vppf);
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
                  unsigned int left_type_index = get_type(GET_INDEX_NODE(ms->op0));
                  unsigned int right_type_index = get_type(GET_INDEX_NODE(ms->op1));
                  if(tree_helper::is_a_vector(TM, left_type_index) && left_type_index != right_type_index)
                     res += "(" + tree_helper::print_type(TM, left_type_index) + ") ";
                  if((right->get_kind() == rshift_expr_K or right->get_kind() == lshift_expr_K) and tree_helper::is_a_pointer(TM, GET_INDEX_NODE(GetPointer<binary_expr>(right)->op0)))
                     res += "(unsigned int)";
                  res += print_node(GET_INDEX_NODE(ms->op1), v, vppf);
                  break;
               }
               case void_cst_K:
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
            const auto vce = GetPointer<const view_convert_expr>(GET_NODE(ms->op1));
            if(vce and GET_NODE(vce->op)->get_kind() == integer_cst_K)
            {
               const auto dest_type = get_type(ms->op1->index);
               const auto source_type = get_type(vce->op->index);
               res = "{union {" + print_type(dest_type) + " dest; " + print_type(source_type) + " source;} __panda_union; __panda_union.source = " + print_node(vce->op->index, v, vppf) + "; " + res;
            }
         }
         if(ms->predicate)
         {
            res = "if(" + print_node(ms->predicate->index, v, vppf) + ") " + res;
         }
         if(GET_NODE(ms->op1)->get_kind() == trunc_div_expr_K or GET_NODE(ms->op1)->get_kind() == trunc_mod_expr_K)
         {
            const auto tde = GetPointer<const binary_expr>(GET_NODE(ms->op1));
            res = "if(" + print_node(tde->op1->index, v, vppf) + " != 0) " + res;
         }
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case init_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         res = print_node(GET_INDEX_NODE(be->op0), v, vppf) + " = ";
         const tree_nodeRef right = GET_NODE(be->op1);
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
               auto* ue = GetPointer<unary_expr>(right);
               res = res + "(" + tree_helper::print_type(TM, get_type(GET_INDEX_NODE(be->op0))) + ") ";
               res += print_node(GET_INDEX_NODE(ue->op), v, vppf);
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
               res += print_node(GET_INDEX_NODE(be->op1), v, vppf);
         }
         break;
      }
      case gimple_return_K:
      {
         auto* re = GetPointer<gimple_return>(node);
         std::vector<tree_nodeRef> pragmas = re->pragmas;
         std::vector<tree_nodeRef>::const_iterator pragma, pragma_end = pragmas.end();
         for(pragma = pragmas.begin(); pragma != pragma_end; ++pragma)
         {
            res += print_node(GET_INDEX_NODE(*pragma), v, vppf) + "\n";
         }
         res += "return ";
         if(re->op != nullptr)
         {
            const tree_nodeRef return_node = GET_NODE(re->op);
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
                  auto* ue = GetPointer<unary_expr>(return_node);
                  res += "(" + tree_helper::print_type(TM, get_type(GET_INDEX_NODE(re->op))) + ") (";
                  res += print_node(GET_INDEX_NODE(ue->op), v, vppf);
                  res += ")";
                  break;
               }
               case constructor_K:
               {
                  res += "(" + tree_helper::print_type(TM, get_type(GET_INDEX_NODE(re->op))) + ") ";
                  res += print_node(GET_INDEX_NODE(re->op), v, vppf);
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
                  res += print_node(GET_INDEX_NODE(re->op), v, vppf);
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
         auto* ce = GetPointer<call_expr>(node);
         function_decl* fd = nullptr;
         tree_nodeRef op0 = GET_NODE(ce->fn);
         bool is_va_start_end = false;

         // sizeof workaround
         bool is_sizeof = false;

         switch(op0->get_kind())
         {
            case addr_expr_K:
            {
               auto* ue = GetPointer<unary_expr>(op0);
               tree_nodeRef fn = GET_NODE(ue->op);
               THROW_ASSERT(fn->get_kind() == function_decl_K, "tree node not currently supported " + fn->get_kind_text());
               fd = GetPointer<function_decl>(fn);
               ///__builtin_va_start should be ad-hoc managed
               std::string fname = tree_helper::print_function_name(TM, fd);
               if(fname == "__builtin_va_start" || fname == "__builtin_va_end" || fname == "__builtin_va_copy")
                  is_va_start_end = true;
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
                  res += fname;
               break;
            }
            case obj_type_ref_K:
            {
               tree_nodeRef fn = tree_helper::find_obj_type_ref_function(ce->fn);
               THROW_ASSERT(GET_NODE(fn)->get_kind(), "tree node not currently supported " + GET_NODE(fn)->get_kind_text());
               auto* local_fd = GetPointer<function_decl>(GET_NODE(fn));
               if(tree_helper::GetFunctionReturnType(GET_NODE(fn)))
                  res += print_node(GET_INDEX_NODE(fn), v, vppf) + " = ";
               res += tree_helper::print_function_name(TM, local_fd);
               break;
            }
            case ssa_name_K:
            {
               res += "(*" + print_node(GET_INDEX_NODE(ce->fn), v, vppf) + ")";
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
            case component_ref_K:
            case bit_field_ref_K:
            case vtable_ref_K:
            case with_cleanup_expr_K:
            case save_expr_K:
            case cond_expr_K:
            case dot_prod_expr_K:
            case ternary_plus_expr_K:
            case ternary_pm_expr_K:
            case ternary_mp_expr_K:
            case ternary_mm_expr_K:
            case bit_ior_concat_expr_K:
            case vec_cond_expr_K:
            case vec_perm_expr_K:
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
               THROW_ERROR(std::string("tree node not currently supported ") + op0->get_kind_text());
         }
         /// Print parameters.
         res += "(";
         if(is_va_start_end)
         {
            THROW_ASSERT(ce->args.size(), "va_start or va_end have to have arguments");
            const tree_nodeRef par1 = GET_NODE(ce->args[0]);
            const unsigned int par1_index = GET_INDEX_NODE(ce->args[0]);
            // print the first removing the address
            if(GetPointer<addr_expr>(par1))
            {
               res += print_node(GET_INDEX_NODE(GetPointer<addr_expr>(par1)->op), v, vppf);
            }
            else if(GetPointer<var_decl>(par1) || GetPointer<ssa_name>(par1))
            {
               res += "*(" + print_node(par1_index, v, vppf) + ")";
            }
            else
               THROW_ERROR("expected an address or a variable " + boost::lexical_cast<std::string>(par1_index));
            size_t arg_index;
            for(arg_index = 1; arg_index < ce->args.size(); arg_index++)
            {
               res += ", ";
               res += print_node(GET_INDEX_NODE(ce->args[arg_index]), v, vppf);
            }
         }
         else if(is_sizeof)
         {
            THROW_ASSERT(ce->args.size() == 1, "Wrong number of arguments: " + boost::lexical_cast<std::string>(ce->args.size()));
            std::string argument = print_node(GET_INDEX_NODE(ce->args[0]), v, vppf);
            const tree_nodeRef arg1 = GET_NODE(ce->args[0]);
            THROW_ASSERT(GetPointer<addr_expr>(arg1), "Argument is not an addr_expr but a " + std::string(arg1->get_kind_text()));
            const addr_expr* ae = GetPointer<addr_expr>(arg1);
            /// This pattern is for gcc 4.5
            if(GetPointer<array_ref>(GET_NODE(ae->op)))
            {
               THROW_ASSERT(GetPointer<array_ref>(GET_NODE(ae->op)), "Argument is not an array ref but a " + std::string(GET_NODE(ae->op)->get_kind_text()));
#if HAVE_ASSERTS
               const array_ref* ar = GetPointer<array_ref>(GET_NODE(ae->op));
#endif
               THROW_ASSERT(GetPointer<string_cst>(GET_NODE(ar->op0)), "Argument is not a string cast but a " + std::string(GET_NODE(ar->op0)->get_kind_text()));
               std::string unquoted_argument = argument.substr(1, argument.size() - 2);
               res += unquoted_argument;
            }
            else
            /// This pattern is for gcc 4.6
            {
               THROW_ASSERT(GetPointer<string_cst>(GET_NODE(ae->op)), "Argument is not a string cast but a " + std::string(GET_NODE(ae->op)->get_kind_text()));
               std::string unquoted_argument = argument.substr(3, argument.size() - 6);
               res += unquoted_argument;
            }
         }
         else
         {
            if(ce->args.size())
            {
               const std::vector<tree_nodeRef>& actual_args = ce->args;
               std::vector<tree_nodeRef> formal_args;
               if(fd)
                  formal_args = fd->list_of_args;
               std::vector<tree_nodeRef>::const_iterator actual_arg, actual_arg_end = actual_args.end();
               std::vector<tree_nodeRef>::const_iterator formal_arg, formal_arg_end = formal_args.end();
               for(actual_arg = actual_args.begin(), formal_arg = formal_args.begin(); actual_arg != actual_arg_end; ++actual_arg)
               {
                  if(formal_arg != formal_arg_end and (is_a_struct(GET_INDEX_NODE(*actual_arg)) or is_an_union(GET_INDEX_NODE(*actual_arg))) and (is_a_struct(GET_INDEX_NODE(*formal_arg)) or is_an_union(GET_INDEX_NODE(*formal_arg))) and
                     (tree_helper::GetRealType(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(*actual_arg))) != tree_helper::GetRealType(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(*formal_arg)))))
                  {
                     THROW_ERROR_CODE(C_EC, "Implicit struct type defintion not supported in gimple assignment " + boost::lexical_cast<std::string>(index));
                  }
                  if(actual_arg != actual_args.begin())
                     res += ", ";
                  res += print_node(GET_INDEX_NODE(*actual_arg), v, vppf);
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
         auto* ce = GetPointer<gimple_call>(node);
         function_decl* fd = nullptr;
         std::vector<tree_nodeRef> pragmas = ce->pragmas;
         std::vector<tree_nodeRef>::const_iterator pragma, pragma_end = pragmas.end();
         for(pragma = pragmas.begin(); pragma != pragma_end; ++pragma)
         {
            res += print_node(GET_INDEX_NODE(*pragma), v, vppf) + "\n";
         }
         tree_nodeRef op0 = GET_NODE(ce->fn);
         bool is_va_start_end = false;

         // sizeof workaroung
         bool is_sizeof = false;

         switch(op0->get_kind())
         {
            case addr_expr_K:
            {
               auto* ue = GetPointer<unary_expr>(op0);
               tree_nodeRef fn = GET_NODE(ue->op);
               THROW_ASSERT(fn->get_kind() == function_decl_K, "tree node not currently supported " + fn->get_kind_text());
               fd = GetPointer<function_decl>(fn);
               ///__builtin_va_start should be ad-hoc managed
               std::string fname = tree_helper::print_function_name(TM, fd);
               if(fname == "__builtin_va_start" || fname == "__builtin_va_end" || fname == "__builtin_va_copy")
                  is_va_start_end = true;
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
                  res += fname;
               break;
            }
            case obj_type_ref_K:
            {
               tree_nodeRef fn = tree_helper::find_obj_type_ref_function(ce->fn);
               THROW_ASSERT(GET_NODE(fn)->get_kind() == function_decl_K, "tree node not currently supported " + fn->get_kind_text());
               auto* local_fd = GetPointer<function_decl>(GET_NODE(fn));
               if(tree_helper::GetFunctionReturnType(GET_NODE(fn)))
                  res += print_node(GET_INDEX_NODE(fn), v, vppf) + " = ";
               res += tree_helper::print_function_name(TM, local_fd);
               break;
            }
            case ssa_name_K:
            {
               res += "(*" + print_node(GET_INDEX_NODE(ce->fn), v, vppf) + ")";
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
            case component_ref_K:
            case bit_field_ref_K:
            case vtable_ref_K:
            case with_cleanup_expr_K:
            case save_expr_K:
            case cond_expr_K:
            case dot_prod_expr_K:
            case ternary_plus_expr_K:
            case ternary_pm_expr_K:
            case ternary_mp_expr_K:
            case ternary_mm_expr_K:
            case bit_ior_concat_expr_K:
            case vec_cond_expr_K:
            case vec_perm_expr_K:
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
               THROW_ERROR(std::string("tree node not currently supported ") + op0->get_kind_text());
         }
         /// Print parameters.
         res += "(";
         if(is_va_start_end)
         {
            THROW_ASSERT(ce->args.size(), "va_start or va_end have to have arguments");
            const tree_nodeRef par1 = GET_NODE(ce->args[0]);
            const unsigned int par1_index = GET_INDEX_NODE(ce->args[0]);
            // print the first removing the address
            if(GetPointer<addr_expr>(par1))
            {
               res += print_node(GET_INDEX_NODE(GetPointer<addr_expr>(par1)->op), v, vppf);
            }
            else if(GetPointer<var_decl>(par1) || GetPointer<ssa_name>(par1))
            {
               res += "*(" + print_node(par1_index, v, vppf) + ")";
            }
            else
               THROW_ERROR("expected an address or a variable " + boost::lexical_cast<std::string>(par1_index));
            size_t arg_index;
            for(arg_index = 1; arg_index < ce->args.size(); arg_index++)
            {
               res += ", ";
               res += print_node(GET_INDEX_NODE(ce->args[arg_index]), v, vppf);
            }
         }
         else if(is_sizeof)
         {
            THROW_ASSERT(ce->args.size() == 1, "Wrong number of arguments: " + boost::lexical_cast<std::string>(ce->args.size()));
            const tree_nodeRef arg1 = GET_NODE(ce->args[0]);
#ifndef NDEBUG
            THROW_ASSERT(GetPointer<addr_expr>(arg1), "Argument is not an addr_expr but a " + std::string(arg1->get_kind_text()));
            const addr_expr* ae = GetPointer<addr_expr>(arg1);
            if(GetPointer<array_ref>(GET_NODE(ae->op)))
            {
               THROW_ASSERT(GetPointer<array_ref>(GET_NODE(ae->op)), "Argument is not an array ref but a " + std::string(GET_NODE(ae->op)->get_kind_text()));
               const array_ref* ar = GetPointer<array_ref>(GET_NODE(ae->op));
               THROW_ASSERT(GetPointer<string_cst>(GET_NODE(ar->op0)), "Argument is not a string cast but a " + std::string(GET_NODE(ar->op0)->get_kind_text()));
            }
            else
            {
               THROW_ASSERT(GetPointer<string_cst>(GET_NODE(ae->op)), "Argument is not a string cast but a " + std::string(GET_NODE(ae->op)->get_kind_text()));
            }
#endif
            std::string argument = print_node(GET_INDEX_NODE(ce->args[0]), v, vppf);
            std::string unquoted_argument = argument.substr(1, argument.size() - 2);
            res += unquoted_argument;
         }
         else
         {
            if(ce->args.size())
            {
               const std::vector<tree_nodeRef>& actual_args = ce->args;
               std::vector<tree_nodeRef> formal_args;
               if(fd)
                  formal_args = fd->list_of_args;
               std::vector<tree_nodeRef>::const_iterator actual_arg, actual_arg_end = actual_args.end();
               std::vector<tree_nodeRef>::const_iterator formal_arg, formal_arg_end = formal_args.end();
               for(actual_arg = actual_args.begin(), formal_arg = formal_args.begin(); actual_arg != actual_arg_end; ++actual_arg)
               {
                  if(formal_arg != formal_arg_end and (is_a_struct(GET_INDEX_NODE(*actual_arg)) or is_an_union(GET_INDEX_NODE(*actual_arg))) and (is_a_struct(GET_INDEX_NODE(*formal_arg)) or is_an_union(GET_INDEX_NODE(*formal_arg))) and
                     (tree_helper::GetRealType(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(*actual_arg))) != tree_helper::GetRealType(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(*formal_arg)))))
                  {
                     THROW_ERROR_CODE(C_EC, "Implicit struct type defintion not supported in gimple assignment " + boost::lexical_cast<std::string>(index));
                  }
                  if(actual_arg != actual_args.begin())
                     res += ", ";
                  res += print_node(GET_INDEX_NODE(*actual_arg), v, vppf);
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
         auto* ae = GetPointer<gimple_asm>(node);
         std::vector<tree_nodeRef> pragmas = ae->pragmas;
         std::vector<tree_nodeRef>::const_iterator pragma, pragma_end = pragmas.end();
         for(pragma = pragmas.begin(); pragma != pragma_end; ++pragma)
         {
            res += print_node(GET_INDEX_NODE(*pragma), v, vppf);
         }
         res += "asm";
         if(ae->volatile_flag)
            res += " __volatile__ ";
         res += "(\"" + ae->str + "\"";
         if(ae->out)
         {
            res += ":";
            auto* tl = GetPointer<tree_list>(GET_NODE(ae->out));
            std::string out_string;
            do
            {
               auto* tl_purp = GetPointer<tree_list>(GET_NODE(tl->purp));
               out_string = "";
               do
               {
                  out_string += print_constant(GET_INDEX_NODE(tl_purp->valu), vppf);
                  if(tl_purp->chan)
                     tl_purp = GetPointer<tree_list>(GET_NODE(tl_purp->chan));
                  else
                     tl_purp = nullptr;
               } while(tl_purp);
               if(out_string == "\"=\"")
                  out_string = "\"=r\"";
               res += out_string;
               res += "(" + print_node(GET_INDEX_NODE(tl->valu), v, vppf) + ")";
               if(tl->chan)
               {
                  res += ",";
                  tl = GetPointer<tree_list>(GET_NODE(tl->chan));
               }
               else
                  tl = nullptr;
            } while(tl);
         }
         else if(ae->in || ae->clob)
            res += ":";
         if(ae->in)
         {
            res += ":";
            auto* tl = GetPointer<tree_list>(GET_NODE(ae->in));
            std::string in_string;
            do
            {
               auto* tl_purp = GetPointer<tree_list>(GET_NODE(tl->purp));
               in_string = "";
               do
               {
                  in_string += print_constant(GET_INDEX_NODE(tl_purp->valu), vppf);
                  if(tl_purp->chan)
                     tl_purp = GetPointer<tree_list>(GET_NODE(tl_purp->chan));
                  else
                     tl_purp = nullptr;
               } while(tl_purp);
               if(in_string == "\"\"")
                  in_string = "\"r\"";
               res += in_string;
               res += "(" + print_node(GET_INDEX_NODE(tl->valu), v, vppf) + ")";
               if(tl->chan)
               {
                  res += ",";
                  tl = GetPointer<tree_list>(GET_NODE(tl->chan));
               }
               else
                  tl = nullptr;
            } while(tl);
         }
         else if(ae->clob)
            res += ":";
         if(ae->clob)
         {
            res += ":";
            auto* tl = GetPointer<tree_list>(GET_NODE(ae->clob));
            do
            {
               res += print_node(GET_INDEX_NODE(tl->valu), v, vppf);
               if(tl->chan)
               {
                  res += " ";
                  tl = GetPointer<tree_list>(GET_NODE(tl->chan));
               }
               else
                  tl = nullptr;
            } while(tl);
         }
         res += ")";
         break;
      }
      case gimple_phi_K:
      {
         auto* pn = GetPointer<gimple_phi>(node);
         res += "/* " + print_node(GET_INDEX_NODE(pn->res), v, vppf) + " = gimple_phi(";
         for(const auto& def_edge : pn->CGetDefEdgesList())
         {
            if(def_edge != pn->CGetDefEdgesList().front())
               res += ", ";
            res += "<" + print_node(GET_INDEX_NODE(def_edge.first), v, vppf) + ", BB" + STR(def_edge.second) + ">";
         }
         res += ") */";
         break;
      }
      case ssa_name_K:
      {
         res += (*vppf)(index);
         // res += print_node(GET_INDEX_NODE(sn->var), v, vppf);
         /*if (!sn->volatile_flag || GET_NODE(sn->CGetDefStmt())->get_kind() == gimple_nop_K)
          res += "_" + boost::lexical_cast<std::string>(sn->vers);*/
         break;
      }
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      {
         res = print_constant(index, vppf);
         break;
      }
      case var_decl_K:
      case result_decl_K:
      case parm_decl_K:
      {
         res = (*vppf)(index);
         break;
      }
      case field_decl_K:
      {
         res = PrintVariable(index);
         break;
      }
      case label_decl_K:
      {
         auto* ld = GetPointer<label_decl>(node);
         if(ld->name)
         {
            auto* id = GetPointer<identifier_node>(GET_NODE(ld->name));
            res = id->strg;
         }
         else
            res = "_unnamed_label_" + boost::lexical_cast<std::string>(index);
         break;
      }
      case gimple_label_K:
      {
         auto* le = GetPointer<gimple_label>(node);
         if(!GetPointer<label_decl>(GET_NODE(le->op))->artificial_flag)
         {
            res += print_node(GET_INDEX_NODE(le->op), v, vppf);
            res += ":";
         }
         break;
      }
      case gimple_goto_K:
      {
         auto* ge = GetPointer<gimple_goto>(node);
         bool is_a_label = GetPointer<label_decl>(GET_NODE(ge->op)) != nullptr;
         res += (is_a_label ? "goto " : "goto *") + print_node(GET_INDEX_NODE(ge->op), v, vppf);
         break;
      }
      case constructor_K:
      {
         res += print_init(index, vppf);
         break;
      }
      case with_size_expr_K:
      {
         auto* wse = GetPointer<with_size_expr>(node);
         res += print_node(GET_INDEX_NODE(wse->op0), v, vppf);
         break;
      }
      case gimple_predict_K:
      case null_node_K:
      {
         break;
      }
      case gimple_pragma_K:
      {
         const auto* pn = GetPointer<const gimple_pragma>(node);
#if 0
         if(pn->directive and (GetPointer<omp_for_pragma>(GET_NODE(pn->directive)) or GetPointer<omp_simd_pragma>(GET_NODE(pn->directive))))
         {
            break;
         }
#endif
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
               res += print_node(GET_INDEX_NODE(pn->scope), v, vppf);
               res += " ";
               res += print_node(GET_INDEX_NODE(pn->directive), v, vppf);
               if(pn->is_block)
                  res += "\n{";
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
         auto* fp = GetPointer<omp_for_pragma>(node);
         res += "for ";
         /// now print clauses
         for(auto& clause : fp->clauses)
            res += " " + clause.first + "(" + clause.second + ")";
         break;
      }
      case omp_parallel_pragma_K:
      {
         auto* pn = GetPointer<omp_parallel_pragma>(node);
         if(!pn->is_shortcut)
         {
            res += "parallel";
         }
         /// now print clauses
         for(auto& clause : pn->clauses)
            res += " " + clause.first + "(" + clause.second + ")";
         break;
      }
      case omp_sections_pragma_K:
      {
         auto* pn = GetPointer<omp_sections_pragma>(node);
         if(!pn->is_shortcut)
         {
            res += "sections";
         }
         break;
      }
      case omp_parallel_sections_pragma_K:
      {
         res += "parallel sections";
         auto* pn = GetPointer<omp_parallel_sections_pragma>(node);
         res += print_node(GET_INDEX_NODE(pn->op0), v, vppf);
         res += " ";
         res += print_node(GET_INDEX_NODE(pn->op1), v, vppf);
         break;
      }
      case omp_section_pragma_K:
      {
         res += "section";
         break;
      }
      case omp_declare_simd_pragma_K:
      {
         auto* fp = GetPointer<omp_declare_simd_pragma>(node);
         res += "declare simd ";
         /// now print clauses
         for(auto& clause : fp->clauses)
            res += " " + clause.first + "(" + clause.second + ")";
         break;
      }
      case omp_simd_pragma_K:
      {
         auto* fp = GetPointer<omp_simd_pragma>(node);
         res += "simd ";
         /// now print clauses
         for(auto& clause : fp->clauses)
            res += " " + clause.first + "(" + clause.second + ")";
         break;
      }
      case omp_critical_pragma_K:
      {
         res += "critical";
         const auto* ocp = GetPointer<const omp_critical_pragma>(node);
         for(const auto& clause : ocp->clauses)
         {
            res += " " + clause.first + "(" + clause.second + ")";
         }
         break;
      }
      case omp_target_pragma_K:
      {
         res += "target";
         const auto* otp = GetPointer<const omp_target_pragma>(node);
         const CustomUnorderedMapUnstable<std::string, std::string>& clauses = otp->clauses;
         CustomUnorderedMapUnstable<std::string, std::string>::const_iterator clause, clause_end = clauses.end();
         for(clause = clauses.begin(); clause != clause_end; ++clause)
         {
            res += " " + clause->first + "(" + clause->second + ")";
         }
         break;
      }
      case omp_task_pragma_K:
      {
         res += "task";
         const auto* otp = GetPointer<const omp_task_pragma>(node);
         const CustomUnorderedMapUnstable<std::string, std::string>& clauses = otp->clauses;
         CustomUnorderedMapUnstable<std::string, std::string>::const_iterator clause, clause_end = clauses.end();
         for(clause = clauses.begin(); clause != clause_end; ++clause)
         {
            res += " " + clause->first + "(" + clause->second + ")";
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
         auto* ch = GetPointer<call_hw_pragma>(node);
         res += ch->HW_component;
         if(ch->ID_implementation.size())
            res += " " + boost::lexical_cast<std::string>(ch->ID_implementation);
         break;
      }
      case call_point_hw_pragma_K:
      {
         res += STR_CST_pragma_keyword_call_point_hw " ";
         auto* ch = GetPointer<call_point_hw_pragma>(node);
         res += ch->HW_component;
         if(ch->ID_implementation.size())
            res += " " + boost::lexical_cast<std::string>(ch->ID_implementation);
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
         auto* ae = GetPointer<assert_expr>(node);
         res += print_node(GET_INDEX_NODE(ae->op0), v, vppf) + "/* " + print_node(GET_INDEX_NODE(ae->op1), v, vppf) + "*/";
         break;
      }
      case reduc_max_expr_K:
      {
         auto* rme = GetPointer<reduc_max_expr>(node);
         res += "/*reduc_max_expr*/" + print_node(GET_INDEX_NODE(rme->op), v, vppf);
         break;
      }
      case reduc_min_expr_K:
      {
         auto* rme = GetPointer<reduc_min_expr>(node);
         res += "/*reduc_min_expr*/" + print_node(GET_INDEX_NODE(rme->op), v, vppf);
         break;
      }
      case reduc_plus_expr_K:
      {
         auto* rpe = GetPointer<reduc_plus_expr>(node);
         res += "/*reduc_plus_expr*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(rpe->type)) + ") ";
         res += "{";
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(rpe->type)));
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(rpe->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int vector_size = size / element_size;
         res += print_node(GET_INDEX_NODE(rpe->op), v, vppf) + "[0]";
         for(unsigned int ind = 1; ind < vector_size; ++ind)
            res += "+" + print_node(GET_INDEX_NODE(rpe->op), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
         for(unsigned int ind = 1; ind < vector_size; ++ind)
            res += ", 0";
         res += "}";
         break;
      }
      case vec_unpack_hi_expr_K:
      {
         auto* vuh = GetPointer<vec_unpack_hi_expr>(node);
         tree_nodeRef op = GET_NODE(vuh->op);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vuh->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vuh->type)));
         const unsigned int vector_size = size / element_size;
         res += "/*" + vuh->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vuh->type)) + ") ";
         res += "{";
         if(op->get_kind() == vector_cst_K)
         {
            auto* vc = GetPointer<vector_cst>(op);
            for(auto i = static_cast<unsigned int>((vc->list_of_valu).size() / 2); i < (vc->list_of_valu).size(); i++) // vector elements
            {
               res += "((" + tree_helper::print_type(TM, element_type) + ") (";
               res += print_constant(GET_INDEX_NODE(vc->list_of_valu[i]), vppf);
               res += "))";
               if(i != (vc->list_of_valu).size() - 1) // not the last element element
                  res += ", ";
            }
         }
         else
         {
            for(unsigned int ind = vector_size; ind < 2 * vector_size; ++ind)
            {
               res += "((" + tree_helper::print_type(TM, element_type) + ") (" + print_node(GET_INDEX_NODE(vuh->op), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]))";
               if(ind != 2 * vector_size - 1)
                  res += ", ";
            }
         }
         res += "}";
         break;
      }
      case vec_unpack_lo_expr_K:
      {
         auto* vul = GetPointer<vec_unpack_lo_expr>(node);
         tree_nodeRef op = GET_NODE(vul->op);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vul->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vul->type)));
         const unsigned int vector_size = size / element_size;
         res += "/*" + vul->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vul->type)) + ") ";
         res += "{";
         if(op->get_kind() == vector_cst_K)
         {
            auto* vc = GetPointer<vector_cst>(op);
            for(unsigned int i = 0; i < (vc->list_of_valu).size() / 2; i++) // vector elements
            {
               res += "((" + tree_helper::print_type(TM, element_type) + ") (";
               res += print_constant(GET_INDEX_NODE(vc->list_of_valu[i]), vppf);
               res += "))";
               if(i != (vc->list_of_valu).size() / 2 - 1) // not the last element element
                  res += ", ";
            }
         }
         else
         {
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "((" + tree_helper::print_type(TM, element_type) + ") (" + print_node(GET_INDEX_NODE(vul->op), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]))";
               if(ind != vector_size - 1)
                  res += ", ";
            }
         }
         res += "}";
         break;
      }
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      {
         auto* vie = GetPointer<unary_expr>(node);
         res += "/*" + vie->get_kind_text() + "*/" + print_node(GET_INDEX_NODE(vie->op), v, vppf);
         break;
      }
      case paren_expr_K:
      {
         auto* vie = GetPointer<unary_expr>(node);
         res += "(" + print_node(GET_INDEX_NODE(vie->op), v, vppf) + ")";
         break;
      }

      case vec_pack_trunc_expr_K:
      {
         auto* vpt = GetPointer<vec_pack_trunc_expr>(node);
         tree_nodeRef op0 = GET_NODE(vpt->op0);
         tree_nodeRef op1 = GET_NODE(vpt->op1);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vpt->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vpt->type)));
         const unsigned int vector_size = size / element_size;
         res += "/*" + vpt->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vpt->type)) + ") ";
         res += "{";
         if(op0->get_kind() == vector_cst_K)
         {
            auto* vc = GetPointer<vector_cst>(op0);
            for(auto& i : (vc->list_of_valu)) // vector elements
            {
               res += "((" + tree_helper::print_type(TM, element_type) + ") (";
               res += print_constant(GET_INDEX_NODE(i), vppf);
               res += ")), ";
            }
         }
         else
         {
            res += "((" + tree_helper::print_type(TM, element_type) + ") (" + print_node(GET_INDEX_NODE(vpt->op0), v, vppf) + "[0])), ";
            for(unsigned int ind = 1; ind < vector_size / 2; ++ind)
               res += "((" + tree_helper::print_type(TM, element_type) + ") (" + print_node(GET_INDEX_NODE(vpt->op0), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "])), ";
         }

         if(op1->get_kind() == vector_cst_K)
         {
            auto* vc = GetPointer<vector_cst>(op1);
            for(unsigned int i = 0; i < (vc->list_of_valu).size(); i++) // vector elements
            {
               res += "((" + tree_helper::print_type(TM, element_type) + ") (";
               res += print_constant(GET_INDEX_NODE(vc->list_of_valu[i]), vppf);
               res += "))";
               if(i != (vc->list_of_valu).size() - 1) // not the last element element
                  res += ", ";
            }
         }
         else
         {
            res += "((" + tree_helper::print_type(TM, element_type) + ") (" + print_node(GET_INDEX_NODE(vpt->op1), v, vppf) + "[0]))";
            for(unsigned int ind = 1; ind < vector_size / 2; ++ind)
               res += ", ((" + tree_helper::print_type(TM, element_type) + ") (" + print_node(GET_INDEX_NODE(vpt->op1), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]))";
         }
         res += "}";
         break;
      }
      case dot_prod_expr_K:
      {
         auto* dpe = GetPointer<ternary_expr>(node);
         unsigned int two_op_type_index = 0;
         tree_helper::get_type_node(GET_NODE(dpe->op2), two_op_type_index);
         const unsigned int element_type = tree_helper::GetElements(TM, two_op_type_index);
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, two_op_type_index));
         const unsigned int vector_size = size / element_size;

         res += "/*" + dpe->get_kind_text() + "*/";
         if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(dpe->type)) && tree_helper::is_a_vector(TM, two_op_type_index) && two_op_type_index != GET_INDEX_NODE(dpe->type))
            res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(dpe->type)) + ")(";
         res += "(" + tree_helper::print_type(TM, two_op_type_index) + ")";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += "(" + print_node(GET_INDEX_NODE(dpe->op0), v, vppf) + "[" + boost::lexical_cast<std::string>(2 * ind) + "]" + " * " + print_node(GET_INDEX_NODE(dpe->op1), v, vppf) + "[" + boost::lexical_cast<std::string>(2 * ind) + "]" + ")";
            res += "+(" + print_node(GET_INDEX_NODE(dpe->op0), v, vppf) + "[" + boost::lexical_cast<std::string>(2 * ind + 1) + "]" + " * " + print_node(GET_INDEX_NODE(dpe->op1), v, vppf) + "[" + boost::lexical_cast<std::string>(2 * ind + 1) + "]" + ")";
            if(ind != (vector_size - 1))
               res += ", ";
         }
         res += "}";
         res += " + " + print_node(GET_INDEX_NODE(dpe->op2), v, vppf);
         if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(dpe->type)) && tree_helper::is_a_vector(TM, two_op_type_index) && two_op_type_index != GET_INDEX_NODE(dpe->type))
            res += ")";
         break;
      }
      case widen_mult_hi_expr_K:
      {
         auto* wmhe = GetPointer<widen_mult_hi_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(wmhe->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(wmhe->type)));
         const unsigned int vector_size = size / element_size;

         res += "/*" + wmhe->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(wmhe->type)) + ") ";
         res += "{";
         for(unsigned int ind = vector_size; ind < vector_size * 2; ++ind)
         {
            res += print_node(GET_INDEX_NODE(wmhe->op0), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            res += " * ";
            res += print_node(GET_INDEX_NODE(wmhe->op1), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            if(ind != (vector_size * 2 - 1))
               res += ", ";
         }
         res += "}";
         break;
      }
      case widen_mult_lo_expr_K:
      {
         auto* wmle = GetPointer<widen_mult_lo_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(wmle->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(wmle->type)));
         const unsigned int vector_size = size / element_size;

         res += "/*" + wmle->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(wmle->type)) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += print_node(GET_INDEX_NODE(wmle->op0), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            res += " * ";
            res += print_node(GET_INDEX_NODE(wmle->op1), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            if(ind != (vector_size - 1))
               res += ", ";
         }
         res += "}";
         break;
      }
      case vec_pack_sat_expr_K:
      case vec_pack_fix_trunc_expr_K:
      {
         auto* vie = GetPointer<binary_expr>(node);
         res += "/*" + vie->get_kind_text() + "*/" + print_node(GET_INDEX_NODE(vie->op0), v, vppf) + " /**/ " + print_node(GET_INDEX_NODE(vie->op1), v, vppf);
         break;
      }
      case vec_extracteven_expr_K:
      {
         auto* vee = GetPointer<vec_extracteven_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vee->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vee->type)));
         const unsigned int vector_size = size / element_size;

         res += "/*" + vee->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vee->type)) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ind += 2)
         {
            res += print_node(GET_INDEX_NODE(vee->op0), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            res += ", ";
         }
         for(unsigned int ind = 0; ind < vector_size; ind += 2)
         {
            res += print_node(GET_INDEX_NODE(vee->op1), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            if(ind != vector_size - 2)
               res += ", ";
         }
         res += "}";
         break;
      }
      case vec_extractodd_expr_K:
      {
         auto* vee = GetPointer<vec_extractodd_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vee->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vee->type)));
         const unsigned int vector_size = size / element_size;

         res += "/*" + vee->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vee->type)) + ") ";
         res += "{";
         for(unsigned int ind = 1; ind < vector_size; ind += 2)
         {
            res += print_node(GET_INDEX_NODE(vee->op0), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            res += ", ";
         }
         for(unsigned int ind = 1; ind < vector_size; ind += 2)
         {
            res += print_node(GET_INDEX_NODE(vee->op1), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            if(ind != vector_size - 1)
               res += ", ";
         }
         res += "}";
         break;
      }
      case vec_interleavehigh_expr_K:
      {
         auto* vie = GetPointer<vec_interleavehigh_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vie->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vie->type)));
         const unsigned int vector_size = size / element_size;

         res += "/*" + vie->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vie->type)) + ") ";
         res += "{";
         for(unsigned int ind = vector_size / 2; ind < vector_size; ++ind)
         {
            res += print_node(GET_INDEX_NODE(vie->op0), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            res += ", ";
            res += print_node(GET_INDEX_NODE(vie->op1), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            if(ind != vector_size - 1)
               res += ", ";
         }
         res += "}";
         break;
      }
      case vec_interleavelow_expr_K:
      {
         auto* vie = GetPointer<vec_interleavelow_expr>(node);
         const unsigned int element_type = tree_helper::GetElements(TM, GET_INDEX_NODE(vie->type));
         const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TM, element_type));
         const unsigned int size = static_cast<unsigned int>(tree_helper::size(TM, GET_INDEX_NODE(vie->type)));
         const unsigned int vector_size = size / element_size;

         res += "/*" + vie->get_kind_text() + "*/";
         res += "(" + tree_helper::print_type(TM, GET_INDEX_NODE(vie->type)) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size / 2; ++ind)
         {
            res += print_node(GET_INDEX_NODE(vie->op0), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            res += ", ";
            res += print_node(GET_INDEX_NODE(vie->op1), v, vppf) + "[" + boost::lexical_cast<std::string>(ind) + "]";
            if(ind != (vector_size / 2) - 1)
               res += ", ";
         }
         res += "}";
         break;
      }
      case case_label_expr_K:
      {
         res = print_constant(index, vppf);
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Printed node " + boost::lexical_cast<std::string>(index) + " res = " + res);

   return res;
}

std::string BehavioralHelper::print_type_declaration(unsigned int type) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Printing type declaration " + boost::lexical_cast<std::string>(type));
   std::string res;
   const tree_nodeRef node_type = TM->get_tree_node_const(type);
   switch(node_type->get_kind())
   {
      case record_type_K:
      {
         auto* rt = GetPointer<record_type>(node_type);
         THROW_ASSERT(tree_helper::GetRealType(TM, type) == type, "Printing declaration of fake type " + boost::lexical_cast<std::string>(type));
         if(rt->unql)
         {
            res += "typedef ";
         }
         const auto qualifiers = rt->qual;
         if(qualifiers != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            res += tree_helper::return_C_qualifiers(qualifiers, false);
         res += "struct ";
         if(not rt->unql)
         {
            if(rt->packed_flag)
            {
               res += " __attribute__((packed)) ";
            }
            if(rt->name)
            {
               res += tree_helper::print_type(TM, GET_INDEX_NODE(rt->name)) + " ";
            }
            else
            {
               res += "Internal_" + boost::lexical_cast<std::string>(type) + " ";
            }
         }
         if(not rt->unql or (not GetPointer<record_type>(GET_NODE(rt->unql))->name and not Param->getOption<bool>(OPT_without_transformation)))
         {
            /// Print the contents of the structure
            res += "\n{\n";
            null_deleter null_del;
            const var_pp_functorConstRef std_vpf(new std_var_pp_functor(BehavioralHelperConstRef(this, null_del)));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Printing content of the structure");
            for(auto& list_of_fld : rt->list_of_flds)
            {
               unsigned int field = GET_INDEX_NODE(list_of_fld);
               auto fld_node = TM->get_tree_node_const(field);
               if(fld_node->get_kind() == type_decl_K)
                  continue;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Printing field " + boost::lexical_cast<std::string>(field));
               const field_decl* fd = GetPointer<field_decl>(fld_node);
               const unsigned int field_type_index = get_type(field);
               if(has_bit_field(field))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bit field");
                  res += tree_helper::print_type(TM, field_type_index) + " ";
                  res += (*std_vpf)(field);
                  res += " : ";
                  res += print_constant(GET_INDEX_NODE(fd->size));
               }
               else
               {
                  res += tree_helper::print_type(TM, field_type_index, false, false, false, field, std_vpf);
               }
               if(fd && fd->packed_flag)
               {
                  res += " __attribute__((packed))";
               }
               res += ";\n";
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            res += '}';
            res += " ";
         }
         if(rt->unql and (rt->name or Param->getOption<bool>(OPT_without_transformation)))
         {
            const record_type* rt_unqal = GetPointer<record_type>(GET_NODE(rt->unql));
            if(rt_unqal->name)
            {
               res += tree_helper::print_type(TM, GET_INDEX_NODE(rt_unqal->name)) + " ";
            }
            else if(Param->getOption<bool>(OPT_without_transformation))
            {
               res += "Internal_" + boost::lexical_cast<std::string>(GET_INDEX_NODE(rt->unql)) + " ";
            }
            res += tree_helper::print_type(TM, GET_INDEX_NODE(rt->name));
         }
         if(rt->unql and rt->algn != GetPointer<record_type>(GET_NODE(rt->unql))->algn)
         {
            res += " __attribute__ ((aligned (" + boost::lexical_cast<std::string>(rt->algn / 8) + ")))";
         }
         break;
      }
      case union_type_K:
      {
         auto* ut = GetPointer<union_type>(node_type);
         THROW_ASSERT(tree_helper::GetRealType(TM, type) == type, "Printing declaration of fake type " + boost::lexical_cast<std::string>(node_type));
         if(ut->unql)
         {
            res += "typedef ";
         }
         const auto qualifiers = ut->qual;
         if(qualifiers != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
            res += tree_helper::return_C_qualifiers(qualifiers, false);
         res += "union ";
         if(not ut->unql)
         {
            if(ut->packed_flag)
            {
               res += " __attribute__((packed)) ";
            }
            if(ut->name)
            {
               res += tree_helper::print_type(TM, GET_INDEX_NODE(ut->name)) + " ";
            }
            else
            {
               res += "Internal_" + boost::lexical_cast<std::string>(type) + " ";
            }
         }
         if(not ut->unql or (not GetPointer<union_type>(GET_NODE(ut->unql))->name and not Param->getOption<bool>(OPT_without_transformation)))
         {
            /// Print the contents of the structure
            res += "\n{\n";
            null_deleter null_del;
            const var_pp_functorConstRef std_vpf(new std_var_pp_functor(BehavioralHelperConstRef(this, null_del)));
            for(auto& list_of_fld : ut->list_of_flds)
            {
               unsigned int field = GET_INDEX_NODE(list_of_fld);
               res += tree_helper::print_type(TM, get_type(field), false, false, false, field, std_vpf);
               res += ";\n";
            }
            res += '}';
            res += " ";
         }
         if(ut->unql and (ut->name or Param->getOption<bool>(OPT_without_transformation)))
         {
            const union_type* ut_unqal = GetPointer<union_type>(GET_NODE(ut->unql));
            if(ut_unqal->name)
            {
               res += tree_helper::print_type(TM, GET_INDEX_NODE(ut_unqal->name)) + " ";
            }
            else if(Param->getOption<bool>(OPT_without_transformation))
            {
               res += "Internal_" + boost::lexical_cast<std::string>(GET_INDEX_NODE(ut->unql)) + " ";
            }
            else
            {
               THROW_UNREACHABLE("");
            }
            res += tree_helper::print_type(TM, GET_INDEX_NODE(ut->name));
         }
         if(ut->unql and ut->algn != GetPointer<union_type>(GET_NODE(ut->unql))->algn)
            res += " __attribute__ ((aligned (" + boost::lexical_cast<std::string>(ut->algn / 8) + "))) ";
         break;
      }
      case enumeral_type_K:
      {
         const enumeral_type* et = GetPointer<enumeral_type>(node_type);
         if(et->unql)
         {
            res += "typedef ";
         }
         const auto quals = et->qual;
         if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
         {
            res += tree_helper::return_C_qualifiers(quals, false);
         }
         res += "enum ";
         if(not et->unql)
         {
            if(et->packed_flag)
            {
               res += " __attribute__((packed)) ";
            }
            if(et->name)
            {
               res += tree_helper::print_type(TM, GET_INDEX_NODE(et->name)) + " ";
            }
            else
            {
               res += "Internal_" + boost::lexical_cast<std::string>(type) + " ";
            }
         }
         if(not et->unql or not GetPointer<enumeral_type>(GET_NODE(et->unql))->name)
         {
            res += "{";
            auto* tl = GetPointer<tree_list>(GET_NODE(et->csts));
            while(tl)
            {
               res += tree_helper::print_type(TM, GET_INDEX_NODE(tl->purp));
               if(tl->valu)
               {
                  res += " = " + print_constant(GET_INDEX_NODE(tl->valu));
               }
               if(tl->chan)
               {
                  res += ",";
                  tl = GetPointer<tree_list>(GET_NODE(tl->chan));
               }
               else
                  tl = nullptr;
            }
            res += "}";
         }
         if(et->unql and et->name)
         {
            const enumeral_type* et_unql = GetPointer<enumeral_type>(GET_NODE(et->unql));
            if(et_unql->name)
            {
               res += tree_helper::print_type(TM, GET_INDEX_NODE(et_unql->name)) + " ";
            }
            res += tree_helper::print_type(TM, GET_INDEX_NODE(et->name));
         }
         break;
      }
      case array_type_K:
      {
         const array_type* at = GetPointer<array_type>(node_type);
         /// Compute the dimensions
         if(not at->size)
         {
            THROW_ERROR_CODE(C_EC, "Declaration of array type without size");
         }
         const tree_nodeRef array_length = GET_NODE(at->size);
         const tree_nodeRef array_t = GET_NODE(at->elts);
         if(array_length->get_kind() != integer_cst_K)
         {
            THROW_ERROR_CODE(C_EC, "Declaration of array type without fixed size");
         }
         const integer_cst* arr_ic = GetPointer<integer_cst>(array_length);
         const type_node* tn = GetPointer<type_node>(array_t);
         const integer_cst* eln_ic = GetPointer<integer_cst>(GET_NODE(tn->size));

         res += "typedef ";
         res += tree_helper::print_type(TM, GET_INDEX_NODE(at->elts));
         res += " ";
         THROW_ASSERT(at->name, "Trying to declare array without name " + boost::lexical_cast<std::string>(type));
         res += tree_helper::print_type(TM, GET_INDEX_NODE(at->name));
         res += "[";
         res += boost::lexical_cast<std::string>(tree_helper::get_integer_cst_value(arr_ic) / tree_helper::get_integer_cst_value(eln_ic));
         res += "]";
         break;
      }
      /// NOTE: this case cannot be moved because of break absence
      case pointer_type_K:
      {
         const pointer_type* pt = GetPointer<pointer_type>(node_type);
         if(pt->unql && GET_NODE(pt->ptd)->get_kind() == function_type_K)
         {
            auto* ft = GetPointer<function_type>(GET_NODE(pt->ptd));
            const auto quals = GetPointer<type_node>(node_type)->qual;
            res += "typedef ";
            res += tree_helper::print_type(TM, GET_INDEX_NODE(ft->retn));
            res += " (* ";
            if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
               res += tree_helper::return_C_qualifiers(quals, false);
            if(GetPointer<type_node>(node_type)->name)
               res += tree_helper::print_type(TM, GET_INDEX_NODE(GetPointer<type_node>(node_type)->name));
            else
               res += "Internal_" + boost::lexical_cast<std::string>(type);
            res += ") (";
            if(ft->prms)
            {
               res += tree_helper::print_type(TM, GET_INDEX_NODE(ft->prms));
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
         if(GetPointer<type_node>(node_type)->unql)
         {
            const auto quals = GetPointer<type_node>(node_type)->qual;
            res += "typedef ";

            if(quals != TreeVocabularyTokenTypes_TokenEnum::FIRST_TOKEN)
               res += tree_helper::return_C_qualifiers(quals, false);

            res += tree_helper::print_type(TM, GET_INDEX_NODE(GetPointer<type_node>(node_type)->unql));
            res += " ";
            /*            if(GetPointer<type_node>(node_type)->algn != 8)
             res += "__attribute__ ((aligned (" + boost::lexical_cast<std::string>(GetPointer<type_node>(node_type)->algn/8) + "))) ";*/
            if(GetPointer<type_node>(node_type)->name)
            {
               res += tree_helper::print_type(TM, GET_INDEX_NODE(GetPointer<type_node>(node_type)->name));
            }
            else
               res += "Internal_" + boost::lexical_cast<std::string>(type);
         }
         else
         {
            THROW_ASSERT(node_type->get_kind() == integer_cst_K, "Expected an integer, got a " + node_type->get_kind_text());
            THROW_ASSERT(GetPointer<type_node>(node_type)->name, "Expected a typedef declaration with a name " + boost::lexical_cast<std::string>(type));
            tree_nodeRef name = GetPointer<type_node>(node_type)->name;
            /* #ifndef NDEBUG
                        identifier_node * id = GetPointer<identifier_node>(GET_NODE(name));
                        THROW_ASSERT(id && id->strg == "bit_size_type", "Expected bit_size_type " + boost::lexical_cast<std::string>(type));
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Printed type declaration " + boost::lexical_cast<std::string>(type));
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
   const tree_nodeRef temp = TM->get_tree_node_const(variable);
   if(temp->get_kind() == indirect_ref_K || temp->get_kind() == misaligned_indirect_ref_K)
      return true;
   else
      return false;
}

bool BehavioralHelper::is_an_array_ref(unsigned int variable) const
{
   const tree_nodeRef temp = TM->get_tree_node_const(variable);
   if(temp->get_kind() == array_ref_K)
      return true;
   else
      return false;
}

bool BehavioralHelper::is_a_component_ref(unsigned int variable) const
{
   const tree_nodeRef temp = TM->get_tree_node_const(variable);
   if(temp->get_kind() == component_ref_K)
      return true;
   else
      return false;
}

bool BehavioralHelper::is_an_addr_expr(unsigned int variable) const
{
   const tree_nodeRef temp = TM->get_tree_node_const(variable);
   if(temp->get_kind() == addr_expr_K)
      return true;
   else
      return false;
}

bool BehavioralHelper::is_a_mem_ref(unsigned int variable) const
{
   const tree_nodeRef temp = TM->get_tree_node_const(variable);
   if(temp->get_kind() == mem_ref_K)
      return true;
   else
      return false;
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
   const tree_nodeRef node = TM->get_tree_node_const(obj);
   switch(node->get_kind())
   {
      case addr_expr_K:
         return true;
      case paren_expr_K:
      case nop_expr_K:
      {
         auto* ue = GetPointer<unary_expr>(node);
         return is_a_constant(GET_INDEX_NODE(ue->op));
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
   const tree_nodeRef node = TM->get_tree_node_const(obj);
   if(node->get_kind() == result_decl_K)
      return true;
   else
      return false;
}

bool BehavioralHelper::is_a_realpart_expr(unsigned int obj) const
{
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   if(temp->get_kind() == realpart_expr_K)
      return true;
   else
      return false;
}

bool BehavioralHelper::is_a_imagpart_expr(unsigned int obj) const
{
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   if(temp->get_kind() == imagpart_expr_K)
      return true;
   else
      return false;
}

bool BehavioralHelper::is_operating_system_function(const unsigned int obj) const
{
   const tree_nodeRef curr_tn = TM->get_tree_node_const(obj);
   const function_decl* fd = GetPointer<function_decl>(curr_tn);
   if(!fd)
      return false;
   return fd->operating_system_flag;
}

unsigned int BehavioralHelper::get_indirect_ref_var(unsigned int obj) const
{
   THROW_ASSERT(is_an_indirect_ref(obj), "obj assumed to be an inderect_ref object");
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   auto* ir = GetPointer<indirect_ref>(temp);
   if(ir)
      return GET_INDEX_NODE(ir->op);
   auto* mir = GetPointer<misaligned_indirect_ref>(temp);
   return GET_INDEX_NODE(mir->op);
}

unsigned int BehavioralHelper::get_array_ref_array(unsigned int obj) const
{
   THROW_ASSERT(is_an_array_ref(obj), "obj assumed to be an array_ref object");
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   auto* ar = GetPointer<array_ref>(temp);
   return GET_INDEX_NODE(ar->op0);
}

unsigned int BehavioralHelper::get_array_ref_index(unsigned int obj) const
{
   THROW_ASSERT(is_an_array_ref(obj), "obj assumed to be an array_ref object");
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   auto* ar = GetPointer<array_ref>(temp);
   return GET_INDEX_NODE(ar->op1);
}

unsigned int BehavioralHelper::get_component_ref_record(unsigned int obj) const
{
   THROW_ASSERT(is_a_component_ref(obj), "obj assumed to be a component_ref object");
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   auto* cr = GetPointer<component_ref>(temp);
   return GET_INDEX_NODE(cr->op0);
}

unsigned int BehavioralHelper::get_component_ref_field(unsigned int obj) const
{
   THROW_ASSERT(is_a_component_ref(obj), "obj assumed to be a component_ref object");
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   auto* cr = GetPointer<component_ref>(temp);
   return GET_INDEX_NODE(cr->op1);
}

unsigned int BehavioralHelper::get_mem_ref_base(unsigned int obj) const
{
   THROW_ASSERT(is_a_mem_ref(obj), "obj assumed to be a mem_ref object");
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   auto* mr = GetPointer<mem_ref>(temp);
   return GET_INDEX_NODE(mr->op0);
}

unsigned int BehavioralHelper::get_mem_ref_offset(unsigned int obj) const
{
   THROW_ASSERT(is_a_mem_ref(obj), "obj assumed to be a mem_ref object");
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   auto* mr = GetPointer<mem_ref>(temp);
   return GET_INDEX_NODE(mr->op1);
}

unsigned int BehavioralHelper::get_operand_from_unary_expr(unsigned int obj) const
{
   THROW_ASSERT(is_an_addr_expr(obj) || is_a_realpart_expr(obj) || is_a_imagpart_expr(obj), "obj assumed to be an addr_expr, a realpart_expr or an imagpart_expr object. obj is " + boost::lexical_cast<std::string>(obj));
   const tree_nodeRef temp = TM->get_tree_node_const(obj);
   auto* ue = GetPointer<unary_expr>(temp);
   return GET_INDEX_NODE(ue->op);
}

unsigned int BehavioralHelper::GetVarFromSsa(unsigned int index) const
{
   const tree_nodeRef temp = TM->get_tree_node_const(index);
   auto* sn = GetPointer<ssa_name>(temp);
   if(sn)
      return GET_INDEX_NODE(sn->var);
   else
      return index;
}

unsigned int BehavioralHelper::get_intermediate_var(unsigned int obj) const
{
   const tree_nodeRef node = TM->get_tree_node_const(obj);
   switch(node->get_kind())
   {
      case modify_expr_K:
      case init_expr_K:
      {
         auto* be = GetPointer<binary_expr>(node);
         const tree_nodeRef right = GET_NODE(be->op1);
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
               auto* ue = GetPointer<unary_expr>(right);
               return GET_INDEX_NODE(ue->op);
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
               return GET_INDEX_NODE(be->op1);
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

const CustomUnorderedSet<unsigned int> BehavioralHelper::GetParameterTypes() const
{
   CustomUnorderedSet<unsigned int> ret;
   tree_nodeRef node = TM->get_tree_node_const(function_index);
   auto* fd = GetPointer<function_decl>(node);
   if(!fd)
      return ret;
   const std::vector<tree_nodeRef>& list_of_args = fd->list_of_args;
   std::vector<tree_nodeRef>::const_iterator it, it_end = list_of_args.end();
   for(it = list_of_args.begin(); it != it_end; ++it)
   {
      const parm_decl* pd = GetPointer<parm_decl>(GET_NODE(*it));
      ret.insert(GET_INDEX_NODE(pd->type));
   }
   auto* ft = GetPointer<function_type>(GET_NODE(fd->type));
   if(!ft || !ft->prms)
      return ret;
   auto* tl = GetPointer<tree_list>(GET_NODE(ft->prms));
   while(tl)
   {
      ret.insert(GET_INDEX_NODE(tl->valu));
      if(tl->chan)
         tl = GetPointer<tree_list>(GET_NODE(tl->chan));
      else
         break;
   }
   return ret;
}

unsigned int BehavioralHelper::is_named_pointer(const unsigned int index) const
{
   THROW_ASSERT(index, "this index does not exist: " + boost::lexical_cast<std::string>(index));
   unsigned int type = get_type(index);
   THROW_ASSERT(type, "this index does not exist: " + boost::lexical_cast<std::string>(type));
   const tree_nodeRef Type_node = TM->get_tree_node_const(type);
   THROW_ASSERT(Type_node, "this index does not exist: " + boost::lexical_cast<std::string>(type));
   if(Type_node->get_kind() == pointer_type_K)
   {
      auto* pt = GetPointer<pointer_type>(Type_node);
      if(pt->name)
         return type;
      else
         return is_named_pointer(GET_INDEX_NODE(pt->ptd));
   }
   else
      return 0;
}

bool BehavioralHelper::is_va_start_call(unsigned int stm) const
{
   if(is_var_args())
   {
      const tree_nodeRef node = TM->get_tree_node_const(stm);
      if(node->get_kind() == gimple_call_K)
      {
         auto* ce = GetPointer<gimple_call>(node);
         tree_nodeRef cefn = GET_NODE(ce->fn);
         if(cefn->get_kind() == addr_expr_K)
         {
            auto* ue = GetPointer<unary_expr>(cefn);
            auto* fd = GetPointer<function_decl>(GET_NODE(ue->op));
            if(fd and tree_helper::print_function_name(TM, fd) == "__builtin_va_start")
            {
               return true;
            }
            else
               return false;
         }
         else
            return false;
      }
      else
         return false;
   }
   else
      return false;
}

bool BehavioralHelper::has_bit_field(unsigned int variable) const
{
   const tree_nodeRef node = TM->get_tree_node_const(variable);
   if(node->get_kind() == field_decl_K)
   {
      auto* fd = GetPointer<field_decl>(node);
      if((fd->list_attr.find(TreeVocabularyTokenTypes_TokenEnum::TOK_BITFIELD)) != fd->list_attr.end())
         return true;
      else
         return false;
   }
   else
      return false;
}

unsigned int BehavioralHelper::get_attributes(unsigned int var) const
{
   const tree_nodeRef node = TM->get_tree_node_const(var);
   switch(node->get_kind())
   {
      case parm_decl_K:
      case ssa_name_K:
      {
         /*ssa_name * sn = GetPointer<ssa_name>(node);
          return get_attributes(GET_INDEX_NODE(sn->var));*/
         return 0;
      }
      case function_decl_K:
      case result_decl_K:
      case var_decl_K:
      {
         THROW_ASSERT(GetPointer<decl_node>(node), "get_attributes is only for decl_node: " + boost::lexical_cast<std::string>(var));
         return GetPointer<decl_node>(node)->attributes ? GET_INDEX_NODE(GetPointer<decl_node>(node)->attributes) : 0;
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Get init of " + PrintVariable(var));
   if(initializations.find(var) != initializations.end())
   {
      unsigned int init = var;
      while(initializations.find(init) != initializations.end())
         init = initializations.find(init)->second;
      if(TM->get_tree_node_const(init)->get_kind() == var_decl_K)
      {
         list_of_variables.insert(init);
         const unsigned int init_of_init = GetInit(init, list_of_variables);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Init is " + boost::lexical_cast<std::string>(init_of_init));
         return init_of_init;
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Init is " + boost::lexical_cast<std::string>(init));
         return init;
      }
   }
   const tree_nodeRef node = TM->get_tree_node_const(var);
   switch(node->get_kind())
   {
      case ssa_name_K:
      {
         auto* sn = GetPointer<ssa_name>(node);
         if(!sn->var)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Without init");
            return 0;
         }
         const unsigned ssa_init = GetInit(GET_INDEX_NODE(sn->var), list_of_variables);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Init is " + boost::lexical_cast<std::string>(ssa_init));
         return ssa_init;
      }
      case var_decl_K:
      {
         auto* vd = GetPointer<var_decl>(node);
         if(vd->init)
         {
            tree_helper::get_used_variables(true, vd->init, list_of_variables);
            const unsigned var_init = GET_INDEX_NODE(vd->init);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Init is " + boost::lexical_cast<std::string>(var_init));
            return var_init;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Without init");
            return 0;
         }
      }
      case constructor_K:
      {
         auto* co = GetPointer<constructor>(TM->get_tree_node_const(var));
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator vend = co->list_of_idx_valu.end();
         for(std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator i = co->list_of_idx_valu.begin(); i != vend; ++i)
         {
            tree_helper::get_used_variables(true, i->second, list_of_variables);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Init is " + boost::lexical_cast<std::string>(var));
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Without init");
         return 0;
      }
      default:
         THROW_UNREACHABLE("get_init: Tree node not yet supported " + boost::lexical_cast<std::string>(var) + " " + node->get_kind_text());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Without init");
   return 0;
}

std::string BehavioralHelper::print_phinode_res(unsigned int phi_node_id, vertex v, const var_pp_functorConstRef vppf) const
{
   const tree_nodeRef node = TM->get_tree_node_const(phi_node_id);
   auto* phi = GetPointer<gimple_phi>(node);

   THROW_ASSERT(phi, "NodeId is not related to a gimple_phi");

   return print_node(GET_INDEX_NODE(phi->res), v, vppf);
}

unsigned int BehavioralHelper::start_with_a_label(const blocRef& block) const
{
   if(block->CGetStmtList().empty())
      return 0;
   tree_nodeRef first_stmt = block->CGetStmtList().front();
   auto* le = GetPointer<gimple_label>(GET_NODE(first_stmt));
   if(le && le->op && GET_NODE(le->op)->get_kind() == label_decl_K)
   {
      auto* ld = GetPointer<label_decl>(GET_NODE(le->op));
      if(ld->name)
         return GET_INDEX_NODE(first_stmt);
      else
         return 0;
   }
   else
      return 0;
}

const std::string BehavioralHelper::get_label_name(unsigned int label_expr_nid) const
{
   tree_nodeRef tn = TM->get_tree_node_const(label_expr_nid);
   auto* le = GetPointer<gimple_label>(tn);
   THROW_ASSERT(le->op && GET_NODE(le->op)->get_kind() == label_decl_K, "label decl expected");
   auto* ld = GetPointer<label_decl>(GET_NODE(le->op));
   THROW_ASSERT(ld->name && GET_NODE(ld->name)->get_kind() == identifier_node_K, "identifier_node expected");
   auto* id = GetPointer<identifier_node>(GET_NODE(ld->name));
   return id->strg;
}

unsigned int BehavioralHelper::end_with_a_cond_or_goto(const blocRef& block) const
{
   if(block->CGetStmtList().empty())
      return 0;
   tree_nodeRef last = block->CGetStmtList().back();
   auto* gc = GetPointer<gimple_cond>(GET_NODE(last));
   if(gc)
      return GET_INDEX_NODE(last);
   auto* se = GetPointer<gimple_switch>(GET_NODE(last));
   if(se)
      return GET_INDEX_NODE(last);
   auto* ge = GetPointer<gimple_goto>(GET_NODE(last));
   if(ge)
      return GET_INDEX_NODE(last);
   auto* gmwi = GetPointer<gimple_multi_way_if>(GET_NODE(last));
   if(gmwi)
      return GET_INDEX_NODE(last);
   return 0;
}

void BehavioralHelper::create_gimple_modify_stmt(unsigned int, blocRef&, tree_nodeRef, tree_nodeRef)
{
   THROW_ERROR("Not implemented");
}

std::string BehavioralHelper::print_forward_declaration(unsigned int type) const
{
   std::string res;
   const tree_nodeRef node_type = TM->get_tree_node_const(type);
   switch(node_type->get_kind())
   {
      case record_type_K:
      {
         auto* rt = GetPointer<record_type>(node_type);
         if(rt->name)
            res += "struct " + tree_helper::print_type(TM, GET_INDEX_NODE(rt->name));
         else
            res += "struct Internal_" + boost::lexical_cast<std::string>(type);
         break;
      }
      case union_type_K:
      {
         auto* rt = GetPointer<union_type>(node_type);
         if(rt->name)
            res += "union " + tree_helper::print_type(TM, GET_INDEX_NODE(rt->name));
         else
            res += "union Internal_" + boost::lexical_cast<std::string>(type);
         break;
      }
      case enumeral_type_K:
      {
         auto* rt = GetPointer<enumeral_type>(node_type);
         if(rt->name)
            res += "enum " + tree_helper::print_type(TM, GET_INDEX_NODE(rt->name));
         else
            res += "enum Internal_" + boost::lexical_cast<std::string>(type);
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
   const tree_nodeRef retNode = TM->get_tree_node_const(index);
   auto* re = GetPointer<gimple_return>(retNode);
   THROW_ASSERT(re, "Expected a return statement");
   return !re->op;
}

unsigned int BehavioralHelper::GetUnqualified(const unsigned int index) const
{
   return tree_helper::GetUnqualified(TM, index);
}

std::string BehavioralHelper::print_type(unsigned int type, bool global, bool print_qualifiers, bool print_storage, unsigned int var, const var_pp_functorConstRef vppf, const std::string& prefix, const std::string& tail) const
{
   return tree_helper::print_type(TM, type, global, print_qualifiers, print_storage, var, vppf, prefix, tail);
}

void BehavioralHelper::rename_a_variable(unsigned int var, const std::string& new_name)
{
   vars_renaming_table[var] = new_name;
}

void BehavioralHelper::clear_renaming_table()
{
   vars_renaming_table.clear();
}

void BehavioralHelper::get_typecast(unsigned int nodeid, CustomUnorderedSet<unsigned int>& types) const
{
   type_casting Visitor(types);
   tree_nodeRef tn = TM->get_tree_node_const(nodeid);
   tn->visit(&Visitor);
}

bool BehavioralHelper::IsDefaultSsaName(const unsigned int ssa_name_index) const
{
   const auto* sn = GetPointer<const ssa_name>(TM->get_tree_node_const(ssa_name_index));
   return sn and sn->default_flag;
}

#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
size_t BehavioralHelper::GetOmpForDegree() const
{
   const auto fd = GetPointer<const function_decl>(TM->get_tree_node_const(function_index));
   return fd->omp_for_wrapper;
}

bool BehavioralHelper::IsOmpFunctionAtomic() const
{
   const auto fd = GetPointer<const function_decl>(TM->get_tree_node_const(function_index));
   return fd->omp_atomic;
}

bool BehavioralHelper::IsOmpBodyLoop() const
{
   const auto fd = GetPointer<const function_decl>(TM->get_tree_node_const(function_index));
   return fd->omp_body_loop;
}
#endif

#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
bool BehavioralHelper::IsOmpAtomic() const
{
   const auto fd = GetPointer<const function_decl>(TM->get_tree_node_const(function_index));
   return fd->omp_atomic;
}
#endif

bool BehavioralHelper::function_has_to_be_printed(unsigned int f_id) const
{
   if(function_name == "__builtin_cond_expr32")
      return false;
#if HAVE_BAMBU_BUILT
   if(tree_helper::IsInLibbambu(TM, f_id))
      return true;
#endif
   return not tree_helper::is_system(TM, f_id);
}

std::string BehavioralHelper::get_asm_string(const unsigned int node_index) const
{
   return tree_helper::get_asm_string(TM, node_index);
}

#if HAVE_BAMBU_BUILT
bool BehavioralHelper::CanBeSpeculated(const unsigned int node_index) const
{
   if(node_index == ENTRY_ID or node_index == EXIT_ID)
   {
      return false;
   }
   const auto tn = TM->CGetTreeNode(node_index);
   const auto ga = GetPointer<const gimple_assign>(tn);
   /// This check must be done before check of load or store since predicated load nd predicated store can be speculated
   if(ga and ga->predicate and GET_NODE(ga->predicate)->get_kind() != integer_cst_K)
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking if " + STR(tn) + " can be speculated");
   switch(tn->get_kind())
   {
      case gimple_nop_K:
      case gimple_phi_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because is a " + tn->get_kind_text());
         return true;
      }
      case(gimple_assign_K):
      {
         THROW_ASSERT(ga and ga->op1, "unexpected condition"); // to silence the clang static analyzer
         switch(GET_NODE(ga->op1)->get_kind())
         {
            case call_expr_K:
            case aggr_init_expr_K:
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because of " + GET_NODE(ga->op1)->get_kind_text() + " in right part of gimple_assign");
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
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because it is a gimple_assign with " + GET_NODE(ga->op1)->get_kind_text() + " in right part of assignment");
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
               THROW_UNREACHABLE(GET_NODE(ga->op1)->get_kind_text() + " - " + STR(tn));
               break;
            }
            default:
               THROW_UNREACHABLE("");
         }
         return true;
      }
      case gimple_asm_K:
      /// Call functions cannot be speculated because of possible effects not caught by virtuals in particular corner case (gcc-4.6 -O0 gcc_regression_simple/20070424-1.c)
      case gimple_call_K:
      case gimple_cond_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_multi_way_if_K:
      case gimple_pragma_K:
      case gimple_return_K:
      case gimple_switch_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because is a " + tn->get_kind_text());
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
   if(node_index == ENTRY_ID or node_index == EXIT_ID)
      return false;
   THROW_ASSERT(node_index, "unexpected condition");
   const auto tn = TM->CGetTreeNode(node_index);
   const auto* gn = GetPointer<const gimple_node>(tn);
   THROW_ASSERT(gn, "unexpected condition: node " + STR(tn) + " is not a gimple_node");
   /*
    * artificial gimple_node can never be moved because they are created to
    * handle specific situations, like for example handling functions returnin
    * structs by value or accepting structs passed by value as parameters
    */
   if(gn->artificial)
      return false;
   const auto* gc = GetPointer<const gimple_call>(tn);
   if(gc and GET_NODE(gc->fn)->get_kind() == addr_expr_K)
   {
      // the node is a gimple_call to a function (no function pointers
      const auto addr_node = GET_NODE(gc->fn);
      const auto* ae = GetPointer<const addr_expr>(addr_node);
      THROW_ASSERT(GET_NODE(ae->op)->get_kind() == function_decl_K, "node  " + STR(GET_NODE(ae->op)) + " is not function_decl but " + GET_NODE(ae->op)->get_kind_text());
      unsigned int called_id = GET_INDEX_NODE(ae->op);
      const std::string fu_name = tree_helper::name_function(TM, called_id);
      /*
       * __builtin_bambu_time_start() and __builtin_bambu_time_stop() can never
       * be moved, even if they have not the artificial flag.
       * the reason is that they must stay exactly where they are placed in
       * order to work properly to compute the number of simulation cycles
       */
      if(fu_name == "__builtin_bambu_time_start" or fu_name == "__builtin_bambu_time_stop")
         return false;
   }
   return true;
}
#endif

bool BehavioralHelper::IsStore(const unsigned int statement_index) const
{
   const auto& fun_mem_data = AppM->CGetFunctionBehavior(function_index)->get_function_mem();
   return tree_helper::IsStore(TM, TM->CGetTreeNode(statement_index), fun_mem_data);
}

bool BehavioralHelper::IsLoad(const unsigned int statement_index) const
{
   const auto& fun_mem_data = AppM->CGetFunctionBehavior(function_index)->get_function_mem();
   return tree_helper::IsLoad(TM, TM->CGetTreeNode(statement_index), fun_mem_data);
}

bool BehavioralHelper::IsLut(const unsigned int statement_index) const
{
   return tree_helper::IsLut(TM, TM->CGetTreeNode(statement_index));
}

void BehavioralHelper::InvaildateVariableName(const unsigned int index)
{
   if(vars_symbol_table.find(index) != vars_symbol_table.end())
   {
      vars_symbol_table.erase(vars_symbol_table.find(index));
   }
}
