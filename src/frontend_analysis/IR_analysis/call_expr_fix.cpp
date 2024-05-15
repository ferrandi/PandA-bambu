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
 * @file call_expr_fix.cpp
 * @brief Analysis step which fix a non-void list of parameters to function with void as input parameter type
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// header include
#include "call_expr_fix.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// STD include
#include <fstream>

/// tree includes
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

call_expr_fix::call_expr_fix(const application_managerRef _AppM, unsigned int _function_id,
                             const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, CALL_EXPR_FIX, _design_flow_manager, _parameters)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

call_expr_fix::~call_expr_fix() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
call_expr_fix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
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

DesignFlowStep_Status call_expr_fix::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   tree_nodeRef temp = TM->GetTreeNode(function_id);
   auto* fdcur = GetPointer<function_decl>(temp);
   auto* sl = GetPointer<statement_list>(fdcur->body);
   bool bb_modified = false;

   /// Checking if there are gimple_call or call_expr for which the fix apply
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->analyzing BB" + std::to_string(block.first));
      for(const auto& statement : block.second->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing node " + statement->ToString());
         if(statement->get_kind() == gimple_call_K)
         {
            auto* ce = GetPointer<gimple_call>(statement);
            std::vector<tree_nodeRef>& args = ce->args;
            auto* ae = GetPointer<addr_expr>(ce->fn);
            if(ae && args.size())
            {
               auto* fd = GetPointer<function_decl>(ae->op);
               if(!fd->undefined_flag)
               {
                  auto* fun_type = GetPointer<function_type>(fd->type);
                  bool is_var_args_p = fun_type->varargs_flag;
                  if(!is_var_args_p)
                  {
                     /// check if fun_type there is only one parameter and it is equal to void
                     tree_nodeRef paramList = fun_type->prms;
                     unsigned int count_param = 0;
                     while(paramList)
                     {
                        tree_nodeRef elem = paramList;
                        auto* node = GetPointer<tree_list>(elem);
                        paramList = node->chan;
                        if(node->valu->get_kind() != void_type_K)
                        {
                           count_param++;
                        }
                     }
                     if(fd->list_of_args.size() == 0 && count_param == 0)
                     {
                        ce->args.clear();
                        bb_modified = true;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actuals cleared");
                     }
                  }
               }
               else if(tree_helper::print_function_name(TM, fd) == "__builtin_dwarf_cfa")
               {
                  ce->args.clear();
                  bb_modified = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actuals cleared");
               }
            }
         }
         if(statement->get_kind() == gimple_assign_K)
         {
            auto* ga = GetPointer<gimple_assign>(statement);
            if(ga->op1->get_kind() == call_expr_K || ga->op1->get_kind() == aggr_init_expr_K)
            {
               auto* ce = GetPointer<call_expr>(ga->op1);
               std::vector<tree_nodeRef>& args = ce->args;
               auto* ae = GetPointer<addr_expr>(ce->fn);
               if(ae && args.size())
               {
                  auto* fd = GetPointer<function_decl>(ae->op);
                  if(!fd->undefined_flag)
                  {
                     auto* fun_type = GetPointer<function_type>(fd->type);
                     bool is_var_args_p = fun_type->varargs_flag;
                     if(!is_var_args_p)
                     {
                        /// check if fun_type there is only one parameter and it is equal to void
                        tree_nodeRef paramList = fun_type->prms;
                        unsigned int count_param = 0;
                        while(paramList)
                        {
                           auto* node = GetPointer<tree_list>(paramList);
                           paramList = node->chan;
                           if(node->valu->get_kind() != void_type_K)
                           {
                              count_param++;
                           }
                        }
                        if(fd->list_of_args.size() == 0 && count_param == 0)
                        {
                           ce->args.clear();
                           bb_modified = true;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actuals cleared");
                        }
                     }
                  }
                  else if(tree_helper::print_function_name(TM, fd) == "__builtin_dwarf_cfa")
                  {
                     ce->args.clear();
                     bb_modified = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actuals cleared");
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
