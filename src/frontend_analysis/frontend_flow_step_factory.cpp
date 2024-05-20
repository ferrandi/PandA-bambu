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
 * @file frontend_flow_step_factory.hpp
 * @brief This class contains the methods to create a frontend flow step
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "frontend_flow_step_factory.hpp"

#include "AddArtificialCallFlowEdges.hpp"
#include "add_bb_ecfg_edges.hpp"
#include "add_op_exit_flow_edges.hpp"
#include "add_op_loop_flow_edges.hpp"
#include "add_op_phi_flow_edges.hpp"
#include "bambu_frontend_flow.hpp"
#include "basic_blocks_cfg_computation.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_ILP_BUILT.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"
#include "config_HAVE_TASTE.hpp"
#include "design_flow_step.hpp"
#include "exceptions.hpp"
#if HAVE_HOST_PROFILING_BUILT
#include "basic_blocks_profiling.hpp"
#endif
#include "BitValueIPA.hpp"
#include "BitValueRange.hpp"
#include "Bit_Value.hpp"
#include "Bit_Value_opt.hpp"
#include "CSE.hpp"
#include "bb_cdg_computation.hpp"
#include "bb_feedback_edges_computation.hpp"
#include "bb_order_computation.hpp"
#include "bb_reachability_computation.hpp"
#include "block_fix.hpp"
#include "build_virtual_phi.hpp"
#include "call_expr_fix.hpp"
#include "call_graph_builtin_call.hpp"
#include "call_graph_computation.hpp"
#include "check_system_type.hpp"
#include "commutative_expr_restructuring.hpp"
#include "complete_bb_graph.hpp"
#include "complete_call_graph.hpp"
#include "compute_implicit_calls.hpp"
#include "cond_expr_restructuring.hpp"
#if HAVE_TASTE
#include "create_address_translation.hpp"
#endif
#include "create_tree_manager.hpp"
#include "dataflow_cg_ext.hpp"
#include "dead_code_elimination.hpp"
#include "dead_code_eliminationIPA.hpp"
#include "determine_memory_accesses.hpp"
#include "dom_post_dom_computation.hpp"
#include "find_max_transformations.hpp"
#if HAVE_FROM_PRAGMA_BUILT
#include "extract_omp_atomic.hpp"
#include "extract_omp_for.hpp"
#endif
#include "FixStructsPassedByValue.hpp"
#include "FixVdef.hpp"
#include "FunctionCallOpt.hpp"
#include "FunctionCallTypeCleanup.hpp"
#include "HWCallInjection.hpp"
#include "IR_lowering.hpp"
#include "InterfaceInfer.hpp"
#include "NI_SSA_liveness.hpp"
#include "extract_gimple_cond_op.hpp"
#include "extract_patterns.hpp"
#include "fanout_opt.hpp"
#include "hdl_function_decl_fix.hpp"
#include "hdl_var_decl_fix.hpp"
#include "loops_analysis_bambu.hpp"
#include "loops_computation.hpp"
#include "lut_transformation.hpp"
#include "mult_expr_fracturing.hpp"
#include "multi_way_if.hpp"
#include "multiple_entry_if_reduction.hpp"
#include "op_cdg_computation.hpp"
#include "op_feedback_edges_computation.hpp"
#include "op_order_computation.hpp"
#include "op_reachability_computation.hpp"
#include "operations_cfg_computation.hpp"
#include "parm2ssa.hpp"
#include "parm_decl_taken_address_fix.hpp"
#include "phi_opt.hpp"
#include "soft_int_cg_ext.hpp"
#if HAVE_FROM_PRAGMA_BUILT
#include "pragma_substitution.hpp"
#endif
#include "predicate_statements.hpp"
#if HAVE_HOST_PROFILING_BUILT
#include "host_profiling.hpp"
#endif
#include "Range_Analysis.hpp"
#include "eSSA.hpp"
#include "rebuild_initializations.hpp"
#include "remove_clobber_ga.hpp"
#include "remove_ending_if.hpp"
#include "scalar_ssa_data_dependence_computation.hpp"
#include "sdc_code_motion.hpp"
#include "serialize_mutual_exclusions.hpp"
#include "short_circuit_taf.hpp"
#include "simple_code_motion.hpp"
#include "soft_float_cg_ext.hpp"
#include "split_return.hpp"
#include "string_cst_fix.hpp"
#include "switch_fix.hpp"
#include "tree2fun.hpp"
#include "un_comparison_lowering.hpp"
#if HAVE_ILP_BUILT
#include "update_schedule.hpp"
#endif
#include "use_counting.hpp"
#include "var_computation.hpp"
#include "var_decl_fix.hpp"
#include "vectorize.hpp"
#include "virtual_aggregate_data_flow_analysis.hpp"
#include "virtual_phi_nodes_split.hpp"
#if HAVE_FROM_PRAGMA_BUILT
#include "pragma_analysis.hpp"
#endif
#include "symbolic_application_frontend_flow_step.hpp"

