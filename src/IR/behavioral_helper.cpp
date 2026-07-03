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
 * @file behavioral_helper.cpp
 * @brief Helper for reading data from ir_manager
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "behavioral_helper.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "cdfg_edge_info.hpp"
#include "compiler_constants.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "type_casting.hpp"
#include "var_pp_functor.hpp"

#include "config_HAVE_ASSERTS.hpp"
#include "config_RELEASE.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <cstddef>
#include <string>
#include <vector>

std::map<std::string, unsigned int> BehavioralHelper::used_name;

std::map<unsigned int, std::string> BehavioralHelper::vars_symbol_table;

std::map<unsigned int, std::string> BehavioralHelper::vars_renaming_table;

/// Max length of a row (at the moment checked only during constructor_node printing)
#define MAX_ROW_LENGTH 128

static std::string NormalizePredicateForC(const ir_nodeConstRef& predicate, const std::string& expression)
{
   return ir_helper::IsBooleanType(predicate) ? "((" + expression + ") & 1)" : expression;
}

BehavioralHelper::BehavioralHelper(const application_managerRef _AppM, unsigned int _index,
                                   const ParameterConstRef _parameters)
    : AppM(application_managerRef(_AppM.get(), null_deleter())),
      TM(_AppM->get_ir_manager()),
      Param(_parameters),
      debug_level(Param->get_class_debug_level("BehavioralHelper", DEBUG_LEVEL_NONE)),
      function_index(_index)
{
}

