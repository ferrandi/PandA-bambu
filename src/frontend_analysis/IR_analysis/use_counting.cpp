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
 * @file use_counting.cpp
 * @brief Analysis step counting how many times a ssa_name is used
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "use_counting.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include <list>
#include <utility>

use_counting::use_counting(const ParameterConstRef _parameters, const application_managerRef _AppM,
                           unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, USE_COUNTING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

use_counting::~use_counting() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
use_counting::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION2, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         relationships.insert(std::make_pair(HWCALL_INJECTION, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
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

DesignFlowStep_Status use_counting::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   const auto fd = GetPointerS<const function_decl>(TM->GetTreeNode(function_id));
   const auto sl = GetPointerS<const statement_list>(fd->body);
   const auto th_debug = tree_helper::debug_level;
   tree_helper::debug_level = debug_level;
   for(const auto& [idx, bb] : sl->list_of_bloc)
   {
      for(const auto& statement_node : bb->CGetStmtList())
      {
         const auto ssa_uses = tree_helper::ComputeSsaUses(statement_node);
         for(const auto& [ssa, use_count] : ssa_uses)
         {
            const auto sn = GetPointerS<ssa_name>(ssa);
            for(auto uses = use_count; uses; --uses)
            {
               sn->AddUseStmt(statement_node);
            }
         }
      }
      for(const auto& phi_node : bb->CGetPhiList())
      {
         const auto ssa_uses = tree_helper::ComputeSsaUses(phi_node);
         for(const auto& [ssa, use_count] : ssa_uses)
         {
            const auto sn = GetPointerS<ssa_name>(ssa);
            for(auto uses = use_count; uses; --uses)
            {
               sn->AddUseStmt(phi_node);
            }
         }
         GetPointerS<gimple_phi>(phi_node)->SetSSAUsesComputed();
      }
      bb->SetSSAUsesComputed();
   }

   tree_helper::debug_level = th_debug;
   return DesignFlowStep_Status::SUCCESS;
}
