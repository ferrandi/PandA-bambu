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
 * @file extract_gimple_cond_op.cpp
 * @brief Analysis step that extract condition from gimple_cond
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "extract_gimple_cond_op.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// tree includes
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

ExtractGimpleCondOp::ExtractGimpleCondOp(const application_managerRef _AppM,
                                         const DesignFlowManagerConstRef _design_flow_manager,
                                         const unsigned int _function_id, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, EXTRACT_GIMPLE_COND_OP, _design_flow_manager, _parameters),
      bb_modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

ExtractGimpleCondOp::~ExtractGimpleCondOp() = default;

void ExtractGimpleCondOp::Initialize()
{
   bb_modified = false;
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
ExtractGimpleCondOp::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
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

DesignFlowStep_Status ExtractGimpleCondOp::InternalExec()
{
   bb_modified = false;
   const auto TM = AppM->get_tree_manager();
   const auto tree_man = tree_manipulationConstRef(new tree_manipulation(TM, parameters, AppM));
   const auto fd = GetPointer<function_decl>(TM->GetTreeNode(function_id));
   const auto sl = GetPointer<statement_list>(fd->body);
   for(const auto& block : sl->list_of_bloc)
   {
      const auto& stmt_list = block.second->CGetStmtList();
      if(stmt_list.size())
      {
         const auto last_stmt = stmt_list.back();
         auto gc = GetPointer<gimple_cond>(last_stmt);
         if(gc && (!tree_helper::IsBooleanType(gc->op0) ||
                   (gc->op0->get_kind() != ssa_name_K && !GetPointer<cst_node>(gc->op0))))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---fixing gimple cond: " + last_stmt->ToString());
            auto new_gc_cond = tree_man->ExtractCondition(last_stmt, block.second, function_id);
            TM->ReplaceTreeNode(last_stmt, gc->op0, new_gc_cond);
            bb_modified = true;
         }
      }
   }
   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
