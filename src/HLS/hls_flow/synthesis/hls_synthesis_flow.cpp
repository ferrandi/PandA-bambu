/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2015-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file hls_synthesis_flow.cpp
 * @brief Definition of the class to create the structural description of a function
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "hls_synthesis_flow.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "custom_set.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"

#include <tuple>

HLSSynthesisFlow::HLSSynthesisFlow(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                   unsigned int _funId, const DesignFlowManager& _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::HLS_SYNTHESIS_FLOW)
{
   composed = true;
}

HLS_step::HLSRelationships
HLSSynthesisFlow::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_hls_flow),
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

DesignFlowStep_Status HLSSynthesisFlow::InternalExec()
{
   return DesignFlowStep_Status::EMPTY;
}
