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
 * @file frontend_flow_step.cpp
 * @brief This class contains the base representation for a generic frontend flow step
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "frontend_flow_step.hpp"
#include "Parameter.hpp"                      // for Parameter, OPT_output_...
#include "application_frontend_flow_step.hpp" // for ApplicationFrontendFlo...
#include "application_manager.hpp"            // for application_manager
#include "call_graph_manager.hpp"             // for application_managerCon...
#include "custom_set.hpp"                     // for set
#include "design_flow_graph.hpp"              // for DesignFlowGraph, Desig...
#include "design_flow_manager.hpp"            // for DesignFlowManager, Des...
#include "exceptions.hpp"                     // for THROW_UNREACHABLE
#include "frontend_flow_step_factory.hpp"     // for DesignFlowStepRef, Fro...
#include "function_frontend_flow_step.hpp"    // for DesignFlowManagerConstRef
#include "graph.hpp"                          // for vertex
#include "hash_helper.hpp"                    // for hash
#include "string_manipulation.hpp"            // for STR GET_CLASS
#include "tree_manager.hpp"                   // for tree_managerConstRef
#include <iosfwd>                             // for ofstream

FrontendFlowStep::FrontendFlowStep(const application_managerRef _AppM, const FrontendFlowStepType _frontend_flow_step_type, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : DesignFlowStep(_design_flow_manager, _parameters), AppM(_AppM), frontend_flow_step_type(_frontend_flow_step_type), print_counter(0)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

FrontendFlowStep::~FrontendFlowStep() = default;

void FrontendFlowStep::CreateSteps(const DesignFlowManagerConstRef design_flow_manager, const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>& frontend_relationships, const application_managerConstRef application_manager,
                                   DesignFlowStepSet& relationships)
{
   const DesignFlowGraphConstRef design_flow_graph = design_flow_manager->CGetDesignFlowGraph();
   const auto* frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(design_flow_manager->CGetDesignFlowStepFactory("Frontend"));
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>::const_iterator frontend_relationship, frontend_relationship_end = frontend_relationships.end();
   for(frontend_relationship = frontend_relationships.begin(); frontend_relationship != frontend_relationship_end; ++frontend_relationship)
   {
      switch(frontend_relationship->second)
      {
         case(ALL_FUNCTIONS):
         {
            const vertex call_graph_computation_step = design_flow_manager->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(FUNCTION_ANALYSIS));
            const DesignFlowStepRef cg_design_flow_step =
                call_graph_computation_step ? design_flow_graph->CGetDesignFlowStepInfo(call_graph_computation_step)->design_flow_step : frontend_flow_step_factory->CreateApplicationFrontendFlowStep(FUNCTION_ANALYSIS);
            relationships.insert(cg_design_flow_step);
            const auto functions_with_body = application_manager->CGetCallGraphManager()->GetReachedBodyFunctions();
            for(const auto function_with_body_id : functions_with_body)
            {
               const vertex sdf_step = design_flow_manager->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(frontend_relationship->first, function_with_body_id));
               const DesignFlowStepRef design_flow_step = sdf_step ? design_flow_graph->CGetDesignFlowStepInfo(sdf_step)->design_flow_step : frontend_flow_step_factory->CreateFunctionFrontendFlowStep(frontend_relationship->first, function_with_body_id);
               relationships.insert(design_flow_step);
            }
            break;
         }
         case(CALLING_FUNCTIONS):
         case(CALLED_FUNCTIONS):
         case(SAME_FUNCTION):
         {
            /// This is managed by FunctionFrontendFlowStep::ComputeRelationships
            break;
         }
         case(WHOLE_APPLICATION):
         {
            vertex sdf_step = design_flow_manager->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(frontend_relationship->first));
            DesignFlowStepRef design_flow_step;
            if(sdf_step)
            {
               design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(sdf_step)->design_flow_step;
               relationships.insert(design_flow_step);
            }
            else
            {
               design_flow_step = frontend_flow_step_factory->GenerateFrontendStep(frontend_relationship->first);
               relationships.insert(design_flow_step);
            }
            break;
         }
         default:
         {
            THROW_UNREACHABLE("Function relationship does not exist");
         }
      }
   }
}

