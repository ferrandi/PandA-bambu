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
 * @file testbench_generation_base_step.cpp
 * @brief hls testbench automatic generation
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */
#include "testbench_generation_base_step.hpp"

#include "config_PACKAGE_BUGREPORT.hpp"
#include "config_PACKAGE_NAME.hpp"
#include "config_PACKAGE_VERSION.hpp"

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

#if HAVE_FROM_DISCREPANCY_BUILT
#include "Discrepancy.hpp"
#endif

#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <iterator>
#include <list>
#include <string>
#include <utility>

#define CST_STR_BAMBU_TESTBENCH "bambu_testbench"

#define COUNT_LOW_INDEX 0
#define COUNT_HIGH_INDEX 31
#define BURST_LOW_INDEX 32
#define BURST_HIGH_INDEX 33
#define LEN_LOW_INDEX 34
#define LEN_HIGH_INDEX 41
#define SIZE_LOW_INDEX 42
#define SIZE_HIGH_INDEX 44
#define ADDR_LOW_INDEX 45
#define ADDR_HIGH_INDEX 76

static unsigned long long local_port_size(const structural_objectRef portInst)
{
   const auto po = GetPointer<port_o>(portInst);
   const auto port_bitwidth = po->get_typeRef()->size * po->get_typeRef()->vector_size;
   const auto port_alignment = po->get_port_alignment() * 8U;
   if(port_alignment)
   {
      return port_bitwidth + ((port_alignment - (port_bitwidth % port_alignment)) & (port_alignment - 1U));
   }
   return get_aligned_bitsize(port_bitwidth);
}

/* Visits all submodules and checks if they have axi ports. Recursively visits axi children. If no axi children are
 * found, current node is the axi controller and can print the axi cache stats. Returns true if it has axi children,
 * false otherwise */
bool TestbenchGenerationBaseStep::printCacheStats(const module* rootMod) const
{
   const unsigned intObjCount = rootMod->get_internal_objects_size();
   bool hasAxiChildren = false;
   for(unsigned i = 0; i < intObjCount; i++)
   {
      const auto subMod = rootMod->get_internal_object(i);
      bool isSubModAxi = false;
      if(subMod->get_kind() == component_o_K)
      {
         if(GetPointer<module>(subMod)->get_out_port_size())
         {
            for(unsigned j = 0; j < GetPointer<module>(subMod)->get_out_port_size(); j++)
            {
               const structural_objectRef& port = GetPointer<module>(subMod)->get_out_port(j);
               if(GetPointer<port_o>(port)->get_port_interface() == port_o::port_interface::M_AXI_AWADDR)
               {
                  hasAxiChildren = true;
                  isSubModAxi = true;
               }
            }
         }
         if(isSubModAxi)
         {
            if(!printCacheStats(GetPointer<module>(subMod)))
            {
               /* This module is an AXI controller check for cache */
               const auto params = GetPointer<module>(subMod)->GetParameters();
               if(params.find("WAY_LINES") != params.end() && boost::lexical_cast<unsigned>(params.at("WAY_LINES")) > 0)
               {
                  const std::string topModuleName =
                      parameters->getOption<std::string>(OPT_simulator) == "VERILATOR" ? "" : "DUT.";
                  std::string moduleHierarchy = topModuleName + GetPointer<module>(subMod)->get_path();
                  std::replace(moduleHierarchy.begin(), moduleHierarchy.end(), '/', '.');
                  const std::string moduleToCounters = ".cache.ctrl.cache_control.cnt";
                  if(output_level >= DEBUG_LEVEL_PEDANTIC)
                  {
                     writer->write("$display(\"Read hits on module " + subMod->get_id() + " : %d\", " +
                                   moduleHierarchy + moduleToCounters + ".read_access_cnt);\n");
                     writer->write("$display(\"Read misses on module " + subMod->get_id() + " : %d\", " +
                                   moduleHierarchy + moduleToCounters + ".read_miss_cnt);\n");
                     writer->write("$display(\"Write hits on module " + subMod->get_id() + " : %d\", " +
                                   moduleHierarchy + moduleToCounters + ".write_access_cnt);\n");
                     writer->write("$display(\"Write misses on module " + subMod->get_id() + " : %d\", " +
                                   moduleHierarchy + moduleToCounters + ".write_miss_cnt);\n");
                  }
                  writer->write("stats_file = $fopen(\"" + subMod->get_id() + "_cache_stats.txt\", \"w\");\n");
                  writer->write_comment("Error in file open\n");
                  writer->write("if (stats_file == `NULL)\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  writer->write("$warning(\"ERROR - Error opening the cache stats file. Simulation will continue, but "
                                "cache stats will not be saved.\");\n");
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end else\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  writer->write("$fwrite(stats_file, \"Read hits: %d\\n\", " + moduleHierarchy + moduleToCounters +
                                ".read_access_cnt - " + moduleHierarchy + moduleToCounters + ".read_miss_cnt);\n");
                  writer->write("$fwrite(stats_file, \"Read misses: %d\\n\", " + moduleHierarchy + moduleToCounters +
                                ".read_miss_cnt);\n");
                  writer->write("$fwrite(stats_file, \"Write hits: %d\\n\", " + moduleHierarchy + moduleToCounters +
                                ".write_access_cnt - " + moduleHierarchy + moduleToCounters + ".write_miss_cnt);\n");
                  writer->write("$fwrite(stats_file, \"Write misses: %d\\n\", " + moduleHierarchy + moduleToCounters +
                                ".write_miss_cnt);\n");
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");
               }
            }
         }
      }
   }
   return hasAxiChildren;
}

TestbenchGenerationBaseStep::TestbenchGenerationBaseStep(const ParameterConstRef _parameters,
                                                         const HLS_managerRef _HLSMgr,
                                                         const DesignFlowManagerConstRef _design_flow_manager,
                                                         const HLSFlowStep_Type _hls_flow_step_type,
                                                         std::string _c_testbench_basename)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, _hls_flow_step_type),
      writer(language_writer::create_writer(HDLWriter_Language::VERILOG,
                                            _HLSMgr->get_HLS_target()->get_technology_manager(), _parameters)),
      mod(nullptr),
      target_period(0.0),
      output_directory(parameters->getOption<std::string>(OPT_output_directory) + "/simulation/"),
      c_testbench_basename(std::move(_c_testbench_basename))
{
   if(!boost::filesystem::exists(output_directory))
   {
      boost::filesystem::create_directories(output_directory);
   }
}

TestbenchGenerationBaseStep::~TestbenchGenerationBaseStep() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
TestbenchGenerationBaseStep::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

void TestbenchGenerationBaseStep::ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
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

bool TestbenchGenerationBaseStep::HasToBeExecuted() const
{
   return true;
}

void TestbenchGenerationBaseStep::Initialize()
{
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
   const auto top_function_id = *(top_function_ids.begin());
   const auto top_hls = HLSMgr->get_HLS(top_function_id);
   cir = top_hls->top->get_circ();
   THROW_ASSERT(GetPointer<const module>(cir), "Not a module");
   mod = GetPointer<const module>(cir);
   target_period = top_hls->HLS_C->get_clock_period();
   hdl_testbench_basename = "testbench_" + cir->get_id();
}

DesignFlowStep_Status TestbenchGenerationBaseStep::Exec()
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
   const auto fsm_reset = tb_fsm->find_member(RESET_PORT_NAME, port_o_K, tb_fsm);
   add_internal_connection(fsm_reset, dut_reset);
   const auto dut_start = dut->find_member(START_PORT_NAME, port_o_K, dut);
   const auto fsm_start = tb_fsm->find_member(START_PORT_NAME, port_o_K, tb_fsm);
   add_internal_connection(fsm_start, dut_start);
   const auto dut_done = dut->find_member(DONE_PORT_NAME, port_o_K, dut);
   const auto fsm_done = tb_fsm->find_member(DONE_PORT_NAME, port_o_K, tb_fsm);
   add_internal_connection(fsm_done, dut_done);

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating handler modules for top level parameters...");
   std::list<structural_objectRef> if_modules;
   // Add interface components relative to each top function argument
   const auto top_bh = top_fb->CGetBehavioralHelper();
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

   const auto interface_type = parameters->getOption<HLSFlowStep_Type>(OPT_interface_type);
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
         const auto if_port = tb_top->add_module_from_technology_library("if_" + arg_interface + "_" + arg_name,
                                                                         "IF_PORT_" + port_o::GetString(arg_port_dir),
                                                                         LIBRARY_STD, tb_cir, TechM);
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
         // TODO: add offset port constant value
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
         if(arg_interface == "array")
         {
            // TODO: add offset port constant value
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--");
      ++idx;
   }

   // Add memory interface component if necessary
   // TODO: wrap inside if(is necessary)
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Generating memory interface...");
   const auto tb_mem =
       tb_top->add_module_from_technology_library("SystemMEM", "TestbenchMEM", LIBRARY_STD, tb_cir, TechM);
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
            else if(GetPointer<port_o>(in_port)->get_connections_size())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---" + in_port->get_path() + " already connected");
            }
            else if(!GetPointer<port_o>(in_port)->get_is_memory())
            {
               THROW_UNREACHABLE("Port " + in_port->get_id() + " not found in DUT module " + dut->get_path());
            }
         }
      }

      for(unsigned i = 0; i < if_mod->get_out_port_size(); ++i)
      {
         const auto out_port = if_mod->get_out_port(i);
         const auto dut_port = dut->find_member(out_port->get_id(), port_o_K, dut);
         if(dut_port)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                           "---" + out_port->get_path() + " <-> " + dut_port->get_path());
            add_internal_connection(out_port, dut_port);
         }
         else if(GetPointer<port_o>(out_port)->get_connections_size())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---" + out_port->get_path() + " already connected");
         }
         else if(!GetPointer<port_o>(out_port)->get_is_memory())
         {
            THROW_UNREACHABLE("Port " + out_port->get_path() + " not found in top level interface");
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

std::vector<std::string> TestbenchGenerationBaseStep::print_var_init(const tree_managerConstRef TM, unsigned int var,
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

unsigned long long TestbenchGenerationBaseStep::generate_init_file(const std::string& dat_filename,
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

std::string TestbenchGenerationBaseStep::write_verilator_testbench() const
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

std::string TestbenchGenerationBaseStep::create_HDL_testbench(bool xilinx_isim) const
{
   if(!parameters->getOption<bool>(OPT_generate_testbench))
   {
      return "";
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating HDL testbench");
   const tree_managerRef TM = HLSMgr->get_tree_manager();

   std::string simulation_values_path = output_directory + STR(STR_CST_testbench_generation_basename) + ".txt";
   const auto generate_vcd_output =
       (parameters->isOption(OPT_generate_vcd) && parameters->getOption<bool>(OPT_generate_vcd)) ||
       (parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy));

   std::string file_name = output_directory + hdl_testbench_basename + writer->get_extension();

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  writing testbench");
   writer->write_comment("File automatically generated by: " PACKAGE_NAME " framework version=" PACKAGE_VERSION "\n");
   writer->write_comment("Send any bug to: " PACKAGE_BUGREPORT "\n");
   writer->WriteLicense();
   // write testbench for simulation
   this->write_hdl_testbench(simulation_values_path, generate_vcd_output, xilinx_isim, TM);
   writer->WriteFile(file_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created HDL testbench");
   return file_name;
}

void TestbenchGenerationBaseStep::write_hdl_testbench(std::string simulation_values_path, bool generate_vcd_output,
                                                      bool xilinx_isim, const tree_managerConstRef TM) const
{
   this->write_underlying_testbench(simulation_values_path, generate_vcd_output, xilinx_isim, TM);

   writer->write("module " + mod->get_id() + "_tb_top;\n");
   writer->write("reg " + STR(CLOCK_PORT_NAME) + ";\n");
   writer->write("initial\nbegin\n");
   writer->write(STR(CLOCK_PORT_NAME) + " = 1;\n");
   writer->write("end\n");

   write_clock_process();

   writer->write(mod->get_id() + "_tb DUT(." + STR(CLOCK_PORT_NAME) + "(" + STR(CLOCK_PORT_NAME) + "));\n");
   writer->write("endmodule\n");
}

void TestbenchGenerationBaseStep::write_initial_block(const std::string& simulation_values_path, bool withMemory,
                                                      const tree_managerConstRef TM, bool generate_vcd_output) const
{
   begin_initial_block();

   /// VCD output generation (optional)
   std::string vcd_output_filename = output_directory + "test.vcd";
   if(generate_vcd_output)
   {
      writer->write_comment("VCD file generation\n");
      writer->write("$dumpfile(\"" + vcd_output_filename + "\");\n");
      bool simulator_supports_dumpvars_directive = parameters->getOption<std::string>(OPT_simulator) == "MODELSIM" ||
                                                   parameters->getOption<std::string>(OPT_simulator) == "ICARUS" ||
                                                   parameters->getOption<std::string>(OPT_simulator) == "XSIM";
      bool dump_all_signals = parameters->isOption(OPT_generate_vcd) && parameters->getOption<bool>(OPT_generate_vcd);
      if(dump_all_signals || !simulator_supports_dumpvars_directive ||
         (static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language)) ==
          HDLWriter_Language::VHDL)
#if HAVE_FROM_DISCREPANCY_BUILT
         || !parameters->isOption(OPT_discrepancy) || !parameters->getOption<bool>(OPT_discrepancy) or
         HLSMgr->RDiscr->selected_vcd_signals.empty()
#endif
      )
      {
         writer->write("$dumpvars;\n");
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
               writer->write("$dumpvars(1, " + sigscope + signame + ");\n");
            }
         }
      }
#endif
   }

   /// open file with values
   open_value_file(simulation_values_path);

   /// open file with results
   const auto result_file = parameters->getOption<std::string>(OPT_simulation_output);
   open_result_file(result_file);

   /// auxiliary variables initialization
   initialize_auxiliary_variables();
   initialize_input_signals(TM);
   init_extra_signals(withMemory);
   memory_initialization();
   end_initial_block();
}

void TestbenchGenerationBaseStep::init_extra_signals(bool withMemory) const
{
   if(mod->find_member(RETURN_PORT_NAME, port_o_K, cir))
   {
      writer->write("ex_" + STR(RETURN_PORT_NAME) + " = 0;\n");
      writer->write("registered_" + STR(RETURN_PORT_NAME) + " = 0;\n");
      writer->write("\n");
   }
   if(withMemory)
   {
      structural_objectRef M_Rdata_ram_port = mod->find_member("M_Rdata_ram", port_o_K, cir);
      THROW_ASSERT(M_Rdata_ram_port, "M_Rdata_ram port is missing");
      auto M_Rdata_ram_port_n_ports =
          M_Rdata_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(M_Rdata_ram_port)->get_ports_size() : 1;
      for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         writer->write("reg_DataReady[" + STR(i) + "] = 0;\n\n");
      }
   }
}

