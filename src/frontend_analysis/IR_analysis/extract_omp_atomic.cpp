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
 *              Copyright (c) 2015-2024 Politecnico di Milano
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
 * @file extract_omp_atomic.cpp
 * @brief Analysis step extracting openmp atomic
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "extract_omp_atomic.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "function_behavior.hpp"

/// parser/compiler include
#include "token_interface.hpp"

/// STL include
#include "custom_set.hpp"
#include <utility>

/// tree includes
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "utility.hpp"

ExtractOmpAtomic::ExtractOmpAtomic(const application_managerRef _AppM, unsigned int _function_id,
                                   const DesignFlowManagerConstRef _design_flow_manager,
                                   const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, EXTRACT_OMP_ATOMIC, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

ExtractOmpAtomic::~ExtractOmpAtomic() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
ExtractOmpAtomic::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   const auto TM = AppM->get_tree_manager();
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
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

DesignFlowStep_Status ExtractOmpAtomic::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   if(debug_level >= DEBUG_LEVEL_PEDANTIC &&
      (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
   {
      WriteBBGraphDot("BB_Before_" + GetName() + ".dot");
      PrintTreeManager(true);
   }
   bool changed = false;
   const auto basic_block_graph = function_behavior->CGetBBGraph(FunctionBehavior::BB);
   VertexIterator basic_block, basic_block_end;
   for(boost::tie(basic_block, basic_block_end) = boost::vertices(*basic_block_graph); basic_block != basic_block_end;
       basic_block++)
   {
      const auto block = basic_block_graph->CGetBBNodeInfo(*basic_block)->block;
      tree_nodeRef gimple_to_be_removed;
      for(const auto& stmt : block->CGetStmtList())
      {
         const auto* pn = GetPointer<gimple_pragma>(stmt);
         if(pn && pn->scope && GetPointer<omp_pragma>(pn->scope))
         {
            const auto oa = GetPointer<omp_atomic_pragma>(pn->directive);
            if(oa)
            {
               if(block->list_of_pred.size() == 1 && block->list_of_pred.front() == BB_ENTRY &&
                  stmt == block->CGetStmtList().front())
               {
                  gimple_to_be_removed = stmt;
                  GetPointer<function_decl>(AppM->get_tree_manager()->GetTreeNode(function_id))->omp_atomic = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found Atomic Omp function");
                  changed = true;
               }
               else
               {
                  THROW_ERROR("Omp atomic not supported");
               }
            }
         }
      }
      if(gimple_to_be_removed)
      {
         block->RemoveStmt(gimple_to_be_removed, AppM);
      }
   }
   if(debug_level >= DEBUG_LEVEL_PEDANTIC &&
      (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
   {
      WriteBBGraphDot("BB_After_" + GetName() + ".dot");
      PrintTreeManager(false);
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
