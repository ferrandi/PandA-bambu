/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file hdl_testbench_generation.cpp
 * @brief Generate HDL testbench for the top-level kernel testing
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "hdl_testbench_generation.hpp"

#include "BackendWrapper.hpp"
#include "Discrepancy.hpp"
#include "HDLGeneratorManager.hpp"
#include "HDL_manager.hpp"
#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "copyrights_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "language_writer.hpp"
#include "library_manager.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_allocation.hpp"
#include "memory_symbol.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "technology_wishbone.hpp"
#include "testbench_generation_constants.hpp"
#include "utility.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <string>
#include <utility>

#define CST_STR_BAMBU_TESTBENCH "bambu_testbench"

#define SETUP_PORT_NAME "setup_port"

HDLTestbenchGeneration::HDLTestbenchGeneration(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                               const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::HDL_TESTBENCH_GENERATION),
      writer(language_writer::create_writer(HDLWriter_Language::VERILOG,
                                            _HLSMgr->get_HLS_device()->get_technology_manager(), _parameters)),
      cir(nullptr),
      mod(nullptr),
      output_sim_directory(parameters->getOption<std::filesystem::path>(OPT_output_hls_directory) / "simulation")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
HDLTestbenchGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::TEST_VECTOR_PARSER, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::VCD_SIGNAL_SELECTION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::TOP_FUNCTION));
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

bool HDLTestbenchGeneration::HasToBeExecuted() const
{
   return true;
}

void HDLTestbenchGeneration::Initialize()
{
   const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = HLSMgr->get_ir_manager()->GetFunction(top_symbols.front());
   const auto top_hls = HLSMgr->get_HLS(top_fnode->index);
   cir = top_hls->top->get_circ();
   THROW_ASSERT(GetPointer<const module_o>(cir), "Not a module");
   mod = GetPointer<const module_o>(cir);
   hdl_testbench_basename = "testbench_" + cir->get_id();
}

