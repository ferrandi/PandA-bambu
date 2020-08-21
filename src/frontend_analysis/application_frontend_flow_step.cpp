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

#include "config_HAVE_BAMBU_BUILT.hpp"          // for HAVE_BAMBU_BUILT
#include "config_HAVE_EXPERIMENTAL.hpp"         // for HAVE_EXPERIME...
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"    // for HAVE_FROM_PRA...
#include "config_HAVE_HOST_PROFILING_BUILT.hpp" // for HAVE_HOST_PRO...
#include "config_HAVE_ILP_BUILT.hpp"            // for HAVE_ILP_BUILT
#include "config_HAVE_RTL_BUILT.hpp"            // for HAVE_RTL_BUILT
#include "config_HAVE_TASTE.hpp"                // for HAVE_TASTE
#include "config_HAVE_ZEBU_BUILT.hpp"           // for HAVE_ZEBU_BUILT

#include "Parameter.hpp"                               // for Parameter
#include "exceptions.hpp"                              // for THROW_UNREACH...
#include "string_manipulation.hpp"                     // for GET_CLASS
#include "symbolic_application_frontend_flow_step.hpp" // for SymbolicAppli...
#include <boost/lexical_cast.hpp>                      // for lexical_cast
#include <iostream>                                    // for ios_base::fai...

