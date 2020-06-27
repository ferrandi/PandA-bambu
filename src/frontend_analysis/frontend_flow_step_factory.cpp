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

/// Autoheader include
#include "config_HAVE_ARCH_BUILT.hpp"
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_ILP_BUILT.hpp"    // for HAVE_ILP_BUILT
#include "config_HAVE_PRAGMA_BUILT.hpp" // for HAVE_PRAGMA_B...
#include "config_HAVE_RTL_BUILT.hpp"    // for HAVE_RTL_BUILT
#include "config_HAVE_TASTE.hpp"        // for HAVE_TASTE
#include "config_HAVE_ZEBU_BUILT.hpp"

#include "exceptions.hpp" // for THROW_UNREACH...

/// Passes includes
#include "add_bb_ecfg_edges.hpp"
#include "design_flow_step.hpp"
#if HAVE_BAMBU_BUILT
#include "AddArtificialCallFlowEdges.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "add_op_ecfg_edges.hpp"
#endif
#include "add_op_exit_flow_edges.hpp"
#include "add_op_loop_flow_edges.hpp"
#if HAVE_BAMBU_BUILT
#include "add_op_phi_flow_edges.hpp"
#endif
#include "aggregate_data_flow_analysis.hpp"
#if HAVE_ZEBU_BUILT
#include "array_ref_fix.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "bambu_frontend_flow.hpp"
#endif
#include "basic_blocks_cfg_computation.hpp"
#if HAVE_HOST_PROFILING_BUILT
#include "basic_blocks_profiling.hpp"
#endif
#include "bb_cdg_computation.hpp"
#include "bb_feedback_edges_computation.hpp"
#include "bb_order_computation.hpp"
#include "bb_reachability_computation.hpp"
#if HAVE_BAMBU_BUILT
#include "BitValueIPA.hpp"
#include "Bit_Value.hpp"
#include "Bit_Value_opt.hpp"
#endif
#include "block_fix.hpp"
#include "build_virtual_phi.hpp"
#if HAVE_ZEBU_BUILT
#include "call_args_structuring.hpp"
#endif
#include "call_expr_fix.hpp"
#if HAVE_BAMBU_BUILT
#include "call_graph_builtin_call.hpp"
#endif
#include "call_graph_computation.hpp"
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
#include "check_critical_session.hpp"
#endif
#if HAVE_HOST_PROFILING_BUILT && HAVE_ZEBU_BUILT
#include "check_pipelinable_loops.hpp"
#endif
#include "check_system_type.hpp"
#include "clean_virtual_phi.hpp"
#include "complete_bb_graph.hpp"
#include "complete_call_graph.hpp"
#if HAVE_BAMBU_BUILT
#include "CSE.hpp"
#include "commutative_expr_restructuring.hpp"
#include "compute_implicit_calls.hpp"
#include "cond_expr_restructuring.hpp"
#endif
#if HAVE_TASTE
#include "create_address_translation.hpp"
#endif
#include "create_tree_manager.hpp"
#if HAVE_ZEBU_BUILT || HAVE_BAMBU_BUILT
#include "dead_code_elimination.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "find_max_cfg_transformations.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "determine_memory_accesses.hpp"
#endif
#include "dom_post_dom_computation.hpp"
#if HAVE_HOST_PROFILING_BUILT && HAVE_EXPERIMENTAL
#include "dynamic_aggregate_data_flow_analysis.hpp"
#include "dynamic_var_computation.hpp"
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
#include "extended_pdg_computation.hpp"
#endif
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
#include "extract_omp_atomic.hpp"
#include "extract_omp_for.hpp"
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
#include "parallel_regions_graph_computation.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "FixStructsPassedByValue.hpp"
#include "FunctionCallTypeCleanup.hpp"
#include "extract_gimple_cond_op.hpp"
#include "extract_patterns.hpp"
#include "fanout_opt.hpp"
#include "function_parm_mask.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "FunctionPointerCallGraphComputation.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "global_variables_analysis.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "hdl_function_decl_fix.hpp"
#include "hdl_var_decl_fix.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "header_structuring.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "HWCallInjection.hpp"
#include "hls_div_cg_ext.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "IR_lowering.hpp"
#include "interface_infer.hpp"
#include "ipa_point_to_analysis.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "instruction_sequences_computation.hpp"
#include "loop_regions_computation.hpp"
#include "loop_regions_flow_computation.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "loops_analysis_bambu.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "loops_analysis_zebu.hpp"
#endif
#include "loops_computation.hpp"
#if HAVE_ZEBU_BUILT
#include "loops_rebuilding.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "lut_transformation.hpp"
#endif
#include "mem_cg_ext.hpp"
#include "memory_data_flow_analysis.hpp"
#if HAVE_BAMBU_BUILT
#include "NI_SSA_liveness.hpp"
#include "multi_way_if.hpp"
#include "multiple_entry_if_reduction.hpp" //modified here
#endif
#include "op_cdg_computation.hpp"
#include "op_feedback_edges_computation.hpp"
#include "op_order_computation.hpp"
#include "op_reachability_computation.hpp"
#include "operations_cfg_computation.hpp"
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
#include "parallel_loop_swap.hpp"
#include "parallel_loops_analysis.hpp"
#endif
#include "parm2ssa.hpp"
#if HAVE_BAMBU_BUILT
#include "parm_decl_taken_address_fix.hpp"
#include "phi_opt.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "pointed_data_computation.hpp"
#include "pointed_data_evaluation.hpp"
#endif
#if HAVE_FROM_PRAGMA_BUILT
#include "pragma_substitution.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "predicate_statements.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "predictability_analysis.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "probability_path.hpp"
#endif
#if HAVE_HOST_PROFILING_BUILT
#include "host_profiling.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "Range_Analysis.hpp"
#include "eSSA.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "rebuild_initializations.hpp"
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
#include "reduced_pdg_computation.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "refined_aggregate_data_flow_analysis.hpp"
#include "refined_var_computation.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "remove_clobber_ga.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "remove_ending_if.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "reverse_restrict_computation.hpp"
#endif
#include "scalar_ssa_data_dependence_computation.hpp"
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
#include "sdc_code_motion.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "serialize_mutual_exclusions.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "short_circuit_structuring.hpp"
#include "sizeof_substitution.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "short_circuit_taf.hpp"
#include "simple_code_motion.hpp"
#include "soft_float_cg_ext.hpp"
#include "split_return.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "source_code_statistics.hpp"
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
#include "speculation_edges_computation.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "split_phi_nodes.hpp"
#endif
#include "string_cst_fix.hpp"
#include "switch_fix.hpp"
#if HAVE_BAMBU_BUILT
#include "un_comparison_lowering.hpp"
#endif
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
#include "unroll_loops.hpp"
#endif
#if HAVE_ZEBU_BUILT && HAVE_RTL_BUILT
#include "update_rtl_weight.hpp"
#endif
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
#include "update_schedule.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "update_tree_weight.hpp"
#endif
#include "use_counting.hpp"
#include "var_computation.hpp"
#include "var_decl_fix.hpp"
#if HAVE_BAMBU_BUILT
#include "vectorize.hpp"
#endif
#include "virtual_aggregate_data_flow_analysis.hpp"
#if HAVE_BAMBU_BUILT
#include "virtual_phi_nodes_split.hpp"
#endif

