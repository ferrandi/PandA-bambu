
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
 * @file hls_flow_step_factory.cpp
 * @brief Factory for scheduling
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "hls_flow_step_factory.hpp"

#include "CallGraphUnfolding.hpp"
#include "FSMSsaLivenessStep.hpp"
#include "Parameter.hpp"
#include "SchedulingStep.hpp"
#include "TopEntityMemoryMapped.hpp"
#include "WB4Intercon_interface.hpp"
#include "WB4_interface.hpp"
#include "add_library.hpp"
#include "allocation.hpp"
#include "behavioral_helper.hpp"
#include "buildFSM.hpp"
#include "bus_interface.hpp"
#include "c_testbench_generation.hpp"
#include "call_graph_manager.hpp"
#include "cdfc_module_binding.hpp"
#include "chordal_coloring_register.hpp"
#include "classic_datapath.hpp"
#include "classical_synthesis_flow.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_step.hpp"
#include "dominator_allocation.hpp"
#include "easy_module_binding.hpp"
#include "evaluation.hpp"
#include "fsm_controller.hpp"
#include "fun_dominator_allocation.hpp"
#include "function_behavior.hpp"
#include "generate_hdl.hpp"
#include "hdl_testbench_generation.hpp"
#include "hls_function_bit_value.hpp"
#include "hls_synthesis_flow.hpp"
#include "initialize_hls.hpp"
#include "ir_manager.hpp"
#include "mem_dominator_allocation.hpp"
#include "memory.hpp"
#include "minimal_interface.hpp"
#include "mux_connection_binding.hpp"
#include "parametric_list_based.hpp"
#include "sched_based_chaining_computation.hpp"
#include "standard_hls.hpp"
#include "storage_value_insertion.hpp"
#include "string_manipulation.hpp"
#include "test_vector_parser.hpp"
#include "testbench_generation.hpp"
#include "top_entity.hpp"
#include "top_entity_omp_cs.hpp"
#include "unique_binding.hpp"
#include "unique_binding_register.hpp"
#include "vertex_coloring_register.hpp"
#include "virtual_hls.hpp"
#include "weighted_clique_register.hpp"
#include "write_hls_summary.hpp"

#include "sdc_scheduling2.hpp"

#if HAVE_VCD_BUILT
#include "HWPathComputation.hpp"
#include "VcdSignalSelection.hpp"
#include "vcd_utility.hpp"
#endif

#include <string>
#include <utility>

HLSFlowStepFactory::HLSFlowStepFactory(const DesignFlowManager& _design_flow_manager, const HLS_managerRef _HLS_mgr,
                                       const ParameterConstRef _parameters)
    : DesignFlowStepFactory(DesignFlowStep::HLS, _design_flow_manager, _parameters), HLS_mgr(_HLS_mgr)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

