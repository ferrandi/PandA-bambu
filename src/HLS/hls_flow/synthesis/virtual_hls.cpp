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
 * @file virtual_hls.cpp
 * @brief A brief description of the C++ Source File
 *
 * Here goes a detailed description of the file
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "virtual_hls.hpp"

#include "Parameter.hpp"
#include "cdfc_module_binding.hpp"
#include "design_flow_manager.hpp"
#include "hls.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "string_manipulation.hpp"
#include "weighted_clique_register.hpp"

virtual_hls::virtual_hls(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId,
                         const DesignFlowManager& _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::VIRTUAL_DESIGN_FLOW)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   composed = true;
}

HLS_step::HLSRelationships
virtual_hls::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));

         ret.insert(std::make_tuple(HLSFlowStep_Type::EASY_MODULE_BINDING, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));

         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::LIST_BASED_SCHEDULING, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm),
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         ret.insert(std::make_tuple(HLSFlowStep_Type::BUILD_FSM, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::DOMINATOR_ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::WHOLE_APPLICATION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->chaining_algorithm, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->liveVariableAlgorithm,
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_storage_value_insertion_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->module_binding_algorithm,
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }

         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_register_allocation_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));

         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_datapath_interconnection_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

DesignFlowStep_Status virtual_hls::InternalExec()
{
   return DesignFlowStep_Status::EMPTY;
}
