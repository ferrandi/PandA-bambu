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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file taste_interface_generation.cpp
 * @brief Class to generate interface for taste architecture
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "taste_interface_generation.hpp"

///. include
#include "Parameter.hpp"

/// circuit includes
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// constants include
#include "taste_constants.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS/memory include
#include "memory.hpp"
#include "memory_cs.hpp"

/// HLS/module_allocation include
#include "add_library.hpp"

/// HLS/stg include
#include "state_transition_graph_manager.hpp"

/// intermediate_representation/aadl_asn include
#include "aadl_information.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_set.hpp"
#include <tuple>
#include <utility>

/// technology include
#include "technology_manager.hpp"

/// technology/physical_library include
#include "technology_node.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_manager.hpp"

#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

TasteInterfaceGeneration::TasteInterfaceGeneration(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : module_interface(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::TASTE_INTERFACE_GENERATION)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

TasteInterfaceGeneration::~TasteInterfaceGeneration() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> TasteInterfaceGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   const auto temp_ret = module_interface::ComputeHLSRelationships(relationship_type);
   auto ret = temp_ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

DesignFlowStep_Status TasteInterfaceGeneration::InternalExec()
{
   const technology_managerRef TM = HLS->HLS_T->get_technology_manager();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto function_name = FB->CGetBehavioralHelper()->get_function_name();

   const auto aadl_information = HLSMgr->aadl_information;

   /// function name to be synthesized
   const std::string module_name = FB->CGetBehavioralHelper()->get_function_name() + "_taste_interface";

   auto SM_minimal_interface = HLS->top;

   HLS->top = structural_managerRef(new structural_manager(parameters));

   auto SM_taste_interface = HLS->top;

   structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(module_name));

   SM_taste_interface->set_top_info(module_name, module_type);

   structural_objectRef taste_interface_circuit = SM_taste_interface->get_circ();
   ///   auto taste_interface_module = GetPointer<const module>(taste_interface_circuit);

   taste_interface_circuit->set_black_box(false);

   /// Set some descriptions and legal stuff
   GetPointer<module>(taste_interface_circuit)->set_description("Top circuit for " + module_name);
   GetPointer<module>(taste_interface_circuit)->set_copyright("Copyright (C) 2004-2020 Politecnico di Milano");
   GetPointer<module>(taste_interface_circuit)->set_authors("module automatically generated by bambu");
   GetPointer<module>(taste_interface_circuit)->set_license("PANDA_GPLv3");

   structural_objectRef minimal_interface = SM_minimal_interface->get_circ();
   auto minimal_interface_module = GetPointer<const module>(minimal_interface);
   minimal_interface->set_owner(taste_interface_circuit);
   minimal_interface->set_id("accelerator");

   /// Adding minimal interface module to the taste interface
   GetPointer<module>(taste_interface_circuit)->add_internal_object(minimal_interface);

   /// Checking if minimal interface has memory interface
   const bool with_memory = [&]() -> bool {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking if internal memory has to be instantiated");
      for(unsigned int i = 0; i < minimal_interface_module->get_in_port_size(); i++)
      {
         const structural_objectRef& port_obj = minimal_interface_module->get_in_port(i);
         if(GetPointer<port_o>(port_obj)->get_is_memory())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Port " + GetPointer<port_o>(port_obj)->get_id());
            if(GetPointer<port_o>(port_obj)->get_id().find("M") == 0)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Memory has to be instantiated");
               return true;
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Memory has not to be instantiated");
      return false;
   }();

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding ports");
   /// Add ports
   structural_type_descriptorRef clock_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   SM_taste_interface->add_port(CLOCK_PORT_NAME, port_o::IN, taste_interface_circuit, clock_type);

   structural_type_descriptorRef reset_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   SM_taste_interface->add_port(RESET_PORT_NAME, port_o::IN, taste_interface_circuit, reset_type);

   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   SM_taste_interface->add_port("apbi_psel", port_o::IN, taste_interface_circuit, bool_type);
   SM_taste_interface->add_port("apbi_penable", port_o::IN, taste_interface_circuit, bool_type);
   SM_taste_interface->add_port("apbi_pwrite", port_o::IN, taste_interface_circuit, bool_type);

   structural_type_descriptorRef word_bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 32));
   SM_taste_interface->add_port("apbi_pwdata", port_o::IN, taste_interface_circuit, word_bool_type);
   SM_taste_interface->add_port("apbi_paddr", port_o::IN, taste_interface_circuit, word_bool_type);

   SM_taste_interface->add_port("apbo_prdata", port_o::OUT, taste_interface_circuit, word_bool_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added ports");

   /// Connect minimal interface
   AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, minimal_interface, CLOCK_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, minimal_interface, RESET_PORT_NAME);

   /// Preparing and connecting reg_status
   THROW_ASSERT(parameters->getOption<std::string>(OPT_top_functions_names).find(STR_CST_taste_reg_status) != std::string::npos, STR_CST_taste_reg_status);
