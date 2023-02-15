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
#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "design_flow_manager.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <utility>

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
         if(design_flow_manager.lock()->GetStatus(HLS_step::ComputeSignature(HLSFlowStep_Type::TEST_VECTOR_PARSER,
                                                                             HLSFlowStepSpecializationConstRef())) ==
            DesignFlowStep_Status::SUCCESS)
         {
            if(HLSMgr->RSim and HLSMgr->RSim->results_available)
            {
               ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_VALUES_XML_GENERATION,
                                          HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
            }
            else
            {
               ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_VALUES_C_GENERATION,
                                          HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
            }
         }

         ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_MEMORY_ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::VCD_SIGNAL_SELECTION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::TOP_FUNCTION));
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
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
   HLSMgr->RSim->filename_bench = (parameters->getOption<std::string>(OPT_simulator) == "VERILATOR") ?
                                      verilator_testbench() :
                                      create_HDL_testbench(false);
   return DesignFlowStep_Status::SUCCESS;
}

std::string TestbenchGenerationBaseStep::print_var_init(const tree_managerConstRef TM, unsigned int var,
                                                        const memoryRef mem)
{
   std::vector<std::string> init_els;
   const auto tn = TM->CGetTreeReindex(var);
   tree_nodeRef init_node;
   auto* vd = GetPointer<var_decl>(GET_CONST_NODE(tn));
   if(vd && vd->init)
   {
      init_node = vd->init;
   }

   if(init_node && (!GetPointer<constructor>(GET_NODE(init_node)) ||
                    GetPointer<constructor>(GET_NODE(init_node))->list_of_idx_valu.size()))
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
         std::string value;
         for(unsigned int l = 0; l < num_elements; l++)
         {
            value = "";
            for(unsigned int i = 0; i < data_bitsize; i++)
            {
               value += "0";
            }
            init_els.push_back(value);
         }
      }
      else
      {
         const auto data_bitsize = tree_helper::Size(tn);
         std::string value;
         for(unsigned int i = 0; i < data_bitsize; i++)
         {
            value += "0";
         }
         init_els.push_back(value);
      }
   }
   std::string init;
   for(unsigned int l = 0; l < init_els.size(); l++)
   {
      if(l)
      {
         init += ",";
      }
      init += init_els[l];
   }
   return init;
}

std::string TestbenchGenerationBaseStep::verilator_testbench() const
{
   if(!parameters->getOption<bool>(OPT_generate_testbench))
   {
      return "";
   }
   std::string simulation_values_path = output_directory + STR(STR_CST_testbench_generation_basename) + ".txt";

   PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "  . Generation of the Verilator testbench");

   if(!boost::filesystem::exists(simulation_values_path))
   {
      THROW_ERROR("Error in generating Verilator testbench, values file missing!");
   }

   std::string fileName = write_verilator_testbench(simulation_values_path);

   PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "  . End of the Verilator testbench");

   return fileName;
}

std::string TestbenchGenerationBaseStep::write_verilator_testbench(const std::string& input_file) const
{
   // Generate the testbench

   const tree_managerRef TM = HLSMgr->get_tree_manager();

   this->write_underlying_testbench(input_file, false, false, TM);
   std::string file_name = output_directory + hdl_testbench_basename + "_tb.v";
   writer->WriteFile(file_name);
   std::ostringstream os;
   simple_indent PP('{', '}', 3);

   // Creating output file
   std::string fileName = output_directory + hdl_testbench_basename + "_main.cpp";
   std::ofstream fileOut(fileName, std::ios::out);

   std::string top_fname = mod->get_typeRef()->id_type;
   PP(os, "#include <iostream>\n");
   PP(os, "#include <string>\n");
   PP(os, "#include <verilated.h>\n");
   PP(os, "#include \"V" + top_fname + "_tb.h\"\n");
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
   PP(os, "   V" + top_fname + "_tb *top;\n");
   PP(os, "\n");
   PP(os, "   std::string vcd_output_filename = \"" + output_directory + "test.vcd\";\n");
   PP(os, "   Verilated::commandArgs(argc, argv);\n");
   PP(os, "   Verilated::debug(0);\n");
   PP(os, "   top = new V" + top_fname + "_tb;\n");
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

   fileOut << os.str() << std::endl;
   fileOut.close();

   return fileName;
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
   bool generate_vcd_output =
       (parameters->isOption(OPT_generate_vcd) and parameters->getOption<bool>(OPT_generate_vcd)) or
       (parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy));

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
      if(dump_all_signals or not simulator_supports_dumpvars_directive or
         (static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language)) ==
          HDLWriter_Language::VHDL)
#if HAVE_FROM_DISCREPANCY_BUILT
         or not parameters->isOption(OPT_discrepancy) or not parameters->getOption<bool>(OPT_discrepancy) or
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
            std::string sigscope = sig_scope.first;
            boost::replace_all(sigscope, STR(HIERARCHY_SEPARATOR), ".");
            for(const auto& signame : sig_scope.second)
            {
               writer->write("$dumpvars(1, " + sigscope + signame + ");\n");
            }
         }
      }
