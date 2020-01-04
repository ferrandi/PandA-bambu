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
 *              Copyright (C) 2019-2020 Politecnico di Milano
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
 * @file hls_bit_value.hpp
 * @brief Composed step to describe HLSFunction on all functions
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "hls_bit_value.hpp"

/// . include
#include "Parameter.hpp"

/// behavior includes
#include "application_frontend_flow_step.hpp"
#include "call_graph_manager.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_flow includes
#include "frontend_flow_step_factory.hpp"
#include "function_frontend_flow_step.hpp"

/// HLS includes
#include "hls_function_step.hpp"
#include "hls_manager.hpp"

/// . utility includes
#include "custom_set.hpp"
#include "utility.hpp"

/// STL includes
#include <tuple>

HLSBitValue::HLSBitValue(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::HLS_BIT_VALUE, HLSFlowStepSpecializationConstRef())
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

HLSBitValue::~HLSBitValue() = default;

void HLSBitValue::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      CustomSet<unsigned int> changed_functions;
      const auto call_graph_manager = HLSMgr->CGetCallGraphManager();
      const auto reached_body_fun_ids = call_graph_manager->GetReachedBodyFunctions();
      for(const auto reached_body_fun_id : reached_body_fun_ids)
      {
         const auto hls_function_bit_value_signature = HLSFunctionStep::ComputeSignature(HLSFlowStep_Type::HLS_FUNCTION_BIT_VALUE, HLSFlowStepSpecializationConstRef(), reached_body_fun_id);
         const auto status = design_flow_manager.lock()->GetStatus(hls_function_bit_value_signature);
         if(status == DesignFlowStep_Status::SUCCESS)
         {
            changed_functions.insert(reached_body_fun_id);
         }
         else
         {
            THROW_ASSERT(status == DesignFlowStep_Status::UNCHANGED, "");
         }
      }
      if(parameters->isOption(OPT_bitvalue_ipa) and parameters->getOption<bool>(OPT_bitvalue_ipa))
      {
         if(not changed_functions.empty())
         {
            vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE_IPA));
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step = frontend_step != NULL_VERTEX ?
                                                           design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                                                           GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))->CreateApplicationFrontendFlowStep(FrontendFlowStepType::BIT_VALUE_IPA);
            relationship.insert(design_flow_step);
         }
      }
      else
      {
         for(const auto changed_function : changed_functions)
         {
            vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE_OPT, changed_function));
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step = frontend_step != NULL_VERTEX ?
                                                           design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                                                           GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))->CreateFunctionFrontendFlowStep(FrontendFlowStepType::BIT_VALUE_OPT, changed_function);
            relationship.insert(design_flow_step);
         }
      }
   }
   HLS_step::ComputeRelationships(relationship, relationship_type);
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> HLSBitValue::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_FUNCTION_BIT_VALUE, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::ALL_FUNCTIONS));
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
   return ret;
}

DesignFlowStep_Status HLSBitValue::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

bool HLSBitValue::HasToBeExecuted() const
{
   return true;
}
