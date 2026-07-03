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
 * @file frontend_flow_step_factory.hpp
 * @brief This class contains the methods to create a frontend flow step
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#include "frontend_flow_step_factory.hpp"

#include "AddArtificialCallFlowEdges.hpp"
#include "BitValueIPA.hpp"
#include "BitValueRange.hpp"
#include "Bit_Value.hpp"
#include "Bit_Value_opt.hpp"
#include "CSE.hpp"
#include "DCE.hpp"
#include "FixStructsPassedByValue.hpp"
#include "FixVdef.hpp"
#include "FunctionCallOpt.hpp"
#include "FunctionCallTypeCleanup.hpp"
#include "HWCallInjection.hpp"
#include "IR_lowering.hpp"
#include "InterfaceInfer.hpp"
#include "NI_SSA_liveness.hpp"
#include "OMPCGExt.hpp"
#include "OMPLowering.hpp"
#include "Range_Analysis.hpp"
#include "add_op_exit_flow_edges.hpp"
#include "bambu_frontend_flow.hpp"
#include "basic_blocks_cfg_computation.hpp"
#include "bb_cdg_computation.hpp"
#include "bb_feedback_edges_computation.hpp"
#include "bb_order_computation.hpp"
#include "block_fix.hpp"
#include "build_virtual_phi.hpp"
#include "call_graph_builtin_call.hpp"
#include "call_graph_computation.hpp"
#include "call_node_fix.hpp"
#include "check_system_type.hpp"
#include "commutative_expr_restructuring.hpp"
#include "complete_bb_graph.hpp"
#include "complete_call_graph.hpp"
#include "create_ir_manager.hpp"
#include "dataflow_cg_ext.hpp"
#include "dead_code_eliminationIPA.hpp"
#include "design_flow_step.hpp"
#include "determine_memory_accesses.hpp"
#include "dom_post_dom_computation.hpp"
#include "exceptions.hpp"
#include "extract_cond_op.hpp"
#include "extract_patterns.hpp"
#include "fanout_opt.hpp"
#include "find_max_transformations.hpp"
#include "ir2fun.hpp"
#include "loops_computation.hpp"
#include "mul_decomposition.hpp"
#include "multi_way_if.hpp"
#include "op_cdg_computation.hpp"
#include "op_feedback_edges_computation.hpp"
#include "op_order_computation.hpp"
#include "operations_cfg_computation.hpp"
#include "parm2ssa.hpp"
#include "parm_decl_taken_address_fix.hpp"
#include "phi_opt.hpp"
#include "predicate_statements.hpp"
#include "rebuild_initialization2.hpp"
#include "scalar_ssa_data_dependence_computation.hpp"
#include "sdc_code_motion.hpp"
#include "select_tree_balancing.hpp"
#include "simple_code_motion.hpp"
#include "soft_float_cg_ext.hpp"
#include "soft_int_cg_ext.hpp"
#include "symbolic_application_frontend_flow_step.hpp"
#include "use_counting.hpp"
#include "var_computation.hpp"
#include "var_decl_fix.hpp"
#include "virtual_aggregate_data_flow_analysis.hpp"

#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

#if HAVE_HOST_PROFILING_BUILT
#include "basic_blocks_profiling.hpp"
#include "host_profiling.hpp"
#endif

#include "update_schedule.hpp"

FrontendFlowStepFactory::FrontendFlowStepFactory(const application_managerRef _AppM,
                                                 const DesignFlowManager& _design_flow_manager,
                                                 const ParameterConstRef _parameters)
    : DesignFlowStepFactory(DesignFlowStep::FRONTEND, _design_flow_manager, _parameters), AppM(_AppM)
{
}

