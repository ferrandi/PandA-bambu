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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file hls_bit_value.cpp
 * @brief Wrapper for bit value analysis in the HLS context
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "hls_bit_value.hpp"

#include "behavioral_helper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "utility.hpp"

/// design_flow includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_analysis includes
#include "application_frontend_flow_step.hpp"
#include "frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_frontend_flow_step.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <cmath>
#include <map>
#include <vector>

#include <iosfwd>

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"

/// HLS includes
#include "hls_flow_step_factory.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

hls_bit_value::hls_bit_value(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::HLS_BIT_VALUE, HLSFlowStepSpecializationConstRef())
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

hls_bit_value::~hls_bit_value() = default;

void hls_bit_value::Initialize()
{
   HLSFunctionStep::Initialize();
}

void hls_bit_value::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   if(HLS && bb_version == FB->GetBBVersion() && GetStatus() == DesignFlowStep_Status::SUCCESS)
   {
      if(relationship_type == INVALIDATION_RELATIONSHIP)
      {
         if(parameters->isOption(OPT_bitvalue_ipa) and parameters->getOption<bool>(OPT_bitvalue_ipa))
         {
            vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE_IPA));
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step = frontend_step != NULL_VERTEX ?
                                                           design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                                                           GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))->CreateApplicationFrontendFlowStep(FrontendFlowStepType::BIT_VALUE_IPA);
            relationship.insert(design_flow_step);
         }
         {
            vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE_OPT, funId));
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step = frontend_step != NULL_VERTEX ?
                                                           design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step :
                                                           GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))->CreateFunctionFrontendFlowStep(FrontendFlowStepType::BIT_VALUE_OPT, funId);
            relationship.insert(design_flow_step);
         }
      }
   }
   HLSFunctionStep::ComputeRelationships(relationship, relationship_type);
}

const std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> hls_bit_value::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::CALLED_FUNCTIONS));
         ret.insert(std::make_tuple(HLSFlowStep_Type::INITIALIZE_HLS, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

DesignFlowStep_Status hls_bit_value::InternalExec()
{
   unsigned int curr_address_bitsize = HLSMgr->get_address_bitsize();
   unsigned int default_address_bitsize = parameters->isOption(OPT_addr_bus_bitsize) ? parameters->getOption<unsigned int>(OPT_addr_bus_bitsize) : 32;
   if(default_address_bitsize != curr_address_bitsize)
   {
      const DesignFlowStepRef design_flow_step = GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))->CreateFunctionFrontendFlowStep(FrontendFlowStepType::BIT_VALUE, funId);
      design_flow_step->Initialize();
      const DesignFlowStep_Status return_status = design_flow_step->Exec();
      return return_status;
   }
   return DesignFlowStep_Status::UNCHANGED;
}
