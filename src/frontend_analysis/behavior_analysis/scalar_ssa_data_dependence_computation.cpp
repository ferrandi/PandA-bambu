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
 * @file scalar_ssa_data_dependence_computation.cpp
 * @brief Analysis step performing data flow analysis based on ssa variables
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "scalar_ssa_data_dependence_computation.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"
#include "string_manipulation.hpp"

ScalarSsaDataDependenceComputation::ScalarSsaDataDependenceComputation(const ParameterConstRef _parameters,
                                                                       const application_managerRef _AppM,
                                                                       unsigned int _function_id,
                                                                       const DesignFlowManager& _design_flow_manager)
    : DataDependenceComputation(_AppM, _function_id, SCALAR_SSA_DATA_FLOW_ANALYSIS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
ScalarSsaDataDependenceComputation::ComputeFrontendRelationships(
    const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(VAR_ANALYSIS, SAME_FUNCTION));
         relationships.insert(std::make_pair(OP_ORDER_COMPUTATION, SAME_FUNCTION));
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

void ScalarSsaDataDependenceComputation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const auto fsaodg = function_behavior->GetOpGraph(FunctionBehavior::FSAODG);
      if(fsaodg.num_vertices() != 0)
      {
         for(const auto& edge : fsaodg.edges())
         {
            function_behavior->ogc->RemoveSelector(edge, DFG_SCA_SELECTOR | FB_DFG_SCA_SELECTOR | ADG_SCA_SELECTOR |
                                                             FB_ADG_SCA_SELECTOR | ODG_SCA_SELECTOR |
                                                             FB_ODG_SCA_SELECTOR);
         }
      }
   }
}