void FrontendFlowStep::ComputeRelationships(DesignFlowStepSet& relationships, const DesignFlowStep::RelationshipType relationship_type)
{
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> frontend_relationships = ComputeFrontendRelationships(relationship_type);
   CreateSteps(design_flow_manager.lock(), frontend_relationships, AppM, relationships);
}

const std::string FrontendFlowStep::GetKindText() const
{
   return EnumToKindText(frontend_flow_step_type);
}

const std::string FrontendFlowStep::EnumToKindText(const FrontendFlowStepType frontend_flow_step_type)
{
   switch(frontend_flow_step_type)
   {
      case(ADD_BB_ECFG_EDGES):
         return "AddBbEcfgdges";
#if HAVE_BAMBU_BUILT
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
         return "AddArtificialCallFlowEdges";
#endif
#if HAVE_ZEBU_BUILT
      case(ADD_OP_ECFG_EDGES):
         return "AddOpEcfgEdges";
#endif
      case(ADD_OP_EXIT_FLOW_EDGES):
         return "AddOpExitFlowEdges";
      case(ADD_OP_LOOP_FLOW_EDGES):
         return "AddOpLoopFlowEdges";
#if HAVE_BAMBU_BUILT
      case(ADD_OP_PHI_FLOW_EDGES):
         return "AddOpPhiFlowEdges";
#endif
      case(AGGREGATE_DATA_FLOW_ANALYSIS):
         return "AggregateDataFlowAnalysis";
#if HAVE_ZEBU_BUILT
      case(ARRAY_REF_FIX):
         return "ArrayRefFix";
#endif
#if HAVE_BAMBU_BUILT
      case(BAMBU_FRONTEND_FLOW):
         return "BambuFrontendFlow";
#endif
      case(BASIC_BLOCKS_CFG_COMPUTATION):
         return "BasicBlocksCfgComputation";
      case(BB_CONTROL_DEPENDENCE_COMPUTATION):
         return "BBControlDependenceComputation";
      case(BB_FEEDBACK_EDGES_IDENTIFICATION):
         return "BBFeedbackEdgesIdentification";
      case(BB_ORDER_COMPUTATION):
         return "BBOrderComputation";
#if HAVE_HOST_PROFILING_BUILT
      case(BASIC_BLOCKS_PROFILING):
         return "BasicBlocksProfiling";
#endif
      case(BB_REACHABILITY_COMPUTATION):
         return "BBReachabilityComputation";
#if HAVE_BAMBU_BUILT
      case(BIT_VALUE):
         return "BitValue";
      case(BIT_VALUE_OPT):
         return "BitValueOpt";
      case BIT_VALUE_IPA:
         return "BitValueIPA";
#endif
      case(BLOCK_FIX):
         return "BlockFix";
      case BUILD_VIRTUAL_PHI:
         return "BuildVirtualPhi";
#if HAVE_ZEBU_BUILT
      case(CALL_ARGS_STRUCTURING):
         return "CallArgsStructuring";
#endif
      case(CALL_EXPR_FIX):
         return "CallExprFix";
#if HAVE_BAMBU_BUILT
      case CALL_GRAPH_BUILTIN_CALL:
         return "CallGraphBuiltinCall";
#endif
#if HAVE_FROM_PRAGMA_BUILT && HAVE_EXPERIMENTAL && HAVE_BAMBU_BUILT
      case CHECK_CRITICAL_SESSION:
         return "CheckCriticalSession";
#endif
#if HAVE_ZEBU_BUILT
      case(CHECK_PIPELINABLE_LOOPS):
         return "CheckPipelinableLoops";
#endif
      case(CHECK_SYSTEM_TYPE):
         return "CheckSystemType";
      case CLEAN_VIRTUAL_PHI:
         return "CleanVirtualPhi";
      case(COMPLETE_BB_GRAPH):
         return "CompleteBBGraph";
      case(COMPLETE_CALL_GRAPH):
         return "CompleteCallGraph";
#if HAVE_BAMBU_BUILT
      case COMPUTE_IMPLICIT_CALLS:
         return "ComputeImplicitCalls";
      case COMMUTATIVE_EXPR_RESTRUCTURING:
         return "CommutativeExprRestructuring";
      case COND_EXPR_RESTRUCTURING:
         return "CondExprRestructuring";
      case CSE_STEP:
         return "CSE";
#endif
#if HAVE_TASTE
      case CREATE_ADDRESS_TRANSLATION:
         return "CreateAddressTranslation";
#endif
      case(CREATE_TREE_MANAGER):
         return "CreateTreeManager";
#if HAVE_ZEBU_BUILT || HAVE_BAMBU_BUILT
      case(DEAD_CODE_ELIMINATION):
         return "DeadCodeElimination";
#endif
#if HAVE_BAMBU_BUILT
      case(DETERMINE_MEMORY_ACCESSES):
         return "DetermineMemoryAccesses";
#endif
      case(DOM_POST_DOM_COMPUTATION):
         return "DomPostDomComputation";
#if HAVE_BAMBU_BUILT
      case(FANOUT_OPT):
         return "FanoutOpt";
      case(MULTIPLE_ENTRY_IF_REDUCTION): // modified here
         return "MultipleEntryIfReduction";
#endif
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case(DYNAMIC_AGGREGATE_DATA_FLOW_ANALYSIS):
         return "DynamicAggregateDataFlowAnalysis";
      case(DYNAMIC_VAR_COMPUTATION):
         return "DynamicVarComputation";
#endif
#if HAVE_BAMBU_BUILT
      case ESSA:
         return "eSSA";
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case(EXTENDED_PDG_COMPUTATION):
         return "ExtendedPdgComputation";
#endif
#if HAVE_BAMBU_BUILT
      case(EXTRACT_GIMPLE_COND_OP):
         return "ExtractGimpleCondOp";
#endif
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
      case(EXTRACT_OMP_ATOMIC):
         return "ExtractOmpAtomic";
      case(EXTRACT_OMP_FOR):
         return "ExtractOmpFor";
#endif
#if HAVE_BAMBU_BUILT
      case(EXTRACT_PATTERNS):
         return "ExtractPatterns";
#endif
#if HAVE_BAMBU_BUILT
      case FIND_MAX_CFG_TRANSFORMATIONS:
         return "FindMaxCfgTransformations";
#endif
      case(FUNCTION_ANALYSIS):
         return "CallGraphComputation";
#if HAVE_BAMBU_BUILT
      case FIX_STRUCTS_PASSED_BY_VALUE:
         return "FixStructsPassedByValue";
      case FUNCTION_PARM_MASK:
         return "FunctionParmMask";
      case FUNCTION_CALL_TYPE_CLEANUP:
         return "FunctionCallTypeCleanup";
#endif
#if HAVE_ZEBU_BUILT
      case(FUNCTION_POINTER_CALLGRAPH_COMPUTATION):
         return "FunctionPointerCallGraphComputation";
      case(GLOBAL_VARIABLES_ANALYSIS):
         return "GlobalVariablesAnalysis";
#endif
#if HAVE_BAMBU_BUILT
      case(HDL_FUNCTION_DECL_FIX):
         return "HDLFunctionDeclFix";
      case(HDL_VAR_DECL_FIX):
         return "HDLVarDeclFix";
#endif
#if HAVE_ZEBU_BUILT
      case(HEADER_STRUCTURING):
         return "HeaderStructuring";
#endif
#if HAVE_BAMBU_BUILT
      case(HLS_DIV_CG_EXT):
         return "HLSDivCGExt";
      case(HWCALL_INJECTION):
         return "HWCallInjection";
#endif
#if HAVE_HOST_PROFILING_BUILT
      case(HOST_PROFILING):
         return "HostProfiling";
#endif
#if HAVE_ZEBU_BUILT
      case(INSTRUCTION_SEQUENCES_COMPUTATION):
         return "InstructionSequencesComputation";
#endif
#if HAVE_BAMBU_BUILT
      case(INTERFACE_INFER):
         return "InterfaceInfer";
      case(IPA_POINT_TO_ANALYSIS):
         return "IpaPointToAnalysis";
      case(IR_LOWERING):
         return "IrLowering";
#endif
      case(LOOP_COMPUTATION):
         return "LoopComputation";
#if HAVE_ZEBU_BUILT
      case(LOOP_REGIONS_COMPUTATION):
         return "LoopRegionsComputation";
      case(LOOP_REGIONS_FLOW_COMPUTATION):
         return "LoopRegionsFlowComputation";
#endif
#if HAVE_BAMBU_BUILT
      case(LOOPS_ANALYSIS_BAMBU):
         return "LoopsAnalysisBambu";
#endif
#if HAVE_ZEBU_BUILT
      case(LOOPS_ANALYSIS_ZEBU):
         return "LoopsAnalysisZebu";
#endif
      case(LOOPS_COMPUTATION):
         return "LoopsComputation";
#if HAVE_ZEBU_BUILT
      case(LOOPS_REBUILDING):
         return "LoopsRebuilding";
#endif
#if HAVE_BAMBU_BUILT
      case(LUT_TRANSFORMATION):
         return "LutTransformation";
#endif
      case(MEMORY_DATA_FLOW_ANALYSIS):
         return "MemoryDataFlowAnalysis";
      case MEM_CG_EXT:
         return "MemCgExt";
#if HAVE_BAMBU_BUILT
      case MULTI_WAY_IF:
         return "MultiWayIf";
      case NI_SSA_LIVENESS:
         return "NiSsaLiveness";
#endif
      case(OP_CONTROL_DEPENDENCE_COMPUTATION):
         return "OpControlDependenceComputation";
      case(OP_FEEDBACK_EDGES_IDENTIFICATION):
         return "OpFeedbackEdgesIdentification";
      case(OP_ORDER_COMPUTATION):
         return "OpOrderComputation";
      case(OP_REACHABILITY_COMPUTATION):
         return "OpReachabilityComputation";
      case(OPERATIONS_CFG_COMPUTATION):
         return "OperationsCfgComputation";
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case(PARALLEL_LOOP_SWAP):
         return "ParallelLoopSwap";
      case(PARALLEL_LOOPS_ANALYSIS):
         return "ParallelLoopsAnalysis";
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case(PARALLEL_REGIONS_GRAPH_COMPUTATION):
         return "ParallelRegionsGraphComputation";
#endif
      case PARM2SSA:
         return "Parm2SSA";
#if HAVE_BAMBU_BUILT
      case PARM_DECL_TAKEN_ADDRESS:
         return "ParmDeclTakenAddress";
      case PHI_OPT:
         return "PhiOpt";
#endif
#if HAVE_ZEBU_BUILT
      case(POINTED_DATA_COMPUTATION):
         return "PointedDataComputation";
      case(POINTED_DATA_EVALUATION):
         return "PointedDataEvaluation";
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case(PRAGMA_ANALYSIS):
         return "PragmaAnalysis";
      case(PRAGMA_SUBSTITUTION):
         return "PragmaSubstitution";
#endif
#if HAVE_BAMBU_BUILT
      case PREDICATE_STATEMENTS:
         return "PredicateStatements";
#endif
#if HAVE_ZEBU_BUILT
      case(PREDICTABILITY_ANALYSIS):
         return "PredictabilityAnalysis";
      case(PROBABILITY_PATH):
         return "ProbabilityPath";
#endif
#if HAVE_BAMBU_BUILT
      case RANGE_ANALYSIS:
         return "RangeAnalysis";
      case(REBUILD_INITIALIZATION):
         return "RebuildInitialization";
      case(REBUILD_INITIALIZATION2):
         return "RebuildInitialization2";
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case(REDUCED_PDG_COMPUTATION):
         return "ReducedPdgComputation";
#endif
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
      case(REFINED_AGGREGATE_DATA_FLOW_ANALYSIS):
         return "RefinedDataFlowAnalysis";
      case(REFINED_VAR_COMPUTATION):
         return "RefinedVarComputation";
#endif
#if HAVE_BAMBU_BUILT
      case REMOVE_CLOBBER_GA:
         return "RemoveClobberGA";
#endif
#if HAVE_BAMBU_BUILT
      case REMOVE_ENDING_IF:
         return "RemoveEndingIf";
#endif
#if HAVE_ZEBU_BUILT
      case(REVERSE_RESTRICT_COMPUTATION):
         return "ReverseRestrictComputation";
#endif
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
      case SDC_CODE_MOTION:
         return "SdcCodeMotion";
#endif
#if HAVE_BAMBU_BUILT
      case SERIALIZE_MUTUAL_EXCLUSIONS:
         return "SerializeMutualExclusions";
#endif
#if HAVE_ZEBU_BUILT
      case(SHORT_CIRCUIT_STRUCTURING):
         return "ShortCircuitStructuring";
      case(SIZEOF_SUBSTITUTION):
         return "SizeofSubstitution";
#endif

#if HAVE_BAMBU_BUILT
      case SPLIT_RETURN:
         return "SplitReturn";
      case SHORT_CIRCUIT_TAF:
         return "ShortCircuitTAF";
      case SIMPLE_CODE_MOTION:
         return "SimpleCodeMotion";
      case(SOFT_FLOAT_CG_EXT):
         return "SoftFloatCgExt";
#endif
#if HAVE_ZEBU_BUILT
      case(SOURCE_CODE_STATISTICS):
         return "SourceCodeStatistics";
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
      case(SPECULATION_EDGES_COMPUTATION):
         return "SpeculationEdgesComputation";
#endif
#if HAVE_ZEBU_BUILT
      case(SPLIT_PHINODES):
         return "SplitPhinodes";
#endif
      case(SCALAR_SSA_DATA_FLOW_ANALYSIS):
         return "ScalarSsaDataFlowAnalysis";
      case(SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP):
         return "SymbolicApplicationFrontendFlowStep";
      case STRING_CST_FIX:
         return "StringCstFix";
      case(SWITCH_FIX):
         return "SwitchFix";
#if HAVE_BAMBU_BUILT
      case UN_COMPARISON_LOWERING:
         return "UnComparisonLowering";
#endif
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
      case(UNROLL_LOOPS):
         return "UnrollLoops";
#endif
#if HAVE_BAMBU_BUILT
      case(UNROLLING_DEGREE):
         return "UnrollingDegree";
#endif
#if HAVE_RTL_BUILT && HAVE_ZEBU_BUILT
      case(UPDATE_RTL_WEIGHT):
         return "UpdateRtlWeight";
#endif
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
      case UPDATE_SCHEDULE:
         return "UpdateSchedule";
#endif
#if HAVE_ZEBU_BUILT
      case(UPDATE_TREE_WEIGHT):
         return "UpdateTreeWeight";
#endif
      case(USE_COUNTING):
         return "UseCounting";
      case(VAR_ANALYSIS):
         return "VarAnalysis";
      case(VAR_DECL_FIX):
         return "VarDeclFix";
#if HAVE_BAMBU_BUILT
      case(VECTORIZE):
         return "Vectorize";
#endif
#if HAVE_BAMBU_BUILT
      case(VERIFICATION_OPERATION):
         return "VerificationOperation";
#endif
      case(VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS):
         return "VirtualAggregateDataFlowAnalysis";
#if HAVE_BAMBU_BUILT
      case VIRTUAL_PHI_NODES_SPLIT:
         return "VirtualPhiNodesSplit";
#endif
      default:
         THROW_UNREACHABLE("Frontend flow step type does not exist");
   }
   return "";
}

const DesignFlowStepFactoryConstRef FrontendFlowStep::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend");
}

void FrontendFlowStep::PrintTreeManager(const bool before) const
{
   const tree_managerConstRef tree_manager = AppM->get_tree_manager();
   const std::string prefix = before ? "before" : "after";
   const std::string file_name = parameters->getOption<std::string>(OPT_output_temporary_directory) + prefix + "_" + GetName();
   const std::string suffix = print_counter == 0 ? "" : "_" + STR(print_counter);
   const std::string raw_file_name = file_name + suffix + ".raw";
   std::ofstream raw_file(raw_file_name.c_str());
   tree_manager->print(raw_file);
   raw_file.close();
   const std::string gimple_file_name = file_name + ".gimple";
   std::ofstream gimple_file(gimple_file_name.c_str());
   tree_manager->PrintGimple(gimple_file, false);
   gimple_file.close();
}

void FrontendFlowStep::PrintInitialIR() const
{
   PrintTreeManager(true);
}

void FrontendFlowStep::PrintFinalIR() const
{
   PrintTreeManager(false);
}
