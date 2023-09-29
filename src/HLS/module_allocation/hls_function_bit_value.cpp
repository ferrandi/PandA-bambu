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
 * @file hls_function_bit_value.cpp
 * @brief Wrapper for bit value analysis in the HLS context
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "hls_function_bit_value.hpp"

#include "behavioral_helper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "memory.hpp"
#include "utility.hpp"

/// design_flow includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_analysis includes
#include "application_frontend_flow_step.hpp"
#include "frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_frontend_flow_step.hpp"

#include "custom_map.hpp"
#include <cmath>
#include <vector>

#include <iosfwd>

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"

/// HLS includes
#include "hls_flow_step_factory.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

HLSFunctionBitValue::HLSFunctionBitValue(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                         unsigned _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::HLS_FUNCTION_BIT_VALUE,
                      HLSFlowStepSpecializationConstRef())
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

HLSFunctionBitValue::~HLSFunctionBitValue() = default;

void HLSFunctionBitValue::Initialize()
{
   HLSFunctionStep::Initialize();
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
HLSFunctionBitValue::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
       relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_memory_allocation_algorithm),
                                              HLSFlowStepSpecializationConstRef(),
                                              HLSFlowStep_Relationship::WHOLE_APPLICATION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
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
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      if(GetStatus() == DesignFlowStep_Status::SUCCESS && !parameters->getOption<int>(OPT_gcc_openmp_simd))
      {
         if(parameters->isOption(OPT_bitvalue_ipa) && parameters->getOption<bool>(OPT_bitvalue_ipa))
         {
            {
               const auto frontend_step = design_flow_manager.lock()->GetDesignFlowStep(
                   ApplicationFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE_IPA));
               const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
               const auto design_flow_step =
                   frontend_step != NULL_VERTEX ?
                       design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                       GetPointer<const FrontendFlowStepFactory>(
                           design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))
                           ->CreateApplicationFrontendFlowStep(FrontendFlowStepType::BIT_VALUE_IPA);
               relationship.insert(design_flow_step);
            }
         }
         const auto frontend_step = design_flow_manager.lock()->GetDesignFlowStep(
             FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE_OPT, funId));
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto design_flow_step =
             frontend_step != NULL_VERTEX ?
                 design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                 GetPointer<const FrontendFlowStepFactory>(
                     design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))
                     ->CreateFunctionFrontendFlowStep(FrontendFlowStepType::BIT_VALUE_OPT, funId);
         relationship.insert(design_flow_step);
      }
   }
   HLS_step::ComputeRelationships(relationship, relationship_type);
}

DesignFlowStep_Status HLSFunctionBitValue::InternalExec()
{
   const auto curr_address_bitsize = HLSMgr->get_address_bitsize();
   auto m64P = parameters->getOption<std::string>(OPT_gcc_m32_mx32).find("-m64") != std::string::npos;
   const auto default_address_bitsize = parameters->isOption(OPT_addr_bus_bitsize) ?
                                            parameters->getOption<unsigned int>(OPT_addr_bus_bitsize) :
                                            (m64P ? 64 : 32);
   if(default_address_bitsize != curr_address_bitsize)
   {
      const auto frontend_step = design_flow_manager.lock()->GetDesignFlowStep(
          FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE, funId));
      const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
      const auto design_flow_step = frontend_step != NULL_VERTEX ?
                                        design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                                        GetPointer<const FrontendFlowStepFactory>(
                                            design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))
                                            ->CreateFunctionFrontendFlowStep(FrontendFlowStepType::BIT_VALUE, funId);
      HLSMgr->Rmem->set_enable_hls_bit_value(true);
      design_flow_step->Initialize();
      const auto return_status = design_flow_step->Exec();
      HLSMgr->Rmem->set_enable_hls_bit_value(false);
      return return_status;
   }
   return DesignFlowStep_Status::UNCHANGED;
}
