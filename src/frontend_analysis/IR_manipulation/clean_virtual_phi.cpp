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
 * @file clean_virtual_phi.hpp
 * @brief Manipulation step which removes virtual phi introduced by GCC/CLANG
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "clean_virtual_phi.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// tree includes
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> CleanVirtualPhi::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

CleanVirtualPhi::CleanVirtualPhi(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, CLEAN_VIRTUAL_PHI, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CleanVirtualPhi::~CleanVirtualPhi() = default;

DesignFlowStep_Status CleanVirtualPhi::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   tree_nodeRef temp = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(temp);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   /// Removing all the virtual phi
   /// The virtual phis refer to memory definitions and uses instead of virtual definitions and uses

   for(auto block : sl->list_of_bloc)
   {
      auto& phi_list = block.second->CGetPhiList();
      for(auto phi = phi_list.begin(); phi != phi_list.end(); phi++)
      {
         const auto gp = GetPointer<const gimple_phi>(GET_NODE(*phi));
         if(gp->virtual_flag)
         {
            auto phi_to_be_removed = *phi;
            /// Moving iterator to avoid its invalidation
            auto tmp_it = phi;
            ++tmp_it;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + phi_to_be_removed->ToString());
            block.second->RemovePhi(phi_to_be_removed);
            --tmp_it;
            phi = tmp_it;
         }
      }
      for(auto& stmt : block.second->CGetStmtList())
      {
         auto gn = GetPointer<gimple_node>(GET_NODE(stmt));
         gn->memdef = tree_nodeRef();
         gn->memuse = tree_nodeRef();
      }
   }

   function_behavior->UpdateBBVersion();

   return DesignFlowStep_Status::SUCCESS;
}

void CleanVirtualPhi::Initialize()
{
   FunctionFrontendFlowStep::Initialize();
}

bool CleanVirtualPhi::HasToBeExecuted() const
{
   return bb_version == 0;
}