#if HAVE_FROM_PRAGMA_BUILT
#include "pragma_analysis.hpp"
#endif
#include "symbolic_application_frontend_flow_step.hpp"

#if HAVE_ARCH_BUILT
FrontendFlowStepFactory::FrontendFlowStepFactory(const application_managerRef _AppM, const ArchManagerRef _arch_manager, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : DesignFlowStepFactory(_design_flow_manager, _parameters), AppM(_AppM), arch_manager(_arch_manager)
{
}
#endif

FrontendFlowStepFactory::FrontendFlowStepFactory(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters) : DesignFlowStepFactory(_design_flow_manager, _parameters), AppM(_AppM)
{
}

FrontendFlowStepFactory::~FrontendFlowStepFactory() = default;

const DesignFlowStepSet FrontendFlowStepFactory::GenerateFrontendSteps(const CustomUnorderedSet<FrontendFlowStepType>& frontend_flow_step_types) const
{
   DesignFlowStepSet frontend_flow_steps;
   CustomUnorderedSet<FrontendFlowStepType>::const_iterator frontend_flow_step_type, frontend_flow_step_type_end = frontend_flow_step_types.end();
   for(frontend_flow_step_type = frontend_flow_step_types.begin(); frontend_flow_step_type != frontend_flow_step_type_end; ++frontend_flow_step_type)
   {
      frontend_flow_steps.insert(GenerateFrontendStep(*frontend_flow_step_type));
   }
   return frontend_flow_steps;
}

const DesignFlowStepRef FrontendFlowStepFactory::GenerateFrontendStep(FrontendFlowStepType frontend_flow_step_type) const
{
   switch(frontend_flow_step_type)
   {
      case ADD_BB_ECFG_EDGES:
#if HAVE_BAMBU_BUILT
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
#endif
#if HAVE_ZEBU_BUILT
      case ADD_OP_ECFG_EDGES:
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
#if HAVE_EXPERIMENTAL && HAVE_ZEBU_BUILT
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
      case FUNCTION_CALL_TYPE_CLEANUP:
      case FIX_STRUCTS_PASSED_BY_VALUE:
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
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case REDUCED_PDG_COMPUTATION:
#endif
#if HAVE_ZEBU_BUILT
#if HAVE_EXPERIMENTAL
      case REFINED_AGGREGATE_DATA_FLOW_ANALYSIS:
      case REFINED_VAR_COMPUTATION:
#endif
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
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      case SWITCH_FIX:
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
      case UPDATE_SCHEDULE:
#endif
#if HAVE_RTL_BUILT && HAVE_ZEBU_BUILT
      case UPDATE_RTL_WEIGHT:
#endif
#if HAVE_ZEBU_BUILT
      case UPDATE_TREE_WEIGHT:
#endif
#if HAVE_BAMBU_BUILT
      case UN_COMPARISON_LOWERING:
#endif
#if HAVE_BAMBU_BUILT
      case UNROLLING_DEGREE:
#endif
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
      case UNROLL_LOOPS:
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
         return DesignFlowStepRef(new SymbolicApplicationFrontendFlowStep(AppM, frontend_flow_step_type, design_flow_manager.lock(), parameters));
      }
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
#endif
#if HAVE_BAMBU_BUILT
      case(BAMBU_FRONTEND_FLOW):
      case BIT_VALUE_IPA:
#endif
      case(COMPLETE_CALL_GRAPH):
#if HAVE_TASTE
      case CREATE_ADDRESS_TRANSLATION:
#endif
      case(CREATE_TREE_MANAGER):
#if HAVE_EXPERIMENTAL && HAVE_ZEBU_BUILT
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
#if HAVE_ZEBU_BUILT
      case POINTED_DATA_EVALUATION:
#endif
      case PARM2SSA:
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
      case(SOURCE_CODE_STATISTICS):
#endif
      case STRING_CST_FIX:
      case(SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP):
#if HAVE_ZEBU_BUILT
      case(SIZEOF_SUBSTITUTION):
#endif
      {
         return CreateApplicationFrontendFlowStep(frontend_flow_step_type);
      }
      default:
         THROW_UNREACHABLE("Frontend flow step type does not exist");
   }
   return DesignFlowStepRef();
}