#ifndef NDEBUG
   {
      const auto number_of_states = HLSMgr->get_HLS(HLSMgr->get_tree_manager()->function_index(STR_CST_taste_reg_status))->STG->get_number_of_states();
      THROW_ASSERT(number_of_states == 1, STR(number_of_states));
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding reg_status");

   /// Preparing and connecting reg_status_update
   const auto reg_status_update = SM_taste_interface->add_module_from_technology_library("reg_status_update", STR_CST_taste_reg_status, WORK_LIBRARY, taste_interface_circuit, TM);
   const auto done_port_converter = SM_taste_interface->add_module_from_technology_library("done_port_converter", UUDATA_CONVERTER_STD, TM->get_library(UUDATA_CONVERTER_STD), taste_interface_circuit, TM);
   AddSignal(SM_taste_interface, minimal_interface, DONE_PORT_NAME, done_port_converter, "in1", "done_port_output");
   AddSignal(SM_taste_interface, done_port_converter, "out1", reg_status_update, "from_done", "done_port_converter_output");
   AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_pwdata", reg_status_update, "from_outside");
   AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, reg_status_update, CLOCK_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, reg_status_update, RESET_PORT_NAME);
   AddConstant(SM_taste_interface, reg_status_update, START_PORT_NAME, "1", 1);

   /// Preparing and connecting reg_status
   const auto reg_status = SM_taste_interface->add_module_from_technology_library("reg_status", register_SE, TM->get_library(register_SE), taste_interface_circuit, TM);
   const auto shift = SM_taste_interface->add_module_from_technology_library("shift", "ui_rshift_expr_FU", TM->get_library("ui_rshift_expr_FU"), taste_interface_circuit, TM);
   shift->SetParameter("PRECISION", "32");
   GetPointer<port_o>(GetPointer<module>(reg_status)->find_member("in1", port_o_K, reg_status))->type_resize(32);
   GetPointer<port_o>(GetPointer<module>(reg_status)->find_member("out1", port_o_K, reg_status))->type_resize(32);
   GetPointer<port_o>(GetPointer<module>(shift)->find_member("in1", port_o_K, shift))->type_resize(32);
   GetPointer<port_o>(GetPointer<module>(shift)->find_member("out1", port_o_K, shift))->type_resize(32);
   const auto start_port_converter = SM_taste_interface->add_module_from_technology_library("start_port_converter", UUDATA_CONVERTER_STD, TM->get_library(UUDATA_CONVERTER_STD), taste_interface_circuit, TM);
   AddSignal(SM_taste_interface, reg_status, "out1", shift, "in1", "reg_status_output");
   AddConstant(SM_taste_interface, shift, "in2", "3", 3);
   AddSignal(SM_taste_interface, reg_status, "out1", reg_status_update, "current_value", "reg_status_output");
   AddSignal(SM_taste_interface, shift, "out1", start_port_converter, "in1", "shift_output");
   AddSignal(SM_taste_interface, start_port_converter, "out1", minimal_interface, START_PORT_NAME, "start_port_converter_output");
   AddConstant(SM_taste_interface, reg_status, "wenable", "1", 1);
   AddSignal(SM_taste_interface, reg_status_update, "return_port", reg_status, "in1", "reg_status_update_output");
   AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, reg_status, CLOCK_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, reg_status, RESET_PORT_NAME);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added reg_status");

   /// Computing the mask for addresses
   unsigned int addr_range = aadl_information->exposed_memory_sizes[function_name];
   unsigned int index;
   for(index = 1; addr_range >= (1u << index); ++index)
      ;
   const auto relative_address_bitsize = index;
   unsigned int address_mask = 0;
   for(index = 0; index < relative_address_bitsize; index++)
      address_mask += 1u << index;

   /// Preparing masked version of address
   const auto filtered_address = SM_taste_interface->add_module_from_technology_library("filtered_address", "ui_bit_and_expr_FU", TM->get_library("ui_bit_and_expr_FU"), taste_interface_circuit, TM);
   GetPointer<port_o>(GetPointer<module>(filtered_address)->find_member("out1", port_o_K, filtered_address))->type_resize(32);
   AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_paddr", filtered_address, "in1");
   AddConstant(SM_taste_interface, filtered_address, "in2", STR(address_mask), 32);

   /// Preparing and connecting endianess_check
   THROW_ASSERT(parameters->getOption<std::string>(OPT_top_functions_names).find(STR_CST_taste_endianess_check + function_name) != std::string::npos, );
   THROW_ASSERT(HLSMgr->get_HLS(HLSMgr->get_tree_manager()->function_index(STR_CST_taste_endianess_check + function_name))->STG->get_number_of_states() == 1,
                "Number of states of " STR_CST_taste_endianess_check + function_name + " is " + STR(HLSMgr->get_HLS(HLSMgr->get_tree_manager()->function_index(STR_CST_taste_endianess_check + function_name))->STG->get_number_of_states()));
   const auto endianess_check = SM_taste_interface->add_module_from_technology_library("endianess_check", STR_CST_taste_endianess_check + function_name, WORK_LIBRARY, taste_interface_circuit, TM);
   AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_pwdata", endianess_check, "arg");
   AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, endianess_check, CLOCK_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, endianess_check, RESET_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_psel", endianess_check, START_PORT_NAME);

