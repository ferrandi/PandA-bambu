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
 * @file sdc_code_motion.cpp
 * @brief Analysis step performing code motion speculation on the basis of sdc results.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "sdc_code_motion.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "basic_block.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// design_flow includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_flow include
#include "frontend_flow_step_factory.hpp"

/// frontend_flow/IR_analysis include
#include "simple_code_motion.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"

/// HLS/scheduling include
#include "sdc_scheduling.hpp"

/// tree includes
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

SDCCodeMotion::SDCCodeMotion(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, SDC_CODE_MOTION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

SDCCodeMotion::~SDCCodeMotion() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> SDCCodeMotion::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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

bool SDCCodeMotion::HasToBeExecuted() const
{
   return parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
          GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch && FunctionFrontendFlowStep::HasToBeExecuted();
}

void SDCCodeMotion::Initialize()
{
}

DesignFlowStep_Status SDCCodeMotion::InternalExec()
{
   const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();

   const tree_managerRef TM = AppM->get_tree_manager();
   auto* fd = GetPointer<function_decl>(TM->get_tree_node_const(function_id));
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   std::map<unsigned int, blocRef>& list_of_bloc = sl->list_of_bloc;

   /// Retrieve result of sdc scheduling
   const auto sdc_scheduling_step = design_flow_manager.lock()->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(HLSFlowStep_Type::SDC_SCHEDULING, HLSFlowStepSpecializationConstRef(), function_id));
   THROW_ASSERT(sdc_scheduling_step, "SDC scheduling hls step not found");
   const auto sdc_scheduling = GetPointer<const SDCScheduling>(design_flow_graph->CGetDesignFlowStepInfo(sdc_scheduling_step)->design_flow_step);
   const auto& movements_list = sdc_scheduling->movements_list;
   for(const auto& movement : movements_list)
   {
      const auto statement_index = movement[0];
      const auto old_basic_block = movement[1];
      const auto new_basic_block = movement[2];
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Moving " + STR(TM->GetTreeReindex(statement_index)) + " from BB" + STR(old_basic_block) + " to BB" + STR(new_basic_block));
#ifndef NDEBUG
      if(not AppM->ApplyNewTransformation())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because reached limit of cfg transformations");
         continue;
      }
#endif
      list_of_bloc[old_basic_block]->RemoveStmt(TM->GetTreeReindex(statement_index));
      list_of_bloc[new_basic_block]->PushBack(TM->GetTreeReindex(statement_index));
#ifndef NDEBUG
      AppM->RegisterTransformation(GetName(), TM->CGetTreeNode(statement_index));
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Moved " + STR(statement_index));
   }
   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}

void SDCCodeMotion::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}