DesignFlowStepSet FrontendFlowStepFactory::GenerateFrontendSteps(
    const CustomUnorderedSet<FrontendFlowStepType>& frontend_flow_step_types) const
{
   DesignFlowStepSet frontend_flow_steps;
   CustomUnorderedSet<FrontendFlowStepType>::const_iterator frontend_flow_step_type,
       frontend_flow_step_type_end = frontend_flow_step_types.end();
   for(frontend_flow_step_type = frontend_flow_step_types.begin();
       frontend_flow_step_type != frontend_flow_step_type_end; ++frontend_flow_step_type)
   {
      frontend_flow_steps.insert(GenerateFrontendStep(*frontend_flow_step_type));
   }
   return frontend_flow_steps;
}

DesignFlowStepRef FrontendFlowStepFactory::GenerateFrontendStep(FrontendFlowStepType frontend_flow_step_type) const
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
      case FUNCTION_CALL_TYPE_CLEANUP:
      case FUNCTION_CALL_OPT:
      case FIX_STRUCTS_PASSED_BY_VALUE:
      case FIX_VDEF:
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
      case SDC_CODE_MOTION:
      case SIMPLE_CODE_MOTION:
      case SOFT_FLOAT_CG_EXT:
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      case IR2FUN:
      case UPDATE_SCHEDULE:
      case UNROLLING_DEGREE:
      case USE_COUNTING:
      case VAR_ANALYSIS:
      case VAR_DECL_FIX:
      case VERIFICATION_OPERATION:
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(
             new SymbolicApplicationFrontendFlowStep(AppM, frontend_flow_step_type, design_flow_manager, parameters));
      }
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
#endif
      case BAMBU_FRONTEND_FLOW:
      case BIT_VALUE_IPA:
      case INTERFACE_INFER:
      case(COMPLETE_CALL_GRAPH):
      case(CREATE_IR_MANAGER):
      case DEAD_CODE_ELIMINATION_IPA:
      case FIND_MAX_TRANSFORMATIONS:
      case FUNCTION_ANALYSIS:
#if HAVE_HOST_PROFILING_BUILT
      case HOST_PROFILING:
#endif
      case MUL_DECOMPOSITION:
      case RANGE_ANALYSIS:
      case SOFT_INT_CG_EXT:
      case(SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP):
      {
         return CreateApplicationFrontendFlowStep(frontend_flow_step_type);
      }
      default:
         THROW_UNREACHABLE("Frontend flow step type does not exist");
   }
   return DesignFlowStepRef();
}

DesignFlowStepRef
FrontendFlowStepFactory::CreateApplicationFrontendFlowStep(const FrontendFlowStepType design_flow_step_type) const
{
   switch(design_flow_step_type)
   {
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
      {
         return DesignFlowStepRef(new BasicBlocksProfiling(AppM, design_flow_manager, parameters));
      }
#endif
      case BAMBU_FRONTEND_FLOW:
      {
         return DesignFlowStepRef(new BambuFrontendFlow(AppM, design_flow_manager, parameters));
      }
      case BIT_VALUE_IPA:
      {
         return DesignFlowStepRef(new BitValueIPA(AppM, design_flow_manager, parameters));
      }
      case INTERFACE_INFER:
      {
         return DesignFlowStepRef(new InterfaceInfer(AppM, design_flow_manager, parameters));
      }
      case(COMPLETE_CALL_GRAPH):
      {
         return DesignFlowStepRef(new CompleteCallGraph(AppM, design_flow_manager, parameters));
      }
      case(CREATE_IR_MANAGER):
      {
         return DesignFlowStepRef(new create_ir_manager(parameters, AppM, design_flow_manager));
      }
      case DEAD_CODE_ELIMINATION_IPA:
      {
         return DesignFlowStepRef(new dead_code_eliminationIPA(AppM, design_flow_manager, parameters));
      }
      case FIND_MAX_TRANSFORMATIONS:
      {
         return DesignFlowStepRef(new FindMaxTransformations(AppM, design_flow_manager, parameters));
      }
      case(FUNCTION_ANALYSIS):
      {
         return DesignFlowStepRef(new call_graph_computation(parameters, AppM, design_flow_manager));
      }
#if HAVE_HOST_PROFILING_BUILT
      case(HOST_PROFILING):
      {
         return DesignFlowStepRef(new HostProfiling(AppM, design_flow_manager, parameters));
      }
#endif
      case MUL_DECOMPOSITION:
      {
         return DesignFlowStepRef(new mul_decomposition(AppM, design_flow_manager, parameters));
      }
      case RANGE_ANALYSIS:
      {
         return DesignFlowStepRef(new RangeAnalysis(AppM, design_flow_manager, parameters));
      }
      case SOFT_INT_CG_EXT:
      {
         return DesignFlowStepRef(new soft_int_cg_ext(AppM, design_flow_manager, parameters));
      }
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
      case FUNCTION_CALL_TYPE_CLEANUP:
      case FUNCTION_CALL_OPT:
      case FIX_STRUCTS_PASSED_BY_VALUE:
      case FIX_VDEF:
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
      case SDC_CODE_MOTION:
      case SIMPLE_CODE_MOTION:
      case SOFT_FLOAT_CG_EXT:
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      case IR2FUN:
      case UNROLLING_DEGREE:
      case UPDATE_SCHEDULE:
      case USE_COUNTING:
      case VAR_ANALYSIS:
      case VAR_DECL_FIX:
      case VERIFICATION_OPERATION:
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
      {
         THROW_UNREACHABLE("Trying to create an application flow step from " +
                           FrontendFlowStep::EnumToKindText(design_flow_step_type));
         break;
      }
      case SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP:
      {
         THROW_UNREACHABLE("Symbolic Application Frontend Flow Step must be created by GenerateFrontendSteps");
         break;
      }
      default:
         THROW_UNREACHABLE("Frontend flow step type does not exist");
   }
   return DesignFlowStepRef();
}

