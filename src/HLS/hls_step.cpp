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
 * @file HLS_step.cpp
 * @brief Implementation of some common methods to manage HLS_step algorithms
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "hls_step.hpp"

#include "Parameter.hpp"
#include "application_frontend_flow_step.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_function_step.hpp"
#include "hls_manager.hpp"
#include "memory.hpp"
#include "technology_manager.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

#include <tuple>

HLSFlowStepSpecialization::HLSFlowStepSpecialization() = default;

HLSFlowStepSpecialization::~HLSFlowStepSpecialization() = default;

HLSFlowStepSpecialization::context_t HLSFlowStepSpecialization::ComputeSignatureContext(SpecializationClass spec_class,
                                                                                        unsigned char context)
{
   return static_cast<context_t>(static_cast<context_t>(spec_class) << 8U | context);
}

HLS_step::HLS_step(signature_t _signature, const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                   const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
                   const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : DesignFlowStep(_signature, _design_flow_manager, _parameters),
      HLSMgr(_HLSMgr),
      hls_flow_step_type(_hls_flow_step_type),
      hls_flow_step_specialization(_hls_flow_step_specialization)
{
}

HLS_step::HLS_step(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                   const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
                   const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLS_step(ComputeSignature(_hls_flow_step_type, _hls_flow_step_specialization), _parameters, _HLSMgr,
               _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization)
{
}

HLS_step::HLSRelationships HLS_step::ComputeHLSRelationships(const DesignFlowStep::RelationshipType) const
{
   return HLSRelationships();
}

HLS_step::~HLS_step() = default;

CustomUnorderedMap<std::string, HLSFlowStep_Type> HLS_step::command_line_name_to_enum;

DesignFlowStep::signature_t
HLS_step::ComputeSignature(const HLSFlowStep_Type hls_flow_step_type,
                           const HLSFlowStepSpecializationConstRef hls_flow_step_specialization)
{
   return DesignFlowStep::ComputeSignature(
       HLS, static_cast<unsigned short>(hls_flow_step_type),
       hls_flow_step_specialization ? hls_flow_step_specialization->GetSignatureContext() : 0);
}

std::string HLS_step::GetName() const
{
   return "HLS::" + EnumToName(hls_flow_step_type) +
          (hls_flow_step_specialization ? "(" + hls_flow_step_specialization->GetName() + ")" : "");
}

std::string HLS_step::EnumToName(const HLSFlowStep_Type hls_flow_step_type)
{
   switch(hls_flow_step_type)
   {
      case HLSFlowStep_Type::UNKNOWN:
         return "Unknown";
      case HLSFlowStep_Type::ADD_LIBRARY:
         return "AddLibrary";
      case HLSFlowStep_Type::ALLOCATION:
         return "Allocation";
      case HLSFlowStep_Type::BB_STG_CREATOR:
         return "BBStgCreator";
      case HLSFlowStep_Type::CDFC_MODULE_BINDING:
         return "CdfcModuleBinding";
      case HLSFlowStep_Type::CALL_GRAPH_UNFOLDING:
         return "CallGraphUnfolding";
      case HLSFlowStep_Type::CHORDAL_COLORING_REGISTER_BINDING:
         return "ChordalColoringRegisterBinding";
      case HLSFlowStep_Type::CLASSIC_DATAPATH_CREATOR:
         return "ClassicDatapathCreator";
      case HLSFlowStep_Type::DATAPATH_CS_CREATOR:
         return "DatapathCreatorCS";
      case HLSFlowStep_Type::DATAPATH_CS_PARALLEL_CREATOR:
         return "DatapathCreatorCSParallel";
      case HLSFlowStep_Type::CLASSICAL_HLS_SYNTHESIS_FLOW:
         return "ClassicalHLSSynthesisFlow";
      case HLSFlowStep_Type::COLORING_REGISTER_BINDING:
         return "ColoringRegisterBinding";
      case HLSFlowStep_Type::CONTROL_FLOW_CHECKER:
         return "ControlFlowChecker";
      case HLSFlowStep_Type::C_TESTBENCH_EXECUTION:
         return "CTestbenchExecution";
      case HLSFlowStep_Type::DOMINATOR_ALLOCATION:
         return "DominatorAllocation";
      case HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION:
         return "DominatorMemoryAllocation";
      case HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION_CS:
         return "DominatorMemoryAllocationCS";
      case HLSFlowStep_Type::DOMINATOR_FUNCTION_ALLOCATION:
         return "DominatorFunctionAllocation";
      case HLSFlowStep_Type::DRY_RUN_EVALUATION:
         return "DryRunEvaluation";
      case HLSFlowStep_Type::EASY_MODULE_BINDING:
         return "EasyModuleBinding";
      case HLSFlowStep_Type::EVALUATION:
         return "Evaluation";
      case HLSFlowStep_Type::FSM_CONTROLLER_CREATOR:
         return "FsmControllerCreator";
      case HLSFlowStep_Type::FSM_CS_CONTROLLER_CREATOR:
         return "FsmCSControllerCreator";
      case HLSFlowStep_Type::FSM_NI_SSA_LIVENESS:
         return "FsmNiSsaLiveness";
      case HLSFlowStep_Type::GENERATE_HDL:
         return "GenerateHdl";
#if HAVE_SIMULATION_WRAPPER_BUILT
      case HLSFlowStep_Type::GENERATE_SIMULATION_SCRIPT:
         return "GenerateSimulationScript";
#endif
      case HLSFlowStep_Type::GENERATE_SYNTHESIS_SCRIPT:
         return "GenerateSynthesisScripts";
#if HAVE_TASTE
      case HLSFlowStep_Type::GENERATE_TASTE_HDL_ARCHITECTURE:
         return "GenerateTasteHDLArchitecture";
      case HLSFlowStep_Type::GENERATE_TASTE_SYNTHESIS_SCRIPT:
         return "GenerateTasteSynthesisScript";
#endif
      case HLSFlowStep_Type::HLS_FUNCTION_BIT_VALUE:
         return "HLSFunctionBitValue";
      case HLSFlowStep_Type::HLS_SYNTHESIS_FLOW:
         return "HLSFlow";
      case HLSFlowStep_Type::INITIALIZE_HLS:
         return "InitializeHLS";
      case HLSFlowStep_Type::INTERFACE_CS_GENERATION:
         return "InterfaceCSGeneration";
      case HLSFlowStep_Type::LIST_BASED_SCHEDULING:
         return "ParametricListBased";
      case HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION:
         return "MinimalInterfaceGeneration";
      case HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION:
         return "InferInterfaceGeneration";
      case HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING:
         return "MuxInterconnectionBinding";
#if HAVE_FROM_PRAGMA_BUILT
      case HLSFlowStep_Type::OMP_ALLOCATION:
         return "OmpAllocation";
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case HLSFlowStep_Type::OMP_BODY_LOOP_SYNTHESIS_FLOW:
         return "OmpBodyLoopSynthesisFlow";
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case HLSFlowStep_Type::OMP_FOR_WRAPPER_CS_SYNTHESIS_FLOW:
         return "OmpForWrapperCSSynthesisFlow";
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case HLSFlowStep_Type::OMP_FUNCTION_ALLOCATION:
         return "OmpFunctionAllocation";
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case HLSFlowStep_Type::OMP_FUNCTION_ALLOCATION_CS:
         return "OmpFunctionAllocationCS";
#endif
      case HLSFlowStep_Type::PIPELINE_CONTROLLER_CREATOR:
         return "PipelineControllerCreator";
      case HLSFlowStep_Type::PORT_SWAPPING:
         return "PortSwapping";
      case HLSFlowStep_Type::SCHED_CHAINING:
         return "SchedChaining";
#if HAVE_ILP_BUILT
      case HLSFlowStep_Type::SDC_SCHEDULING:
         return "SDCScheduling";
#endif
      case HLSFlowStep_Type::SIMULATION_EVALUATION:
         return "SimulationEvaluation";
      case HLSFlowStep_Type::STANDARD_HLS_FLOW:
         return "StandardHLSFlow";
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT
      case HLSFlowStep_Type::SYNTHESIS_EVALUATION:
         return "SynthesisEvaluation";
#endif
#if HAVE_TASTE
      case HLSFlowStep_Type::TASTE_INTERFACE_GENERATION:
         return "TasteInterfaceGeneration";
#endif
      case HLSFlowStep_Type::TESTBENCH_GENERATION:
         return "TestbenchGeneration";
      case HLSFlowStep_Type::TEST_VECTOR_PARSER:
         return "TestVectorParser";
      case HLSFlowStep_Type::TOP_ENTITY_CREATION:
         return "TopEntityCreation";
      case HLSFlowStep_Type::TOP_ENTITY_CS_CREATION:
         return "TopEntityCSCreation";
      case HLSFlowStep_Type::TOP_ENTITY_CS_PARALLEL_CREATION:
         return "TopEntityCSParallelCreation";
      case HLSFlowStep_Type::TOP_ENTITY_MEMORY_MAPPED_CREATION:
         return "TopEntityMemoryMappedCreation";
      case HLSFlowStep_Type::UNIQUE_MODULE_BINDING:
         return "UniqueModuleBinding";
      case HLSFlowStep_Type::UNIQUE_REGISTER_BINDING:
         return "UniqueRegisterBinding";
      case HLSFlowStep_Type::VALUES_SCHEME_STORAGE_VALUE_INSERTION:
         return "ValuesSchemeStorageValueInsertion";
#if HAVE_VCD_BUILT
      case HLSFlowStep_Type::HW_PATH_COMPUTATION:
         return "HWPathComputation";
      case HLSFlowStep_Type::HW_DISCREPANCY_ANALYSIS:
         return "HWDiscrepancyAnalysis";
      case HLSFlowStep_Type::VCD_SIGNAL_SELECTION:
         return "VcdSignalSelection";
      case HLSFlowStep_Type::VCD_UTILITY:
         return "VcdUtility";
#endif
      case HLSFlowStep_Type::VIRTUAL_DESIGN_FLOW:
         return "VirtualDesignFlow";
      case HLSFlowStep_Type::WB4_INTERCON_INTERFACE_GENERATION:
         return "WB4InterconInterfaceGeneration";
      case HLSFlowStep_Type::WB4_INTERFACE_GENERATION:
         return "WB4InterfaceGeneration";
      case HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING:
         return "WeightedCliqueRegisterBinding";
      case HLSFlowStep_Type::WRITE_HLS_SUMMARY:
         return "WriteHLSSummary";
      default:
         THROW_UNREACHABLE("HLS flow step type does not exist");
   }
   return "";
}

void HLS_step::ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                                    const DesignFlowStep::RelationshipType relationship_type)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing relationships of " + GetName());
   const auto hls_flow_step_factory = GetPointerS<const HLSFlowStepFactory>(CGetDesignFlowStepFactory());
   const auto DFM = design_flow_manager.lock();
   const auto DFG = DFM->CGetDesignFlowGraph();
   const auto CGM = HLSMgr->CGetCallGraphManager();
   const auto& functions = CGM->GetReachedBodyFunctions();
   const auto TreeM = HLSMgr->get_tree_manager();
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TM = HLS_D->get_technology_manager();

   /// check if __builtin_memcpy has to be synthesized
   const auto memcpy_function =
       HLSMgr->Rmem && HLSMgr->Rmem->has_implicit_memcpy() ? TreeM->GetFunction(MEMCPY)->index : 0;
   const auto steps_to_be_created = ComputeHLSRelationships(relationship_type);
   for(auto const& [step_type, step_spec, rel_type] : steps_to_be_created)
   {
      switch(rel_type)
      {
         case HLSFlowStep_Relationship::TOP_FUNCTION:
         {
            const auto frontend_flow_step_factory =
                GetPointerS<const FrontendFlowStepFactory>(DFM->CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND));
            const auto call_graph_computation_step =
                DFM->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(COMPLETE_CALL_GRAPH));
            const auto cg_design_flow_step =
                call_graph_computation_step != DesignFlowGraph::null_vertex() ?
                    DFG->CGetNodeInfo(call_graph_computation_step)->design_flow_step :
                    frontend_flow_step_factory->CreateApplicationFrontendFlowStep(COMPLETE_CALL_GRAPH);
            design_flow_step_set.insert(cg_design_flow_step);
            /// Root function cannot be computed at the beginning
            if(boost::num_vertices(*(CGM->CGetCallGraph())) == 0)
            {
               break;
            }
            // TODO: top function and root functions are not the same, here OPT_top_functions_names should be used
            for(const auto top_function : CGM->GetRootFunctions())
            {
               const auto hls_step =
                   DFM->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(step_type, step_spec, top_function));
               const auto design_flow_step =
                   hls_step != DesignFlowGraph::null_vertex() ?
                       DFG->CGetNodeInfo(hls_step)->design_flow_step :
                       hls_flow_step_factory->CreateHLSFlowStep(step_type, top_function, step_spec);
               design_flow_step_set.insert(design_flow_step);
            }
            break;
         }
         case HLSFlowStep_Relationship::ALL_FUNCTIONS:
         {
            const auto frontend_flow_step_factory =
                GetPointerS<const FrontendFlowStepFactory>(DFM->CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND));
            const auto call_graph_computation_step =
                DFM->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(COMPLETE_CALL_GRAPH));
            const auto cg_design_flow_step =
                call_graph_computation_step != DesignFlowGraph::null_vertex() ?
                    DFG->CGetNodeInfo(call_graph_computation_step)->design_flow_step :
                    frontend_flow_step_factory->CreateApplicationFrontendFlowStep(COMPLETE_CALL_GRAPH);
            design_flow_step_set.insert(cg_design_flow_step);
            for(auto const function : functions)
            {
               const auto hls_step =
                   DFM->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(step_type, step_spec, function));
               const auto design_flow_step =
                   hls_step != DesignFlowGraph::null_vertex() ?
                       DFG->CGetNodeInfo(hls_step)->design_flow_step :
                       hls_flow_step_factory->CreateHLSFlowStep(step_type, function, step_spec);
               design_flow_step_set.insert(design_flow_step);
            }
            if(memcpy_function)
            {
               const auto hls_step =
                   DFM->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(step_type, step_spec, memcpy_function));
               const auto design_flow_step =
                   hls_step != DesignFlowGraph::null_vertex() ?
                       DFG->CGetNodeInfo(hls_step)->design_flow_step :
                       hls_flow_step_factory->CreateHLSFlowStep(step_type, memcpy_function, step_spec);
               design_flow_step_set.insert(design_flow_step);
            }
            break;
         }
         case HLSFlowStep_Relationship::WHOLE_APPLICATION:
         {
            const auto hls_step = DFM->GetDesignFlowStep(HLS_step::ComputeSignature(step_type, step_spec));
            const auto design_flow_step = hls_step != DesignFlowGraph::null_vertex() ?
                                              DFG->CGetNodeInfo(hls_step)->design_flow_step :
                                              hls_flow_step_factory->CreateHLSFlowStep(step_type, 0, step_spec);
            design_flow_step_set.insert(design_flow_step);
            break;
         }
         case HLSFlowStep_Relationship::SAME_FUNCTION:
         case HLSFlowStep_Relationship::CALLED_FUNCTIONS:
         {
            /// Managed in HLSFunctionStep::ComputeRelationships
            break;
         }
         default:
            THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computing relationships of " + GetName());
}

DesignFlowStepFactoryConstRef HLS_step::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::HLS);
}