FrontendFlowStepFactory::FrontendFlowStepFactory(const application_managerRef _AppM,
                                                 const DesignFlowManagerConstRef _design_flow_manager,
                                                 const ParameterConstRef _parameters)
    : DesignFlowStepFactory(DesignFlowStep::FRONTEND, _design_flow_manager, _parameters), AppM(_AppM)
{
}

FrontendFlowStepFactory::~FrontendFlowStepFactory() = default;

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
      case FUNCTION_CALL_TYPE_CLEANUP:
      case FUNCTION_CALL_OPT:
      case FIX_STRUCTS_PASSED_BY_VALUE:
      case FIX_VDEF:
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
#if HAVE_ILP_BUILT
      case SDC_CODE_MOTION:
#endif
      case SERIALIZE_MUTUAL_EXCLUSIONS:
      case SPLIT_RETURN:
      case SHORT_CIRCUIT_TAF:
      case SIMPLE_CODE_MOTION:
      case SOFT_FLOAT_CG_EXT:
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      case SWITCH_FIX:
      case TREE2FUN:
#if HAVE_ILP_BUILT
      case UPDATE_SCHEDULE:
#endif
      case UN_COMPARISON_LOWERING:
      case UNROLLING_DEGREE:
      case USE_COUNTING:
      case VAR_ANALYSIS:
      case VAR_DECL_FIX:
      case VECTORIZE:
      case VERIFICATION_OPERATION:
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
      case VIRTUAL_PHI_NODES_SPLIT:
      {
         return DesignFlowStepRef(new SymbolicApplicationFrontendFlowStep(AppM, frontend_flow_step_type,
                                                                          design_flow_manager.lock(), parameters));
      }
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
#endif
      case BAMBU_FRONTEND_FLOW:
      case BIT_VALUE_IPA:
      case INTERFACE_INFER:
      case(COMPLETE_CALL_GRAPH):
#if HAVE_TASTE
      case CREATE_ADDRESS_TRANSLATION:
#endif
      case(CREATE_TREE_MANAGER):
      case DEAD_CODE_ELIMINATION_IPA:
      case FIND_MAX_TRANSFORMATIONS:
      case FUNCTION_ANALYSIS:
      case HDL_FUNCTION_DECL_FIX:
#if HAVE_HOST_PROFILING_BUILT
      case HOST_PROFILING:
#endif
      case MULT_EXPR_FRACTURING:
#if HAVE_FROM_PRAGMA_BUILT
      case PRAGMA_ANALYSIS:
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case PRAGMA_SUBSTITUTION:
#endif
      case RANGE_ANALYSIS:
      case SOFT_INT_CG_EXT:
      case STRING_CST_FIX:
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
         return DesignFlowStepRef(new BasicBlocksProfiling(AppM, design_flow_manager.lock(), parameters));
      }
#endif
      case BAMBU_FRONTEND_FLOW:
      {
         return DesignFlowStepRef(new BambuFrontendFlow(AppM, design_flow_manager.lock(), parameters));
      }
      case BIT_VALUE_IPA:
      {
         return DesignFlowStepRef(new BitValueIPA(AppM, design_flow_manager.lock(), parameters));
      }
      case INTERFACE_INFER:
      {
         return DesignFlowStepRef(new InterfaceInfer(AppM, design_flow_manager.lock(), parameters));
      }
      case(COMPLETE_CALL_GRAPH):
      {
         return DesignFlowStepRef(new CompleteCallGraph(AppM, design_flow_manager.lock(), parameters));
      }
#if HAVE_TASTE
      case CREATE_ADDRESS_TRANSLATION:
      {
         return DesignFlowStepRef(new CreateAddressTranslation(AppM, design_flow_manager.lock(), parameters));
      }