DesignFlowStep_Status HDLTestbenchGeneration::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Generating testbench HDL");
   const auto output_directory = parameters->getOption<std::filesystem::path>(OPT_output_directory);
   const structural_managerRef tb_top(new structural_manager(parameters));
   tb_top->set_top_info(CST_STR_BAMBU_TESTBENCH "_impl",
                        structural_type_descriptorRef(new structural_type_descriptor(CST_STR_BAMBU_TESTBENCH "_impl")));
   const auto tb_cir = tb_top->get_circ();
   const auto tb_mod = GetPointerS<module_o>(tb_cir);
   const auto add_internal_connection = [&](structural_objectRef src, structural_objectRef dest) {
      THROW_ASSERT(src->get_kind() == dest->get_kind(), "Port with different types cannot be connected.");
      const auto sig_id = "sig_" + dest->get_id();
      auto sig = tb_cir->find_member(sig_id, signal_o_K, tb_cir);
      if(!sig)
      {
         sig = tb_top->add_sign(sig_id, tb_cir, dest->get_typeRef());
         tb_top->add_connection(dest, sig);
      }
      src->set_type(dest->get_typeRef());
      src->type_resize(STD_GET_SIZE(dest->get_typeRef()));

      tb_top->add_connection(sig, src);
   };

   /// Set some descriptions and legal stuff
   tb_mod->set_description("Testbench top component");
   tb_mod->set_copyright(GENERATED_COPYRIGHT);
   tb_mod->set_authors("Component automatically generated by bambu");
   tb_mod->set_license(GENERATED_LICENSE);

   /// command signal type descriptor
   const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));
   /// add clock port
   const auto clock_port = tb_top->add_port(CLOCK_PORT_NAME, port_o::IN, tb_cir, bool_type);
   GetPointerS<port_o>(clock_port)->set_is_clock(true);

   const auto TechM = HLSMgr->get_HLS_device()->get_technology_manager();
   const auto std_lib_manager = TechM->get_library_manager(LIBRARY_STD);
   HDLGeneratorManager mgm(HLSMgr, parameters);

   // Add top module wrapper
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating top level interface wrapper...");
   const auto top_id = [&]() {
      const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
      THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
      const auto top_fnode = HLSMgr->get_ir_manager()->GetFunction(top_symbols.front());
      return top_fnode->index;
   }();
   const auto top_fb = HLSMgr->CGetFunctionBehavior(top_id);
   const auto top_bh = top_fb->CGetBehavioralHelper();
   mgm.create_generic_module("TestbenchDUT", nullptr, top_fb, LIBRARY_STD, "TestbenchDUT");
   const auto dut = tb_top->add_module_from_technology_library("DUT", "TestbenchDUT", LIBRARY_STD, tb_cir, TechM);
   const auto dut_clock = dut->find_member(CLOCK_PORT_NAME, port_o_K, dut);
   THROW_ASSERT(dut_clock, "");
   tb_top->add_connection(clock_port, dut_clock);

   // Add generated testbench FSM
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating testbench FSM...");
   const auto tb_fsm =
       tb_top->add_module_from_technology_library("SystemFSM", "TestbenchFSM", LIBRARY_STD, tb_cir, TechM);
   tb_fsm->SetParameter(
       "RESFILE",
       "\"\"" +
           proximate_if_subpath(parameters->getOption<std::filesystem::path>(OPT_simulation_output), output_directory)
               .string() +
           "\"\"");
   tb_fsm->SetParameter("RESET_ACTIVE", parameters->getOption<bool>(OPT_reset_level) ? "1" : "0");
   tb_fsm->SetParameter("CLOCK_PERIOD", "2.0");
   tb_fsm->SetParameter("MAX_SIM_CYCLES", parameters->getOption<std::string>(OPT_max_sim_cycles));
   const auto fsm_clock = tb_fsm->find_member(CLOCK_PORT_NAME, port_o_K, tb_fsm);
   tb_top->add_connection(clock_port, fsm_clock);

   const auto fsm_reset = tb_fsm->find_member(RESET_PORT_NAME, port_o_K, tb_fsm);
   const auto fsm_setup = tb_fsm->find_member(SETUP_PORT_NAME, port_o_K, tb_fsm);
   const auto dut_cache_reset_port = dut->find_member(CACHE_RESET_PORT_NAME, port_o_K, dut);
   // if the accelerator has a setup port, it must be connected to the setup signal
   if(dut_cache_reset_port)
   {
      add_internal_connection(dut_cache_reset_port, fsm_setup);
   }
   auto fsm_start = tb_fsm->find_member(START_PORT_NAME, port_o_K, tb_fsm);
   auto dut_done = dut->find_member(DONE_PORT_NAME, port_o_K, dut);
   THROW_ASSERT(dut_done, "DUT done_port is missing.");
   const auto dut_idle = dut->find_member(IDLE_PORT_NAME, port_o_K, dut);
   if(dut_idle)
   {
      const auto sig = tb_top->add_sign("sig_map_" IDLE_PORT_NAME, tb_cir, dut_idle->get_typeRef());
      tb_top->add_connection(dut_idle, sig);
   }

   std::list<structural_objectRef> if_modules;
   const auto interface_type = parameters->getOption<HLSFlowStep_Type>(OPT_interface_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating memory interface...");
   structural_objectRef tb_mem;
   const auto top_fname = top_bh->GetFunctionName();
   const auto function_parameters = top_bh->get_parameters();
   const auto& ifaces = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces;
   bool is_banked = false;
   const auto use_bus = ifaces.find("bus") != ifaces.end();
   if(use_bus)
   {
      const auto& bus_if = ifaces.at("bus");
      if(bus_if.find(FunctionArchitecture::iface_bank_number) != bus_if.end())
      {
         is_banked = std::stoul((bus_if.find(FunctionArchitecture::iface_bank_number)->second)) > 0U;
      }
   }
   bool is_bus_pipelined = parameters->isOption(OPT_bus_pipelined) && parameters->getOption<bool>(OPT_bus_pipelined);

   if(interface_type == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION ||
      interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating TestbenchMEMMinimal...");
      tb_mem =
          tb_top->add_module_from_technology_library("SystemMEM", "TestbenchMEMMinimal", LIBRARY_STD, tb_cir, TechM);
      const auto emulate_bram = parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
                                    MemoryAllocation_Policy::ALL_BRAM ||
                                parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
                                    MemoryAllocation_Policy::EXT_PIPELINED_BRAM;
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Emulate BRAM: " + std::string(emulate_bram ? "true" : "false"));
      tb_mem->SetParameter("EMULATE_BRAM", STR(emulate_bram ? 1 : 0));
      tb_mem->SetParameter("QUEUE_SIZE", STR(HLSMgr->get_parameter()->getOption<unsigned int>(OPT_tb_queue_size)));
      const auto oe_port = dut->find_member(MOUT_OE_PORT_NAME, port_o_K, dut);
      if(oe_port)
      {
         const auto bp_port_tb = tb_mem->find_member(MOUT_BACK_PRESSURE_PORT_NAME, port_o_K, tb_mem);
         bp_port_tb->type_resize(STD_GET_SIZE(oe_port->get_typeRef()));
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Resized Mout_back_pressure_port");
         if(!is_banked)
         {
            const auto m_bp_port_tb = tb_mem->find_member(M_BACK_PRESSURE_PORT_NAME, port_o_K, tb_mem);
            m_bp_port_tb->type_resize(STD_GET_SIZE(oe_port->get_typeRef()));
            structural_objectRef const_obj_mbp =
                tb_top->add_constant("const_M_back_pressure", tb_cir, m_bp_port_tb->get_typeRef(), STR(0));
            tb_top->add_connection(const_obj_mbp, m_bp_port_tb);
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Resized M_back_pressure_port");
         }
         if(!is_bus_pipelined && !is_banked)
         {
            const auto tag_port_tb = tb_mem->find_member(M_TAG_PORT_NAME, port_o_K, tb_mem);
            tag_port_tb->type_resize(STD_GET_SIZE(oe_port->get_typeRef()));
            const auto Mout_tag_port_tb = tb_mem->find_member(MOUT_TAG_PORT_NAME, port_o_K, tb_mem);
            Mout_tag_port_tb->type_resize(STD_GET_SIZE(oe_port->get_typeRef()));
            structural_objectRef const_obj_tag =
                tb_top->add_constant("const_Mout_tag", tb_cir, Mout_tag_port_tb->get_typeRef(), STR(0));
            tb_top->add_connection(const_obj_tag, Mout_tag_port_tb);
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Resized M_tag_port and Mout_tag_port");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generated TestbenchMEMMinimal");
   }
   else if(interface_type == HLSFlowStep_Type::WB4_INTERFACE_GENERATION ||
           interface_type == HLSFlowStep_Type::WB4_INTERCON_INTERFACE_GENERATION)
   {
      tb_mem =
          tb_top->add_module_from_technology_library("SystemMEM", "TestbenchMEMWishboneB4", LIBRARY_STD, tb_cir, TechM);
   }
   else
   {
      THROW_ERROR("Testbench generation for selected interface type is not yet supported.");
   }
   tb_mem->SetParameter("MEM_DELAY_READ", parameters->getOption<std::string>(OPT_bram_high_latency) == "_3" ?
                                              "3" :
                                          parameters->getOption<std::string>(OPT_bram_high_latency) == "_4" ?
                                              "4" :
                                              parameters->getOption<std::string>(OPT_mem_delay_read));
   tb_mem->SetParameter("MEM_DELAY_WRITE", parameters->getOption<std::string>(OPT_bram_high_latency) == "_3" ?
                                               "1" :
                                           parameters->getOption<std::string>(OPT_bram_high_latency) == "_4" ?
                                               "2" :
                                               parameters->getOption<std::string>(OPT_mem_delay_write));
   tb_mem->SetParameter("base_addr", STR(HLSMgr->base_address));
   tb_mem->SetParameter("index",
                        std::to_string(top_bh->GetParameters().size() + (top_bh->GetFunctionReturnType() != 0)));
   if_modules.push_back(tb_mem);

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating handler modules for top level parameters...");
   if(parameters->getOption<bool>(OPT_memory_mapped_top))
   {
      const std::string if_suffix =
          interface_type == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION ? "Minimal" : "WishboneB4";
      const auto master_port_module = "TestbenchArgMap" + if_suffix;
      size_t idx = 0;
      std::list<structural_objectRef> master_ports;
      for(const auto& par : top_bh->GetParameters())
      {
         const auto par_name = top_bh->PrintVariable(par->index);
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Parameter " + par_name);
         const auto par_bitsize = ir_helper::SizeAlloc(par);
         const auto par_symbol = HLSMgr->Rmem->get_symbol(par->index, top_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                        "---Interface: " + STR(par_bitsize) + "-bits memory mapped at " +
                            STR(par_symbol->get_address()));
         const auto master_port = tb_top->add_module_from_technology_library("master_" + par_name, master_port_module,
                                                                             LIBRARY_STD, tb_cir, TechM);
         master_port->SetParameter("index", STR(idx));
         master_port->SetParameter("bitsize", STR(par_bitsize));
         master_port->SetParameter("tgt_addr", STR(par_symbol->get_address()));

         master_ports.push_back(master_port);
         ++idx;
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
      }

      const auto return_type = ir_helper::GetFunctionReturnType(HLSMgr->get_ir_manager()->GetIRNode(top_id));
      if(return_type)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Return value port");
         const auto return_bitsize = ir_helper::SizeAlloc(return_type);
         const auto return_symbol = HLSMgr->Rmem->get_symbol(return_type->index, top_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                        "---Interface: " + STR(return_bitsize) + "-bits memory mapped at " +
                            STR(return_symbol->get_address()));
         const auto master_port = tb_top->add_module_from_technology_library(
             "master_return_port", "TestbenchReturnMap" + if_suffix, LIBRARY_STD, tb_cir, TechM);
         master_port->SetParameter("index", STR(idx));
         master_port->SetParameter("bitsize", STR(return_bitsize));
         master_port->SetParameter("tgt_addr", STR(return_symbol->get_address()));

         const auto m_i_done = master_port->find_member("i_" DONE_PORT_NAME, port_o_K, master_port);
         const auto m_done = master_port->find_member(DONE_PORT_NAME, port_o_K, master_port);
         THROW_ASSERT(m_i_done, "Port i_" DONE_PORT_NAME " not found in module " + master_port->get_path());
         THROW_ASSERT(m_done, "Port " DONE_PORT_NAME " not found in module " + master_port->get_path());
         const auto sig = tb_top->add_sign("sig_map_" DONE_PORT_NAME, tb_cir, dut_done->get_typeRef());
         tb_top->add_connection(dut_done, sig);
         tb_top->add_connection(sig, m_i_done);
         dut_done = m_done;

         master_ports.push_back(master_port);
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
      }

      const auto start_symbol = HLSMgr->Rmem->get_symbol(top_id, top_id);
      const auto master_start = tb_top->add_module_from_technology_library(
          "start_master", "TestbenchStartMap" + if_suffix, LIBRARY_STD, tb_cir, TechM);
      master_start->SetParameter("tgt_addr", STR(start_symbol->get_address()));
      master_ports.push_back(master_start);

      if_modules.insert(if_modules.end(), master_ports.begin(), master_ports.end());
      if(master_ports.size())
      {
         const auto master_mod = GetPointerS<module_o>(master_ports.front());
         unsigned int k = 0;

         // Daisy chain start signal through all memory master modules
         for(const auto& master_port : master_ports)
         {
            const auto m_i_start = master_port->find_member("i_" START_PORT_NAME, port_o_K, master_port);
            const auto m_start = master_port->find_member(START_PORT_NAME, port_o_K, master_port);
            THROW_ASSERT(m_i_start, "Port i_" START_PORT_NAME " not found in module " + master_port->get_path());
            THROW_ASSERT(m_start, "Port " START_PORT_NAME " not found in module " + master_port->get_path());
            const auto sig = tb_top->add_sign("sig_" START_PORT_NAME "_" + STR(k), tb_cir, fsm_start->get_typeRef());
            tb_top->add_connection(fsm_start, sig);
            tb_top->add_connection(sig, m_i_start);
            fsm_start = m_start;
            ++k;
         }

         // Merge all matching out signals from memory master modules and testbench memory
         master_ports.push_front(tb_mem);
         for(unsigned int i = 0; i < master_mod->get_out_port_size(); ++i)
         {
            const auto out_port = master_mod->get_out_port(i);
            if(!GetPointerS<const port_o>(out_port)->get_is_memory())
            {
               continue;
            }
            const auto bus_merger = tb_top->add_module_from_technology_library(
                "merge_" + out_port->get_id(), "bus_merger", LIBRARY_STD, tb_cir, TechM);
            const auto merge_out = GetPointerS<module_o>(bus_merger)->get_out_port(0);
            const auto dut_port = dut->find_member(out_port->get_id(), port_o_K, dut);
            THROW_ASSERT(dut_port, "Port " + out_port->get_id() + " not found in module " + dut->get_path());
            add_internal_connection(merge_out, dut_port);
            const auto merge_port = GetPointerS<module_o>(bus_merger)->get_in_port(0);
            const auto merge_port_o = GetPointerS<port_o>(merge_port);
            merge_port_o->add_n_ports(static_cast<unsigned int>(master_ports.size()), merge_port);
            merge_port_o->type_resize(STD_GET_SIZE(dut_port->get_typeRef()));
            k = 0;
            for(const auto& master_port : master_ports)
            {
               const auto m_port = master_port->find_member(out_port->get_id(), port_o_K, master_port);
               THROW_ASSERT(m_port, "Port " + out_port->get_id() + " not found in module " + master_port->get_id());
               m_port->type_resize(STD_GET_SIZE(dut_port->get_typeRef()));
               const auto sig =
                   tb_top->add_sign("sig_" + out_port->get_id() + "_" + STR(k), tb_cir, dut_port->get_typeRef());
               tb_top->add_connection(m_port, sig);
               tb_top->add_connection(sig, merge_port_o->get_port(k));
               ++k;
            }
         }
      }
   }
   else
   {
      // Add interface components relative to each top function parameter
      const auto is_interface_inferred = interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION;
      const auto func_arch = HLSMgr->module_arch->GetArchitecture(top_bh->GetFunctionName());
      size_t interface_index = 0;
      for(const auto& arg : top_bh->GetParameters())
      {
         const auto arg_name = top_bh->PrintVariable(arg->index);
         const auto& parm_attrs = func_arch->parms.at(arg_name);
         const auto& bundle_name = parm_attrs.at(FunctionArchitecture::parm_bundle);
         const auto& iface_attrs = func_arch->ifaces.at(bundle_name);
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Parameter " + arg_name);
         if(is_interface_inferred && ir_helper::IsPointerType(arg) &&
            iface_attrs.find(FunctionArchitecture::iface_direction) == iface_attrs.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--Unused parameter");
            ++interface_index;
            continue;
         }
         const auto arg_port = dut->find_member(arg_name, port_o_K, dut);
         const auto& arg_interface = iface_attrs.at(FunctionArchitecture::iface_mode);

         if(arg_interface == "default")
         {
            const auto arg_port_dir = GetPointer<port_o>(arg_port)->get_port_direction();
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---Interface: " + arg_interface + " " + port_o::GetString(arg_port_dir));
            const auto if_port = tb_top->add_module_from_technology_library(
                "if_" + arg_interface + "_" + arg_name, "IF_PORT_" + port_o::GetString(arg_port_dir), LIBRARY_STD,
                tb_cir, TechM);
            if_modules.push_back(if_port);
            if_port->SetParameter("index", STR(interface_index));

            THROW_ASSERT(arg_port, "Top level interface is missing port for argument '" + arg_name + "'");
            const auto val_port = if_port->find_member("val_port", port_o_K, if_port);
            add_internal_connection(val_port, arg_port);
         }
         else if(arg_interface == "m_axi")
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---Interface: " + arg_interface + " (bundle: " + bundle_name + ")");
            const auto axim_bundle_name = "if_m_axi_" + bundle_name;
            const auto axim_bundle = tb_cir->find_member(axim_bundle_name + "_fu", module_o_K, tb_cir);

            if(!axim_bundle)
            {
               mgm.create_generic_module("TestbenchAXIM", nullptr, top_fb, LIBRARY_STD, axim_bundle_name);
               const auto if_port = tb_top->add_module_from_technology_library(
                   axim_bundle_name + "_fu", axim_bundle_name, LIBRARY_STD, tb_cir, TechM);
               if_modules.push_back(if_port);
               if_port->SetParameter("index", tb_mem->GetParameter("index"));
            }
            const auto if_port = tb_top->add_module_from_technology_library("if_addr_" + arg_name, "IF_PORT_IN",
                                                                            LIBRARY_STD, tb_cir, TechM);
            if_modules.push_back(if_port);
            if_port->SetParameter("index", STR(interface_index));

            THROW_ASSERT(arg_port, "Top level interface is missing port for argument '" + arg_name + "'");
            const auto val_port = if_port->find_member("val_port", port_o_K, if_port);
            add_internal_connection(val_port, arg_port);
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---Interface: " + arg_interface + " " +
                               iface_attrs.at(FunctionArchitecture::iface_direction) +
                               (bundle_name != arg_name ? (" (bundle: " + bundle_name + ")") : ""));
            const auto if_port_name = "if_" + arg_interface + "_" + bundle_name;
            const auto if_port_bundle = tb_cir->find_member(if_port_name + "_fu", module_o_K, tb_cir);
            if(!if_port_bundle)
            {
               mgm.create_generic_module("Testbench" + capitalize(arg_interface), nullptr, top_fb, LIBRARY_STD,
                                         if_port_name);
               const auto if_port = tb_top->add_module_from_technology_library(if_port_name + "_fu", if_port_name,
                                                                               LIBRARY_STD, tb_cir, TechM);
               if_port->SetParameter("index", STR(interface_index));
               if_modules.push_back(if_port);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
         ++interface_index;
      }

      const auto return_port = dut->find_member(RETURN_PORT_NAME, port_o_K, dut);
      if(return_port)
      {
         const auto if_port =
             tb_top->add_module_from_technology_library("if_return_port", "IF_PORT_OUT", LIBRARY_STD, tb_cir, TechM);
         if_modules.push_back(if_port);
         if_port->SetParameter("index", STR(interface_index));

         const auto val_port = if_port->find_member("val_port", port_o_K, if_port);
         add_internal_connection(val_port, return_port);
         ++interface_index;
      }

      const auto dut_start = dut->find_member(START_PORT_NAME, port_o_K, dut);
      THROW_ASSERT(dut_start, "");
      add_internal_connection(fsm_start, dut_start);

      if(use_bus)
      {
         const auto TM = HLSMgr->get_ir_manager();
         const auto fnode = TM->GetIRNode(top_id);
         const auto fname = ir_helper::GetFunctionName(fnode);
         const auto iface_attrs = ifaces.at("bus");
         const auto use_axi_protocol =
             iface_attrs.at(FunctionArchitecture::iface_mode) == "m_axi" ||
             (iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end());

         auto bank_number = 0ULL;
         auto chunk_size = 0ULL;
         if(iface_attrs.find(FunctionArchitecture::iface_bank_number) != iface_attrs.end())
         {
            bank_number = std::stoull(iface_attrs.at(FunctionArchitecture::iface_bank_number));
            if(bank_number > 0ULL)
            {
               chunk_size = std::stoull(iface_attrs.at(FunctionArchitecture::iface_chunk_size));
            }
         }

         const auto external_bit = floor_log2(HLSMgr->base_address);
         const auto bank_number_bitsize = ceil_log2(bank_number);

         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->");
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Banked memory interface information");
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Bank_number required: " + STR(bank_number));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Chunk_size required: " + STR(chunk_size));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---BANK_ID first bit: " + STR(external_bit));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---BANK_ID bitsize: " + STR(bank_number_bitsize));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---BANK_ID last bit: " + STR(external_bit - bank_number_bitsize));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Use axi protocol for banked memory: " + STR(use_axi_protocol));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");

         const auto channel_number = parameters->getOption<unsigned long long>(OPT_channels_number);
         const auto tb_number = std::max(bank_number, channel_number);

         if(use_axi_protocol)
         {
            // Creates a testbench axi for each axi channel
            for(unsigned i = 0; i < tb_number; i++)
            {
               const auto axim_bundle_name = "if_m_axi_" + STR(i);
               const auto axim_bundle = tb_cir->find_member(axim_bundle_name + "_fu", module_o_K, tb_cir);
               if(!axim_bundle)
               {
                  mgm.create_generic_module("TestbenchAXIM", nullptr, top_fb, LIBRARY_STD, axim_bundle_name);
                  const auto if_port = tb_top->add_module_from_technology_library(
                      axim_bundle_name + "_fu", axim_bundle_name, LIBRARY_STD, tb_cir, TechM);
                  if_port->SetParameter("index", tb_mem->GetParameter("index"));
                  if_modules.push_back(if_port);
               }
            }

            // Adds the bankadapter components for axi tb
            interface_index += 1;
            for(unsigned int i = 0; i < bank_number; i++)
            {
               const auto tb_bank_adapter = tb_top->add_module_from_technology_library(
                   "TBBankAdpater_" + STR(i), "TestbenchBankAdapterAXI", LIBRARY_STD, tb_cir, TechM);
               const auto tb_bank_adapter_mod = GetPointer<module_o>(tb_bank_adapter);
               if_modules.push_back(tb_bank_adapter);

               tb_bank_adapter_mod->SetParameter("BANK_ID_SIZE", STR(bank_number_bitsize));
               tb_bank_adapter_mod->SetParameter("ADAPTER_ID", STR(i));
               tb_bank_adapter_mod->SetParameter("EXTERNAL_BANK_ID_LAST_BIT", STR(external_bit - bank_number_bitsize));
               tb_bank_adapter_mod->SetParameter("index", STR(interface_index));

               const auto bank_adapter_setup_port =
                   tb_bank_adapter->find_member(SETUP_PORT_NAME, port_o_K, tb_bank_adapter);
               if(bank_adapter_setup_port)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                                 "---" + bank_adapter_setup_port->get_path() + " <-> " + fsm_setup->get_path());
                  add_internal_connection(bank_adapter_setup_port, fsm_setup);
               }

               const auto bank_adapter_clock_port =
                   tb_bank_adapter->find_member(CLOCK_PORT_NAME, port_o_K, tb_bank_adapter);
               if(bank_adapter_clock_port)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                                 "---" + bank_adapter_clock_port->get_path() + " <-> " + clock_port->get_path());
                  tb_top->add_connection(clock_port, bank_adapter_clock_port);
               }

               const auto dut_m_axi_awaddr = dut->find_member("m_axi_" + STR(i) + "_awaddr", port_o_K, dut);
               THROW_ASSERT(dut_m_axi_awaddr,
                            "Port m_axi_" + STR(i) + "_awaddr should be present in " + dut->get_path());
               const auto dut_m_axi_araddr = dut->find_member("m_axi_" + STR(i) + "_araddr", port_o_K, dut);
               THROW_ASSERT(dut_m_axi_awaddr,
                            "Port m_axi_" + STR(i) + "_araddr should be present in " + dut->get_path());

               const auto ba_m_axi_awaddr = tb_bank_adapter->find_member("m_axi_awaddr_in", port_o_K, tb_bank_adapter);
               const auto ba_m_axi_araddr = tb_bank_adapter->find_member("m_axi_araddr_in", port_o_K, tb_bank_adapter);

               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                              "---" + ba_m_axi_awaddr->get_path() + " <-> " + dut_m_axi_awaddr->get_path());
               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                              "---" + ba_m_axi_araddr->get_path() + " <-> " + dut_m_axi_araddr->get_path());

               ba_m_axi_awaddr->type_resize(STD_GET_SIZE(dut_m_axi_awaddr->get_typeRef()));
               ba_m_axi_araddr->type_resize(STD_GET_SIZE(dut_m_axi_araddr->get_typeRef()));
               const auto sig_m_axi_awaddr_in =
                   tb_top->add_sign("sign_m_axi_" + STR(i) + "_awaddr_in", tb_cir, ba_m_axi_awaddr->get_typeRef());
               const auto sig_m_axi_araddr_in =
                   tb_top->add_sign("sign_m_axi_" + STR(i) + "_araddr_in", tb_cir, ba_m_axi_araddr->get_typeRef());
               tb_top->add_connection(dut_m_axi_awaddr, sig_m_axi_awaddr_in);
               tb_top->add_connection(sig_m_axi_awaddr_in, ba_m_axi_awaddr);
               tb_top->add_connection(dut_m_axi_araddr, sig_m_axi_araddr_in);
               tb_top->add_connection(sig_m_axi_araddr_in, ba_m_axi_araddr);

               const auto ba_m_axi_awaddr_out = tb_bank_adapter->find_member("m_axi_awaddr", port_o_K, tb_bank_adapter);
               const auto ba_m_axi_araddr_out = tb_bank_adapter->find_member("m_axi_araddr", port_o_K, tb_bank_adapter);
               ba_m_axi_awaddr_out->type_resize(STD_GET_SIZE(ba_m_axi_awaddr->get_typeRef()) + bank_number_bitsize);
               ba_m_axi_araddr_out->type_resize(STD_GET_SIZE(ba_m_axi_araddr->get_typeRef()) + bank_number_bitsize);

               const auto tb_axi = tb_cir->find_member("if_m_axi_" + STR(i) + "_fu", module_o_K, tb_cir);
               THROW_ASSERT(tb_axi, "Creating a TestbenchBankAdapterAXI for not existing bundle, budnle id:" + STR(i));

               const auto tb_m_axi_awaddr = tb_axi->find_member("m_axi_" + STR(i) + "_awaddr", port_o_K, tb_axi);
               const auto tb_m_axi_araddr = tb_axi->find_member("m_axi_" + STR(i) + "_araddr", port_o_K, tb_axi);

               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                              "---" + ba_m_axi_awaddr_out->get_path() + " <-> " + tb_m_axi_awaddr->get_path());
               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                              "---" + ba_m_axi_araddr_out->get_path() + " <-> " + tb_m_axi_araddr->get_path());

               tb_m_axi_awaddr->type_resize(STD_GET_SIZE(ba_m_axi_awaddr_out->get_typeRef()));
               tb_m_axi_araddr->type_resize(STD_GET_SIZE(ba_m_axi_araddr_out->get_typeRef()));
               const auto sig_m_axi_awaddr =
                   tb_top->add_sign("sign_m_axi_" + STR(i) + "_awaddr", tb_cir, tb_m_axi_awaddr->get_typeRef());
               const auto sig_m_axi_araddr =
                   tb_top->add_sign("sign_m_axi_" + STR(i) + "_araddr", tb_cir, tb_m_axi_awaddr->get_typeRef());
               tb_top->add_connection(ba_m_axi_awaddr_out, sig_m_axi_awaddr);
               tb_top->add_connection(sig_m_axi_awaddr, tb_m_axi_awaddr);
               tb_top->add_connection(ba_m_axi_araddr_out, sig_m_axi_araddr);
               tb_top->add_connection(sig_m_axi_araddr, tb_m_axi_araddr);
            }
         }
         else if(is_banked)
         {
            // Adds the bankadapter components for minimal tb
            const auto tb_bank_adapter = tb_top->add_module_from_technology_library(
                "TBBankAdapater", "TestbenchBankAdapter", LIBRARY_STD, tb_cir, TechM);
            const auto tb_bank_adapter_mod = GetPointer<module_o>(tb_bank_adapter);
            if_modules.push_back(tb_bank_adapter);

            tb_bank_adapter_mod->SetParameter("BANK_ID_SIZE", STR(bank_number_bitsize));

            const auto dut_Mout_oe_ram = dut->find_member(MOUT_OE_PORT_NAME, port_o_K, dut);
            const auto dut_Mout_addr_ram = dut->find_member(MOUT_ADDRESSS_RAM_PORT, port_o_K, dut);
            const auto bank_channel_number = STD_GET_SIZE(dut_Mout_oe_ram->get_typeRef());

            tb_bank_adapter_mod->SetParameter("CHANNEL_NUMBER", STR(bank_channel_number));
            tb_bank_adapter_mod->SetParameter(
                "BITSIZE_IN", STR(STD_GET_SIZE(dut_Mout_addr_ram->get_typeRef()) / bank_channel_number));

            tb_bank_adapter_mod->SetParameter("EXTERNAL_BANK_ID_LAST_BIT", STR(external_bit - bank_number_bitsize));
            interface_index += 1;
            tb_bank_adapter_mod->SetParameter("index", STR(interface_index));

            const auto bank_adapter_setup_port =
                tb_bank_adapter->find_member(SETUP_PORT_NAME, port_o_K, tb_bank_adapter);
            if(bank_adapter_setup_port)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                              "---" + bank_adapter_setup_port->get_path() + " <-> " + fsm_setup->get_path());
               add_internal_connection(bank_adapter_setup_port, fsm_setup);
            }

            const auto bank_adapter_clock_port =
                tb_bank_adapter->find_member(CLOCK_PORT_NAME, port_o_K, tb_bank_adapter);
            if(bank_adapter_clock_port)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                              "---" + bank_adapter_clock_port->get_path() + " <-> " + clock_port->get_path());
               tb_top->add_connection(clock_port, bank_adapter_clock_port);
            }

            const auto bank_adapter_input =
                tb_bank_adapter->find_member(MOUT_ADDRESSS_RAM_PORT, port_o_K, tb_bank_adapter);
            const auto bank_adapter_output =
                tb_bank_adapter->find_member(MOUT_ADDRESSS_RAM_PORT_OUT, port_o_K, tb_bank_adapter);

            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---" + bank_adapter_input->get_path() + " <-> " + dut_Mout_addr_ram->get_path());
            bank_adapter_input->type_resize(STD_GET_SIZE(dut_Mout_addr_ram->get_typeRef()));
            add_internal_connection(dut_Mout_addr_ram, bank_adapter_input);

            const auto tb_Mout_addr_ram = tb_mem->find_member(MOUT_ADDRESSS_RAM_PORT, port_o_K, tb_mem);
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---" + bank_adapter_output->get_path() + " <-> " + tb_Mout_addr_ram->get_path());
            bank_adapter_output->type_resize(bank_channel_number * bank_number_bitsize +
                                             STD_GET_SIZE(dut_Mout_addr_ram->get_typeRef()));
            add_internal_connection(tb_Mout_addr_ram, bank_adapter_output);
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Connecting DUT control ports...");
   {
      const auto dut_reset = dut->find_member(RESET_PORT_NAME, port_o_K, dut);
      THROW_ASSERT(dut_reset, "");
      add_internal_connection(fsm_reset, dut_reset);
      const auto fsm_done = tb_fsm->find_member(DONE_PORT_NAME, port_o_K, tb_fsm);
      add_internal_connection(fsm_done, dut_done);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Connecting testbench modules...");
   for(const auto& if_obj : if_modules)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Module " + if_obj->get_id());
      const auto if_mod = GetPointerS<module_o>(if_obj);
      for(unsigned i = 0; i < if_mod->get_in_port_size(); ++i)
      {
         const auto in_port = if_mod->get_in_port(i);
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                        "Considering port " + in_port->get_path() + " of type " + in_port->get_kind_text());
         if(GetPointerS<port_o>(in_port)->get_connections_size())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---" + in_port->get_path() + " already connected");
            continue;
         }
         if(in_port->get_id() == CLOCK_PORT_NAME)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---" + in_port->get_path() + " <-> " + clock_port->get_path());
            tb_top->add_connection(clock_port, in_port);
         }
         else if(in_port->get_id() == RESET_PORT_NAME)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---" + in_port->get_path() + " <-> " + fsm_reset->get_path());
            add_internal_connection(in_port, fsm_reset);
         }
         else if(in_port->get_id() == SETUP_PORT_NAME)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---" + in_port->get_path() + " <-> " + fsm_setup->get_path());
            add_internal_connection(in_port, fsm_setup);
         }
         else
         {
            const auto dut_port = dut->find_member(in_port->get_id(), port_o_K, dut);
            if(dut_port)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                              "---" + in_port->get_path() + " <-> " + dut_port->get_path());
               add_internal_connection(in_port, dut_port);
            }
            else if(GetPointerS<port_o>(in_port)->get_is_memory())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                              "---Memory port " + in_port->get_id() + " not present in DUT module " + dut->get_path());
            }
            else
            {
               THROW_UNREACHABLE("Port " + in_port->get_id() + " not found in DUT module " + dut->get_path());
            }
         }
      }

      for(unsigned i = 0; i < if_mod->get_out_port_size(); ++i)
      {
         const auto out_port = if_mod->get_out_port(i);
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                        "Considering port " + out_port->get_path() + " of type " + out_port->get_kind_text());
         if(GetPointerS<port_o>(out_port)->get_connections_size())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---" + out_port->get_path() + " already connected");
            continue;
         }
         const auto dut_port = dut->find_member(out_port->get_id(), port_o_K, dut);
         if(dut_port)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---" + out_port->get_path() + " <-> " + dut_port->get_path());
            add_internal_connection(out_port, dut_port);
         }
         else if(GetPointerS<port_o>(out_port)->get_is_memory())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---Memory port " + out_port->get_id() + " not present in DUT module " + dut->get_path());
         }
         else
         {
            THROW_UNREACHABLE("Port " + out_port->get_id() + " not found in DUT module " + dut->get_path());
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating testbench HDL...");
   const auto tb_filename = output_sim_directory / CST_STR_BAMBU_TESTBENCH;
   const auto is_sim_verilator = parameters->getOption<std::string>(OPT_simulator) == "VERILATOR";
   HDL_manager HDLMgr(HLSMgr, HLSMgr->get_HLS_device(), parameters);
   std::list<std::string> hdl_files, aux_files;
   const std::list<structural_objectRef> tb_circuits = {tb_cir};
   HDLMgr.hdl_gen(tb_filename, tb_circuits, hdl_files, aux_files, HDL_OUT_MIX);
   THROW_ASSERT(hdl_files.size() == 1, "Expected single testbench file");
   THROW_ASSERT(aux_files.size() <= 1, "Expected at most a single testbench aux file");
   auto testbench_filename = hdl_files.front();
   if(aux_files.size())
   {
      HLSMgr->aux_files.push_back(hdl_files.front());
      testbench_filename = aux_files.front();
   }
   {
      std::ifstream bambu_tb(testbench_filename);
      std::ofstream bambu_tb_dpi(testbench_filename + ".dpi");

      if(is_sim_verilator)
      {
         bambu_tb_dpi << "// verilator lint_off BLKANDNBLK\n"
                      << "// verilator lint_off BLKSEQ\n\n";
      }

      bambu_tb_dpi << "`timescale 1ns / 1ps\n"
                   << "// CONSTANTS DECLARATION\n"
                   << "`define MAX_COMMENT_LENGTH 1000\n"
                   << "`define INIT_TIME " STR_CST_INIT_TIME "\n\n";

      if(parameters->getOption<int>(OPT_output_level) < OUTPUT_LEVEL_VERY_PEDANTIC)
      {
         bambu_tb_dpi << "`define NDEBUG\n\n";
      }

      bambu_tb_dpi << R"(
`ifdef __M64
typedef longint unsigned ptr_t;
`else
typedef int unsigned ptr_t;
`endif

)";
      bambu_tb_dpi << bambu_tb.rdbuf();

      auto tb_writer = language_writer::create_writer(HDLWriter_Language::VERILOG,
                                                      HLSMgr->get_HLS_device()->get_technology_manager(), parameters);

      tb_writer->write_comment("MODULE DECLARATION\n");
      tb_writer->write("module " CST_STR_BAMBU_TESTBENCH "(" CLOCK_PORT_NAME ");\n");
      tb_writer->write(STR(STD_OPENING_CHAR));
      tb_writer->write("\ninput " CLOCK_PORT_NAME ";\n\n");

      tb_writer->write("initial\n");
      tb_writer->write(STR(STD_OPENING_CHAR));
      tb_writer->write("begin\n");

      /// VCD output generation (optional)
      tb_writer->write("`ifndef VERILATOR\n");
      tb_writer->write_comment("VCD file generation\n");
      const auto vcd_output_filename = (output_sim_directory / "test.vcd").lexically_proximate(output_directory);
      tb_writer->write("$dumpfile(\"" + vcd_output_filename.string() + "\");\n");
#if HAVE_FROM_DISCREPANCY_BUILT
      const auto dumpvars_discrepancy =
          parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy);
      if(dumpvars_discrepancy)
      {
         tb_writer->write("`ifdef GENERATE_VCD_DISCREPANCY\n");
         const auto simulator_supports_dumpvars_directive =
             parameters->getOption<std::string>(OPT_simulator) == "MODELSIM" ||
             parameters->getOption<std::string>(OPT_simulator) == "ICARUS" ||
             parameters->getOption<std::string>(OPT_simulator) == "XSIM";
         if(!simulator_supports_dumpvars_directive ||
            (parameters->getOption<HDLWriter_Language>(OPT_writer_language) == HDLWriter_Language::VHDL) ||
            HLSMgr->RDiscr->selected_vcd_signals.empty())
         {
            tb_writer->write("`define GENERATE_VCD\n");
         }
         else
         {
            for(const auto& sig_scope : HLSMgr->RDiscr->selected_vcd_signals)
            {
               /*
                * since the SignalSelectorVisitor used to select the signals is
                * quite optimistic and it is based only on naming conventions on
                * the signals, it can select more signal than needed or even select
                * some signals that are not present. if this happens, asking the
                * simulator to dump the missing signal through the $dumpvars
                * directive would result in an error, aborting the simulation. for
                * this reason we use the dumpvars directive to select only the
                * scopes, and we then print all the signals in the scope, without
                * naming them one-by-one
                */
               const auto sigscope = boost::replace_all_copy(sig_scope.first, HIERARCHY_SEPARATOR, ".");
               for(const auto& signame : sig_scope.second)
               {
                  tb_writer->write("$dumpvars(1, " + sigscope + HDL_manager::convert_to_identifier(signame) + ");\n");
               }
            }
         }
         tb_writer->write("`else\n");
      }
#endif

      tb_writer->write("`ifdef GENERATE_VCD\n");
      tb_writer->write("$dumpvars;\n");
      tb_writer->write("`endif\n");
      if(dumpvars_discrepancy)
      {
         tb_writer->write("`endif\n");
      }
      tb_writer->write("`endif\n");

      tb_writer->write(STR(STD_CLOSING_CHAR));
      tb_writer->write("end\n\n");

      tb_writer->write(tb_cir->get_id() + " system(." CLOCK_PORT_NAME "(" CLOCK_PORT_NAME "));\n\n");

      tb_writer->write(STR(STD_CLOSING_CHAR));
      tb_writer->write("endmodule\n\n");

      tb_writer->write("`ifndef VERILATOR\n");
      tb_writer->write("module clocked_" CST_STR_BAMBU_TESTBENCH ";\n");
      tb_writer->write(STR(STD_OPENING_CHAR));
      tb_writer->write("parameter HALF_CLOCK_PERIOD=1.0;\n");
      tb_writer->write("\nreg " CLOCK_PORT_NAME ";\n");
      tb_writer->write("initial " CLOCK_PORT_NAME " = 1;\n");
      tb_writer->write("always # HALF_CLOCK_PERIOD " CLOCK_PORT_NAME " = !" CLOCK_PORT_NAME ";\n\n");
      tb_writer->write(CST_STR_BAMBU_TESTBENCH " bambu_testbench(." CLOCK_PORT_NAME "(" CLOCK_PORT_NAME "));\n\n");
      tb_writer->write(STR(STD_CLOSING_CHAR));
      tb_writer->write("endmodule\n");
      tb_writer->write("`endif\n\n");

      bambu_tb_dpi << tb_writer->WriteString();

      if(is_sim_verilator)
      {
         bambu_tb_dpi << "// verilator lint_on BLKANDNBLK\n";
         bambu_tb_dpi << "// verilator lint_on BLKSEQ\n";
      }
   }
   std::filesystem::remove(testbench_filename);
   std::filesystem::rename(testbench_filename + ".dpi", testbench_filename);

   auto res = BackendWrapper::LoadResults(parameters);
   auto outputs = res.child("application").child("outputs");
   if(!outputs)
   {
      outputs = res.child("application").append_child("outputs");
   }
   outputs.append_child("testbench").text() = proximate_if_subpath(testbench_filename, output_directory).c_str();
   BackendWrapper::StoreResults(res, parameters);

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
   return DesignFlowStep_Status::SUCCESS;
}
