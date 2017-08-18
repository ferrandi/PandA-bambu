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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 * @brief rebuild initializations where it is possible
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/

///Header include
#include "rebuild_initializations.hpp"

///Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"


///Parameter include
#include "Parameter.hpp"

///parser/treegcc include
#include "token_interface.hpp"

///STD include
#include <fstream>

///Tree include
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_reindex.hpp"
#include "tree_helper.hpp"


rebuild_initializations::rebuild_initializations(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager) :
   FunctionFrontendFlowStep(_AppM, _function_id, REBUILD_INITIALIZATION, _design_flow_manager, Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship> > rebuild_initializations::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship> > relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP) :
      case(INVALIDATION_RELATIONSHIP) :
      case(PRECEDENCE_RELATIONSHIP) :
         {
            break;
         }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}


rebuild_initializations::~rebuild_initializations()
{

}

DesignFlowStep_Status rebuild_initializations::InternalExec()
{
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      PrintTreeManager(true);
   }
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   tree_managerRef TM = AppM->get_tree_manager();
   tree_manipulationRef tree_man(new tree_manipulation(TM, parameters));
   tree_nodeRef tn = TM->get_tree_node_const(function_id);
   function_decl * fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   statement_list * sl = GetPointer<statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");
   std::map<unsigned int, blocRef>::iterator B_it_end = sl->list_of_bloc.end();

   TreeNodeMap<std::map<long long int, tree_nodeRef> > inits;

   /// for each basic block B in CFG do > Consider all blocks successively
   for(std::map<unsigned int, blocRef>::iterator B_it = sl->list_of_bloc.begin(); B_it != B_it_end ; ++B_it)
   {
      blocRef B = B_it->second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(B->number));
      const auto & list_of_stmt = B->CGetStmtList();
      auto it_los_end = list_of_stmt.end();
      auto it_los = list_of_stmt.begin();
      while(it_los != it_los_end)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(*it_los)->ToString());
         if(GET_NODE(*it_los)->get_kind() == gimple_assign_K)
         {
            gimple_assign * ga =  GetPointer<gimple_assign>(GET_NODE(*it_los));
            enum kind code0 = GET_NODE(ga->op0)->get_kind();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Left part of assignment " +  GET_NODE(ga->op0)->get_kind_text() + (code0  == array_ref_K ? " - Type is " +  tree_helper::CGetType(GET_NODE(ga->op0))->get_kind_text() : ""));

            ///NOTE: the check has to be performed on the type of the elements of the array and not on the constant in the right part to avoid rebuilding of array of pointers
            if(code0  == array_ref_K and tree_helper::CGetType(GET_NODE(ga->op0))->get_kind() == integer_type_K)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "check for initializations such as var[const_index] = const_value; " + STR(GET_INDEX_NODE(ga->op0)));
               array_ref * ar = GetPointer<array_ref>(GET_NODE(ga->op0));
               if(GET_NODE(ar->op0)->get_kind() == var_decl_K && GET_NODE(ar->op1)->get_kind() == integer_cst_K)
               {
                  var_decl * vd = GetPointer<var_decl>(GET_NODE(ar->op0));
                  if(vd->readonly_flag)
                  {
                     THROW_ASSERT(not vd->init, "Writing element of read only array already initialized: " + STR(ga->op0));
                     inits[ar->op0][tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_NODE(ar->op1)))] = ga->op1;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Statement removed " + GET_NODE(*it_los)->ToString());
                     if(ga->memdef)
                     {
                        const auto gimple_nop_id = TM->new_tree_node_id();
                        std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
                        gimple_nop_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
                        TM->create_tree_node(gimple_nop_id, gimple_nop_K, gimple_nop_schema);
                        GetPointer<ssa_name>(GET_NODE(ga->memdef))->SetDefStmt(TM->GetTreeReindex(gimple_nop_id));
                     }
                     if(ga->vdef)
                     {
                        const auto gimple_nop_id = TM->new_tree_node_id();
                        std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
                        gimple_nop_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
                        TM->create_tree_node(gimple_nop_id, gimple_nop_K, gimple_nop_schema);
                        GetPointer<ssa_name>(GET_NODE(ga->vdef))->SetDefStmt(TM->GetTreeReindex(gimple_nop_id));
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + STR(*it_los));
                     B->RemoveStmt(*it_los);
                     it_los = list_of_stmt.begin();
                     it_los_end = list_of_stmt.end();
                     continue;
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(*it_los)->ToString());
         ++it_los;
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examining BB" + STR(B->number));
   }
   const auto integer_type = tree_man->create_default_integer_type();
   for(const auto init : inits)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Rebuilding init of " + STR(init.first));
      std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> constructor_tree_node_schema;
      const auto array_type = behavioral_helper->get_type(init.first->index);
      constructor_tree_node_schema[TOK(TOK_TYPE)] = STR(array_type);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Type is " + STR(TM->CGetTreeNode(array_type)));
      const auto element_type = TM->GetTreeReindex(tree_helper::GetElements(TM, array_type));
      unsigned int constructor_index = TM->new_tree_node_id();
      TM->create_tree_node(constructor_index, constructor_K, constructor_tree_node_schema);
      constructor * constr = GetPointer<constructor>(TM->get_tree_node_const(constructor_index));
      const long long int last_index = init.second.rbegin()->first;
      long long int index = 0;
      for(index = 0; index <= last_index; index++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(index));
         if(init.second.find(index) != init.second.end())
         {
            constr->add_idx_valu(tree_man->CreateIntegerCst(integer_type, index, TM->new_tree_node_id()), init.second.find(index)->second);
         }
         else
         {
            THROW_ASSERT(GET_NODE(element_type)->get_kind() == integer_type_K, "Type not supported " + STR(element_type));
            const auto default_value = tree_man->CreateIntegerCst(element_type, 0, TM->new_tree_node_id());
            constr->add_idx_valu(tree_man->CreateIntegerCst(integer_type, index, TM->new_tree_node_id()), default_value);
         }
      }
      GetPointer<var_decl>(GET_NODE(init.first))->init = TM->GetTreeReindex(constructor_index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Rebuilt init of " + STR(init.first));
   }
   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      PrintTreeManager(false);
   }
   return DesignFlowStep_Status::SUCCESS;
}

