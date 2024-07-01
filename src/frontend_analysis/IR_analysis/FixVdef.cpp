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
 *              Copyright (C) 2021-2024 Politecnico di Milano
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
 * @file FixVdef.cpp
 * @brief Simplifies memory dependency data structure by merging memdeps in virtual dependencies.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "FixVdef.hpp"

#include "Parameter.hpp"                    // for Parameter
#include "application_manager.hpp"          // for application_manager, app...
#include "dbgPrintHelper.hpp"               // for DEBUG_LEVEL_VERY_PEDANTIC
#include "design_flow_graph.hpp"            // for DesignFlowGraph, DesignF...
#include "design_flow_manager.hpp"          // for DesignFlowManager, Desig...
#include "design_flow_step_factory.hpp"     // for DesignFlowManagerConstRef
#include "exceptions.hpp"                   // for THROW_ASSERT, THROW_UNRE...
#include "hls_manager.hpp"                  // for HLS_manager
#include "string_manipulation.hpp"          // for STR, GET_CLASS
#include "technology_flow_step.hpp"         // for TechnologyFlowStep_Type
#include "technology_flow_step_factory.hpp" // for TechnologyFlowStepFactory
#include "tree_basic_block.hpp"             // for bloc
#include "tree_manager.hpp"                 // for tree_manager
#include "tree_node.hpp"                    // for gimple_assign

FixVdef::FixVdef(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id,
                 const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, FIX_VDEF, _design_flow_manager, Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
FixVdef::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(HWCALL_INJECTION, SAME_FUNCTION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
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
         THROW_UNREACHABLE("");
   }
   return relationships;
}

FixVdef::~FixVdef() = default;

void FixVdef::ComputeRelationships(DesignFlowStepSet& relationship,
                                   const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto technology_flow_step_factory = GetPointerS<const TechnologyFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

DesignFlowStep_Status FixVdef::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   const auto tn = TM->GetTreeNode(function_id);
   const auto fd = GetPointer<const function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto sl = GetPointer<const statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");
   THROW_ASSERT(GetPointer<const HLS_manager>(AppM), "unexpected condition");
   const auto isSingleMem = GetPointerS<const HLS_manager>(AppM)->IsSingleWriteMemory();
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(block.first));
      for(const auto& s : block.second->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing statement " + s->ToString());
         const auto gn = GetPointerS<gimple_node>(s);
         if(isSingleMem)
         {
            gn->vdef = gn->memdef;
            if(!gn->vuses.empty())
            {
               gn->vuses.clear();
            }
            if(gn->memuse)
            {
               gn->AddVuse(gn->memuse);
            }
            if(!gn->vovers.empty())
            {
               gn->vovers.clear();
            }
         }
         gn->memdef = nullptr;
         gn->memuse = nullptr;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(block.first));
   }
   return DesignFlowStep_Status::SUCCESS;
}
