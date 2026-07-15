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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file function_allocation.cpp
 * @brief Base class to allocate functions in high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "function_allocation.hpp"

#include "Parameter.hpp"
#include "fun_dominator_allocation.hpp"
#include "functions.hpp"
#include "hls_manager.hpp"

function_allocation::function_allocation(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                         const DesignFlowManager& _design_flow_manager,
                                         const HLSFlowStep_Type _hls_flow_step_type)
    : HLS_step(_Param, _HLSMgr, _design_flow_manager, _hls_flow_step_type)
{
}

HLS_step::HLSRelationships
function_allocation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::INITIALIZE_HLS, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::ALL_FUNCTIONS));
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

void function_allocation::Initialize()
{
   HLS_step::Initialize();
   HLSMgr->Rfuns = std::make_unique<functions>();
}
