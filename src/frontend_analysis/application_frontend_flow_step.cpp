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
 * @file application_frontend_flow_step.cpp
 * @brief This class contains the base representation for a generic frontend flow step which works on the whole function
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "application_frontend_flow_step.hpp"

#include "Parameter.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp"
#include "symbolic_application_frontend_flow_step.hpp"

#include <iostream>

#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

ApplicationFrontendFlowStep::ApplicationFrontendFlowStep(DesignFlowStep::signature_t _signature,
                                                         const application_managerRef _AppM,
                                                         const FrontendFlowStepType _frontend_flow_step_type,
                                                         const DesignFlowManager& _design_flow_manager,
                                                         const ParameterConstRef _parameters)
    : FrontendFlowStep(_signature, _AppM, _frontend_flow_step_type, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

ApplicationFrontendFlowStep::ApplicationFrontendFlowStep(const application_managerRef _AppM,
                                                         const FrontendFlowStepType _frontend_flow_step_type,
                                                         const DesignFlowManager& _design_flow_manager,
                                                         const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(ComputeSignature(_frontend_flow_step_type), _AppM, _frontend_flow_step_type,
                                  _design_flow_manager, _parameters)
{
}

DesignFlowStep::signature_t
ApplicationFrontendFlowStep::ComputeSignature(const FrontendFlowStepType frontend_flow_step_type)
{
   switch(frontend_flow_step_type)
   {
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
      case ADD_OP_EXIT_FLOW_EDGES:
      case BASIC_BLOCKS_CFG_COMPUTATION:
      case BB_CONTROL_DEPENDENCE_COMPUTATION:
      case BB_FEEDBACK_EDGES_IDENTIFICATION:
      case BB_ORDER_COMPUTATION:
      case BIT_VALUE:
      case BIT_VALUE_OPT:
      case BITVALUE_RANGE:
      case BLOCK_FIX:
      case BUILD_VIRTUAL_PHI:
      case CALL_NODE_FIX:
      case CALL_GRAPH_BUILTIN_CALL:
      case CHECK_SYSTEM_TYPE:
      case COMPLETE_BB_GRAPH:
      case COMMUTATIVE_EXPR_RESTRUCTURING:
      case SELECT_TREE_BALANCING:
      case CSE_STEP:
      case DATAFLOW_CG_EXT:
      case DCE_PASS:
      case DETERMINE_MEMORY_ACCESSES:
      case DOM_POST_DOM_COMPUTATION:
      case(FANOUT_OPT):
      case EXTRACT_COND_OP:
      case EXTRACT_PATTERNS:
      case FIX_STRUCTS_PASSED_BY_VALUE:
      case FIX_VDEF:
      case FUNCTION_CALL_TYPE_CLEANUP:
      case FUNCTION_CALL_OPT:
      case HWCALL_INJECTION:
      case IR_LOWERING:
      case LOOP_COMPUTATION:
      case LOOPS_COMPUTATION:
      case MULTI_WAY_IF:
      case NI_SSA_LIVENESS:
      case OMP_CG_EXT:
      case OMP_LOWERING:
      case OP_CONTROL_DEPENDENCE_COMPUTATION:
      case OP_FEEDBACK_EDGES_IDENTIFICATION:
      case OP_ORDER_COMPUTATION:
      case OPERATIONS_CFG_COMPUTATION:
      case PARM2SSA:
      case PARM_DECL_TAKEN_ADDRESS:
      case PHI_OPT:
      case PREDICATE_STATEMENTS:
      case REBUILD_INITIALIZATION2:
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      case SDC_CODE_MOTION:
      case SIMPLE_CODE_MOTION:
      case SOFT_FLOAT_CG_EXT:
      case IR2FUN:
      case UPDATE_SCHEDULE:
      case UNROLLING_DEGREE:
      case USE_COUNTING:
      case VAR_ANALYSIS:
      case VAR_DECL_FIX:
      case VERIFICATION_OPERATION:
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
      {
         return SymbolicApplicationFrontendFlowStep::ComputeSignature(frontend_flow_step_type);
      }
      case(BAMBU_FRONTEND_FLOW):
      case BIT_VALUE_IPA:
      case INTERFACE_INFER:
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
#endif
      case(COMPLETE_CALL_GRAPH):
      case(CREATE_IR_MANAGER):
      case DEAD_CODE_ELIMINATION_IPA:
      case FIND_MAX_TRANSFORMATIONS:
      case(FUNCTION_ANALYSIS):
#if HAVE_HOST_PROFILING_BUILT
      case(HOST_PROFILING):
#endif
      case MUL_DECOMPOSITION:
      case RANGE_ANALYSIS:
      case SOFT_INT_CG_EXT:
      case(SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP):
      {
         return DesignFlowStep::ComputeSignature(APPLICATION_FRONTEND,
                                                 static_cast<unsigned short>(frontend_flow_step_type), 0);
      }

      default:
         break;
   }
   THROW_UNREACHABLE("Frontend flow step type does not exist");
   return 0;
}

std::string ApplicationFrontendFlowStep::GetName() const
{
   return "Frontend::" + GetKindText();
}

bool ApplicationFrontendFlowStep::HasToBeExecuted() const
{
   return true;
}