#endif
      case(CREATE_TREE_MANAGER):
      {
         return DesignFlowStepRef(new create_tree_manager(parameters, AppM, design_flow_manager.lock()));
      }
      case DEAD_CODE_ELIMINATION_IPA:
      {
         return DesignFlowStepRef(new dead_code_eliminationIPA(AppM, design_flow_manager.lock(), parameters));
      }
      case FIND_MAX_TRANSFORMATIONS:
      {
         return DesignFlowStepRef(new FindMaxTransformations(AppM, design_flow_manager.lock(), parameters));
      }
      case(FUNCTION_ANALYSIS):
      {
         return DesignFlowStepRef(new call_graph_computation(parameters, AppM, design_flow_manager.lock()));
      }
      case HDL_FUNCTION_DECL_FIX:
      {
         return DesignFlowStepRef(new HDLFunctionDeclFix(AppM, design_flow_manager.lock(), parameters));
      }
#if HAVE_HOST_PROFILING_BUILT
      case(HOST_PROFILING):
      {
         return DesignFlowStepRef(new HostProfiling(AppM, design_flow_manager.lock(), parameters));
      }
#endif
      case MULT_EXPR_FRACTURING:
      {
         return DesignFlowStepRef(new mult_expr_fracturing(AppM, design_flow_manager.lock(), parameters));
      }