void TestbenchGenerationBaseStep::write_output_checks(const tree_managerConstRef TM) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing output checks");
   const HLSFlowStep_Type interface_type = parameters->getOption<HLSFlowStep_Type>(OPT_interface_type);
   if(interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
   {
      const auto& DesignSignature = HLSMgr->RSim->simulationArgSignature;
      for(const auto& par : DesignSignature)
      {
         auto portInst = mod->find_member(par, port_o_K, cir);
         if(!portInst)
         {
            portInst = mod->find_member(par + "_o", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_dout", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member("s_axis_" + par + "_TDATA", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_din", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member("m_axis_" + par + "_TDATA", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_d0", port_o_K, cir);
            if(portInst)
            {
               auto InterfaceType = GetPointer<port_o>(portInst)->get_port_interface();
               if(InterfaceType == port_o::port_interface::PI_DOUT)
               {
                  const auto manage_pidout = [&](const std::string& portID) {
                     auto port_name = portInst->get_id();
                     auto terminate = port_name.size() > 3 ? port_name.size() - std::string("_d" + portID).size() : 0;
                     THROW_ASSERT(port_name.substr(terminate) == "_d" + portID, "inconsistent interface");
                     auto orig_name = port_name.substr(0, terminate);
                     auto port_addr = mod->find_member(orig_name + "_address" + portID, port_o_K, cir);
                     THROW_ASSERT(port_addr && GetPointer<port_o>(port_addr)->get_port_interface() ==
                                                   port_o::port_interface::PI_ADDRESS,
                                  "inconsistent interface");
                     auto port_ce = mod->find_member(orig_name + "_ce" + portID, port_o_K, cir);
                     THROW_ASSERT(port_ce && GetPointer<port_o>(port_ce)->get_port_interface() ==
                                                 port_o::port_interface::PI_CHIPENABLE,
                                  "inconsistent interface");
                     auto port_we = mod->find_member(orig_name + "_we" + portID, port_o_K, cir);
                     THROW_ASSERT(port_we && GetPointer<port_o>(port_we)->get_port_interface() ==
                                                 port_o::port_interface::PI_WRITEENABLE,
                                  "inconsistent interface");
                     auto port_q = mod->find_member(orig_name + "_q" + portID, port_o_K, cir);
                     std::string mem_aggregated;
                     {
                        const auto bitsize = local_port_size(portInst);
                        mem_aggregated = "{";
                        for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
                        {
                           if(bitsize_index)
                           {
                              mem_aggregated += ", ";
                           }
                           mem_aggregated += "_bambu_testbench_mem_[paddr" + port_name.substr(0, port_name.size() - 1) +
                                             "0 + " + STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr + " +
                                             port_addr->get_id() + "*" +
                                             STR(GetPointer<port_o>(port_addr)->get_port_alignment()) + "]";
                        }
                        mem_aggregated += "}";
                     }
                     writer->write("always @(posedge " CLOCK_PORT_NAME ")\n");
                     writer->write(STR(STD_OPENING_CHAR));
                     writer->write("begin\n");
                     writer->write("if (" + port_ce->get_id() + " == 1'b1 && " + port_we->get_id() + " == 1'b1)\n");
                     writer->write(STR(STD_OPENING_CHAR));
                     writer->write("begin\n");
                     writer->write(mem_aggregated + " <= " + port_name + ";\n");
                     writer->write(STR(STD_CLOSING_CHAR));
                     writer->write("end\n");
                     if(port_q)
                     {
                        writer->write("else if (" + port_ce->get_id() + " == 1'b1)\n");
                        writer->write(STR(STD_OPENING_CHAR));
                        writer->write("begin\n");
                        writer->write(port_q->get_id() + " <= " + mem_aggregated + ";\n");
                        writer->write(STR(STD_CLOSING_CHAR));
                        writer->write("end\n");
                     }
                     writer->write(STR(STD_CLOSING_CHAR));
                     writer->write("end\n");
                  };
                  manage_pidout("0");

                  portInst = mod->find_member(par + "_d1", port_o_K, cir);
                  if(portInst)
                  {
                     InterfaceType = GetPointer<port_o>(portInst)->get_port_interface();
                     if(InterfaceType == port_o::port_interface::PI_DOUT)
                     {
                        manage_pidout("1");
                     }
                  }
                  continue;
               }
               else
               {
                  portInst = structural_objectRef();
               }
            }
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_q0", port_o_K, cir);
            if(portInst)
            {
               auto InterfaceType = GetPointer<port_o>(portInst)->get_port_interface();
               if(InterfaceType == port_o::port_interface::PI_DIN)
               {
                  const auto manage_pidin = [&](const std::string& portID) {
                     auto port_name = portInst->get_id();
                     auto terminate = port_name.size() > 3 ? port_name.size() - std::string("_q" + portID).size() : 0;
                     THROW_ASSERT(port_name.substr(terminate) == "_q" + portID, "inconsistent interface");
                     auto orig_name = port_name.substr(0, terminate);
                     auto port_addr = mod->find_member(orig_name + "_address" + portID, port_o_K, cir);
                     THROW_ASSERT(port_addr && GetPointer<port_o>(port_addr)->get_port_interface() ==
                                                   port_o::port_interface::PI_ADDRESS,
                                  "inconsistent interface");
                     auto port_ce = mod->find_member(orig_name + "_ce" + portID, port_o_K, cir);
                     THROW_ASSERT(port_ce && GetPointer<port_o>(port_ce)->get_port_interface() ==
                                                 port_o::port_interface::PI_CHIPENABLE,
                                  "inconsistent interface");
                     std::string mem_aggregated;
                     {
                        const auto bitsize = local_port_size(portInst);
                        mem_aggregated = "{";
                        for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
                        {
                           if(bitsize_index)
                           {
                              mem_aggregated += ", ";
                           }
                           mem_aggregated += "_bambu_testbench_mem_[paddr" + port_name.substr(0, port_name.size() - 1) +
                                             "0 + " + STR(bitsize <= 8 ? 0 : ((bitsize - bitsize_index) / 8 - 1)) +
                                             " - base_addr + " + port_addr->get_id() + "*" +
                                             STR(GetPointer<port_o>(port_addr)->get_port_alignment()) + "]";
                        }
                        mem_aggregated += "}";
                     }
                     writer->write("always @(posedge " CLOCK_PORT_NAME ")\n");
                     writer->write(STR(STD_OPENING_CHAR));
                     writer->write("begin\n");
                     writer->write("if (" + port_ce->get_id() + " == 1'b1)\n");
                     writer->write(STR(STD_OPENING_CHAR));
                     writer->write("begin\n");
                     writer->write(port_name + " <= " + mem_aggregated + ";\n");
                     writer->write(STR(STD_CLOSING_CHAR));
                     writer->write("end\n");
                     writer->write(STR(STD_CLOSING_CHAR));
                     writer->write("end\n");
                  };
                  manage_pidin("0");
                  portInst = mod->find_member(par + "_q1", port_o_K, cir);
                  if(portInst)
                  {
                     InterfaceType = GetPointer<port_o>(portInst)->get_port_interface();
                     if(InterfaceType == port_o::port_interface::PI_DIN)
                     {
                        manage_pidin("1");
                     }
                  }
                  continue;
               }
               else
               {
                  portInst = structural_objectRef();
               }
            }
         }
         THROW_ASSERT(portInst, "unexpected condition: " + par);
         auto InterfaceType = GetPointer<port_o>(portInst)->get_port_interface();
         auto port_name = portInst->get_id();
         if(InterfaceType == port_o::port_interface::PI_WNONE)
         {
            auto port_vld = mod->find_member(port_name + "_vld", port_o_K, cir);
            auto has_valid =
                port_vld && GetPointer<port_o>(port_vld)->get_port_interface() == port_o::port_interface::PI_WVALID;
            writer->write("always @(negedge " CLOCK_PORT_NAME ")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("if (" + (has_valid ? port_vld->get_id() : DONE_PORT_NAME) + " == 1)\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("registered_" + port_name + " <= " + port_name + ";\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
         }
         else if(InterfaceType == port_o::port_interface::PI_FDIN ||
                 InterfaceType == port_o::port_interface::PI_M_AXIS_TDATA ||
                 InterfaceType == port_o::port_interface::PI_FDOUT ||
                 InterfaceType == port_o::port_interface::PI_S_AXIS_TDATA)
         {
            structural_objectRef port_write;
            if(InterfaceType == port_o::port_interface::PI_FDIN)
            {
               port_write = mod->find_member(port_name.substr(0, port_name.size() - sizeof("_din") + 1U) + "_write",
                                             port_o_K, cir);
               THROW_ASSERT(port_write && GetPointer<port_o>(port_write)->get_port_interface() ==
                                              port_o::port_interface::PI_WRITE,
                            "unexpected condition");
            }
            else if(InterfaceType == port_o::port_interface::PI_FDOUT)
            {
               port_write = mod->find_member(port_name.substr(0, port_name.size() - sizeof("_dout") + 1U) + "_read",
                                             port_o_K, cir);
               THROW_ASSERT(port_write &&
                                GetPointer<port_o>(port_write)->get_port_interface() == port_o::port_interface::PI_READ,
                            "unexpected condition");
            }
            else if(InterfaceType == port_o::port_interface::PI_M_AXIS_TDATA)
            {
               port_write = mod->find_member(port_name.substr(0, port_name.size() - sizeof("_TDATA") + 1U) + "_TVALID",
                                             port_o_K, cir);
               THROW_ASSERT(port_write && GetPointer<port_o>(port_write)->get_port_interface() ==
                                              port_o::port_interface::PI_M_AXIS_TVALID,
                            "unexpected condition");
            }
            else if(InterfaceType == port_o::port_interface::PI_S_AXIS_TDATA)
            {
               port_write = mod->find_member(port_name.substr(0, port_name.size() - sizeof("_TDATA") + 1U) + "_TREADY",
                                             port_o_K, cir);
               THROW_ASSERT(port_write && GetPointer<port_o>(port_write)->get_port_interface() ==
                                              port_o::port_interface::PI_S_AXIS_TREADY,
                            "unexpected condition");
            }
            THROW_ASSERT(port_write, "");

            writer->write("always @(negedge " CLOCK_PORT_NAME ")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("if (" + port_write->get_id() + " == 1)\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            if(InterfaceType == port_o::port_interface::PI_FDIN ||
               InterfaceType == port_o::port_interface::PI_M_AXIS_TDATA)
            {
               writer->write("registered_" + port_name + "[fifo_counter_" + port_name + "] <= " + port_name + ";\n");
            }
            writer->write("fifo_counter_" + port_name + " <= fifo_counter_" + port_name + " + 1;\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
         }
      }
   }
   writer->write("always @(negedge " CLOCK_PORT_NAME ")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("if (start_results_comparison == 1)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");

   if(interface_type == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION or
      interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION or
      interface_type == HLSFlowStep_Type::INTERFACE_CS_GENERATION)
   {
      const auto& DesignSignature = HLSMgr->RSim->simulationArgSignature;
      for(const auto& par : DesignSignature)
      {
         auto portInst = mod->find_member(par, port_o_K, cir);
         if(!portInst)
         {
            portInst = mod->find_member(par + "_o", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_dout", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member("s_axis_" + par + "_TDATA", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_din", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member("m_axis_" + par + "_TDATA", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_d0", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_q0", port_o_K, cir);
         }
         THROW_ASSERT(portInst, "unexpected condition");
         auto InterfaceType = GetPointer<port_o>(portInst)->get_port_interface();
         if(InterfaceType == port_o::port_interface::PI_DEFAULT)
         {
            if(GetPointer<port_o>(portInst)->get_is_memory() ||
               (GetPointer<port_o>(portInst)->get_is_extern() && GetPointer<port_o>(portInst)->get_is_global()) ||
               !portInst->get_typeRef()->treenode || !tree_helper::is_a_pointer(TM, portInst->get_typeRef()->treenode))
            {
               continue;
            }

            std::string unmangled_name = portInst->get_id();
            std::string port_name = HDL_manager::convert_to_identifier(writer.get(), unmangled_name);
            std::string output_name = "ex_" + unmangled_name;
            unsigned long long int bitsize;
            bool is_real;
            const auto pi_node = TM->CGetTreeReindex(portInst->get_typeRef()->treenode);
            if(tree_helper::IsArrayEquivType(pi_node))
            {
               const auto pt_type = tree_helper::CGetArrayBaseType(pi_node);
               bitsize = tree_helper::Size(pt_type);
               is_real = tree_helper::IsRealType(pt_type);
            }
            else
            {
               const auto port_type = tree_helper::CGetType(pi_node);
               auto pt_type = tree_helper::CGetPointedType(port_type);
               if(tree_helper::IsArrayEquivType(pt_type))
               {
                  pt_type = tree_helper::CGetArrayBaseType(pt_type);
               }
               bitsize = tree_helper::Size(pt_type);
               is_real = tree_helper::IsRealType(pt_type);
            }
            writer->write("\n");
            writer->write_comment("OPTIONAL - Read a value for " + unmangled_name +
                                  " --------------------------------------------------------------\n");

            writer->write("_i_ = 0;\n");
            writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\" || _ch_ == \"o\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            {
               writer->write("if (_ch_ == \"o\")\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write("compare_outputs = 1;\n");
                  writer->write(R"(_r_ = $fscanf(file,"%b\n", )" + output_name + "); ");
                  writer->write_comment("expected format: bbb...b (example: 00101110)\n");

                  writer->write("if (_r_ != 1)\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  {
                     writer->write_comment("error\n");
                     writer->write("$display(\"ERROR - Unknown error while reading the file. Character found: %c\", "
                                   "_ch_[7:0]);\n");
                     writer->write("$fclose(res_file);\n");
                     writer->write("$fclose(file);\n");
                     writer->write("$finish;\n");
                  }
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  size_t escaped_pos = port_name.find('\\');
                  std::string nonescaped_name = port_name;
                  if(escaped_pos != std::string::npos)
                  {
                     nonescaped_name.erase(std::remove(nonescaped_name.begin(), nonescaped_name.end(), '\\'),
                                           nonescaped_name.end());
                  }
                  if(is_real)
                  {
                     if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
                     {
                        writer->write("$display(\" comparison = %b " + nonescaped_name +
                                      " = %d "
                                      " _bambu_testbench_mem_[" +
                                      nonescaped_name + " + %d - base_addr] = %20.20f  expected = %20.20f \", ");
                        writer->write("{");
                        for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
                        {
                           if(bitsize_index)
                           {
                              writer->write(", ");
                           }
                           writer->write("_bambu_testbench_mem_[" + port_name + " + _i_*" + STR(bitsize / 8) + " + " +
                                         STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]");
                        }
                        writer->write("} == " + output_name + ", ");
                        writer->write(port_name + ", _i_*" + STR(bitsize / 8) + ", " +
                                      (bitsize == 32 ? "bits32_to_real64" : "$bitstoreal") + "({");
                        for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
                        {
                           if(bitsize_index)
                           {
                              writer->write(", ");
                           }
                           writer->write("_bambu_testbench_mem_[" + port_name + " + _i_*" + STR(bitsize / 8) + " + " +
                                         STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]");
                        }
                        writer->write("}), " + STR(bitsize == 32 ? "bits32_to_real64" : "$bitstoreal") + "(" +
                                      output_name + "));\n");
                     }
                     if(bitsize == 32 || bitsize == 64)
                     {
                        if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
                        {
                           writer->write(R"($display(" FP error %f \n", compute_ulp)" +
                                         (bitsize == 32 ? STR(32) : STR(64)) + "({");
                           for(unsigned int bitsize_index = 0; bitsize_index < bitsize;
                               bitsize_index = bitsize_index + 8)
                           {
                              if(bitsize_index)
                              {
                                 writer->write(", ");
                              }
                              writer->write("_bambu_testbench_mem_[" + port_name + " + _i_*" + STR(bitsize / 8) +
                                            " + " + STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]");
                           }
                           writer->write("}, " + output_name);
                           writer->write("));\n");
                        }
                        writer->write("if (compute_ulp" + (bitsize == 32 ? STR(32) : STR(64)) + "({");
                        for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
                        {
                           if(bitsize_index)
                           {
                              writer->write(", ");
                           }
                           writer->write("_bambu_testbench_mem_[" + port_name + " + _i_*" + STR(bitsize / 8) + " + " +
                                         STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]");
                        }
                        writer->write("}, " + output_name);
                        if(bitsize == 64)
                        {
                           writer->write(") > " + STR(parameters->getOption<double>(OPT_max_ulp)) + ".0)\n");
                        }
                        else
                        {
                           writer->write(") > " + STR(parameters->getOption<double>(OPT_max_ulp)) + ")\n");
                        }
                     }
                     else
                     {
                        THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                                         "floating point precision not yet supported: " + STR(bitsize));
                     }
                  }
                  else
                  {
                     if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
                     {
                        writer->write(
                            "$display(\"" + nonescaped_name + " = %d _bambu_testbench_mem_[" + nonescaped_name +
                            " + %d - base_addr] = %d  expected = %d \\n\", _bambu_testbench_mem_[(" + port_name +
                            " - base_addr) + _i_] == " + output_name + ", _i_, _bambu_testbench_mem_[(" + port_name +
                            " - base_addr) + _i_], " + output_name + ");\n");
                     }
                     writer->write("if (_bambu_testbench_mem_[(" + port_name +
                                   " - base_addr) + _i_] !== " + output_name + ")\n");
                  }
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  writer->write("success = 0;\n");
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  writer->write("_i_ = _i_ + 1;\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");

               writer->write("else\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write_comment("skip comments and empty lines\n");
                  writer->write("_r_ = $fgets(line, file);\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");
            }
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");

            writer->write("if (_ch_ == \"e\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("_r_ = $fgets(line, file);\n");
            writer->write("_ch_ = $fgetc(file);\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            writer->write("else\n");
            writer->write("begin\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write_comment("error\n");
            writer->write(
                "$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
            writer->write("$fclose(res_file);\n");
            writer->write("$fclose(file);\n");
            writer->write("$finish;\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
         }
         else if(InterfaceType == port_o::port_interface::PI_RNONE || InterfaceType == port_o::port_interface::PI_DIN ||
                 InterfaceType == port_o::port_interface::PI_FDOUT ||
                 InterfaceType == port_o::port_interface::PI_S_AXIS_TDATA)
         {
            writer->write("\n");
            writer->write_comment("OPTIONAL - skip expected value for " + portInst->get_id() +
                                  " --------------------------------------------------------------\n");

            writer->write("_i_ = 0;\n");
            writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\" || _ch_ == \"o\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            {
               writer->write("if (_ch_ == \"o\")\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write("compare_outputs = 1;\n");
                  writer->write("_ch_ = $fgetc(file);\n");
                  writer->write(R"(while (_ch_ == "\n" || _ch_ == "0" || _ch_ == "1") )");
                  writer->write("_ch_ = $fgetc(file);\n");
                  writer->write("_i_ = _i_ + 1;\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");

               writer->write("else\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write_comment("skip comments and empty lines\n");
                  writer->write("_r_ = $fgets(line, file);\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");
            }
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");

            writer->write("if (_ch_ == \"e\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("_r_ = $fgets(line, file);\n");
            writer->write("_ch_ = $fgetc(file);\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            writer->write("else\n");
            writer->write("begin\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write_comment("error\n");
            writer->write(
                "$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
            writer->write("$fclose(res_file);\n");
            writer->write("$fclose(file);\n");
            writer->write("$finish;\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
         }
         else if(InterfaceType == port_o::port_interface::PI_WNONE)
         {
            auto orig_name = portInst->get_id();
            auto port_to_be_compared = "registered_" + orig_name;
            std::string output_name = "ex_" + orig_name;
            writer->write("\n");
            writer->write_comment("OPTIONAL - Read a value for " + orig_name +
                                  " --------------------------------------------------------------\n");

            writer->write("_i_ = 0;\n");
            writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\" || _ch_ == \"o\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            {
               writer->write("if (_ch_ == \"o\")\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write("compare_outputs = 1;\n");
                  writer->write(R"(_r_ = $fscanf(file,"%b\n", )" + output_name + "); ");
                  writer->write_comment("expected format: bbb...b (example: 00101110)\n");
                  writer->write("if (_r_ != 1)\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  {
                     writer->write_comment("error\n");
                     writer->write("$display(\"ERROR - Unknown error while reading the file. Character found: %c\", "
                                   "_ch_[7:0]);\n");
                     writer->write("$fclose(res_file);\n");
                     writer->write("$fclose(file);\n");
                     writer->write("$finish;\n");
                  }
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");
                  writer->write("else\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  {
                     if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
                     {
                        writer->write("$display(\"Value found for output " + orig_name + ": %b\", " + output_name +
                                      ");\n");
                     }
                  }
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  if(portInst->get_typeRef()->type == structural_type_descriptor::REAL)
                  {
                     if(GET_TYPE_SIZE(portInst) == 32)
                     {
                        writer->write("$display(\" " + orig_name +
                                      " = %20.20f   expected = %20.20f \", bits32_to_real64(" + port_to_be_compared +
                                      "), bits32_to_real64(" + output_name + "));\n");
                        writer->write(R"($display(" FP error %f \n", compute_ulp32()" + port_to_be_compared + ", " +
                                      output_name + "));\n");
                        writer->write("if (compute_ulp32(" + port_to_be_compared + ", " + output_name + ") > " +
                                      STR(parameters->getOption<double>(OPT_max_ulp)) + ")\n");
                     }
                     else if(GET_TYPE_SIZE(portInst) == 64)
                     {
                        writer->write("$display(\" " + orig_name + " = %20.20f   expected = %20.20f \", $bitstoreal(" +
                                      port_to_be_compared + "), $bitstoreal(" + output_name + "));\n");
                        writer->write(R"($display(" FP error %f \n", compute_ulp64()" + port_to_be_compared + ", " +
                                      output_name + "));\n");
                        writer->write("if (compute_ulp64(" + port_to_be_compared + ", " + output_name + ") > " +
                                      STR(parameters->getOption<double>(OPT_max_ulp)) + ".0)\n");
                     }
                     else
                     {
                        THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                                         "floating point precision not yet supported: " + STR(GET_TYPE_SIZE(portInst)));
                     }
                  }
                  else
                  {
                     if(GET_TYPE_SIZE(portInst) > 64)
                     {
                        writer->write("$display(\" " + orig_name + " = %x   expected = %x \\n\", " +
                                      port_to_be_compared + ", " + output_name + ");\n");
                     }
                     else
                     {
                        writer->write("$display(\" " + orig_name + " = %d   expected = %d \\n\", " +
                                      port_to_be_compared + ", " + output_name + ");\n");
                     }
                     writer->write("if (" + port_to_be_compared + " !== " + output_name + ")\n");
                  }
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  writer->write("success = 0;\n");
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  writer->write("_i_ = _i_ + 1;\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");

               writer->write("else\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write_comment("skip comments and empty lines\n");
                  writer->write("_r_ = $fgets(line, file);\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");
            }
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");

            writer->write("if (_ch_ == \"e\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("_r_ = $fgets(line, file);\n");
            writer->write("_ch_ = $fgetc(file);\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            writer->write("else\n");
            writer->write("begin\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write_comment("error\n");
            writer->write(
                "$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
            writer->write("$fclose(res_file);\n");
            writer->write("$fclose(file);\n");
            writer->write("$finish;\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
         }
         else if(InterfaceType == port_o::port_interface::PI_FDIN ||
                 InterfaceType == port_o::port_interface::PI_M_AXIS_TDATA)
         {
            auto orig_name = portInst->get_id();
            auto port_to_be_compared = "registered_" + orig_name + "[_i_]";
            std::string output_name = "ex_" + orig_name;
            writer->write("\n");
            writer->write_comment("OPTIONAL - Read a value for " + orig_name +
                                  " --------------------------------------------------------------\n");

            writer->write("_i_ = 0;\n");
            writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\" || _ch_ == \"o\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            {
               writer->write("if (_ch_ == \"o\")\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write("compare_outputs = 1;\n");
                  writer->write(R"(_r_ = $fscanf(file,"%b\n", )" + output_name + "); ");
                  writer->write_comment("expected format: bbb...b (example: 00101110)\n");
                  writer->write("if (_r_ != 1)\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  {
                     writer->write_comment("error\n");
                     writer->write("$display(\"ERROR - Unknown error while reading the file. Character found: %c\", "
                                   "_ch_[7:0]);\n");
                     writer->write("$fclose(res_file);\n");
                     writer->write("$fclose(file);\n");
                     writer->write("$finish;\n");
                  }
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");
                  writer->write("else\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  {
                     if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
                     {
                        writer->write("$display(\"Value found for output " + orig_name + ": %b\", " + output_name +
                                      ");\n");
                     }
                  }
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  if(portInst->get_typeRef()->type == structural_type_descriptor::REAL)
                  {
                     if(GET_TYPE_SIZE(portInst) == 32)
                     {
                        writer->write("$display(\" " + orig_name +
                                      " = %20.20f   expected = %20.20f \", bits32_to_real64(" + port_to_be_compared +
                                      "), bits32_to_real64(" + output_name + "));\n");
                        writer->write(R"($display(" FP error %f \n", compute_ulp32()" + port_to_be_compared + ", " +
                                      output_name + "));\n");
                        writer->write("if (compute_ulp32(" + port_to_be_compared + ", " + output_name + ") > " +
                                      STR(parameters->getOption<double>(OPT_max_ulp)) + ")\n");
                     }
                     else if(GET_TYPE_SIZE(portInst) == 64)
                     {
                        writer->write("$display(\" " + orig_name + " = %20.20f   expected = %20.20f \", $bitstoreal(" +
                                      port_to_be_compared + "), $bitstoreal(" + output_name + "));\n");
                        writer->write(R"($display(" FP error %f \n", compute_ulp64()" + port_to_be_compared + ", " +
                                      output_name + "));\n");
                        writer->write("if (compute_ulp64(" + port_to_be_compared + ", " + output_name + ") > " +
                                      STR(parameters->getOption<double>(OPT_max_ulp)) + ".0)\n");
                     }
                     else
                     {
                        THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                                         "floating point precision not yet supported: " + STR(GET_TYPE_SIZE(portInst)));
                     }
                  }
                  else
                  {
                     if(GET_TYPE_SIZE(portInst) > 64)
                     {
                        writer->write("$display(\" " + orig_name + " = %x   expected = %x \\n\", " +
                                      port_to_be_compared + ", " + output_name + ");\n");
                     }
                     else
                     {
                        writer->write("$display(\" " + orig_name + " = %d   expected = %d \\n\", " +
                                      port_to_be_compared + ", " + output_name + ");\n");
                     }
                     writer->write("if (" + port_to_be_compared + " !== " + output_name + ")\n");
                  }
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  writer->write("success = 0;\n");
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  writer->write("_i_ = _i_ + 1;\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");

               writer->write("else\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write_comment("skip comments and empty lines\n");
                  writer->write("_r_ = $fgets(line, file);\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");
            }
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");

            writer->write("if (_ch_ == \"e\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("_r_ = $fgets(line, file);\n");
            writer->write("_ch_ = $fgetc(file);\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            writer->write("else\n");
            writer->write("begin\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write_comment("error\n");
            writer->write(
                "$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
            writer->write("$fclose(res_file);\n");
            writer->write("$fclose(file);\n");
            writer->write("$finish;\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
         }
         else if(InterfaceType == port_o::port_interface::PI_DOUT)
         {
            auto port_name = portInst->get_id();
            auto terminate = port_name.size() > 3 ? port_name.size() - sizeof("_d0") + 1 : 0;
            THROW_ASSERT(port_name.substr(terminate) == "_d0", "inconsistent interface");
            auto orig_name = port_name.substr(0, terminate);

            std::string port_to_be_compared;
            {
               const auto bitsize = local_port_size(portInst);
               const auto port_addr = mod->find_member(orig_name + "_address0", port_o_K, cir);
               THROW_ASSERT(port_addr && GetPointer<port_o>(port_addr)->get_port_interface() ==
                                             port_o::port_interface::PI_ADDRESS,
                            "inconsistent interface");

               port_to_be_compared = "{";
               for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
               {
                  if(bitsize_index)
                  {
                     port_to_be_compared += ", ";
                  }
                  port_to_be_compared += "_bambu_testbench_mem_[paddr" + port_name + " + " +
                                         STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr + _i_*" +
                                         STR(GetPointer<port_o>(port_addr)->get_port_alignment()) + "]";
               }
               port_to_be_compared += "}";
            }
            std::string output_name = "ex_" + port_name;
            writer->write("\n");
            writer->write_comment("OPTIONAL - Read a value for " + orig_name +
                                  " --------------------------------------------------------------\n");

            writer->write("_i_ = 0;\n");
            writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\" || _ch_ == \"o\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            {
               writer->write("if (_ch_ == \"o\")\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write("compare_outputs = 1;\n");
                  writer->write(R"(_r_ = $fscanf(file,"%b\n", )" + output_name + "); ");
                  writer->write_comment("expected format: bbb...b (example: 00101110)\n");
                  writer->write("if (_r_ != 1)\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  {
                     writer->write_comment("error\n");
                     writer->write("$display(\"ERROR - Unknown error while reading the file. Character found: %c\", "
                                   "_ch_[7:0]);\n");
                     writer->write("$fclose(res_file);\n");
                     writer->write("$fclose(file);\n");
                     writer->write("$finish;\n");
                  }
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");
                  writer->write("else\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  {
                     if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
                     {
                        writer->write("$display(\"Value found for output " + orig_name + ": %b\", " + output_name +
                                      ");\n");
                     }
                  }
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  if(portInst->get_typeRef()->type == structural_type_descriptor::REAL)
                  {
                     if(GET_TYPE_SIZE(portInst) == 32)
                     {
                        writer->write("$display(\" " + orig_name +
                                      " = %20.20f   expected = %20.20f \", bits32_to_real64(" + port_to_be_compared +
                                      "), bits32_to_real64(" + output_name + "));\n");
                        writer->write(R"($display(" FP error %f \n", compute_ulp32()" + port_to_be_compared + ", " +
                                      output_name + "));\n");
                        writer->write("if (compute_ulp32(" + port_to_be_compared + ", " + output_name + ") > " +
                                      STR(parameters->getOption<double>(OPT_max_ulp)) + ")\n");
                     }
                     else if(GET_TYPE_SIZE(portInst) == 64)
                     {
                        writer->write("$display(\" " + orig_name + " = %20.20f   expected = %20.20f \", $bitstoreal(" +
                                      port_to_be_compared + "), $bitstoreal(" + output_name + "));\n");
                        writer->write(R"($display(" FP error %f \n", compute_ulp64()" + port_to_be_compared + ", " +
                                      output_name + "));\n");
                        writer->write("if (compute_ulp64(" + port_to_be_compared + ", " + output_name + ") > " +
                                      STR(parameters->getOption<double>(OPT_max_ulp)) + ".0)\n");
                     }
                     else
                     {
                        THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                                         "floating point precision not yet supported: " + STR(GET_TYPE_SIZE(portInst)));
                     }
                  }
                  else
                  {
                     if(GET_TYPE_SIZE(portInst) > 64)
                     {
                        writer->write("$display(\" " + orig_name + " = %x   expected = %x \\n\", " +
                                      port_to_be_compared + ", " + output_name + ");\n");
                     }
                     else
                     {
                        writer->write("$display(\" " + orig_name + " = %d   expected = %d \\n\", " +
                                      port_to_be_compared + ", " + output_name + ");\n");
                     }
                     writer->write("if (" + port_to_be_compared + " !== " + output_name + ")\n");
                  }
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  writer->write("success = 0;\n");
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  writer->write("_i_ = _i_ + 1;\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");

               writer->write("else\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write_comment("skip comments and empty lines\n");
                  writer->write("_r_ = $fgets(line, file);\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");
            }
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");

            writer->write("if (_ch_ == \"e\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("_r_ = $fgets(line, file);\n");
            writer->write("_ch_ = $fgetc(file);\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            writer->write("else\n");
            writer->write("begin\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write_comment("error\n");
            writer->write(
                "$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
            writer->write("$fclose(res_file);\n");
            writer->write("$fclose(file);\n");
            writer->write("$finish;\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
         }
         else
         {
            THROW_ERROR("not supported port interface type");
         }
      }
   }
   else if(interface_type == HLSFlowStep_Type::WB4_INTERFACE_GENERATION)
   {
      const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
      THROW_ASSERT(top_functions.size() == 1, "");
      const auto topFunctionId = *(top_functions.begin());
      const BehavioralHelperConstRef behavioral_helper =
          HLSMgr->CGetFunctionBehavior(topFunctionId)->CGetBehavioralHelper();
      const memoryRef mem = HLSMgr->Rmem;
      const auto function_parameters = mem->get_function_parameters(topFunctionId);
      for(auto const& function_parameter : function_parameters)
      {
         const auto var = function_parameter.first;
         const auto var_node = TM->CGetTreeReindex(var);
         if(tree_helper::IsPointerType(var_node) && var != behavioral_helper->GetFunctionReturnType(topFunctionId))
         {
            const auto variableName = behavioral_helper->PrintVariable(var);
            const auto port_name = HDL_manager::convert_to_identifier(writer.get(), variableName);
            const auto output_name = "ex_" + variableName;
            unsigned long long int bitsize;
            bool is_real;
            if(tree_helper::IsArrayEquivType(var_node))
            {
               const auto pt_type = tree_helper::CGetArrayBaseType(var_node);
               bitsize = tree_helper::Size(pt_type);
               is_real = tree_helper::IsRealType(pt_type);
            }
            else
            {
               const auto pt_type = tree_helper::CGetPointedType(tree_helper::CGetType(var_node));
               bitsize = tree_helper::Size(pt_type);
               is_real = tree_helper::IsRealType(pt_type);
            }

            writer->write("\n");
            writer->write_comment("OPTIONAL - Read a value for " + variableName +
                                  " --------------------------------------------------------------\n");

            writer->write("_i_ = 0;\n");
            writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\" || _ch_ == \"o\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            {
               writer->write("if (_ch_ == \"o\")\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write("compare_outputs = 1;\n");
                  writer->write(R"(_r_ = $fscanf(file,"%b\n", )" + output_name + "); ");
                  writer->write_comment("expected format: bbb...b (example: 00101110)\n");

                  writer->write("if (_r_ != 1)\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  {
                     writer->write_comment("error\n");
                     writer->write("$display(\"ERROR - Unknown error while reading the file. Character found: %c\", "
                                   "_ch_[7:0]);\n");
                     writer->write("$fclose(res_file);\n");
                     writer->write("$fclose(file);\n");
                     writer->write("$finish;\n");
                  }
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");
                  writer->write("else\n");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  {
                     if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
                     {
                        writer->write("$display(\"Value found for output " + variableName + ": %b\", " + output_name +
                                      ");\n");
                     }
                  }
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  size_t escaped_pos = port_name.find('\\');
                  std::string nonescaped_name = port_name;
                  if(escaped_pos != std::string::npos)
                  {
                     nonescaped_name.erase(std::remove(nonescaped_name.begin(), nonescaped_name.end(), '\\'),
                                           nonescaped_name.end());
                  }
                  if(is_real)
                  {
                     if(output_level > OUTPUT_LEVEL_MINIMUM)
                     {
                        writer->write("$display(\" comparison%b " + nonescaped_name +
                                      " = %d "
                                      " _bambu_testbench_mem_[" +
                                      nonescaped_name + " + %d - base_addr] = %20.20f  expected = %20.20f \", ");
                        writer->write("{");
                        for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
                        {
                           if(bitsize_index)
                           {
                              writer->write(", ");
                           }
                           writer->write("_bambu_testbench_mem_[" + port_name + " + _i_*" + STR(bitsize / 8) + " + " +
                                         STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]");
                        }
                        writer->write("} == " + output_name + ", ");
                        writer->write(port_name + ", _i_*" + STR(bitsize / 8) + ", " +
                                      (bitsize == 32 ? "bits32_to_real64" : "$bitstoreal") + "({");
                        for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
                        {
                           if(bitsize_index)
                           {
                              writer->write(", ");
                           }
                           writer->write("_bambu_testbench_mem_[" + port_name + " + _i_*" + STR(bitsize / 8) + " + " +
                                         STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]");
                        }
                        writer->write("}), " + STR(bitsize == 32 ? "bits32_to_real64" : "$bitstoreal") + "(" +
                                      output_name + "));\n");
                     }
                     if(bitsize == 32 || bitsize == 64)
                     {
                        if(output_level > OUTPUT_LEVEL_MINIMUM)
                        {
                           writer->write(R"($display(" FP error %f \n", compute_ulp)" +
                                         (bitsize == 32 ? STR(32) : STR(64)) + "({");
                           for(unsigned int bitsize_index = 0; bitsize_index < bitsize;
                               bitsize_index = bitsize_index + 8)
                           {
                              if(bitsize_index)
                              {
                                 writer->write(", ");
                              }
                              writer->write("_bambu_testbench_mem_[" + port_name + " + _i_*" + STR(bitsize / 8) +
                                            " + " + STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]");
                           }
                           writer->write("}, " + output_name);
                           writer->write("));\n");
                        }
                        writer->write("if (compute_ulp" + (bitsize == 32 ? STR(32) : STR(64)) + "({");
                        for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
                        {
                           if(bitsize_index)
                           {
                              writer->write(", ");
                           }
                           writer->write("_bambu_testbench_mem_[" + port_name + " + _i_*" + STR(bitsize / 8) + " + " +
                                         STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]");
                        }
                        writer->write("}, " + output_name);
                        if(bitsize == 64)
                        {
                           writer->write(") > " + STR(parameters->getOption<double>(OPT_max_ulp)) + ".0)\n");
                        }
                        else
                        {
                           writer->write(") > " + STR(parameters->getOption<double>(OPT_max_ulp)) + ")\n");
                        }
                     }
                     else
                     {
                        THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                                         "floating point precision not yet supported: " + STR(bitsize));
                     }
                  }
                  else
                  {
                     if(output_level > OUTPUT_LEVEL_MINIMUM)
                     {
                        writer->write("$display(\"comparison = %d _bambu_testbench_mem_[" + nonescaped_name +
                                      " + %d - base_addr] = %d  expected = %d \\n\", _bambu_testbench_mem_[(" +
                                      port_name + " - base_addr) + _i_] == " + output_name +
                                      ", _i_, _bambu_testbench_mem_[(" + port_name + " - base_addr) + _i_], " +
                                      output_name + ");\n");
                     }
                     writer->write("if (_bambu_testbench_mem_[(" + port_name +
                                   " - base_addr) + _i_] !== " + output_name + ")\n");
                  }
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  writer->write("success = 0;\n");
                  writer->write(STR(STD_CLOSING_CHAR));
                  writer->write("end\n");

                  writer->write("_i_ = _i_ + 1;\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");

               writer->write("else\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               {
                  writer->write_comment("skip comments and empty lines\n");
                  writer->write("_r_ = $fgets(line, file);\n");
                  writer->write("_ch_ = $fgetc(file);\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");
            }
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");

            writer->write("if (_ch_ == \"e\")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("_r_ = $fgets(line, file);\n");
            writer->write("_ch_ = $fgetc(file);\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            writer->write("else\n");
            writer->write("begin\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write_comment("error\n");
            writer->write(
                "$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
            writer->write("$fclose(res_file);\n");
            writer->write("$fclose(file);\n");
            writer->write("$finish;\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
         }
      }
   }

   if(mod->find_member(RETURN_PORT_NAME, port_o_K, cir))
   {
      std::string output_name = "ex_" RETURN_PORT_NAME;
      structural_objectRef return_port = mod->find_member(RETURN_PORT_NAME, port_o_K, cir);

      writer->write("_i_ = 0;\n");
      writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\" || _ch_ == \"o\")\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      {
         writer->write("if (_ch_ == \"o\")\n");
         writer->write(STR(STD_OPENING_CHAR));
         writer->write("begin\n");
         {
            writer->write("compare_outputs = 1;\n");
            writer->write(R"(_r_ = $fscanf(file,"%b\n", )" + output_name + "); ");
            writer->write_comment("expected format: bbb...b (example: 00101110)\n");

            writer->write("if (_r_ != 1)\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            {
               writer->write_comment("error\n");
               writer->write(
                   "$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
               writer->write("$fclose(res_file);\n");
               writer->write("$fclose(file);\n");
               writer->write("$finish;\n");
            }
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            writer->write("else\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            {
               if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
               {
                  writer->write("$display(\"Value found for output " + output_name + ": %b\", " + output_name + ");\n");
               }
            }
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");

            if(return_port->get_typeRef()->type == structural_type_descriptor::REAL)
            {
               if(GET_TYPE_SIZE(return_port) == 32)
               {
                  writer->write("$display(\" " RETURN_PORT_NAME
                                " = %20.20f   expected = %20.20f \", bits32_to_real64(registered_" RETURN_PORT_NAME
                                "), bits32_to_real64(ex_" RETURN_PORT_NAME "));\n");
                  writer->write(R"($display(" FP error %f \n", compute_ulp32(registered_)" RETURN_PORT_NAME ", " +
                                output_name + "));\n");
                  writer->write("if (compute_ulp32(registered_" RETURN_PORT_NAME ", " + output_name + ") > " +
                                STR(parameters->getOption<double>(OPT_max_ulp)) + ")\n");
               }
               else if(GET_TYPE_SIZE(return_port) == 64)
               {
                  writer->write("$display(\" " RETURN_PORT_NAME
                                " = %20.20f   expected = %20.20f \", $bitstoreal(registered_" RETURN_PORT_NAME
                                "), $bitstoreal(ex_" RETURN_PORT_NAME "));\n");
                  writer->write(R"($display(" FP error %f \n", compute_ulp64(registered_)" RETURN_PORT_NAME ", " +
                                output_name + "));\n");
                  writer->write("if (compute_ulp64(registered_" RETURN_PORT_NAME ", " + output_name + ") > " +
                                STR(parameters->getOption<double>(OPT_max_ulp)) + ".0)\n");
               }
               else
               {
                  THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                                   "floating point precision not yet supported: " + STR(GET_TYPE_SIZE(return_port)));
               }
            }
            else
            {
               writer->write("$display(\" " RETURN_PORT_NAME " = %d   expected = %d \\n\", registered_" RETURN_PORT_NAME
                             ", ex_" RETURN_PORT_NAME ");\n");
               writer->write("if (registered_" RETURN_PORT_NAME " !== " + output_name + ")\n");
            }
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("success = 0;\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");

            writer->write("_i_ = _i_ + 1;\n");
            writer->write("_ch_ = $fgetc(file);\n");
         }
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");

         writer->write("else\n");
         writer->write(STR(STD_OPENING_CHAR));
         writer->write("begin\n");
         {
            writer->write_comment("skip comments and empty lines\n");
            writer->write("_r_ = $fgets(line, file);\n");
            writer->write("_ch_ = $fgetc(file);\n");
         }
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");

      writer->write("if (_ch_ == \"e\")\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      writer->write("_r_ = $fgets(line, file);\n");
      writer->write("_ch_ = $fgetc(file);\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write("else\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      writer->write_comment("error\n");
      writer->write("$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
      writer->write("$fclose(res_file);\n");
      writer->write("$fclose(file);\n");
      writer->write("$finish;\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
   }

   writer->write_comment("Compare output with expected output (if given)\n");
   writer->write("if (compare_outputs == 1)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   {
      bool hasAxi = false;
      if(mod->get_out_port_size())
      {
         for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
         {
            const structural_objectRef& port = mod->get_out_port(i);
            if(GetPointer<port_o>(port)->get_port_interface() == port_o::port_interface::M_AXI_AWADDR)
            {
               hasAxi = true;
            }
         }
      }
      if(hasAxi)
      {
         printCacheStats(mod);
      }

      writer->write("$display(\"Simulation ended after %d cycles.\\n\", sim_time);\n");
      writer->write("if (success == 1)\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      {
         writer->write("$display(\"Simulation completed with success\\n\");\n");
         writer->write("$fwrite(res_file, \"1\\t\");\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write("else\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      {
         writer->write("$display(\"Simulation FAILED\\n\");\n");
         writer->write("$fwrite(res_file, \"0\\t\");\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
   }
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write("else\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   {
      writer->write("$display(\"Simulation ended after %d cycles (no expected outputs specified).\\n\", sim_time);\n");
      writer->write("$fwrite(res_file, \"-\\t\");\n");
   }
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");

   writer->write("$fwrite(res_file, \"%d\\n\", sim_time);\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n\n");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written output checks");
}

void TestbenchGenerationBaseStep::write_underlying_testbench(const std::string simulation_values_path,
                                                             bool generate_vcd_output, bool xilinx_isim,
                                                             const tree_managerConstRef TM) const
{
   if(mod->get_in_out_port_size())
   {
      THROW_ERROR("Module with IO ports cannot be synthesized");
   }
   write_hdl_testbench_prolog();
   write_module_begin();
   write_compute_ulps_functions();
   write_auxiliary_signal_declaration();
   bool withMemory = false;
   bool hasMultiIrq = false;
   write_signals(TM, withMemory, hasMultiIrq);
   write_slave_initializations(withMemory);
   write_module_instantiation(xilinx_isim);
   write_initial_block(simulation_values_path, withMemory, TM, generate_vcd_output);
   begin_file_reading_operation();
   reading_base_memory_address_from_file();
   memory_initialization_from_file();
   write_file_reading_operations();
   end_file_reading_operation();
   if(withMemory)
   {
      write_memory_handler();
   }
   write_interface_handler();
   write_call(hasMultiIrq);
   write_output_checks(TM);
   testbench_controller_machine();
   write_sim_time_calc();
   write_max_simulation_time_control();
   write_module_end();
}

void TestbenchGenerationBaseStep::write_clock_process() const
{
   /// write clock switching operation
   writer->write_comment("Clock switching: 1 cycle every CLOCK_PERIOD seconds\n");
   writer->write("always # `HALF_CLOCK_PERIOD clock = !clock;\n\n");
}

void TestbenchGenerationBaseStep::write_hdl_testbench_prolog() const
{
   if(parameters->getOption<std::string>(OPT_simulator) == "VERILATOR")
   {
      writer->write_comment("verilator lint_off BLKANDNBLK\n");
      writer->write_comment("verilator lint_off BLKSEQ\n");
   }

   writer->write("`timescale 1ns / 1ps\n");
   writer->write_comment("CONSTANTS DECLARATION\n");
   writer->write(
       "`define EOF 32'hFFFF_FFFF\n`define NULL 0\n`define MAX_COMMENT_LENGTH 1000\n`define SIMULATION_LENGTH " +
       STR(parameters->getOption<long long int>(OPT_max_sim_cycles)) + "\n`define INIT_TIME  " +
       std::string(STR_CST_INIT_TIME) + "\n\n");
   auto half_target_period_string = STR(target_period / 2);
   // If the value it is integer, we add .0 to describe a float otherwise modelsim returns conversion error
   if(half_target_period_string.find('.') == std::string::npos)
   {
      half_target_period_string += ".0";
   }
   if(parameters->getOption<std::string>(OPT_simulator) == "VERILATOR")
   {
      writer->write("`define HALF_CLOCK_PERIOD 1\n\n");
   }
   else
   {
      writer->write("`define HALF_CLOCK_PERIOD " + half_target_period_string + "\n\n");
   }
   writer->write("`define CLOCK_PERIOD (2*`HALF_CLOCK_PERIOD)\n\n");
   if(parameters->getOption<std::string>(OPT_bram_high_latency) != "" &&
      parameters->getOption<std::string>(OPT_bram_high_latency) == "_3")
   {
      writer->write("`define MEM_DELAY_READ 3\n\n");
      writer->write("`define MEM_MAX_DELAY " +
                    (parameters->getOption<unsigned>(OPT_mem_delay_write) > 3 ?
                         parameters->getOption<std::string>(OPT_mem_delay_write) :
                         "3") +
                    "\n\n");
   }
   else if(parameters->getOption<std::string>(OPT_bram_high_latency) != "" &&
           parameters->getOption<std::string>(OPT_bram_high_latency) == "_4")
   {
      writer->write("`define MEM_DELAY_READ 4\n\n");
      writer->write("`define MEM_MAX_DELAY " +
                    (parameters->getOption<unsigned>(OPT_mem_delay_write) > 4 ?
                         parameters->getOption<std::string>(OPT_mem_delay_write) :
                         "4") +
                    "\n\n");
   }
   else if(parameters->getOption<std::string>(OPT_bram_high_latency) == "")
   {
      writer->write("`define MEM_DELAY_READ " + parameters->getOption<std::string>(OPT_mem_delay_read) + "\n\n");
      writer->write(
          "`define MEM_MAX_DELAY " +
          (parameters->getOption<unsigned>(OPT_mem_delay_write) > parameters->getOption<unsigned>(OPT_mem_delay_read) ?
               parameters->getOption<std::string>(OPT_mem_delay_write) :
               parameters->getOption<std::string>(OPT_mem_delay_read)) +
          "\n\n");
   }
   else
   {
      THROW_ERROR("unexpected bram high latency delay");
   }
   writer->write("`define MEM_DELAY_WRITE " + parameters->getOption<std::string>(OPT_mem_delay_write) + "\n\n");
}

void TestbenchGenerationBaseStep::write_module_begin() const
{
   writer->write_comment("MODULE DECLARATION\n");
   writer->write("module " + mod->get_id() + "_tb(" + STR(CLOCK_PORT_NAME) + ");\n");
   writer->write(STR(STD_OPENING_CHAR));
}

void TestbenchGenerationBaseStep::write_module_end() const
{
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("endmodule\n\n");
   if(parameters->getOption<std::string>(OPT_simulator) == "VERILATOR")
   {
      writer->write_comment("verilator lint_on BLKANDNBLK\n");
      writer->write_comment("verilator lint_on BLKSEQ\n");
   }
}

void TestbenchGenerationBaseStep::write_module_instantiation(bool xilinx_isim) const
{
   const auto target_language = static_cast<HDLWriter_Language>(parameters->getOption<int>(OPT_writer_language));
   const auto target_writer = target_language == HDLWriter_Language::VERILOG ?
                                  writer :
                                  language_writer::create_writer(
                                      target_language, HLSMgr->get_HLS_target()->get_technology_manager(), parameters);

   /// write module instantiation and ports binding
   writer->write_comment("MODULE INSTANTIATION AND PORTS BINDING\n");
   auto module_name = HDL_manager::get_mod_typename(target_writer.get(), cir);
   if(module_name[0] == '\\' and target_language == HDLWriter_Language::VHDL)
   {
      module_name = "\\" + module_name;
   }
   writer->write_module_instance_begin(cir, module_name, !xilinx_isim);
   std::string prefix = "";
   if(mod->get_in_port_size())
   {
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         if(i == 0)
         {
            prefix = ".";
         }
         else
         {
            prefix = ", .";
         }

         auto port_name_formal = HDL_manager::convert_to_identifier(writer.get(), mod->get_in_port(i)->get_id());
         auto port_name_actual = port_name_formal;
         if(parameters->isOption(OPT_clock_name) &&
            port_name_actual == parameters->getOption<std::string>(OPT_clock_name))
         {
            port_name_actual = CLOCK_PORT_NAME;
         }
         else if(parameters->isOption(OPT_reset_name) &&
                 port_name_actual == parameters->getOption<std::string>(OPT_reset_name))
         {
            port_name_actual = RESET_PORT_NAME;
         }
         else if(parameters->isOption(OPT_start_name) &&
                 port_name_actual == parameters->getOption<std::string>(OPT_start_name))
         {
            port_name_actual = START_PORT_NAME;
         }
         writer->write(prefix + port_name_formal + "(" + port_name_actual + ")");
      }
   }
   if(mod->get_out_port_size())
   {
      for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         auto port_name_formal = HDL_manager::convert_to_identifier(writer.get(), mod->get_out_port(i)->get_id());
         auto port_name_actual = port_name_formal;
         if(parameters->isOption(OPT_done_name) &&
            port_name_actual == parameters->getOption<std::string>(OPT_done_name))
         {
            port_name_actual = DONE_PORT_NAME;
         }
         writer->write(prefix + port_name_formal + "(" + port_name_actual + ")");
      }
   }

   writer->write_module_instance_end(cir);

   if(xilinx_isim)
   {
      writer->write("glbl #(" + STR(target_period / 2 * 1000) + ", 0) glbl();");
   }
}

void TestbenchGenerationBaseStep::write_auxiliary_signal_declaration() const
{
   const auto testbench_memsize = [&]() {
      const auto mem_size =
          HLSMgr->Rmem->get_memory_address() - parameters->getOption<unsigned long long int>(OPT_base_address);
      return mem_size ? mem_size : 1;
   }();
   writer->write("parameter MEMSIZE = " + STR(testbench_memsize));

   /// writing memory-related parameters
   if(mod->ExistsParameter(MEMORY_PARAMETER))
   {
      const auto memory_str = mod->GetParameter(MEMORY_PARAMETER);
      const auto mem_tag = convert_string_to_vector<std::string>(memory_str, ";");
      for(const auto& i : mem_tag)
      {
         const auto mem_add = convert_string_to_vector<std::string>(i, "=");
         THROW_ASSERT(mem_add.size() == 2, "malformed address");
         writer->write(", ");
         std::string name = mem_add[0];
         std::string value = mem_add[1];
         if(value.find("\"\"") != std::string::npos)
         {
            boost::replace_all(value, "\"\"", "\"");
         }
         else if(value.find('\"') != std::string::npos)
         {
            boost::replace_all(value, "\"", "");
            value = STR(value.size()) + "'b" + value;
         }
         writer->write(name + "=" + value);
      }
   }
   writer->write(";\n");

   writer->write_comment("AUXILIARY VARIABLES DECLARATION\n");
   writer->write("time startTime, endTime, sim_time;\n");
   writer->write("integer res_file, file, stats_file, _r_, _n_, _i_, _addr_i_;\n");
   writer->write("integer _ch_;\n");
   writer->write("reg compare_outputs, success; // Flag: True if input vector specifies expected outputs\n");
   writer->write("reg [8*`MAX_COMMENT_LENGTH:0] line; // Comment line read from file\n\n");

   writer->write("reg [31:0] addr, base_addr;\n");
   /* Check if there is at least one interface type */
   bool type_found = false;

   for(auto& fun : HLSMgr->design_attributes)
   {
      for(auto& par : fun.second)
      {
         if(par.second.count(attr_interface_type))
         {
            type_found = true;
         }
      }
   }

   if(type_found)
   {
      const auto& DesignSignature = HLSMgr->RSim->simulationArgSignature;
      bool writeP = false;
      for(const auto& par : DesignSignature)
      {
         auto portInst = mod->find_member(par, port_o_K, cir);
         if(!portInst)
         {
            portInst = mod->find_member(par + "_i", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_dout", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member("s_axis_" + par + "_TDATA", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_din", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member("m_axis_" + par + "_TDATA", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_d0", port_o_K, cir);
         }
         if(!portInst)
         {
            portInst = mod->find_member(par + "_q0", port_o_K, cir);
         }
         THROW_ASSERT(portInst, "unexpected condition");
         const auto InterfaceType = GetPointer<port_o>(portInst)->get_port_interface();
         const auto input_name = HDL_manager::convert_to_identifier(writer.get(), portInst->get_id());
         if(InterfaceType == port_o::port_interface::PI_RNONE || InterfaceType == port_o::port_interface::PI_WNONE ||
            InterfaceType == port_o::port_interface::PI_DIN || InterfaceType == port_o::port_interface::PI_DOUT ||
            InterfaceType == port_o::port_interface::PI_FDOUT ||
            InterfaceType == port_o::port_interface::PI_S_AXIS_TDATA ||
            InterfaceType == port_o::port_interface::PI_FDIN ||
            InterfaceType == port_o::port_interface::PI_M_AXIS_TDATA)
         {
            writer->write("reg [31:0] paddr" + input_name + ";\n");
            writeP = true;
         }
      }
      if(writeP)
      {
         writer->write("\n");
      }
   }

   /* Check if AWADDR ports are present. If there are, declare a variable to store the last valid AWADDR for each
    * bundle and the delay vectors
    */
   if(mod->get_out_port_size())
   {
      for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         const auto port = mod->get_out_port(i);
         if(GetPointer<port_o>(port)->get_port_interface() == port_o::port_interface::M_AXI_AWADDR)
         {
            const auto bitsize =
                GetPointer<port_o>(port)->get_typeRef()->size * GetPointer<port_o>(port)->get_typeRef()->vector_size;

            const std::string portQual = "AWADDR";
            auto portPrefix = GetPointer<port_o>(port)->get_id();
            auto idx = portPrefix.find(portQual);

            if(idx != std::string::npos)
            {
               portPrefix.erase(idx, portQual.length());
            }

            writer->write("reg [" + STR(bitsize - 1) + ":0] last_" + GetPointer<port_o>(port)->get_id() + ";\n");

            const auto portAWADDR = mod->find_member(portPrefix + "AWADDR", port_o_K, cir);
            const auto portWDATA = mod->find_member(portPrefix + "WDATA", port_o_K, cir);

            const auto wAddrSize = GetPointer<port_o>(portAWADDR)->get_typeRef()->size *
                                   GetPointer<port_o>(portAWADDR)->get_typeRef()->vector_size;
            const auto wDataSize = GetPointer<port_o>(portWDATA)->get_typeRef()->size *
                                   GetPointer<port_o>(portWDATA)->get_typeRef()->vector_size;

            writer->write("reg [" + STR(wDataSize - 1) + ":0] " + portPrefix + "wBitmask;\n");
            writer->write("reg [" + STR(wAddrSize - 1) + ":0] " + portPrefix + "currAddr;\n");
            writer->write("reg [" + STR(wAddrSize - 1) + ":0] " + portPrefix + "endAddr;\n");

            /* Get queue size from pragma parameters, if present */
            std::string queueSize = "10";
            const std::string interfaceType = "m_axi_";
            THROW_ASSERT(portPrefix.find(interfaceType) == 0 && portPrefix.back() == '_',
                         "Unexpected port prefix format");
            std::string bundleName = portPrefix;
            idx = 0;
            bundleName.erase(idx, interfaceType.length());
            idx = bundleName.size() - 1;
            bundleName.erase(idx, 1);

            /* Look for a matching bundle name with defined buffer size */
            for(const auto& fun : HLSMgr->design_attributes)
            {
               for(const auto& par : fun.second)
               {
                  if(par.second.find(attr_bundle_name) != par.second.end() &&
                     par.second.at(attr_bundle_name) == bundleName &&
                     par.second.find(attr_buf_size) != par.second.end() && par.second.at(attr_buf_size) != "")
                  {
                     queueSize = par.second.at(attr_buf_size);
                  }
               }
            }
            writer->write("`define " + portPrefix + "MAX_QUEUE_SIZE " + queueSize + "\n");
            writer->write("reg [" + STR(ADDR_HIGH_INDEX) + " : 0] " + portPrefix + "awqueue [`" + portPrefix +
                          "MAX_QUEUE_SIZE" + " - 1 : 0];\n");
            writer->write("reg [" + STR(ADDR_HIGH_INDEX) + " : 0] " + portPrefix + "arqueue [`" + portPrefix +
                          "MAX_QUEUE_SIZE" + " - 1 : 0];\n");
            writer->write("integer " + portPrefix + "awqueue_size = 0;\n");
            writer->write("integer " + portPrefix + "arqueue_size = 0;\n");
         }
      }
   }

   writer->write("reg [7:0] _bambu_testbench_mem_ [0:MEMSIZE-1];\n\n");
   writer->write("reg [7:0] _bambu_databyte_;\n\n");
   writer->write("reg [3:0] __state, __next_state;\n");
   writer->write("reg start_results_comparison;\n");
   writer->write("reg next_start_port;\n");
   writer->write("integer currTime;\n");
}

void TestbenchGenerationBaseStep::begin_initial_block() const
{
   writer->write("\n");
   writer->write_comment("Operation to be executed just one time\n");
   writer->write("initial\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
}

void TestbenchGenerationBaseStep::end_initial_block() const
{
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
}

void TestbenchGenerationBaseStep::open_value_file(const std::string& input_values_filename) const
{
   writer->write_comment("OPEN FILE WITH VALUES FOR SIMULATION\n");
   writer->write("file = $fopen(\"" + input_values_filename + "\",\"r\");\n");
   writer->write_comment("Error in file open\n");
   writer->write("if (file == `NULL)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("$display(\"ERROR - Error opening the file\");\n");
   writer->write("$finish;");
   writer->write_comment("Terminate\n");
   writer->write("");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
}

void TestbenchGenerationBaseStep::open_result_file(const std::string& result_file) const
{
   writer->write_comment("OPEN FILE WHERE results will be written\n");
   writer->write("res_file = $fopen(\"" + result_file + "\",\"w\");\n\n");
   writer->write_comment("Error in file open\n");
   writer->write("if (res_file == `NULL)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("$display(\"ERROR - Error opening the res_file\");\n");
   writer->write("$fclose(file);\n");
   writer->write("$finish;");
   writer->write_comment("Terminate\n");
   writer->write("");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
}

void TestbenchGenerationBaseStep::initialize_auxiliary_variables() const
{
   writer->write_comment("Variables initialization\n");
   writer->write("sim_time = 0;\n");
   writer->write("startTime = 0;\n");
   writer->write("endTime = 0;\n");
   writer->write("_ch_ = 0;\n");
   writer->write("_n_ = 0;\n");
   writer->write("_r_ = 0;\n");
   writer->write("line = 0;\n");
   writer->write("_i_ = 0;\n");
   writer->write("_addr_i_ = 0;\n");
   writer->write("compare_outputs = 0;\n");
   writer->write("start_next_sim = 0;\n");
   writer->write("__next_state = 0;\n");
   writer->write(RESET_PORT_NAME " = 0;\n");
   writer->write("next_start_port = 0;\n");
   writer->write("success = 1;\n");
}

void TestbenchGenerationBaseStep::initialize_input_signals(const tree_managerConstRef TM) const
{
   for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
   {
      const auto port_obj = mod->get_in_port(i);
      const auto port_if = GetPointer<port_o>(port_obj)->get_port_interface();
      const auto port_name = [&]() -> std::string {
         const auto port_id = port_obj->get_id();
         if(parameters->isOption(OPT_clock_name) && port_id == parameters->getOption<std::string>(OPT_clock_name))
         {
            return CLOCK_PORT_NAME;
         }
         else if(parameters->isOption(OPT_reset_name) && port_id == parameters->getOption<std::string>(OPT_reset_name))
         {
            return RESET_PORT_NAME;
         }
         else if(parameters->isOption(OPT_start_name) && port_id == parameters->getOption<std::string>(OPT_start_name))
         {
            return START_PORT_NAME;
         }
         return port_id;
      }();

      if(GetPointer<port_o>(port_obj)->get_is_memory() || WB_ACKIM_PORT_NAME == port_name)
      {
         continue;
      }
      if(CLOCK_PORT_NAME != port_name && START_PORT_NAME != port_name && RESET_PORT_NAME != port_name)
      {
         writer->write(HDL_manager::convert_to_identifier(writer.get(), port_name) + " = 0;\n");
      }
      if(port_obj->get_typeRef()->treenode > 0 && tree_helper::is_a_pointer(TM, port_obj->get_typeRef()->treenode))
      {
         writer->write("ex_" + port_name + " = 0;\n");
      }
      if(port_if == port_o::port_interface::PI_FDOUT || port_if == port_o::port_interface::PI_S_AXIS_TDATA)
      {
         writer->write("fifo_counter_" + port_obj->get_id() + " = 0;\n");
      }
   }
   writer->write("\n");
   for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
   {
      const auto port_obj = mod->get_out_port(i);
      const auto interfaceType = GetPointer<port_o>(port_obj)->get_port_interface();
      if(interfaceType == port_o::port_interface::PI_WNONE)
      {
         writer->write("ex_" + port_obj->get_id() + " = 0;\n");
         writer->write("registered_" + port_obj->get_id() + " = 0;\n");
      }
      else if(interfaceType == port_o::port_interface::PI_FDIN ||
              interfaceType == port_o::port_interface::PI_M_AXIS_TDATA)
      {
         writer->write("fifo_counter_" + port_obj->get_id() + " = 0;\n");
      }
      else if(interfaceType == port_o::port_interface::PI_DOUT)
      {
         writer->write("ex_" + port_obj->get_id() + " = 0;\n");
      }
   }
   writer->write("\n");
}

void TestbenchGenerationBaseStep::testbench_controller_machine() const
{
   writer->write("always @(*)\n");
   writer->write("  begin\n");
   writer->write("     start_results_comparison = 0;\n");
   if(!parameters->getOption<bool>(OPT_reset_level))
   {
      writer->write("     " RESET_PORT_NAME " = 1;\n");
   }
   else
   {
      writer->write("     " RESET_PORT_NAME " = 0;\n");
   }
   writer->write("     start_next_sim = 0;\n");
   writer->write("     next_start_port = 0;\n");
   writer->write("     case (__state)\n");
   writer->write("       0:\n");
   writer->write("         begin\n");
   if(!parameters->getOption<bool>(OPT_reset_level))
   {
      writer->write("            " RESET_PORT_NAME " = 0;\n");
   }
   else
   {
      writer->write("            " RESET_PORT_NAME " = 1;\n");
   }
   writer->write("            __next_state = 1;\n");
   writer->write("         end\n");
   writer->write("       1:\n");
   writer->write("         begin\n");
   if(!parameters->getOption<bool>(OPT_reset_level))
   {
      writer->write("            " RESET_PORT_NAME " = 0;\n");
   }
   else
   {
      writer->write("            " RESET_PORT_NAME " = 1;\n");
   }
   writer->write("            __next_state = 2;\n");
   writer->write("         end\n");
   writer->write("       2:\n");
   writer->write("         if(currTime > `INIT_TIME)\n");
   writer->write("           begin\n");
   writer->write("              next_start_port = 1;\n");
   writer->write("              if (done_port == 1'b1)\n");
   writer->write("                begin\n");
   writer->write("                  __next_state = 4;\n");
   writer->write("                end\n");
   writer->write("              else\n");
   writer->write("                __next_state = 3;\n");
   writer->write("           end\n");
   writer->write("         else\n");
   writer->write("           __next_state = 2;\n");
   writer->write("       3:\n");
   writer->write("         if (done_port == 1'b1)\n");
   writer->write("           begin\n");
   writer->write("              __next_state = 4;\n");
   writer->write("           end\n");
   writer->write("         else\n");
   writer->write("           __next_state = 3;\n");
   writer->write("       4:\n");
   writer->write("         begin\n");
   writer->write("            start_results_comparison = 1;\n");
   writer->write("            __next_state = 5;\n");
   writer->write("         end\n");
   writer->write("       5:\n");
   writer->write("         begin\n");
   if(HLSMgr->RSim->test_vectors.size() <= 1)
   {
      writer->write_comment("wait a cycle (needed for a correct simulation)\n");
      writer->write("            $fclose(res_file);\n");
      writer->write("            $fclose(file);\n");
      writer->write("            $finish;\n");
   }
   else
   {
      writer->write_comment("Restart a new computation if possible\n");
      writer->write("            __next_state = 2;\n");
   }
   writer->write("         end\n");
   writer->write("       default:\n");
   writer->write("         begin\n");
   writer->write("            __next_state = 0;\n");
   writer->write("         end\n");
   writer->write("     endcase // case (__state)\n");
   writer->write("  end // always @ (*)\n");

   writer->write("always @(posedge " CLOCK_PORT_NAME ")\n");
   writer->write("  begin\n");
   writer->write("  __state <= __next_state;\n");
   writer->write("  start_port <= next_start_port;\n");
   writer->write("  end\n");

   /* Look for AWADDR ports. For every port, write the AXI signals controller */
   std::string portPrefix;
   if(mod->get_out_port_size())
   {
      for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         const auto port = mod->get_out_port(i);
         if(GetPointer<port_o>(port)->get_port_interface() == port_o::port_interface::M_AXI_AWADDR)
         {
            const std::string portSpecializer = "AWADDR";
            const auto index = GetPointer<port_o>(port)->get_id().find(portSpecializer);
            if(index != std::string::npos)
            {
               portPrefix = GetPointer<port_o>(port)->get_id();
               portPrefix.erase(index, portSpecializer.length());
            }

            writer->write("always@(posedge " CLOCK_PORT_NAME ") begin\n");
            writer->write("  " + portPrefix + "ARREADY <= (" + portPrefix + "arqueue_size < `" + portPrefix +
                          "MAX_QUEUE_SIZE);\n");
            writer->write("  " + portPrefix + "AWREADY <= (" + portPrefix + "awqueue_size < `" + portPrefix +
                          "MAX_QUEUE_SIZE);\n");
            writer->write("  " + portPrefix + "WREADY <= 1'b1;\n");
            writer->write("  " + portPrefix + "BVALID <= 1'b0;\n");
            writer->write("  " + portPrefix + "RVALID <= 1'b0;\n");
            writer->write("  " + portPrefix + "RRESP <= 2'b00;\n");
            writer->write("  " + portPrefix + "RLAST <= 1'b0;\n");
            writer->write("  if(" + portPrefix + "AWVALID) begin\n");
            writer->write("    if(" + portPrefix + "awqueue_size < `" + portPrefix + "MAX_QUEUE_SIZE) begin\n");
            writer->write("      " + portPrefix + "awqueue[" + portPrefix + "awqueue_size] = {" + portPrefix +
                          "AWADDR, " + portPrefix + "AWSIZE, " + portPrefix + "AWLEN, " + portPrefix +
                          "AWBURST, 32'd1};\n");
            writer->write("      " + portPrefix + "awqueue_size <= " + portPrefix + "awqueue_size + 1;\n");
            writer->write("    end\n");
            writer->write("  end\n");
            writer->write("  if(" + portPrefix + "ARVALID) begin\n");
            writer->write("    if(" + portPrefix + "arqueue_size < `" + portPrefix + "MAX_QUEUE_SIZE) begin\n");
            writer->write("      " + portPrefix + "arqueue[" + portPrefix + "arqueue_size] <= {" + portPrefix +
                          "ARADDR, " + portPrefix + "ARSIZE, " + portPrefix + "ARLEN, " + portPrefix +
                          "ARBURST, -(32'd`MEM_DELAY_READ)};\n");
            writer->write("      " + portPrefix + "arqueue_size <= " + portPrefix + "arqueue_size + 1;\n");
            writer->write("    end\n");
            writer->write("  end\n");

            /* Increment delay counters at each clock cycle, if they are in the delay phase */
            writer->write("  for(_i_ = 0; _i_ < " + portPrefix + "awqueue_size; _i_ = _i_ + 1) begin\n");
            writer->write("    if(" + portPrefix + "awqueue[_i_][" + STR(COUNT_HIGH_INDEX) + "] == 1'b1) begin\n");
            writer->write("      " + portPrefix + "awqueue[_i_][" + STR(COUNT_HIGH_INDEX) + " : " +
                          STR(COUNT_LOW_INDEX) + "] = " + portPrefix + "awqueue[_i_][" + STR(COUNT_HIGH_INDEX) + " : " +
                          STR(COUNT_LOW_INDEX) + "] + 1;\n");
            writer->write("    end\n");
            writer->write("  end\n");
            writer->write("  for(_i_ = 0; _i_ < " + portPrefix + "arqueue_size; _i_ = _i_ + 1) begin\n");
            writer->write("    if(" + portPrefix + "arqueue[_i_][" + STR(COUNT_HIGH_INDEX) + "] == 1'b1) begin\n");
            writer->write("      " + portPrefix + "arqueue[_i_][" + STR(COUNT_HIGH_INDEX) + " : " +
                          STR(COUNT_LOW_INDEX) + "] = " + portPrefix + "arqueue[_i_][" + STR(COUNT_HIGH_INDEX) + " : " +
                          STR(COUNT_LOW_INDEX) + "] + 1;\n");
            writer->write("    end\n");
            writer->write("  end\n");

            /* Check if the first element of the write queue is supposed to be handled (delay = 0) */
            writer->write("  if(" + portPrefix + "awqueue_size > 0 && " + portPrefix + "awqueue[0][" +
                          STR(COUNT_HIGH_INDEX) + " : " + STR(COUNT_LOW_INDEX) + "] == 0) begin \n");
            writer->write("    " + portPrefix + "BRESP <= 2'b00;\n");
            writer->write("    " + portPrefix + "BVALID <= 1'b1;\n");
            writer->write("    if(" + portPrefix + "BREADY) begin\n");
            writer->write("      for(_i_ = 1; _i_ < " + portPrefix + "awqueue_size; _i_ = _i_ + 1) begin\n");
            writer->write("        " + portPrefix + "awqueue[_i_ - 1] = " + portPrefix + "awqueue[_i_];\n");
            writer->write("      end\n");
            /* It is possible that we are both removing and adding an element from the queue. In this case, the size
             * is not yet updated, so we must move the new element outside the loop and keep the same queue length in
             * this clock cycle */
            writer->write("      if(" + portPrefix + "AWVALID && " + portPrefix + "AWREADY) begin\n");
            writer->write("        " + portPrefix + "awqueue[" + portPrefix + "awqueue_size - 1] = " + portPrefix +
                          "awqueue[" + portPrefix + "awqueue_size];\n");
            writer->write("        " + portPrefix + "awqueue_size <= " + portPrefix + "awqueue_size;\n");
            writer->write("      end else begin\n");
            writer->write("        " + portPrefix + "awqueue_size <= " + portPrefix + "awqueue_size - 1;\n");
            writer->write("      end\n");
            writer->write("    end\n");
            writer->write("  end\n");

            writer->write("  if(" + portPrefix + "WVALID) begin\n");
            /* Find the correct transaction. It is the first transaction whose counter is not negative */
            /* Assert is not available in verilog, using empty if */
            writer->write(
                "    if(" + portPrefix + "awqueue_size > 0 || " + portPrefix +
                "AWVALID == 1'b1); else $error(\"Received data on write channel, but no transaction in queue\");\n");
            writer->write("    if(" + portPrefix + "awqueue_size > 0) begin\n");
            writer->write("      _i_ = 0;\n");
            writer->write("      while(" + portPrefix + "awqueue[_i_][" + STR(COUNT_HIGH_INDEX) + "] == 1 && _i_ < " +
                          portPrefix + "awqueue_size) begin\n");
            writer->write("        _i_ = _i_ + 1;\n");
            writer->write("      end\n");
            writer->write("      if(_i_ < " + portPrefix + "awqueue_size || " + portPrefix +
                          "AWVALID == 1'b1); else $error(\"Received write data, but all transactions in queue are in "
                          "delay phase\");\n");
            writer->write("      if(_i_ < " + portPrefix + "awqueue_size) begin \n");
            writer->write("        if(" + portPrefix + "awqueue[_i_][" + STR(BURST_HIGH_INDEX) + " : " +
                          STR(BURST_LOW_INDEX) + "] == 2'b00) begin\n");
            /* Fixed burst */
            writer->write("          " + portPrefix + "currAddr = " + portPrefix + "awqueue[_i_][" +
                          STR(ADDR_HIGH_INDEX) + " : " + STR(ADDR_LOW_INDEX) + "];\n");
            writer->write("        end else if(" + portPrefix + "awqueue[_i_][" + STR(BURST_HIGH_INDEX) + " : " +
                          STR(BURST_LOW_INDEX) + "] == 2'b01) begin\n");
            /* Incremental burst */
            writer->write("          " + portPrefix + "currAddr = " + portPrefix + "awqueue[_i_][" +
                          STR(ADDR_HIGH_INDEX) + " : " + STR(ADDR_LOW_INDEX) + "] + (" + portPrefix + "awqueue[_i_][" +
                          STR(COUNT_HIGH_INDEX) + " : " + STR(COUNT_LOW_INDEX) + "] - 1) * (1 << " + portPrefix +
                          "awqueue[_i_][" + STR(SIZE_HIGH_INDEX) + " : " + STR(SIZE_LOW_INDEX) + "]);\n");
            writer->write("        end else if(" + portPrefix + "awqueue[_i_][" + STR(BURST_HIGH_INDEX) + " : " +
                          STR(BURST_LOW_INDEX) + "] == 2'b10) begin\n");
            /* Wrap burst */
            writer->write("          " + portPrefix + "endAddr = " + portPrefix + "awqueue[_i_][" +
                          STR(ADDR_HIGH_INDEX) + " : " + STR(ADDR_LOW_INDEX) + "] - (" + portPrefix + "awqueue[_i_][" +
                          STR(ADDR_HIGH_INDEX) + " : " + STR(ADDR_LOW_INDEX) + "] % ((" + portPrefix + "awqueue[_i_][" +
                          STR(LEN_HIGH_INDEX) + " : " + STR(LEN_LOW_INDEX) + "] + 1) * (1 << " + portPrefix +
                          "awqueue[_i_][" + STR(SIZE_HIGH_INDEX) + " : " + STR(SIZE_LOW_INDEX) + "]))) + ((" +
                          portPrefix + "awqueue[_i_][" + STR(LEN_HIGH_INDEX) + " : " + STR(LEN_LOW_INDEX) +
                          "] + 1) * (1 << " + portPrefix + "awqueue[_i_][" + STR(SIZE_HIGH_INDEX) + " : " +
                          STR(SIZE_LOW_INDEX) + "]));\n");
            writer->write("          " + portPrefix + "currAddr = " + portPrefix + "awqueue[_i_][" +
                          STR(ADDR_HIGH_INDEX) + " : " + STR(ADDR_LOW_INDEX) + "] + (" + portPrefix + "awqueue[_i_][" +
                          STR(COUNT_HIGH_INDEX) + " : " + STR(COUNT_LOW_INDEX) + "] - 1) * (1 << " + portPrefix +
                          "awqueue[_i_][" + STR(SIZE_HIGH_INDEX) + " : " + STR(SIZE_LOW_INDEX) + "]);\n");
            writer->write("          if(" + portPrefix + "currAddr > " + portPrefix + "endAddr) begin\n");
            writer->write("            " + portPrefix + "currAddr = " + portPrefix + "currAddr - ((" + portPrefix +
                          "awqueue[_i_][" + STR(LEN_HIGH_INDEX) + " : " + STR(LEN_LOW_INDEX) + "] + 1) * (1 << " +
                          portPrefix + "awqueue[_i_][" + STR(SIZE_HIGH_INDEX) + " : " + STR(SIZE_LOW_INDEX) + "]));\n");
            writer->write("          end\n");
            writer->write("        end\n");
            writer->write("      end else begin\n");
            writer->write("        " + portPrefix + "currAddr = " + portPrefix + "AWADDR;\n");
            writer->write("      end\n");
            writer->write("    end else begin\n");
            writer->write("      " + portPrefix + "currAddr = " + portPrefix + "AWADDR;\n");
            writer->write("    end\n");
            /* Realign address */
            writer->write("    " + portPrefix + "currAddr = " + portPrefix + "currAddr - (" + portPrefix +
                          "currAddr % (1 << " + portPrefix + "awqueue[_i_][" + STR(SIZE_HIGH_INDEX) + " : " +
                          STR(SIZE_LOW_INDEX) + "]));\n");

            /* Compute bitmask and overwrite data */
            const auto portWDATA = mod->find_member(portPrefix + "WDATA", port_o_K, cir);
            const auto bitsizeWDATA = GetPointer<port_o>(portWDATA)->get_typeRef()->size *
                                      GetPointer<port_o>(portWDATA)->get_typeRef()->vector_size;

            for(unsigned bitsize_index = 0; bitsize_index < bitsizeWDATA; bitsize_index = bitsize_index + 8)
            {
               writer->write("    " + portPrefix + "wBitmask[" + STR(bitsize_index + 7) + " : " + STR(bitsize_index) +
                             "] = {8{" + portPrefix + "WSTRB[" + STR(bitsize_index / 8) + "]}};\n");
            }
            for(unsigned bitsize_index = 0; bitsize_index < bitsizeWDATA; bitsize_index = bitsize_index + 8)
            {
               writer->write("    _bambu_testbench_mem_[" + portPrefix + "currAddr + " +
                             STR((bitsizeWDATA - bitsize_index) / 8 - 1) + " - base_addr] <= (_bambu_testbench_mem_[" +
                             portPrefix + "currAddr + " + STR((bitsizeWDATA - bitsize_index) / 8 - 1) +
                             " - base_addr] & ~" + portPrefix + "wBitmask[" + STR(bitsizeWDATA - bitsize_index - 1) +
                             " : " + STR(bitsizeWDATA - bitsize_index - 8) + "]) | (" + portPrefix + "WDATA[" +
                             STR(bitsizeWDATA - bitsize_index - 1) + " : " + STR(bitsizeWDATA - bitsize_index - 8) +
                             "] & " + portPrefix + "wBitmask[" + STR(bitsizeWDATA - bitsize_index - 1) + " : " +
                             STR(bitsizeWDATA - bitsize_index - 8) + "]);\n");
            }
            writer->write("    if(" + portPrefix + "WLAST) begin\n");
            writer->write("      " + portPrefix + "awqueue[_i_][" + STR(COUNT_HIGH_INDEX) + " : " +
                          STR(COUNT_LOW_INDEX) + "] <= -(32'd`MEM_DELAY_WRITE - 1);\n");
            writer->write("    end else begin\n");
            writer->write("      " + portPrefix + "awqueue[_i_][" + STR(COUNT_HIGH_INDEX) + " : " +
                          STR(COUNT_LOW_INDEX) + "] <= " + portPrefix + "awqueue[_i_][" + STR(COUNT_HIGH_INDEX) +
                          " : " + STR(COUNT_LOW_INDEX) + "] + 1;\n");
            writer->write("    end\n");
            writer->write("  end\n");

            /* Check if the first element in the read queue is ready to be handled (delay is positive) */
            writer->write("  if(" + portPrefix + "arqueue_size > 0 && " + portPrefix + "arqueue[0][" +
                          STR(COUNT_HIGH_INDEX) + "] == 1'b0) begin\n");
            writer->write("    " + portPrefix + "RVALID <= 1'b1;\n");
            writer->write("    if(" + portPrefix + "arqueue[0][" + STR(BURST_HIGH_INDEX) + " : " +
                          STR(BURST_LOW_INDEX) + "] == 2'b00) begin\n");
            /* Fixed burst */
            writer->write("      " + portPrefix + "currAddr = " + portPrefix + "arqueue[0][" + STR(ADDR_HIGH_INDEX) +
                          " : " + STR(ADDR_LOW_INDEX) + "];\n");
            writer->write("    end else if(" + portPrefix + "arqueue[0][" + STR(BURST_HIGH_INDEX) + " : " +
                          STR(BURST_LOW_INDEX) + "] == 2'b01) begin\n");
            /* Incremental burst */
            writer->write("      " + portPrefix + "currAddr = " + portPrefix + "arqueue[0][" + STR(ADDR_HIGH_INDEX) +
                          " : " + STR(ADDR_LOW_INDEX) + "] + " + portPrefix + "arqueue[0][" + STR(COUNT_HIGH_INDEX) +
                          " : " + STR(COUNT_LOW_INDEX) + "] * (1 << " + portPrefix + "arqueue[0][" +
                          STR(SIZE_HIGH_INDEX) + " : " + STR(SIZE_LOW_INDEX) + "]);\n");
            writer->write("    end else if(" + portPrefix + "arqueue[0][" + STR(BURST_HIGH_INDEX) + " : " +
                          STR(BURST_LOW_INDEX) + "] == 2'b10) begin\n");
            /* Wrap burst */
            writer->write("      " + portPrefix + "endAddr = " + portPrefix + "arqueue[0][" + STR(ADDR_HIGH_INDEX) +
                          " : " + STR(ADDR_LOW_INDEX) + "] - (" + portPrefix + "arqueue[0][" + STR(ADDR_HIGH_INDEX) +
                          " : " + STR(ADDR_LOW_INDEX) + "] % ((" + portPrefix + "arqueue[0][" + STR(LEN_HIGH_INDEX) +
                          " : " + STR(LEN_LOW_INDEX) + "] + 1) * (1 << " + portPrefix + "arqueue[0][" +
                          STR(SIZE_HIGH_INDEX) + " : " + STR(SIZE_LOW_INDEX) + "]))) + ((" + portPrefix +
                          "arqueue[0][" + STR(LEN_HIGH_INDEX) + " : " + STR(LEN_LOW_INDEX) + "] + 1) * (1 << " +
                          portPrefix + "arqueue[0][" + STR(SIZE_HIGH_INDEX) + " : " + STR(SIZE_LOW_INDEX) + "]));\n");
            writer->write("      " + portPrefix + "currAddr = " + portPrefix + "arqueue[0][" + STR(ADDR_HIGH_INDEX) +
                          " : " + STR(ADDR_LOW_INDEX) + "] + " + portPrefix + "arqueue[0][" + STR(COUNT_HIGH_INDEX) +
                          " : " + STR(COUNT_LOW_INDEX) + "] * (1 << " + portPrefix + "arqueue[0][" +
                          STR(SIZE_HIGH_INDEX) + " : " + STR(SIZE_LOW_INDEX) + "]);\n");
            writer->write("      if(" + portPrefix + "currAddr > " + portPrefix + "endAddr) begin\n");
            writer->write("        " + portPrefix + "currAddr = " + portPrefix + "currAddr - ((" + portPrefix +
                          "arqueue[0][" + STR(LEN_HIGH_INDEX) + " : " + STR(LEN_LOW_INDEX) + "] + 1) * (1 << " +
                          portPrefix + "arqueue[0][" + STR(SIZE_HIGH_INDEX) + " : " + STR(SIZE_LOW_INDEX) + "]));\n");
            writer->write("      end\n");
            writer->write("    end\n");

            /* Realign address */
            writer->write("    " + portPrefix + "currAddr = " + portPrefix + "currAddr - (" + portPrefix +
                          "currAddr % (1 << " + portPrefix + "arqueue[0][" + STR(SIZE_HIGH_INDEX) + " : " +
                          STR(SIZE_LOW_INDEX) + "]));\n");

            /* Compute aggregate memory for RDATA */
            std::string mem_aggregated;
            const auto portRDATA = mod->find_member(portPrefix + "RDATA", port_o_K, cir);
            const auto bitsizeRDATA = GetPointer<port_o>(portRDATA)->get_typeRef()->size *
                                      GetPointer<port_o>(portRDATA)->get_typeRef()->vector_size;
            {
               mem_aggregated = "{";
               for(unsigned int bitsize_index = 0; bitsize_index < bitsizeRDATA; bitsize_index = bitsize_index + 8)
               {
                  if(bitsize_index)
                  {
                     mem_aggregated += ", ";
                  }
                  mem_aggregated += "_bambu_testbench_mem_[" + portPrefix + "currAddr + " +
                                    STR((bitsizeRDATA - bitsize_index) / 8 - 1) + " - base_addr]";
               }
               mem_aggregated += "}";
            }

            writer->write("    " + portPrefix + "RDATA <= " + mem_aggregated + ";\n");
            writer->write("    " + portPrefix + "RRESP <= 2'b00;\n");
            writer->write("    if(" + portPrefix + "arqueue[0][" + STR(COUNT_HIGH_INDEX) + " : " +
                          STR(COUNT_LOW_INDEX) + "] == " + portPrefix + "arqueue[0][" + STR(LEN_HIGH_INDEX) + " : " +
                          STR(LEN_LOW_INDEX) + "]) begin\n");
            writer->write("      " + portPrefix + "RLAST <= 1'b1;\n");
            writer->write("      if(" + portPrefix + "RREADY) begin\n");
            writer->write("        for(_i_ = 1; _i_ < " + portPrefix + "arqueue_size; _i_ = _i_ + 1) begin\n");
            writer->write("          " + portPrefix + "arqueue[_i_ - 1] = " + portPrefix + "arqueue[_i_];\n");
            writer->write("        end\n");
            /* As for the write queue, it's also possible that we are adding and removing a transaction at the same
             * time
             */
            writer->write("        if(" + portPrefix + "ARVALID && " + portPrefix + "ARREADY) begin\n");
            writer->write("          " + portPrefix + "arqueue[" + portPrefix + "arqueue_size - 1] = " + portPrefix +
                          "arqueue[" + portPrefix + "arqueue_size];\n");
            writer->write("          " + portPrefix + "arqueue_size <= " + portPrefix + "arqueue_size;\n");
            writer->write("        end else begin\n");
            writer->write("          " + portPrefix + "arqueue_size <= " + portPrefix + "arqueue_size - 1;\n");
            writer->write("        end\n");
            writer->write("      end\n");
            writer->write("    end else if(" + portPrefix + "RREADY) begin\n");
            writer->write("      " + portPrefix + "arqueue[0][" + STR(COUNT_HIGH_INDEX) + " : " + STR(COUNT_LOW_INDEX) +
                          "] <= " + portPrefix + "arqueue[0][" + STR(COUNT_HIGH_INDEX) + " : " + STR(COUNT_LOW_INDEX) +
                          "] + 1;\n");
            writer->write("    end\n");
            writer->write("  end\n");
            writer->write("end\n");
         }
      }
   }
}

void TestbenchGenerationBaseStep::memory_initialization() const
{
   writer->write("for (addr = 0; addr < MEMSIZE; addr = addr + 1)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("_bambu_testbench_mem_[addr] = 8'b0;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
}

void TestbenchGenerationBaseStep::write_max_simulation_time_control() const
{
   writer->write("always @(posedge " CLOCK_PORT_NAME ")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("currTime = $time;\n");
   writer->write("if ($time >= startTime && (($time - startTime)/`CLOCK_PERIOD > "
                 "`SIMULATION_LENGTH))\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("$display(\"Simulation not completed into %d cycles\", `SIMULATION_LENGTH);\n");
   writer->write("$fwrite(res_file, \"X\\t\");\n");
   writer->write("$fwrite(res_file, \"%d\\n\", `SIMULATION_LENGTH);\n");
   writer->write("$fclose(res_file);\n");
   writer->write("$fclose(file);\n");
   writer->write("$finish;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n\n");
}

void TestbenchGenerationBaseStep::reading_base_memory_address_from_file() const
{
   writer->write_comment(
       "reading base address memory --------------------------------------------------------------\n");
   writer->write("_ch_ = $fgetc(file);\n");
   writer->write("if ($feof(file))\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   {
      writer->write("$display(\"No more values found. Simulation(s) executed: %d.\\n\", _n_);\n");
      writer->write("$fclose(res_file);\n");
      writer->write("$fclose(file);\n");
      writer->write("$finish;\n");
   }
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\" || _ch_ == \"b\")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   {
      writer->write("if (_ch_ == \"b\")\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      writer->write(R"(_r_ = $fscanf(file,"%b\n", base_addr); )");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write("else\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      writer->write("_r_ = $fgets(line, file);\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write("_ch_ = $fgetc(file);\n");
   }
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
}

void TestbenchGenerationBaseStep::memory_initialization_from_file() const
{
   writer->write_comment("initializing memory --------------------------------------------------------------\n");
   writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\" || _ch_ == \"m\")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   {
      writer->write("if (_ch_ == \"m\")\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      {
         writer->write("_r_ = $fscanf(file,\"%b\\n\", _bambu_databyte_);\n");
         writer->write("_bambu_testbench_mem_[_addr_i_] = _bambu_databyte_;\n");
         writer->write("_addr_i_ = _addr_i_ + 1;\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write("else\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      writer->write("_r_ = $fgets(line, file);\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write("_ch_ = $fgetc(file);\n");
   }
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
}

void TestbenchGenerationBaseStep::begin_file_reading_operation() const
{
   writer->write("\n");

   writer->write_comment("Assigning values\n");
   writer->write("always @ (posedge " + STR(CLOCK_PORT_NAME) + ")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("if (next_" + STR(START_PORT_NAME) + " == 1'b1)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
}

void TestbenchGenerationBaseStep::end_file_reading_operation() const
{
   writer->write_comment("Simulation start\n");
   writer->write("startTime = $time;\n$display(\"Reading of vector values from input file completed. Simulation "
                 "started.\");\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n\n");
}

void TestbenchGenerationBaseStep::write_sim_time_calc() const
{
   writer->write_comment("Check done_port signal\n");
   writer->write("always @(negedge " CLOCK_PORT_NAME ")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("if (done_port == 1)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");

   writer->write("endTime = $time;\n\n");
   writer->write_comment("Simulation time (clock cycles) = 1+(time elapsed (seconds) / clock cycle (seconds per "
                         "cycle)) (until done is 1)\n");
   writer->write("sim_time = $rtoi((endTime + `HALF_CLOCK_PERIOD - startTime)/`CLOCK_PERIOD);\n\n");
   writer->write("success = 1;\n");
   writer->write("compare_outputs = 0;\n");

   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
}

void TestbenchGenerationBaseStep::read_input_value_from_file(const std::string& input_name,
                                                             bool& first_valid_input) const
{
   if(input_name != CLOCK_PORT_NAME && input_name != RESET_PORT_NAME && input_name != START_PORT_NAME)
   {
      writer->write("\n");
      writer->write_comment("Read a value for " + input_name +
                            " --------------------------------------------------------------\n");
      if(!first_valid_input)
      {
         writer->write("_ch_ = $fgetc(file);\n");
      }

      writer->write("while (_ch_ == \"/\" || _ch_ == \"\\n\")\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      {
         writer->write("_r_ = $fgets(line, file);\n");
         writer->write("_ch_ = $fgetc(file);\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");

      if(first_valid_input)
      {
         /// write statement for new vectors' check
         writer->write_comment("If no character found\n");
         writer->write("if (_ch_ == -1)\n");
         writer->write(STR(STD_OPENING_CHAR));
         writer->write("begin\n");
         {
            writer->write("$display(\"No more values found. Simulation(s) executed: %d.\\n\", _n_);\n");
            writer->write("$fclose(res_file);\n");
            writer->write("$fclose(file);\n");
            writer->write("$finish;\n");
         }
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
         writer->write("else\n");
         writer->write(STR(STD_OPENING_CHAR));
         writer->write("begin\n");
         {
            writer->write_comment("Vectors count\n");
            writer->write("_n_ = _n_ + 1;\n");
            writer->write("$display(\"Start reading vector %d's values from input file.\\n\", _n_);\n");
         }
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
         first_valid_input = false;
      }

      writer->write("if (_ch_ == \"p\")\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      {
         writer->write(R"(_r_ = $fscanf(file,"%b\n", )" + input_name + "); ");
         writer->write_comment("expected format: bbb...b (example: 00101110)\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");

      writer->write("if (_r_ != 1) ");
      writer->write_comment("error\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      {
         writer->write("_ch_ = $fgetc(file);\n");
         writer->write("if (_ch_ == `EOF) ");
         writer->write_comment("end-of-file reached\n");
         writer->write(STR(STD_OPENING_CHAR));
         writer->write("begin\n");
         {
            writer->write(
                "$display(\"ERROR - End of file reached before getting all the values for the parameters\");\n");
            writer->write("$fclose(res_file);\n");
            writer->write("$fclose(file);\n");
            writer->write("$finish;\n");
         }
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
         writer->write("else ");
         writer->write_comment("generic error\n");
         writer->write(STR(STD_OPENING_CHAR));
         writer->write("begin\n");
         {
            writer->write(
                "$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
            writer->write("$fclose(res_file);\n");
            writer->write("$fclose(file);\n");
            writer->write("$finish;\n");
         }
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write("else\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      {
         std::string nonescaped_name = input_name;
         nonescaped_name.erase(std::remove(nonescaped_name.begin(), nonescaped_name.end(), '\\'),
                               nonescaped_name.end());
         if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
         {
            writer->write("$display(\"Value found for input " + nonescaped_name + ": %b\", " + input_name + ");\n");
         }
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write_comment("Value for " + input_name +
                            " found ---------------------------------------------------------------\n");
   }
}

void TestbenchGenerationBaseStep::write_compute_ulps_functions() const
{
   writer->write("\n");
   writer->write("function real bits32_to_real64;\n");
   writer->write("  input [31:0] in1;\n");
   writer->write("  reg [7:0] exponent1;\n");
   writer->write("  reg is_exp_zero;\n");
   writer->write("  reg is_all_ones;\n");
   writer->write("  reg [10:0] exp_tmp;\n");
   writer->write("  reg [63:0] out1;\n");
   writer->write("begin\n");
   writer->write("  exponent1 = in1[30:23];\n");
   writer->write("  is_exp_zero = exponent1 == 8'd0;\n");
   writer->write("  is_all_ones = exponent1 == {8{1'b1}};\n");
   writer->write("  exp_tmp = {3'd0, exponent1} + 11'd896;\n");
   writer->write("  out1[63] = in1[31];\n");
   writer->write("  out1[62:52] = is_exp_zero ? 11'd0 : (is_all_ones ? {11{1'b1}} : exp_tmp);\n");
   writer->write("  out1[51:29] = in1[22:0];\n");
   writer->write("  out1[28:0] = 29'd0;\n");
   writer->write("  bits32_to_real64 = $bitstoreal(out1);\n");
   writer->write("end\n");
   writer->write("endfunction\n");

   writer->write("function real compute_ulp32;\n");
   writer->write("  input [31:0] computed;\n");
   writer->write("  input [31:0] expected;\n");
   writer->write("  real computedR;\n");
   writer->write("  real expectedR;\n");
   writer->write("  real diffR;\n");
   writer->write("  reg [31:0] denom;\n");
   writer->write("  real denomR;\n");
   writer->write("begin\n");
   writer->write("  if (expected[30:23] == {8{1'b1}} ||computed[30:23] == {8{1'b1}})\n");
   writer->write("    compute_ulp32 = computed != expected && (computed[22:0] == 23'd0 || expected[22:0] == 23'd0) ? "
                 "{1'b0,({8{1'b1}}-8'd1),{23'b1} } : 32'd0;\n");
   writer->write("  else\n");
   writer->write("  begin\n");
   writer->write("    denom = 32'd0;\n");
   writer->write("    if (expected[30:0] == 31'd0)\n");
   writer->write("      denom[30:23] = 8'd104;\n");
   writer->write("    else\n");
   writer->write("      denom[30:23] = expected[30:23]-8'd23;\n");
   writer->write("    computedR = bits32_to_real64({1'b0, computed[30:0]});\n");
   writer->write("    expectedR = bits32_to_real64({1'b0, expected[30:0]});\n");
   writer->write("    denomR = bits32_to_real64(denom);\n");
   writer->write("    diffR = computedR - expectedR;\n");
   writer->write("    if(diffR < 0.0)\n");
   writer->write("      diffR = - diffR;\n");
   writer->write("    if (expected[30:0] == 31'd0 && computed[30:0] == 31'd0  && expected[31] != computed[31] )\n");
   writer->write("      compute_ulp32 = 1.0;\n");
   writer->write("    else\n");
   writer->write("      compute_ulp32 = diffR / denomR;\n");
   writer->write("  end\n");
   writer->write("end\n");
   writer->write("endfunction\n");
   writer->write("\n");
   writer->write("function real compute_ulp64;\n");
   writer->write("  input [63:0] computed;\n");
   writer->write("  input [63:0] expected;\n");
   writer->write("  real computedR;\n");
   writer->write("  real expectedR;\n");
   writer->write("  real diffR;\n");
   writer->write("  reg [63:0] denom;\n");
   writer->write("  real denomR;\n");
   writer->write("begin\n");
   writer->write("  if (expected[62:52] == {11{1'b1}} ||computed[62:52] == {11{1'b1}})\n");
   writer->write("    compute_ulp64 = computed != expected && (computed[51:0] == 52'd0 || expected[51:0] == 52'd0) ? "
                 "{1'b0,({11{1'b1}}-11'd1),{52'b1} } : 64'd0;\n");
   writer->write("  else\n");
   writer->write("  begin\n");
   writer->write("    denom = 64'd0;\n");
   writer->write("    if (expected[62:0] == 63'd0)\n");
   writer->write("      denom[62:52] = 11'd971;\n");
   writer->write("    else\n");
   writer->write("      denom[62:52] = expected[62:52]-11'd52;\n");
   writer->write("    computedR = $bitstoreal({1'b0, computed[62:0]});\n");
   writer->write("    expectedR = $bitstoreal({1'b0, expected[62:0]});\n");
   writer->write("    denomR = $bitstoreal(denom);\n");
   writer->write("    diffR = computedR - expectedR;\n");
   writer->write("    if(diffR < 0.0)\n");
   writer->write("      diffR = - diffR;\n");
   writer->write("    if (expected[62:0] == 63'd0 && computed[62:0] == 63'd0  && expected[63] != computed[63] )\n");
   writer->write("      compute_ulp64 = 1.0;\n");
   writer->write("    else\n");
   writer->write("      compute_ulp64 = diffR / denomR;\n");
   writer->write("  end\n");
   writer->write("end\n");
   writer->write("endfunction\n");
}
