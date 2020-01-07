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
 * @file add_library.cpp
 * @brief Implementation of the class to add the current module to the technology library
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "add_library.hpp"
#include "Parameter.hpp" // for ParameterConstRef
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp" // for INDENT_DBG_MEX, DEBUG_LEVEL_VERY_...
#include "exceptions.hpp"     // for THROW_ASSERT, THROW_UNREACHABLE
#include "hls.hpp"            // for HLS_managerRef
#include "hls_manager.hpp"    // for HLS_managerRef
#include "hls_target.hpp"     // for target_deviceRef
#include "library_manager.hpp"
#include "string_manipulation.hpp" // for STR GET_CLASS
#include "technology_manager.hpp"  // for WORK_LIBRARY
#include "technology_node.hpp"     // for functional_unit, operation (ptr o...

#include "fu_binding.hpp"
#include "hls_constraints.hpp"
#include "memory.hpp"
#include "reg_binding.hpp"
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

#include "area_model.hpp"
#include "omp_functions.hpp"
#include "time_model.hpp"

/// circuit include
#include "structural_manager.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// STL includes
#include "custom_set.hpp"
#include <tuple>

AddLibrarySpecialization::AddLibrarySpecialization(const bool _interfaced) : interfaced(_interfaced)
{
}

const std::string AddLibrarySpecialization::GetKindText() const
{
   return interfaced ? "Interfaced" : "";
}

const std::string AddLibrarySpecialization::GetSignature() const
{
   return interfaced ? "Interfaced" : "";
}

add_library::add_library(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::ADD_LIBRARY, _hls_flow_step_specialization)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

