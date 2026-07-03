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
 * @file var_decl_fix.cpp
 * @brief Pre-analysis step fixing variable_val_node duplication.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "var_decl_fix.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"

#include <string>

VarDeclFix::VarDeclFix(const application_managerRef _AppM, unsigned int _function_id,
                       const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, VAR_DECL_FIX, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
VarDeclFix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CHECK_SYSTEM_TYPE, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM_DECL_TAKEN_ADDRESS, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool VarDeclFix::HasToBeExecuted() const
{
   return (bb_version == 0 || GetStatus() == DesignFlowStep_Status::UNEXECUTED) &&
          FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status VarDeclFix::InternalExec()
{
   const auto TM = AppM->get_ir_manager();
   const auto fnode = TM->GetIRNode(function_id);
   const auto* fd = GetPointerS<const function_val_node>(fnode);
   const auto* sl = GetPointerS<const statement_list_node>(fd->body);
   const auto fname = ir_helper::GetFunctionName(fnode);

   /// Already considered decl_node
   CustomUnorderedSet<unsigned int> already_examinated_decls;

   /// Already found variable and parameter names
   CustomUnorderedSet<std::string> already_examinated_names;

   /// Already found type names
   CustomUnorderedSet<std::string> already_examinated_type_names;

   /// Already visited address expression (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited_ae;

   bool modified = false;

   // Function parameters are part of the interface and their name is used as key by the user, thus they have precedence
   // and may not be renamed when possible
   for(const auto& arg : fd->list_of_args)
   {
#if HAVE_ASSERTS
      const auto* arg_decl = GetPointerS<argument_val_node>(arg);
      THROW_ASSERT(arg_decl->name->get_kind() == identifier_node_K, "unexpected condition");
      const auto orig_parm_name = GetPointerS<identifier_node>(arg_decl->name)->strg;
#endif
      modified |= recursive_examinate(arg, already_examinated_decls, already_examinated_names,
                                      already_examinated_type_names, already_visited_ae);
      THROW_ASSERT(orig_parm_name == GetPointerS<identifier_node>(arg_decl->name)->strg, "unexpected condition");
   }

   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      for(const auto& stmt : bb->CGetStmtList())
      {
         modified |= recursive_examinate(stmt, already_examinated_decls, already_examinated_names,
                                         already_examinated_type_names, already_visited_ae);
      }
   }

   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool VarDeclFix::recursive_examinate(const ir_nodeRef& curr_tn,
                                     CustomUnorderedSet<unsigned int>& already_examinated_decls,
                                     CustomUnorderedSet<std::string>& already_examinated_names,
                                     CustomUnorderedSet<std::string>& already_examinated_type_names,
                                     CustomUnorderedSet<unsigned int>& already_visited_ae)
{
   bool modified = false;
   const auto TM = AppM->get_ir_manager();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Analyzing recursively " + curr_tn->get_kind_text() + " " + STR(curr_tn->index) + ": " +
                      curr_tn->ToString());
   switch(curr_tn->get_kind())
   {
      case variable_val_node_K:
      case argument_val_node_K:
      {
         if(already_examinated_decls.insert(curr_tn->index).second)
         {
            auto* dn = GetPointerS<decl_node>(curr_tn);
            recursive_examinate(dn->type, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
            if(dn->name)
            {
               const auto decl_name = GetPointerS<const identifier_node>(dn->name)->strg;

               // check if the variable_val_node
               if(curr_tn->get_kind() == variable_val_node_K)
               { /* this is a variable declaration */
                  const auto* cast_res = GetPointerS<variable_val_node>(curr_tn);
                  if(cast_res->static_flag)
                  {
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "Found a static variable with identifier <" + decl_name + "> within function #" +
                                       STR(function_id));
                  }
               }

               if(!already_examinated_names.insert(decl_name).second)
               {
                  const auto name_id = ir_helper::NormalizeTypename(decl_name);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---" + name_id + " is a duplicated " + curr_tn->get_kind_text());
                  /// create a new identifier_node IR node
                  ir_manager::IRSchema IR_schema;
                  unsigned int var_decl_name_nid_test;
                  unsigned int var_decl_unique_id = 0;
                  std::string id0;
                  do
                  {
                     id0 = name_id + STR(var_decl_unique_id++);
                     var_decl_name_nid_test = TM->find_identifier_nodeID(id0);
                  } while(var_decl_name_nid_test);
                  IR_schema[TOK(TOK_STRG)] = id0;
                  dn->name = TM->create_ir_node(identifier_node_K, IR_schema);
                  function_behavior->GetBehavioralHelper()->InvaildateVariableName(dn->index);
                  modified = true;
               }
            }
         }
         break;
      }
      case CASE_UNARY_NODES:
      {
         if(curr_tn->get_kind() == addr_node_K && !already_visited_ae.insert(curr_tn->index).second)
         {
            break;
         }
         const auto* ue = GetPointerS<unary_node>(curr_tn);
         recursive_examinate(ue->op, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
         break;
      }
      case ssa_node_K:
      {
         const auto* sn = GetPointerS<ssa_node>(curr_tn);
         if(sn->var)
         {
            recursive_examinate(sn->var, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         break;
      }
      case CASE_TYPE_NODES:
      {
         if(already_examinated_decls.insert(curr_tn->index).second)
         {
            if(curr_tn->get_kind() == struct_ty_node_K)
            {
               auto* ty = GetPointerS<struct_ty_node>(curr_tn);
               if(!ty->system_flag && ty->name)
               {
                  const auto name_id = GetPointerS<identifier_node>(ty->name)->strg;
                  if(!already_examinated_type_names.insert(name_id).second)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + name_id + " is a duplicated type");
                     /// create a new identifier_node IR node
                     ir_manager::IRSchema IR_schema;
                     unsigned int var_decl_name_nid_test;
                     unsigned int var_decl_unique_id = 0;
                     do
                     {
                        auto id0 = name_id + STR(var_decl_unique_id++);
                        var_decl_name_nid_test = TM->find_identifier_nodeID(id0);
                     } while(var_decl_name_nid_test);
                     ty->name = TM->create_ir_node(identifier_node_K, IR_schema);
                     modified = true;
                  }
               }
            }
         }
         break;
      }
      case call_node_K:
      {
         const auto* ce = GetPointerS<call_node>(curr_tn);
         for(const auto& arg : ce->args)
         {
            recursive_examinate(arg, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                                already_visited_ae);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto* gc = GetPointerS<call_stmt>(curr_tn);
         for(const auto& arg : gc->args)
         {
            recursive_examinate(arg, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                                already_visited_ae);
         }
         if(gc->predicate)
         {
            recursive_examinate(gc->predicate, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         break;
      }
      case assign_stmt_K:
      {
         const auto* gm = GetPointerS<assign_stmt>(curr_tn);
         recursive_examinate(gm->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
         recursive_examinate(gm->op1, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
         if(gm->predicate)
         {
            recursive_examinate(gm->predicate, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto* be = GetPointerS<binary_node>(curr_tn);
         recursive_examinate(be->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
         recursive_examinate(be->op1, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto* te = GetPointerS<ternary_node>(curr_tn);
         recursive_examinate(te->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
         if(te->op1)
         {
            recursive_examinate(te->op1, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         if(te->op2)
         {
            recursive_examinate(te->op2, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         break;
      }
      case lut_node_K:
      {
         const auto* le = GetPointerS<lut_node>(curr_tn);
         recursive_examinate(le->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
         recursive_examinate(le->op1, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
         if(le->op2)
         {
            recursive_examinate(le->op2, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         if(le->op3)
         {
            recursive_examinate(le->op3, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         if(le->op4)
         {
            recursive_examinate(le->op4, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         if(le->op5)
         {
            recursive_examinate(le->op5, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         if(le->op6)
         {
            recursive_examinate(le->op6, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         if(le->op7)
         {
            recursive_examinate(le->op7, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         if(le->op8)
         {
            recursive_examinate(le->op8, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         break;
      }
      case constructor_node_K:
      {
         const auto* co = GetPointerS<constructor_node>(curr_tn);
         for(const auto& [idx, valu] : co->list_of_idx_valu)
         {
            recursive_examinate(valu, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                                already_visited_ae);
         }
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto* gmwi = GetPointerS<multi_way_if_stmt>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               recursive_examinate(cond.first, already_examinated_decls, already_examinated_names,
                                   already_examinated_type_names, already_visited_ae);
            }
         }
         break;
      }
      case return_stmt_K:
      {
         const auto* re = GetPointerS<return_stmt>(curr_tn);
         if(re->op)
         {
            recursive_examinate(re->op, already_examinated_decls, already_examinated_names,
                                already_examinated_type_names, already_visited_ae);
         }
         break;
      }
      case field_val_node_K:
      case function_val_node_K:
      case nop_stmt_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
         break;
      case phi_stmt_K:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case CASE_FAKE_NODES:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + std::string(curr_tn->get_kind_text()));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Analyzed recursively " + STR(curr_tn->index) + ": " + STR(curr_tn));
   return modified;
}