std::string BehavioralHelper::print_vertex(const OpGraph& g, OpGraph::vertex_descriptor v,
                                           const std::unique_ptr<var_pp_functor>& vppf, bool dot) const
{
   const auto node = g.CGetNodeInfo(v).node;
   std::string res;
   if(node && node->get_kind() != nop_stmt_K)
   {
      res = PrintNode(node, vppf);
      switch(node->get_kind())
      {
         case assign_stmt_K:
         case call_stmt_K:
            res += ";";
            break;
         case call_node_K:
         case constructor_node_K:
         case multi_way_if_stmt_K:
         case nop_stmt_K:
         case phi_stmt_K:
         case return_stmt_K:
         case identifier_node_K:
         case ssa_node_K:
         case statement_list_node_K:
         case lut_node_K:
         case CASE_BINARY_NODES:
         case CASE_CST_NODES:
         case CASE_DECL_NODES:
         case CASE_FAKE_NODES:
         case CASE_TERNARY_NODES:
         case CASE_TYPE_NODES:
         case CASE_UNARY_NODES:
         default:
         {
            break;
         }
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

std::string BehavioralHelper::PrintInit(const ir_nodeConstRef& node, const std::unique_ptr<var_pp_functor>& vppf) const
{
   std::string res;
   switch(node->get_kind())
   {
      case constructor_node_K:
      {
         const auto constr = GetPointerS<const constructor_node>(node);
         bool designated_initializers_needed = false;
         res += '{';
         auto i = constr->list_of_idx_valu.begin();
         const auto vend = constr->list_of_idx_valu.end();
         /// check if designated initializers are really needed
         const auto firstnode = i != vend ? constr->list_of_idx_valu.front().first : ir_nodeRef();
         if(firstnode && firstnode->get_kind() == field_val_node_K)
         {
            const auto fd = GetPointerS<const field_val_node>(firstnode);
            const auto parent = fd->parent;
            std::vector<ir_nodeRef> field_list;
            THROW_ASSERT(parent->get_kind() == struct_ty_node_K, "expected a struct_ty_node");
            field_list = GetPointerS<const struct_ty_node>(parent)->list_of_flds;
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
            if(designated_initializers_needed && i->first && i->first->get_kind() == field_val_node_K)
            {
               current += ".";
               current += PrintVariable(i->first->index);
               current += "=";
            }
            auto val = i->second;
            THROW_ASSERT(val, "Something of unexpected happen");
            if(val->get_kind() == addr_node_K)
            {
               const auto ae = GetPointerS<const addr_node>(val);
               const auto op = ae->op;
               if(op->get_kind() == function_val_node_K)
               {
                  val = ae->op;
                  THROW_ASSERT(val, "Something of unexpected happen");
               }
            }
            if(val->get_kind() == function_val_node_K)
            {
               current += ir_helper::GetFunctionName(val);
            }
            else if(val->get_kind() == constructor_node_K)
            {
               current += PrintInit(i->second, vppf);
            }
            else if(val->get_kind() == variable_val_node_K)
            {
               current += PrintVariable(i->second->index);
            }
            else if(val->get_kind() == ssa_node_K)
            {
               current += PrintVariable(i->second->index);
            }
            else
            {
               current += PrintNode(i->second, vppf);
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
      case addr_node_K:
      case nop_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      {
         res = PrintConstant(node, vppf);
         break;
      }
      case gep_node_K:
      case add_node_K:
      {
         res += PrintNode(node, vppf);
         break;
      }
      case variable_val_node_K:
      {
         const auto vd = GetPointerS<const variable_val_node>(node);
         THROW_ASSERT(vd->init, "expected a initialization value: " + STR(node));
         res += PrintInit(vd->init, vppf);
         break;
      }
      case call_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case not_node_K:
      case fptoi_node_K:
      case itofp_node_K:
      case unaligned_mem_access_node_K:
      case lut_node_K:
      case neg_node_K:
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
      case mem_access_node_K:
      case min_node_K:
      case sub_node_K:
      case mul_node_K:
      case ne_node_K:
      case fdiv_node_K:
      case shr_node_K:
      case idiv_node_K:
      case irem_node_K:
      case widen_mul_node_K:
      case field_val_node_K:
      case function_val_node_K:
      case argument_val_node_K:
      case module_unit_node_K:
      case identifier_node_K:
      case abs_node_K:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case extract_bit_node_K:
      case add_sat_node_K:
      case sub_sat_node_K:
      case extractvalue_node_K:
      case extractelement_node_K:
      case frem_node_K:
      default:
      {
         THROW_ERROR("Currently not supported nodeID " + STR(node));
      }
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
   const auto var_node = TM->GetIRNode(var);
   if(var_node->get_kind() == unaligned_mem_access_node_K)
   {
      const auto mir = GetPointerS<const unaligned_mem_access_node>(var_node);
      auto pointer = mir->op->index;
      std::string pointer_name = PrintVariable(pointer);
      vars_symbol_table[var] = "*" + pointer_name;
      return vars_symbol_table[var];
   }
   if(var_node->get_kind() == mem_access_node_K)
   {
      const auto mr = GetPointerS<const mem_access_node>(var_node);
      const ir_manipulationRef tm(new ir_manipulation(AppM->get_ir_manager(), Param, AppM));
      const auto pointer_ty_node = tm->GetPointerType(mr->type, 8);
      const auto type_string = ir_helper::PrintType(pointer_ty_node);
      vars_symbol_table[var] = "*((" + type_string + ")(" + PrintVariable(mr->op->index) + "))";
      return vars_symbol_table[var];
   }
   if(var_node->get_kind() == identifier_node_K)
   {
      const auto in = GetPointerS<const identifier_node>(var_node);
      vars_symbol_table[var] = in->strg;
      return vars_symbol_table[var];
   }
   if(var_node->get_kind() == field_val_node_K)
   {
      const auto fd = GetPointerS<const field_val_node>(var_node);
      if(fd->name)
      {
         const auto id = GetPointerS<const identifier_node>(fd->name);
         return ir_helper::NormalizeTypename(id->strg);
      }
      else
      {
         return INTERNAL + STR(var);
      }
   }
   if(var_node->get_kind() == function_val_node_K)
   {
      return ir_helper::GetFunctionName(var_node);
   }
   if(var_node->get_kind() == ssa_node_K)
   {
      const auto sa = GetPointerS<const ssa_node>(var_node);
      std::string name;
      if(sa->var)
      {
         name = PrintVariable(sa->var->index);
      }
      THROW_ASSERT(sa->GetDefStmt(), sa->ToString() + " has not define statement");
      if(sa->virtual_flag || (sa->GetDefStmt()->get_kind() != nop_stmt_K))
      {
         name += ("_" + STR(sa->vers));
      }
      else
      {
         THROW_ASSERT(sa->var, "the name has to be defined for parameters");
      }
      return name;
   }
   if(var_node->get_kind() == variable_val_node_K || var_node->get_kind() == argument_val_node_K)
   {
      const auto dn = GetPointerS<const decl_node>(var_node);
      if(dn->name)
      {
         const auto id = GetPointerS<const identifier_node>(dn->name);
         vars_symbol_table[var] = ir_helper::NormalizeTypename(id->strg);
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

std::string BehavioralHelper::PrintConstant(const ir_nodeConstRef& node,
                                            const std::unique_ptr<var_pp_functor>& vppf) const
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
      case constant_int_val_node_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->constant_int_val_node");
         const auto ic = GetPointerS<const constant_int_val_node>(node);
         const auto it = GetPointer<const integer_ty_node>(ic->type);
         const auto unsigned_flag = (it && it->unsigned_flag) || ic->type->get_kind() == pointer_ty_node_K;
         auto value = ir_helper::GetConstValue(node, !unsigned_flag);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Value is " + STR(value));
         // TODO: fix for bitwidth greater than 64 bits
         if((it && it->bitsize == 64) && (value == (static_cast<long long int>(-0x08000000000000000LL))))
         {
            res = "(long long int)-0x08000000000000000";
         }
         else if((it && it->bitsize == 32) && (value == (static_cast<long int>(-0x080000000L))))
         {
            res = "(long int)-0x080000000";
         }
         else
         {
            if(it && it->unsigned_flag)
            {
               res += STR(value & ((integer_cst_t(1) << it->bitsize) - 1));
            }
            else
            {
               res += STR(value);
            }
         }
         if(it && it->bitsize > 32)
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
            else if(ic->type->get_kind() == pointer_ty_node_K)
            {
               res += "/*B*/";
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
         break;
      }
      case constant_fp_val_node_K:
      {
         const auto rc = GetPointerS<const constant_fp_val_node>(node);
         if(rc->overflow_flag)
         {
            res += " overflow";
         }
         if(ir_helper::IsRealType(rc->type))
         {
            if(GetPointerS<const real_ty_node>(rc->type)->bitsize == 80) /// long double
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
            else if(GetPointerS<const real_ty_node>(rc->type)->bitsize == 64) /// double
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
      case constant_vector_val_node_K:
      {
         const auto vc = GetPointerS<const constant_vector_val_node>(node);
         THROW_ASSERT(vc->type->get_kind() == vector_ty_node_K, "Vector constant of type " + vc->type->get_kind_text());
         if(GetPointerS<const vector_ty_node>(vc->type)->elts->get_kind() != pointer_ty_node_K)
         {
            res += "(" + ir_helper::PrintType(vc->type, true) + ") ";
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
      case nop_node_K:
      {
         const auto ue = GetPointerS<const unary_node>(node);
         res += "(" + ir_helper::PrintType(ue->type) + ") ";
         res += PrintConstant(ue->op, vppf);
         break;
      }
      case addr_node_K:
      {
         const auto ue = GetPointerS<const unary_node>(node);
         if(is_a_constant(ue->op->index))
         {
            res += "(&(" + PrintConstant(ue->op) + "))";
         }
         else if(ue->op->get_kind() == function_val_node_K)
         {
            res += ir_helper::GetFunctionName(ue->op);
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
      case mem_access_node_K:
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case field_val_node_K:
      case function_val_node_K:
      case argument_val_node_K:
      case module_unit_node_K:
      case variable_val_node_K:
      case abs_node_K:
      case not_node_K:
      case fptoi_node_K:
      case itofp_node_K:
      case unaligned_mem_access_node_K:
      case neg_node_K:
      case bitcast_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      default:
      {
         THROW_ERROR("Var object is not a constant " + STR(node) + " " + node->get_kind_text());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Printed constant " + res);
   return res;
}

unsigned long long BehavioralHelper::get_size(unsigned int var) const
{
   return ir_helper::Size(TM->GetIRNode(var));
}

std::string BehavioralHelper::GetFunctionName() const
{
   return ir_helper::GetFunctionName(TM->GetIRNode(function_index));
}

unsigned int BehavioralHelper::get_function_index() const
{
   return function_index;
}

unsigned int BehavioralHelper::GetFunctionReturnType() const
{
   const auto return_type = ir_helper::GetFunctionReturnType(TM->GetIRNode(function_index));
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
   const auto fun = TM->GetIRNode(function_index);
   const auto fd = GetPointerS<const function_val_node>(fun);
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
      const auto ft = GetPointerS<const function_ty_node>(fd->type);
      for(const auto& p : ft->list_of_args_type)
      {
         parameters.push_back(p->index);
      }
   }
   return parameters;
}

std::vector<ir_nodeRef> BehavioralHelper::GetParameters() const
{
   const auto fun = TM->GetIRNode(function_index);
   const auto fd = GetPointerS<const function_val_node>(fun);
   if(fd->list_of_args.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Parameter list size: " + STR(fd->list_of_args.size()));
      return fd->list_of_args;
   }

   std::vector<ir_nodeRef> parameters;
   const auto ft = GetPointerS<const function_ty_node>(fd->type);
   for(const auto& p : ft->list_of_args_type)
   {
      parameters.push_back(p);
   }
   return parameters;
}

bool BehavioralHelper::has_implementation() const
{
   return ir_helper::IsFunctionImplemented(TM->GetIRNode(function_index));
}

void BehavioralHelper::add_initialization(unsigned int var, unsigned int init)
{
   initializations[var] = init;
}

unsigned int BehavioralHelper::get_type(const unsigned int var) const
{
   return ir_helper::CGetType(TM->GetIRNode(var))->index;
}

unsigned int BehavioralHelper::get_pointed_type(const unsigned int type) const
{
   return ir_helper::CGetPointedType(TM->GetIRNode(type))->index;
}

unsigned int BehavioralHelper::GetElements(const unsigned int type) const
{
   return ir_helper::CGetElements(TM->GetIRNode(type))->index;
}

std::string BehavioralHelper::PrintVarDeclaration(unsigned int var, const std::unique_ptr<var_pp_functor>& vppf,
                                                  bool init_has_to_be_printed) const
{
   std::string return_value;
   const auto curr_tn = TM->GetIRNode(var);
   THROW_ASSERT(GetPointer<const decl_node>(curr_tn) || GetPointer<const ssa_node>(curr_tn),
                "Call pparameter_type_indexrint_var_declaration on node " + STR(var) + " which is of type " +
                    curr_tn->get_kind_text());
   const decl_node* dn = nullptr;
   if(GetPointer<const decl_node>(curr_tn))
   {
      dn = GetPointerS<const decl_node>(curr_tn);
   }
   /// If it is not a decl node (then it is an ssa-name) or it's a not system decl_node
   if(!dn || !(dn->operating_system_flag || dn->library_system_flag) || ir_helper::IsInLibbambu(curr_tn))
   {
      return_value += ir_helper::PrintType(ir_helper::CGetType(curr_tn), false, init_has_to_be_printed, curr_tn, vppf);
      CustomUnorderedSet<unsigned int> list_of_variables;
      const unsigned int init = GetInit(var, list_of_variables);
      if(init && init_has_to_be_printed)
      {
         return_value += " = " + PrintInit(TM->GetIRNode(init), vppf);
      }
   }
   return return_value;
}

bool BehavioralHelper::is_var_args() const
{
   ir_nodeRef tn = TM->GetIRNode(function_index);
   THROW_ASSERT(tn->get_kind() == function_val_node_K, "function_index is not a function decl");
   tn = GetPointer<function_val_node>(tn)->type;
   return GetPointer<function_ty_node>(tn)->varargs_flag;
}

std::string BehavioralHelper::PrintNode(unsigned int _node, const std::unique_ptr<var_pp_functor>& vppf) const
{
   const auto node = TM->GetIRNode(_node);
   return PrintNode(node, vppf);
}

std::string BehavioralHelper::PrintNode(const ir_nodeConstRef& node, const std::unique_ptr<var_pp_functor>& vppf) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Printing node " + STR(node));
   std::string res = "";
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Type is " + std::string(node->get_kind_text()));
   switch(node->get_kind())
   {
         /* Binary arithmetic and logic expressions.  */
      case add_sat_node_K:
      {
         const auto be = GetPointerS<const binary_node>(node);
         const auto res_size = ir_helper::Size(be->type);
         std::string left, right;
         left = ("(" + PrintNode(be->op0, vppf) + ")");
         right = ("(" + PrintNode(be->op1, vppf) + ")");
         if(ir_helper::IsSignedIntegerType(be->type))
         {
            const auto op_size = ir_helper::Size(be->op0);
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
      case sub_sat_node_K:
      {
         const auto be = GetPointerS<const binary_node>(node);
         const auto left = "(" + PrintNode(be->op0, vppf) + ")";
         const auto right = "(" + PrintNode(be->op1, vppf) + ")";
         if(ir_helper::IsSignedIntegerType(be->type))
         {
            const auto res_size = ir_helper::Size(be->type);
            const auto op_size = ir_helper::Size(be->op0);
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
      case frem_node_K:
      {
         const auto be = GetPointerS<const binary_node>(node);
         std::string left, right;
         THROW_ASSERT(GetPointer<const real_ty_node>(be->type), "unexpected case");
         left = ("(" + PrintNode(be->op0, vppf) + ")");
         right = ("(" + PrintNode(be->op1, vppf) + ")");

         const auto rt = GetPointerS<const real_ty_node>(be->type);
         if(rt->bitsize == 80)
         {
            res += "fmodl(" + left + "," + right + ")";
         }
         else if(rt->bitsize == 64)
         {
            res += "fmod(" + left + "," + right + ")";
         }
         else if(rt->bitsize == 32)
         {
            res += "fmodf(" + left + "," + right + ")";
         }
         else
         {
            THROW_ERROR("fmod on a real number with not supported precision");
         }
         break;
      }
      case add_node_K:
      {
         const auto be = GetPointerS<const binary_node>(node);
         const auto op = ir_helper::op_symbol(node);
         const auto left_op_type = ir_helper::CGetType(be->op0);
         const auto right_op_type = ir_helper::CGetType(be->op1);
         const auto vector = ir_helper::IsVectorType(be->type) &&
                             ((ir_helper::IsVectorType(left_op_type) && left_op_type->index != be->type->index) ||
                              (ir_helper::IsVectorType(right_op_type) && right_op_type->index != be->type->index));
         if(vector)
         {
            const auto element_type = ir_helper::CGetElements(be->type);
            const auto element_size = ir_helper::SizeAlloc(element_type);
            const auto size = ir_helper::SizeAlloc(be->type);
            const auto vector_size = size / element_size;
            res += "(" + ir_helper::PrintType(be->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(be->op0, vppf) + ")[" + STR(ind) + "] + (" + PrintNode(be->op1, vppf) + ")[" +
                      STR(ind) + "]";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
         }
         else
         {
            const auto type = ir_helper::CGetType(node);
            unsigned int prec = 0;
            unsigned int algn = 0;
            if(type && type->get_kind() == integer_ty_node_K)
            {
               prec = GetPointerS<const integer_ty_node>(type)->bitsize;
               algn = GetPointerS<const integer_ty_node>(type)->algn;
            }
            // bitfield type
            if(prec != algn && prec % algn)
            {
               res += "((";
            }
            res += "(" + ir_helper::PrintType(be->type) + ")(";

            if(GetPointer<const decl_node>(be->op0) || GetPointer<const ssa_node>(be->op0))
            {
               res += PrintNode(be->op0, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op0, vppf) + ")");
            }
            res += std::string(" ") + op + " ";

            if(GetPointer<const decl_node>(be->op1) || GetPointer<const ssa_node>(be->op1))
            {
               res += PrintNode(be->op1, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op1, vppf) + ")");
            }
            res += ")";
            if(prec != algn && prec % algn)
            {
               res += ")%(1";
               if(prec > 32)
               {
                  res += "LL";
               }
               if(GetPointerS<const integer_ty_node>(type)->unsigned_flag)
               {
                  res += "U";
               }
               res += " << " + STR(prec) + "))";
            }
         }
         break;
      }
      case or_node_K:
      case xor_node_K:
      case and_node_K:
      {
         const auto be = GetPointerS<const binary_node>(node);
         const auto left_op_type = ir_helper::CGetType(be->op0);
         const auto right_op_type = ir_helper::CGetType(be->op1);
         const auto op = ir_helper::op_symbol(node);
         const auto vector = ir_helper::IsVectorType(be->type) &&
                             ((ir_helper::IsVectorType(left_op_type) && left_op_type->index != be->type->index) ||
                              (ir_helper::IsVectorType(right_op_type) && right_op_type->index != be->type->index));
         if(vector)
         {
            const auto element_type = ir_helper::CGetElements(be->type);
            const auto element_size = ir_helper::SizeAlloc(element_type);
            const auto size = ir_helper::SizeAlloc(be->type);
            const auto vector_size = size / element_size;
            res += "(" + ir_helper::PrintType(be->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(be->op0, vppf) + ")[" + STR(ind) + "] " + op + " (" + PrintNode(be->op1, vppf) +
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
            const auto type = ir_helper::CGetType(node);
            bool bit_expression = type && type->get_kind() == pointer_ty_node_K;
            unsigned int prec = 0;
            unsigned int algn = 0;
            if(type && type->get_kind() == integer_ty_node_K)
            {
               prec = GetPointerS<const integer_ty_node>(type)->bitsize;
               algn = GetPointerS<const integer_ty_node>(type)->algn;
            }
            // bitfield type
            if(prec != algn && prec % algn)
            {
               res += "((";
            }
            if(bit_expression)
            {
               res += "((" + ir_helper::PrintType(ir_helper::CGetType(node)) + ")(((unsigned)(";
            }

            if(GetPointer<const decl_node>(be->op0) || GetPointer<const ssa_node>(be->op0))
            {
               res += PrintNode(be->op0, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op0, vppf) + ")");
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

            if(GetPointer<const decl_node>(be->op1) || GetPointer<const ssa_node>(be->op1))
            {
               res += PrintNode(be->op1, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op1, vppf) + ")");
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
               if(GetPointerS<const integer_ty_node>(type)->unsigned_flag)
               {
                  res += "U";
               }
               res += " << " + STR(prec) + "))";
            }
         }
         break;
      }
      case mul_node_K:
      case sub_node_K:
      case idiv_node_K:
      case irem_node_K:
      case fdiv_node_K:
      case shl_node_K:
      case shr_node_K:
      {
         const auto op = ir_helper::op_symbol(node);
         const auto be = GetPointerS<const binary_node>(node);
         const auto left_op_type = ir_helper::CGetType(be->op0);
         const auto right_op_type = ir_helper::CGetType(be->op1);
         const auto vector = ir_helper::IsVectorType(be->type) &&
                             ((ir_helper::IsVectorType(left_op_type) && left_op_type->index != be->type->index) ||
                              (ir_helper::IsVectorType(right_op_type) && right_op_type->index != be->type->index));
         if(vector)
         {
            const auto element_type = ir_helper::CGetElements(be->type);
            const auto element_size = ir_helper::SizeAlloc(element_type);
            const auto size = ir_helper::SizeAlloc(be->type);
            const auto vector_size = size / element_size;
            res += "(" + ir_helper::PrintType(be->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res +=
                   "(" + PrintNode(be->op0, vppf) + ")[" + STR(ind) + "] " + op + " (" + PrintNode(be->op1, vppf) + ")";
               if(be->op1->get_kind() != constant_int_val_node_K)
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
            const auto type = ir_helper::CGetType(node);
            unsigned int prec = 0;
            unsigned int algn = 0;
            if(type && type->get_kind() == integer_ty_node_K)
            {
               prec = GetPointerS<const integer_ty_node>(type)->bitsize;
               algn = GetPointerS<const integer_ty_node>(type)->algn;
            }
            // bitfield type
            if(prec != algn && prec % algn)
            {
               res += "((";
            }
            if((node->get_kind() == shl_node_K || node->get_kind() == shr_node_K) &&
               left_op_type->index != be->type->index)
            {
               res += "(" + ir_helper::PrintType(be->type) + ")(";
            }
            if(GetPointer<const decl_node>(be->op0) || GetPointer<const ssa_node>(be->op0))
            {
               res += PrintNode(be->op0, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op0, vppf) + ")");
            }
            if((node->get_kind() == shl_node_K || node->get_kind() == shr_node_K) &&
               left_op_type->index != be->type->index)
            {
               res += ")";
            }
            res += std::string(" ") + op + " ";
            if(GetPointer<const decl_node>(be->op1) || GetPointer<const ssa_node>(be->op1))
            {
               res += PrintNode(be->op1, vppf);
            }
            else
            {
               res += ("(" + PrintNode(be->op1, vppf) + ")");
            }
            if(prec != algn && prec % algn)
            {
               res += ")%(1";
               if(prec > 32)
               {
                  res += "LL";
               }
               if(GetPointerS<const integer_ty_node>(type)->unsigned_flag)
               {
                  res += "U";
               }
               res += " << " + STR(prec) + "))";
            }
         }
         break;
      }
      case widen_mul_node_K:
      {
         const auto op = ir_helper::op_symbol(node);
         const auto be = GetPointerS<const binary_node>(node);
         const auto return_type = ir_helper::CGetType(node);

         res += "((" + ir_helper::PrintType(return_type) + ")(" + PrintNode(be->op0, vppf) + "))";
         res += std::string(" ") + op + " ";
         res += "((" + ir_helper::PrintType(return_type) + ")(" + PrintNode(be->op1, vppf) + "))";
         break;
      }
      case extract_bit_node_K:
      {
         const auto be = GetPointerS<const binary_node>(node);
         res += "(bool)(((unsigned long long int)(" + PrintNode(be->op0, vppf);
         res += std::string(") >> ");
         res += PrintNode(be->op1, vppf) + ") & 1)";
         break;
      }
      case extractvalue_node_K:
      {
         const auto be = GetPointerS<const extractvalue_node>(node);
         const auto return_type = ir_helper::CGetType(node);
         res += "(" + ir_helper::PrintType(return_type) + ")(" + PrintNode(be->op0, vppf) + " >> " +
                PrintNode(be->op1, vppf) + ");";
         break;
      }
      case insertvalue_node_K:
      {
         const auto te = GetPointerS<const insertvalue_node>(node);
         unsigned long long op2_size;
         op2_size = ir_helper::Size(te->op2);
         const auto return_type = ir_helper::CGetType(node);
         res += "(" + ir_helper::PrintType(return_type) + ")";
         res += "(((" + PrintNode(te->op0, vppf) + " >> (" + STR(op2_size) + " + " + PrintNode(te->op2, vppf) + "))";
         res += " << (" + STR(op2_size) + " + " + PrintNode(te->op2, vppf) + ")) | (( " + PrintNode(te->op1, vppf) +
                " << ";
         res += PrintNode(te->op2, vppf) + ") | ((" + PrintNode(te->op0, vppf) + " << " + PrintNode(te->op2, vppf);
         res += ") >> " + PrintNode(te->op2, vppf) + ")))";
         break;
      }
      case extractelement_node_K:
      {
         const auto be = GetPointerS<const extractvalue_node>(node);
         const auto return_type = ir_helper::CGetType(node);
         res += "((" + ir_helper::PrintType(return_type) + ")(" + PrintNode(be->op0, vppf) + "[" +
                PrintNode(be->op1, vppf) + "]))";
         break;
      }
      case insertelement_node_K:
      {
         const auto iee = GetPointerS<const insertelement_node>(node);
         const auto element_type = ir_helper::CGetElements(iee->type);
         const auto element_size = ir_helper::SizeAlloc(element_type);
         const auto size = ir_helper::SizeAlloc(iee->type);
         const auto vector_size = size / element_size;

         res += "/*" + iee->get_kind_text() + "*/";
         res += "(" + ir_helper::PrintType(iee->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += "(" + PrintNode(iee->op2, vppf) + " == " + STR(ind) + " ? " + PrintNode(iee->op1, vppf) + " : " +
                   PrintNode(iee->op0, vppf) + "[" + STR(ind) + "])";
            if(ind != vector_size - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case gep_node_K:
      {
         const auto ppe = GetPointerS<const gep_node>(node);
         const auto op = ir_helper::op_symbol(node);
         const auto binary_op_cast = ir_helper::IsPointerType(node);
         const auto type_node = ir_helper::CGetType(ppe->op0);
         const auto left_op_cast = ir_helper::IsPointerType(ppe->op0);
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
         const auto right_cost = right_op_node->get_kind() == constant_int_val_node_K;
         THROW_ASSERT(!ir_helper::IsPointerType(ppe->op1), "expected a right operand different from a pointer");
         THROW_ASSERT(ir_helper::IsPointerType(ppe->type),
                      "expected a pointer type: " + ppe->type->get_kind_text() + " - " + STR(ppe->type));

         /// check possible pointer arithmetic reverse
         unsigned long long deltabit;
         const auto pointed_type = ir_helper::CGetPointedType(ir_helper::CGetType(ppe->op0));
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "Pointed type (" + STR(pointed_type) + ") is " + pointed_type->get_kind_text());
         if(ir_helper::IsVoidType(pointed_type))
         {
            const auto vt = GetPointerS<const void_ty_node>(pointed_type);
            deltabit = vt->algn;
         }
         else
         {
            deltabit = ir_helper::Size(pointed_type);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "deltabit is " + STR(deltabit));
         integer_cst_t pointer_offset = 0;
         std::string right_offset_var;
         if(right_cost)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Offset is constant");
            pointer_offset = ir_helper::GetConstValue(right_op_node, ir_helper::IsSignedIntegerType(right_op_node));
            if(deltabit / 8 == 0)
            {
               do_reverse_pointer_arithmetic = false;
            }
            else if(pointed_type->get_kind() != array_ty_node_K && deltabit && pointer_offset > deltabit &&
                    ((pointer_offset % (deltabit / 8)) == 0))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Arithmetic pointer pattern matched");
               const auto ic = GetPointerS<const constant_int_val_node>(right_op_node);
               const auto it = GetPointer<const integer_ty_node>(ic->type);
               if(it && (it->bitsize == 32))
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
            bool exit = pointed_type->get_kind() == array_ty_node_K;
            while(!do_reverse_pointer_arithmetic && !exit)
            {
               switch(right_op_node->get_kind())
               {
                  case ssa_node_K:
                  {
                     if(!GetPointerS<const ssa_node>(right_op_node)->GetDefStmt())
                     {
                        exit = true;
                        break;
                     }
                     const auto rssa = GetPointerS<const ssa_node>(right_op_node);
                     const auto defstmt = rssa->GetDefStmt();
                     if(defstmt->get_kind() != assign_stmt_K)
                     {
                        exit = true;
                        break;
                     }
                     right_op_node = GetPointerS<const assign_stmt>(defstmt)->op1;
                     INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                    "New op node is " + STR(right_op_node->index) + " - " +
                                        right_op_node->get_kind_text());
                     break;
                  }
                     {
                        THROW_UNREACHABLE("");
                        break;
                     }
                  case mul_node_K:
                  {
                     const auto mult = GetPointerS<const mul_node>(right_op_node);
                     if(mult->op1->get_kind() == constant_int_val_node_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "-->Right part of multiply is an integer constant " + STR(mult->op1->index));
                        const auto size_of_pointer = ir_helper::GetConstValue(mult->op1);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "---Size of pointer is " + STR(size_of_pointer));
                        if(size_of_pointer == (deltabit / 8))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Constant is the size of the pointed");
                           right_offset_var += PrintNode(mult->op0, vppf);
                           do_reverse_pointer_arithmetic = true;
                        }
                        else
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                          "-->Constant is not the size of the pointed: " + STR(size_of_pointer) +
                                              " vs " + STR(deltabit / 8));
                           const auto temp1 = ir_helper::CGetType(mult->op1);
                           THROW_ASSERT(GetPointer<const integer_ty_node>(temp1),
                                        "Type of integer cast " + STR(mult->op1->index) + " is not an integer type");
                           if(deltabit && (deltabit / 8) && (size_of_pointer % ((deltabit / 8)) == 0))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                             "---Constant is a multiple of the  size of the pointed");
                              right_offset_var += PrintNode(mult->op0, vppf);
                              right_offset_var += " * ";
                              right_offset_var += STR(size_of_pointer / ((deltabit / 8)));
                              do_reverse_pointer_arithmetic = true;
                           }
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
                        INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                       "<--Right offset variable is " + right_offset_var);
                     }
                     else if(mult->op0->get_kind() == constant_int_val_node_K)
                     {
                        const auto size_of_pointer = ir_helper::GetConstValue(mult->op0);
                        if(size_of_pointer == (deltabit / 8))
                        {
                           right_offset_var += PrintNode(mult->op1, vppf);
                           do_reverse_pointer_arithmetic = true;
                        }
                        else
                        {
                           const auto temp1 = ir_helper::CGetType(mult->op0);
                           THROW_ASSERT(GetPointer<const integer_ty_node>(temp1),
                                        "Type of integer cast " + STR(mult->op0->index) + " is not an integer type");
                           if((deltabit / 8) && (size_of_pointer % ((deltabit / 8)) == 0))
                           {
                              right_offset_var += PrintNode(mult->op1, vppf);
                              right_offset_var += " * ";
                              right_offset_var += STR(size_of_pointer / ((deltabit / 8)));
                              do_reverse_pointer_arithmetic = true;
                           }
                        }
                     }
                     exit = true;
                     break;
                  }
                  case neg_node_K:
                  {
                     const auto ne = GetPointerS<const neg_node>(right_op_node);
                     right_offset_var += "-";
                     right_op_node = ne->op;
                     INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                    "New op node is " + STR(ne->op->index) + " - " + right_op_node->get_kind_text());
                     break;
                  }
                  case nop_node_K:
                  {
                     const auto ne = GetPointerS<const nop_node>(right_op_node);
                     right_op_node = ne->op;
                     INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                                    "New op node is " + STR(ne->op->index) + " - " + right_op_node->get_kind_text());
                     break;
                  }
                  case call_node_K:
                  case constructor_node_K:
                  case identifier_node_K:
                  case statement_list_node_K:
                  case addr_node_K:
                  case not_node_K:
                  case fptoi_node_K:
                  case itofp_node_K:
                  case unaligned_mem_access_node_K:
                  case bitcast_node_K:
                  case abs_node_K:
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
                  case mem_access_node_K:
                  case min_node_K:
                  case sub_node_K:
                  case ne_node_K:
                  case add_node_K:
                  case gep_node_K:
                  case fdiv_node_K:
                  case shr_node_K:
                  case idiv_node_K:
                  case irem_node_K:
                  case widen_mul_node_K:
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
               right_offset_var = PrintNode(ppe->op1, vppf);
            }
         }
         bool char_pointer = false;
         if(!do_reverse_pointer_arithmetic && ir_helper::Size(GetPointerS<const pointer_ty_node>(type_node)->ptd) == 8)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "Reversing pointer arithmetic successful because of char pointer");
            do_reverse_pointer_arithmetic = true;
            char_pointer = true;
         }
         if(binary_op_cast && !do_reverse_pointer_arithmetic)
         {
            res += "(" + ir_helper::PrintType(ppe->type) + ")(";
         }
         if((left_op_cast && ir_helper::IsVoidType(pointed_type)) || !do_reverse_pointer_arithmetic)
         {
            res += "((unsigned char*)";
         }
         res += PrintNode(ppe->op0, vppf);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "After printing of left part " + res);
         if((left_op_cast && ir_helper::IsVoidType(pointed_type)) || !do_reverse_pointer_arithmetic)
         {
            res += ")";
         }
         res += std::string(" ") + op + " ";
         /*
                  if(!do_reverse_pointer_arithmetic)
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
               const auto type = GetPointerS<const constant_int_val_node>(right_op_node)->type;
               const auto it = GetPointer<const integer_ty_node>(type);
               bool unsigned_flag = (it && it->unsigned_flag) || type->get_kind() == pointer_ty_node_K;
               if(unsigned_flag)
               {
                  res += "u";
               }
            }
         }
         else
         {
            if(right_op_node->get_kind() == constant_int_val_node_K)
            {
               res += STR(ir_helper::GetConstValue(right_op_node));
            }
            else
            {
               res += PrintNode(ppe->op1, vppf);
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
      case lt_node_K:
      case le_node_K:
      case gt_node_K:
      case ge_node_K:
      case eq_node_K:
      case ne_node_K:
      {
         const auto binary_op_cast = ir_helper::IsPointerType(node);
         if(binary_op_cast)
         {
            res += "((" + ir_helper::PrintType(ir_helper::CGetType(node)) + ")(";
         }
         const auto op = ir_helper::op_symbol(node);
         const auto be = GetPointerS<const binary_node>(node);
         const auto& left_op = be->op0;
         const auto& right_op = be->op1;
         const auto left_op_type = ir_helper::CGetType(be->op0);
         const auto right_op_type = ir_helper::CGetType(be->op1);

         bool vector = ir_helper::IsVectorType(be->type) && ir_helper::IsVectorType(right_op_type) &&
                       right_op_type->index != be->type->index;
         if(vector)
         {
            const auto element_type = ir_helper::CGetElements(be->type);
            const auto element_size = ir_helper::SizeAlloc(element_type);
            const auto size = ir_helper::SizeAlloc(be->type);
            const auto vector_size = size / element_size;
            res += "(" + ir_helper::PrintType(be->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(left_op, vppf) + ")[" + STR(ind) + "] " + op + " (" + PrintNode(right_op, vppf) +
                      ")[" + STR(ind) + "]";
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
            const auto left_op_cast = ir_helper::IsPointerType(left_op) && ir_helper::IsPointerType(right_op) &&
                                      (ir_helper::Size(left_op) != ir_helper::Size(right_op));
            const auto right_op_cast = ir_helper::IsPointerType(right_op) && ir_helper::IsPointerType(left_op) &&
                                       (ir_helper::Size(left_op) != ir_helper::Size(right_op));
            const auto left_op_bracket = !(GetPointer<decl_node>(be->op0) || GetPointer<ssa_node>(be->op0));
            const auto right_op_bracket = !(GetPointer<decl_node>(be->op1) || GetPointer<ssa_node>(be->op1));

            if(left_op_bracket)
            {
               res += "(";
            }
            if(left_op_cast)
            {
               res += "((unsigned long int)";
            }

            if(ir_helper::IsVectorType(be->type) && ir_helper::IsVectorType(left_op_type) &&
               left_op_type->index != be->type->index)
            {
               res += "(" + ir_helper::PrintType(be->type) + ")(";
            }

            res += PrintNode(left_op, vppf);

            if(ir_helper::IsVectorType(be->type) && ir_helper::IsVectorType(left_op_type) &&
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

            res += PrintNode(right_op, vppf);

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
      case lut_node_K:
      {
         const auto le = GetPointerS<const lut_node>(node);
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
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op6, vppf) + ")<<5) | ";
         }
         if(le->op5)
         {
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op5, vppf) + ")<<4) | ";
         }
         if(le->op4)
         {
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op4, vppf) + ")<<3) | ";
         }
         if(le->op3)
         {
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op3, vppf) + ")<<2) | ";
         }
         if(le->op2)
         {
            concat_shift_string = concat_shift_string + "((" + PrintNode(le->op2, vppf) + ")<<1) | ";
         }
         concat_shift_string = concat_shift_string + "(" + PrintNode(le->op1, vppf) + ")";
         res = res + "(" + PrintNode(le->op0->index, vppf) + ">>(" + concat_shift_string + "))&1";
         break;
      }
      case neg_node_K:
      case not_node_K:
      {
         const auto ue = GetPointerS<const unary_node>(node);
         const auto op = ir_helper::op_symbol(node);
         res = res + " " + op + "(" + PrintNode(ue->op, vppf) + ")";
         break;
      }
      case addr_node_K:
      {
         const auto ue = GetPointerS<const addr_node>(node);
         ///&array is printed back as array
         if(ue->op->get_kind() == variable_val_node_K && (ir_helper::CGetType(ue->op)->get_kind() == array_ty_node_K))
         {
            res += PrintNode(ue->op, vppf);
            break;
         }
         res += "(" + ir_helper::op_symbol(node) + "(" + PrintNode(ue->op, vppf) + "))";
         break;
      }
      case function_val_node_K:
      {
         res += ir_helper::GetFunctionName(node);
         break;
      }
      case fptoi_node_K:
      case itofp_node_K:
      case nop_node_K:
      {
         const auto ue = GetPointerS<const unary_node>(node);
         const auto type = ir_helper::CGetType(node);
         unsigned int prec = 0;
         unsigned int algn = 0;
         if(type && type->get_kind() == integer_ty_node_K)
         {
            prec = GetPointerS<const integer_ty_node>(type)->bitsize;
            algn = GetPointerS<const integer_ty_node>(type)->algn;
         }

         const auto operand_type = ir_helper::CGetType(ue->op);
         unsigned int operand_prec = 0;
         unsigned int operand_algn = 0;
         if(operand_type && operand_type->get_kind() == integer_ty_node_K)
         {
            operand_prec = GetPointerS<const integer_ty_node>(operand_type)->bitsize;
            operand_algn = GetPointerS<const integer_ty_node>(operand_type)->algn;
         }

         std::string operand_res = PrintNode(ue->op, vppf);
         if(operand_prec != operand_algn && operand_prec % operand_algn && ir_helper::IsSignedIntegerType(operand_type))
         {
            operand_res = "((union {" + ir_helper::PrintType(operand_type) + " orig; " +
                          ir_helper::PrintType(operand_type) + " bitfield : " + STR(operand_prec) + ";}){" +
                          operand_res + "}).bitfield";
         }

         // bitfield type
         if(prec != algn && prec % algn)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Bitfield");
            bool ui = ir_helper::IsUnsignedIntegerType(operand_type) && ir_helper::IsSignedIntegerType(type);
            if(ui)
            {
               res += "((" + ir_helper::PrintType(type) + ")(";
            }
            res += "(";
            res += operand_res;
            res += ")%(1";
            if(prec > 32)
            {
               res += "LL";
            }
            if(GetPointerS<const integer_ty_node>(type)->unsigned_flag)
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
            res = res + "(" + ir_helper::PrintType(ue->type) + ") (";
            res += operand_res;
            res += ")";
         }
         break;
      }
      case bitcast_node_K:
      {
         const auto bitcast_expr = GetPointerS<const bitcast_node>(node);
         if(GetPointer<const constant_int_val_node>(bitcast_expr->op))
         {
            res = res + "__panda_union.dest;}";
         }
         else
         {
            if(ir_helper::IsPointerType(bitcast_expr->type))
            {
               res = res + "((" + ir_helper::PrintType(bitcast_expr->type) + ") (" + PrintNode(bitcast_expr->op, vppf) +
                     "))";
            }
            else
            {
               res = res + "*((" + ir_helper::PrintType(bitcast_expr->type) + " * ) &(" +
                     PrintNode(bitcast_expr->op, vppf) + "))";
            }
         }
         break;
      }
      case unaligned_mem_access_node_K:
      {
         const auto mir = GetPointerS<const unaligned_mem_access_node>(node);
         res = "*(" + PrintNode(mir->op, vppf) + ")";
         break;
      }
      case mem_access_node_K:
      {
         const auto mr = GetPointerS<const mem_access_node>(node);
         const ir_manipulationRef tm(new ir_manipulation(AppM->get_ir_manager(), Param, AppM));
         const auto pointer_ty_node = tm->GetPointerType(mr->type, 8);
         const std::string type_string = ir_helper::PrintType(pointer_ty_node);
         res = "(*((" + type_string + ")(" + PrintNode(mr->op, vppf) + ")))";
         break;
      }
      case min_node_K:
      {
         const auto me = GetPointerS<const min_node>(node);
         if(ir_helper::IsVectorType(me->type))
         {
            const auto element_type = ir_helper::CGetElements(me->type);
            const auto element_size = ir_helper::SizeAlloc(element_type);
            const auto size = ir_helper::SizeAlloc(me->type);
            const auto vector_size = size / element_size;
            res += "/*" + me->get_kind_text() + "*/";
            res += "(" + ir_helper::PrintType(me->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(me->op0, vppf) + ")[" + STR(ind) + "] < (" + PrintNode(me->op1, vppf) + ")[" +
                      STR(ind) + "] ? " + "(" + PrintNode(me->op0, vppf) + ")[" + STR(ind) + "]" + " : " + "(" +
                      PrintNode(me->op1, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
         }
         else
         {
            std::string op_0 = PrintNode(me->op0, vppf), op_1 = PrintNode(me->op1, vppf);
            res += op_0 + " < " + op_1 + " ? " + op_0 + " : " + op_1;
         }
         break;
      }
      case max_node_K:
      {
         const auto me = GetPointerS<const max_node>(node);
         if(ir_helper::IsVectorType(me->type))
         {
            const auto element_type = ir_helper::CGetElements(me->type);
            const auto element_size = ir_helper::SizeAlloc(element_type);
            const auto size = ir_helper::SizeAlloc(me->type);
            const auto vector_size = size / element_size;
            res += "/*" + me->get_kind_text() + "*/";
            res += "(" + ir_helper::PrintType(me->type) + ") ";
            res += "{";
            for(unsigned int ind = 0; ind < vector_size; ++ind)
            {
               res += "(" + PrintNode(me->op0, vppf) + ")[" + STR(ind) + "] > (" + PrintNode(me->op1, vppf) + ")[" +
                      STR(ind) + "] ? " + "(" + PrintNode(me->op0, vppf) + ")[" + STR(ind) + "]" + " : " + "(" +
                      PrintNode(me->op1, vppf) + ")[" + STR(ind) + "]";
               if(ind != vector_size - 1)
               {
                  res += ", ";
               }
            }
            res += "}";
         }
         else
         {
            std::string op_0 = PrintNode(me->op0, vppf), op_1 = PrintNode(me->op1, vppf);
            res += op_0 + " > " + op_1 + " ? " + op_0 + " : " + op_1;
         }
         break;
      }
      case abs_node_K:
      {
         const auto ae = GetPointerS<const abs_node>(node);
         std::string op_0 = PrintNode(ae->op, vppf);
         if(GetPointer<const real_ty_node>(ae->type))
         {
            const auto rt = GetPointerS<const real_ty_node>(ae->type);
            if(rt->bitsize == 80)
            {
               res += "__builtin_fabsl(" + op_0 + ")";
            }
            else if(rt->bitsize == 64)
            {
               res += "__builtin_fabs(" + op_0 + ")";
            }
            else if(rt->bitsize == 32)
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
      case select_node_K:
      {
         const auto ce = GetPointerS<const select_node>(node);
         res = NormalizePredicateForC(ce->op0, PrintNode(ce->op0, vppf)) + " ? " + PrintNode(ce->op1, vppf) + " : " +
               PrintNode(ce->op2, vppf);
         break;
      }
      case ternary_add_node_K:
      {
         const auto te = GetPointerS<const ternary_node>(node);
         res = PrintNode(te->op0, vppf) + " + " + PrintNode(te->op1, vppf) + " + " + PrintNode(te->op2, vppf);
         break;
      }
      case ternary_as_node_K:
      {
         const auto te = GetPointerS<const ternary_node>(node);
         res = PrintNode(te->op0, vppf) + " + " + PrintNode(te->op1, vppf) + " - " + PrintNode(te->op2, vppf);
         break;
      }
      case ternary_sa_node_K:
      {
         const auto te = GetPointerS<const ternary_node>(node);
         res = PrintNode(te->op0, vppf) + " - " + PrintNode(te->op1, vppf) + " + " + PrintNode(te->op2, vppf);
         break;
      }
      case ternary_ss_node_K:
      {
         const auto te = GetPointerS<const ternary_node>(node);
         res = PrintNode(te->op0, vppf) + " - " + PrintNode(te->op1, vppf) + " - " + PrintNode(te->op2, vppf);
         break;
      }
      case fshl_node_K:
      case fshr_node_K:
      {
         const auto te = GetPointerS<const ternary_node>(node);
         const auto type_size = ir_helper::Size(te->type);
         const auto left_op_cast = ir_helper::IsPointerType(te->op0);
         const auto right_op_cast = ir_helper::IsPointerType(te->op1);
         res += "(";
         if(left_op_cast)
         {
            res += "((unsigned long int)";
         }
         res += PrintNode(te->op0, vppf);
         if(left_op_cast)
         {
            res += ")";
         }
         res += " << (";
         if(node->get_kind() == fshl_node_K)
         {
            res += PrintNode(te->op2, vppf);
            res += " % ";
            res += STR(type_size);
         }
         else
         {
            res += STR(type_size);
            res += " - (";
            res += PrintNode(te->op2, vppf);
            res += " % ";
            res += STR(type_size);
            res += ")";
         }
         res += ")) | (";

         if(right_op_cast)
         {
            res += "((unsigned long int)";
         }
         res += PrintNode(te->op1, vppf);
         if(right_op_cast)
         {
            res += ")";
         }
         res += " >> (";
         if(node->get_kind() == fshr_node_K)
         {
            res += PrintNode(te->op2, vppf);
            res += " % ";
            res += STR(type_size);
         }
         else
         {
            res += STR(type_size);
            res += " - (";
            res += PrintNode(te->op2, vppf);
            res += " % ";
            res += STR(type_size);
            res += ")";
         }
         res += "))";
         break;
      }
      case concat_bit_node_K:
      {
         const auto te = GetPointerS<const ternary_node>(node);
         res = PrintNode(te->op0, vppf) + " | (" + PrintNode(te->op1, vppf) + " & ((1ULL<<" + PrintNode(te->op2, vppf) +
               ")-1))";
         break;
      }
      case shufflevector_node_K:
      {
         const auto vpe = GetPointerS<const shufflevector_node>(node);
         const auto element_type = ir_helper::CGetElements(vpe->type);
         const auto element_size = ir_helper::SizeAlloc(element_type);
         const auto size = ir_helper::SizeAlloc(vpe->op0);
         const auto vector_size = size / element_size;
         res += "/*" + vpe->get_kind_text() + "*/";
         res += "(" + ir_helper::PrintType(vpe->type) + ") ";
         res += "{";
         for(unsigned int ind = 0; ind < vector_size; ++ind)
         {
            res += "((((" + PrintNode(vpe->op2, vppf) + ")[" + STR(ind) + "])%" + STR(2 * vector_size) + ") < " +
                   STR(vector_size) + ") ? (" + PrintNode(vpe->op0, vppf) + ")[(((" + PrintNode(vpe->op2, vppf) + ")[" +
                   STR(ind) + "])%" + STR(2 * vector_size) + ")] : (" + PrintNode(vpe->op1, vppf) + ")[(((" +
                   PrintNode(vpe->op2, vppf) + ")[" + STR(ind) + "])%" + STR(2 * vector_size) + ")-" +
                   STR(vector_size) + "]";
            if(ind != vector_size - 1)
            {
               res += ", ";
            }
         }
         res += "}";
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<const multi_way_if_stmt>(node);
         res = "if(";
         bool first = true;
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(first)
            {
               THROW_ASSERT(cond.first, "First condition of multi way if " + STR(node->index) + " is empty");
               res += NormalizePredicateForC(cond.first, PrintNode(cond.first, vppf));
               first = false;
            }
            else if(cond.first)
            {
               res += " /* else if(" + NormalizePredicateForC(cond.first, PrintNode(cond.first, vppf)) + ")*/";
            }
         }
         res += ")";
         break;
      }
      case assign_stmt_K:
      {
         const auto ms = GetPointerS<const assign_stmt>(node);
         res = "";
         if(ir_helper::IsArrayType(ms->op0))
         {
            const auto size = ir_helper::SizeAlloc(ms->op0);
            res += "__builtin_memcpy(";
            if(GetPointer<const mem_access_node>(ms->op0))
            {
               res += "&";
            }
            res += PrintNode(ms->op0, vppf) + ", ";
            if(ms->op1->get_kind() == bitcast_node_K)
            {
               const auto bitcast_expr = GetPointerS<const bitcast_node>(ms->op1);
               res += "&" + PrintNode(bitcast_expr->op, vppf) + ", ";
            }
            else
            {
               if(GetPointer<const mem_access_node>(ms->op1))
               {
                  res += "&";
               }
               res += PrintNode(ms->op1, vppf) + ", ";
            }
            res += STR(size / 8) + ")";
            break;
         }
         if((!Param->getOption<bool>(OPT_without_transformation)) &&
            (ir_helper::IsStructType(ms->op0) && ir_helper::IsStructType(ms->op1) &&
             ir_helper::PrintType((ir_helper::CGetType(ms->op0))) !=
                 ir_helper::PrintType((ir_helper::CGetType(ms->op1)))))
         {
            THROW_ERROR_CODE(C_EC, "Implicit struct type definition not supported in assign_stmt " + STR(node->index) +
                                       " - " + ir_helper::PrintType((ir_helper::CGetType(ms->op0))) + " vs. " +
                                       ir_helper::PrintType((ir_helper::CGetType(ms->op1))));
         }
         res += PrintNode(ms->op0, vppf) + " = ";
         const auto right = ms->op1;
         /// check for type conversion
         switch(right->get_kind())
         {
            case constructor_node_K:
            {
               res += "(" + ir_helper::PrintType(ir_helper::CGetType(ms->op0)) + ") ";
               res += PrintNode(ms->op1, vppf);
               break;
            }
            case constant_vector_val_node_K:
            {
               const auto vc = GetPointerS<const constant_vector_val_node>(right);
               const auto type = ir_helper::CGetType(ms->op0);
               if(type->index != vc->type->index)
               {
                  res += "(" + ir_helper::PrintType(type) + ") ";
               }
               res += PrintNode(ms->op1, vppf);
               break;
            }
            case call_node_K:
            case identifier_node_K:
            case ssa_node_K:
            case statement_list_node_K:
            case constant_int_val_node_K:
            case constant_fp_val_node_K:
            case lut_node_K:
            case CASE_BINARY_NODES:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_NODE_STMTS:
            case CASE_TERNARY_NODES:
            case CASE_TYPE_NODES:
            case CASE_UNARY_NODES:
            {
               const auto left_type = ir_helper::CGetType(ms->op0);
               const auto right_type = ir_helper::CGetType(ms->op1);
               if(ir_helper::IsVectorType(left_type) && left_type->index != right_type->index)
               {
                  res += "(" + ir_helper::PrintType(left_type) + ") ";
               }
               if((right->get_kind() == shr_node_K || right->get_kind() == shl_node_K) &&
                  ir_helper::IsPointerType(GetPointerS<const binary_node>(right)->op0))
               {
                  res += "(unsigned int)";
               }
               res += PrintNode(ms->op1, vppf);
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         const auto bitcast_expr = GetPointer<const bitcast_node>(right);
         if(bitcast_expr && bitcast_expr->op->get_kind() == constant_int_val_node_K)
         {
            const auto dest_type = ir_helper::CGetType(ms->op1);
            const auto source_type = ir_helper::CGetType(bitcast_expr->op);
            res = "{union {" + ir_helper::PrintType(dest_type) + " dest; " + ir_helper::PrintType(source_type) +
                  " source;} __panda_union; __panda_union.source = " + PrintNode(bitcast_expr->op, vppf) + "; " + res;
         }
         if(ms->predicate)
         {
            res = "if(" + NormalizePredicateForC(ms->predicate, PrintNode(ms->predicate, vppf)) + ") " + res;
         }
         if(ms->op1->get_kind() == idiv_node_K || ms->op1->get_kind() == irem_node_K)
         {
            const auto tde = GetPointerS<const binary_node>(ms->op1);
            res = "if(" + PrintNode(tde->op1, vppf) + " != 0) " + res;
         }
         break;
      }
      case nop_stmt_K:
      {
         res += "/*nop_stmt*/";
         break;
      }
      case return_stmt_K:
      {
         const auto re = GetPointerS<const return_stmt>(node);
         res += "return ";
         if(re->op != nullptr)
         {
            const auto return_node = re->op;
            /// check for type conversion
            switch(return_node->get_kind())
            {
               case fptoi_node_K:
               case itofp_node_K:
               case nop_node_K:
               {
                  const auto ue = GetPointerS<const unary_node>(return_node);
                  res += "(" + ir_helper::PrintType(ir_helper::CGetType(re->op)) + ") (";
                  res += PrintNode(ue->op, vppf);
                  res += ")";
                  break;
               }
               case constructor_node_K:
               {
                  res += "(" + ir_helper::PrintType(ir_helper::CGetType(re->op)) + ") ";
                  res += PrintNode(re->op, vppf);
                  break;
               }
               case mem_access_node_K:
               case call_node_K:
               case identifier_node_K:
               case ssa_node_K:
               case statement_list_node_K:
               case abs_node_K:
               case addr_node_K:
               case not_node_K:
               case unaligned_mem_access_node_K:
               case neg_node_K:
               case bitcast_node_K:
               case lut_node_K:
               case CASE_BINARY_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_NODE_STMTS:
               case CASE_TERNARY_NODES:
               case CASE_TYPE_NODES:
               default:
               {
                  res += PrintNode(re->op, vppf);
                  break;
               }
            }
         }
         res += ";";
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointerS<const call_node>(node);
         const function_val_node* fd = nullptr;
         const auto op0 = ce->fn;
         bool is_va_start_end = false;

         auto op0_kind = op0->get_kind();

         switch(op0_kind)
         {
            case addr_node_K:
            {
               const auto ue = GetPointerS<const unary_node>(op0);
               const auto fn = ue->op;
               THROW_ASSERT(fn->get_kind() == function_val_node_K,
                            "IR node not currently supported " + fn->get_kind_text());
               fd = GetPointerS<const function_val_node>(fn);
               ///__builtin_va_start should be ad-hoc managed
               std::string fname = ir_helper::GetFunctionName(fn);
               if(fname == "__builtin_va_start" || fname == "__builtin_va_end" || fname == "__builtin_va_copy")
               {
                  is_va_start_end = true;
               }
               if(fname == "__builtin_constant_p")
               {
                  THROW_ERROR_CODE(C_EC, "Not supported function " + fname);
               }
               else
               {
                  res += fname;
               }
               break;
            }
            case ssa_node_K:
            {
               res += "(*" + PrintNode(ce->fn, vppf) + ")";
               break;
            }
            case mem_access_node_K:
            case call_node_K:
            case constructor_node_K:
            case identifier_node_K:
            case statement_list_node_K:
            case CASE_TERNARY_NODES:
            case lut_node_K:
            case CASE_BINARY_NODES:
            case CASE_CST_NODES:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_NODE_STMTS:
            case CASE_NON_ADDR_UNARY_EXPRESSION:
            case CASE_TYPE_NODES:
            default:
            {
               THROW_ERROR(std::string("IR node not currently supported ") + op0->get_kind_text());
            }
         }
         /// Print parameters.
         res += "(";
         if(is_va_start_end)
         {
            THROW_ASSERT(ce->args.size(), "va_start or va_end have to have arguments");
            const auto par1 = ce->args[0];
            // print the first removing the address
            if(GetPointer<const addr_node>(par1))
            {
               res += PrintNode(GetPointerS<const addr_node>(par1)->op, vppf);
            }
            else if(GetPointer<const variable_val_node>(par1) || GetPointer<const ssa_node>(par1))
            {
               res += "*(" + PrintNode(par1, vppf) + ")";
            }
            else
            {
               THROW_ERROR("expected an address or a variable " + STR(par1->index));
            }
            for(size_t arg_index = 1; arg_index < ce->args.size(); arg_index++)
            {
               res += ", ";
               res += PrintNode(ce->args[arg_index], vppf);
            }
         }
         else
         {
            if(ce->args.size())
            {
               const auto& actual_args = ce->args;
               std::vector<ir_nodeRef> formal_args;
               if(fd)
               {
                  formal_args = fd->list_of_args;
               }
               std::vector<ir_nodeRef>::const_iterator actual_arg, actual_arg_end = actual_args.end();
               std::vector<ir_nodeRef>::const_iterator formal_arg, formal_arg_end = formal_args.end();
               for(actual_arg = actual_args.begin(), formal_arg = formal_args.begin(); actual_arg != actual_arg_end;
                   ++actual_arg)
               {
                  if(formal_arg != formal_arg_end && ir_helper::IsStructType(*actual_arg) &&
                     ir_helper::IsStructType(*formal_arg) &&
                     ((ir_helper::CGetType(*actual_arg))->index != (ir_helper::CGetType(*formal_arg))->index))
                  {
                     THROW_ERROR_CODE(C_EC, "Implicit struct type definition not supported in assign_stmt " +
                                                STR(node->index));
                  }
                  if(actual_arg != actual_args.begin())
                  {
                     res += ", ";
                  }
                  res += PrintNode(*actual_arg, vppf);
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
      case call_stmt_K:
      {
         const auto ce = GetPointerS<const call_stmt>(node);
         const function_val_node* fd = nullptr;
         const auto op0 = ce->fn;
         bool is_va_start_end = false;

         auto op0_kind = op0->get_kind();

         switch(op0_kind)
         {
            case addr_node_K:
            {
               const auto ue = GetPointerS<const unary_node>(op0);
               const auto fn = ue->op;
               THROW_ASSERT(fn->get_kind() == function_val_node_K,
                            "IR node not currently supported " + fn->get_kind_text());
               fd = GetPointerS<const function_val_node>(fn);
               ///__builtin_va_start should be ad-hoc managed
               std::string fname = ir_helper::GetFunctionName(fn);
               if(fname == "__builtin_va_start" || fname == "__builtin_va_end" || fname == "__builtin_va_copy")
               {
                  is_va_start_end = true;
               }
               if(fname == "__builtin_constant_p")
               {
                  THROW_ERROR_CODE(C_EC, "Not supported function " + fname);
               }
               else
               {
                  res += fname;
               }
               break;
            }
            case ssa_node_K:
            {
               res += "(*" + PrintNode(ce->fn, vppf) + ")";
               break;
            }
            case mem_access_node_K:
            case call_node_K:
            case constructor_node_K:
            case identifier_node_K:
            case statement_list_node_K:
            case CASE_TERNARY_NODES:
            case lut_node_K:
            case CASE_BINARY_NODES:
            case CASE_CST_NODES:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_NODE_STMTS:
            case CASE_NON_ADDR_UNARY_EXPRESSION:
            case CASE_TYPE_NODES:
            default:
            {
               THROW_ERROR(std::string("IR node not currently supported ") + op0->get_kind_text());
            }
         }
         /// Print parameters.
         res += "(";
         if(is_va_start_end)
         {
            THROW_ASSERT(ce->args.size(), "va_start or va_end have to have arguments");
            const auto par1 = ce->args[0];
            // print the first removing the address
            if(GetPointer<const addr_node>(par1))
            {
               res += PrintNode(GetPointerS<const addr_node>(par1)->op, vppf);
            }
            else if(GetPointer<const variable_val_node>(par1) || GetPointer<const ssa_node>(par1))
            {
               res += "*(" + PrintNode(par1, vppf) + ")";
            }
            else
            {
               THROW_ERROR("expected an address or a variable " + STR(par1));
            }
            for(size_t arg_index = 1; arg_index < ce->args.size(); arg_index++)
            {
               res += ", ";
               res += PrintNode(ce->args[arg_index], vppf);
            }
         }
         else
         {
            if(ce->args.size())
            {
               const auto& actual_args = ce->args;
               std::vector<ir_nodeRef> formal_args;
               if(fd)
               {
                  formal_args = fd->list_of_args;
               }
               std::vector<ir_nodeRef>::const_iterator actual_arg, actual_arg_end = actual_args.end();
               std::vector<ir_nodeRef>::const_iterator formal_arg, formal_arg_end = formal_args.end();
               for(actual_arg = actual_args.begin(), formal_arg = formal_args.begin(); actual_arg != actual_arg_end;
                   ++actual_arg)
               {
                  if(formal_arg != formal_arg_end && ir_helper::IsStructType(*actual_arg) &&
                     ir_helper::IsStructType(*formal_arg) &&
                     ((ir_helper::CGetType(*actual_arg))->index != (ir_helper::CGetType(*formal_arg))->index))
                  {
                     THROW_ERROR_CODE(C_EC, "Implicit struct type definition not supported in assign_stmt " +
                                                STR(node->index));
                  }
                  if(actual_arg != actual_args.begin())
                  {
                     res += ", ";
                  }
                  res += PrintNode(*actual_arg, vppf);
                  if(formal_arg != formal_arg_end)
                  {
                     ++formal_arg;
                  }
               }
            }
         }
         res += ")";
         if(ce->predicate)
         {
            res = "if(" + NormalizePredicateForC(ce->predicate, PrintNode(ce->predicate, vppf)) + ") " + res;
         }
         break;
      }
      case phi_stmt_K:
      {
         const auto pn = GetPointerS<const phi_stmt>(node);
         res += "/* " + PrintNode(pn->res, vppf) + " = phi_stmt(";
         for(const auto& def_edge : pn->CGetDefEdgesList())
         {
            if(def_edge != pn->CGetDefEdgesList().front())
            {
               res += ", ";
            }
            res += "<" + PrintNode(def_edge.first, vppf) + ", BB" + STR(def_edge.second) + ">";
         }
         res += ") */";
         break;
      }
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
      {
         res = PrintConstant(node, vppf);
         break;
      }
      case ssa_node_K:
      case variable_val_node_K:
      case argument_val_node_K:
      {
         res = (*vppf)(node->index);
         break;
      }
      case field_val_node_K:
      {
         res = PrintVariable(node->index);
         break;
      }
      case constructor_node_K:
      {
         res += PrintInit(node, vppf);
         break;
      }
      case module_unit_node_K:
      case identifier_node_K:
      case statement_list_node_K:
      case CASE_FAKE_NODES:
      case CASE_TYPE_NODES:
      {
         THROW_ERROR(std::string("IR node not currently supported ") + node->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Printed node " + STR(node->index) + " res = " + res);

   return res;
}

std::string BehavioralHelper::print_type_declaration(unsigned int type) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Printing type declaration " + STR(type));
   std::string res;
   const auto node_type = TM->GetIRNode(type);
   switch(node_type->get_kind())
   {
      case struct_ty_node_K:
      {
         const auto rt = GetPointerS<const struct_ty_node>(node_type);
         THROW_ASSERT((node_type)->index == type, "Printing declaration of fake type " + STR(type));

         res += ir_helper::PrintType(node_type) + " ";

         /// Print the contents of the structure
         res += "\n{\n";
         null_deleter null_del;
         const std::unique_ptr<var_pp_functor> std_vpf =
             std::make_unique<std_var_pp_functor>(BehavioralHelperConstRef(this, null_del));
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Printing content of the structure");
         for(auto& list_of_fld : rt->list_of_flds)
         {
            auto field = list_of_fld->index;
            const auto fld_node = TM->GetIRNode(field);
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Printing field " + STR(field));
            const auto fd = GetPointerS<const field_val_node>(fld_node);
            const auto field_type = ir_helper::CGetType(fld_node);
            if(has_bit_field(field))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Bit field");
               res += ir_helper::PrintType(field_type) + " ";
               res += (*std_vpf)(field);
               res += " : ";
               res += STR(fd->bitsizealloc);
            }
            else
            {
               res += ir_helper::PrintType(field_type, false, false, fld_node, std_vpf);
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

         break;
      }
      case array_ty_node_K:
      case pointer_ty_node_K:
      case function_ty_node_K:
      case integer_ty_node_K:
      case real_ty_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
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
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Printed type declaration " + STR(type));
   return res;
}

bool BehavioralHelper::is_bool(unsigned int index) const
{
   return ir_helper::IsBooleanType(TM->GetIRNode(index));
}

bool BehavioralHelper::is_int(unsigned int index) const
{
   return ir_helper::IsSignedIntegerType(TM->GetIRNode(index));
}

bool BehavioralHelper::is_unsigned(unsigned int index) const
{
   return ir_helper::IsUnsignedIntegerType(TM->GetIRNode(index));
}

bool BehavioralHelper::is_real(unsigned int index) const
{
   return ir_helper::IsRealType(TM->GetIRNode(index));
}

bool BehavioralHelper::is_a_struct(unsigned int variable) const
{
   return ir_helper::IsStructType(TM->GetIRNode(variable));
}

bool BehavioralHelper::is_an_array(unsigned int variable) const
{
   return ir_helper::IsArrayEquivType(ir_helper::CGetType(TM->GetIRNode(variable)));
}

bool BehavioralHelper::is_a_vector(unsigned int variable) const
{
   return ir_helper::IsVectorType(TM->GetIRNode(variable));
}

bool BehavioralHelper::is_a_pointer(unsigned int variable) const
{
   return ir_helper::IsPointerType(TM->GetIRNode(variable));
}

bool BehavioralHelper::is_an_indirect_ref(unsigned int variable) const
{
   const auto temp = TM->GetIRNode(variable);
   if(temp->get_kind() == unaligned_mem_access_node_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_an_addr_node(unsigned int variable) const
{
   const auto temp = TM->GetIRNode(variable);
   if(temp->get_kind() == addr_node_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_a_mem_access(unsigned int variable) const
{
   const auto temp = TM->GetIRNode(variable);
   if(temp->get_kind() == mem_access_node_K)
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool BehavioralHelper::is_a_constant(unsigned int obj) const
{
   const auto node = TM->GetIRNode(obj);
   switch(node->get_kind())
   {
      case addr_node_K:
         return true;
      case nop_node_K:
      {
         const auto ue = GetPointerS<const unary_node>(node);
         return is_a_constant(ue->op->index);
      }
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
         return true;
      case mem_access_node_K:
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case field_val_node_K:
      case function_val_node_K:
      case argument_val_node_K:
      case module_unit_node_K:
      case variable_val_node_K:
      case abs_node_K:
      case not_node_K:
      case fptoi_node_K:
      case itofp_node_K:
      case unaligned_mem_access_node_K:
      case neg_node_K:
      case bitcast_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      default:
      {
         return false;
      }
   }
}

bool BehavioralHelper::is_operating_system_function(const unsigned int obj) const
{
   const ir_nodeRef curr_tn = TM->GetIRNode(obj);
   const function_val_node* fd = GetPointer<function_val_node>(curr_tn);
   if(!fd)
   {
      return false;
   }
   return fd->operating_system_flag;
}

unsigned int BehavioralHelper::get_indirect_ref_var(unsigned int obj) const
{
   THROW_ASSERT(is_an_indirect_ref(obj), "obj assumed to be an inderect_ref object");
   const ir_nodeRef temp = TM->GetIRNode(obj);
   auto mir = GetPointer<unaligned_mem_access_node>(temp);
   return mir->op->index;
}

unsigned int BehavioralHelper::get_mem_access_base(unsigned int obj) const
{
   THROW_ASSERT(is_a_mem_access(obj), "obj assumed to be a mem_access_node object");
   const ir_nodeRef temp = TM->GetIRNode(obj);
   auto mr = GetPointer<mem_access_node>(temp);
   return mr->op->index;
}

unsigned int BehavioralHelper::get_operand_from_unary_node(unsigned int obj) const
{
   THROW_ASSERT(is_an_addr_node(obj), "obj assumed to be an addr_node object. obj is " + STR(obj));
   const ir_nodeRef temp = TM->GetIRNode(obj);
   auto ue = GetPointer<unary_node>(temp);
   return ue->op->index;
}

unsigned int BehavioralHelper::GetVarFromSsa(unsigned int index) const
{
   const ir_nodeRef temp = TM->GetIRNode(index);
   auto sn = GetPointer<ssa_node>(temp);
   if(sn)
   {
      return sn->var->index;
   }
   else
   {
      return index;
   }
}

IRNodeConstSet BehavioralHelper::GetParameterTypes() const
{
   IRNodeConstSet ret;
   const auto node = TM->GetIRNode(function_index);
   const auto fd = GetPointer<const function_val_node>(node);
   if(!fd)
   {
      return ret;
   }
   for(const auto& arg : fd->list_of_args)
   {
      const auto pd = GetPointerS<const argument_val_node>(arg);
      ret.insert(pd->type);
   }
   const auto ft = GetPointerS<const function_ty_node>(fd->type);
   THROW_ASSERT(ft, "unexpected pattern");
   for(const auto& p : ft->list_of_args_type)
   {
      ret.insert(p);
   }
   return ret;
}

unsigned int BehavioralHelper::is_named_pointer(const unsigned int index) const
{
   THROW_ASSERT(index, "this index does not exist: " + STR(index));
   const auto type = TM->GetIRNode(index);
   THROW_ASSERT(type, "this index does not exist: " + STR(type));
   const auto Type_node = ir_helper::CGetType(type);
   THROW_ASSERT(Type_node, "this index does not exist: " + STR(type));
   if(Type_node->get_kind() == pointer_ty_node_K)
   {
      const auto pt = GetPointerS<const pointer_ty_node>(Type_node);
      return is_named_pointer(pt->ptd->index);
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
      const ir_nodeRef node = TM->GetIRNode(stm);
      if(node->get_kind() == call_stmt_K)
      {
         const auto ce = GetPointerS<const call_stmt>(node);
         if(ce->fn->get_kind() == addr_node_K)
         {
            const auto ue = GetPointerS<const unary_node>(ce->fn);
            if(ue->op->get_kind() == function_val_node_K && ir_helper::GetFunctionName(ue->op) == "__builtin_va_start")
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
   const ir_nodeRef node = TM->GetIRNode(variable);
   if(node->get_kind() == field_val_node_K)
   {
      auto fd = GetPointer<field_val_node>(node);
      return fd->bitfield;
   }
   else
   {
      return false;
   }
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
      if(TM->GetIRNode(init)->get_kind() == variable_val_node_K)
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
   const ir_nodeRef node = TM->GetIRNode(var);
   switch(node->get_kind())
   {
      case ssa_node_K:
      {
         auto sn = GetPointer<ssa_node>(node);
         if(!sn->var)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Without init");
            return 0;
         }
         const unsigned ssa_init = GetInit(sn->var->index, list_of_variables);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Init is " + STR(ssa_init));
         return ssa_init;
      }
      case variable_val_node_K:
      {
         auto vd = GetPointer<variable_val_node>(node);
         if(vd->init)
         {
            ir_helper::get_used_variables(true, vd->init, list_of_variables);
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
      case constructor_node_K:
      {
         auto co = GetPointerS<const constructor_node>(TM->GetIRNode(var));
         auto vend = co->list_of_idx_valu.end();
         for(auto i = co->list_of_idx_valu.begin(); i != vend; ++i)
         {
            ir_helper::get_used_variables(true, i->second, list_of_variables);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Init is " + STR(var));
         return var;
      }
      case call_node_K:
      case identifier_node_K:
      case statement_list_node_K:
      case field_val_node_K:
      case function_val_node_K:
      case argument_val_node_K:
      case module_unit_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_FAKE_NODES:
      case CASE_NODE_STMTS:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Without init");
         return 0;
      }
      default:
      {
         THROW_UNREACHABLE("get_init: IR node not yet supported " + STR(var) + " " + node->get_kind_text());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Without init");
   return 0;
}

std::string BehavioralHelper::print_phinode_res(unsigned int phi_node_id,
                                                const std::unique_ptr<var_pp_functor>& vppf) const
{
   const auto node = TM->GetIRNode(phi_node_id);
   const auto phi = GetPointer<const phi_stmt>(node);
   THROW_ASSERT(phi, "NodeId is not related to a phi_stmt");
   return PrintNode(phi->res, vppf);
}

unsigned int BehavioralHelper::end_with_a_cond_or_goto(const blocRef& block) const
{
   if(block->CGetStmtList().empty())
   {
      return 0;
   }
   ir_nodeRef last = block->CGetStmtList().back();
   auto gmwi = GetPointer<multi_way_if_stmt>(last);
   if(gmwi)
   {
      return last->index;
   }
   return 0;
}

std::string BehavioralHelper::print_forward_declaration(unsigned int type) const
{
   std::string res;
   const auto node_type = TM->GetIRNode(type);
   switch(node_type->get_kind())
   {
      case struct_ty_node_K:
      {
         const auto rt = GetPointerS<const struct_ty_node>(node_type);
         if(rt->name)
         {
            res += "struct " + ir_helper::PrintType(rt->name);
         }
         else
         {
            res += "struct Internal_" + STR(type);
         }
         break;
      }
      case array_ty_node_K:
      case call_node_K:
      case constructor_node_K:
      case function_ty_node_K:
      case identifier_node_K:
      case integer_ty_node_K:
      case pointer_ty_node_K:
      case real_ty_node_K:
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
         THROW_ERROR("Not yet supported " + std::string(node_type->get_kind_text()));
      }
   }
   return res;
}

std::string BehavioralHelper::print_type(unsigned int type) const
{
   return ir_helper::PrintType(TM->GetIRNode(type));
}

void BehavioralHelper::rename_a_variable(unsigned int var, const std::string& new_name)
{
   vars_renaming_table[var] = new_name;
}

void BehavioralHelper::clear_renaming_table()
{
   vars_renaming_table.clear();
}

void BehavioralHelper::GetTypecast(const ir_nodeConstRef& tn, IRNodeConstSet& types) const
{
   type_casting Visitor(types);
   tn->visit(&Visitor);
}

bool BehavioralHelper::IsDefaultSsaName(const unsigned int ssa_name_index) const
{
   const auto sn = GetPointer<const ssa_node>(TM->GetIRNode(ssa_name_index));
   return sn && sn->default_flag;
}

bool BehavioralHelper::function_has_to_be_printed(unsigned int f_id) const
{
   const auto fnode = TM->GetIRNode(f_id);
   if(ir_helper::GetFunctionName(TM->GetIRNode(function_index)) == "__builtin_select32")
   {
      return false;
   }
   if(ir_helper::IsInLibbambu(fnode))
   {
      return true;
   }
   return !ir_helper::IsSystemType(fnode);
}

bool BehavioralHelper::RequiresPredicationForControlMotion(const unsigned int node_index) const
{
   if(node_index == ENTRY_ID || node_index == EXIT_ID)
   {
      return false;
   }
   const auto tn = TM->GetIRNode(node_index);
   const auto ga = GetPointer<const assign_stmt>(tn);
   if(IsStore(node_index))
   {
      return true;
   }
   if(IsLoad(node_index))
   {
      return true;
   }
   switch(tn->get_kind())
   {
      case nop_stmt_K:
      case phi_stmt_K:
      case multi_way_if_stmt_K:
      case return_stmt_K:
      {
         return false;
      }
      case(assign_stmt_K):
      {
         THROW_ASSERT(ga && ga->op1, "unexpected condition"); // to silence the clang static analyzer
         return ga->op1->get_kind() == call_node_K;
      }
      case call_stmt_K:
      case call_node_K:
      {
         return true;
      }
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      {
         THROW_UNREACHABLE(tn->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   THROW_UNREACHABLE("");
   return false;
}

bool BehavioralHelper::CanBeMovedAcrossControlDependence(const unsigned int node_index) const
{
   if(node_index == ENTRY_ID || node_index == EXIT_ID)
   {
      return false;
   }
   const auto tn = TM->GetIRNode(node_index);
   const auto ga = GetPointer<const assign_stmt>(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "-->Checking if " + STR(tn) + " can be moved across a control dependence");
   if(RequiresPredicationForControlMotion(node_index))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "<--Yes because predication can protect this operation during control motion");
      return true;
   }
   switch(tn->get_kind())
   {
      case nop_stmt_K:
      case phi_stmt_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Yes because is a " + tn->get_kind_text());
         return true;
      }
      case(assign_stmt_K):
      {
         THROW_ASSERT(ga && ga->op1, "unexpected condition"); // to silence the clang static analyzer
         switch(ga->op1->get_kind())
         {
            case CASE_CST_NODES:
            case CASE_DECL_NODES:
            case CASE_TERNARY_NODES:
            case CASE_UNARY_NODES:
            case CASE_BINARY_NODES:
            case ssa_node_K:
            case constructor_node_K:
            case lut_node_K:
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "<--Yes because it is a assign_stmt with " + ga->op1->get_kind_text() +
                                  " in right part of assignment");
               return true;
            }
            case call_node_K:
            case CASE_NODE_STMTS:
            case identifier_node_K:
            case statement_list_node_K:
            case CASE_FAKE_NODES:
            case CASE_TYPE_NODES:
            {
               THROW_UNREACHABLE(ga->op1->get_kind_text() + " - " + STR(tn));
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         return true;
      }
      case multi_way_if_stmt_K:
      case return_stmt_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--No because is a " + tn->get_kind_text());
         return false;
      }
      case call_stmt_K:
      case call_node_K:
      {
         THROW_UNREACHABLE(tn->get_kind_text() + " - " + STR(tn));
         break;
      }
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
      {
         THROW_UNREACHABLE(tn->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
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
   const auto tn = TM->GetIRNode(node_index);
   const auto gn = GetPointer<const node_stmt>(tn);
   THROW_ASSERT(gn, "unexpected condition: node " + STR(tn) + " is not a node_stmt");
   /*
    * artificial node_stmt can never be moved because they are created to
    * handle specific situations, like for example handling functions return
    * structs by value or accepting structs passed by value as parameters
    */
   if(gn->artificial)
   {
      return false;
   }
   const auto gc = GetPointer<const call_stmt>(tn);
   if(gc && gc->fn->get_kind() == addr_node_K)
   {
      // the node is a call_stmt to a function (no function pointers
      const auto addr_ref = gc->fn;
      const auto ae = GetPointerS<const addr_node>(addr_ref);
      THROW_ASSERT(ae->op->get_kind() == function_val_node_K,
                   "node  " + STR(ae->op) + " is not function_val_node but " + ae->op->get_kind_text());
      const auto fu_name = ir_helper::GetFunctionName(ae->op);
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
   return ir_helper::IsStore(TM->GetIRNode(statement_index), fun_mem_data);
}

bool BehavioralHelper::IsLoad(const unsigned int statement_index) const
{
   const auto& fun_mem_data = AppM->CGetFunctionBehavior(function_index)->get_function_mem();
   return ir_helper::IsLoad(TM->GetIRNode(statement_index), fun_mem_data);
}

bool BehavioralHelper::IsLut(const unsigned int statement_index) const
{
   return ir_helper::IsLut(TM->GetIRNode(statement_index));
}

void BehavioralHelper::InvaildateVariableName(const unsigned int index)
{
   vars_symbol_table.erase(index);
}