#if 1
   /// Preparing and connecting endianess_inversion
   THROW_ASSERT(parameters->getOption<std::string>(OPT_top_functions_names).find(STR_CST_taste_endianess_inversion) != std::string::npos, "");
   THROW_ASSERT(HLSMgr->get_HLS(HLSMgr->get_tree_manager()->function_index(STR_CST_taste_endianess_inversion))->STG->get_number_of_states() == 1, "");
   const auto swap32_in = SM_taste_interface->add_module_from_technology_library("swap32_in", STR_CST_taste_endianess_inversion, WORK_LIBRARY, taste_interface_circuit, TM);
   const auto swap32_in_cond_expr = SM_taste_interface->add_module_from_technology_library("swap32_in_cond_expr", "ui_cond_expr_FU", TM->get_library("ui_cond_expr_FU"), taste_interface_circuit, TM);
   AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_pwdata", swap32_in, "x");
   AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, swap32_in, CLOCK_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, swap32_in, RESET_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_psel", swap32_in, START_PORT_NAME);

   AddSignal(SM_taste_interface, endianess_check, "return_port", swap32_in_cond_expr, "in1", "endianess_check_output");
   AddSignal(SM_taste_interface, swap32_in, "return_port", swap32_in_cond_expr, "in2", "swap32_in_output");
   AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_pwdata", swap32_in_cond_expr, "in3");
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating parameter registers");
   /// Creating parameter registers
   THROW_ASSERT(aadl_information->function_parameters.find("PI_" + function_name) != aadl_information->function_parameters.end(), "Parameters information of PI_" + function_name + " not found");
   for(const auto& function_parameter : aadl_information->function_parameters.find("PI_" + function_name)->second)
   {
      if(function_parameter.num_registers)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating registers for " + function_parameter.name);
         THROW_ASSERT(function_parameter.num_registers == 1, "Multiple registers not supported");
         const auto param_reg = SM_taste_interface->add_module_from_technology_library(function_parameter.name + "_reg", register_SE, TM->get_library(register_SE), taste_interface_circuit, TM);
         AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, param_reg, CLOCK_PORT_NAME);
         switch(function_parameter.direction)
         {
            case AadlInformation::AadlParameter::Direction::IN:
            {
               /// Resizing register size
               const auto address_compare = SM_taste_interface->add_module_from_technology_library("address_compare_" + function_parameter.name, "ui_eq_expr_FU", TM->get_library("ui_eq_expr_FU"), taste_interface_circuit, TM);
               const auto port = minimal_interface->find_member(function_parameter.name, port_o_K, minimal_interface);
               THROW_ASSERT(port, "");
               const auto port_size = GET_TYPE_SIZE(port);
               GetPointer<port_o>(GetPointer<module>(param_reg)->find_member("in1", port_o_K, param_reg))->type_resize(port_size);
               GetPointer<port_o>(GetPointer<module>(param_reg)->find_member("out1", port_o_K, param_reg))->type_resize(port_size);

               /// Converting size to fit register
               const auto param_converter = SM_taste_interface->add_module_from_technology_library("param_converter_" + function_parameter.name, UUDATA_CONVERTER_STD, TM->get_library(UUDATA_CONVERTER_STD), taste_interface_circuit, TM);
               GetPointer<port_o>(GetPointer<module>(param_converter)->find_member("in1", port_o_K, param_converter))->type_resize(32);
               AddSignal(SM_taste_interface, swap32_in_cond_expr, "out1", param_converter, "in1", "endianess_inversion_output");

               /// Connecting register
               AddSignal(SM_taste_interface, filtered_address, "out1", address_compare, "in1", "filtered_address_output");
               AddConstant(SM_taste_interface, address_compare, "in2", std::to_string(function_parameter.bambu_address), 32);
               AddSignal(SM_taste_interface, address_compare, "out1", param_reg, "wenable", "address_compare_output");
               AddSignal(SM_taste_interface, param_converter, "out1", param_reg, "in1", "param_converter_" + function_parameter.name + "_output");
               AddSignal(SM_taste_interface, param_reg, "out1", minimal_interface, function_parameter.name, function_parameter.name + "_reg_output");
               break;
            }
            case AadlInformation::AadlParameter::Direction::OUT:
            {
               /// Resizing register size
               const auto port = minimal_interface->find_member(RETURN_PORT_NAME, port_o_K, minimal_interface);
               THROW_ASSERT(port, "");
               const auto port_size = GET_TYPE_SIZE(port);
               GetPointer<port_o>(GetPointer<module>(param_reg)->find_member("in1", port_o_K, param_reg))->type_resize(port_size);
               GetPointer<port_o>(GetPointer<module>(param_reg)->find_member("out1", port_o_K, param_reg))->type_resize(port_size);

               /// Connects return port to register
               AddSignal(SM_taste_interface, minimal_interface, RETURN_PORT_NAME, param_reg, "in1", "return_port_output");

               /// Connects done port to register
               AddSignal(SM_taste_interface, minimal_interface, DONE_PORT_NAME, param_reg, "wenable", "done_port_output");

               /// Converting size to fit data bus
               const auto param_converter = SM_taste_interface->add_module_from_technology_library("param_converter_return_port", UUDATA_CONVERTER_STD, TM->get_library(UUDATA_CONVERTER_STD), taste_interface_circuit, TM);
               AddSignal(SM_taste_interface, param_reg, "out1", param_converter, "in1", "return_port_reg_output");
               break;
            }
            case AadlInformation::AadlParameter::Direction::INOUT:
            {
               THROW_UNREACHABLE("");
               break;
            }
            default:
               THROW_UNREACHABLE("");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created registers for " + function_parameter.name);
      }
      else if(function_parameter.pointer)
      {
         AddConstant(SM_taste_interface, minimal_interface, function_parameter.name, STR(function_parameter.bambu_address), 32);
      }
      else
      {
         THROW_UNREACHABLE("Not allocated parameter");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created parameter registers");

   /// Connect and instantiate memory
   structural_objectRef memory;
   if(with_memory)
   {
      memory = SM_taste_interface->add_module_from_technology_library("local_memory", ARRAY_1D_STD_BRAM_NN, LIBRARY_STD_FU, taste_interface_circuit, TM);
      memory->SetParameter("address_space_rangesize", STR(aadl_information->internal_memory_sizes[function_name]));
      memory->SetParameter("BRAM_BITSIZE", STR(HLSMgr->Rmem->get_bram_bitsize()));

      /// component specialization
      unsigned int bus_data_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
      unsigned int bus_addr_bitsize = HLSMgr->get_address_bitsize();
      unsigned int bus_size_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
      unsigned int bus_data_bytesize = HLSMgr->Rmem->get_bus_data_bitsize() / 8;

      unsigned int bus_tag_bitsize = 0;
      if(HLS->Param->isOption(OPT_context_switch))
         bus_tag_bitsize = GetPointer<memory_cs>(HLSMgr->Rmem)->get_bus_tag_bitsize();

      const unsigned int n_elements = aadl_information->internal_memory_sizes[function_name] / bus_data_bytesize + ((aadl_information->internal_memory_sizes[function_name] % bus_data_bytesize) ? 1 : 0);

      memory->SetParameter("n_elements", STR(n_elements));

      std::string init_filename_a = function_name + "_a.data";
      std::string init_filename_b = function_name + "_b.data";
      std::ofstream init_file_a(init_filename_a.c_str());
      std::ofstream init_file_b(init_filename_b.c_str());

      memory->SetParameter("MEMORY_INIT_file_a", "\"\"" + init_filename_a + "\"\"");
      memory->SetParameter("MEMORY_INIT_file_b", "\"\"" + init_filename_b + "\"\"");

      for(unsigned int row_index = 0; row_index < n_elements; row_index++)
      {
         init_file_a << std::string(bus_data_bitsize / 2, '0') << std::endl;
         init_file_b << std::string(bus_data_bitsize / 2, '0') << std::endl;
      }
      init_file_a.close();
      init_file_b.close();

      for(unsigned int i = 0; i < GetPointer<module>(memory)->get_in_port_size(); i++)
      {
         structural_objectRef port = GetPointer<module>(memory)->get_in_port(i);
         if(port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
            GetPointer<port_o>(port)->add_n_ports(2, port);
         if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
            port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
      }

      const auto in1_port = memory->find_member("in1", port_vector_o_K, memory);
      in1_port->type_resize(HLSMgr->Rmem->get_bus_data_bitsize());
      const auto in2_port = memory->find_member("in2", port_vector_o_K, memory);
      in2_port->type_resize(HLSMgr->get_address_bitsize());
      const auto in3_port = memory->find_member("in3", port_vector_o_K, memory);
      in3_port->type_resize(HLSMgr->Rmem->get_bus_size_bitsize());
      /*
            const auto in4_port = memory->find_member("in4", port_vector_o_K, memory);
            GetPointer<port_o>(in4_port)->add_n_ports(2, in4_port);*/
      AddConstant(SM_taste_interface, memory, "in4[0]", "1", 1);
      AddConstant(SM_taste_interface, memory, "in4[1]", "1", 1);

      for(unsigned int i = 0; i < GetPointer<module>(memory)->get_out_port_size(); i++)
      {
         structural_objectRef port = GetPointer<module>(memory)->get_out_port(i);
         if(port->get_kind() == port_vector_o_K && GetPointer<port_o>(port)->get_ports_size() == 0)
            GetPointer<port_o>(port)->add_n_ports(2, port);
         if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
            port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
      }

      CustomSet<std::pair<std::string, std::string>> mem_signals;
      mem_signals.insert(std::pair<std::string, std::string>("M_DataRdy", "Sout_DataRdy"));
      mem_signals.insert(std::pair<std::string, std::string>("M_Rdata_ram", "Sin_Rdata_ram"));
      mem_signals.insert(std::pair<std::string, std::string>("Mout_addr_ram", "S_addr_ram"));
      mem_signals.insert(std::pair<std::string, std::string>("Mout_data_ram_size", "S_data_ram_size"));
      mem_signals.insert(std::pair<std::string, std::string>("Mout_oe_ram", "S_oe_ram"));
      mem_signals.insert(std::pair<std::string, std::string>("Mout_we_ram", "S_we_ram"));
      mem_signals.insert(std::pair<std::string, std::string>("Mout_Wdata_ram", "S_Wdata_ram"));

      for(const auto& mem_signal : mem_signals)
      {
         const auto port1 = minimal_interface->find_member(mem_signal.first, port_vector_o_K, minimal_interface);
         THROW_ASSERT(port1, mem_signal.first + " not found in " + minimal_interface->get_path());
         const auto port2 = memory->find_member(mem_signal.second, port_vector_o_K, memory);
         THROW_ASSERT(port2, mem_signal.second + " not found in " + memory->get_path());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connecting " + port1->get_path() + " to " + port2->get_path());
         add_sign_vector(SM_taste_interface, port1, port2, mem_signal.first);
      }
      AddConstant(SM_taste_interface, memory, "Sin_DataRdy[0]", STR(0), 1);
      AddConstant(SM_taste_interface, memory, "Sin_DataRdy[1]", STR(0), 1);
      AddConstant(SM_taste_interface, memory, "proxy_sel_LOAD[0]", STR(0), 1);
      AddConstant(SM_taste_interface, memory, "proxy_sel_LOAD[1]", STR(0), 1);
      AddConstant(SM_taste_interface, memory, "proxy_sel_STORE[0]", STR(0), 1);
      AddConstant(SM_taste_interface, memory, "proxy_sel_STORE[1]", STR(0), 1);
      const auto proxy_in1_port = memory->find_member("proxy_in1", port_vector_o_K, memory);
      proxy_in1_port->type_resize(HLSMgr->Rmem->get_bus_data_bitsize());
      const auto proxy_in2_port = memory->find_member("proxy_in2", port_vector_o_K, memory);
      proxy_in2_port->type_resize(HLSMgr->get_address_bitsize());
      const auto proxy_in3_port = memory->find_member("proxy_in3", port_vector_o_K, memory);
      proxy_in3_port->type_resize(HLSMgr->Rmem->get_bus_size_bitsize());
      AddConstant(SM_taste_interface, memory, "proxy_in1[0]", STR(0), HLSMgr->Rmem->get_bus_data_bitsize());
      AddConstant(SM_taste_interface, memory, "proxy_in1[1]", STR(0), HLSMgr->Rmem->get_bus_data_bitsize());
      AddConstant(SM_taste_interface, memory, "proxy_in2[0]", STR(0), HLSMgr->get_address_bitsize());
      AddConstant(SM_taste_interface, memory, "proxy_in2[1]", STR(0), HLSMgr->get_address_bitsize());
      AddConstant(SM_taste_interface, memory, "proxy_in3[0]", STR(0), HLSMgr->Rmem->get_bus_size_bitsize());
      AddConstant(SM_taste_interface, memory, "proxy_in3[1]", STR(0), HLSMgr->Rmem->get_bus_size_bitsize());

      /// Preparing and connecting memory_translation
      THROW_ASSERT(parameters->getOption<std::string>(OPT_top_functions_names).find(STR_CST_taste_address_translation + function_name) != std::string::npos, "");
      THROW_ASSERT(HLSMgr->get_HLS(HLSMgr->get_tree_manager()->function_index(STR_CST_taste_address_translation + function_name))->STG->get_number_of_states() == 1, "");
      const auto memory_translation = SM_taste_interface->add_module_from_technology_library("memory_translation", STR_CST_taste_address_translation + function_name, WORK_LIBRARY, taste_interface_circuit, TM);
      AddSignal(SM_taste_interface, filtered_address, "out1", memory_translation, "arg", "memory_translation_arg");
      AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, memory_translation, CLOCK_PORT_NAME);
      AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, memory_translation, RESET_PORT_NAME);
      AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_psel", memory_translation, START_PORT_NAME);

      /// Preparing and connecting data_size
      THROW_ASSERT(parameters->getOption<std::string>(OPT_top_functions_names).find(STR_CST_taste_data_size + function_name) != std::string::npos, "");
      THROW_ASSERT(HLSMgr->get_HLS(HLSMgr->get_tree_manager()->function_index(STR_CST_taste_data_size + function_name))->STG->get_number_of_states() == 1, "");
      const auto data_size = SM_taste_interface->add_module_from_technology_library("data_size", STR_CST_taste_data_size + function_name, WORK_LIBRARY, taste_interface_circuit, TM);
      AddSignal(SM_taste_interface, filtered_address, "out1", data_size, "arg", "data_size_arg");
      AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, data_size, CLOCK_PORT_NAME);
      AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, data_size, RESET_PORT_NAME);
      AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_psel", data_size, START_PORT_NAME);

      /// Preparing and connecting memory_enabling
      THROW_ASSERT(parameters->getOption<std::string>(OPT_top_functions_names).find(STR_CST_taste_memory_enabling + function_name) != std::string::npos, "");
      THROW_ASSERT(HLSMgr->get_HLS(HLSMgr->get_tree_manager()->function_index(STR_CST_taste_memory_enabling + function_name))->STG->get_number_of_states() == 1, "");
      const auto memory_enabling = SM_taste_interface->add_module_from_technology_library("memory_enabling", STR_CST_taste_memory_enabling + function_name, WORK_LIBRARY, taste_interface_circuit, TM);
      AddSignal(SM_taste_interface, filtered_address, "out1", memory_enabling, "arg", "memory_enabling_arg");
      AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, memory_enabling, CLOCK_PORT_NAME);
      AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, memory_enabling, RESET_PORT_NAME);
      AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_psel", memory_enabling, START_PORT_NAME);

      /// Preparing and connecting actual_enabling of the memory
      const auto memory_enabling_converter = SM_taste_interface->add_module_from_technology_library("memory_enabling_converter", UUDATA_CONVERTER_STD, TM->get_library(UUDATA_CONVERTER_STD), taste_interface_circuit, TM);
      const auto actual_memory_enabling = SM_taste_interface->add_module_from_technology_library("actual_memory_enabling", "ui_bit_and_expr_FU", TM->get_library("ui_bit_and_expr_FU"), taste_interface_circuit, TM);
      AddSignal(SM_taste_interface, memory_enabling, "return_port", memory_enabling_converter, "in1", "memory_enabling_output");
      AddSignal(SM_taste_interface, memory_enabling_converter, "out1", actual_memory_enabling, "in1", "memory_enabling_converter_output");
      AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_psel", actual_memory_enabling, "in2");

      /// Prepearing and connecting load enable
      const auto not_write = SM_taste_interface->add_module_from_technology_library("load", "ui_bit_not_expr_FU", TM->get_library("ui_bit_not_expr_FU"), taste_interface_circuit, TM);
      AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_pwrite", not_write, "in1");

      /// Preparing and connecting sel_LOAD of the memory
      const auto sel_LOAD = SM_taste_interface->add_module_from_technology_library("sel_LOAD", "ui_bit_and_expr_FU", TM->get_library("ui_bit_and_expr_FU"), taste_interface_circuit, TM);
      AddSignal(SM_taste_interface, actual_memory_enabling, "out1", sel_LOAD, "in1", "actual_memory_enabling_output");
      AddSignal(SM_taste_interface, not_write, "out1", sel_LOAD, "in2", "not_write_output");

      /// Preparing and connecting sel_STORE of the memory
      const auto sel_STORE = SM_taste_interface->add_module_from_technology_library("sel_STORE", "ui_bit_and_expr_FU", TM->get_library("ui_bit_and_expr_FU"), taste_interface_circuit, TM);
      AddSignal(SM_taste_interface, actual_memory_enabling, "out1", sel_STORE, "in1", "actual_memory_enabling_output");
      AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_pwrite", sel_STORE, "in2");

      /// Connecting input of memory - in1
      AddSignal(SM_taste_interface, swap32_in_cond_expr, "out1", memory, "in1[0]", "endianess_inversion_output");
      AddConstant(SM_taste_interface, memory, "in1[1]", "0", HLSMgr->Rmem->get_bus_data_bitsize());
      /// Connecting input of memory - in2
      const auto in2_converter = SM_taste_interface->add_module_from_technology_library("in2_converter", UUDATA_CONVERTER_STD, TM->get_library(UUDATA_CONVERTER_STD), taste_interface_circuit, TM);
      AddSignal(SM_taste_interface, memory_translation, RETURN_PORT_NAME, in2_converter, "in1", "in2_converter_output");
      AddSignal(SM_taste_interface, in2_converter, "out1", memory, "in2[0]", "in2_0");
      AddConstant(SM_taste_interface, memory, "in2[1]", "0", HLSMgr->get_address_bitsize());
      /// Connecting input of memory - in3
      const auto in3_converter = SM_taste_interface->add_module_from_technology_library("in3_converter", UUDATA_CONVERTER_STD, TM->get_library(UUDATA_CONVERTER_STD), taste_interface_circuit, TM);
      AddSignal(SM_taste_interface, data_size, RETURN_PORT_NAME, in3_converter, "in1", "in3_converter_outpuT");
      AddSignal(SM_taste_interface, in3_converter, "out1", memory, "in3[0]", "in3_0");
      AddConstant(SM_taste_interface, memory, "in3[1]", "0", HLSMgr->Rmem->get_bus_size_bitsize());
      /// Connecting input of memory - sel_LOAD
      AddSignal(SM_taste_interface, sel_LOAD, "out1", memory, "sel_LOAD[0]", "sel_LOAD_output");
      AddConstant(SM_taste_interface, memory, "sel_LOAD[1]", "0", 1);
      /// Connecting input of memory - sel_STORE
      AddSignal(SM_taste_interface, sel_STORE, "out1", memory, "sel_STORE[0]", "sel_STORE_output");
      AddConstant(SM_taste_interface, memory, "sel_STORE[1]", "0", 1);
      /// Connecting input of memory - clock - reset
      AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, memory, CLOCK_PORT_NAME);
      AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, memory, RESET_PORT_NAME);
   }
   /// Preparing output
   THROW_ASSERT(parameters->getOption<std::string>(OPT_top_functions_names).find(STR_CST_taste_output_multiplexer + function_name) != std::string::npos, "");
   THROW_ASSERT(HLSMgr->get_HLS(HLSMgr->get_tree_manager()->function_index(STR_CST_taste_output_multiplexer + function_name))->STG->get_number_of_states() == 1, "");
   const auto output_multiplexer = SM_taste_interface->add_module_from_technology_library("output_multiplexer", STR_CST_taste_output_multiplexer + function_name, WORK_LIBRARY, taste_interface_circuit, TM);
   AddSignal(SM_taste_interface, filtered_address, "out1", output_multiplexer, "address", "memory_enabling_arg");
   AddSignal(SM_taste_interface, reg_status, "out1", output_multiplexer, "reg_status", "reg_status_output");
   const auto return_port = minimal_interface->find_member(RETURN_PORT_NAME, port_o_K, minimal_interface);
   if(return_port)
      AddSignal(SM_taste_interface, minimal_interface, RETURN_PORT_NAME, output_multiplexer, "function_return_port", "return_port_output");
   if(with_memory)
   {
      AddSignal(SM_taste_interface, memory, "out1[0]", output_multiplexer, "from_memory", "from_memory_output");
      const auto fake_register = SM_taste_interface->add_module_from_technology_library("fake_register", register_SE, TM->get_library(register_SE), taste_interface_circuit, TM);
      GetPointer<port_o>(GetPointer<module>(fake_register)->find_member("in1", port_o_K, fake_register))->type_resize(32);
      GetPointer<port_o>(GetPointer<module>(fake_register)->find_member("out1", port_o_K, fake_register))->type_resize(32);
      AddSignal(SM_taste_interface, memory, "out1[1]", fake_register, "in1", "fake_signal");
   }