#endif
   }

   /// open file with values
   std::string input_values_filename = simulation_values_path;
   open_value_file(input_values_filename);

   /// open file with results
   auto result_file = parameters->getOption<std::string>(OPT_simulation_output);
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
         else if(InterfaceType == port_o::port_interface::PI_FDIN || InterfaceType == port_o::port_interface::PI_FDOUT)
         {
            structural_objectRef port_write;
            if(boost::ends_with(port_name, "_din"))
            {
               port_write = mod->find_member(port_name.substr(0, port_name.size() - sizeof("_din") + 1U) + "_write",
                                             port_o_K, cir);
               THROW_ASSERT(port_write && GetPointer<port_o>(port_write)->get_port_interface() ==
                                              port_o::port_interface::PI_WRITE,
                            "unexpected condition");
            }
            else if(boost::ends_with(port_name, "_dout"))
            {
               port_write = mod->find_member(port_name.substr(0, port_name.size() - sizeof("_dout") + 1U) + "_read",
                                             port_o_K, cir);
               THROW_ASSERT(port_write &&
                                GetPointer<port_o>(port_write)->get_port_interface() == port_o::port_interface::PI_READ,
                            "unexpected condition");
            }
            else if(boost::ends_with(port_name, "_TDATA"))
            {
               port_write = mod->find_member(port_name.substr(0, port_name.size() - sizeof("_TDATA") + 1U) + "_TVALID",
                                             port_o_K, cir);
               THROW_ASSERT(port_write && (GetPointer<port_o>(port_write)->get_port_interface() ==
                                               port_o::port_interface::PI_M_AXIS_TVALID ||
                                           GetPointer<port_o>(port_write)->get_port_interface() ==
                                               port_o::port_interface::PI_S_AXIS_TVALID),
                            "unexpected condition");
            }
            THROW_ASSERT(port_write, "");

            writer->write("always @(negedge " CLOCK_PORT_NAME ")\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("if (" + port_write->get_id() + " == 1)\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            if(InterfaceType == port_o::port_interface::PI_FDIN)
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
                        writer->write("$display(\"" + nonescaped_name + " = _bambu_testbench_mem_[" + nonescaped_name +
                                      " + %d - base_addr] = %d  expected = %d \\n\", _i_, _bambu_testbench_mem_[(" +
                                      port_name + " - base_addr) + _i_], " + output_name + ");\n");
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
                 InterfaceType == port_o::port_interface::PI_FDOUT)
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
         else if(InterfaceType == port_o::port_interface::PI_FDIN)
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
      const std::map<unsigned int, memory_symbolRef>& function_parameters = mem->get_function_parameters(topFunctionId);
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
                        writer->write("$display(\"comparison = _bambu_testbench_mem_[" + nonescaped_name +
                                      " + %d - base_addr] = %d  expected = %d \\n\", _i_, _bambu_testbench_mem_[(" +
                                      port_name + " - base_addr) + _i_], " + output_name + ");\n");
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
   writer->write("integer res_file, file, _r_, _n_, _i_, _addr_i_;\n");
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
            InterfaceType == port_o::port_interface::PI_FDOUT || InterfaceType == port_o::port_interface::PI_FDIN)
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

   /* Check if AWADDR ports are present. If there are, declare a variable to store the last valid AWADDR for each bundle
    * and the delay vectors
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
      if(GetPointer<port_o>(port_obj)->get_port_interface() == port_o::port_interface::PI_FDOUT)
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
      else if(interfaceType == port_o::port_interface::PI_FDIN)
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
            /* It is possible that we are both removing and adding an element from the queue. In this case, the size is
             * not yet updated, so we must move the new element outside the loop and keep the same queue length in this
             * clock cycle */
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

            /* Compute aggregate memory for WDATA */
            const auto portWDATA = mod->find_member(portPrefix + "WDATA", port_o_K, cir);
            const auto bitsizeWDATA = GetPointer<port_o>(portWDATA)->get_typeRef()->size *
                                      GetPointer<port_o>(portWDATA)->get_typeRef()->vector_size;
            std::string mem_aggregated;
            {
               mem_aggregated = "{";
               for(unsigned int bitsize_index = 0; bitsize_index < bitsizeWDATA; bitsize_index = bitsize_index + 8)
               {
                  if(bitsize_index)
                  {
                     mem_aggregated += ", ";
                  }
                  mem_aggregated += "_bambu_testbench_mem_[" + portPrefix + "currAddr + " +
                                    STR((bitsizeWDATA - bitsize_index) / 8 - 1) + " - base_addr]";
               }
               mem_aggregated += "}";
            }

            for(unsigned bitsize_index = 0; bitsize_index < bitsizeWDATA; bitsize_index = bitsize_index + 8)
            {
               writer->write("    " + portPrefix + "wBitmask[" + STR(bitsize_index + 7) + " : " + STR(bitsize_index) +
                             "] = {8{" + portPrefix + "WSTRB[" + STR(bitsize_index / 8) + "]}};\n");
            }
            writer->write("    " + mem_aggregated + " <= (" + mem_aggregated + " & ~" + portPrefix + "wBitmask) | (" +
                          portPrefix + "WDATA & " + portPrefix + "wBitmask);\n");
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

            /* Compute aggregate memory for RDATA */
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
            /* As for the write queue, it's also possible that we are adding and removing a transaction at the same time
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
   writer->write(
       "startTime = $time;\n$display(\"Reading of vector values from input file completed. Simulation started.\");\n");
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
