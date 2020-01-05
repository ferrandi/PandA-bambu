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
 * @file block_fix.cpp
 * @brief Analysis step which modifies the control flow graph of the tree to make it more compliant and simple
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// header include
#include "block_fix.hpp"

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
#include "tree_reindex.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

BlockFix::BlockFix(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, BLOCK_FIX, _design_flow_manager, _parameters)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

BlockFix::~BlockFix() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>> BlockFix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(CALL_EXPR_FIX, SAME_FUNCTION));
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

DesignFlowStep_Status BlockFix::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   tree_nodeRef temp = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(temp);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));

   std::map<unsigned int, blocRef>& list_of_bloc = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator it3, it3_end = list_of_bloc.end();

   // Adding entry block
   blocRef entry_bloc = blocRef(new bloc(BB_ENTRY));
   // Set of successor of entry
   std::vector<unsigned int>& succ_entry = entry_bloc->list_of_succ;
   for(it3 = list_of_bloc.begin(); it3 != it3_end; ++it3)
   {
      std::vector<unsigned int>::iterator it2, it2_end;
      if(it3->second)
      {
         it2_end = it3->second->list_of_pred.end();
         for(it2 = it3->second->list_of_pred.begin(); it2 != it2_end; ++it2)
         {
            if(*it2 == BB_ENTRY)
               succ_entry.push_back(it3->second->number);
         }
      }
   }
   /// set of predecessor of exit is missing! TO BE CHECK
   // Adding exit
   blocRef exit_bloc = blocRef(new bloc(BB_EXIT));
   sl->list_of_bloc[BB_ENTRY] = entry_bloc;
   sl->list_of_bloc[BB_EXIT] = exit_bloc;

   /// Checking if there are gimple_labels which can be removed
   /// Computing reachable labels
   CustomSet<unsigned int> reachable_labels;
   for(auto block : sl->list_of_bloc)
   {
      for(auto statement : block.second->CGetStmtList())
      {
         const auto* gg = GetPointer<const gimple_goto>(GET_NODE(statement));
         if(gg)
         {
            THROW_ASSERT(gg->op and GetPointer<const label_decl>(GET_NODE(gg->op)), "Unexpexted condition :" + gg->ToString());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found a reachable label " + gg->op->ToString());
            reachable_labels.insert(GET_INDEX_NODE(gg->op));
         }
         const auto gs = GetPointer<const gimple_switch>(GET_NODE(statement));
         if(gs)
         {
            for(const auto& vec_op : GetPointer<const tree_vec>(GET_NODE(gs->op1))->list_of_op)
            {
               const auto cle = GetPointer<const case_label_expr>(GET_NODE(vec_op));
               if(cle->got and GetPointer<const label_decl>(GET_NODE(cle->got)))
               {
                  reachable_labels.insert(cle->got->index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found a reachable label " + cle->got->ToString());
               }
            }
         }
      }
   }
   std::list<std::pair<tree_nodeRef, unsigned int>> to_be_removed;
   for(auto block : sl->list_of_bloc)
   {
      for(auto statement : block.second->CGetStmtList())
      {
         const auto* gl = GetPointer<const gimple_label>(GET_NODE(statement));
         if(gl)
         {
            const auto* ld = GetPointer<const label_decl>(GET_NODE(gl->op));
            if(ld and reachable_labels.find(ld->index) == reachable_labels.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found a removable label " + statement->ToString());
               to_be_removed.push_back(std::pair<tree_nodeRef, unsigned int>(statement, block.first));
            }
         }
      }
   }

   for(auto removing : to_be_removed)
   {
      sl->list_of_bloc[removing.second]->RemoveStmt(removing.first);
   }

   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}
