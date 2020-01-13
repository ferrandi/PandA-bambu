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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file hls_synthesis_flow.cpp
 * @brief Definition of the class to create the structural description of a function
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "hls_synthesis_flow.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "function_behavior.hpp"

/// HLS include
#include "hls_manager.hpp"

/// STL include
#include "custom_set.hpp"
#include <tuple>

/// tree include
#include "behavioral_helper.hpp"
#include "design_flow_manager.hpp"

HLSSynthesisFlow::HLSSynthesisFlow(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::HLS_SYNTHESIS_FLOW)
{
   composed = true;
}

HLSSynthesisFlow::~HLSSynthesisFlow() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> HLSSynthesisFlow::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
#if HAVE_FROM_PRAGMA_BUILT
         const auto function_behavior = HLSMgr->GetFunctionBehavior(funId);
         const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
         if(parameters->isOption(OPT_context_switch))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::OMP_FUNCTION_ALLOCATION_CS, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
            if(design_flow_manager.lock()->GetStatus(HLS_step::ComputeSignature(HLSFlowStep_Type::OMP_FUNCTION_ALLOCATION_CS, HLSFlowStepSpecializationConstRef())) == DesignFlowStep_Status::SUCCESS)
            {
               if(behavioral_helper->IsOmpBodyLoop())
               {
                  ret.insert(std::make_tuple(HLSFlowStep_Type::OMP_BODY_LOOP_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
               }
               else if(behavioral_helper->GetOmpForDegree())
               {
                  if(parameters->isOption(OPT_context_switch))
                  {
                     ret.insert(std::make_tuple(HLSFlowStep_Type::OMP_FOR_WRAPPER_CS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
                  }
#if HAVE_EXPERIMENTAL
                  else
                  {
                     ret.insert(std::make_tuple(HLSFlowStep_Type::OMP_FOR_WRAPPER_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
                  }
#endif
               }
               else
               {
                  ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_hls_flow), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
               }
            }
         }
         else
#endif
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_hls_flow), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
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

DesignFlowStep_Status HLSSynthesisFlow::InternalExec()
{
   return DesignFlowStep_Status::EMPTY;
}
