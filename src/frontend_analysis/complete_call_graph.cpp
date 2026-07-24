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
 * @file complete_call_graph.cpp
 * @brief This class models the ending of execution of all functions which can add a function to call graph
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "complete_call_graph.hpp"

#include "Parameter.hpp"
#include "exceptions.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp"

#include <string>

CompleteCallGraph::CompleteCallGraph(const application_managerRef _AppM, const DesignFlowManager& _design_flow_manager,
                                     const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, COMPLETE_CALL_GRAPH, _design_flow_manager, _parameters)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
CompleteCallGraph::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(DATAFLOW_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SOFT_INT_CG_EXT, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(MUL_DECOMPOSITION, WHOLE_APPLICATION));
         if(parameters->isOption(OPT_openmp) && parameters->getOption<bool>(OPT_openmp))
         {
            relationships.insert(std::make_pair(OMP_LOWERING, ALL_FUNCTIONS));
         }
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(IR2FUN, ALL_FUNCTIONS));
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
         THROW_UNREACHABLE("Relationship type does not exist");
   }
   return relationships;
}

DesignFlowStep_Status CompleteCallGraph::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

bool CompleteCallGraph::HasToBeExecuted() const
{
   return true;
}
