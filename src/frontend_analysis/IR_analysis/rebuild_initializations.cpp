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
 * @file rebuild_initializations.cpp
 * @brief rebuild initialization where it is possible
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "rebuild_initializations.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "function_behavior.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"

/// STD include
#include <fstream>
#include <utility>
#include <vector>

rebuild_initialization::rebuild_initialization(const ParameterConstRef Param, const application_managerRef _AppM,
                                               unsigned int _function_id,
                                               const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, REBUILD_INITIALIZATION, _design_flow_manager, Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
rebuild_initialization::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

rebuild_initialization::~rebuild_initialization() = default;

DesignFlowStep_Status rebuild_initialization::InternalExec()
{
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   tree_managerRef TM = AppM->get_tree_manager();
   tree_manipulationRef tree_man(new tree_manipulation(TM, parameters, AppM));
   tree_nodeRef tn = TM->GetTreeNode(function_id);
   auto* fd = GetPointerS<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointerS<statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");
   auto B_it_end = sl->list_of_bloc.end();

   TreeNodeMap<std::map<integer_cst_t, tree_nodeRef>> inits;

   /// for each basic block B in CFG do > Consider all blocks successively
   for(auto B_it = sl->list_of_bloc.begin(); B_it != B_it_end; ++B_it)
   {
      blocRef B = B_it->second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(B->number));
      const auto& list_of_stmt = B->CGetStmtList();
      auto it_los_end = list_of_stmt.end();
      auto it_los = list_of_stmt.begin();
      while(it_los != it_los_end)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + (*it_los)->ToString());
         if((*it_los)->get_kind() == gimple_assign_K)
         {
            auto* ga = GetPointerS<gimple_assign>(*it_los);
            enum kind code0 = ga->op0->get_kind();
            INDENT_DBG_MEX(
                DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                "Left part of assignment " + ga->op0->get_kind_text() +
                    (code0 == array_ref_K ? " - Type is " + tree_helper::CGetType(ga->op0)->get_kind_text() : ""));

            /// NOTE: the check has to be performed on the type of the elements of the array and not on the constant in
            /// the right part to avoid rebuilding of array of pointers
            if(code0 == array_ref_K && tree_helper::CGetType(ga->op0)->get_kind() == integer_type_K)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "check for an initialization such as var[const_index] = const_value; " +
                                 STR(ga->op0->index));
               auto* ar = GetPointerS<array_ref>(ga->op0);
               if(ar->op0->get_kind() == var_decl_K && ar->op1->get_kind() == integer_cst_K)
               {
                  auto* vd = GetPointerS<var_decl>(ar->op0);
                  if(vd->readonly_flag)
                  {
                     THROW_ASSERT(!vd->init, "Writing element of read only array already initialized: " + STR(ga->op0));
                     inits[ar->op0][tree_helper::GetConstValue(ar->op1)] = ga->op1;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "<--Statement removed " + (*it_los)->ToString());
                     if(ga->memdef)
                     {
                        const auto gimple_nop_id = TM->new_tree_node_id();
                        std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
                        gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
                        gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
                        TM->create_tree_node(gimple_nop_id, gimple_nop_K, gimple_nop_schema);
                        GetPointerS<ssa_name>(ga->memdef)->SetDefStmt(TM->GetTreeNode(gimple_nop_id));
                     }
                     if(ga->vdef)
                     {
                        const auto gimple_nop_id = TM->new_tree_node_id();
                        std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
                        gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
                        gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
                        TM->create_tree_node(gimple_nop_id, gimple_nop_K, gimple_nop_schema);
                        GetPointerS<ssa_name>(ga->vdef)->SetDefStmt(TM->GetTreeNode(gimple_nop_id));
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + STR(*it_los));
                     B->RemoveStmt(*it_los, AppM);
                     it_los = list_of_stmt.begin();
                     it_los_end = list_of_stmt.end();
                     continue;
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + (*it_los)->ToString());
         ++it_los;
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(B->number));
   }
   const auto integer_type = tree_man->GetSignedIntegerType();
   for(const auto& init : inits)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Rebuilding init of " + STR(init.first));
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> constructor_tree_node_schema;
      const auto array_type = tree_helper::CGetType(init.first);
      constructor_tree_node_schema[TOK(TOK_TYPE)] = STR(array_type->index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Type is " + STR(array_type));
      const auto element_type = tree_helper::CGetElements(array_type);
      auto constructor_index = TM->new_tree_node_id();
      TM->create_tree_node(constructor_index, constructor_K, constructor_tree_node_schema);
      auto* constr = GetPointerS<constructor>(TM->GetTreeNode(constructor_index));
      const auto last_index = init.second.rbegin()->first;
      integer_cst_t index = 0;
      for(index = 0; index <= last_index; index++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(index));
         if(init.second.count(index))
         {
            constr->add_idx_valu(TM->CreateUniqueIntegerCst(index, integer_type), init.second.at(index));
         }
         else
         {
            THROW_ASSERT(element_type->get_kind() == integer_type_K, "Type not supported " + STR(element_type));
            const auto default_value = TM->CreateUniqueIntegerCst(0, element_type);
            constr->add_idx_valu(TM->CreateUniqueIntegerCst(index, integer_type), default_value);
         }
      }
      GetPointerS<var_decl>(init.first)->init = TM->GetTreeNode(constructor_index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Rebuilt init of " + STR(init.first));
   }
   return DesignFlowStep_Status::SUCCESS;
}