const DesignFlowStepRef FrontendFlowStepFactory::CreateApplicationFrontendFlowStep(const FrontendFlowStepType design_flow_step_type) const
{
   switch(design_flow_step_type)
   {
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
      {
         return DesignFlowStepRef(new BasicBlocksProfiling(AppM, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case BAMBU_FRONTEND_FLOW:
      {
         return DesignFlowStepRef(new BambuFrontendFlow(AppM, design_flow_manager.lock(), parameters));
      }
      case BIT_VALUE_IPA:
      {
         return DesignFlowStepRef(new BitValueIPA(AppM, design_flow_manager.lock(), parameters));
      }
#endif
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
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case DYNAMIC_VAR_COMPUTATION:
      {
         return DesignFlowStepRef(new DynamicVarComputation(AppM, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case FIND_MAX_CFG_TRANSFORMATIONS:
      {
         return DesignFlowStepRef(new FindMaxCFGTransformations(AppM, design_flow_manager.lock(), parameters));
      }
      case FUNCTION_PARM_MASK:
      {
         return DesignFlowStepRef(new function_parm_mask(AppM, design_flow_manager.lock(), parameters));
      }
#endif
      case(FUNCTION_ANALYSIS):
      {
         return DesignFlowStepRef(new call_graph_computation(parameters, AppM, design_flow_manager.lock()));
      }
#if HAVE_ZEBU_BUILT
      case(FUNCTION_POINTER_CALLGRAPH_COMPUTATION):
      {
         return DesignFlowStepRef(new FunctionPointerCallGraphComputation(parameters, AppM, design_flow_manager.lock()));
      }
#endif
#if HAVE_BAMBU_BUILT
      case HDL_FUNCTION_DECL_FIX:
      {
         return DesignFlowStepRef(new HDLFunctionDeclFix(AppM, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_HOST_PROFILING_BUILT
      case(HOST_PROFILING):
      {
         return DesignFlowStepRef(new HostProfiling(AppM, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case IPA_POINT_TO_ANALYSIS:
      {
         return DesignFlowStepRef(new ipa_point_to_analysis(AppM, design_flow_manager.lock(), parameters));
      }
#endif
      case PARM2SSA:
      {
         return DesignFlowStepRef(new parm2ssa(AppM, design_flow_manager.lock(), parameters));
      }
#if HAVE_ZEBU_BUILT
      case(POINTED_DATA_EVALUATION):
      {
         return DesignFlowStepRef(new PointedDataEvaluation(AppM, design_flow_manager.lock(), parameters));
      }
#endif
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
#if HAVE_BAMBU_BUILT
      case RANGE_ANALYSIS:
      {
         return DesignFlowStepRef(new RangeAnalysis(AppM, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ZEBU_BUILT
      case(SIZEOF_SUBSTITUTION):
      {
         return DesignFlowStepRef(new SizeofSubstitution(AppM, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ZEBU_BUILT
      case(SOURCE_CODE_STATISTICS):
      {
         return DesignFlowStepRef(new SourceCodeStatistics(AppM, design_flow_manager.lock(), parameters));
      }
#endif
      case STRING_CST_FIX:
      {
         return DesignFlowStepRef(new string_cst_fix(AppM, design_flow_manager.lock(), parameters));
      }
      case ADD_BB_ECFG_EDGES:
#if HAVE_BAMBU_BUILT
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
#endif
#if HAVE_ZEBU_BUILT
      case ADD_OP_ECFG_EDGES:
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
#if HAVE_EXPERIMENTAL && HAVE_ZEBU_BUILT
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
      case FUNCTION_CALL_TYPE_CLEANUP:
      case FIX_STRUCTS_PASSED_BY_VALUE:
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
#if HAVE_EXPERIMENTAL && HAVE_ZEBU_BUILT
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
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case REDUCED_PDG_COMPUTATION:
#endif
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case REFINED_AGGREGATE_DATA_FLOW_ANALYSIS:
      case REFINED_VAR_COMPUTATION:
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
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      case SWITCH_FIX:
#if HAVE_BAMBU_BUILT
      case UN_COMPARISON_LOWERING:
#endif
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
      case UNROLL_LOOPS:
#endif
#if HAVE_BAMBU_BUILT
      case UNROLLING_DEGREE:
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
         THROW_UNREACHABLE("Trying to create an application flow step from " + FrontendFlowStep::EnumToKindText(design_flow_step_type));
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

const DesignFlowStepRef FrontendFlowStepFactory::CreateFunctionFrontendFlowStep(const FrontendFlowStepType design_flow_step_type, const unsigned int function_id) const
{
   switch(design_flow_step_type)
   {
      case ADD_BB_ECFG_EDGES:
      {
         return DesignFlowStepRef(new AddBbEcfgEdges(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#if HAVE_BAMBU_BUILT
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
      {
         return DesignFlowStepRef(new AddArtificialCallFlowEdges(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ZEBU_BUILT
      case ADD_OP_ECFG_EDGES:
      {
         return DesignFlowStepRef(new AddOpEcfgEdges(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
      case ADD_OP_EXIT_FLOW_EDGES:
      {
         return DesignFlowStepRef(new AddOpExitFlowEdges(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case ADD_OP_LOOP_FLOW_EDGES:
      {
         return DesignFlowStepRef(new AddOpLoopFlowEdges(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#if HAVE_BAMBU_BUILT
      case ADD_OP_PHI_FLOW_EDGES:
      {
         return DesignFlowStepRef(new AddOpPhiFlowEdges(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
      case AGGREGATE_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(new AggregateDataFlowAnalysis(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#if HAVE_ZEBU_BUILT
      case(ARRAY_REF_FIX):
      {
         return DesignFlowStepRef(new array_ref_fix(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
      case BASIC_BLOCKS_CFG_COMPUTATION:
      {
         return DesignFlowStepRef(new BasicBlocksCfgComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BB_CONTROL_DEPENDENCE_COMPUTATION:
      {
         return DesignFlowStepRef(new BBCdgComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BB_FEEDBACK_EDGES_IDENTIFICATION:
      {
         return DesignFlowStepRef(new bb_feedback_edges_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BB_ORDER_COMPUTATION:
      {
         return DesignFlowStepRef(new BBOrderComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BB_REACHABILITY_COMPUTATION:
      {
         return DesignFlowStepRef(new BBReachabilityComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#if HAVE_BAMBU_BUILT
      case BIT_VALUE:
      {
         return DesignFlowStepRef(new Bit_Value(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case BIT_VALUE_OPT:
      {
         return DesignFlowStepRef(new Bit_Value_opt(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
      case BLOCK_FIX:
      {
         return DesignFlowStepRef(new BlockFix(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case BUILD_VIRTUAL_PHI:
      {
         return DesignFlowStepRef(new BuildVirtualPhi(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#if HAVE_ZEBU_BUILT
      case CALL_ARGS_STRUCTURING:
      {
         return DesignFlowStepRef(new CallArgsStructuring(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
      case CALL_EXPR_FIX:
      {
         return DesignFlowStepRef(new call_expr_fix(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case CALL_GRAPH_BUILTIN_CALL:
      {
         return DesignFlowStepRef(new CallGraphBuiltinCall(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case CHECK_CRITICAL_SESSION:
      {
         return DesignFlowStepRef(new CheckCriticalSession(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ZEBU_BUILT
      case CHECK_PIPELINABLE_LOOPS:
      {
         return DesignFlowStepRef(new CheckPipelinableLoops(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
      case CHECK_SYSTEM_TYPE:
      {
         return DesignFlowStepRef(new CheckSystemType(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case CLEAN_VIRTUAL_PHI:
      {
         return DesignFlowStepRef(new CleanVirtualPhi(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case COMPLETE_BB_GRAPH:
      {
         return DesignFlowStepRef(new CompleteBBGraph(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#if HAVE_BAMBU_BUILT
      case COMPUTE_IMPLICIT_CALLS:
      {
         return DesignFlowStepRef(new compute_implicit_calls(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case COMMUTATIVE_EXPR_RESTRUCTURING:
      {
         return DesignFlowStepRef(new commutative_expr_restructuring(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case COND_EXPR_RESTRUCTURING:
      {
         return DesignFlowStepRef(new CondExprRestructuring(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case CSE_STEP:
      {
         return DesignFlowStepRef(new CSE(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_ZEBU_BUILT || HAVE_BAMBU_BUILT
      case DEAD_CODE_ELIMINATION:
      {
         return DesignFlowStepRef(new dead_code_elimination(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_BAMBU_BUILT
      case DETERMINE_MEMORY_ACCESSES:
      {
         return DesignFlowStepRef(new determine_memory_accesses(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
      case DOM_POST_DOM_COMPUTATION:
      {
         return DesignFlowStepRef(new dom_post_dom_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#if HAVE_BAMBU_BUILT
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
         return DesignFlowStepRef(new MultipleEntryIfReduction(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case DYNAMIC_AGGREGATE_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(new DynamicAggregateDataFlowAnalysis(AppM, design_flow_manager.lock(), function_id, parameters));
      }
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case EXTENDED_PDG_COMPUTATION:
      {
         return DesignFlowStepRef(new extended_pdg_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_BAMBU_BUILT
      case EXTRACT_GIMPLE_COND_OP:
      {
         return DesignFlowStepRef(new ExtractGimpleCondOp(AppM, design_flow_manager.lock(), function_id, parameters));
      }
#endif
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
      case EXTRACT_OMP_ATOMIC:
      {
         return DesignFlowStepRef(new ExtractOmpAtomic(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case EXTRACT_OMP_FOR:
      {
         return DesignFlowStepRef(new ExtractOmpFor(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case EXTRACT_PATTERNS:
      {
         return DesignFlowStepRef(new extract_patterns(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case FIX_STRUCTS_PASSED_BY_VALUE:
      {
         return DesignFlowStepRef(new FixStructsPassedByValue(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case FUNCTION_CALL_TYPE_CLEANUP:
      {
         return DesignFlowStepRef(new FunctionCallTypeCleanup(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_ZEBU_BUILT
      case GLOBAL_VARIABLES_ANALYSIS:
      {
         return DesignFlowStepRef(new GlobalVariablesAnalysis(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_BAMBU_BUILT
      case HDL_VAR_DECL_FIX:
      {
         return DesignFlowStepRef(new HDLVarDeclFix(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ZEBU_BUILT
      case HEADER_STRUCTURING:
      {
         return DesignFlowStepRef(new header_structuring(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case INSTRUCTION_SEQUENCES_COMPUTATION:
      {
         return DesignFlowStepRef(new InstructionSequencesComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_BAMBU_BUILT
      case HLS_DIV_CG_EXT:
      {
         return DesignFlowStepRef(new hls_div_cg_ext(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case HWCALL_INJECTION:
      {
         return DesignFlowStepRef(new HWCallInjection(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case INTERFACE_INFER:
      {
         return DesignFlowStepRef(new interface_infer(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case IR_LOWERING:
      {
         return DesignFlowStepRef(new IR_lowering(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
      case LOOP_COMPUTATION:
      {
         return DesignFlowStepRef(new loops_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#if HAVE_ZEBU_BUILT
      case LOOP_REGIONS_COMPUTATION:
      {
         return DesignFlowStepRef(new loop_regions_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case LOOP_REGIONS_FLOW_COMPUTATION:
      {
         return DesignFlowStepRef(new loop_regions_flow_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_BAMBU_BUILT
      case LOOPS_ANALYSIS_BAMBU:
      {
         return DesignFlowStepRef(new LoopsAnalysisBambu(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_ZEBU_BUILT
      case LOOPS_ANALYSIS_ZEBU:
      {
         return DesignFlowStepRef(new LoopsAnalysisZebu(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
      case LOOPS_COMPUTATION:
      {
         return DesignFlowStepRef(new loops_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#if HAVE_ZEBU_BUILT
      case LOOPS_REBUILDING:
      {
         return DesignFlowStepRef(new LoopsRebuilding(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case LUT_TRANSFORMATION:
      {
         return DesignFlowStepRef(new lut_transformation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
      case MEM_CG_EXT:
      {
         return DesignFlowStepRef(new mem_cg_ext(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case MEMORY_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(new MemoryDataFlowAnalysis(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#if HAVE_BAMBU_BUILT
      case MULTI_WAY_IF:
      {
         return DesignFlowStepRef(new multi_way_if(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case NI_SSA_LIVENESS:
      {
         return DesignFlowStepRef(new NI_SSA_liveness(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
      case OP_CONTROL_DEPENDENCE_COMPUTATION:
      {
         return DesignFlowStepRef(new OpCdgComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case OP_FEEDBACK_EDGES_IDENTIFICATION:
      {
         return DesignFlowStepRef(new op_feedback_edges_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case OP_ORDER_COMPUTATION:
      {
         return DesignFlowStepRef(new OpOrderComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case OP_REACHABILITY_COMPUTATION:
      {
         return DesignFlowStepRef(new OpReachabilityComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case OPERATIONS_CFG_COMPUTATION:
      {
         return DesignFlowStepRef(new operations_cfg_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#if HAVE_EXPERIMENTAL && HAVE_ZEBU_BUILT
      case PARALLEL_LOOP_SWAP:
      {
         return DesignFlowStepRef(new parallel_loop_swap(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case PARALLEL_LOOPS_ANALYSIS:
      {
         return DesignFlowStepRef(new ParallelLoopsAnalysis(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case PARALLEL_REGIONS_GRAPH_COMPUTATION:
      {
         return DesignFlowStepRef(new ParallelRegionsGraphComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_BAMBU_BUILT
      case PARM_DECL_TAKEN_ADDRESS:
      {
         return DesignFlowStepRef(new parm_decl_taken_address_fix(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case PHI_OPT:
      {
         return DesignFlowStepRef(new PhiOpt(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ZEBU_BUILT
      case POINTED_DATA_COMPUTATION:
      {
         return DesignFlowStepRef(new PointedDataComputation(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case PREDICATE_STATEMENTS:
      {
         return DesignFlowStepRef(new PredicateStatements(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ZEBU_BUILT
      case PREDICTABILITY_ANALYSIS:
      {
         return DesignFlowStepRef(new predictability_analysis(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_ZEBU_BUILT
      case PROBABILITY_PATH:
      {
         return DesignFlowStepRef(new probability_path(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_BAMBU_BUILT
      case REBUILD_INITIALIZATION:
      {
         return DesignFlowStepRef(new rebuild_initialization(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case REBUILD_INITIALIZATION2:
      {
         return DesignFlowStepRef(new rebuild_initialization2(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif

#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case REDUCED_PDG_COMPUTATION:
      {
         return DesignFlowStepRef(new reduced_pdg_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case REFINED_AGGREGATE_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(new RefinedAggregateDataFlowAnalysis(AppM, function_id, design_flow_manager.lock(), parameters));
      }
      case REFINED_VAR_COMPUTATION:
      {
         return DesignFlowStepRef(new RefinedVarComputation(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case REMOVE_CLOBBER_GA:
      {
         return DesignFlowStepRef(new remove_clobber_ga(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case REMOVE_ENDING_IF:
      {
         return DesignFlowStepRef(new RemoveEndingIf(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_ZEBU_BUILT
      case REVERSE_RESTRICT_COMPUTATION:
      {
         return DesignFlowStepRef(new ReverseRestrictComputation(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
      case SDC_CODE_MOTION:
      {
         return DesignFlowStepRef(new SDCCodeMotion(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case SERIALIZE_MUTUAL_EXCLUSIONS:
      {
         return DesignFlowStepRef(new SerializeMutualExclusions(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ZEBU_BUILT
      case SHORT_CIRCUIT_STRUCTURING:
      {
         return DesignFlowStepRef(new short_circuit_structuring(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_BAMBU_BUILT
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
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case SPECULATION_EDGES_COMPUTATION:
      {
         return DesignFlowStepRef(new speculation_edges_computation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_ZEBU_BUILT
      case SPLIT_PHINODES:
      {
         return DesignFlowStepRef(new split_phi_nodes(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
      case SCALAR_SSA_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(new ScalarSsaDataDependenceComputation(parameters, AppM, function_id, design_flow_manager.lock()));
      }
      case SWITCH_FIX:
      {
         return DesignFlowStepRef(new SwitchFix(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#if HAVE_BAMBU_BUILT
      case UN_COMPARISON_LOWERING:
      {
         return DesignFlowStepRef(new UnComparisonLowering(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
      case UNROLL_LOOPS:
      {
         return DesignFlowStepRef(new UnrollLoops(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case UNROLLING_DEGREE:
      {
         THROW_UNREACHABLE("Not updated step");
         break;
      }
#endif
#if HAVE_RTL_BUILT && HAVE_ZEBU_BUILT
      case UPDATE_RTL_WEIGHT:
      {
         return DesignFlowStepRef(new update_rtl_weight(parameters, AppM, arch_manager, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
      case UPDATE_SCHEDULE:
      {
         return DesignFlowStepRef(new UpdateSchedule(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_ZEBU_BUILT
      case UPDATE_TREE_WEIGHT:
      {
         return DesignFlowStepRef(new update_tree_weight(parameters, AppM, arch_manager, function_id, design_flow_manager.lock()));
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
#if HAVE_BAMBU_BUILT
      case VECTORIZE:
      {
         return DesignFlowStepRef(new Vectorize(AppM, function_id, design_flow_manager.lock(), parameters));
      }
#endif
#if HAVE_BAMBU_BUILT
      case VERIFICATION_OPERATION:
      {
         THROW_UNREACHABLE("Not updated step");
         break;
      }
#endif
      case VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS:
      {
         return DesignFlowStepRef(new VirtualAggregateDataFlowAnalysis(AppM, design_flow_manager.lock(), function_id, parameters));
      }
#if HAVE_BAMBU_BUILT
      case VIRTUAL_PHI_NODES_SPLIT:
      {
         return DesignFlowStepRef(new virtual_phi_nodes_split(parameters, AppM, function_id, design_flow_manager.lock()));
      }
#endif
#if HAVE_HOST_PROFILING_BUILT
      case BASIC_BLOCKS_PROFILING:
#endif
#if HAVE_BAMBU_BUILT
      case BAMBU_FRONTEND_FLOW:
      case BIT_VALUE_IPA:
#endif
      case(COMPLETE_CALL_GRAPH):
#if HAVE_TASTE
      case CREATE_ADDRESS_TRANSLATION:
#endif
      case(CREATE_TREE_MANAGER):
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case(DYNAMIC_VAR_COMPUTATION):
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
      case(POINTED_DATA_EVALUATION):
#endif
#if HAVE_PRAGMA_BUILT
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
      case STRING_CST_FIX:
      case(SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP):
      {
         THROW_UNREACHABLE("Trying to create a function frontend flow step from " + FrontendFlowStep::EnumToKindText(design_flow_step_type));
         break;
      }
      default:
         THROW_UNREACHABLE("Frontend flow step type does not exist");
   }
   return DesignFlowStepRef();
}

const std::string FrontendFlowStepFactory::GetPrefix() const
{
   return "Frontend";
}
