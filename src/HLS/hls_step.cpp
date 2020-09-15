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
 * @file HLS_step.cpp
 * @brief Implementation of some common methods to manage HLS_step algorithms
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// header include
#include "hls_step.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_analysis includes
#include "application_frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_function_step.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS/memory include
#include "memory.hpp"

/// STL includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <tuple>

/// technology include
#include "technology_manager.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "tree_helper.hpp"
#include "tree_manager.hpp"

HLSFlowStepSpecialization::~HLSFlowStepSpecialization() = default;

HLS_step::HLS_step(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type, const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : DesignFlowStep(_design_flow_manager, _parameters), HLSMgr(_HLSMgr), hls_flow_step_type(_hls_flow_step_type), hls_flow_step_specialization(_hls_flow_step_specialization)
{
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> HLS_step::ComputeHLSRelationships(const DesignFlowStep::RelationshipType) const
{
   return CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>();
}

HLS_step::~HLS_step() = default;

CustomUnorderedMap<std::string, HLSFlowStep_Type> HLS_step::command_line_name_to_enum;

const std::string HLS_step::GetSignature() const
{
   return ComputeSignature(hls_flow_step_type, hls_flow_step_specialization);
}

const std::string HLS_step::ComputeSignature(const HLSFlowStep_Type hls_flow_step_type, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization)
{
   return "HLS::" + std::to_string(static_cast<unsigned int>(hls_flow_step_type)) + (hls_flow_step_specialization ? "::" + hls_flow_step_specialization->GetSignature() : "");
}

const std::string HLS_step::GetName() const
{
   return "HLS::" + GetKindText();
}

const std::string HLS_step::GetKindText() const
{
   return EnumToName(hls_flow_step_type) + (hls_flow_step_specialization ? "(" + hls_flow_step_specialization->GetKindText() + ")" : "");
}

const std::string HLS_step::EnumToName(const HLSFlowStep_Type hls_flow_step_type)
{
   switch(hls_flow_step_type)
   {
      case HLSFlowStep_Type::UNKNOWN:
         return "Unknown";
      case HLSFlowStep_Type::ADD_LIBRARY:
         return "AddLibrary";
      case HLSFlowStep_Type::ALLOCATION:
         return "Allocation";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::AREA_ESTIMATION:
         return "AreaEstimation";
      case HLSFlowStep_Type::AXI4LITE_INTERFACE_GENERATION:
         return "Axi4LiteInterfaceGeneration";
#endif
      case HLSFlowStep_Type::BB_STG_CREATOR:
         return "BBStgCreator";
      case HLSFlowStep_Type::CDFC_MODULE_BINDING:
         return "CdfcModuleBinding";
      case HLSFlowStep_Type::CALL_GRAPH_UNFOLDING:
         return "CallGraphUnfolding";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::CHAINING_BASED_LIVENESS:
         return "ChainingBasedLiveness";
#endif
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
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::CLOCK_SLACK_ESTIMATION:
         return "ClockSlackEstimation";
#endif
      case HLSFlowStep_Type::COLORING_REGISTER_BINDING:
         return "ColoringRegisterBinding";
      case HLSFlowStep_Type::CONTROL_FLOW_CHECKER:
         return "ControlFlowChecker";
      case HLSFlowStep_Type::C_TESTBENCH_EXECUTION:
         return "CTestbenchExecution";
      case HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION:
         return "DominatorMemoryAllocation";
      case HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION_CS:
         return "DominatorMemoryAllocationCS";
      case HLSFlowStep_Type::DOMINATOR_FUNCTION_ALLOCATION:
         return "DominatorFunctionAllocation";
      case HLSFlowStep_Type::DRY_RUN_EVALUATION:
         return "DryRunEvaluation";
#if HAVE_BEAGLE
      case HLSFlowStep_Type::DSE_DESIGN_FLOW:
         return "DseDesignFlow"
#endif
#if HAVE_EXPERIMENTAL
             case HLSFlowStep_Type::DUMP_DESIGN_FLOW : return "DumpDesignFlow";
#endif
      case HLSFlowStep_Type::EASY_MODULE_BINDING:
         return "EasyModuleBinding";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::EDGES_REDUCTION_EVALUATION:
         return "EdgesReductionEvaluation";
      case HLSFlowStep_Type::EPDG_SCHED_CHAINING:
         return "EpdgSchedChaining";
#endif
      case HLSFlowStep_Type::EVALUATION:
         return "Evaluation";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::EXPLORE_MUX_DESIGN_FLOW:
         return "ExploreMuxDesignFlow";
      case HLSFlowStep_Type::EXPORT_PCORE:
         return "ExportPcore";
#endif
      case HLSFlowStep_Type::FIXED_SCHEDULING:
         return "FixedScheduling";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::FSL_INTERFACE_GENERATION:
         return "FslInterfaceGeneration";
#endif
      case HLSFlowStep_Type::FSM_CONTROLLER_CREATOR:
         return "FsmControllerCreator";
      case HLSFlowStep_Type::FSM_CS_CONTROLLER_CREATOR:
         return "FsmCSControllerCreator";
      case HLSFlowStep_Type::FSM_NI_SSA_LIVENESS:
         return "FsmNiSsaLiveness";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::FU_REG_BINDING_DESIGN_FLOW:
         return "FuRegBindingDesignFlow";
#endif
      case HLSFlowStep_Type::GENERATE_HDL:
         return "GenerateHdl";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::GENERATE_RESP:
         return "GenerateResp";
#endif
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
      case HLSFlowStep_Type::HLS_BIT_VALUE:
         return "HLSBitValue";
      case HLSFlowStep_Type::HLS_FUNCTION_BIT_VALUE:
         return "HLSFunctionBitValue";
      case HLSFlowStep_Type::HLS_SYNTHESIS_FLOW:
         return "HLSFlow";
#if HAVE_ILP_BUILT && HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::ILP_NEW_FORM_SCHEDULING:
         return "IlpNewFormScheduling";
      case HLSFlowStep_Type::ILP_SCHEDULING:
         return "IlpScheduling";
#endif
      case HLSFlowStep_Type::INITIALIZE_HLS:
         return "InitializeHLS";
      case HLSFlowStep_Type::INTERFACE_CS_GENERATION:
         return "InterfaceCSGeneration";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::K_COFAMILY_REGISTER_BINDING:
         return "KCofamilyRegisterBinding";
      case HLSFlowStep_Type::LEFT_EDGE_REGISTER_BINDING:
         return "LeftEdgeRegisterBinding";
#endif
      case HLSFlowStep_Type::LIST_BASED_SCHEDULING:
         return "ListBasedScheduling";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::MEMORY_CONFLICT_GRAPH:
         return "MemoryConflictGraph";
#endif
      case HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION:
         return "MinimalInterfaceGeneration";
      case HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION:
         return "InferInterfaceGeneration";
      case HLSFlowStep_Type::MINIMAL_TESTBENCH_GENERATION:
         return "MinimalTestbenchGeneration";
      case HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING:
         return "MuxInterconnectionBinding";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::NPI_INTERFACE_GENERATION:
         return "NpiInterfaceGeneration";
      case HLSFlowStep_Type::NUM_AF_EDGES_EVALUATION:
         return "NumAfEdgesEvaluation";
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case HLSFlowStep_Type::OMP_ALLOCATION:
         return "OmpAllocation";
#endif
#if HAVE_FROM_PRAGMA_BUILT
      case HLSFlowStep_Type::OMP_BODY_LOOP_SYNTHESIS_FLOW:
         return "OmpBodyLoopSynthesisFlow";
#endif
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT
      case HLSFlowStep_Type::OMP_FOR_WRAPPER_SYNTHESIS_FLOW:
         return "OmpForWrapperSynthesisFlow";
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
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::PARALLEL_CONTROLLER_CREATOR:
         return "ParallelControllerCreator";
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
#if HAVE_ILP_BUILT && HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::SILP_SCHEDULING:
         return "SilpScheduling";
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
      case HLSFlowStep_Type::TESTBENCH_MEMORY_ALLOCATION:
         return "TestbenchMemoryAllocation";
      case HLSFlowStep_Type::TESTBENCH_VALUES_C_GENERATION:
         return "TestbenchValuesCGeneration";
      case HLSFlowStep_Type::TESTBENCH_VALUES_XML_GENERATION:
         return "TestbenchValuesXMLGeneration";
      case HLSFlowStep_Type::TEST_VECTOR_PARSER:
         return "TestVectorParser";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::TIME_ESTIMATION:
         return "TimeEstimation";
#endif
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
      case HLSFlowStep_Type::WB4_TESTBENCH_GENERATION:
         return "WB4TestbenchGeneration";
      case HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING:
         return "WeightedCliqueRegisterBinding";
      case HLSFlowStep_Type::WRITE_HLS_SUMMARY:
         return "WriteHLSSummary";
#if HAVE_EXPERIMENTAL
      case HLSFlowStep_Type::XML_HLS_SYNTHESIS_FLOW:
         return "XMLHLSSynthesisFlow";
#endif
      default:
         THROW_UNREACHABLE("HLS flow step type does not exist");
   }
   return "";
}

void HLS_step::ComputeRelationships(DesignFlowStepSet& design_flow_step_set, const DesignFlowStep::RelationshipType relationship_type)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing relationships of " + GetName());
   const auto* hls_flow_step_factory = GetPointer<const HLSFlowStepFactory>(CGetDesignFlowStepFactory());
   const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
   const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();
   CustomOrderedSet<unsigned int> functions = call_graph_manager->GetReachedBodyFunctions();
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const HLS_targetRef HLS_T = HLSMgr->get_HLS_target();
   const technology_managerRef TM = HLS_T->get_technology_manager();

   /// check if __builtin_memcpy has to be synthesized
   if(HLSMgr->Rmem and HLSMgr->Rmem->has_implicit_memcpy())
   {
      unsigned int memcpy_function_id = TreeM->function_index("__internal_bambu_memcpy");
      functions.insert(memcpy_function_id);
   }
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> steps_to_be_created = ComputeHLSRelationships(relationship_type);
   for(auto const& step_to_be_created : steps_to_be_created)
   {
      switch(std::get<2>(step_to_be_created))
      {
         case HLSFlowStep_Relationship::ALL_FUNCTIONS:
         {
            const auto* frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));
            const vertex call_graph_computation_step = design_flow_manager.lock()->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(FUNCTION_ANALYSIS));
            const DesignFlowStepRef cg_design_flow_step =
                call_graph_computation_step ? design_flow_graph->CGetDesignFlowStepInfo(call_graph_computation_step)->design_flow_step : frontend_flow_step_factory->CreateApplicationFrontendFlowStep(FUNCTION_ANALYSIS);
            design_flow_step_set.insert(cg_design_flow_step);
            for(auto const function : functions)
            {
               std::string function_name = tree_helper::normalized_ID(tree_helper::name_function(TreeM, function));
               /// FIXME: temporary deactivated
               if(false) // function already implemented
               {
                  continue;
               }
               vertex hls_step = design_flow_manager.lock()->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(std::get<0>(step_to_be_created), std::get<1>(step_to_be_created), function));
               const DesignFlowStepRef design_flow_step =
                   hls_step ? design_flow_graph->CGetDesignFlowStepInfo(hls_step)->design_flow_step : hls_flow_step_factory->CreateHLSFlowStep(std::get<0>(step_to_be_created), function, std::get<1>(step_to_be_created));
               design_flow_step_set.insert(design_flow_step);
            }
            break;
         }
         case HLSFlowStep_Relationship::SAME_FUNCTION:
         case HLSFlowStep_Relationship::CALLED_FUNCTIONS:
         {
            /// Managed in HLSFunctionStep::ComputeRelationships
            break;
         }
         case HLSFlowStep_Relationship::TOP_FUNCTION:
         {
            const auto* frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));
            const vertex call_graph_computation_step = design_flow_manager.lock()->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(FUNCTION_ANALYSIS));
            const DesignFlowStepRef cg_design_flow_step =
                call_graph_computation_step ? design_flow_graph->CGetDesignFlowStepInfo(call_graph_computation_step)->design_flow_step : frontend_flow_step_factory->CreateApplicationFrontendFlowStep(FUNCTION_ANALYSIS);
            design_flow_step_set.insert(cg_design_flow_step);
            /// Root function cannot be computed at the beginning
            if(boost::num_vertices(*(call_graph_manager->CGetCallGraph())) == 0)
               break;
            for(const auto top_function : call_graph_manager->GetRootFunctions())
            {
               vertex hls_step = design_flow_manager.lock()->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(std::get<0>(step_to_be_created), std::get<1>(step_to_be_created), top_function));
               const DesignFlowStepRef design_flow_step =
                   hls_step ? design_flow_graph->CGetDesignFlowStepInfo(hls_step)->design_flow_step : hls_flow_step_factory->CreateHLSFlowStep(std::get<0>(step_to_be_created), top_function, std::get<1>(step_to_be_created));
               design_flow_step_set.insert(design_flow_step);
            }
            break;
         }
         case HLSFlowStep_Relationship::WHOLE_APPLICATION:
         {
            vertex hls_step = design_flow_manager.lock()->GetDesignFlowStep(HLS_step::ComputeSignature(std::get<0>(step_to_be_created), std::get<1>(step_to_be_created)));
            const DesignFlowStepRef design_flow_step = hls_step ? design_flow_graph->CGetDesignFlowStepInfo(hls_step)->design_flow_step : hls_flow_step_factory->CreateHLSFlowStep(std::get<0>(step_to_be_created), 0, std::get<1>(step_to_be_created));
            design_flow_step_set.insert(design_flow_step);
            break;
         }
         default:
            THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computing relationships of " + GetName());
}

const DesignFlowStepFactoryConstRef HLS_step::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory("HLS");
}
