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
 * @file frontend_flow_step.cpp
 * @brief This class contains the base representation for a generic frontend flow step
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "frontend_flow_step.hpp"

#include "Parameter.hpp"
#include "application_frontend_flow_step.hpp"
#include "application_manager.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "exceptions.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_frontend_flow_step.hpp"
#include "graph.hpp"
#include "hash_helper.hpp"
#include "ir_manager.hpp"
#include "string_manipulation.hpp"

#include <iosfwd>

FrontendFlowStep::FrontendFlowStep(DesignFlowStep::signature_t _signature, const application_managerRef _AppM,
                                   const FrontendFlowStepType _frontend_flow_step_type,
                                   const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : DesignFlowStep(_signature, _design_flow_manager, _parameters),
      AppM(_AppM),
      frontend_flow_step_type(_frontend_flow_step_type),
      print_counter(0)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

void FrontendFlowStep::CreateSteps(
    const DesignFlowManager& design_flow_manager,
    const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>& frontend_relationships,
    const application_managerConstRef application_manager, DesignFlowStepSet& relationships)
{
   const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
   const auto frontend_flow_step_factory = GetPointerS<const FrontendFlowStepFactory>(
       design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND));
   for(const auto& [step_type, rel_type] : frontend_relationships)
   {
      switch(rel_type)
      {
         case(ALL_FUNCTIONS):
         {
            const auto call_graph_computation_step =
                design_flow_manager.GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(FUNCTION_ANALYSIS));
            const auto cg_design_flow_step =
                call_graph_computation_step != DesignFlowGraph::null_vertex() ?
                    design_flow_graph->CGetNodeInfo(call_graph_computation_step)->design_flow_step :
                    frontend_flow_step_factory->CreateApplicationFrontendFlowStep(FUNCTION_ANALYSIS);
            relationships.insert(cg_design_flow_step);
            const auto functions_with_body = application_manager->CGetCallGraphManager().GetReachedBodyFunctions();
            for(const auto function_with_body_id : functions_with_body)
            {
               const auto sdf_step = design_flow_manager.GetDesignFlowStep(
                   FunctionFrontendFlowStep::ComputeSignature(step_type, function_with_body_id));
               const auto design_flow_step =
                   sdf_step != DesignFlowGraph::null_vertex() ?
                       design_flow_graph->CGetNodeInfo(sdf_step)->design_flow_step :
                       frontend_flow_step_factory->CreateFunctionFrontendFlowStep(step_type, function_with_body_id);
               relationships.insert(design_flow_step);
            }
            break;
         }
         case(WHOLE_APPLICATION):
         {
            const auto sdf_signature = ApplicationFrontendFlowStep::ComputeSignature(step_type);
            const auto sdf_step = design_flow_manager.GetDesignFlowStep(sdf_signature);
            DesignFlowStepRef design_flow_step;
            if(sdf_step != DesignFlowGraph::null_vertex())
            {
               design_flow_step = design_flow_graph->CGetNodeInfo(sdf_step)->design_flow_step;
            }
            else
            {
               design_flow_step = frontend_flow_step_factory->GenerateFrontendStep(step_type);
            }
            relationships.insert(design_flow_step);
            break;
         }
         case(CALLING_FUNCTIONS):
         case(CALLED_FUNCTIONS):
         case(SAME_FUNCTION):
         {
            /// This is managed by FunctionFrontendFlowStep::ComputeRelationships
            break;
         }
         default:
         {
            THROW_UNREACHABLE("Function relationship does not exist");
         }
      }
   }
}

void FrontendFlowStep::ComputeRelationships(DesignFlowStepSet& relationships,
                                            const DesignFlowStep::RelationshipType relationship_type)
{
   const auto frontend_relationships = ComputeFrontendRelationships(relationship_type);
   CreateSteps(design_flow_manager, frontend_relationships, AppM, relationships);
}

std::string FrontendFlowStep::GetKindText() const
{
   return EnumToKindText(frontend_flow_step_type);
}

