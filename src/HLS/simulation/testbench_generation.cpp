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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file testbench_generation.cpp
 * @brief .
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "testbench_generation.hpp"

///. include
#include "Parameter.hpp"

/// STL includes
#include "custom_set.hpp"
#include <tuple>

TestbenchGeneration::TestbenchGeneration(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager) : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::TESTBENCH_GENERATION)
{
   composed = true;
}

TestbenchGeneration::~TestbenchGeneration() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> TestbenchGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::TEST_VECTOR_PARSER, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         const HLSFlowStep_Type interface_type = parameters->getOption<HLSFlowStep_Type>(OPT_interface_type);
#ifndef NDEBUG
         bool interface = interface_type == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION or interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION or interface_type == HLSFlowStep_Type::WB4_INTERFACE_GENERATION or
                          interface_type == HLSFlowStep_Type::INTERFACE_CS_GENERATION
#if HAVE_TASTE
                          or interface_type == HLSFlowStep_Type::TASTE_INTERFACE_GENERATION
#endif
             ;
         THROW_ASSERT(interface, "Unexpected interface type");
#endif
         ret.insert(std::make_tuple(interface_type == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION or interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION or interface_type == HLSFlowStep_Type::INTERFACE_CS_GENERATION
#if HAVE_TASTE
                                            or interface_type == HLSFlowStep_Type::TASTE_INTERFACE_GENERATION
#endif
                                        ?
                                        HLSFlowStep_Type::MINIMAL_TESTBENCH_GENERATION :
                                        HLSFlowStep_Type::WB4_TESTBENCH_GENERATION,
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
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

DesignFlowStep_Status TestbenchGeneration::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

bool TestbenchGeneration::HasToBeExecuted() const
{
   return true;
}
