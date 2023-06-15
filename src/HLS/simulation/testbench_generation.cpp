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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
#include "c_backend.hpp"
#include "c_backend_information.hpp"
#include "c_backend_step_factory.hpp"
#include "call_graph_manager.hpp"
#include "copyrights_strings.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
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
#include "tree_reindex.hpp"
#include "utility.hpp"

#include <algorithm>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iterator>
#include <list>
#include <string>
#include <utility>

#define CST_STR_BAMBU_TESTBENCH "bambu_testbench"

TestbenchGeneration::TestbenchGeneration(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                         const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::TESTBENCH_GENERATION),
      writer(language_writer::create_writer(HDLWriter_Language::VERILOG,
                                            _HLSMgr->get_HLS_target()->get_technology_manager(), _parameters)),
      cir(nullptr),
      mod(nullptr),
      output_directory(parameters->getOption<std::string>(OPT_output_directory) + "/simulation/"),
      c_testbench_basename(STR_CST_testbench_generation_basename)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
TestbenchGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::TEST_VECTOR_PARSER, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_MEMORY_ALLOCATION, HLSFlowStepSpecializationConstRef(),
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

void TestbenchGeneration::ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                                               const DesignFlowStep::RelationshipType relationship_type)
{
   HLS_step::ComputeRelationships(design_flow_step_set, relationship_type);

   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto c_backend_factory =
             GetPointer<const CBackendStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend"));

         design_flow_step_set.insert(c_backend_factory->CreateCBackendStep(CBackendInformationConstRef(
             new CBackendInformation(CBackendInformation::CB_HLS, output_directory + c_testbench_basename + ".c"))));
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
         break;
      }
   }
}

bool TestbenchGeneration::HasToBeExecuted() const
{
   return true;
}