DesignFlowStepRef
HLSFlowStepFactory::CreateHLSFlowStep(const HLSFlowStep_Type type, const unsigned int funId,
                                      const HLSFlowStepSpecializationConstRef hls_flow_step_specialization) const
{
   INDENT_DBG_MEX(
       DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
       std::string("-->Creating step HLS::") + HLS_step::EnumToName(type) +
           (hls_flow_step_specialization ? "(" + hls_flow_step_specialization->GetName() + ")" : "") +
           (funId ? "::" + HLS_mgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->GetFunctionName() : ""));
   DesignFlowStepRef design_flow_step = DesignFlowStepRef();
   switch(type)
   {
      case HLSFlowStep_Type::ADD_LIBRARY:
      {
         design_flow_step = DesignFlowStepRef(
             new add_library(parameters, HLS_mgr, funId, design_flow_manager, hls_flow_step_specialization));
         break;
      }
      case HLSFlowStep_Type::ALLOCATION:
      {
         design_flow_step = DesignFlowStepRef(new allocation(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::BUILD_FSM:
      {
         design_flow_step = DesignFlowStepRef(new buildFSM(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::CALL_GRAPH_UNFOLDING:
      {
         design_flow_step = DesignFlowStepRef(new CallGraphUnfolding(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::CDFC_MODULE_BINDING:
      {
         design_flow_step = DesignFlowStepRef(
             new cdfc_module_binding(parameters, HLS_mgr, funId, design_flow_manager, hls_flow_step_specialization));
         break;
      }
      case HLSFlowStep_Type::CHORDAL_COLORING_REGISTER_BINDING:
      {
         design_flow_step =
             DesignFlowStepRef(new chordal_coloring_register(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::CLASSIC_DATAPATH_CREATOR:
      {
         design_flow_step = DesignFlowStepRef(new classic_datapath(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::CLASSICAL_HLS_SYNTHESIS_FLOW:
      {
         design_flow_step = DesignFlowStepRef(new ClassicalHLSSynthesisFlow(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::COLORING_REGISTER_BINDING:
      {
         design_flow_step =
             DesignFlowStepRef(new vertex_coloring_register(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::DOMINATOR_ALLOCATION:
      {
         design_flow_step = DesignFlowStepRef(new dominator_allocation(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::DOMINATOR_FUNCTION_ALLOCATION:
      {
         design_flow_step = DesignFlowStepRef(new fun_dominator_allocation(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION:
      {
         design_flow_step = DesignFlowStepRef(
             new mem_dominator_allocation(parameters, HLS_mgr, design_flow_manager, hls_flow_step_specialization));
         break;
      }
      case HLSFlowStep_Type::EASY_MODULE_BINDING:
      {
         design_flow_step = DesignFlowStepRef(new easy_module_binding(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::LIST_BASED_SCHEDULING:
      {
         design_flow_step = DesignFlowStepRef(
             new parametric_list_based(parameters, HLS_mgr, funId, design_flow_manager, hls_flow_step_specialization));
         break;
      }
      case HLSFlowStep_Type::EVALUATION:
      {
         design_flow_step = DesignFlowStepRef(new Evaluation(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::FSM_CONTROLLER_CREATOR:
      {
         design_flow_step = DesignFlowStepRef(new fsm_controller(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::FSM_NI_SSA_LIVENESS:
      {
         design_flow_step = DesignFlowStepRef(new FSMSsaLivenessStep(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::GENERATE_HDL:
      {
         design_flow_step = DesignFlowStepRef(new generate_hdl(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::HLS_FUNCTION_BIT_VALUE:
      {
         design_flow_step = DesignFlowStepRef(new HLSFunctionBitValue(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::HLS_SYNTHESIS_FLOW:
      {
         design_flow_step = DesignFlowStepRef(new HLSSynthesisFlow(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::HW_PATH_COMPUTATION:
      {
         design_flow_step = DesignFlowStepRef(new HWPathComputation(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::INITIALIZE_HLS:
      {
         design_flow_step = DesignFlowStepRef(new InitializeHLS(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::BUS_INTERFACE_GENERATION:
      {
         design_flow_step = DesignFlowStepRef(new bus_interface(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION:
      {
         design_flow_step = DesignFlowStepRef(new minimal_interface(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION:
      {
         design_flow_step = DesignFlowStepRef(new minimal_interface(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING:
      {
         design_flow_step =
             DesignFlowStepRef(new mux_connection_binding(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::SCHED_CHAINING:
      {
         design_flow_step =
             DesignFlowStepRef(new sched_based_chaining_computation(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::SDC_SCHEDULING:
      {
         design_flow_step = DesignFlowStepRef(
             new SDCScheduling2(parameters, HLS_mgr, funId, design_flow_manager, hls_flow_step_specialization));
         break;
      }
      case HLSFlowStep_Type::STANDARD_HLS_FLOW:
      {
         design_flow_step = DesignFlowStepRef(new standard_hls(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::TESTBENCH_GENERATION:
      {
         design_flow_step = DesignFlowStepRef(new TestbenchGeneration(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::C_TESTBENCH_GENERATION:
      {
         design_flow_step = DesignFlowStepRef(new CTestbenchGeneration(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::HDL_TESTBENCH_GENERATION:
      {
         design_flow_step = DesignFlowStepRef(new HDLTestbenchGeneration(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::TEST_VECTOR_PARSER:
      {
         design_flow_step = DesignFlowStepRef(new TestVectorParser(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::TOP_ENTITY_CREATION:
      {
         design_flow_step = DesignFlowStepRef(new top_entity(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::TOP_ENTITY_OMP_CS_CREATION:
      {
         design_flow_step = DesignFlowStepRef(new top_entity_omp_cs(parameters, HLS_mgr, funId, design_flow_manager,
                                                                    HLSFlowStep_Type::TOP_ENTITY_OMP_CS_CREATION));
         break;
      }

      case HLSFlowStep_Type::TOP_ENTITY_MEMORY_MAPPED_CREATION:
      {
         design_flow_step =
             DesignFlowStepRef(new TopEntityMemoryMapped(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::UNIQUE_MODULE_BINDING:
      {
         design_flow_step = DesignFlowStepRef(new unique_binding(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::UNIQUE_REGISTER_BINDING:
      {
         design_flow_step =
             DesignFlowStepRef(new unique_binding_register(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::VALUES_SCHEME_STORAGE_VALUE_INSERTION:
      {
         design_flow_step =
             DesignFlowStepRef(new storage_value_insertion(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
#if HAVE_VCD_BUILT
      case HLSFlowStep_Type::VCD_SIGNAL_SELECTION:
      {
         design_flow_step = DesignFlowStepRef(new VcdSignalSelection(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::VCD_UTILITY:
      {
         design_flow_step = DesignFlowStepRef(new vcd_utility(parameters, HLS_mgr, design_flow_manager));
         break;
      }
#endif
      case HLSFlowStep_Type::VIRTUAL_DESIGN_FLOW:
      {
         design_flow_step = DesignFlowStepRef(new virtual_hls(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::WB4_INTERFACE_GENERATION:
      {
         design_flow_step = DesignFlowStepRef(new WB4_interface(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::WB4_INTERCON_INTERFACE_GENERATION:
      {
         design_flow_step =
             DesignFlowStepRef(new WB4Intercon_interface(parameters, HLS_mgr, funId, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING:
      {
         design_flow_step = DesignFlowStepRef(new weighted_clique_register(
             parameters, HLS_mgr, funId, design_flow_manager, hls_flow_step_specialization));
         break;
      }
      case HLSFlowStep_Type::WRITE_HLS_SUMMARY:
      {
         design_flow_step = DesignFlowStepRef(new WriteHLSSummary(parameters, HLS_mgr, design_flow_manager));
         break;
      }
      case HLSFlowStep_Type::UNKNOWN:
      {
         THROW_UNREACHABLE("");
         break;
      }
      default:
         THROW_UNREACHABLE("HLSFlowStep algorithm not supported");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created step " + design_flow_step->GetName());
   return design_flow_step;
}

DesignFlowStepRef
HLSFlowStepFactory::CreateHLSFlowStep(const HLSFlowStep_Type type,
                                      const HLSFlowStepSpecializationConstRef hls_flow_step_specialization) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating step " + HLS_step::EnumToName(type));
   DesignFlowStepRef design_flow_step = DesignFlowStepRef();
   switch(type)
   {
      case HLSFlowStep_Type::EVALUATION:
      case HLSFlowStep_Type::GENERATE_HDL:
      case HLSFlowStep_Type::TEST_VECTOR_PARSER:
      case HLSFlowStep_Type::CALL_GRAPH_UNFOLDING:
      case HLSFlowStep_Type::CLASSICAL_HLS_SYNTHESIS_FLOW:
      case HLSFlowStep_Type::DOMINATOR_ALLOCATION:
      case HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION:
      case HLSFlowStep_Type::DOMINATOR_FUNCTION_ALLOCATION:
      case HLSFlowStep_Type::HLS_SYNTHESIS_FLOW:
      case HLSFlowStep_Type::HW_PATH_COMPUTATION:
      case HLSFlowStep_Type::TESTBENCH_GENERATION:
      case HLSFlowStep_Type::C_TESTBENCH_GENERATION:
      case HLSFlowStep_Type::HDL_TESTBENCH_GENERATION:
#if HAVE_VCD_BUILT
      case HLSFlowStep_Type::VCD_SIGNAL_SELECTION:
      case HLSFlowStep_Type::VCD_UTILITY:
#endif
      case HLSFlowStep_Type::WRITE_HLS_SUMMARY:

      {
         design_flow_step = CreateHLSFlowStep(type, 0, hls_flow_step_specialization);
         break;
      }
      case HLSFlowStep_Type::UNKNOWN:
      case HLSFlowStep_Type::ADD_LIBRARY:
      case HLSFlowStep_Type::ALLOCATION:
      case HLSFlowStep_Type::BUILD_FSM:
      case HLSFlowStep_Type::HLS_FUNCTION_BIT_VALUE:
      case HLSFlowStep_Type::CDFC_MODULE_BINDING:
      case HLSFlowStep_Type::CHORDAL_COLORING_REGISTER_BINDING:
      case HLSFlowStep_Type::CLASSIC_DATAPATH_CREATOR:
      case HLSFlowStep_Type::COLORING_REGISTER_BINDING:
      case HLSFlowStep_Type::EASY_MODULE_BINDING:
      case HLSFlowStep_Type::FSM_CONTROLLER_CREATOR:
      case HLSFlowStep_Type::FSM_NI_SSA_LIVENESS:
      case HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION:
      case HLSFlowStep_Type::INITIALIZE_HLS:
      case HLSFlowStep_Type::BUS_INTERFACE_GENERATION:
      case HLSFlowStep_Type::LIST_BASED_SCHEDULING:
      case HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION:
      case HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING:
      case HLSFlowStep_Type::SCHED_CHAINING:
      case HLSFlowStep_Type::SDC_SCHEDULING:
      case HLSFlowStep_Type::STANDARD_HLS_FLOW:
      case HLSFlowStep_Type::TOP_ENTITY_CREATION:
      case HLSFlowStep_Type::TOP_ENTITY_OMP_CS_CREATION:
      case HLSFlowStep_Type::TOP_ENTITY_MEMORY_MAPPED_CREATION:
      case HLSFlowStep_Type::UNIQUE_MODULE_BINDING:
      case HLSFlowStep_Type::UNIQUE_REGISTER_BINDING:
      case HLSFlowStep_Type::VALUES_SCHEME_STORAGE_VALUE_INSERTION:
      case HLSFlowStep_Type::VIRTUAL_DESIGN_FLOW:
      case HLSFlowStep_Type::WB4_INTERCON_INTERFACE_GENERATION:
      case HLSFlowStep_Type::WB4_INTERFACE_GENERATION:
      case HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING:
      default:
         THROW_UNREACHABLE("Step not expected: " + HLS_step::EnumToName(type));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created step " + HLS_step::EnumToName(type));
   return design_flow_step;
}