#if HAVE_PRAGMA_BUILT
      case PRAGMA_ANALYSIS:
      {
         return DesignFlowStepRef(new PragmaAnalysis(AppM, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case(PRAGMA_SUBSTITUTION):
      {
         return DesignFlowStepRef(new PragmaSubstitution(AppM, design_flow_manager.lock(), parameters));
      }
#endif
      case RANGE_ANALYSIS:
      {
         return DesignFlowStepRef(new RangeAnalysis(AppM, design_flow_manager.lock(), parameters));
      }
      case SOFT_INT_CG_EXT:
      {
         return DesignFlowStepRef(new soft_int_cg_ext(AppM, design_flow_manager.lock(), parameters));
      }
      case STRING_CST_FIX:
      {
         return DesignFlowStepRef(new string_cst_fix(AppM, design_flow_manager.lock(), parameters));
      }
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
      case FUNCTION_CALL_TYPE_CLEANUP:
      case FUNCTION_CALL_OPT:
      case FIX_STRUCTS_PASSED_BY_VALUE:
      case FIX_VDEF:
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
#if HAVE_ILP_BUILT
      case SDC_CODE_MOTION:
#endif
      case SERIALIZE_MUTUAL_EXCLUSIONS:
      case SPLIT_RETURN:
      case SHORT_CIRCUIT_TAF:
      case SIMPLE_CODE_MOTION:
      case SOFT_FLOAT_CG_EXT:
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      case SWITCH_FIX:
      case TREE2FUN:
      case UN_COMPARISON_LOWERING:
      case UNROLLING_DEGREE:
#if HAVE_ILP_BUILT
      case UPDATE_SCHEDULE:
#endif
      case USE_COUNTING:
      case VAR_ANALYSIS:
      case VAR_DECL_FIX:
      case VECTORIZE:
      case VERIFICATION_OPERATION:
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
      case VIRTUAL_PHI_NODES_SPLIT:
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
      case ADD_BB_ECFG_EDGES:
      {
         return DesignFlowStepRef(new AddBbEcfgEdges(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
      {
         return DesignFlowStepRef(
             new AddArtificialCallFlowEdges(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case ADD_OP_EXIT_FLOW_EDGES:
      {
         return DesignFlowStepRef(new AddOpExitFlowEdges(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case ADD_OP_LOOP_FLOW_EDGES:
      {
         return DesignFlowStepRef(new AddOpLoopFlowEdges(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case ADD_OP_PHI_FLOW_EDGES:
      {
         return DesignFlowStepRef(new AddOpPhiFlowEdges(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case BASIC_BLOCKS_CFG_COMPUTATION:
      {
         return DesignFlowStepRef(
             new BasicBlocksCfgComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BB_CONTROL_DEPENDENCE_COMPUTATION:
      {
         return DesignFlowStepRef(new BBCdgComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BB_FEEDBACK_EDGES_IDENTIFICATION:
      {
         return DesignFlowStepRef(
             new bb_feedback_edges_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BB_ORDER_COMPUTATION:
      {
         return DesignFlowStepRef(new BBOrderComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BB_REACHABILITY_COMPUTATION:
      {
         return DesignFlowStepRef(
             new BBReachabilityComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BIT_VALUE:
      {
         return DesignFlowStepRef(new Bit_Value(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BIT_VALUE_OPT:
      {
         return DesignFlowStepRef(new Bit_Value_opt(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BITVALUE_RANGE:
      {
         return DesignFlowStepRef(new BitValueRange(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BLOCK_FIX:
      {
         return DesignFlowStepRef(new BlockFix(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case BUILD_VIRTUAL_PHI:
      {
         return DesignFlowStepRef(new BuildVirtualPhi(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case CALL_EXPR_FIX:
      {
         return DesignFlowStepRef(new call_expr_fix(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case CALL_GRAPH_BUILTIN_CALL:
      {
         return DesignFlowStepRef(new CallGraphBuiltinCall(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case CHECK_SYSTEM_TYPE:
      {
         return DesignFlowStepRef(new CheckSystemType(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case COMPLETE_BB_GRAPH:
      {
         return DesignFlowStepRef(new CompleteBBGraph(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case COMPUTE_IMPLICIT_CALLS:
      {
         return DesignFlowStepRef(
             new compute_implicit_calls(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case COMMUTATIVE_EXPR_RESTRUCTURING:
      {
         return DesignFlowStepRef(
             new commutative_expr_restructuring(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case COND_EXPR_RESTRUCTURING:
      {
         return DesignFlowStepRef(new CondExprRestructuring(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case CSE_STEP:
      {
         return DesignFlowStepRef(new CSE(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case DATAFLOW_CG_EXT:
      {
         return DesignFlowStepRef(new dataflow_cg_ext(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case DEAD_CODE_ELIMINATION:
      {
         return DesignFlowStepRef(new dead_code_elimination(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case DETERMINE_MEMORY_ACCESSES:
      {
         return DesignFlowStepRef(
             new determine_memory_accesses(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case DOM_POST_DOM_COMPUTATION:
      {
         return DesignFlowStepRef(
             new dom_post_dom_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case(ESSA):
      {
         return DesignFlowStepRef(new eSSA(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case(FANOUT_OPT):
      {
         return DesignFlowStepRef(new fanout_opt(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case(MULTIPLE_ENTRY_IF_REDUCTION):
      {
         return DesignFlowStepRef(
             new MultipleEntryIfReduction(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case EXTRACT_GIMPLE_COND_OP:
      {
         return DesignFlowStepRef(new ExtractGimpleCondOp(AppM, design_flow_manager.lock(), function_id, parameters));
      }
#if HAVE_FROM_PRAGMA_BUILT
      case EXTRACT_OMP_ATOMIC:
      {
         return DesignFlowStepRef(new ExtractOmpAtomic(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case EXTRACT_OMP_FOR:
      {
         return DesignFlowStepRef(new ExtractOmpFor(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
      case EXTRACT_PATTERNS:
      {
         return DesignFlowStepRef(new extract_patterns(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case FIX_STRUCTS_PASSED_BY_VALUE:
      {
         return DesignFlowStepRef(
             new FixStructsPassedByValue(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case FIX_VDEF:
      {
         return DesignFlowStepRef(new FixVdef(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case FUNCTION_CALL_TYPE_CLEANUP:
      {
         return DesignFlowStepRef(
             new FunctionCallTypeCleanup(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case FUNCTION_CALL_OPT:
      {
         return DesignFlowStepRef(new FunctionCallOpt(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case HDL_VAR_DECL_FIX:
      {
         return DesignFlowStepRef(new HDLVarDeclFix(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case HWCALL_INJECTION:
      {
         return DesignFlowStepRef(new HWCallInjection(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case IR_LOWERING:
      {
         return DesignFlowStepRef(new IR_lowering(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case LOOP_COMPUTATION:
      {
         return DesignFlowStepRef(new loops_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case LOOPS_ANALYSIS_BAMBU:
      {
         return DesignFlowStepRef(new LoopsAnalysisBambu(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case LOOPS_COMPUTATION:
      {
         return DesignFlowStepRef(new loops_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case LUT_TRANSFORMATION:
      {
         return DesignFlowStepRef(new lut_transformation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case MULTI_WAY_IF:
      {
         return DesignFlowStepRef(new multi_way_if(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case NI_SSA_LIVENESS:
      {
         return DesignFlowStepRef(new NI_SSA_liveness(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case OP_CONTROL_DEPENDENCE_COMPUTATION:
      {
         return DesignFlowStepRef(new OpCdgComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case OP_FEEDBACK_EDGES_IDENTIFICATION:
      {
         return DesignFlowStepRef(
             new op_feedback_edges_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case OP_ORDER_COMPUTATION:
      {
         return DesignFlowStepRef(new OpOrderComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case OP_REACHABILITY_COMPUTATION:
      {
         return DesignFlowStepRef(
             new OpReachabilityComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case OPERATIONS_CFG_COMPUTATION:
      {
         return DesignFlowStepRef(
             new operations_cfg_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case PARM2SSA:
      {
         return DesignFlowStepRef(new parm2ssa(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case PARM_DECL_TAKEN_ADDRESS:
      {
         return DesignFlowStepRef(
             new parm_decl_taken_address_fix(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case PHI_OPT:
      {
         return DesignFlowStepRef(new PhiOpt(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case PREDICATE_STATEMENTS:
      {
         return DesignFlowStepRef(new PredicateStatements(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case REBUILD_INITIALIZATION:
      {
         return DesignFlowStepRef(
             new rebuild_initialization(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case REBUILD_INITIALIZATION2:
      {
         return DesignFlowStepRef(
             new rebuild_initialization2(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case REMOVE_CLOBBER_GA:
      {
         return DesignFlowStepRef(new remove_clobber_ga(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case REMOVE_ENDING_IF:
      {
         return DesignFlowStepRef(new RemoveEndingIf(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#if HAVE_ILP_BUILT
      case SDC_CODE_MOTION:
      {
         return DesignFlowStepRef(new SDCCodeMotion(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
      case SERIALIZE_MUTUAL_EXCLUSIONS:
      {
         return DesignFlowStepRef(
             new SerializeMutualExclusions(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case SPLIT_RETURN:
      {
         return DesignFlowStepRef(new SplitReturn(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case SHORT_CIRCUIT_TAF:
      {
         return DesignFlowStepRef(new short_circuit_taf(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case SIMPLE_CODE_MOTION:
      {
         return DesignFlowStepRef(new simple_code_motion(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case SOFT_FLOAT_CG_EXT:
      {
         return DesignFlowStepRef(new soft_float_cg_ext(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(
             new ScalarSsaDataDependenceComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case SWITCH_FIX:
      {
         return DesignFlowStepRef(new SwitchFix(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case TREE2FUN:
      {
         return DesignFlowStepRef(new tree2fun(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case UN_COMPARISON_LOWERING:
      {
         return DesignFlowStepRef(new UnComparisonLowering(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case UNROLLING_DEGREE:
      {
         THROW_UNREACHABLE("Not updated step");
         break;
      }

#if HAVE_ILP_BUILT
      case UPDATE_SCHEDULE:
      {
         return DesignFlowStepRef(new UpdateSchedule(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
      case USE_COUNTING:
      {
         return DesignFlowStepRef(new use_counting(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case VAR_ANALYSIS:
      {
         return DesignFlowStepRef(new VarComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case VAR_DECL_FIX:
      {
         return DesignFlowStepRef(new VarDeclFix(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case VECTORIZE:
      {
         return DesignFlowStepRef(new Vectorize(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case VERIFICATION_OPERATION:
      {
         THROW_UNREACHABLE("Not updated step");
         break;
      }
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(
             new VirtualAggregateDataFlowAnalysis(AppM, design_flow_manager.lock(), function_id, parameters));
      }
      case VIRTUAL_PHI_NODES_SPLIT:
      {
         return DesignFlowStepRef(
             new virtual_phi_nodes_split(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
#endif
      case BAMBU_FRONTEND_FLOW:
      case BIT_VALUE_IPA:
      case INTERFACE_INFER:
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
#if HAVE_PRAGMA_BUILT
      case(PRAGMA_ANALYSIS):
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case(PRAGMA_SUBSTITUTION):
#endif
      case RANGE_ANALYSIS:
      case SOFT_INT_CG_EXT:
      case STRING_CST_FIX:
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
