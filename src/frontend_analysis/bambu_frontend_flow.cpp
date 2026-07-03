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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file bambu_frontend_flow.cpp
 * @brief The step representing the frontend flow for bambu
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 */
#include "bambu_frontend_flow.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_frontend_flow_step.hpp"
#include "hash_helper.hpp"
#include "hls_step.hpp"
#include "ir_manager.hpp"
#include "language_writer.hpp"
#include "string_manipulation.hpp"

#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

#if HAVE_HOST_PROFILING_BUILT
#include "host_profiling.hpp"
#endif

#include <iosfwd>
#include <string>

BambuFrontendFlow::BambuFrontendFlow(const application_managerRef _AppM, const DesignFlowManager& _design_flow_manager,
                                     const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, BAMBU_FRONTEND_FLOW, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BambuFrontendFlow::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(ADD_ARTIFICIAL_CALL_FLOW_EDGES, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(ADD_OP_EXIT_FLOW_EDGES, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(OP_CONTROL_DEPENDENCE_COMPUTATION, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SCALAR_SSA_DATA_FLOW_ANALYSIS, WHOLE_APPLICATION));

         relationships.insert(std::make_pair(BITVALUE_RANGE, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(COMPLETE_CALL_GRAPH, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(CSE_STEP, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(DCE_PASS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(EXTRACT_PATTERNS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(FANOUT_OPT, WHOLE_APPLICATION));
         if(!parameters->IsParameter("function-opt") || parameters->GetParameter<bool>("function-opt"))
         {
            relationships.insert(std::make_pair(FUNCTION_CALL_OPT, WHOLE_APPLICATION));
         }
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SOFT_INT_CG_EXT, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(MUL_DECOMPOSITION, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(INTERFACE_INFER, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(NI_SSA_LIVENESS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(PHI_OPT, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(PARM2SSA, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(PREDICATE_STATEMENTS, WHOLE_APPLICATION));
         if(!(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy)))
         {
            relationships.insert(std::make_pair(SIMPLE_CODE_MOTION, WHOLE_APPLICATION));
         }
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(IR2FUN, WHOLE_APPLICATION));

         relationships.insert(std::make_pair(MULTI_WAY_IF, WHOLE_APPLICATION));
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            relationships.insert(std::make_pair(COMMUTATIVE_EXPR_RESTRUCTURING, WHOLE_APPLICATION));
            relationships.insert(std::make_pair(SELECT_TREE_BALANCING, WHOLE_APPLICATION));
            relationships.insert(std::make_pair(SDC_CODE_MOTION, WHOLE_APPLICATION));
         }
#if HAVE_HOST_PROFILING_BUILT
         if(parameters->getOption<HostProfiling_Method>(OPT_profiling_method) != HostProfiling_Method::PM_NONE)
         {
            relationships.insert(std::make_pair(HOST_PROFILING, WHOLE_APPLICATION));
         }
#endif
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

bool BambuFrontendFlow::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status BambuFrontendFlow::Exec()
{
   if(parameters->getOption<bool>(OPT_print_dot) || debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      AppM->CGetCallGraphManager().GetCallGraph().writeDot(
          parameters->getOption<std::filesystem::path>(OPT_dot_directory) / "call_graph_final.dot");
   }
   return DesignFlowStep_Status::EMPTY;
}