const std::string FrontendFlowStep::EnumToKindText(const FrontendFlowStepType frontend_flow_step_type)
{
   switch(frontend_flow_step_type)
   {
      case ADD_ARTIFICIAL_CALL_FLOW_EDGES:
         return "AddArtificialCallFlowEdges";
      case(ADD_OP_EXIT_FLOW_EDGES):
         return "AddOpExitFlowEdges";
      case(BAMBU_FRONTEND_FLOW):
         return "BambuFrontendFlow";
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
      case(BIT_VALUE):
         return "BitValue";
      case(BIT_VALUE_OPT):
         return "BitValueOpt";
      case(BITVALUE_RANGE):
         return "BitValueRange";
      case BIT_VALUE_IPA:
         return "BitValueIPA";
      case(BLOCK_FIX):
         return "BlockFix";
      case BUILD_VIRTUAL_PHI:
         return "BuildVirtualPhi";
      case(CALL_NODE_FIX):
         return "CallNodeFix";
      case CALL_GRAPH_BUILTIN_CALL:
         return "CallGraphBuiltinCall";
      case(CHECK_SYSTEM_TYPE):
         return "CheckSystemType";
      case(COMPLETE_BB_GRAPH):
         return "CompleteBBGraph";
      case(COMPLETE_CALL_GRAPH):
         return "CompleteCallGraph";
      case COMMUTATIVE_EXPR_RESTRUCTURING:
         return "CommutativeExprRestructuring";
      case SELECT_TREE_BALANCING:
         return "SelectTreeBalancing";
      case CSE_STEP:
         return "CSE";
      case(CREATE_IR_MANAGER):
         return "CreateIRManager";
      case DATAFLOW_CG_EXT:
         return "DataflowCGExt";
      case(DCE_PASS):
         return "DeadCodeElimination";
      case(DEAD_CODE_ELIMINATION_IPA):
         return "DeadCodeEliminationIPA";
      case(DETERMINE_MEMORY_ACCESSES):
         return "DetermineMemoryAccesses";
      case(DOM_POST_DOM_COMPUTATION):
         return "DomPostDomComputation";
      case(FANOUT_OPT):
         return "FanoutOpt";
      case(EXTRACT_COND_OP):
         return "ExtractCondOp";
      case(EXTRACT_PATTERNS):
         return "ExtractPatterns";
      case FIND_MAX_TRANSFORMATIONS:
         return "FindMaxTransformations";
      case(FUNCTION_ANALYSIS):
         return "CallGraphComputation";
      case FIX_STRUCTS_PASSED_BY_VALUE:
         return "FixStructsPassedByValue";
      case FIX_VDEF:
         return "FixVdef";
      case FUNCTION_CALL_TYPE_CLEANUP:
         return "FunctionCallTypeCleanup";
      case FUNCTION_CALL_OPT:
         return "FunctionCallOpt";
      case(HWCALL_INJECTION):
         return "HWCallInjection";
#if HAVE_HOST_PROFILING_BUILT
      case(HOST_PROFILING):
         return "HostProfiling";
#endif
      case(INTERFACE_INFER):
         return "InterfaceInfer";
      case(IR_LOWERING):
         return "IrLowering";
      case(LOOP_COMPUTATION):
         return "LoopComputation";
      case(LOOPS_COMPUTATION):
         return "LoopsComputation";
      case MULTI_WAY_IF:
         return "MultiWayIf";
      case(MUL_DECOMPOSITION):
         return "MultExprFracturing";
      case NI_SSA_LIVENESS:
         return "NiSsaLiveness";
      case OMP_CG_EXT:
         return "OMPCGExt";
      case OMP_LOWERING:
         return "OMPLowering";
      case(OP_CONTROL_DEPENDENCE_COMPUTATION):
         return "OpControlDependenceComputation";
      case(OP_FEEDBACK_EDGES_IDENTIFICATION):
         return "OpFeedbackEdgesIdentification";
      case(OP_ORDER_COMPUTATION):
         return "OpOrderComputation";
      case(OPERATIONS_CFG_COMPUTATION):
         return "OperationsCfgComputation";
      case PARM2SSA:
         return "Parm2SSA";
      case PARM_DECL_TAKEN_ADDRESS:
         return "ParmDeclTakenAddressFix";
      case PHI_OPT:
         return "PhiOpt";
      case PREDICATE_STATEMENTS:
         return "PredicateStatements";
      case RANGE_ANALYSIS:
         return "RangeAnalysis";
      case(REBUILD_INITIALIZATION2):
         return "RebuildInitialization2";
      case SDC_CODE_MOTION:
         return "SdcCodeMotion";
      case SIMPLE_CODE_MOTION:
         return "SimpleCodeMotion";
      case(SOFT_FLOAT_CG_EXT):
         return "SoftFloatCgExt";
      case(SOFT_INT_CG_EXT):
         return "SoftIntCGExt";
      case(SCALAR_SSA_DATA_FLOW_ANALYSIS):
         return "ScalarSsaDataFlowAnalysis";
      case(SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP):
         return "SymbolicApplicationFrontendFlowStep";
      case(IR2FUN):
         return "IR2Fun";
      case(UNROLLING_DEGREE):
         return "UnrollingDegree";
      case UPDATE_SCHEDULE:
         return "UpdateSchedule";
      case(USE_COUNTING):
         return "UseCounting";
      case(VAR_ANALYSIS):
         return "VarAnalysis";
      case(VAR_DECL_FIX):
         return "VarDeclFix";
      case(VERIFICATION_OPERATION):
         return "VerificationOperation";
      case(VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS):
         return "VirtualAggregateDataFlowAnalysis";
      default:
         THROW_UNREACHABLE("Frontend flow step type does not exist");
   }
   return "";
}

DesignFlowStepFactoryConstRef FrontendFlowStep::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND);
}

void FrontendFlowStep::PrintIRManager(const bool before) const
{
   const ir_managerConstRef ir_manager = AppM->get_ir_manager();
   const std::string prefix = before ? "before" : "after";
   const std::string file_name =
       parameters->getOption<std::string>(OPT_output_temporary_directory) + "/" + prefix + "_" + GetName();
   const std::string suffix = print_counter == 0 ? "" : "_" + STR(print_counter);
   const std::string raw_file_name = file_name + suffix + ".raw";
   std::ofstream raw_file(raw_file_name.c_str());
   ir_manager->print(raw_file);
   raw_file.close();
   const std::string llvm_file_name = file_name + ".bllvm";
   std::ofstream llvm_file(llvm_file_name.c_str());
   ir_manager->PrintBambuLLVM(llvm_file);
   llvm_file.close();
}

void FrontendFlowStep::PrintInitialIR() const
{
   if(!parameters->IsParameter("print-ir-manager") || parameters->GetParameter<unsigned int>("print-ir-manager"))
   {
      PrintIRManager(true);
   }
}

void FrontendFlowStep::PrintFinalIR() const
{
   if(!parameters->IsParameter("print-ir-manager") || parameters->GetParameter<unsigned int>("print-ir-manager"))
   {
      PrintIRManager(false);
   }
}
