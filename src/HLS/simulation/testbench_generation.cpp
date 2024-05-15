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
 * @file testbench_generation.cpp
 * @brief Generate HDL testbench for the top-level kernel testing
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "testbench_generation.hpp"

#include "Discrepancy.hpp"
#include "HDL_manager.hpp"
#include "ModuleGeneratorManager.hpp"
#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "copyrights_strings.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "library_manager.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_symbol.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "technology_wishbone.hpp"
#include "testbench_generation_constants.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
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

TestbenchGeneration::TestbenchGeneration(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                         const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::TESTBENCH_GENERATION),
      writer(language_writer::create_writer(HDLWriter_Language::VERILOG,
                                            _HLSMgr->get_HLS_device()->get_technology_manager(), _parameters)),
      cir(nullptr),
      mod(nullptr),
      output_directory(parameters->getOption<std::filesystem::path>(OPT_output_directory) / "simulation")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
TestbenchGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
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

bool TestbenchGeneration::HasToBeExecuted() const
{
   return true;
}

void TestbenchGeneration::Initialize()
{
   const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = HLSMgr->get_tree_manager()->GetFunction(top_symbols.front());
   const auto top_hls = HLSMgr->get_HLS(top_fnode->index);
   cir = top_hls->top->get_circ();
   THROW_ASSERT(GetPointer<const module>(cir), "Not a module");
   mod = GetPointer<const module>(cir);
   hdl_testbench_basename = "testbench_" + cir->get_id();
}