#if 1
   /// Output endianess conversion
   THROW_ASSERT(parameters->getOption<std::string>(OPT_top_functions_names).find(STR_CST_taste_endianess_inversion) != std::string::npos, "");
   THROW_ASSERT(HLSMgr->get_HLS(HLSMgr->get_tree_manager()->function_index(STR_CST_taste_endianess_inversion))->STG->get_number_of_states() == 1, "");
   const auto swap32_out = SM_taste_interface->add_module_from_technology_library("swap32_out", STR_CST_taste_endianess_inversion, WORK_LIBRARY, taste_interface_circuit, TM);
   const auto swap32_out_cond_expr = SM_taste_interface->add_module_from_technology_library("swap32_iout_cond_expr", "ui_cond_expr_FU", TM->get_library("ui_cond_expr_FU"), taste_interface_circuit, TM);
   AddSignal(SM_taste_interface, output_multiplexer, RETURN_PORT_NAME, swap32_out, "x", "output_multiplexer_output");
   AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, output_multiplexer, CLOCK_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, output_multiplexer, RESET_PORT_NAME);

   AddConnection(SM_taste_interface, taste_interface_circuit, CLOCK_PORT_NAME, swap32_out, CLOCK_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, RESET_PORT_NAME, swap32_out, RESET_PORT_NAME);
   AddConnection(SM_taste_interface, taste_interface_circuit, "apbi_psel", swap32_out, START_PORT_NAME);
   AddSignal(SM_taste_interface, endianess_check, "return_port", swap32_out_cond_expr, "in1", "endianess_check_output");
   AddSignal(SM_taste_interface, swap32_out, "return_port", swap32_out_cond_expr, "in2", "swap32_out_output");
   AddSignal(SM_taste_interface, output_multiplexer, RETURN_PORT_NAME, swap32_out_cond_expr, "in3", "output_multiplexer_output");
   AddSignal(SM_taste_interface, swap32_out_cond_expr, "out1", taste_interface_circuit, "apbo_prdata", "swap32_out_cond_expr_output");
