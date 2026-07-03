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
 * @file standard_hls.cpp
 * @brief Implementation of the methods to create the structural description of the component
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "standard_hls.hpp"

#include "Parameter.hpp"
#include "TopEntityMemoryMapped.hpp"
#include "add_library.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "cpu_time.hpp"
#include "custom_set.hpp"
#include "datapath_creator.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "module_interface.hpp"
#include "technology_manager.hpp"

#include <tuple>

standard_hls::standard_hls(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId,
                           const DesignFlowManager& _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::STANDARD_HLS_FLOW)
{
   composed = true;
}

HLS_step::HLSRelationships
standard_hls::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::VIRTUAL_DESIGN_FLOW, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_datapath_architecture),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::FSM_CONTROLLER_CREATOR, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         HLSFlowStep_Type top_entity_type;

         bool found = false;
         if(parameters->isOption(OPT_context_switch))
         {
            const auto is_context_handler = [&]() {
               const auto FB = HLSMgr->CGetFunctionBehavior(funId);
               const auto omp_info = FB->GetOMPInfo();
               return FB->IsOMPCore() && omp_info && omp_info->context_count > 1U;
            }();
            if(is_context_handler)
            {
               found = true;
               top_entity_type = HLSFlowStep_Type::TOP_ENTITY_OMP_CS_CREATION;
               ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(),
                                          HLSFlowStep_Relationship::SAME_FUNCTION));
            }
         }
         if(!found) // use standard
         {
            top_entity_type =
                HLSMgr->hasToBeInterfaced(funId) && (HLSMgr->CGetCallGraphManager().ExistsAddressedFunction() ||
                                                     parameters->getOption<bool>(OPT_memory_mapped_top)) ?
                    HLSFlowStep_Type::TOP_ENTITY_MEMORY_MAPPED_CREATION :
                    HLSFlowStep_Type::TOP_ENTITY_CREATION;
         }
         ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::ADD_LIBRARY,
                                    HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(false)),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->hasToBeInterfaced(funId))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::ADD_LIBRARY,
                                       HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type),
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(HLSFlowStep_Type::BUS_INTERFACE_GENERATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
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
   return ret;
}

DesignFlowStep_Status standard_hls::InternalExec()
{
   if(HLSMgr->CGetCallGraphManager().GetRootFunctions().count(funId))
   {
      STOP_TIME(HLSMgr->HLS_execution_time);
   }
   return DesignFlowStep_Status::EMPTY;
}