add_library::~add_library() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> add_library::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   const auto* const add_library_specialization = GetPointer<const AddLibrarySpecialization>(hls_flow_step_specialization);
   THROW_ASSERT(hls_flow_step_specialization, "Empty specialization type");
   THROW_ASSERT(add_library_specialization, "Wrong specialization type: " + hls_flow_step_specialization->GetKindText());
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(add_library_specialization->interfaced)
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION)); // add dependence to omp_function
            if(HLSMgr->Rfuns)
            {
               bool found = false;
               if(parameters->isOption(OPT_context_switch))
               {
                  auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
                  THROW_ASSERT(omp_functions, "OMP_functions must not be null");
                  if(omp_functions->omp_for_wrappers.find(funId) != omp_functions->omp_for_wrappers.end())
                  {
                     const HLSFlowStep_Type top_entity_type = HLSFlowStep_Type::TOP_ENTITY_CS_PARALLEL_CREATION;
                     ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
                     found = true;
                  }
                  else
                  {
                     if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
                        found = true;
                     if(omp_functions->atomic_functions.find(funId) != omp_functions->atomic_functions.end())
                        found = true;
                     if(omp_functions->parallelized_functions.find(funId) != omp_functions->parallelized_functions.end())
                        found = true;
                     if(found) // use new top_entity
                     {
                        const HLSFlowStep_Type top_entity_type = HLSFlowStep_Type::TOP_ENTITY_CS_CREATION;
                        ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
                     }
                  }
               }
               if(!found) // use standard
               {
                  const auto cg_man = HLSMgr->CGetCallGraphManager();
                  const HLSFlowStep_Type top_entity_type = HLSMgr->hasToBeInterfaced(funId) and (cg_man->ExistsAddressedFunction() or parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION) ?
                                                               HLSFlowStep_Type::TOP_ENTITY_MEMORY_MAPPED_CREATION :
                                                               HLSFlowStep_Type::TOP_ENTITY_CREATION;
                  ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
               }
            }
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
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
   const auto* const add_library_specialization = GetPointer<const AddLibrarySpecialization>(hls_flow_step_specialization);
   const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
   THROW_ASSERT(HLS->top, "Top has not been set");
   std::string module_name = HLS->top->get_circ()->get_typeRef()->id_type;
   const technology_managerRef TM = HLS->HLS_T->get_technology_manager();
   const std::string wrapped_fu_name = WRAPPED_PROXY_PREFIX + module_name;
   technology_nodeRef wrapper_tn = TM->get_fu(wrapped_fu_name, PROXY_LIBRARY);
   if(wrapper_tn)
   {
      TM->get_library_manager(PROXY_LIBRARY)->remove_fu(wrapped_fu_name);
   }
   const std::string proxy_fu_name = PROXY_PREFIX + module_name;
   technology_nodeRef proxy_tn = TM->get_fu(proxy_fu_name, PROXY_LIBRARY);
   if(proxy_tn)
   {
      TM->get_library_manager(PROXY_LIBRARY)->remove_fu(proxy_fu_name);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding " + module_name + " to " + WORK_LIBRARY + " - Object is " + HLS->top->get_circ()->get_path());
   TM->add_resource(WORK_LIBRARY, module_name, HLS->top);
   double clock_period_value = HLS->HLS_C->get_clock_period();
   double cprf = HLS->HLS_C->get_clock_period_resource_fraction();
   double clk = cprf * clock_period_value;
   const target_deviceRef device = HLS->HLS_T->get_target_device();
   auto* fu = GetPointer<functional_unit>(TM->get_fu(module_name, WORK_LIBRARY));
   fu->set_clock_period(clock_period_value);
   fu->set_clock_period_resource_fraction(cprf);
   std::string module_parameters = (HLS->top->get_circ() and GetPointer<module>(HLS->top->get_circ()) and GetPointer<module>(HLS->top->get_circ())->get_NP_functionality()) ?
                                       GetPointer<module>(HLS->top->get_circ())->get_NP_functionality()->get_NP_functionality(NP_functionality::LIBRARY) :
                                       "";
   if(module_parameters.find(" ") != std::string::npos)
      module_parameters = module_parameters.substr(module_parameters.find(" "));
   fu->CM->add_NP_functionality(HLS->top->get_circ(), NP_functionality::LIBRARY, module_name + module_parameters);
   if(!add_library_specialization->interfaced)
   {
      std::string function_name = BH->get_function_name();
      TM->add_operation(WORK_LIBRARY, module_name, function_name);
      auto* op = GetPointer<operation>(fu->get_operation(function_name));
      op->time_m = time_model::create_model(device->get_type(), parameters);
      op->primary_inputs_registered = HLS->registered_inputs;
      /// First computing if operation is bounded, then computing call_delay; call_delay depends on the value of bounded
      if(HLS->STG and HLS->STG->CGetStg()->CGetStateTransitionGraphInfo()->is_a_dag)
      {
         bool is_bounded = !HLSMgr->Rmem->has_proxied_internal_variables(funId) && !parameters->getOption<bool>(OPT_disable_bounded_function);
         const structural_objectRef cir = HLS->top->get_circ();
         auto* mod = GetPointer<module>(cir);
         for(unsigned int i = 0; i < mod->get_in_port_size() && is_bounded; i++)
         {
            const structural_objectRef& port_obj = mod->get_in_port(i);
            if(GetPointer<port_o>(port_obj)->get_is_memory())
               is_bounded = false; /// functions accessing memory are classified as unbounded
         }
         if(is_bounded)
         {
            unsigned int min_cycles = HLS->STG->CGetStg()->CGetStateTransitionGraphInfo()->min_cycles;
            unsigned int max_cycles = HLS->STG->CGetStg()->CGetStateTransitionGraphInfo()->max_cycles;
            if(max_cycles == min_cycles && min_cycles > 0 && min_cycles < 8)
            {
               op->bounded = true;
            }
            else
            {
               op->bounded = false;
            }
         }
         else
         {
            op->bounded = false;
         }
      }
      else
      {
         op->bounded = false;
      }
      double call_delay = HLS->allocation_information ? HLS->allocation_information->estimate_call_delay() : clock_period_value;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Estimated call delay " + STR(call_delay));
      if(op->bounded)
      {
         double exec_time;
         unsigned int min_cycles = HLS->STG->CGetStg()->CGetStateTransitionGraphInfo()->min_cycles;
         unsigned int max_cycles = HLS->STG->CGetStg()->CGetStateTransitionGraphInfo()->max_cycles;
         if(min_cycles > 1)
            exec_time = clk * (min_cycles - 1) + call_delay;
         else
            exec_time = call_delay;
         op->time_m->set_execution_time(exec_time, min_cycles);
         if(max_cycles > 1)
         {
            op->time_m->set_stage_period(call_delay);
            /// FIXME: identification of pipelined functions is disabled since liveness analysis of parameters have not yet been added
            //         if(false and not HLS->Rfu->has_resource_sharing())
            //         {
            //            const ControlStep ii(1);
            //            op->time_m->set_initiation_time(ii);
            //         }
            //         else
            {
               //+1 prevents chaining of two operations mapped on the same functional unit
               const ControlStep ii(max_cycles + 1);
               op->time_m->set_initiation_time(ii);
            }
         }
         else
            op->time_m->set_stage_period(0.0);
         if(min_cycles <= 1 && (HLSMgr->Rmem->get_allocated_space() + HLSMgr->Rmem->get_allocated_parameters_memory()) == 0)
            fu->logical_type = functional_unit::COMBINATIONAL;
      }
      else
      {
         op->time_m->set_execution_time(call_delay, 0);
      }
      op->time_m->set_synthesis_dependent(true);
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Added " + module_name + (op->bounded ? "" : "(unbounded)") + " to WORK_LIBRARY");
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Added " + module_name + " to WORK_LIBRARY");
   }
   fu->area_m = area_model::create_model(device->get_type(), parameters);
   fu->area_m->set_area_value(2000); /// fake number to avoid sharing of functions

   return DesignFlowStep_Status::SUCCESS;
}
