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
 * @file remove_clobber_ga.cpp
 * @brief Analysis step that removes clobber gimple_assign introduced by GCC v4.7 and greater.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// header include
#include "remove_clobber_ga.hpp"

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

#include "hls_device.hpp"
#include "hls_manager.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

remove_clobber_ga::remove_clobber_ga(const application_managerRef _AppM, unsigned int _function_id,
                                     const DesignFlowManagerConstRef _design_flow_manager,
                                     const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, REMOVE_CLOBBER_GA, _design_flow_manager, _parameters)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

remove_clobber_ga::~remove_clobber_ga() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
remove_clobber_ga::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
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

DesignFlowStep_Status remove_clobber_ga::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   std::map<unsigned int, tree_nodeRef> var_substitution_table;
   std::map<unsigned int, CustomOrderedSet<tree_nodeRef>> stmt_to_be_removed;

   tree_nodeRef temp = TM->GetTreeNode(function_id);
   auto* fd = GetPointer<function_decl>(temp);
   auto* sl = GetPointer<statement_list>(fd->body);
   const bool is_single_write_memory =
       GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->IsSingleWriteMemory();

   for(const auto& block : sl->list_of_bloc)
   {
      const auto curr_bb = block.first;
      if(curr_bb == bloc::ENTRY_BLOCK_ID)
      {
         continue;
      }
      if(curr_bb == bloc::EXIT_BLOCK_ID)
      {
         continue;
      }
      for(const auto& stmt : block.second->CGetStmtList())
      {
         /// skip all non-clobber gimple_assign
         tree_nodeRef tn = stmt;
         auto* ga = GetPointer<gimple_assign>(tn);
         if(!ga || !ga->clobber)
         {
            continue;
         }
         if(is_single_write_memory)
         {
            var_substitution_table[ga->memdef->index] = ga->memuse;
         }
         stmt_to_be_removed[curr_bb].insert(stmt);
      }
   }

   if(is_single_write_memory)
   {
      /// perform the substitution
      for(const auto& block : sl->list_of_bloc)
      {
         const auto curr_bb = block.first;
         if(curr_bb == bloc::ENTRY_BLOCK_ID)
         {
            continue;
         }
         if(curr_bb == bloc::EXIT_BLOCK_ID)
         {
            continue;
         }
         for(const auto& phi : block.second->CGetPhiList())
         {
            auto* gp = GetPointer<gimple_phi>(phi);
            if(gp->virtual_flag)
            {
               for(const auto& def_edge : gp->CGetDefEdgesList())
               {
                  if(var_substitution_table.find(def_edge.first->index) != var_substitution_table.end())
                  {
                     tree_nodeRef res = var_substitution_table.find(def_edge.first->index)->second;
                     while(var_substitution_table.find(res->index) != var_substitution_table.end())
                     {
                        res = var_substitution_table.find(res->index)->second;
                     }
                     THROW_ASSERT(!(GetPointer<ssa_name>(res) &&
                                    GetPointer<gimple_assign>(GetPointer<ssa_name>(res)->CGetDefStmt()) &&
                                    GetPointer<gimple_assign>(GetPointer<ssa_name>(res)->CGetDefStmt())->clobber),
                                  "unexpected condition");
                     gp->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(res, def_edge.second));
                  }
               }
            }
         }
         for(const auto& stmt : block.second->CGetStmtList())
         {
            /// consider only gimple statements using virtual operands
            tree_nodeRef tn = stmt;
            auto* gn = GetPointer<gimple_node>(tn);
            THROW_ASSERT(gn, "unexpected condition");
            if(!gn->memuse)
            {
               continue;
            }
            if(var_substitution_table.find(gn->memuse->index) != var_substitution_table.end())
            {
               gn->memuse = var_substitution_table.at(gn->memuse->index);
            }
         }
      }
   }

   /// now remove the clobber gimple_assign
   const auto stbr_it_end = stmt_to_be_removed.end();
   for(auto stbr_it = stmt_to_be_removed.begin(); stbr_it != stbr_it_end; ++stbr_it)
   {
      unsigned int curr_bb = stbr_it->first;
      for(const auto& to_be_removed : stbr_it->second)
      {
         sl->list_of_bloc[curr_bb]->RemoveStmt(to_be_removed, AppM);
      }
   }

   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}
