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
#include "memory_allocation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

InitializeHLS::InitializeHLS(const ParameterConstRef _parameters, const HLS_managerRef _HLS_mgr,
                             unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLS_mgr, _function_id, _design_flow_manager, HLSFlowStep_Type::INITIALIZE_HLS)
{
}

void InitializeHLS::ComputeRelationships(DesignFlowStepSet& relationship,
                                         const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND));
         const auto frontend_flow_signature = ApplicationFrontendFlowStep::ComputeSignature(BAMBU_FRONTEND_FLOW);
         const auto frontend_flow_step = design_flow_manager.lock()->GetDesignFlowStep(frontend_flow_signature);
         const auto design_flow_step =
             frontend_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(frontend_flow_step)->design_flow_step :
                 frontend_flow_step_factory->CreateApplicationFrontendFlowStep(BAMBU_FRONTEND_FLOW);
         relationship.insert(design_flow_step);

         const auto technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
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
      case PRECEDENCE_RELATIONSHIP:
      case INVALIDATION_RELATIONSHIP:
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
   HLS->controller_type = parameters->getOption<HLSFlowStep_Type>(OPT_controller_architecture);
   if(HLSMgr->GetFunctionBehavior(funId)->is_simple_pipeline())
   {
      HLS->controller_type = HLSFlowStep_Type::PIPELINE_CONTROLLER_CREATOR;
   }
   HLS->module_binding_algorithm = parameters->getOption<HLSFlowStep_Type>(OPT_fu_binding_algorithm);
   HLS->liveness_algorithm = parameters->getOption<HLSFlowStep_Type>(OPT_liveness_algorithm);
   HLS->chaining_algorithm = parameters->getOption<HLSFlowStep_Type>(OPT_chaining_algorithm);

   return DesignFlowStep_Status::SUCCESS;
}