DesignFlowStepRef
FrontendFlowStepFactory::CreateFunctionFrontendFlowStep(const FrontendFlowStepType design_flow_step_type,
                                                        const unsigned int function_id) const
{
   switch(design_flow_step_type)
   {
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
      {
         return DesignFlowStepRef(new AddArtificialCallFlowEdges(AppM, function_id, design_flow_manager, parameters));
      }
      case ADD_OP_EXIT_FLOW_EDGES:
      {
         return DesignFlowStepRef(new AddOpExitFlowEdges(parameters, AppM, function_id, design_flow_manager));
      }
      case BASIC_BLOCKS_CFG_COMPUTATION:
      {
         return DesignFlowStepRef(new BasicBlocksCfgComputation(parameters, AppM, function_id, design_flow_manager));
      }
      case BB_CONTROL_DEPENDENCE_COMPUTATION:
      {
         return DesignFlowStepRef(new BBCdgComputation(parameters, AppM, function_id, design_flow_manager));
      }
      case BB_FEEDBACK_EDGES_IDENTIFICATION:
      {
         return DesignFlowStepRef(
             new bb_feedback_edges_computation(parameters, AppM, function_id, design_flow_manager));
      }
      case BB_ORDER_COMPUTATION:
      {
         return DesignFlowStepRef(new BBOrderComputation(parameters, AppM, function_id, design_flow_manager));
      }
      case BIT_VALUE:
      {
         return DesignFlowStepRef(new Bit_Value(parameters, AppM, function_id, design_flow_manager));
      }
      case BIT_VALUE_OPT:
      {
         return DesignFlowStepRef(new Bit_Value_opt(parameters, AppM, function_id, design_flow_manager));
      }
      case BITVALUE_RANGE:
      {
         return DesignFlowStepRef(new BitValueRange(parameters, AppM, function_id, design_flow_manager));
      }
      case BLOCK_FIX:
      {
         return DesignFlowStepRef(new BlockFix(AppM, function_id, design_flow_manager, parameters));
      }
      case BUILD_VIRTUAL_PHI:
      {
         return DesignFlowStepRef(new BuildVirtualPhi(AppM, function_id, design_flow_manager, parameters));
      }
      case CALL_NODE_FIX:
      {
         return DesignFlowStepRef(new call_node_fix(AppM, function_id, design_flow_manager, parameters));
      }
      case CALL_GRAPH_BUILTIN_CALL:
      {
         return DesignFlowStepRef(new CallGraphBuiltinCall(AppM, function_id, design_flow_manager, parameters));
      }
      case CHECK_SYSTEM_TYPE:
      {
         return DesignFlowStepRef(new CheckSystemType(parameters, AppM, function_id, design_flow_manager));
      }
      case COMPLETE_BB_GRAPH:
      {
         return DesignFlowStepRef(new CompleteBBGraph(AppM, function_id, design_flow_manager, parameters));
      }
      case COMMUTATIVE_EXPR_RESTRUCTURING:
      {
         return DesignFlowStepRef(
             new commutative_expr_restructuring(AppM, function_id, design_flow_manager, parameters));
      }
      case SELECT_TREE_BALANCING:
      {
         return DesignFlowStepRef(new SelectTreeBalancing(AppM, function_id, design_flow_manager, parameters));
      }
      case CSE_STEP:
      {
         return DesignFlowStepRef(new CSE(parameters, AppM, function_id, design_flow_manager));
      }
      case DATAFLOW_CG_EXT:
      {
         return DesignFlowStepRef(new dataflow_cg_ext(parameters, AppM, function_id, design_flow_manager));
      }
      case DCE_PASS:
      {
         return DesignFlowStepRef(new DCE(parameters, AppM, function_id, design_flow_manager));
      }
      case DETERMINE_MEMORY_ACCESSES:
      {
         return DesignFlowStepRef(new determine_memory_accesses(parameters, AppM, function_id, design_flow_manager));
      }
      case DOM_POST_DOM_COMPUTATION:
      {
         return DesignFlowStepRef(new dom_post_dom_computation(parameters, AppM, function_id, design_flow_manager));
      }
      case(FANOUT_OPT):
      {
         return DesignFlowStepRef(new fanout_opt(parameters, AppM, function_id, design_flow_manager));
      }
      case EXTRACT_COND_OP:
      {
         return DesignFlowStepRef(new ExtractCondOp(AppM, design_flow_manager, function_id, parameters));
      }
      case EXTRACT_PATTERNS:
      {
         return DesignFlowStepRef(new extract_patterns(parameters, AppM, function_id, design_flow_manager));
      }
      case FIX_STRUCTS_PASSED_BY_VALUE:
      {
         return DesignFlowStepRef(new FixStructsPassedByValue(parameters, AppM, function_id, design_flow_manager));
      }
      case FIX_VDEF:
      {
         return DesignFlowStepRef(new FixVdef(parameters, AppM, function_id, design_flow_manager));
      }
      case FUNCTION_CALL_TYPE_CLEANUP:
      {
         return DesignFlowStepRef(new FunctionCallTypeCleanup(parameters, AppM, function_id, design_flow_manager));
      }
      case FUNCTION_CALL_OPT:
      {
         return DesignFlowStepRef(new FunctionCallOpt(parameters, AppM, function_id, design_flow_manager));
      }
      case HWCALL_INJECTION:
      {
         return DesignFlowStepRef(new HWCallInjection(parameters, AppM, function_id, design_flow_manager));
      }
      case IR_LOWERING:
      {
         return DesignFlowStepRef(new IR_lowering(parameters, AppM, function_id, design_flow_manager));
      }
      case LOOP_COMPUTATION:
      {
         return DesignFlowStepRef(new loops_computation(parameters, AppM, function_id, design_flow_manager));
      }
      case LOOPS_COMPUTATION:
      {
         return DesignFlowStepRef(new loops_computation(parameters, AppM, function_id, design_flow_manager));
      }
      case MULTI_WAY_IF:
      {
         return DesignFlowStepRef(new multi_way_if(parameters, AppM, function_id, design_flow_manager));
      }
      case NI_SSA_LIVENESS:
      {
         return DesignFlowStepRef(new NI_SSA_liveness(parameters, AppM, function_id, design_flow_manager));
      }
      case OMP_CG_EXT:
      {
         return DesignFlowStepRef(new OMPCGExt(parameters, AppM, function_id, design_flow_manager));
      }
      case OMP_LOWERING:
      {
         return DesignFlowStepRef(new OMPLowering(parameters, AppM, function_id, design_flow_manager));
      }
      case OP_CONTROL_DEPENDENCE_COMPUTATION:
      {
         return DesignFlowStepRef(new OpCdgComputation(parameters, AppM, function_id, design_flow_manager));
      }
      case OP_FEEDBACK_EDGES_IDENTIFICATION:
      {
         return DesignFlowStepRef(
             new op_feedback_edges_computation(parameters, AppM, function_id, design_flow_manager));
      }
      case OP_ORDER_COMPUTATION:
      {
         return DesignFlowStepRef(new OpOrderComputation(parameters, AppM, function_id, design_flow_manager));
      }
      case OPERATIONS_CFG_COMPUTATION:
      {
         return DesignFlowStepRef(new operations_cfg_computation(parameters, AppM, function_id, design_flow_manager));
      }
      case PARM2SSA:
      {
         return DesignFlowStepRef(new parm2ssa(parameters, AppM, function_id, design_flow_manager));
      }
      case PARM_DECL_TAKEN_ADDRESS:
      {
         return DesignFlowStepRef(new parm_decl_taken_address_fix(parameters, AppM, function_id, design_flow_manager));
      }
      case PHI_OPT:
      {
         return DesignFlowStepRef(new PhiOpt(AppM, function_id, design_flow_manager, parameters));
      }
      case PREDICATE_STATEMENTS:
      {
         return DesignFlowStepRef(new PredicateStatements(AppM, function_id, design_flow_manager, parameters));
      }
      case REBUILD_INITIALIZATION2:
      {
         return DesignFlowStepRef(new rebuild_initialization2(parameters, AppM, function_id, design_flow_manager));
      }
      case SDC_CODE_MOTION:
      {
         return DesignFlowStepRef(new SDCCodeMotion(AppM, function_id, design_flow_manager, parameters));
      }
      case SIMPLE_CODE_MOTION:
      {
         return DesignFlowStepRef(new simple_code_motion(parameters, AppM, function_id, design_flow_manager));
      }
      case SOFT_FLOAT_CG_EXT:
      {
         return DesignFlowStepRef(new soft_float_cg_ext(parameters, AppM, function_id, design_flow_manager));
      }
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(
             new ScalarSsaDataDependenceComputation(parameters, AppM, function_id, design_flow_manager));
      }
      case IR2FUN:
      {
         return DesignFlowStepRef(new ir2fun(parameters, AppM, function_id, design_flow_manager));
      }
      case UNROLLING_DEGREE:
      {
         THROW_UNREACHABLE("Not updated step");
         break;
      }
      case UPDATE_SCHEDULE:
      {
         return DesignFlowStepRef(new UpdateSchedule(AppM, function_id, design_flow_manager, parameters));
      }
      case USE_COUNTING:
      {
         return DesignFlowStepRef(new use_counting(parameters, AppM, function_id, design_flow_manager));
      }
      case VAR_ANALYSIS:
      {
         return DesignFlowStepRef(new VarComputation(parameters, AppM, function_id, design_flow_manager));
      }
      case VAR_DECL_FIX:
      {
         return DesignFlowStepRef(new VarDeclFix(AppM, function_id, design_flow_manager, parameters));
      }
      case VERIFICATION_OPERATION:
      {
         THROW_UNREACHABLE("Not updated step");
         break;
      }
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(
             new VirtualAggregateDataFlowAnalysis(AppM, design_flow_manager, function_id, parameters));
      }
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
#endif
      case BAMBU_FRONTEND_FLOW:
      case BIT_VALUE_IPA:
      case INTERFACE_INFER:
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
         THROW_UNREACHABLE("Trying to create a function frontend flow step from " +
                           FrontendFlowStep::EnumToKindText(design_flow_step_type));
         break;
      }
      default:
         THROW_UNREACHABLE("Frontend flow step type does not exist");
   }
   return DesignFlowStepRef();
}
