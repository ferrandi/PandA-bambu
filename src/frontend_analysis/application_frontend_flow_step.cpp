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
 * @file application_frontend_flow_step.cpp
 * @brief This class contains the base representation for a generic frontend flow step which works on the whole function
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "application_frontend_flow_step.hpp"

#include "Parameter.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp"
#include "symbolic_application_frontend_flow_step.hpp"

#include <iostream>

#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_ILP_BUILT.hpp"
#include "config_HAVE_TASTE.hpp"

ApplicationFrontendFlowStep::ApplicationFrontendFlowStep(DesignFlowStep::signature_t _signature,
                                                         const application_managerRef _AppM,
                                                         const FrontendFlowStepType _frontend_flow_step_type,
                                                         const DesignFlowManagerConstRef _design_flow_manager,
                                                         const ParameterConstRef _parameters)
    : FrontendFlowStep(_signature, _AppM, _frontend_flow_step_type, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

ApplicationFrontendFlowStep::ApplicationFrontendFlowStep(const application_managerRef _AppM,
                                                         const FrontendFlowStepType _frontend_flow_step_type,
                                                         const DesignFlowManagerConstRef _design_flow_manager,
                                                         const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(ComputeSignature(_frontend_flow_step_type), _AppM, _frontend_flow_step_type,
                                  _design_flow_manager, _parameters)
{
}

ApplicationFrontendFlowStep::~ApplicationFrontendFlowStep() = default;

DesignFlowStep::signature_t
ApplicationFrontendFlowStep::ComputeSignature(const FrontendFlowStepType frontend_flow_step_type)
{
   switch(frontend_flow_step_type)
   {
      case ADD_BB_ECFG_EDGES:
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
      case ADD_OP_EXIT_FLOW_EDGES:
      case ADD_OP_LOOP_FLOW_EDGES:
      case ADD_OP_PHI_FLOW_EDGES:
      case BASIC_BLOCKS_CFG_COMPUTATION:
      case BB_CONTROL_DEPENDENCE_COMPUTATION:
      case BB_FEEDBACK_EDGES_IDENTIFICATION:
      case BB_ORDER_COMPUTATION:
      case BB_REACHABILITY_COMPUTATION:
      case BIT_VALUE:
      case BIT_VALUE_OPT:
      case BITVALUE_RANGE:
      case BLOCK_FIX:
      case BUILD_VIRTUAL_PHI:
      case CALL_EXPR_FIX:
      case CALL_GRAPH_BUILTIN_CALL:
      case CHECK_SYSTEM_TYPE:
      case COMPLETE_BB_GRAPH:
      case COMPUTE_IMPLICIT_CALLS:
      case COMMUTATIVE_EXPR_RESTRUCTURING:
      case COND_EXPR_RESTRUCTURING:
      case CSE_STEP:
      case DATAFLOW_CG_EXT:
      case DEAD_CODE_ELIMINATION:
      case DETERMINE_MEMORY_ACCESSES:
      case DOM_POST_DOM_COMPUTATION:
      case ESSA:
      case(FANOUT_OPT):
      case MULTIPLE_ENTRY_IF_REDUCTION:
      case EXTRACT_GIMPLE_COND_OP:
#if HAVE_FROM_PRAGMA_BUILT
      case EXTRACT_OMP_ATOMIC:
      case EXTRACT_OMP_FOR:
#endif
      case EXTRACT_PATTERNS:
      case FIX_STRUCTS_PASSED_BY_VALUE:
      case FIX_VDEF:
      case FUNCTION_CALL_TYPE_CLEANUP:
      case FUNCTION_CALL_OPT:
      case HDL_VAR_DECL_FIX:
      case HWCALL_INJECTION:
      case IR_LOWERING:
      case LOOP_COMPUTATION:
      case LOOPS_ANALYSIS_BAMBU:
      case LOOPS_COMPUTATION:
      case LUT_TRANSFORMATION:
      case MULTI_WAY_IF:
      case NI_SSA_LIVENESS:
      case OP_CONTROL_DEPENDENCE_COMPUTATION:
      case OP_FEEDBACK_EDGES_IDENTIFICATION:
      case OP_ORDER_COMPUTATION:
      case OP_REACHABILITY_COMPUTATION:
      case OPERATIONS_CFG_COMPUTATION:
      case PARM2SSA:
      case PARM_DECL_TAKEN_ADDRESS:
      case PHI_OPT:
      case PREDICATE_STATEMENTS:
      case REBUILD_INITIALIZATION:
      case REBUILD_INITIALIZATION2:
      case REMOVE_CLOBBER_GA:
      case REMOVE_ENDING_IF:
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
#if HAVE_ILP_BUILT
      case SDC_CODE_MOTION:
#endif
      case SERIALIZE_MUTUAL_EXCLUSIONS:
      case SPLIT_RETURN:
      case SHORT_CIRCUIT_TAF:
      case SIMPLE_CODE_MOTION:
      case SOFT_FLOAT_CG_EXT:
      case SWITCH_FIX:
      case TREE2FUN:
      case UN_COMPARISON_LOWERING:
#if HAVE_ILP_BUILT
      case UPDATE_SCHEDULE:
#endif
      case UNROLLING_DEGREE:
      case USE_COUNTING:
      case VAR_ANALYSIS:
      case VAR_DECL_FIX:
      case VECTORIZE:
      case VERIFICATION_OPERATION:
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
      case VIRTUAL_PHI_NODES_SPLIT:
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
#if HAVE_TASTE
      case CREATE_ADDRESS_TRANSLATION:
#endif
      case(CREATE_TREE_MANAGER):
      case DEAD_CODE_ELIMINATION_IPA:
      case FIND_MAX_TRANSFORMATIONS:
      case(FUNCTION_ANALYSIS):
      case HDL_FUNCTION_DECL_FIX:
#if HAVE_HOST_PROFILING_BUILT
      case(HOST_PROFILING):
#endif
      case MULT_EXPR_FRACTURING:
#if HAVE_FROM_PRAGMA_BUILT
      case(PRAGMA_ANALYSIS):
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case(PRAGMA_SUBSTITUTION):
#endif
      case RANGE_ANALYSIS:
      case SOFT_INT_CG_EXT:
      case(STRING_CST_FIX):
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
