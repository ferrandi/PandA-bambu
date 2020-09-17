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
 * @file wishbone_interface_tesbench.cpp
 * @brief Class to compute testbenches for high-level synthesis
 *
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

/// Header include
#include "wishbone_interface_testbench.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// circuit include
#include "structural_objects.hpp"

/// design_flows/backend/ToHDL include
#include "HDL_manager.hpp"
#include "language_writer.hpp"

/// HLS include
#include "hls_manager.hpp"

/// HLS/memory includes
#include "memory.hpp"
#include "memory_symbol.hpp"

#if HAVE_FROM_DISCREPANCY_BUILT
// include from HLS/vcd
#include "Discrepancy.hpp"
#endif

/// STD include
#include <string>

/// STL includes
#include "custom_map.hpp"
#include <vector>

/// technology/physical_library include
#include "technology_wishbone.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "string_manipulation.hpp" // for STR
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

WishboneInterfaceTestbench::WishboneInterfaceTestbench(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : TestbenchGenerationBaseStep(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::WB4_TESTBENCH_GENERATION)
{
}

WishboneInterfaceTestbench::~WishboneInterfaceTestbench() = default;

void WishboneInterfaceTestbench::write_wishbone_input_signal_declaration(const tree_managerConstRef TreeM) const
{
   /// write input signals declaration
   if(mod->get_in_port_size())
   {
      writer->write_comment("INPUT SIGNALS\n");
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         const structural_objectRef& port_obj = mod->get_in_port(i);
         if(CLOCK_PORT_NAME == port_obj->get_id())
            writer->write("input ");
         else if(GetPointer<port_o>(port_obj)->get_is_memory() || WB_ACKIM_PORT_NAME == port_obj->get_id())
            writer->write("wire ");
         else
            writer->write("reg ");

         writer->write(writer->type_converter(port_obj->get_typeRef()) + writer->type_converter_size(port_obj));
         if(port_obj->get_kind() != port_o_K && port_obj->get_typeRef()->type != structural_type_descriptor::VECTOR_BOOL)
         {
            unsigned int lsb = GetPointer<port_o>(mod->get_in_port(i))->get_lsb();
            writer->write("[" + STR(GetPointer<port_o>(mod->get_in_port(i))->get_ports_size() - 1 + lsb) + ":" + STR(lsb) + "] ");
         }
         writer->write(HDL_manager::convert_to_identifier(writer.get(), mod->get_in_port(i)->get_id()) + ";\n");
         if(port_obj->get_typeRef()->treenode > 0 && tree_helper::is_a_pointer(TreeM, port_obj->get_typeRef()->treenode))
         {
            unsigned int pt_type_index = tree_helper::get_pointed_type(TreeM, tree_helper::get_type_index(TreeM, port_obj->get_typeRef()->treenode));
            tree_nodeRef pt_node = TreeM->get_tree_node_const(pt_type_index);
            if(GetPointer<array_type>(pt_node))
            {
               while(GetPointer<array_type>(pt_node))
               {
                  pt_type_index = GET_INDEX_NODE(GetPointer<array_type>(pt_node)->elts);
                  pt_node = GET_NODE(GetPointer<array_type>(pt_node)->elts);
               }
            }
            long long int bitsize = tree_helper::size(TreeM, pt_type_index);
            writer->write("reg [" + STR(bitsize - 1) + ":0] ex_" + port_obj->get_id() + ";\n");
         }
      }
      writer->write("\n");
   }
}

void WishboneInterfaceTestbench::write_wishbone_callFSM_signal_declaration() const
{
   writer->write_comment("State machine present and next state\n");
   writer->write("reg [7:0] call_state;\n");
   writer->write("reg [7:0] next_call_state;\n");

   writer->write("reg cyc_fsm;\n");
   writer->write("reg stb_fsm;\n");
   writer->write("reg we_fsm;\n");
   writer->write("reg done_port;\n");
   writer->write("reg start_next_sim;\n");
}

