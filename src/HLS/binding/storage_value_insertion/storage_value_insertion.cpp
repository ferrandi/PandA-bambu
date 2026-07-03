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
 * @file storage_value_insertion.cpp
 * @brief Storage value insertion step suitable for variables with stages
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "storage_value_insertion.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "cpu_time.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "liveVariables.hpp"
#include "storage_value_information.hpp"

storage_value_insertion::storage_value_insertion(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                                 unsigned int _funId, const DesignFlowManager& _design_flow_manager)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager,
                      HLSFlowStep_Type::VALUES_SCHEME_STORAGE_VALUE_INSERTION)
{
}

HLS_step::HLSRelationships
storage_value_insertion::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::DOMINATOR_ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::WHOLE_APPLICATION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->liveVariableAlgorithm,
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

void storage_value_insertion::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->storage_value_information = std::make_unique<StorageValueInformation>(HLSMgr, funId);
   HLS->storage_value_information->Initialize();
}

DesignFlowStep_Status storage_value_insertion::InternalExec()
{
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      START_TIME(step_time);
   }
   const auto IRM = HLSMgr->get_ir_manager();
   unsigned int i = 0;
   const auto states = HLS->fsm_info->vertices();
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();

   for(auto vIt : states)
   {
      // std::cerr << "current state for sv " << HLS->Rliv->get_name(vIt) << std::endl;
      const auto& live = HLS->Rliv->getLiveInFsmVariables(vIt);
      for(auto k : live)
      {
         if(!HLS->storage_value_information->is_a_storage_value(vIt, k.first, k.second))
         {
            // std::cerr << BH->PrintVariable(k.first) << "(" << k.first << ")"
            //           << "=" << k.second << " ->" << i << "\n";
            HLS->storage_value_information->set_storage_value_index(vIt, k.first, k.second, i);
            i++;
         }
      }
   }
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "-->Storage Value Information of function " + BH->GetFunctionName() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of storage values inserted: " + std::to_string(i));
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "Time to compute storage value information: " + print_cpu_time(step_time) + " seconds");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   return DesignFlowStep_Status::SUCCESS;
}
