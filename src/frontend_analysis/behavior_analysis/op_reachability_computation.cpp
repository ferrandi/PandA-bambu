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
 * @file op_reachability_computation.cpp
 * @brief Analysis step computing reachability between operations
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
/// Header include
#include "op_reachability_computation.hpp"

///. includes
#include "Parameter.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

OpReachabilityComputation::OpReachabilityComputation(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, OP_REACHABILITY_COMPUTATION, _design_flow_manager, _Param)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

OpReachabilityComputation::~OpReachabilityComputation() = default;

void OpReachabilityComputation::Initialize()
{
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> OpReachabilityComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         /// This dependence is necessary to guarantee that FunctionBehavior::CheckReachability can be used
         /// FunctionBehavior::CheckReachability indeed uses map_levels
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OP_ORDER_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_REACHABILITY_COMPUTATION, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status OpReachabilityComputation::InternalExec()
{
   return DesignFlowStep_Status::EMPTY;
}
