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
 * @file add_library.cpp
 * @brief Implementation of the class to add the current module to the technology library
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "add_library.hpp"

#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "area_info.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "library_manager.hpp"
#include "memory.hpp"
#include "reg_binding.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"

#include <tuple>

AddLibrarySpecialization::AddLibrarySpecialization(const bool _interfaced) : interfaced(_interfaced)
{
}

std::string AddLibrarySpecialization::GetName() const
{
   return interfaced ? "Interfaced" : "";
}

HLSFlowStepSpecialization::context_t AddLibrarySpecialization::GetSignatureContext() const
{
   return ComputeSignatureContext(ADD_LIBRARY, interfaced);
}

add_library::add_library(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned _funId,
                         const DesignFlowManager& _design_flow_manager,
                         const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::ADD_LIBRARY,
                      _hls_flow_step_specialization)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
add_library::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   const auto* const add_library_specialization =
       GetPointer<const AddLibrarySpecialization>(hls_flow_step_specialization);
   THROW_ASSERT(hls_flow_step_specialization, "Empty specialization type");
   THROW_ASSERT(add_library_specialization, "Wrong specialization type: " + hls_flow_step_specialization->GetName());
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         if(add_library_specialization->interfaced)
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type),
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(HLSFlowStep_Type::BUS_INTERFACE_GENERATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm),
                                       HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION)); // add dependence to omp_function

            const auto is_context_handler = [&]() {
               const auto FB = HLSMgr->CGetFunctionBehavior(funId);
               const auto omp_info = FB->GetOMPInfo();
               return FB->IsOMPCore() && omp_info && omp_info->context_count > 1U;
            }();
            if(is_context_handler)
            {
               const HLSFlowStep_Type top_entity_type = HLSFlowStep_Type::TOP_ENTITY_OMP_CS_CREATION;
               ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(),
                                          HLSFlowStep_Relationship::SAME_FUNCTION));
            }
            else
            {
               const auto& CGM = HLSMgr->CGetCallGraphManager();
               const HLSFlowStep_Type top_entity_type =
                   HLSMgr->hasToBeInterfaced(funId) &&
                           (CGM.ExistsAddressedFunction() || parameters->getOption<bool>(OPT_memory_mapped_top)) ?
                       HLSFlowStep_Type::TOP_ENTITY_MEMORY_MAPPED_CREATION :
                       HLSFlowStep_Type::TOP_ENTITY_CREATION;
               ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(),
                                          HLSFlowStep_Relationship::SAME_FUNCTION));
            }
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status add_library::InternalExec()
{
   const auto add_library_specialization = GetPointerS<const AddLibrarySpecialization>(hls_flow_step_specialization);
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   THROW_ASSERT(HLS->top, "Top has not been set");
   const auto& module_name = HLS->top->get_circ()->get_typeRef()->id_type;
   const auto TechM = HLS->HLS_D->get_technology_manager();
   const auto wrapped_fu_name = WRAPPED_PROXY_PREFIX + module_name;
   const auto wrapper_tn = TechM->get_fu(wrapped_fu_name, PROXY_LIBRARY);
   if(wrapper_tn)
   {
      TechM->get_library_manager(PROXY_LIBRARY)->remove_fu(wrapped_fu_name);
   }
   const auto proxy_fu_name = PROXY_PREFIX + module_name;
   const auto proxy_tn = TechM->get_fu(proxy_fu_name, PROXY_LIBRARY);
   if(proxy_tn)
   {
      TechM->get_library_manager(PROXY_LIBRARY)->remove_fu(proxy_fu_name);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Adding " + module_name + " to " + WORK_LIBRARY + " - Object is " + HLS->top->get_circ()->get_path());
   TechM->add_resource(WORK_LIBRARY, module_name, HLS->top);
   const auto clock_period_value = HLS->HLS_C->get_clock_period();
   const auto cprf = HLS->HLS_C->get_clock_period_resource_fraction();
   const auto clk = cprf * clock_period_value;
   const auto device = HLS->HLS_D;
   const auto fu = GetPointerS<functional_unit>(TechM->get_fu(module_name, WORK_LIBRARY));
   fu->set_clock_period(clock_period_value);
   fu->set_clock_period_resource_fraction(cprf);
   auto module_parameters = (HLS->top->get_circ() && GetPointer<module_o>(HLS->top->get_circ()) &&
                             GetPointerS<module_o>(HLS->top->get_circ())->get_NP_functionality()) ?
                                GetPointerS<module_o>(HLS->top->get_circ())
                                    ->get_NP_functionality()
                                    ->get_NP_functionality(NP_functionality::LIBRARY) :
                                "";
   if(module_parameters.find(' ') != std::string::npos)
   {
      module_parameters = module_parameters.substr(module_parameters.find(' '));
   }
   fu->CM->add_NP_functionality(HLS->top->get_circ(), NP_functionality::LIBRARY, module_name + module_parameters);
   if(!add_library_specialization->interfaced)
   {
      const auto function_name = BH->GetFunctionName();
      TechM->add_operation(WORK_LIBRARY, module_name, function_name);
      const auto op = GetPointerS<operation>(fu->get_operation(function_name));
      op->primary_inputs_registered = HLS->registered_inputs;
      const auto is_function_pipelined = FB->is_function_pipelined();
      const auto tsymbol = BH->GetFunctionName();
      const auto tarch = HLSMgr->module_arch->GetArchitecture(tsymbol);
      const auto is_dataflow_module =
          tarch && tarch->attrs.find(FunctionArchitecture::func_dataflow_module) != tarch->attrs.end() &&
          tarch->attrs.find(FunctionArchitecture::func_dataflow_module)->second == "1";
      auto fsm_info = HLS->fsm_info;
      if(is_function_pipelined)
      {
         op->bounded = !fsm_info->hasDummyState;
         if(is_dataflow_module && op->bounded && FB->get_initiation_time() > 1)
         {
            op->bounded = false;
         }
      }
      else
      {
         op->bounded = !is_dataflow_module && fsm_info->bounded;
      }
      const auto call_delay =
          HLS->allocation_information ? HLS->allocation_information->estimate_call_delay() : clock_period_value;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Estimated call delay " + STR(call_delay));
      op->time_m = std::make_shared<time_info>();
      if(op->bounded)
      {
         THROW_ASSERT(fsm_info, "FSM not created");
         const auto minCycles = fsm_info->minCycles;
         const auto maxCycles = fsm_info->maxCycles;
         const auto exec_time = [&]() {
            if(minCycles > 1)
            {
               return clk * (minCycles - 1) + call_delay;
            }
            return call_delay;
         }();
         op->time_m->set_execution_time(exec_time, minCycles);
         if(maxCycles > 1)
         {
            if(is_function_pipelined)
            {
               op->time_m->set_stage_period(call_delay);
               op->time_m->set_initiation_time(FB->get_initiation_time());
            }
            else
            {
               op->time_m->set_stage_period(0.0);
            }
         }
         else
         {
            op->time_m->set_stage_period(0.0);
         }
         if(minCycles <= 1 &&
            (HLSMgr->Rmem->get_allocated_space() + HLSMgr->Rmem->get_allocated_parameters_memory()) == 0)
         {
            fu->logical_type = functional_unit::COMBINATIONAL;
         }
      }
      else
      {
         op->time_m->set_execution_time(call_delay, 0);
      }
      op->time_m->set_synthesis_dependent(true);
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                     "Added " + module_name + (op->bounded ? "" : "(unbounded)") + " to WORK_LIBRARY");
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Added " + module_name + " to WORK_LIBRARY");
   }
   fu->area_m = std::make_shared<area_info>();
   fu->area_m->resources[area_info::AREA] = 2000; /// fake number to avoid sharing of functions

   return DesignFlowStep_Status::SUCCESS;
}
