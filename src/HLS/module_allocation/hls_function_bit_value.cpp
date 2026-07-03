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
 * @file hls_function_bit_value.cpp
 * @brief Wrapper for bit value analysis in the HLS context
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "hls_function_bit_value.hpp"

#include "Parameter.hpp"
#include "application_frontend_flow_step.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "exceptions.hpp"
#include "frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "function_frontend_flow_step.hpp"
#include "hls.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "memory.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

#include <cmath>
#include <iosfwd>
#include <vector>

HLSFunctionBitValue::HLSFunctionBitValue(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                         unsigned _funId, const DesignFlowManager& _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::HLS_FUNCTION_BIT_VALUE,
                      HLSFlowStepSpecializationConstRef())
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

void HLSFunctionBitValue::Initialize()
{
   HLSFunctionStep::Initialize();
}

HLS_step::HLSRelationships
HLSFunctionBitValue::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_memory_allocation_algorithm),
                                              HLSFlowStepSpecializationConstRef(),
                                              HLSFlowStep_Relationship::WHOLE_APPLICATION));
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
   return relationships;
}

void HLSFunctionBitValue::ComputeRelationships(DesignFlowStepSet& relationship,
                                               const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP && GetStatus() == DesignFlowStep_Status::SUCCESS)
   {
      if(parameters->isOption(OPT_bitvalue_ipa) && parameters->getOption<bool>(OPT_bitvalue_ipa))
      {
         const auto frontend_step = design_flow_manager.GetDesignFlowStep(
             ApplicationFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE_IPA));
         const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
         const auto design_flow_step = frontend_step != DesignFlowGraph::null_vertex() ?
                                           design_flow_graph->CGetNodeInfo(frontend_step)->design_flow_step :
                                           GetPointer<const FrontendFlowStepFactory>(
                                               design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND))
                                               ->CreateApplicationFrontendFlowStep(FrontendFlowStepType::BIT_VALUE_IPA);
         relationship.insert(design_flow_step);
      }
      const auto frontend_step = design_flow_manager.GetDesignFlowStep(
          FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE_OPT, funId));
      const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
      const auto design_flow_step =
          frontend_step != DesignFlowGraph::null_vertex() ?
              design_flow_graph->CGetNodeInfo(frontend_step)->design_flow_step :
              GetPointer<const FrontendFlowStepFactory>(
                  design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND))
                  ->CreateFunctionFrontendFlowStep(FrontendFlowStepType::BIT_VALUE_OPT, funId);
      relationship.insert(design_flow_step);
   }
   HLS_step::ComputeRelationships(relationship, relationship_type);
}

DesignFlowStep_Status HLSFunctionBitValue::InternalExec()
{
   const auto curr_address_bitsize = HLSMgr->get_address_bitsize();
   auto m64P = parameters->getOption<std::string>(OPT_cc_m_env).find("-m64") != std::string::npos;
   const auto default_address_bitsize = parameters->isOption(OPT_addr_bus_bitsize) ?
                                            parameters->getOption<unsigned int>(OPT_addr_bus_bitsize) :
                                            (m64P ? 64 : 32);
   if(default_address_bitsize != curr_address_bitsize)
   {
      const auto frontend_step = design_flow_manager.GetDesignFlowStep(
          FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE, funId));
      const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
      const auto design_flow_step = frontend_step != DesignFlowGraph::null_vertex() ?
                                        design_flow_graph->CGetNodeInfo(frontend_step)->design_flow_step :
                                        GetPointer<const FrontendFlowStepFactory>(
                                            design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND))
                                            ->CreateFunctionFrontendFlowStep(FrontendFlowStepType::BIT_VALUE, funId);
      HLSMgr->Rmem->set_enable_hls_bit_value(true);
      design_flow_step->Initialize();
      const auto return_status = design_flow_step->Exec();
      HLSMgr->Rmem->set_enable_hls_bit_value(false);
      return return_status;
   }
   return DesignFlowStep_Status::UNCHANGED;
}