#endif

   taste_interface_circuit->AddParameter("paddr", "0");
   taste_interface_circuit->AddParameter("pindex", "0");
   SM_taste_interface->add_NP_functionality(SM_taste_interface->get_circ(), NP_functionality::LIBRARY, function_name + "_taste_interface paddr pindex");

   SM_taste_interface->INIT(true);

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      SM_taste_interface->WriteDot(FB->CGetBehavioralHelper()->get_function_name() + "/HLS_TasteInterface.dot", structural_manager::COMPLETE_G);
   }
   return DesignFlowStep_Status::SUCCESS;
}

void TasteInterfaceGeneration::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   module_interface::ComputeRelationships(relationship, relationship_type);
   const auto function_name = HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name();
   if(function_name.find(STR_CST_taste_data_size) != std::string::npos)
      return;
   if(function_name.find(STR_CST_taste_memory_enabling) != std::string::npos)
      return;
   if(function_name.find(STR_CST_taste_address_translation) != std::string::npos)
      return;
   if(function_name.find(STR_CST_taste_endianess_check) != std::string::npos)
      return;
   if(function_name.find(STR_CST_taste_endianess_inversion) != std::string::npos)
      return;
   if(function_name.find(STR_CST_taste_reg_status) != std::string::npos)
      return;
   if(function_name.find(STR_CST_taste_output_multiplexer) != std::string::npos)
      return;
   if(relationship_type == DesignFlowStep::DEPENDENCE_RELATIONSHIP)
   {
      const auto hls_flow_step_factory = GetPointer<const HLSFlowStepFactory>(CGetDesignFlowStepFactory());
      const auto TM = HLSMgr->get_tree_manager();

      const auto reg_status_function_id = TM->function_index(STR_CST_taste_reg_status);
      THROW_ASSERT(reg_status_function_id, STR_CST_taste_reg_status);
      const auto reg_status_signature = ComputeSignature(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)), reg_status_function_id);
      const auto reg_status_vertex = design_flow_manager.lock()->GetDesignFlowStep(reg_status_signature);
      const auto reg_status_step = reg_status_vertex != NULL_VERTEX ? design_flow_manager.lock()->CGetDesignFlowGraph()->CGetDesignFlowStepInfo(reg_status_vertex)->design_flow_step :
                                                                      hls_flow_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::ADD_LIBRARY, reg_status_function_id, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)));
      relationship.insert(reg_status_step);

      const auto endianess_inversion_function_id = TM->function_index(STR_CST_taste_endianess_inversion);
      THROW_ASSERT(endianess_inversion_function_id, "");
      const auto endianess_inversion_signature = ComputeSignature(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)), endianess_inversion_function_id);
      const auto endianess_inversion_vertex = design_flow_manager.lock()->GetDesignFlowStep(endianess_inversion_signature);
      const auto endianess_inversion_step = endianess_inversion_vertex != NULL_VERTEX ?
                                                design_flow_manager.lock()->CGetDesignFlowGraph()->CGetDesignFlowStepInfo(endianess_inversion_vertex)->design_flow_step :
                                                hls_flow_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::ADD_LIBRARY, endianess_inversion_function_id, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)));
      relationship.insert(endianess_inversion_step);

      const auto endianess_check_function_id = TM->function_index(STR_CST_taste_endianess_check + function_name);
      THROW_ASSERT(endianess_check_function_id, "");
      const auto endianess_check_signature = ComputeSignature(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)), endianess_check_function_id);
      const auto endianess_check_vertex = design_flow_manager.lock()->GetDesignFlowStep(endianess_check_signature);
      const auto endianess_check_step = endianess_check_vertex != NULL_VERTEX ? design_flow_manager.lock()->CGetDesignFlowGraph()->CGetDesignFlowStepInfo(endianess_check_vertex)->design_flow_step :
                                                                                hls_flow_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::ADD_LIBRARY, endianess_check_function_id, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)));
      relationship.insert(endianess_check_step);

      const auto output_multiplexer_function_id = TM->function_index(STR_CST_taste_output_multiplexer + function_name);
      THROW_ASSERT(output_multiplexer_function_id, "");
      const auto output_multiplexer_signature = ComputeSignature(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)), output_multiplexer_function_id);
      const auto output_multiplexer_vertex = design_flow_manager.lock()->GetDesignFlowStep(output_multiplexer_signature);
      const auto output_multiplexer_step = output_multiplexer_vertex != NULL_VERTEX ?
                                               design_flow_manager.lock()->CGetDesignFlowGraph()->CGetDesignFlowStepInfo(output_multiplexer_vertex)->design_flow_step :
                                               hls_flow_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::ADD_LIBRARY, output_multiplexer_function_id, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)));
      relationship.insert(output_multiplexer_step);

      const auto memory_enabling_function_id = TM->function_index(STR_CST_taste_memory_enabling + function_name);
      if(memory_enabling_function_id)
      {
         const auto memory_enabling_signature = ComputeSignature(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)), memory_enabling_function_id);
         const auto memory_enabling_vertex = design_flow_manager.lock()->GetDesignFlowStep(memory_enabling_signature);
         const auto memory_enabling_step = memory_enabling_vertex != NULL_VERTEX ? design_flow_manager.lock()->CGetDesignFlowGraph()->CGetDesignFlowStepInfo(memory_enabling_vertex)->design_flow_step :
                                                                                   hls_flow_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::ADD_LIBRARY, memory_enabling_function_id, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)));
         relationship.insert(memory_enabling_step);

         const auto data_size_function_id = TM->function_index(STR_CST_taste_data_size + function_name);
         THROW_ASSERT(data_size_function_id, STR_CST_taste_data_size + function_name);
         const auto data_size_signature = ComputeSignature(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)), data_size_function_id);
         const auto data_size_vertex = design_flow_manager.lock()->GetDesignFlowStep(data_size_signature);
         const auto data_size_step = data_size_vertex != NULL_VERTEX ? design_flow_manager.lock()->CGetDesignFlowGraph()->CGetDesignFlowStepInfo(data_size_vertex)->design_flow_step :
                                                                       hls_flow_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::ADD_LIBRARY, data_size_function_id, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)));
         relationship.insert(data_size_step);

         const auto address_translation_function_id = TM->function_index(STR_CST_taste_address_translation + function_name);
         THROW_ASSERT(address_translation_function_id, "");
         const auto address_translation_signature = ComputeSignature(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)), address_translation_function_id);
         const auto address_translation_vertex = design_flow_manager.lock()->GetDesignFlowStep(address_translation_signature);
         const auto address_translation_step = address_translation_vertex != NULL_VERTEX ?
                                                   design_flow_manager.lock()->CGetDesignFlowGraph()->CGetDesignFlowStepInfo(address_translation_vertex)->design_flow_step :
                                                   hls_flow_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::ADD_LIBRARY, address_translation_function_id, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)));
         relationship.insert(address_translation_step);
      }
   }
}

bool TasteInterfaceGeneration::HasToBeExecuted() const
{
   const auto function_name = HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name();
   if(function_name.find(STR_CST_taste_data_size) != std::string::npos)
      return false;
   if(function_name.find(STR_CST_taste_memory_enabling) != std::string::npos)
      return false;
   if(function_name.find(STR_CST_taste_address_translation) != std::string::npos)
      return false;
   if(function_name.find(STR_CST_taste_endianess_check) != std::string::npos)
      return false;
   if(function_name.find(STR_CST_taste_endianess_inversion) != std::string::npos)
      return false;
   if(function_name.find(STR_CST_taste_reg_status) != std::string::npos)
      return false;
   if(function_name.find(STR_CST_taste_output_multiplexer) != std::string::npos)
      return false;
   return module_interface::HasToBeExecuted();
}
