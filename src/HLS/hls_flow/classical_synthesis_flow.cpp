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
 * @file classical_synthesis_flow.cpp
 * @brief
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "classical_synthesis_flow.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "evaluation_mode.hpp"

ClassicalHLSSynthesisFlow::ClassicalHLSSynthesisFlow(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                                     const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::CLASSICAL_HLS_SYNTHESIS_FLOW)
{
   composed = true;
}

HLS_step::HLSRelationships
ClassicalHLSSynthesisFlow::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_memory_allocation_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));

         if(parameters->isOption(OPT_export_core_mode))
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_export_core_mode),
                                       HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         ret.insert(std::make_tuple(HLSFlowStep_Type::WRITE_HLS_SUMMARY, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::WHOLE_APPLICATION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::EVALUATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         if(parameters->isOption(OPT_generate_testbench) && parameters->getOption<bool>(OPT_generate_testbench))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_GENERATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::TOP_FUNCTION));
         }
#if HAVE_VCD_BUILT
         if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::VCD_UTILITY, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::TOP_FUNCTION));
         }
#endif
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

DesignFlowStep_Status ClassicalHLSSynthesisFlow::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

bool ClassicalHLSSynthesisFlow::HasToBeExecuted() const
{
   return true;
}
