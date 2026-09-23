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
 *   Copyright (C) 2023-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file dominator_allocation.cpp
 * @brief Composed pass to wrap function and memory dominator allocation
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "dominator_allocation.hpp"

#include "Parameter.hpp"
#include "memory_allocation.hpp"

dominator_allocation::dominator_allocation(const ParameterConstRef _parameters, const HLS_managerRef _HLS_mgr,
                                           const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _HLS_mgr, _design_flow_manager, HLSFlowStep_Type::DOMINATOR_ALLOCATION)
{
   composed = true;
}

HLS_step::HLSRelationships
dominator_allocation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
      case(PRECEDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_memory_allocation_algorithm),
                                    HLSFlowStepSpecializationConstRef(new MemoryAllocationSpecialization(
                                        parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy),
                                        parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type))),
                                    HLSFlowStep_Relationship::WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

bool dominator_allocation::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status dominator_allocation::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}
