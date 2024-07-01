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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file BitValueRange.cpp
 * @brief Class performing some optimizations on the IR exploiting Bit Value analysis but executed after Range Analysis.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "BitValueRange.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "string_manipulation.hpp"

BitValueRange::BitValueRange(const ParameterConstRef _parameters, const application_managerRef _AppM,
                             unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BITVALUE_RANGE, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

BitValueRange::~BitValueRange() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BitValueRange::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BIT_VALUE_OPT, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(BITVALUE_RANGE, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(RANGE_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
            {
               relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
            }
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

DesignFlowStep_Status BitValueRange::InternalExec()
{
   const auto design_flow_step = GetPointerS<const FrontendFlowStepFactory>(
                                     design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND))
                                     ->CreateFunctionFrontendFlowStep(FrontendFlowStepType::BIT_VALUE_OPT, function_id);
   design_flow_step->Initialize();
   const auto return_status = design_flow_step->Exec();
   return_status == DesignFlowStep_Status::SUCCESS ? function_behavior->UpdateBBVersion() : 0;
   return return_status;
}

bool BitValueRange::HasToBeExecuted() const
{
   return (FunctionFrontendFlowStep::HasToBeExecuted() || bitvalue_version != function_behavior->GetBitValueVersion());
}