DesignFlowStep_Status TestbenchGeneration::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Generating testbench HDL");
   const structural_managerRef tb_top(new structural_manager(parameters));
   tb_top->set_top_info(CST_STR_BAMBU_TESTBENCH "_impl",
                        structural_type_descriptorRef(new structural_type_descriptor(CST_STR_BAMBU_TESTBENCH "_impl")));
   const auto tb_cir = tb_top->get_circ();
   const auto tb_mod = GetPointerS<module>(tb_cir);
   const auto add_internal_connection = [&](structural_objectRef src, structural_objectRef dest) {
      THROW_ASSERT(src->get_kind() == dest->get_kind(), "Port with different types cannot be connected.");
      const auto sig_id = "sig_" + dest->get_id();
      auto sig = tb_cir->find_member(sig_id, signal_o_K, tb_cir);
      if(!sig)
      {
         sig = tb_top->add_sign(sig_id, tb_cir, dest->get_typeRef());
         tb_top->add_connection(dest, sig);
      }
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
   ModuleGeneratorManager mgm(HLSMgr, parameters);

   // Add top module wrapper
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating top level interface wrapper...");
   const auto top_id = [&]() {
      const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
      THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
      const auto top_fnode = HLSMgr->get_tree_manager()->GetFunction(top_symbols.front());
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
   tb_fsm->SetParameter("RESFILE", "\"\"" + parameters->getOption<std::string>(OPT_simulation_output) + "\"\"");
   tb_fsm->SetParameter("RESET_ACTIVE", parameters->getOption<bool>(OPT_reset_level) ? "1" : "0");
   tb_fsm->SetParameter("CLOCK_PERIOD", "2.0");
   tb_fsm->SetParameter("MAX_SIM_CYCLES", parameters->getOption<std::string>(OPT_max_sim_cycles));
   const auto fsm_clock = tb_fsm->find_member(CLOCK_PORT_NAME, port_o_K, tb_fsm);
   tb_top->add_connection(clock_port, fsm_clock);

   const auto fsm_reset = tb_fsm->find_member(RESET_PORT_NAME, port_o_K, tb_fsm);
   const auto fsm_setup = tb_fsm->find_member(SETUP_PORT_NAME, port_o_K, tb_fsm);
   auto fsm_start = tb_fsm->find_member(START_PORT_NAME, port_o_K, tb_fsm);
   auto dut_done = dut->find_member(DONE_PORT_NAME, port_o_K, dut);
   THROW_ASSERT(dut_done, "DUT done_port is missing.");

   std::list<structural_objectRef> if_modules;
   const auto interface_type = parameters->getOption<HLSFlowStep_Type>(OPT_interface_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating memory interface...");
   structural_objectRef tb_mem;
   if(interface_type == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION ||
      interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION ||
      interface_type == HLSFlowStep_Type::INTERFACE_CS_GENERATION)
   {
      tb_mem =
          tb_top->add_module_from_technology_library("SystemMEM", "TestbenchMEMMinimal", LIBRARY_STD, tb_cir, TechM);
      tb_mem->SetParameter("QUEUE_SIZE", STR(HLSMgr->get_parameter()->getOption<unsigned int>(OPT_tb_queue_size)));
      const auto bp_port = dut->find_member(MOUT_BACK_PRESSURE_PORT_NAME, port_o_K, dut);
      if(bp_port ==
         nullptr) // if the the internal memory_ctrl does not use bp I have to set the correct size of the bp signal
      {
         const auto bp_port_tb = tb_mem->find_member(MOUT_BACK_PRESSURE_PORT_NAME, port_o_K, tb_mem);
         const auto oe_port = dut->find_member(MOUT_OE_PORT_NAME, port_o_K, dut);
         if(oe_port)
         {
            bp_port_tb->type_resize(STD_GET_SIZE(oe_port->get_typeRef()));
         }
      }
      if(interface_type == HLSFlowStep_Type::INTERFACE_CS_GENERATION)
      {
         tb_mem->SetParameter("QUEUE_SIZE", STR(1));
      }
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
   tb_mem->SetParameter("MEM_DELAY_WRITE", parameters->getOption<std::string>(OPT_mem_delay_write));
   tb_mem->SetParameter("base_addr", STR(HLSMgr->base_address));
   tb_mem->SetParameter("index",
                        std::to_string(top_bh->GetParameters().size() + (top_bh->GetFunctionReturnType(top_id) != 0)));
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
         const auto par_bitsize = tree_helper::SizeAlloc(par);
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

      const auto return_type = tree_helper::GetFunctionReturnType(HLSMgr->get_tree_manager()->GetTreeNode(top_id));
      if(return_type)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Return value port");
         const auto return_bitsize = tree_helper::SizeAlloc(return_type);
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
         const auto master_mod = GetPointerS<module>(master_ports.front());
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
            const auto merge_out = GetPointerS<module>(bus_merger)->get_out_port(0);
            const auto dut_port = dut->find_member(out_port->get_id(), port_o_K, dut);
            THROW_ASSERT(dut_port, "Port " + out_port->get_id() + " not found in module " + dut->get_path());
            add_internal_connection(merge_out, dut_port);
            const auto merge_port = GetPointerS<module>(bus_merger)->get_in_port(0);
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
      const auto func_arch = HLSMgr->module_arch->GetArchitecture(top_bh->GetMangledFunctionName());
      size_t idx = 0;
      for(const auto& arg : top_bh->GetParameters())
      {
         const auto arg_name = top_bh->PrintVariable(arg->index);
         const auto& parm_attrs = func_arch->parms.at(arg_name);
         const auto& bundle_name = parm_attrs.at(FunctionArchitecture::parm_bundle);
         const auto& iface_attrs = func_arch->ifaces.at(bundle_name);
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Parameter " + arg_name);
         if(is_interface_inferred && tree_helper::IsPointerType(arg) &&
            iface_attrs.find(FunctionArchitecture::iface_direction) == iface_attrs.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--Unused parameter");
            ++idx;
            continue;
         }
         const auto arg_port = dut->find_member(arg_name, port_o_K, dut);
         const auto arg_interface = iface_attrs.at(FunctionArchitecture::iface_mode);

         if(arg_interface == "default")
         {
            const auto arg_port_dir = GetPointer<port_o>(arg_port)->get_port_direction();
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---Interface: " + arg_interface + " " + port_o::GetString(arg_port_dir));
            const auto if_port = tb_top->add_module_from_technology_library(
                "if_" + arg_interface + "_" + arg_name, "IF_PORT_" + port_o::GetString(arg_port_dir), LIBRARY_STD,
                tb_cir, TechM);
            if_modules.push_back(if_port);
            if_port->SetParameter("index", STR(idx));

            THROW_ASSERT(arg_port, "Top level interface is missing port for argument '" + arg_name + "'");
            const auto val_port = if_port->find_member("val_port", port_o_K, if_port);
            add_internal_connection(val_port, arg_port);
         }
         else if(arg_interface == "m_axi")
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---Interface: " + arg_interface + " (bundle: " + bundle_name + ")");
            const auto axim_bundle_name = "if_m_axi_" + bundle_name;
            const auto axim_bundle = tb_cir->find_member(axim_bundle_name + "_fu", component_o_K, tb_cir);
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
            if_port->SetParameter("index", STR(idx));

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
            const auto if_port_bundle = tb_cir->find_member(if_port_name + "_fu", component_o_K, tb_cir);
            if(!if_port_bundle)
            {
               mgm.create_generic_module("Testbench" + capitalize(arg_interface), nullptr, top_fb, LIBRARY_STD,
                                         if_port_name);
               const auto if_port = tb_top->add_module_from_technology_library(if_port_name + "_fu", if_port_name,
                                                                               LIBRARY_STD, tb_cir, TechM);
               if_port->SetParameter("index", STR(idx));
               if_modules.push_back(if_port);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
         ++idx;
      }

      const auto return_port = dut->find_member(RETURN_PORT_NAME, port_o_K, dut);
      if(return_port)
      {
         const auto if_port =
             tb_top->add_module_from_technology_library("if_return_port", "IF_PORT_OUT", LIBRARY_STD, tb_cir, TechM);
         if_modules.push_back(if_port);
         if_port->SetParameter("index", STR(idx));

         const auto val_port = if_port->find_member("val_port", port_o_K, if_port);
         add_internal_connection(val_port, return_port);
      }

      const auto dut_start = dut->find_member(START_PORT_NAME, port_o_K, dut);
      THROW_ASSERT(dut_start, "");
      add_internal_connection(fsm_start, dut_start);
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Connecting DUT control ports...");
   {
      const auto has_dataflow =
          std::any_of(HLSMgr->module_arch->begin(), HLSMgr->module_arch->end(), [](const auto& fsymbol_arch) {
             return (fsymbol_arch.second->attrs.find(FunctionArchitecture::func_dataflow_top) !=
                         fsymbol_arch.second->attrs.end() &&
                     fsymbol_arch.second->attrs.find(FunctionArchitecture::func_dataflow_top)->second == "1") ||
                    (fsymbol_arch.second->attrs.find(FunctionArchitecture::func_dataflow_module) !=
                         fsymbol_arch.second->attrs.end() &&
                     fsymbol_arch.second->attrs.find(FunctionArchitecture::func_dataflow_module)->second == "1");
          });
      if(has_dataflow)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Generating dataflow termination logic...");
         std::vector<structural_objectRef> tb_done_ports;
         for(const auto& if_obj : if_modules)
         {
            const auto tb_done_port = if_obj->find_member("tb_done_port", port_o_K, if_obj);
            if(tb_done_port)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                              "---Considering testbench done port form " + if_obj->get_path());
               tb_done_ports.push_back(tb_done_port);
            }
         }
         if(tb_done_ports.size())
         {
            // Compute logic AND between all dataflow done ports from testbench interface modules
            const auto tb_done =
                tb_top->add_module_from_technology_library("tb_done_and", AND_GATE_STD, LIBRARY_STD, tb_cir, TechM);
            const auto tb_done_out = GetPointerS<module>(tb_done)->get_out_port(0);
            {
               const auto merge_port = GetPointerS<module>(tb_done)->get_in_port(0);
               const auto merge_port_o = GetPointerS<port_o>(merge_port);
               merge_port_o->add_n_ports(static_cast<unsigned int>(tb_done_ports.size()), merge_port);

               unsigned int i = 0;
               for(const auto& tb_done_port : tb_done_ports)
               {
                  const auto sig =
                      tb_top->add_sign("sig_tb_done_" + std::to_string(i), tb_cir, tb_done_port->get_typeRef());
                  tb_top->add_connection(tb_done_port, sig);
                  tb_top->add_connection(sig, merge_port_o->get_port(i));
                  ++i;
               }
            }

            // Compute logic OR between dataflow done ports' logic AND and standard DUT done port
            const auto done_or =
                tb_top->add_module_from_technology_library("tb_done_port", OR_GATE_STD, LIBRARY_STD, tb_cir, TechM);
            const auto tb_done_port = GetPointerS<module>(done_or)->get_out_port(0);
            {
               const auto merge_port = GetPointerS<module>(done_or)->get_in_port(0);
               const auto merge_port_o = GetPointerS<port_o>(merge_port);
               merge_port_o->add_n_ports(2U, merge_port);

               add_internal_connection(merge_port_o->get_port(0U), dut_done);
               {
                  const auto sig = tb_top->add_sign("sig_tb_done_port", tb_cir, tb_done_out->get_typeRef());
                  tb_top->add_connection(tb_done_out, sig);
                  tb_top->add_connection(sig, merge_port_o->get_port(1U));
               }
            }

            dut_done = tb_done_port;
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
         }
      }

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
      const auto if_mod = GetPointerS<module>(if_obj);
      for(unsigned i = 0; i < if_mod->get_in_port_size(); ++i)
      {
         const auto in_port = if_mod->get_in_port(i);
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
   const auto tb_filename = output_directory / CST_STR_BAMBU_TESTBENCH;
   const auto is_sim_verilator = parameters->getOption<std::string>(OPT_simulator) == "VERILATOR";
   HDL_manager HDLMgr(HLSMgr, HLSMgr->get_HLS_device(), parameters);
   std::list<std::string> hdl_files, aux_files;
   const std::list<structural_objectRef> tb_circuits = {tb_cir};
   HDLMgr.hdl_gen(tb_filename, tb_circuits, hdl_files, aux_files, true);
   THROW_ASSERT(hdl_files.size() == 1, "Expected single testbench file");
   THROW_ASSERT(aux_files.size() <= 1, "Expected at most a single testbench aux file");
   if(aux_files.size())
   {
      HLSMgr->aux_files.push_back(hdl_files.front());
      HLSMgr->RSim->filename_bench = aux_files.front();
   }
   else
   {
      HLSMgr->RSim->filename_bench = hdl_files.front();
   }
   {
      std::ifstream bambu_tb(HLSMgr->RSim->filename_bench);
      std::ofstream bambu_tb_dpi(HLSMgr->RSim->filename_bench + ".dpi");

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
      const auto vcd_output_filename = output_directory / "test.vcd";
      tb_writer->write("$dumpfile(\"" + vcd_output_filename.string() + "\");\n");
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
#if HAVE_FROM_DISCREPANCY_BUILT
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
               const auto sigscope = boost::replace_all_copy(sig_scope.first, STR(HIERARCHY_SEPARATOR), ".");
               for(const auto& signame : sig_scope.second)
               {
                  tb_writer->write("$dumpvars(1, " + sigscope + signame + ");\n");
               }
            }
         }
#endif
         tb_writer->write("`else\n");
      }

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
   std::filesystem::remove(HLSMgr->RSim->filename_bench);
   std::filesystem::rename(HLSMgr->RSim->filename_bench + ".dpi", HLSMgr->RSim->filename_bench);

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
   return DesignFlowStep_Status::SUCCESS;
}

std::vector<std::string> TestbenchGeneration::print_var_init(const tree_managerConstRef TM, unsigned int var,
                                                             const memoryRef mem)
{
   std::vector<std::string> init_els;
   const auto tn = TM->GetTreeNode(var);
   const auto init_node = [&]() -> tree_nodeRef {
      const auto vd = GetPointer<const var_decl>(tn);
      if(vd && vd->init)
      {
         return vd->init;
      }
      return nullptr;
   }();

   if(init_node &&
      (!GetPointer<const constructor>(init_node) || GetPointerS<const constructor>(init_node)->list_of_idx_valu.size()))
   {
      fu_binding::write_init(TM, tn, init_node, init_els, mem, 0);
   }
   else if(tn->get_kind() == string_cst_K || tn->get_kind() == integer_cst_K || tn->get_kind() == real_cst_K)
   {
      fu_binding::write_init(TM, tn, tn, init_els, mem, 0);
   }
   else if(!GetPointer<gimple_call>(tn))
   {
      if(tree_helper::IsArrayType(tn))
      {
         const auto type = tree_helper::CGetType(tn);
         const auto data_bitsize = tree_helper::GetArrayElementSize(type);
         const auto num_elements = tree_helper::GetArrayTotalSize(type);
         init_els.insert(init_els.end(), num_elements, std::string(data_bitsize, '0'));
      }
      else
      {
         const auto data_bitsize = tree_helper::SizeAlloc(tn);
         init_els.push_back(std::string(data_bitsize, '0'));
      }
   }
   return init_els;
}

unsigned long long TestbenchGeneration::generate_init_file(const std::string& dat_filename,
                                                           const tree_managerConstRef TM, unsigned int var,
                                                           const memoryRef mem)
{
   std::stringstream init_bits;
   std::ofstream useless;
   unsigned long long vec_size = 0, elts_size = 0;
   const auto var_type = tree_helper::CGetType(TM->GetTreeNode(var));
   const auto bitsize_align = GetPointer<const type_node>(var_type)->algn;
   THROW_ASSERT((bitsize_align % 8) == 0, "Alignment is not byte aligned.");
   fu_binding::fill_array_ref_memory(init_bits, useless, var, vec_size, elts_size, mem, TM, false, bitsize_align);

   std::ofstream init_dat(dat_filename, std::ios::binary);
   while(!init_bits.eof())
   {
      std::string bitstring;
      init_bits >> bitstring;
      THROW_ASSERT(bitstring.size() % 8 == 0, "Memory word initializer is not aligned");
      size_t i;
      // Memory is little-endian, thus last byte goes in first
      for(i = bitstring.size(); i >= 8; i -= 8)
      {
         char byteval = 0;
         for(size_t k = 0; k < 8; ++k)
         {
            byteval = byteval | static_cast<char>((bitstring.at(i - k - 1U) != '0') << k);
         }
         init_dat.put(byteval);
      }
   }
   unsigned long long bytes = static_cast<unsigned long long>(init_dat.tellp());
   THROW_ASSERT((bytes % (bitsize_align / 8)) == 0, "Memory initialization bytes not aligned");
   return bytes;
}