ApplicationFrontendFlowStep::ApplicationFrontendFlowStep(const application_managerRef _AppM, const FrontendFlowStepType _frontend_flow_step_type, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FrontendFlowStep(_AppM, _frontend_flow_step_type, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

ApplicationFrontendFlowStep::~ApplicationFrontendFlowStep() = default;

const std::string ApplicationFrontendFlowStep::ComputeSignature(const FrontendFlowStepType frontend_flow_step_type)
{
   switch(frontend_flow_step_type)
   {
      case ADD_BB_ECFG_EDGES:
#if HAVE_ZEBU_BUILT
      case ADD_OP_ECFG_EDGES:
#endif
#if HAVE_BAMBU_BUILT
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
#endif
      case ADD_OP_EXIT_FLOW_EDGES:
      case ADD_OP_LOOP_FLOW_EDGES:
#if HAVE_BAMBU_BUILT
      case ADD_OP_PHI_FLOW_EDGES:
#endif
      case AGGREGATE_DATA_FLOW_ANALYSIS:
#if HAVE_ZEBU_BUILT
      case(ARRAY_REF_FIX):
#endif
      case BASIC_BLOCKS_CFG_COMPUTATION:
      case BB_CONTROL_DEPENDENCE_COMPUTATION:
      case BB_FEEDBACK_EDGES_IDENTIFICATION:
      case BB_ORDER_COMPUTATION:
      case BB_REACHABILITY_COMPUTATION:
#if HAVE_BAMBU_BUILT
      case BIT_VALUE:
      case BIT_VALUE_OPT:
#endif
      case BLOCK_FIX:
      case BUILD_VIRTUAL_PHI:
#if HAVE_ZEBU_BUILT
      case CALL_ARGS_STRUCTURING:
#endif
      case CALL_EXPR_FIX:
#if HAVE_BAMBU_BUILT
      case CALL_GRAPH_BUILTIN_CALL:
#endif
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case CHECK_CRITICAL_SESSION:
#endif
#if HAVE_ZEBU_BUILT
      case CHECK_PIPELINABLE_LOOPS:
#endif
      case CHECK_SYSTEM_TYPE:
      case CLEAN_VIRTUAL_PHI:
      case COMPLETE_BB_GRAPH:
#if HAVE_BAMBU_BUILT
      case COMPUTE_IMPLICIT_CALLS:
      case COMMUTATIVE_EXPR_RESTRUCTURING:
      case COND_EXPR_RESTRUCTURING:
      case CSE_STEP:
#endif
#if HAVE_ZEBU_BUILT || HAVE_BAMBU_BUILT
      case DEAD_CODE_ELIMINATION:
#endif
#if HAVE_BAMBU_BUILT
      case DETERMINE_MEMORY_ACCESSES:
#endif
      case DOM_POST_DOM_COMPUTATION:
#if HAVE_BAMBU_BUILT
      case ESSA:
      case(FANOUT_OPT):
      case MULTIPLE_ENTRY_IF_REDUCTION:
#endif
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case DYNAMIC_AGGREGATE_DATA_FLOW_ANALYSIS:
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case EXTENDED_PDG_COMPUTATION:
#endif
#if HAVE_BAMBU_BUILT
      case EXTRACT_GIMPLE_COND_OP:
#endif
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
      case EXTRACT_OMP_ATOMIC:
      case EXTRACT_OMP_FOR:
#endif
#if HAVE_BAMBU_BUILT
      case EXTRACT_PATTERNS:
      case FIX_STRUCTS_PASSED_BY_VALUE:
      case FUNCTION_CALL_TYPE_CLEANUP:
#endif
#if HAVE_ZEBU_BUILT
      case GLOBAL_VARIABLES_ANALYSIS:
#endif
#if HAVE_BAMBU_BUILT
      case HDL_VAR_DECL_FIX:
#endif
#if HAVE_ZEBU_BUILT
      case HEADER_STRUCTURING:
      case INSTRUCTION_SEQUENCES_COMPUTATION:
#endif
#if HAVE_BAMBU_BUILT
      case HLS_DIV_CG_EXT:
      case HWCALL_INJECTION:
      case INTERFACE_INFER:
      case IR_LOWERING:
#endif
      case LOOP_COMPUTATION:
#if HAVE_ZEBU_BUILT
      case LOOP_REGIONS_COMPUTATION:
      case LOOP_REGIONS_FLOW_COMPUTATION:
#endif
#if HAVE_BAMBU_BUILT
      case LOOPS_ANALYSIS_BAMBU:
#endif
#if HAVE_ZEBU_BUILT
      case LOOPS_ANALYSIS_ZEBU:
#endif
      case LOOPS_COMPUTATION:
#if HAVE_ZEBU_BUILT
      case LOOPS_REBUILDING:
#endif
#if HAVE_BAMBU_BUILT
      case LUT_TRANSFORMATION:
#endif
      case MEM_CG_EXT:
      case MEMORY_DATA_FLOW_ANALYSIS:
#if HAVE_BAMBU_BUILT
      case MULTI_WAY_IF:
      case NI_SSA_LIVENESS:
#endif
      case OP_CONTROL_DEPENDENCE_COMPUTATION:
      case OP_FEEDBACK_EDGES_IDENTIFICATION:
      case OP_ORDER_COMPUTATION:
      case OP_REACHABILITY_COMPUTATION:
      case OPERATIONS_CFG_COMPUTATION:
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case PARALLEL_LOOP_SWAP:
      case PARALLEL_LOOPS_ANALYSIS:
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case PARALLEL_REGIONS_GRAPH_COMPUTATION:
#endif
#if HAVE_BAMBU_BUILT
      case PARM_DECL_TAKEN_ADDRESS:
      case PHI_OPT:
#endif
#if HAVE_ZEBU_BUILT
      case POINTED_DATA_COMPUTATION:
#endif
#if HAVE_BAMBU_BUILT
      case PREDICATE_STATEMENTS:
#endif
#if HAVE_ZEBU_BUILT
      case PREDICTABILITY_ANALYSIS:
      case PROBABILITY_PATH:
#endif
#if HAVE_BAMBU_BUILT
      case REBUILD_INITIALIZATION:
      case REBUILD_INITIALIZATION2:
#endif
#if HAVE_ZEBU_BUILT
#if HAVE_EXPERIMENTAL
      case REFINED_AGGREGATE_DATA_FLOW_ANALYSIS:
      case REFINED_VAR_COMPUTATION:
#endif
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case REDUCED_PDG_COMPUTATION:
#endif
#if HAVE_BAMBU_BUILT
      case REMOVE_CLOBBER_GA:
#endif
#if HAVE_BAMBU_BUILT
      case REMOVE_ENDING_IF:
#endif
#if HAVE_ZEBU_BUILT
      case REVERSE_RESTRICT_COMPUTATION:
#endif
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
      case SDC_CODE_MOTION:
#endif
#if HAVE_BAMBU_BUILT
      case SERIALIZE_MUTUAL_EXCLUSIONS:
#endif
#if HAVE_ZEBU_BUILT
      case SHORT_CIRCUIT_STRUCTURING:
#endif
#if HAVE_BAMBU_BUILT
      case SPLIT_RETURN:
      case SHORT_CIRCUIT_TAF:
      case SIMPLE_CODE_MOTION:
      case SOFT_FLOAT_CG_EXT:
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case SPECULATION_EDGES_COMPUTATION:
#endif
#if HAVE_ZEBU_BUILT
      case SPLIT_PHINODES:
#endif
      case SWITCH_FIX:
#if HAVE_BAMBU_BUILT
      case UN_COMPARISON_LOWERING:
#endif
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
      case UNROLL_LOOPS:
#endif
#if HAVE_RTL_BUILT && HAVE_ZEBU_BUILT
      case UPDATE_RTL_WEIGHT:
#endif
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
      case UPDATE_SCHEDULE:
#endif
#if HAVE_ZEBU_BUILT
      case UPDATE_TREE_WEIGHT:
#endif
#if HAVE_BAMBU_BUILT
      case UNROLLING_DEGREE:
#endif
      case USE_COUNTING:
      case VAR_ANALYSIS:
      case VAR_DECL_FIX:
#if HAVE_BAMBU_BUILT
      case VECTORIZE:
#endif
#if HAVE_BAMBU_BUILT
      case VERIFICATION_OPERATION:
#endif
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
#if HAVE_BAMBU_BUILT
      case VIRTUAL_PHI_NODES_SPLIT:
#endif
      {
         return SymbolicApplicationFrontendFlowStep::ComputeSignature(frontend_flow_step_type);
      }
#if HAVE_BAMBU_BUILT
      case(BAMBU_FRONTEND_FLOW):
      case BIT_VALUE_IPA:
#endif
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
#endif
      case(COMPLETE_CALL_GRAPH):
#if HAVE_TASTE
      case CREATE_ADDRESS_TRANSLATION:
#endif
      case(CREATE_TREE_MANAGER):
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case DYNAMIC_VAR_COMPUTATION:
#endif
#if HAVE_BAMBU_BUILT
      case FIND_MAX_CFG_TRANSFORMATIONS:
      case FUNCTION_PARM_MASK:
#endif
      case(FUNCTION_ANALYSIS):
#if HAVE_ZEBU_BUILT
      case(FUNCTION_POINTER_CALLGRAPH_COMPUTATION):
#endif
#if HAVE_BAMBU_BUILT
      case HDL_FUNCTION_DECL_FIX:
#endif
#if HAVE_HOST_PROFILING_BUILT
      case(HOST_PROFILING):
#endif
#if HAVE_BAMBU_BUILT
      case(IPA_POINT_TO_ANALYSIS):
#endif
      case PARM2SSA:
#if HAVE_ZEBU_BUILT
      case POINTED_DATA_EVALUATION:
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case(PRAGMA_ANALYSIS):
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case(PRAGMA_SUBSTITUTION):
#endif
#if HAVE_BAMBU_BUILT
      case RANGE_ANALYSIS:
#endif
#if HAVE_ZEBU_BUILT
      case(SIZEOF_SUBSTITUTION):
#endif
#if HAVE_ZEBU_BUILT
      case(SOURCE_CODE_STATISTICS):
#endif
      case(STRING_CST_FIX):
      case(SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP):
      {
         return "Frontend::" + boost::lexical_cast<std::string>(frontend_flow_step_type);
      }

      default:
         THROW_UNREACHABLE("Frontend flow step type does not exist");
   }
   return "";
}

const std::string ApplicationFrontendFlowStep::GetSignature() const
{
   return ComputeSignature(frontend_flow_step_type);
}

const std::string ApplicationFrontendFlowStep::GetName() const
{
   return "Frontend::" + GetKindText();
}

bool ApplicationFrontendFlowStep::HasToBeExecuted() const
{
   return true;
}
