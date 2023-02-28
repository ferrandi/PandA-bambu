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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
#include "hls_manager.hpp"
#include "hls_target.hpp"
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
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto* frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));
         const std::string frontend_flow_signature = ApplicationFrontendFlowStep::ComputeSignature(BAMBU_FRONTEND_FLOW);
         const vertex frontend_flow_step = design_flow_manager.lock()->GetDesignFlowStep(frontend_flow_signature);
         const DesignFlowStepRef design_flow_step =
             frontend_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(frontend_flow_step)->design_flow_step :
                                  frontend_flow_step_factory->CreateApplicationFrontendFlowStep(BAMBU_FRONTEND_FLOW);
         relationship.insert(design_flow_step);

         const auto* technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const std::string technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const DesignFlowStepRef technology_design_flow_step =
             technology_flow_step ?
                 design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_EXPERIMENTAL && HAVE_PRAGMA_BUILT
         if(parameters->isOption(OPT_parse_pragma) and parameters->getOption<bool>(OPT_parse_pragma) and
            relationship_type == PRECEDENCE_RELATIONSHIP)
         {
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const ActorGraphFlowStepFactory* actor_graph_flow_step_factory =
                GetPointer<const ActorGraphFlowStepFactory>(
                    design_flow_manager.lock()->CGetDesignFlowStepFactory("ActorGraph"));
            const std::string actor_graph_creator_signature =
                ActorGraphFlowStep::ComputeSignature(ACTOR_GRAPHS_CREATOR, input_function, 0, "");
            const vertex actor_graph_creator_step =
                design_flow_manager.lock()->GetDesignFlowStep(actor_graph_creator_signature);
            const DesignFlowStepRef design_flow_step =
                actor_graph_creator_step ?
                    design_flow_graph->CGetDesignFlowStepInfo(actor_graph_creator_step)->design_flow_step :
                    actor_graph_flow_step_factory->CreateActorGraphStep(ACTOR_GRAPHS_CREATOR, input_function);
            relationship.insert(design_flow_step);
         }
#endif
         break;
      }
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
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT
   if(GetPointer<const function_decl>(HLSMgr->get_tree_manager()->CGetTreeNode(funId))->omp_for_wrapper)
   {
      HLS->controller_type = HLSFlowStep_Type::PARALLEL_CONTROLLER_CREATOR;
      HLS->module_binding_algorithm =
          static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_fu_binding_algorithm));
      HLS->liveness_algorithm = HLSFlowStep_Type::CHAINING_BASED_LIVENESS;
      HLS->chaining_algorithm = HLSFlowStep_Type::EPDG_SCHED_CHAINING;
   }
   else
#endif
   {
      HLS->controller_type = static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_controller_architecture));
      HLS->module_binding_algorithm =
          static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_fu_binding_algorithm));
      HLS->liveness_algorithm = static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_liveness_algorithm));
      HLS->chaining_algorithm = static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_chaining_algorithm));
   }

   return DesignFlowStep_Status::SUCCESS;
}