void TestbenchGeneration::Initialize()
{
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
   const auto top_function_id = *(top_function_ids.begin());
   const auto top_hls = HLSMgr->get_HLS(top_function_id);
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

   const auto TechM = HLSMgr->get_HLS_target()->get_technology_manager();
   const auto std_lib_manager = TechM->get_library_manager(LIBRARY_STD);
   ModuleGeneratorManager mgm(HLSMgr, parameters);

   // Add top module wrapper
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating top level interface wrapper...");
   const auto top_id = [&]() {
      const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
      THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
      return *top_function_ids.begin();
   }();
   const auto top_fb = HLSMgr->CGetFunctionBehavior(top_id);
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
   const auto dut_reset = dut->find_member(RESET_PORT_NAME, port_o_K, dut);
   THROW_ASSERT(dut_reset, "");
   const auto fsm_reset = tb_fsm->find_member(RESET_PORT_NAME, port_o_K, tb_fsm);
   add_internal_connection(fsm_reset, dut_reset);
   const auto dut_done = dut->find_member(DONE_PORT_NAME, port_o_K, dut);
   THROW_ASSERT(dut_done, "");
   const auto fsm_done = tb_fsm->find_member(DONE_PORT_NAME, port_o_K, tb_fsm);
   add_internal_connection(fsm_done, dut_done);
   auto fsm_start = tb_fsm->find_member(START_PORT_NAME, port_o_K, tb_fsm);

   std::list<structural_objectRef> if_modules;
   const auto interface_type = parameters->getOption<HLSFlowStep_Type>(OPT_interface_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating handler modules for top level parameters...");
   const auto top_bh = top_fb->CGetBehavioralHelper();
   if(parameters->getOption<bool>(OPT_memory_mapped_top))
   {
      const std::string if_suffix =
          interface_type == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION ? "Minimal" : "WishboneB4";
      const auto master_port_module = "TestbenchArgMap" + if_suffix;
      size_t idx = top_bh->GetFunctionReturnType(top_id) ? 1 : 0;
      std::list<structural_objectRef> master_ports;
      for(const auto& par : top_bh->GetParameters())
      {
         const auto par_name = top_bh->PrintVariable(GET_INDEX_CONST_NODE(par));
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Parameter " + par_name);
         const auto par_bitsize = tree_helper::Size(par);
         const auto par_symbol = HLSMgr->Rmem->get_symbol(GET_INDEX_CONST_NODE(par), top_id);
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

      const auto start_symbol = HLSMgr->Rmem->get_symbol(top_id, top_id);
      const auto master_start = tb_top->add_module_from_technology_library(
          "_master_start", "TestbenchStartMap" + if_suffix, LIBRARY_STD, tb_cir, TechM);
      master_start->SetParameter("tgt_addr", STR(start_symbol->get_address()));
      master_ports.push_back(master_start);

      if_modules.insert(if_modules.end(), master_ports.begin(), master_ports.end());
      if(master_ports.size())
      {
         const auto master_mod = GetPointerS<module>(master_ports.front());
         THROW_ASSERT(master_mod->get_in_port_size() >= 3, "At least three in ports must be present.");
         THROW_ASSERT(master_mod->get_out_port_size() >= 1, "At least an out port must be present.");
         unsigned int k = 0;

         // Daisy chain start_port signal through all memory master modules
         for(const auto& master_port : master_ports)
         {
            const auto mmod = GetPointerS<module>(master_port);
            const auto m_start = mmod->get_in_port(2);
            const auto m_start_o = mmod->get_out_port(0);
            THROW_ASSERT(m_start->get_id() == "i_" START_PORT_NAME, "");
            THROW_ASSERT(m_start_o->get_id() == START_PORT_NAME, "");
            const auto sig = tb_top->add_sign("sig_" START_PORT_NAME + STR(k), tb_cir, fsm_start->get_typeRef());
            tb_top->add_connection(fsm_start, sig);
            tb_top->add_connection(sig, m_start);
            fsm_start = m_start_o;
            ++k;
         }

         // Merge all matching out signals from memory master modules
         for(unsigned int i = 1; i < master_mod->get_out_port_size(); ++i)
         {
            const auto out_port = master_mod->get_out_port(i);
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
               const auto m_port = GetPointerS<module>(master_port)->get_out_port(i);
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
      const auto dut_start = dut->find_member(START_PORT_NAME, port_o_K, dut);
      THROW_ASSERT(dut_start, "");
      add_internal_connection(fsm_start, dut_start);

      // Add interface components relative to each top function argument
      auto idx = 0U;
      {
         const auto return_port = dut->find_member(RETURN_PORT_NAME, port_o_K, dut);
         if(return_port)
         {
            const auto if_port =
                tb_top->add_module_from_technology_library("if_return_port", "IF_PORT_OUT", LIBRARY_STD, tb_cir, TechM);
            if_modules.push_back(if_port);
            if_port->SetParameter("index", STR(idx));

            const auto val_port = if_port->find_member("val_port", port_o_K, if_port);
            add_internal_connection(val_port, return_port);
            ++idx;
         }
      }

      const auto is_interface_inferred = interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION;
      const auto DesignAttributes = HLSMgr->design_attributes.find(top_bh->GetMangledFunctionName());
      THROW_ASSERT(!is_interface_inferred || DesignAttributes != HLSMgr->design_attributes.end(),
                   "Original signature not found for function: " + top_bh->GetMangledFunctionName() + " (" +
                       top_bh->get_function_name() + ")");
      for(const auto& arg : top_bh->GetParameters())
      {
         const auto arg_name = top_bh->PrintVariable(GET_INDEX_CONST_NODE(arg));
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Parameter " + arg_name);
         THROW_ASSERT(!is_interface_inferred || DesignAttributes->second.count(arg_name),
                      "Interface attributes missing for parameter " + arg_name);
         if(is_interface_inferred && tree_helper::IsPointerType(arg) &&
            !DesignAttributes->second.at(arg_name).count(attr_interface_dir))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--Unused parameter");
            ++idx;
            continue;
         }
         const auto arg_port = dut->find_member(arg_name, port_o_K, dut);
         const auto arg_interface = [&]() -> std::string {
            if(is_interface_inferred)
            {
               THROW_ASSERT(DesignAttributes->second.at(arg_name).count(attr_interface_type),
                            "Not matched parameter name: " + arg_name);
               return DesignAttributes->second.at(arg_name).at(attr_interface_type);
            }
            return "default";
         }();
         const auto bundle_name = [&]() {
            if(is_interface_inferred)
            {
               const auto attr_it = DesignAttributes->second.at(arg_name).find(attr_bundle_name);
               if(attr_it != DesignAttributes->second.at(arg_name).end())
               {
                  return attr_it->second;
               }
            }
            return arg_name;
         }();

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
            const auto axim_bundle = tb_cir->find_member(axim_bundle_name, component_o_K, tb_cir);
            if(!axim_bundle)
            {
               mgm.create_generic_module("TestbenchAXIM", nullptr, top_fb, LIBRARY_STD, axim_bundle_name);
               const auto if_port = tb_top->add_module_from_technology_library(axim_bundle_name, axim_bundle_name,
                                                                               LIBRARY_STD, tb_cir, TechM);
               if_modules.push_back(if_port);
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
                               DesignAttributes->second.at(arg_name).at(attr_interface_dir) +
                               (bundle_name != arg_name ? (" (bundle: " + bundle_name + ")") : ""));
            const auto if_port_name = "if_" + arg_interface + "_" + bundle_name;
            const auto if_port_bundle = tb_cir->find_member(if_port_name, component_o_K, tb_cir);
            if(!if_port_bundle)
            {
               mgm.create_generic_module("Testbench" + capitalize(arg_interface), nullptr, top_fb, LIBRARY_STD,
                                         if_port_name);
               const auto if_port =
                   tb_top->add_module_from_technology_library(if_port_name, if_port_name, LIBRARY_STD, tb_cir, TechM);
               if_port->SetParameter("index", STR(idx));
               if_modules.push_back(if_port);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
         ++idx;
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating memory interface...");
   structural_objectRef tb_mem;
   if(interface_type == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION ||
      interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
   {
      tb_mem =
          tb_top->add_module_from_technology_library("SystemMEM", "TestbenchMEMMinimal", LIBRARY_STD, tb_cir, TechM);
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
   if_modules.push_back(tb_mem);

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
         else if(in_port->get_id() == DONE_PORT_NAME)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---" + in_port->get_path() + " <-> " + dut_done->get_path());
            add_internal_connection(in_port, dut_done);
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
   const auto tb_filename = output_directory + CST_STR_BAMBU_TESTBENCH;
   const auto is_sim_verilator = parameters->getOption<std::string>(OPT_simulator) == "VERILATOR";
   HDL_manager HDLMgr(HLSMgr, HLSMgr->get_HLS_target()->get_target_device(), parameters);
   std::list<std::string> hdl_files, aux_files;
   const std::list<structural_objectRef> tb_circuits = {tb_cir};
   HDLMgr.hdl_gen(tb_filename, tb_circuits, false, hdl_files, aux_files);
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
`ifdef M32
typedef int unsigned ptr_t;
`else
typedef longint unsigned ptr_t;
`endif

)";
      bambu_tb_dpi << bambu_tb.rdbuf();

      auto tb_writer = language_writer::create_writer(HDLWriter_Language::VERILOG,
                                                      HLSMgr->get_HLS_target()->get_technology_manager(), parameters);

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
      const auto vcd_output_filename = output_directory + "test.vcd";
      tb_writer->write("$dumpfile(\"" + vcd_output_filename + "\");\n");
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
            (static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language)) ==
             HDLWriter_Language::VHDL) ||
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
   boost::filesystem::remove(HLSMgr->RSim->filename_bench);
   boost::filesystem::rename(HLSMgr->RSim->filename_bench + ".dpi", HLSMgr->RSim->filename_bench);

   if(parameters->getOption<std::string>(OPT_simulator) == "VERILATOR")
   {
      HLSMgr->aux_files.push_back(write_verilator_testbench());
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
   return DesignFlowStep_Status::SUCCESS;
}

std::string TestbenchGeneration::write_verilator_testbench() const
{
   const std::string filename = output_directory + "bambu_testbench.cpp";
   std::ofstream os(filename, std::ios::out);
   simple_indent PP('{', '}', 3);

   std::string top_fname = mod->get_typeRef()->id_type;
   PP(os, "#include <iostream>\n");
   PP(os, "#include <string>\n");
   PP(os, "#include <verilated.h>\n");
   PP(os, "#include \"Vbambu_testbench.h\"\n");
   PP(os, "\n");
   PP(os, "#if VM_TRACE\n");
   PP(os, "# include <verilated_vcd_c.h>\n");
   PP(os, "#endif\n");
   PP(os, "\n");
   PP(os, "\n");
   PP(os, "#define SIMULATION_MAX " + STR(2 * parameters->getOption<long long int>(OPT_max_sim_cycles)) + "ULL\n\n");
   PP(os, "static vluint64_t CLOCK_PERIOD = 2;\n");
   PP(os, "static vluint64_t HALF_CLOCK_PERIOD = CLOCK_PERIOD/2;\n");
   PP(os, "\n");
   PP(os, "vluint64_t main_time = 0;\n");
   PP(os, "\n");
   PP(os, "double sc_time_stamp ()  {return main_time;}\n");
   PP(os, "\n");
   PP(os, "int main (int argc, char **argv, char **env)\n");
   PP(os, "{\n");
   PP(os, "   Vbambu_testbench *top;\n");
   PP(os, "\n");
   PP(os, "   std::string vcd_output_filename = \"" + output_directory + "test.vcd\";\n");
   PP(os, "   Verilated::commandArgs(argc, argv);\n");
   PP(os, "   Verilated::debug(0);\n");
   PP(os, "   top = new Vbambu_testbench{\"clocked_" CST_STR_BAMBU_TESTBENCH "\"};\n");
   PP(os, "   \n");
   PP(os, "   \n");
   PP(os, "   #if VM_TRACE\n");
   PP(os, "   Verilated::traceEverOn(true);\n");
   PP(os, "   VerilatedVcdC* tfp = new VerilatedVcdC;\n");
   PP(os, "   #endif\n");
   PP(os, "   main_time=0;\n");
   PP(os, "#if VM_TRACE\n");
   PP(os, "   top->trace (tfp, 99);\n");
   PP(os, "   tfp->set_time_unit(\"n\");\n");
   PP(os, "   tfp->set_time_resolution(\"p\");\n");
   PP(os, "   tfp->open (vcd_output_filename.c_str());\n");
   PP(os, "#endif\n");
   PP(os, "   long long int cycleCounter = 0;\n");
   PP(os, "   top->" CLOCK_PORT_NAME " = 1;\n");
   PP(os, "   while (!Verilated::gotFinish() && cycleCounter < SIMULATION_MAX)\n");
   PP(os, "   {\n");
   PP(os, "     top->" CLOCK_PORT_NAME " = top->" CLOCK_PORT_NAME " == 0 ? 1 : 0;\n");
   PP(os, "     top->eval();\n");
   PP(os, "#if VM_TRACE\n");
   PP(os, "     if (tfp) tfp->dump (main_time);\n");
   PP(os, "#endif\n");
   PP(os, "     main_time += HALF_CLOCK_PERIOD;\n");
   PP(os, "     cycleCounter++;\n");
   PP(os, "   }\n");
   PP(os, "if(cycleCounter>=SIMULATION_MAX)\n");
   PP(os, "  std::cerr << \"Simulation not completed into " +
              STR(parameters->getOption<long long int>(OPT_max_sim_cycles)) + " cycles\\n\";\n");
   PP(os, "#if VM_TRACE\n");
   PP(os, "   if (tfp) tfp->dump (main_time);\n");
   PP(os, "#endif\n");
   PP(os, "   top->final();\n");
   PP(os, "   #if VM_TRACE\n");
   PP(os, "   tfp->close();\n");
   PP(os, "   delete tfp;\n");
   PP(os, "   #endif\n");
   PP(os, "   delete top;\n");
   PP(os, "   exit(0L);\n");
   PP(os, "}");

   return filename;
}

std::vector<std::string> TestbenchGeneration::print_var_init(const tree_managerConstRef TM, unsigned int var,
                                                             const memoryRef mem)
{
   std::vector<std::string> init_els;
   const auto tn = TM->CGetTreeReindex(var);
   const auto init_node = [&]() -> tree_nodeRef {
      const auto vd = GetPointer<const var_decl>(GET_CONST_NODE(tn));
      if(vd && vd->init)
      {
         return vd->init;
      }
      return nullptr;
   }();

   if(init_node && (!GetPointer<const constructor>(GET_CONST_NODE(init_node)) ||
                    GetPointerS<const constructor>(GET_CONST_NODE(init_node))->list_of_idx_valu.size()))
   {
      fu_binding::write_init(TM, tn, init_node, init_els, mem, 0);
   }
   else if(GET_CONST_NODE(tn)->get_kind() == string_cst_K || GET_CONST_NODE(tn)->get_kind() == integer_cst_K ||
           GET_CONST_NODE(tn)->get_kind() == real_cst_K)
   {
      fu_binding::write_init(TM, tn, tn, init_els, mem, 0);
   }
   else if(!GetPointer<gimple_call>(GET_CONST_NODE(tn)))
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
         const auto data_bitsize = tree_helper::Size(tn);
         init_els.push_back(std::string(data_bitsize, '0'));
      }
   }
   return init_els;
}

unsigned long long TestbenchGeneration::generate_init_file(const std::string& dat_filename,
                                                           const tree_managerConstRef TM, unsigned int var,
                                                           const memoryRef mem)
{
   std::ofstream init_dat(dat_filename, std::ios::binary);
   std::vector<std::string> init_els;
   const auto tn = TM->CGetTreeReindex(var);
   const auto init_node = [&]() -> tree_nodeRef {
      const auto vd = GetPointer<const var_decl>(GET_CONST_NODE(tn));
      if(vd && vd->init)
      {
         return vd->init;
      }
      return nullptr;
   }();

   if(init_node && (!GetPointer<const constructor>(GET_CONST_NODE(init_node)) ||
                    GetPointerS<const constructor>(GET_CONST_NODE(init_node))->list_of_idx_valu.size()))
   {
      fu_binding::write_init(TM, tn, init_node, init_els, mem, 0);
   }
   else if(GET_CONST_NODE(tn)->get_kind() == string_cst_K || GET_CONST_NODE(tn)->get_kind() == integer_cst_K ||
           GET_CONST_NODE(tn)->get_kind() == real_cst_K)
   {
      fu_binding::write_init(TM, tn, tn, init_els, mem, 0);
   }
   else if(!GetPointer<gimple_call>(GET_CONST_NODE(tn)))
   {
      const auto zero_bytes_count = [&]() {
         if(tree_helper::IsArrayType(tn))
         {
            const auto type = tree_helper::CGetType(tn);
            const auto data_bitsize = tree_helper::GetArrayElementSize(type);
            const auto num_elements = tree_helper::GetArrayTotalSize(type);
            return get_aligned_bitsize(data_bitsize) * num_elements;
         }
         return get_aligned_bitsize(tree_helper::Size(tn));
      }();
      std::fill_n(std::ostream_iterator<char>(init_dat), zero_bytes_count, 0);
      return zero_bytes_count;
   }

   unsigned long long byte_count = 0;
   for(const auto& bitstring : init_els)
   {
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
      byte_count += bitstring.size() / 8;
   }
   return byte_count;
}