rebuild_initialization2::rebuild_initialization2(const ParameterConstRef Param, const application_managerRef _AppM,
                                                 unsigned int _function_id,
                                                 const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, REBUILD_INITIALIZATION2, _design_flow_manager, Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
rebuild_initialization2::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

rebuild_initialization2::~rebuild_initialization2() = default;

static tree_nodeRef extractOp1(tree_nodeRef opSSA)
{
   if(opSSA->get_kind() == integer_cst_K)
   {
      return tree_nodeRef();
   }
   THROW_ASSERT(opSSA->get_kind() == ssa_name_K, "unexpected condition:" + opSSA->ToString());
   auto* ssa_opSSA = GetPointerS<ssa_name>(opSSA);
   auto opSSA_def_stmt = ssa_opSSA->CGetDefStmt();
   if(opSSA_def_stmt->get_kind() == gimple_nop_K || opSSA_def_stmt->get_kind() == gimple_phi_K)
   {
      return tree_nodeRef();
   }
   THROW_ASSERT(opSSA_def_stmt->get_kind() == gimple_assign_K,
                "unexpected condition: " + opSSA_def_stmt->get_kind_text());
   auto* opSSA_assign = GetPointerS<gimple_assign>(opSSA_def_stmt);
   return opSSA_assign->op1;
}

static bool varFound(tree_nodeRef node, unsigned& vd_index, tree_nodeRef& vd_node)
{
   THROW_ASSERT(node->get_kind() == addr_expr_K, "unexpected condition");
   auto* ae = GetPointerS<addr_expr>(node);
   auto ae_op = ae->op;
   if(ae_op->get_kind() == parm_decl_K)
   {
      return false;
   }
   THROW_ASSERT(ae_op->get_kind() == var_decl_K, "unexpected condition: " + ae_op->get_kind_text());
   vd_index = ae->op->index;
   vd_node = ae->op;
   return true;
}

#define REBUILD2_DEVEL 0
#if REBUILD2_DEVEL
#define unexpetedPattern(node) \
   THROW_ERROR("unexpected condition: " + node->get_kind_text() + " --- " + node->ToString());
#else
static bool unexpetedPattern(tree_nodeRef)
{
   return false;
}
#endif

bool rebuild_initialization2::extract_var_decl_ppe(tree_nodeRef addr_assign_op1, unsigned& vd_index,
                                                   tree_nodeRef& vd_node)
{
   auto* ppe = GetPointerS<pointer_plus_expr>(addr_assign_op1);
   auto ppe_op0 = ppe->op0;
   auto addr2_assign_op1 = extractOp1(ppe_op0);
   if(!addr2_assign_op1)
   {
      return false;
   }
   if(addr2_assign_op1->get_kind() == view_convert_expr_K || addr2_assign_op1->get_kind() == nop_expr_K)
   {
      auto* ue = GetPointerS<unary_expr>(addr2_assign_op1);
      auto ue_op = ue->op;
      auto addr3_assign_op1 = extractOp1(ue_op);
      if(!addr3_assign_op1)
      {
         return false;
      }
      if(addr3_assign_op1->get_kind() == addr_expr_K)
      {
         return varFound(addr3_assign_op1, vd_index, vd_node);
      }
      else if(ppe->op1->get_kind() == integer_cst_K && tree_helper::GetConstValue(ppe->op1) == 0)
      {
         if(addr3_assign_op1->get_kind() == ssa_name_K)
         {
            auto addr4_assign_op1 = extractOp1(addr3_assign_op1);
            if(!addr4_assign_op1)
            {
               return false;
            }
            if(addr4_assign_op1->get_kind() == pointer_plus_expr_K)
            {
               addr_assign_op1 = addr4_assign_op1;
               return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
            }
            else if(addr4_assign_op1->get_kind() == addr_expr_K)
            {
               return varFound(addr4_assign_op1, vd_index, vd_node);
            }
            else if(addr4_assign_op1->get_kind() == ssa_name_K)
            {
               auto addr5_assign_op1 = extractOp1(addr4_assign_op1);
               if(!addr5_assign_op1)
               {
                  return false;
               }
               if(addr5_assign_op1->get_kind() == pointer_plus_expr_K)
               {
                  addr_assign_op1 = addr5_assign_op1;
                  return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
               }
               else
               {
                  return unexpetedPattern(addr5_assign_op1);
               }
            }
            else if(addr4_assign_op1->get_kind() == mem_ref_K)
            {
               return false;
            }
            else
            {
               return unexpetedPattern(addr4_assign_op1);
            }
         }
         else if(addr3_assign_op1->get_kind() == pointer_plus_expr_K)
         {
            addr_assign_op1 = addr3_assign_op1;
            return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
         }
         else
         {
            return unexpetedPattern(addr3_assign_op1);
         }
      }
      else if(addr3_assign_op1->get_kind() == ssa_name_K) /// starting from this condition offset is not anymore null
      {
         auto addr4_assign_op1 = extractOp1(addr3_assign_op1);
         if(!addr4_assign_op1)
         {
            return false;
         }
         if(addr4_assign_op1->get_kind() == addr_expr_K)
         {
            return varFound(addr4_assign_op1, vd_index, vd_node);
         }
         else if(addr4_assign_op1->get_kind() == ssa_name_K)
         {
            auto addr5_assign_op1 = extractOp1(addr4_assign_op1);
            if(!addr5_assign_op1)
            {
               return false;
            }
            return unexpetedPattern(addr5_assign_op1);
         }
         else if(addr4_assign_op1->get_kind() == nop_expr_K)
         {
            auto* ne1 = GetPointerS<nop_expr>(addr4_assign_op1);
            auto ne1_op = ne1->op;
            auto addr5_assign_op1 = extractOp1(ne1_op);
            if(!addr5_assign_op1)
            {
               return false;
            }
            return unexpetedPattern(addr5_assign_op1);
         }
         else if(addr4_assign_op1->get_kind() == pointer_plus_expr_K)
         {
            return false;
         }
         else
         {
            return unexpetedPattern(addr4_assign_op1);
         }
      }
      else if(addr3_assign_op1->get_kind() == view_convert_expr_K)
      {
         auto* ue1 = GetPointerS<unary_expr>(addr3_assign_op1);
         auto ue1_op = ue1->op;
         if(ue1_op->get_kind() == ssa_name_K)
         {
            auto addr4_assign_op1 = extractOp1(ue1_op);
            if(!addr4_assign_op1)
            {
               return false;
            }
            if(addr4_assign_op1->get_kind() == addr_expr_K)
            {
               return varFound(addr4_assign_op1, vd_index, vd_node);
            }
            else if(addr4_assign_op1->get_kind() == pointer_plus_expr_K)
            {
               return false;
            }
            else
            {
               return unexpetedPattern(addr4_assign_op1);
            }
         }
         else
         {
            return unexpetedPattern(ue1_op);
         }
      }
      else if(addr3_assign_op1->get_kind() == pointer_plus_expr_K)
      {
         return false;
      }
      else if(addr3_assign_op1->get_kind() == plus_expr_K)
      {
         return false;
      }
      else if(addr3_assign_op1->get_kind() == call_expr_K)
      {
         return false;
      }
      else
      {
         return unexpetedPattern(addr3_assign_op1);
      }
   }
   else if(addr2_assign_op1->get_kind() == addr_expr_K)
   {
      return varFound(addr2_assign_op1, vd_index, vd_node);
   }
   else if(addr2_assign_op1->get_kind() == ssa_name_K)
   {
      auto addr3_assign_op1 = extractOp1(addr2_assign_op1);
      if(!addr3_assign_op1)
      {
         return false;
      }
      if(addr3_assign_op1->get_kind() == pointer_plus_expr_K)
      {
         if(ppe->op1->get_kind() == integer_cst_K && tree_helper::GetConstValue(ppe->op1) == 0)
         {
            addr_assign_op1 = addr3_assign_op1;
            return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
         }
         else
         {
            return false;
         }
      }
      else if(addr3_assign_op1->get_kind() == addr_expr_K)
      {
         return varFound(addr3_assign_op1, vd_index, vd_node);
      }
      else
      {
         return unexpetedPattern(addr3_assign_op1);
      }
   }
   else if(addr2_assign_op1->get_kind() == pointer_plus_expr_K)
   {
      return false;
   }
   else if(addr2_assign_op1->get_kind() == mem_ref_K)
   {
      return false;
   }
   else if(addr2_assign_op1->get_kind() == call_expr_K)
   {
      return false;
   }
   else if(addr2_assign_op1->get_kind() == cond_expr_K)
   {
      return false;
   }
   else
   {
      return unexpetedPattern(addr2_assign_op1);
   }
}

bool rebuild_initialization2::extract_var_decl(const mem_ref* me, unsigned& vd_index, tree_nodeRef& vd_node,
                                               tree_nodeRef& addr_assign_op1)
{
   auto me_op1 = me->op1;
   THROW_ASSERT(me_op1->get_kind() == integer_cst_K, "unexpected condition");
   THROW_ASSERT(tree_helper::GetConstValue(me->op1) == 0, "unexpected condition");
   auto me_op0 = me->op0;
   addr_assign_op1 = extractOp1(me_op0);
   if(!addr_assign_op1)
   {
      return false;
   }
   if(addr_assign_op1->get_kind() == pointer_plus_expr_K)
   {
      return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
   }
   else if(addr_assign_op1->get_kind() == ssa_name_K)
   {
      auto addr2_assign_op1 = extractOp1(addr_assign_op1);
      if(!addr2_assign_op1)
      {
         return false;
      }
      if(addr2_assign_op1->get_kind() == nop_expr_K)
      {
         auto* ne = GetPointerS<nop_expr>(addr2_assign_op1);
         auto ne_op = ne->op;
         auto addr3_assign_op1 = extractOp1(ne_op);
         if(!addr3_assign_op1)
         {
            return false;
         }
         return unexpetedPattern(addr3_assign_op1);
      }
      else if(addr2_assign_op1->get_kind() == pointer_plus_expr_K)
      {
         addr_assign_op1 = addr2_assign_op1;
         return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
      }
      else if(addr2_assign_op1->get_kind() == addr_expr_K)
      {
         return varFound(addr2_assign_op1, vd_index, vd_node);
      }
      else if(addr2_assign_op1->get_kind() == mem_ref_K)
      {
         return false;
      }
      else
      {
         return unexpetedPattern(addr2_assign_op1);
      }
   }
   else if(addr_assign_op1->get_kind() == addr_expr_K)
   {
      return varFound(addr_assign_op1, vd_index, vd_node);
   }
   else if(addr_assign_op1->get_kind() == view_convert_expr_K || addr_assign_op1->get_kind() == nop_expr_K ||
           addr_assign_op1->get_kind() == convert_expr_K)
   {
      auto* ue = GetPointerS<unary_expr>(addr_assign_op1);
      auto ue_op = ue->op;
      auto addr1_assign_op1 = extractOp1(ue_op);
      if(!addr1_assign_op1)
      {
         return false;
      }
      if(addr1_assign_op1->get_kind() == addr_expr_K)
      {
         return varFound(addr1_assign_op1, vd_index, vd_node);
      }
      else if(addr1_assign_op1->get_kind() == ssa_name_K)
      {
         auto addr2_assign_op1 = extractOp1(addr1_assign_op1);
         if(!addr2_assign_op1)
         {
            return false;
         }
         if(addr2_assign_op1->get_kind() == pointer_plus_expr_K)
         {
            addr_assign_op1 = addr2_assign_op1;
            return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
         }
         return unexpetedPattern(addr2_assign_op1);
      }
      else if(addr1_assign_op1->get_kind() == nop_expr_K)
      {
         auto* ne1 = GetPointerS<unary_expr>(addr1_assign_op1);
         auto ne1_op = ne1->op;
         auto addr2_assign_op1 = extractOp1(ne1_op);
         if(!addr2_assign_op1)
         {
            return false;
         }
         if(addr2_assign_op1->get_kind() == ssa_name_K)
         {
            auto addr3_assign_op1 = extractOp1(ne1_op);
            if(!addr3_assign_op1)
            {
               return false;
            }
            if(addr3_assign_op1->get_kind() == pointer_plus_expr_K)
            {
               addr_assign_op1 = addr3_assign_op1;
               return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
            }
            else
            {
               return unexpetedPattern(addr3_assign_op1);
            }
         }
         else
         {
            return unexpetedPattern(addr2_assign_op1);
         }
      }
      else if(addr1_assign_op1->get_kind() == pointer_plus_expr_K)
      {
         addr_assign_op1 = addr1_assign_op1;
         return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
      }
      else
      {
         return unexpetedPattern(addr1_assign_op1);
      }
   }
   else if(addr_assign_op1->get_kind() == mem_ref_K)
   {
      return false;
   }
   else if(addr_assign_op1->get_kind() == call_expr_K)
   {
      return false;
   }
   else if(addr_assign_op1->get_kind() == cond_expr_K)
   {
      return false;
   }
   else
   {
      return unexpetedPattern(addr_assign_op1);
   }
}

#define foundNonConstant(VD)          \
   do                                 \
   {                                  \
      nonConstantVars.insert(VD);     \
      auto key = TM->GetTreeNode(VD); \
      inits.erase(key);               \
   } while(0)

#if REBUILD2_DEVEL
#define unexpetedPattern2(node, VD) \
   THROW_ERROR("unexpected condition: " + node->get_kind_text() + " --- " + node->ToString());
#else
#define unexpetedPattern2(node, VD) foundNonConstant(VD)
#endif

tree_nodeRef getAssign(tree_nodeRef SSAop, unsigned vd_index, CustomOrderedSet<unsigned>& nonConstantVars,
                       TreeNodeMap<std::map<integer_cst_t, tree_nodeRef>>& inits, tree_managerRef TM)
{
   THROW_ASSERT(SSAop->get_kind() == ssa_name_K, "unexpected condition");
   auto* ssa_var = GetPointerS<ssa_name>(SSAop);
   auto ssa_def_stmt = ssa_var->CGetDefStmt();
   if(ssa_def_stmt->get_kind() == gimple_nop_K || ssa_def_stmt->get_kind() == gimple_phi_K)
   {
      nonConstantVars.insert(vd_index);
      auto key = TM->GetTreeNode(vd_index);
      inits.erase(key);
      return tree_nodeRef();
   }
   else
   {
      THROW_ASSERT(ssa_def_stmt->get_kind() == gimple_assign_K,
                   "unexpected condition: " + ssa_def_stmt->get_kind_text());
      auto* assign = GetPointerS<gimple_assign>(ssa_def_stmt);
      return assign->op1;
   }
}

bool rebuild_initialization2::look_for_ROMs()
{
   tree_managerRef TM = AppM->get_tree_manager();
   tree_manipulationRef tree_man(new tree_manipulation(TM, parameters, AppM));
   tree_nodeRef tn = TM->GetTreeNode(function_id);
   auto* fd = GetPointerS<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointerS<statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");
   bool not_supported = false;
   std::map<unsigned, unsigned> var_writing_BB_relation;
   std::map<unsigned, unsigned long long> var_writing_size_relation;
   std::map<unsigned, unsigned long long> var_writing_elts_size_relation;
   CustomOrderedSet<unsigned> nonConstantVars;
   TreeNodeMap<std::map<integer_cst_t, tree_nodeRef>> inits;

   /// for each basic block B in CFG compute constantVars candidates
   for(const auto& Bit : sl->list_of_bloc)
   {
      // used to collect the reads of a given variable done in the same basic block.
      // This is done to avoid to classify a variable constant in case is first
      // written the read and then written again.
      CustomOrderedSet<unsigned> VarsReadSeen;
      auto B = Bit.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining for write BB" + STR(B->number));
      const auto& list_of_stmt = B->CGetStmtList();
      for(const auto& inst : list_of_stmt)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + inst->ToString());
         auto gn = GetPointerS<gimple_node>(inst);
         auto stmt_kind = inst->get_kind();
         if(gn->vdef && stmt_kind != gimple_assign_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement: pattern not supported");
            not_supported = true;
            break;
         }
         if(stmt_kind == gimple_assign_K && gn->vdef)
         {
            auto ga = GetPointerS<gimple_assign>(inst);
            auto op0 = ga->op0;
            auto op1 = ga->op1;
            if(op0->get_kind() == mem_ref_K)
            {
               unsigned vd_index = 0;
               tree_nodeRef vd_node;
               tree_nodeRef addr_assign_op1;
               auto* me = GetPointerS<mem_ref>(op0);
               auto resolved = extract_var_decl(me, vd_index, vd_node, addr_assign_op1);
               if(resolved && nonConstantVars.find(vd_index) == nonConstantVars.end())
               {
                  THROW_ASSERT(vd_index && vd_node, "unexpected condition");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---variable written: " + TM->GetTreeNode(vd_index)->ToString());
                  /// are we writing a constant value
                  if(!GetPointer<cst_node>(op1))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---variable is not constant(1): " + TM->GetTreeNode(vd_index)->ToString());
                     foundNonConstant(vd_index);
                  }
                  if(nonConstantVars.find(vd_index) == nonConstantVars.end())
                  {
                     if(VarsReadSeen.find(vd_index) != VarsReadSeen.end())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---variable is not constant(2): " + TM->GetTreeNode(vd_index)->ToString());
                        foundNonConstant(vd_index);
                     }
                     else if(var_writing_BB_relation.find(vd_index) == var_writing_BB_relation.end())
                     {
                        /// first check if the variable is initialized
                        auto* vd = GetPointerS<var_decl>(vd_node);
                        if(vd->init)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---variable is initialized: " + TM->GetTreeNode(vd_index)->ToString());
                           foundNonConstant(vd_index);
                        }
                        else if(not vd->scpe or vd->scpe->get_kind() == translation_unit_decl_K)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---variable is not local: " + TM->GetTreeNode(vd_index)->ToString());
                           foundNonConstant(vd_index);
                        }
                        else
                        {
                           auto Type = vd->type;
                           /// then we check if the variable is an array
                           if(Type->get_kind() == array_type_K)
                           {
                              std::vector<unsigned long long> dims;
                              unsigned long long elts_size;
                              auto type_index = tree_helper::CGetType(vd_node)->index;
                              tree_helper::get_array_dim_and_bitsize(TM, type_index, dims, elts_size);
                              if(dims.size() == 1)
                              {
                                 /// then in case we are fine we classify as good candidate for being a constant var
                                 var_writing_BB_relation[vd_index] = B->number;
                                 var_writing_size_relation[vd_index] = dims[0];
                                 var_writing_elts_size_relation[vd_index] = elts_size;
                              }
                              else
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---variable is not constant(3): " +
                                                    TM->GetTreeNode(vd_index)->ToString());
                                 foundNonConstant(vd_index);
                              }
                           }
                           else
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---variable is currently classified as non-constant: " +
                                                 TM->GetTreeNode(vd_index)->ToString());
                              foundNonConstant(vd_index);
                           }
                        }
                     }
                     else if(var_writing_BB_relation.find(vd_index)->second != B->number)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---variable is not constant(4): " + TM->GetTreeNode(vd_index)->ToString());
                        foundNonConstant(vd_index);
                     }
                     /// if it is still a good candidate
                     if(nonConstantVars.find(vd_index) == nonConstantVars.end())
                     {
                        /// check if the offset is constant
                        if(addr_assign_op1->get_kind() == pointer_plus_expr_K)
                        {
                           auto* ppe = GetPointerS<pointer_plus_expr>(addr_assign_op1);
                           auto ppe_op1 = ppe->op1;
                           if(ppe_op1->get_kind() == ssa_name_K)
                           {
                              auto offset_assign_op1 = getAssign(ppe_op1, vd_index, nonConstantVars, inits, TM);
                              if(!offset_assign_op1)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---variable is not constant(9): " +
                                                    TM->GetTreeNode(vd_index)->ToString());
                              }
                              else
                              {
                                 if(offset_assign_op1->get_kind() == lshift_expr_K)
                                 {
                                    auto* ls = GetPointerS<lshift_expr>(offset_assign_op1);
                                    auto ls_op1 = ls->op1;
                                    if(ls_op1->get_kind() == integer_cst_K)
                                    {
                                       THROW_ASSERT(tree_helper::GetConstValue(ls->op1) >= 0, "");
                                       auto nbit = static_cast<unsigned long long>(tree_helper::GetConstValue(ls->op1));
                                       THROW_ASSERT(nbit < 32, "unexpected condition");
                                       std::vector<unsigned long long> dims;
                                       THROW_ASSERT(var_writing_elts_size_relation.find(vd_index) !=
                                                        var_writing_elts_size_relation.end(),
                                                    "unexpected condition");
                                       auto elts_size = var_writing_elts_size_relation[vd_index];
                                       if(elts_size != (8ULL << nbit))
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---variable is not constant(9c): " +
                                                             TM->GetTreeNode(vd_index)->ToString());
                                          foundNonConstant(vd_index);
                                       }
                                       else
                                       {
                                          auto ls_op0 = ls->op0;
                                          if(ls_op0->get_kind() == ssa_name_K)
                                          {
                                             auto nop_assign_op1 =
                                                 getAssign(ls_op0, vd_index, nonConstantVars, inits, TM);
                                             if(!nop_assign_op1)
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---variable is not constant(9): " +
                                                                   TM->GetTreeNode(vd_index)->ToString());
                                             }
                                             else
                                             {
                                                if(nop_assign_op1->get_kind() == nop_expr_K)
                                                {
                                                   auto* ne = GetPointerS<nop_expr>(nop_assign_op1);
                                                   auto ne_op = ne->op;
                                                   if(ne_op->get_kind() == integer_cst_K)
                                                   {
                                                      /// index is constant
                                                      inits[vd_node][tree_helper::GetConstValue(ne->op)] = ga->op1;
                                                   }
                                                   else
                                                   {
                                                      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                     "---variable is not constant(5a): " +
                                                                         TM->GetTreeNode(vd_index)->ToString());
                                                      foundNonConstant(vd_index);
                                                   }
                                                }
                                                else if(nop_assign_op1->get_kind() == integer_cst_K)
                                                {
                                                   /// index is constant
                                                   inits[vd_node][tree_helper::GetConstValue(nop_assign_op1)] = ga->op1;
                                                }
                                                else if(nop_assign_op1->get_kind() != view_convert_expr_K)
                                                {
                                                   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                  "---variable is not constant(5b): " +
                                                                      TM->GetTreeNode(vd_index)->ToString());
                                                   foundNonConstant(vd_index);
                                                }
                                                else
                                                {
                                                   unexpetedPattern2(nop_assign_op1, vd_index);
                                                }
                                             }
                                          }
                                          else
                                          {
                                             unexpetedPattern2(ls_op0, vd_index);
                                          }
                                       }
                                    }
                                    else
                                    {
                                       unexpetedPattern2(ls_op1, vd_index);
                                    }
                                 }
                                 else if(offset_assign_op1->get_kind() == ssa_name_K)
                                 {
                                    auto offset_assign1_op1 =
                                        getAssign(offset_assign_op1, vd_index, nonConstantVars, inits, TM);
                                    if(!offset_assign1_op1)
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---variable is not constant(9): " +
                                                          TM->GetTreeNode(vd_index)->ToString());
                                    }
                                    else
                                    {
                                       if(offset_assign1_op1->get_kind() == nop_expr_K)
                                       {
                                          auto* ne = GetPointerS<nop_expr>(offset_assign1_op1);
                                          auto ne_op = ne->op;
                                          if(ne_op->get_kind() == integer_cst_K)
                                          {
                                             inits[vd_node][tree_helper::GetConstValue(ne->op)] = ga->op1;
                                          }
                                          else if(ne_op->get_kind() == ssa_name_K)
                                          {
                                             auto offset_assign2_op1 =
                                                 getAssign(ne_op, vd_index, nonConstantVars, inits, TM);
                                             if(!offset_assign2_op1)
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---variable is not constant(5c): " +
                                                                   TM->GetTreeNode(vd_index)->ToString());
                                             }
                                             else if(offset_assign2_op1->get_kind() == integer_cst_K)
                                             {
                                                inits[vd_node][tree_helper::GetConstValue(offset_assign2_op1)] =
                                                    ga->op1;
                                             }
                                             else
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---variable is not constant(5c): " +
                                                                   TM->GetTreeNode(vd_index)->ToString());
                                                foundNonConstant(vd_index);
                                             }
                                          }
                                          else
                                          {
                                             unexpetedPattern2(ne_op, vd_index);
                                          }
                                       }
                                       else
                                       {
                                          unexpetedPattern2(offset_assign1_op1, vd_index);
                                       }
                                    }
                                 }
                                 else if(offset_assign_op1->get_kind() == nop_expr_K)
                                 {
                                    auto* ne = GetPointerS<nop_expr>(offset_assign_op1);
                                    auto ne_op = ne->op;
                                    if(ne_op->get_kind() == ssa_name_K)
                                    {
                                       auto offset_assign3_op1 = getAssign(ne_op, vd_index, nonConstantVars, inits, TM);
                                       if(!offset_assign3_op1)
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---variable is not constant(9): " +
                                                             TM->GetTreeNode(vd_index)->ToString());
                                       }
                                       else
                                       {
                                          unexpetedPattern2(offset_assign3_op1, vd_index);
                                       }
                                    }
                                    else
                                    {
                                       unexpetedPattern2(ne_op, vd_index);
                                    }
                                 }
                                 else
                                 {
                                    unexpetedPattern2(offset_assign_op1, vd_index);
                                 }
                              }
                           }
                           else if(ppe_op1->get_kind() == integer_cst_K)
                           {
                              THROW_ASSERT(var_writing_elts_size_relation.find(vd_index) !=
                                               var_writing_elts_size_relation.end(),
                                           "unexpected condition");
                              inits[vd_node]
                                   [tree_helper::GetConstValue(ppe_op1) /
                                    (static_cast<integer_cst_t>(var_writing_elts_size_relation[vd_index]) / 8)] =
                                       ga->op1;
                           }
                           else
                           {
                              unexpetedPattern2(ppe_op1, vd_index);
                           }
                        }
                        else if(addr_assign_op1->get_kind() == view_convert_expr_K)
                        {
                           inits[vd_node][0] = ga->op1;
                        }
                        else if(addr_assign_op1->get_kind() == ssa_name_K)
                        {
                           inits[vd_node][0] = ga->op1;
                        }
                        else if(addr_assign_op1->get_kind() == addr_expr_K)
                        {
                           inits[vd_node][0] = ga->op1;
                        }
                        else
                        {
                           unexpetedPattern2(addr_assign_op1, vd_index);
                        }
                     }
                  }
                  /// else do nothing
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Examined statement: pattern not supported");
                  not_supported = true;
                  break;
               }
            }
            else if(op0->get_kind() == ssa_name_K && op1->get_kind() == call_expr_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement: IPA is required");
               not_supported = true;
               break;
            }
            else if(op0->get_kind() == realpart_expr_K || op0->get_kind() == imagpart_expr_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement: Not supported");
               not_supported = true;
               break;
            }
            else if(op0->get_kind() == ssa_name_K && op1->get_kind() == mem_ref_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement: Not supported");
               not_supported = true;
               break;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Examined statement: Not supported" + op0->get_kind_text());
               not_supported = true;
               break;
            }
         }
         else if(stmt_kind == gimple_assign_K && !gn->vuses.empty())
         {
            auto ga = GetPointerS<gimple_assign>(inst);
            auto op1 = ga->op1;
            if(op1->get_kind() == mem_ref_K)
            {
               unsigned vd_index = 0;
               tree_nodeRef vd_node;
               tree_nodeRef dummy_var;
               auto* me = GetPointerS<mem_ref>(op1);
               auto resolved = extract_var_decl(me, vd_index, vd_node, dummy_var);
               if(resolved)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---variable read: " + TM->GetTreeNode(vd_index)->ToString());
                  VarsReadSeen.insert(vd_index);
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Examined statement: pattern not supported");
                  not_supported = true;
                  break;
               }
            }
            else
            {
               for(auto var_written : var_writing_BB_relation)
               {
                  if(var_written.second == B->number)
                  {
                     VarsReadSeen.insert(var_written.first);
                  }
               }
            }
         }
         else if(!gn->vuses.empty())
         {
            for(auto var_written : var_writing_BB_relation)
            {
               if(var_written.second == B->number)
               {
                  VarsReadSeen.insert(var_written.first);
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined for write BB" + STR(B->number));
      if(not_supported)
      {
         break;
      }
   }
   if(not_supported || var_writing_BB_relation.empty() || var_writing_BB_relation.size() == nonConstantVars.size())
   {
      return false;
   }

   /// compute the CFG
   BBGraphsCollectionRef GCC_bb_graphs_collection(
       new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(AppM, function_id)), parameters));
   BBGraphRef GCC_bb_graph(new BBGraph(GCC_bb_graphs_collection, CFG_SELECTOR));
   CustomUnorderedMap<unsigned int, vertex> inverse_vertex_map;
   /// add vertices
   for(const auto& block : sl->list_of_bloc)
   {
      inverse_vertex_map[block.first] =
          GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second)));
   }
   /// add edges
   for(const auto& curr_bb_pair : sl->list_of_bloc)
   {
      auto curr_bb = curr_bb_pair.first;
      auto lop_it_end = sl->list_of_bloc[curr_bb]->list_of_pred.end();
      for(auto lop_it = sl->list_of_bloc[curr_bb]->list_of_pred.begin(); lop_it != lop_it_end; ++lop_it)
      {
         THROW_ASSERT(inverse_vertex_map.find(*lop_it) != inverse_vertex_map.end(),
                      "BB" + STR(*lop_it) + " (successor of BB" + STR(curr_bb) + ") does not exist");
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[*lop_it], inverse_vertex_map[curr_bb], CFG_SELECTOR);
      }
      auto los_it_end = sl->list_of_bloc[curr_bb]->list_of_succ.end();
      for(auto los_it = sl->list_of_bloc[curr_bb]->list_of_succ.begin(); los_it != los_it_end; ++los_it)
      {
         if(*los_it == bloc::EXIT_BLOCK_ID)
         {
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[curr_bb], inverse_vertex_map[*los_it], CFG_SELECTOR);
         }
      }
      if(sl->list_of_bloc[curr_bb]->list_of_succ.empty())
      {
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[curr_bb], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                           CFG_SELECTOR);
      }
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                     CFG_SELECTOR);

   /// check if reads are consistent with writes: writes are always dominating the following reads
   for(const auto& Bit : sl->list_of_bloc)
   {
      auto B = Bit.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining for write BB" + STR(B->number));
      const auto& list_of_stmt = B->CGetStmtList();
      for(const auto& inst : list_of_stmt)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + inst->ToString());
         auto gn = GetPointerS<gimple_node>(inst);
         auto stmt_kind = inst->get_kind();
         if(stmt_kind == gimple_assign_K && !gn->vuses.empty())
         {
            auto ga = GetPointerS<gimple_assign>(inst);
            auto op1 = ga->op1;
            if(op1->get_kind() == mem_ref_K)
            {
               unsigned vd_index = 0;
               tree_nodeRef vd_node;
               tree_nodeRef dummy_var;
               auto* me = GetPointerS<mem_ref>(op1);
               auto resolved = extract_var_decl(me, vd_index, vd_node, dummy_var);
               if(resolved && nonConstantVars.find(vd_index) == nonConstantVars.end())
               {
                  if(var_writing_BB_relation.find(vd_index) != var_writing_BB_relation.end())
                  {
                     auto BB_written = var_writing_BB_relation.find(vd_index)->second;
                     if(GCC_bb_graph->IsReachable(inverse_vertex_map[BB_written], inverse_vertex_map[B->number]))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---variable is not constant(6): " + TM->GetTreeNode(vd_index)->ToString());
                        foundNonConstant(vd_index);
                     }
                  }
               }
               else
               {
                  THROW_ASSERT(resolved, "unexpected condition");
               }
            }
            else
            {
               for(auto var_written : var_writing_BB_relation)
               {
                  if(nonConstantVars.find(var_written.first) == nonConstantVars.end())
                  {
                     auto BB_written = var_written.second;
                     if(GCC_bb_graph->IsReachable(inverse_vertex_map[BB_written], inverse_vertex_map[B->number]))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---variable is not constant(7): " +
                                           TM->GetTreeNode(var_written.first)->ToString());
                        foundNonConstant(var_written.first);
                     }
                  }
               }
            }
         }
         else if(!gn->vuses.empty())
         {
            for(auto var_written : var_writing_BB_relation)
            {
               if(nonConstantVars.find(var_written.first) == nonConstantVars.end())
               {
                  auto BB_written = var_written.second;
                  if(GCC_bb_graph->IsReachable(inverse_vertex_map[BB_written], inverse_vertex_map[B->number]))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---variable is not constant(8): " +
                                        TM->GetTreeNode(var_written.first)->ToString());
                     foundNonConstant(var_written.first);
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined for write BB" + STR(B->number));
   }

   /// list all constant variables found
   CustomOrderedSet<unsigned> ConstantVars;
   for(auto vars : var_writing_BB_relation)
   {
      if(nonConstantVars.find(vars.first) == nonConstantVars.end())
      {
         auto key = TM->GetTreeNode(vars.first);
         auto initIt = inits.find(key);
         THROW_ASSERT(initIt != inits.end(), "unexpected condition");
         THROW_ASSERT(var_writing_size_relation.find(vars.first) != var_writing_size_relation.end(),
                      "unexpected condition");
         if(initIt->second.size() == var_writing_size_relation.find(vars.first)->second)
         {
            auto vd_node = TM->GetTreeNode(vars.first);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Constant variable: " + vd_node->ToString());
            ConstantVars.insert(vars.first);
            GetPointerS<var_decl>(vd_node)->readonly_flag = true;
         }
         else
         {
            inits.erase(initIt);
         }
      }
   }
   if(ConstantVars.empty())
   {
      return false;
   }

   for(const auto& Bit : sl->list_of_bloc)
   {
      auto B = Bit.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining for write BB" + STR(B->number));
      const auto& list_of_stmt = B->CGetStmtList();
      auto it_los_end = list_of_stmt.end();
      auto it_los = list_of_stmt.begin();
      while(it_los != it_los_end)
      {
         auto& inst = *it_los;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + inst->ToString());
         auto gn = GetPointerS<gimple_node>(inst);
         auto stmt_kind = inst->get_kind();
         if(stmt_kind == gimple_assign_K && gn->vdef)
         {
            auto ga = GetPointerS<gimple_assign>(inst);
            auto op0 = ga->op0;
            if(op0->get_kind() == mem_ref_K)
            {
               unsigned vd_index = 0;
               tree_nodeRef vd_node;
               tree_nodeRef dummy_var;
               auto* me = GetPointerS<mem_ref>(op0);
               auto resolved = extract_var_decl(me, vd_index, vd_node, dummy_var);
               if(resolved && ConstantVars.find(vd_index) != ConstantVars.end())
               {
                  if(ga->memdef)
                  {
                     const auto gimple_nop_id = TM->new_tree_node_id();
                     std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
                     gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
                     gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
                     TM->create_tree_node(gimple_nop_id, gimple_nop_K, gimple_nop_schema);
                     GetPointerS<ssa_name>(ga->memdef)->SetDefStmt(TM->GetTreeNode(gimple_nop_id));
                  }
                  if(ga->vdef)
                  {
                     const auto gimple_nop_id = TM->new_tree_node_id();
                     std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
                     gimple_nop_schema[TOK(TOK_SRCP)] = BUILTIN_SRCP;
                     gimple_nop_schema[TOK(TOK_SCPE)] = STR(function_id);
                     TM->create_tree_node(gimple_nop_id, gimple_nop_K, gimple_nop_schema);
                     GetPointerS<ssa_name>(ga->vdef)->SetDefStmt(TM->GetTreeNode(gimple_nop_id));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + STR(*it_los));
                  B->RemoveStmt(*it_los, AppM);
                  it_los = list_of_stmt.begin();
                  it_los_end = list_of_stmt.end();
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement");
                  continue;
               }
            }
         }
         ++it_los;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined for write BB" + STR(B->number));
   }
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   const auto integer_type = tree_man->GetSignedIntegerType();
   for(const auto& init : inits)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Rebuilding init of " + STR(init.first));
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> constructor_tree_node_schema;
      const auto array_type = tree_helper::CGetType(init.first);
      constructor_tree_node_schema[TOK(TOK_TYPE)] = STR(array_type->index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Type is " + STR(array_type));
      const auto element_type = tree_helper::CGetElements(array_type);
      auto constructor_index = TM->new_tree_node_id();
      TM->create_tree_node(constructor_index, constructor_K, constructor_tree_node_schema);
      auto* constr = GetPointerS<constructor>(TM->GetTreeNode(constructor_index));
      const auto last_index = init.second.rbegin()->first;
      integer_cst_t index = 0;
      for(index = 0; index <= last_index; index++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(index));
         if(init.second.count(index))
         {
            constr->add_idx_valu(TM->CreateUniqueIntegerCst(index, integer_type), init.second.at(index));
         }
         else
         {
            THROW_ASSERT(element_type->get_kind() == integer_type_K, "Type not supported " + STR(element_type));
            const auto default_value = TM->CreateUniqueIntegerCst(0, element_type);
            constr->add_idx_valu(TM->CreateUniqueIntegerCst(index, integer_type), default_value);
         }
      }
      GetPointerS<var_decl>(init.first)->init = TM->GetTreeNode(constructor_index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Rebuilt init of " + STR(init.first));
   }

   return true;
}

DesignFlowStep_Status rebuild_initialization2::InternalExec()
{
   bool modified = look_for_ROMs();
   if(modified)
   {
      function_behavior->UpdateBBVersion();
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
