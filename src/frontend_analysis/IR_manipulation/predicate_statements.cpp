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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file predicate_statements.cpp
 * @brief This class contains the methods for setting predicates for instructions which cannot be speculated
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "predicate_statements.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

PredicateStatements::PredicateStatements(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, FrontendFlowStepType::PREDICATE_STATEMENTS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

PredicateStatements::~PredicateStatements() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>> PredicateStatements::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_FROM_PRAGMA_BUILT
         relationships.insert(std::make_pair(PRAGMA_ANALYSIS, WHOLE_APPLICATION));
#endif
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(MEM_CG_EXT, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool PredicateStatements::HasToBeExecuted() const
{
   return bb_version == 0;
}

DesignFlowStep_Status PredicateStatements::InternalExec()
{
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   const auto TM = AppM->get_tree_manager();
   const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   const auto true_value_id = TM->new_tree_node_id();
   const auto boolean_type = tree_man->create_boolean_type();
   const auto true_value = tree_man->CreateIntegerCst(boolean_type, 1, true_value_id);

   bool bb_modified = false;
   const auto fd = GetPointer<const function_decl>(TM->CGetTreeNode(function_id));
   const auto sl = GetPointer<const statement_list>(GET_NODE(fd->body));
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
         auto ga = GetPointer<gimple_assign>(GET_NODE(stmt));
         if(behavioral_helper->CanBeSpeculated(stmt->index) or not ga or (GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K))
         {
            continue;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Predicating " + STR(stmt));
            THROW_ASSERT(!ga->predicate, "unexpected condition");
            ga->predicate = true_value;
         }
      }
   }

   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