void WishboneInterfaceTestbench::write_call(bool hasMultiIrq) const
{
   const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_functions.size() == 1, "");
   const unsigned int topFunctionId = *(top_functions.begin());
   const BehavioralHelperConstRef behavioral_helper = HLSMgr->CGetFunctionBehavior(topFunctionId)->CGetBehavioralHelper();
   const memoryRef mem = HLSMgr->Rmem;
   const std::map<unsigned int, memory_symbolRef>& function_parameters = mem->get_function_parameters(topFunctionId);
   std::vector<std::string> parameterNames;
   for(auto const& function_parameter : function_parameters)
   {
      unsigned int var = function_parameter.first;
      std::string variableName = (var == behavioral_helper->GetFunctionReturnType(topFunctionId)) ? RETURN_PORT_NAME : behavioral_helper->PrintVariable(var);
      parameterNames.push_back(variableName);
   }
   bool has_return = behavioral_helper->GetFunctionReturnType(topFunctionId);
   // sequential logic
   writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   if(!parameters->getOption<bool>(OPT_level_reset))
      writer->write("if (" + std::string(RESET_PORT_NAME) + " == 1'b0)\n");
   else
      writer->write("if (" + std::string(RESET_PORT_NAME) + " == 1'b1)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("call_state <= 8'd0;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write("else\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("call_state <= next_call_state;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");

   // State transition table
   writer->write("always @(call_state or start_port or ack_os or irq)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("next_call_state = 8'd0;\n");
   writer->write("case (call_state)\n");

   writer->write("8'd0:\n");
   writer->write("if (start_port == 1'b1)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("next_call_state = 8'd1;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write("else\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("next_call_state = 8'd0;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");

   unsigned int state = 0;
   for(auto itr = parameterNames.rbegin(), end = parameterNames.rend(); itr != end; ++itr)
   {
      if(*itr != RETURN_PORT_NAME)
      {
         writer->write_comment("Send parameter " + *itr + "\n");
         writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
         writer->write("if (ack_os == 1'b1)\n");
         writer->write(STR(STD_OPENING_CHAR));
         writer->write("begin\n");
         writer->write("next_call_state = 8'd" + boost::lexical_cast<std::string>(state + 1) + ";\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
         writer->write("else\n");
         writer->write(STR(STD_OPENING_CHAR));
         writer->write("begin\n");
         writer->write("next_call_state = 8'd" + boost::lexical_cast<std::string>(state) + ";\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
         if(itr != end - 1)
         {
            writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
            writer->write("next_call_state = 8'd" + boost::lexical_cast<std::string>(state + 1) + ";\n");
         }
      }
   }
   writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
   if(hasMultiIrq)
      writer->write("if (irq[0] == 1'b1)\n");
   else
      writer->write("if (irq == 1'b1)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("next_call_state = 8'd" + boost::lexical_cast<std::string>(state + 1) + ";\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write("else\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("next_call_state = 8'd" + boost::lexical_cast<std::string>(state) + ";\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");

   if(has_return)
   {
      writer->write_comment("Retrieve " + std::string(RETURN_PORT_NAME) + "\n");
      writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
      writer->write("if (ack_os == 1'b1)\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      writer->write("next_call_state = 8'd" + boost::lexical_cast<std::string>(state + 1) + ";\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write("else\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      writer->write("next_call_state = 8'd" + boost::lexical_cast<std::string>(state) + ";\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
   }

   writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("next_call_state = 8'd0;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");

   writer->write("default: next_call_state = 8'd0;\n");
   writer->write("endcase\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");

   // Output function
   writer->write("always @(*)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("case (call_state)\n");

   writer->write("8'd0:\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("cyc_fsm = 1'b0;\n");
   writer->write("stb_fsm = 1'b0;\n");
   writer->write("we_fsm = 1'b0;\n");
   writer->write("done_port = 1'b0;\n");
   writer->write("sel_is = 8'b11111111;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");

   state = 0;
   for(auto itr = parameterNames.rbegin(), end = parameterNames.rend(); itr != end; ++itr)
   {
      if(*itr != RETURN_PORT_NAME)
      {
         writer->write_comment("Send parameter " + *itr + "\n");
         writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
         writer->write(STR(STD_OPENING_CHAR));
         writer->write("begin\n");
         writer->write("addr_is = ADDRESS_OFFSET_" + *itr + ";\n");
         writer->write("dat_is = " + HDL_manager::convert_to_identifier(writer.get(), *itr) + ";\n");
         writer->write("cyc_fsm = 1'b1;\n");
         writer->write("stb_fsm = 1'b1;\n");
         writer->write("we_fsm = 1'b1;\n");
         writer->write("sel_is = 8'b11111111;\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
         writer->write_comment("Send parameter " + *itr + "\n");
         if(itr != end - 1)
         {
            writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
            writer->write(STR(STD_OPENING_CHAR));
            writer->write("begin\n");
            writer->write("addr_is = ADDRESS_OFFSET_" + *itr + ";\n");
            writer->write("dat_is = " + HDL_manager::convert_to_identifier(writer.get(), *itr) + ";\n");
            writer->write("cyc_fsm = 1'b1;\n");
            writer->write("stb_fsm = 1'b0;\n");
            writer->write("we_fsm = 1'b1;\n");
            writer->write("sel_is = 8'b11111111;\n");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
         }
      }
   }
   writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("cyc_fsm = 1'b0;\n");
   writer->write("stb_fsm = 1'b0;\n");
   writer->write("we_fsm = 1'b0;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");

   if(has_return)
   {
      writer->write_comment("Retrieve " + std::string(RETURN_PORT_NAME) + "\n");
      writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      writer->write("addr_is = ADDRESS_OFFSET_" + std::string(RETURN_PORT_NAME) + ";\n");
      writer->write("cyc_fsm = 1'b1;\n");
      writer->write("stb_fsm = 1'b1;\n");
      writer->write("we_fsm = 1'b0;\n");
      writer->write("sel_is = 8'b11111111;\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
   }

   writer->write("8'd" + boost::lexical_cast<std::string>(++state) + ":\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("cyc_fsm = 1'b0;\n");
   writer->write("stb_fsm = 1'b0;\n");
   writer->write("we_fsm = 1'b0;\n");
   writer->write("done_port = 1'b1;\n");
   if(has_return)
   {
      writer->write("registered_" + std::string(RETURN_PORT_NAME) + " = dat_os;\n");
   }
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");

   writer->write("default:\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("endcase\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n\n");

   writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("we_is <= we_fsm & !ack_os;\n");
   writer->write("cyc_is <= cyc_fsm & !ack_os;\n");
   writer->write("stb_is <= stb_fsm & !ack_os;\n\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
}

void WishboneInterfaceTestbench::write_memory_handler() const
{
   structural_objectRef dat_om = mod->find_member(WB_DATOM_PORT_NAME, port_o_K, cir);
   unsigned int data_bus_bitsize = GET_TYPE_SIZE(dat_om);
   unsigned int dataBusByteSize = data_bus_bitsize >> 3;

   writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   if(!parameters->getOption<bool>(OPT_level_reset))
      writer->write("if (" + std::string(RESET_PORT_NAME) + " == 1'b0)" + STR(STD_OPENING_CHAR) + "\n");
   else
      writer->write("if (" + std::string(RESET_PORT_NAME) + " == 1'b1)" + STR(STD_OPENING_CHAR) + "\n");
   writer->write("ack_delayed <= 1'b0;" + STR(STD_CLOSING_CHAR) + "\n");
   writer->write("else" + STR(STD_OPENING_CHAR) + "\n");
   writer->write("ack_delayed <= stb_om & cyc_om & !ack_im;");
   writer->write(STR(STD_CLOSING_CHAR) + "\n");
   writer->write("end\n");

   writer->write("assign ack_im = ack_delayed;\n");

   writer->write_comment("memory read/write controller\n");
   writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("if (cyc_om & stb_om)\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("if (we_om && base_addr <= addr_om && addr_om <= (base_addr + MEMSIZE - " + STR(dataBusByteSize) + "))\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("case (sel_om)\n");
   unsigned int caseValue = 0;
   for(unsigned int j = 0; j < dataBusByteSize; ++j)
   {
      caseValue = (caseValue | (1U << j));
      writer->write(boost::lexical_cast<std::string>(dataBusByteSize) + "'d" + boost::lexical_cast<std::string>(caseValue) + ":\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      for(unsigned int i = 0; i <= j; ++i)
      {
         writer->write("_bambu_testbench_mem_[(addr_om - base_addr) + " + STR(i) + "] <= dat_om[" + STR((i * 8) + 7) + ":" + STR((i * 8)) + "];\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
   }
   writer->write("default:\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("$display(\"ERROR - Unsupported sel value.\");\n"
                 "$fclose(res_file);\n$fclose(file);\n$finish;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("endcase\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write("if (!we_om && base_addr <= addr_om && addr_om <= (base_addr + MEMSIZE - " + STR(dataBusByteSize) + "))\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("case (sel_om)\n");
   caseValue = 0;
   for(unsigned int j = 0; j < dataBusByteSize; ++j)
   {
      caseValue = (caseValue | (1U << j));
      writer->write(boost::lexical_cast<std::string>(dataBusByteSize) + "'d" + boost::lexical_cast<std::string>(caseValue) + ":\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write("begin\n");
      for(unsigned int i = 0; i <= j; ++i)
      {
         writer->write("dat_im[" + STR((i * 8) + 7) + ":" + STR((i * 8)) + "] <= _bambu_testbench_mem_[(addr_om - base_addr) + " + STR(i) + "];\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
   }
   writer->write("default:\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("$display(\"ERROR - Unsupported sel value.\");\n"
                 "$fclose(res_file);\n$fclose(file);\n$finish;\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("endcase\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n\n");
}

void WishboneInterfaceTestbench::write_wishbone_output_signal_declaration(bool& withMemory, bool& hasMultiIrq) const
{
   hasMultiIrq = false;
   withMemory = false;
   /// write output signals declaration
   if(mod->get_out_port_size())
   {
      writer->write_comment("OUTPUT SIGNALS\n");
      for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         std::string portId = mod->get_out_port(i)->get_id();
         writer->write("wire " + writer->type_converter(mod->get_out_port(i)->get_typeRef()) + writer->type_converter_size(mod->get_out_port(i)));
         if(portId == "irq" && mod->get_out_port(i)->get_kind() == port_vector_o_K)
            hasMultiIrq |= GetPointer<port_o>(mod->get_out_port(i))->get_ports_size() > 1;

         if(mod->get_out_port(i)->get_kind() != port_o_K && mod->get_out_port(i)->get_typeRef()->type != structural_type_descriptor::VECTOR_BOOL)
         {
            unsigned int lsb = GetPointer<port_o>(mod->get_out_port(i))->get_lsb();
            writer->write("[" + STR(GetPointer<port_o>(mod->get_out_port(i))->get_ports_size() - 1 + lsb) + ":" + STR(lsb) + "] ");
         }
         withMemory |= portId == WB_STBOM_PORT_NAME || portId == WB_CYCOM_PORT_NAME;
         writer->write(HDL_manager::convert_to_identifier(writer.get(), portId) + ";\n");
      }
      writer->write("\n");
   }
}

void WishboneInterfaceTestbench::write_signals(const tree_managerConstRef TreeM, bool& withMemory, bool& hasMultiIrq) const
{
   write_wishbone_callFSM_signal_declaration();
   writer->write("reg " + std::string(START_PORT_NAME) + ";\n");
   writer->write("reg ack_delayed;\n");
   write_wishbone_input_signal_declaration(TreeM);
   write_wishbone_output_signal_declaration(withMemory, hasMultiIrq);

   // parameter
   writer->write_comment("Function parameters\n");

   const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_functions.size() == 1, "");
   const unsigned int topFunctionId = *(top_functions.begin());
   const BehavioralHelperConstRef behavioral_helper = HLSMgr->CGetFunctionBehavior(topFunctionId)->CGetBehavioralHelper();
   const memoryRef mem = HLSMgr->Rmem;
   const std::map<unsigned int, memory_symbolRef>& function_parameters = mem->get_function_parameters(topFunctionId);
   for(auto const& function_parameter : function_parameters)
   {
      unsigned int var = function_parameter.first;
      std::string variableName = (var == behavioral_helper->GetFunctionReturnType(topFunctionId)) ? RETURN_PORT_NAME : behavioral_helper->PrintVariable(var);
      unsigned int variableType = tree_helper::get_type_index(TreeM, var);
      unsigned int variableBitSize = tree_helper::size(TreeM, variableType);
      unsigned int expectedVariableBitSize = tree_helper::size(TreeM, variableType);
      if(tree_helper::is_a_pointer(TreeM, var))
      {
         unsigned int pt_type_index = tree_helper::get_pointed_type(TreeM, tree_helper::get_type_index(TreeM, var));
         tree_nodeRef pt_node = TreeM->get_tree_node_const(pt_type_index);
         if(GetPointer<array_type>(pt_node))
         {
            while(GetPointer<array_type>(pt_node))
            {
               pt_type_index = GET_INDEX_NODE(GetPointer<array_type>(pt_node)->elts);
               pt_node = GET_NODE(GetPointer<array_type>(pt_node)->elts);
            }
         }
         expectedVariableBitSize = tree_helper::size(TreeM, pt_type_index);
      }
      if(var == behavioral_helper->GetFunctionReturnType(topFunctionId))
      {
         writer->write_comment(variableName + " -> " + STR(function_parameter.first) + "\n");
         writer->write("reg [" + STR(expectedVariableBitSize) + "-1:0] ex_" + variableName + ";\n");
         writer->write("reg [" + STR(variableBitSize) + "-1:0] registered_" + variableName + ";\n");
         writer->write("parameter ADDRESS_OFFSET_" + variableName + " = " + STR(mem->get_parameter_base_address(topFunctionId, var)) + ";\n");
      }
      else
      {
         writer->write_comment(variableName + " -> " + STR(var) + "\n");
         writer->write("reg [" + STR(variableBitSize) + "-1:0] " + HDL_manager::convert_to_identifier(writer.get(), variableName) + ";\n");
         writer->write("parameter ADDRESS_OFFSET_" + variableName + " = " + STR(mem->get_parameter_base_address(topFunctionId, var)) + ";\n");
         if(tree_helper::is_a_pointer(TreeM, var))
         {
            unsigned int pt_type_index = tree_helper::get_pointed_type(TreeM, tree_helper::get_type_index(TreeM, var));
            /// FIXME: real numbers at the moment have to be considered differently because of computation of ulp; c++ code is still managed in the old way
            if(tree_helper::is_real(TreeM, pt_type_index) or flag_cpp)
            {
               writer->write("reg [" + STR(expectedVariableBitSize - 1) + ":0] ex_" + variableName + ";\n");
            }
            else
            {
               writer->write("reg [7:0] ex_" + variableName + ";\n");
            }
         }
      }
   }
}

void WishboneInterfaceTestbench::write_slave_initializations(bool) const
{
   return;
}

void WishboneInterfaceTestbench::write_file_reading_operations() const
{
   const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_functions.size() == 1, "");
   const unsigned int topFunctionId = *(top_functions.begin());
   const BehavioralHelperConstRef behavioral_helper = HLSMgr->CGetFunctionBehavior(topFunctionId)->CGetBehavioralHelper();
   const memoryRef mem = HLSMgr->Rmem;
   const std::map<unsigned int, memory_symbolRef>& function_parameters = mem->get_function_parameters(topFunctionId);
   std::vector<std::string> parameterNames;
   for(auto const& function_parameter : function_parameters)
   {
      unsigned int var = function_parameter.first;
      std::string variableName = (var == behavioral_helper->GetFunctionReturnType(topFunctionId)) ? RETURN_PORT_NAME : behavioral_helper->PrintVariable(var);
      parameterNames.push_back(variableName);
   }
   std::string topFunctionName = behavioral_helper->PrintVariable(topFunctionId);
   bool first_valid_input = true;
   // file reading operations
   for(std::vector<std::string>::size_type i = 1; i < parameterNames.size(); ++i)
   {
      if(parameterNames[i] != topFunctionName && parameterNames[i] != RETURN_PORT_NAME)
      {
         std::string input_name = HDL_manager::convert_to_identifier(writer.get(), parameterNames[i]);
         read_input_value_from_file(input_name, first_valid_input);
      }
   }
   if(not first_valid_input)
      writer->write("_ch_ = $fgetc(file);\n");
}

void WishboneInterfaceTestbench::init_extra_signals(bool) const
{
}
