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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file initialize_hls.cpp
 * @brief Step which initializes HLS data structure
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "initialize_hls.hpp"

#include "Parameter.hpp"
#include "application_frontend_flow_step.hpp"
#include "call_graph_manager.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "frontend_flow_step_factory.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "memory_allocation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

InitializeHLS::InitializeHLS(const ParameterConstRef _parameters, const HLS_managerRef _HLS_mgr,
                             unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLS_mgr, _function_id, _design_flow_manager, HLSFlowStep_Type::INITIALIZE_HLS)
{
}

void InitializeHLS::ComputeRelationships(DesignFlowStepSet& relationship,
                                         const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
         const auto frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(
             design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND));
         const auto frontend_flow_signature = ApplicationFrontendFlowStep::ComputeSignature(BAMBU_FRONTEND_FLOW);
         const auto frontend_flow_step = design_flow_manager.GetDesignFlowStep(frontend_flow_signature);
         const auto design_flow_step =
             frontend_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(frontend_flow_step)->design_flow_step :
                 frontend_flow_step_factory->CreateApplicationFrontendFlowStep(BAMBU_FRONTEND_FLOW);
         relationship.insert(design_flow_step);

         const auto technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
             design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   HLSFunctionStep::ComputeRelationships(relationship, relationship_type);
}

void InitializeHLS::Initialize()
{
   /// NOTE: this overrides HLSFunctionStep::Initialize which cannot be invoked since HLS has not yet been set
}

DesignFlowStep_Status InitializeHLS::InternalExec()
{
   HLS = HLS_manager::create_HLS(HLSMgr, funId);
   HLS->module_binding_algorithm = parameters->getOption<HLSFlowStep_Type>(OPT_fu_binding_algorithm);
   HLS->liveVariableAlgorithm = parameters->getOption<HLSFlowStep_Type>(OPT_liveVariableAlgorithm);
   HLS->chaining_algorithm = parameters->getOption<HLSFlowStep_Type>(OPT_chaining_algorithm);

   return DesignFlowStep_Status::SUCCESS;
}
