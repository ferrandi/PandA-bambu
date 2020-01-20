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
 * @file minimal_interface_testbench.cpp
 * @brief Class to compute testbenches for high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

/// Header include
#include "minimal_interface_testbench.hpp"

///. include
#include "Parameter.hpp"

/// circuit include
#include "structural_objects.hpp"

/// design_flows/backend/ToHDL include
#include "HDL_manager.hpp"
#include "language_writer.hpp"

/// HLS include
#include "hls_manager.hpp"

/// HLS/memory includes
#include "memory_allocation.hpp"

#if HAVE_FROM_DISCREPANCY_BUILT
// include from HLS/vcd
#include "Discrepancy.hpp"
#endif

// include from HLS/simulation
#include "SimulationInformation.hpp"

/// STD include
#include <string>

/// tree include
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "math_function.hpp"

MinimalInterfaceTestbench::MinimalInterfaceTestbench(const ParameterConstRef _parameters, const HLS_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager)
    : TestbenchGenerationBaseStep(_parameters, _AppM, _design_flow_manager, HLSFlowStep_Type::MINIMAL_TESTBENCH_GENERATION)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

MinimalInterfaceTestbench::~MinimalInterfaceTestbench() = default;

void MinimalInterfaceTestbench::cond_load(long long int Mout_addr_ram_bitsize, std::string, const std::string& post_slice2, const std::string& res_string, unsigned int i, const std::string& in_else, const std::string& mem_aggregate) const
{
   writer->write("assign " + res_string + post_slice2 + " = ((base_addr <= Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) + "] && Mout_addr_ram[" +
                 boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)))" + " ? " + mem_aggregate + " : " + in_else + ";\n");
}

void MinimalInterfaceTestbench::write_call(bool) const
{
   writer->write("always @(negedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write("begin\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("if (done_port == 1)\n");
   writer->write("begin\n");
   writer->write(STR(STD_OPENING_CHAR));

   if(mod->find_member(RETURN_PORT_NAME, port_o_K, cir))
   {
      writer->write("registered_" + std::string(RETURN_PORT_NAME) + " = " + std::string(RETURN_PORT_NAME) + ";\n");
   }

   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
}

void MinimalInterfaceTestbench::write_memory_handler() const
{
   const MemoryAllocation_Policy memory_allocation_policy = parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy);
   structural_objectRef Mout_data_ram_size_port = mod->find_member("Mout_data_ram_size", port_o_K, cir);
   THROW_ASSERT(Mout_data_ram_size_port, "Mout_data_ram_size port is missing");
   structural_objectRef Mout_Wdata_ram_port = mod->find_member("Mout_Wdata_ram", port_o_K, cir);
   THROW_ASSERT(Mout_Wdata_ram_port, "Mout_Wdata_ram port is missing");
   long long int Mout_Wdata_ram_bitsize = Mout_Wdata_ram_port->get_typeRef()->size * Mout_Wdata_ram_port->get_typeRef()->vector_size;
   if(Mout_data_ram_size_port->get_kind() == port_vector_o_K)
   {
      unsigned int Mout_data_ram_size_n_ports = Mout_data_ram_size_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(Mout_data_ram_size_port)->get_ports_size() : 1;
      long long int Mout_data_ram_size_bitsize = Mout_data_ram_size_port->get_typeRef()->size * Mout_data_ram_size_port->get_typeRef()->vector_size;
      for(unsigned int i = 0; i < Mout_data_ram_size_n_ports; ++i)
      {
         std::string mask_string = "(1 << Mout_data_ram_size[" + boost::lexical_cast<std::string>((i + 1) * Mout_data_ram_size_bitsize - 1) + ":" + boost::lexical_cast<std::string>((i)*Mout_data_ram_size_bitsize) + "]) -1";
         writer->write("assign mask[" + boost::lexical_cast<std::string>((i + 1) * Mout_Wdata_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>((i)*Mout_Wdata_ram_bitsize) + "] = " + mask_string + ";\n");
      }
   }
   else
      writer->write("assign mask = (1 << Mout_data_ram_size) -1;\n");

   writer->write_comment("OffChip Memory write\n");
   writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR) + "\n");

   structural_objectRef Mout_addr_ram_port = mod->find_member("Mout_addr_ram", port_o_K, cir);
   THROW_ASSERT(Mout_addr_ram_port, "Mout_addr_ram port is missing");
   long long int Mout_addr_ram_bitsize = Mout_addr_ram_port->get_typeRef()->size * Mout_addr_ram_port->get_typeRef()->vector_size;
   unsigned int Mout_Wdata_ram_n_ports = Mout_Wdata_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(Mout_Wdata_ram_port)->get_ports_size() : 1;
   long long int bitsize = Mout_Wdata_ram_port->get_typeRef()->size * Mout_Wdata_ram_port->get_typeRef()->vector_size;
   for(unsigned int i = 0; i < Mout_Wdata_ram_n_ports; ++i)
   {
      std::string mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
      std::string post_slice;
      if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
         post_slice = "[" + boost::lexical_cast<std::string>(i) + "]";
      writer->write("if (Mout_we_ram" + post_slice + " === 1'b1 && base_addr <= Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) +
                    "] && Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE))\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      if(Mout_Wdata_ram_port->get_kind() == port_vector_o_K)
         post_slice = "[" + boost::lexical_cast<std::string>((i + 1) * bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * bitsize) + "]";
      else
         post_slice = "";
      writer->write(mem_aggregate + " = (Mout_Wdata_ram" + post_slice + " & mask" + post_slice + ") | (" + mem_aggregate + " & ~(mask" + post_slice + "));\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
   }
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n\n");

   structural_objectRef Min_Wdata_ram_port = mod->find_member("Min_Wdata_ram", port_o_K, cir);
   if(Min_Wdata_ram_port)
      writer->write("assign Min_Wdata_ram = 0;\n");

   structural_objectRef M_Rdata_ram_port = mod->find_member("M_Rdata_ram", port_o_K, cir);
   THROW_ASSERT(M_Rdata_ram_port, "M_Rdata_ram port is missing");
   bitsize = M_Rdata_ram_port->get_typeRef()->size * M_Rdata_ram_port->get_typeRef()->vector_size;
   unsigned int M_Rdata_ram_port_n_ports = M_Rdata_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(M_Rdata_ram_port)->get_ports_size() : 1;

   structural_objectRef Sout_Rdata_ram_port = mod->find_member("Sout_Rdata_ram", port_o_K, cir);
   structural_objectRef Sin_Rdata_ram_port = mod->find_member("Sin_Rdata_ram", port_o_K, cir);
   if(Sin_Rdata_ram_port)
   {
      for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "[" + boost::lexical_cast<std::string>(i) + "]";
         std::string post_slice2;
         if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            post_slice2 = "[" + boost::lexical_cast<std::string>((i + 1) * bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * bitsize) + "]";
         else
            post_slice2 = "";

         cond_load(Mout_addr_ram_bitsize, post_slice1, post_slice2, "Sin_Rdata_ram", i, STR(bitsize) + "'b0", mem_aggregate);
      }
      THROW_ASSERT(Sout_Rdata_ram_port, "Sout_Rdata_ram port is missing");
      writer->write("assign M_Rdata_ram = Sout_Rdata_ram;\n");
   }
   else if(Sout_Rdata_ram_port)
   {
      for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "[" + boost::lexical_cast<std::string>(i) + "]";
         std::string post_slice2;
         if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            post_slice2 = "[" + boost::lexical_cast<std::string>((i + 1) * bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * bitsize) + "]";
         else
            post_slice2 = "";
         if(memory_allocation_policy == MemoryAllocation_Policy::ALL_BRAM or memory_allocation_policy == MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
         {
            cond_load(Mout_addr_ram_bitsize, post_slice1, post_slice2, "M_Rdata_ram_delayed_temporary", i, STR(bitsize) + "'b0", mem_aggregate);
            writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
            writer->write("begin");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("for (_i_=0; _i_<`MEM_DELAY_READ-1; _i_=_i_+1)");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("if(_i_ == `MEM_DELAY_READ-2)");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("M_Rdata_ram_delayed[_i_]" + post_slice2 + " <= M_Rdata_ram_delayed_temporary" + post_slice2 + ";");
            writer->write(STR(STD_CLOSING_CHAR) + "\n");
            writer->write("else");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("M_Rdata_ram_delayed[_i_]" + post_slice2 + " <= M_Rdata_ram_delayed[_i_+1]" + post_slice2 + ";");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write(STR(STD_CLOSING_CHAR) + "\n");
            writer->write(STR(STD_CLOSING_CHAR) + "\n");
            writer->write("end\n");
            writer->write("assign M_Rdata_ram" + post_slice2 + " = M_Rdata_ram_delayed[0]" + post_slice2 + "|" + "Sout_Rdata_ram" + post_slice2 + " ;\n\n");
         }
         else
         {
            cond_load(Mout_addr_ram_bitsize, post_slice1, post_slice2, "M_Rdata_ram", i, "Sout_Rdata_ram" + post_slice2, mem_aggregate);
         }
      }
   }
   else
   {
      for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "[" + boost::lexical_cast<std::string>(i) + "]";
         std::string post_slice2;
         if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            post_slice2 = "[" + boost::lexical_cast<std::string>((i + 1) * bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * bitsize) + "]";
         else
            post_slice2 = "";
         if(memory_allocation_policy == MemoryAllocation_Policy::ALL_BRAM or memory_allocation_policy == MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
         {
            cond_load(Mout_addr_ram_bitsize, post_slice1, post_slice2, "M_Rdata_ram_delayed_temporary", i, STR(bitsize) + "'b0", mem_aggregate);
            writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
            writer->write("begin");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("for (_i_=0; _i_<`MEM_DELAY_READ-1; _i_=_i_+1)");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("if(_i_ == `MEM_DELAY_READ-2)");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("M_Rdata_ram_delayed[_i_]" + post_slice2 + " <= M_Rdata_ram_delayed_temporary" + post_slice2 + ";");
            writer->write(STR(STD_CLOSING_CHAR) + "\n");
            writer->write("else");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("M_Rdata_ram_delayed[_i_]" + post_slice2 + " <= M_Rdata_ram_delayed[_i_+1]" + post_slice2 + ";");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write(STR(STD_CLOSING_CHAR) + "\n");
            writer->write(STR(STD_CLOSING_CHAR) + "\n");
            writer->write("end\n");
            writer->write("assign M_Rdata_ram" + post_slice2 + " = M_Rdata_ram_delayed[0]" + post_slice2 + ";\n\n");
         }
         else
         {
            cond_load(Mout_addr_ram_bitsize, post_slice1, post_slice2, "M_Rdata_ram", i, STR(bitsize) + "'b0", mem_aggregate);
         }
      }
   }
   structural_objectRef Sin_Wdata_ram_port = mod->find_member("S_Wdata_ram", port_o_K, cir);
   if(Sin_Wdata_ram_port)
      writer->write("assign S_Wdata_ram = Mout_Wdata_ram;\n");

   writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR) + "\n");
   for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
   {
      std::string post_slice1;
      if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
         post_slice1 = "[" + boost::lexical_cast<std::string>(i) + "]";

      writer->write("if ((Mout_oe_ram" + post_slice1 + "===1'b1 && base_addr <= Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) +
                    "] && Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)))\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      writer->write("if (reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] >= 0 && reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] < `MEM_DELAY_READ-1)");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      writer->write("reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] <= 1 + reg_DataReady[" + boost::lexical_cast<std::string>(i) + "];");
      writer->write(STR(STD_CLOSING_CHAR) + "\n");
      writer->write("else");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      writer->write("reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] <= 0;\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n\n");

      writer->write("else if ((Mout_we_ram" + post_slice1 + "===1'b1 && base_addr <= Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) +
                    "] && Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)))\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      writer->write("if (reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] >= 0 && reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] < `MEM_DELAY_WRITE-1)");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      writer->write("reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] <= 1 + reg_DataReady[" + boost::lexical_cast<std::string>(i) + "];\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("else");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      writer->write("reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] <= 0;\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n\n");

      writer->write("else");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      writer->write("reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] <= 0;");
      writer->write(STR(STD_CLOSING_CHAR) + "\n");
   }
   writer->write(STR(STD_CLOSING_CHAR) + "\n");
   writer->write("end\n\n");

   structural_objectRef Sin_DataRdy_port = mod->find_member("Sin_DataRdy", port_o_K, cir);
   structural_objectRef Sout_DataRdy_port = mod->find_member("Sout_DataRdy", port_o_K, cir);
   if(Sin_DataRdy_port)
   {
      for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "[" + boost::lexical_cast<std::string>(i) + "]";
         writer->write("assign Sin_DataRdy" + post_slice1 + " = (base_addr <= Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) +
                       "] && Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)) && (((reg_DataReady[" +
                       boost::lexical_cast<std::string>(i) + "] == `MEM_DELAY_READ-1)) || (Mout_we_ram" + post_slice1 + "===1'b1 && (reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] == `MEM_DELAY_WRITE-1)));\n");
      }
      THROW_ASSERT(Sout_DataRdy_port, "Sout_DataRdy port is missing");
      writer->write("assign M_DataRdy = Sout_DataRdy;\n");
   }
   else if(Sout_DataRdy_port)
   {
      for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "[" + boost::lexical_cast<std::string>(i) + "]";
         writer->write("assign M_DataRdy" + post_slice1 + " = Sout_DataRdy" + post_slice1 + " | ((base_addr <= Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                       boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) + "] && Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) +
                       "] < (base_addr + MEMSIZE)) && (((reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] == `MEM_DELAY_READ-1)) || (Mout_we_ram" + post_slice1 + "===1'b1 && (reg_DataReady[" + boost::lexical_cast<std::string>(i) +
                       "] == `MEM_DELAY_WRITE-1))));\n");
      }
   }
   else
   {
      for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "[" + boost::lexical_cast<std::string>(i) + "]";
         writer->write("assign M_DataRdy" + post_slice1 + " = (base_addr <= Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) +
                       "] && Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)) && (((reg_DataReady[" +
                       boost::lexical_cast<std::string>(i) + "] == `MEM_DELAY_READ-1)) || (Mout_we_ram" + post_slice1 + "===1'b1 && (reg_DataReady[" + boost::lexical_cast<std::string>(i) + "] == `MEM_DELAY_WRITE-1)));\n");
      }
   }

   /// Master inputs set up
   structural_objectRef Min_oe_ram_port = mod->find_member("Min_oe_ram", port_o_K, cir);
   if(Min_oe_ram_port)
      writer->write("assign Min_oe_ram = 0;\n");
   structural_objectRef Min_we_ram_port = mod->find_member("Min_we_ram", port_o_K, cir);
   if(Min_we_ram_port)
      writer->write("assign Min_we_ram = 0;\n");
   structural_objectRef Min_addr_ram_port = mod->find_member("Min_addr_ram", port_o_K, cir);
   if(Min_addr_ram_port)
      writer->write("assign Min_addr_ram = 0;\n");
   structural_objectRef Min_data_ram_size_port = mod->find_member("Min_data_ram_size", port_o_K, cir);
   if(Min_data_ram_size_port)
      writer->write("assign Min_data_ram_size = 0;\n");
   /// Slave inputs connections
   structural_objectRef Mout_oe_ram_port = mod->find_member("Mout_oe_ram", port_o_K, cir);
   THROW_ASSERT(Mout_oe_ram_port, "Mout_oe_ram port is missing");
   structural_objectRef S_oe_ram_port = mod->find_member("S_oe_ram", port_o_K, cir);
   if(S_oe_ram_port)
   {
      writer->write("assign S_oe_ram = Mout_oe_ram;\n");
   }
   structural_objectRef Mout_we_ram_port = mod->find_member("Mout_we_ram", port_o_K, cir);
   THROW_ASSERT(Mout_we_ram_port, "Mout_we_ram port is missing");
   structural_objectRef S_we_ram_port = mod->find_member("S_we_ram", port_o_K, cir);
   if(S_we_ram_port)
   {
      writer->write("assign S_we_ram = Mout_we_ram;\n");
   }
   structural_objectRef S_addr_ram_port = mod->find_member("S_addr_ram", port_o_K, cir);
   if(S_addr_ram_port)
   {
      writer->write("assign S_addr_ram = Mout_addr_ram;\n");
   }

   structural_objectRef S_data_ram_size_port = mod->find_member("S_data_ram_size", port_o_K, cir);
   if(S_data_ram_size_port)
   {
      writer->write("assign S_data_ram_size = Mout_data_ram_size;\n");
   }

   writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR) + "\n");
   for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports && i < Mout_Wdata_ram_n_ports; ++i)
   {
      std::string post_slice1;
      if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
         post_slice1 = "[" + boost::lexical_cast<std::string>(i) + "]";
      writer->write("if (Mout_we_ram" + post_slice1 + "===1'b1 && Mout_oe_ram" + post_slice1 + "===1'b1)\n");
      writer->write("begin\n");
      writer->write(STR(STD_OPENING_CHAR));
      writer->write_comment("error\n");
      writer->write("$display(\"ERROR - Mout_we_ram and Mout_oe_ram both enabled\");\n");
      writer->write("$fclose(res_file);\n");
      writer->write("$fclose(file);\n");
      writer->write("$finish;\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
   }
   writer->write(STR(STD_CLOSING_CHAR) + "\n");
   writer->write("end\n\n");

   return;
}

void MinimalInterfaceTestbench::write_interface_handler() const
{
   if(mod->get_in_port_size())
   {
      bool firstRValid = true;
      bool firstWAck = true;
      bool firstFull_n = true;
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         const structural_objectRef& portInst = mod->get_in_port(i);
         auto InterfaceType = GetPointer<port_o>(portInst)->get_port_interface();
         if(InterfaceType != port_o::port_interface::PI_DEFAULT)
         {
            /// check if you have both _vld and _ack
            std::string orig_port_name;
            bool have_both = false;
            if(InterfaceType == port_o::port_interface::PI_RVALID)
            {
               orig_port_name = portInst->get_id();
               auto size_string = orig_port_name.size() - std::string("_vld").size();
               orig_port_name = orig_port_name.substr(0, size_string);
               auto port_ack = mod->find_member(orig_port_name + "_ack", port_o_K, cir);
               if(port_ack)
               {
                  have_both = true;
               }
            }
            if(InterfaceType == port_o::port_interface::PI_WACK)
            {
               orig_port_name = portInst->get_id();
               auto size_string = orig_port_name.size() - std::string("_ack").size();
               orig_port_name = orig_port_name.substr(0, size_string);
               auto port_valid = mod->find_member(orig_port_name + "_vld", port_o_K, cir);
               if(port_valid)
               {
                  have_both = true;
               }
            }

            if(InterfaceType == port_o::port_interface::PI_RVALID || InterfaceType == port_o::port_interface::PI_WACK || InterfaceType == port_o::port_interface::PI_EMPTY_N || InterfaceType == port_o::port_interface::PI_FULL_N)
            {
               if(!have_both && (firstRValid && InterfaceType == port_o::port_interface::PI_RVALID))
               {
                  firstRValid = false;
                  writer->write("reg __vld_port_state = 0;\n");
               }
               if(!have_both && (firstWAck && InterfaceType == port_o::port_interface::PI_WACK))
               {
                  firstWAck = false;
                  writer->write("integer __ack_port_state = 0;\n");
               }
               if(firstFull_n && InterfaceType == port_o::port_interface::PI_FULL_N)
               {
                  firstFull_n = false;
                  writer->write("integer __full_n_port_state = 0;\n");
               }
               if(InterfaceType == port_o::port_interface::PI_RVALID)
                  writer->write_comment("RVALID handler\n");
               else if(InterfaceType == port_o::port_interface::PI_WACK)
                  writer->write_comment("WACK handler\n");
               else if(InterfaceType == port_o::port_interface::PI_EMPTY_N)
                  writer->write_comment("EMPTY_N handler\n");
               else if(InterfaceType == port_o::port_interface::PI_FULL_N)
                  writer->write_comment("FULL_N handler\n");
               else
                  THROW_ERROR("unsupported interface type");
               writer->write("always @ (posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               if(have_both && InterfaceType == port_o::port_interface::PI_WACK)
               {
                  writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) + " <= " + HDL_manager::convert_to_identifier(writer.get(), orig_port_name) + "_vld & !" +
                                HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) + ";\n");
               }
               else
               {
                  if(InterfaceType == port_o::port_interface::PI_RVALID)
                  {
                     if(have_both)
                        writer->write("if(__state == 3)\n");
                     else
                        writer->write("if(__state == 3 && __vld_port_state == 0)\n");
                  }
                  else if(InterfaceType == port_o::port_interface::PI_WACK)
                  {
                     writer->write("if(__state == 3)\n");
                  }
                  else if(InterfaceType == port_o::port_interface::PI_EMPTY_N)
                     writer->write("if(__state == 3)\n");
                  else if(InterfaceType == port_o::port_interface::PI_FULL_N)
                     writer->write("if(__state == 3)\n");
                  else
                     THROW_ERROR("unsupported interface type");
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");
                  if(InterfaceType == port_o::port_interface::PI_WACK)
                     writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) + " <= __ack_port_state < 1 ? 1'b0 : 1'b1;\n");
                  else if(InterfaceType == port_o::port_interface::PI_FULL_N)
                     writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) + " <= __full_n_port_state < 3 ? 1'b0 : 1'b1;\n");
                  else
                     writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) + " <= 1'b1;\n");
                  if(InterfaceType == port_o::port_interface::PI_RVALID)
                  {
                     if(!have_both)
                        writer->write("__vld_port_state <= 1;");
                  }
                  else if(InterfaceType == port_o::port_interface::PI_WACK)
                     writer->write("__ack_port_state <= __ack_port_state + 1;");
                  else if(InterfaceType == port_o::port_interface::PI_EMPTY_N)
                     ;
                  else if(InterfaceType == port_o::port_interface::PI_FULL_N)
                     writer->write("__full_n_port_state <= __full_n_port_state + 1;");
                  else
                     THROW_ERROR("unsupported interface type");
                  writer->write(STR(STD_CLOSING_CHAR) + "\n");
                  writer->write("end\n");
                  writer->write("else");
                  writer->write(STR(STD_OPENING_CHAR) + "\n");
                  writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) + " <= 1'b0;");
                  writer->write(STR(STD_CLOSING_CHAR) + "\n");
               }
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write("end\n");
            }
         }
      }
   }
}

void MinimalInterfaceTestbench::write_slave_initializations(bool with_memory) const
{
   if(with_memory)
      return;
   /// write slave signals initialization
   if(mod->get_in_port_size())
   {
      bool print_header_comment = true;
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         const structural_objectRef& port_obj = mod->get_in_port(i);
         if(GetPointer<port_o>(port_obj)->get_is_memory())
         {
            if(GetPointer<port_o>(port_obj)->get_id().find("S") == 0)
            {
               if(print_header_comment)
               {
                  print_header_comment = false;
                  writer->write_comment("Slave signals initialization\n");
               }
               writer->write("assign " + HDL_manager::convert_to_identifier(writer.get(), mod->get_in_port(i)->get_id()) + " = 0;\n");
            }
         }
      }
      if(!print_header_comment)
         writer->write("\n");
   }
}

void MinimalInterfaceTestbench::write_input_signal_declaration(const tree_managerConstRef TreeM, bool& with_memory) const
{
   /// write input signals declaration
   if(mod->get_in_port_size())
   {
      writer->write_comment("INPUT SIGNALS\n");
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         const structural_objectRef& port_obj = mod->get_in_port(i);
         if(GetPointer<port_o>(port_obj)->get_is_memory())
         {
            if(GetPointer<port_o>(port_obj)->get_id().find("M") == 0)
               with_memory = true;
            writer->write("wire ");
         }
         else if(parameters->isOption(OPT_clock_name) && GetPointer<port_o>(port_obj)->get_id() == parameters->getOption<std::string>(OPT_clock_name))
            writer->write("input ");
         else if(GetPointer<port_o>(port_obj)->get_id() == CLOCK_PORT_NAME)
            writer->write("input ");
         else
         {
            writer->write("reg ");
         }

         writer->write(writer->type_converter(port_obj->get_typeRef()) + writer->type_converter_size(port_obj));
         auto port_name = mod->get_in_port(i)->get_id();
         if(parameters->isOption(OPT_clock_name) && GetPointer<port_o>(port_obj)->get_id() == parameters->getOption<std::string>(OPT_clock_name))
            port_name = CLOCK_PORT_NAME;
         else if(parameters->isOption(OPT_reset_name) && GetPointer<port_o>(port_obj)->get_id() == parameters->getOption<std::string>(OPT_reset_name))
            port_name = RESET_PORT_NAME;
         else if(parameters->isOption(OPT_start_name) && GetPointer<port_o>(port_obj)->get_id() == parameters->getOption<std::string>(OPT_start_name))
            port_name = START_PORT_NAME;
         writer->write(HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
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

            /// FIXME: real numbers at the moment have to be considered diffently because of computation of ulp; c++ code is still managed in the old way
            if(tree_helper::is_real(TreeM, pt_type_index) or flag_cpp)
            {
               long long int bitsize = tree_helper::size(TreeM, pt_type_index);
               writer->write("reg [" + STR(bitsize - 1) + ":0] ex_" + port_obj->get_id() + ";\n");
            }
            else
            {
               writer->write("reg [7:0] ex_" + port_obj->get_id() + ";\n");
            }
         }
      }
      writer->write("\n");
   }
   writer->write("reg start_next_sim;\n");
}

void MinimalInterfaceTestbench::write_output_signal_declaration() const
{
   /// write output signals declaration
   if(mod->get_out_port_size())
   {
      writer->write_comment("OUTPUT SIGNALS\n");
      for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         auto portInst = mod->get_out_port(i);
         writer->write("wire " + writer->type_converter(portInst->get_typeRef()) + writer->type_converter_size(portInst));
         auto port_name = portInst->get_id();
         if(parameters->isOption(OPT_done_name) && port_name == parameters->getOption<std::string>(OPT_done_name))
            port_name = DONE_PORT_NAME;
         writer->write(HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
         if(port_name == RETURN_PORT_NAME)
         {
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) + writer->type_converter_size(portInst));
            writer->write("ex_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) + writer->type_converter_size(portInst));
            writer->write("registered_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
         }
         if(GetPointer<port_o>(portInst)->get_port_interface() == port_o::port_interface::PI_WNONE)
         {
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) + writer->type_converter_size(portInst));
            writer->write("ex_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) + writer->type_converter_size(portInst));
            writer->write("registered_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
         }
         else if(GetPointer<port_o>(portInst)->get_port_interface() == port_o::port_interface::PI_DOUT)
         {
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) + writer->type_converter_size(portInst));
            writer->write("ex_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
         }
      }
      writer->write("\n");
   }
}

void MinimalInterfaceTestbench::write_signals(const tree_managerConstRef TreeM, bool& withMemory, bool&) const
{
   const MemoryAllocation_Policy memory_allocation_policy = parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy);
   write_input_signal_declaration(TreeM, withMemory);
   write_output_signal_declaration();
   // write parameters
   if(withMemory)
   {
      structural_objectRef M_Rdata_ram_port = mod->find_member("M_Rdata_ram", port_o_K, cir);
      long long int bitsize = M_Rdata_ram_port->get_typeRef()->size * M_Rdata_ram_port->get_typeRef()->vector_size;
      unsigned int n_ports = M_Rdata_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(M_Rdata_ram_port)->get_ports_size() : 1;
      writer->write("reg signed [31:0] reg_DataReady[" + STR(n_ports - 1) + ":0];\n");
      writer->write("wire [" + STR(bitsize * n_ports - 1) + ":0] mask;\n\n");
      if(memory_allocation_policy == MemoryAllocation_Policy::ALL_BRAM or memory_allocation_policy == MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         writer->write("wire [" + STR(bitsize * n_ports - 1) + ":0] M_Rdata_ram_delayed_temporary;\n\n");
         writer->write("reg [" + STR(bitsize * n_ports - 1) + ":0] M_Rdata_ram_delayed [`MEM_DELAY_READ-2:0];\n\n");
      }
   }
}

void MinimalInterfaceTestbench::read_input_value_from_file_RNONE(const std::string& input_name, bool& first_valid_input, unsigned bitsize) const
{
   if(input_name != CLOCK_PORT_NAME && input_name != RESET_PORT_NAME && input_name != START_PORT_NAME)
   {
      writer->write("\n");
      writer->write_comment("Read a value for " + input_name + " --------------------------------------------------------------\n");
      if(!first_valid_input)
         writer->write("_ch_ = $fgetc(file);\n");

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
         writer->write("_r_ = $fscanf(file,\"%b\\n\", paddr" + input_name + "); ");
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
            writer->write("$display(\"ERROR - End of file reached before getting all the values for the parameters\");\n");
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
            writer->write("$display(\"ERROR - Unknown error while reading the file. Character found: %c\", _ch_[7:0]);\n");
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
         std::string mem_aggregate = "{";
         for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
         {
            if(bitsize_index)
               mem_aggregate += ", ";
            mem_aggregate += "_bambu_testbench_mem_[paddr" + input_name + " + " + STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]";
         }
         mem_aggregate += "}";
         writer->write("assign " + input_name + " = " + mem_aggregate + ";\n");
         size_t escaped_pos = input_name.find('\\');
         std::string nonescaped_name = input_name;
         if(escaped_pos != std::string::npos)
            nonescaped_name.erase(std::remove(nonescaped_name.begin(), nonescaped_name.end(), '\\'), nonescaped_name.end());
         if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
            writer->write("$display(\"Value found for input " + nonescaped_name + ": %b\", " + input_name + ");\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n");
      writer->write_comment("Value for " + input_name + " found ---------------------------------------------------------------\n");
   }
}

void MinimalInterfaceTestbench::write_file_reading_operations() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Write file reading operations");
   /// file reading operations
   // cppcheck-suppress variableScope
   bool first_valid_input = true;
   /// iterate over all interface ports
   const auto& DesignSignature = HLSMgr->RSim->simulationArgSignature;
   for(auto par : DesignSignature)
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
         portInst = mod->find_member(par + "_din", port_o_K, cir);
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
      std::string input_name = HDL_manager::convert_to_identifier(writer.get(), portInst->get_id());
      if(InterfaceType == port_o::port_interface::PI_DEFAULT)
         read_input_value_from_file(input_name, first_valid_input);
      else if(InterfaceType == port_o::port_interface::PI_RNONE)
      {
         auto port_bitwidth = GetPointer<port_o>(portInst)->get_typeRef()->size * GetPointer<port_o>(portInst)->get_typeRef()->vector_size;
         unsigned bitsize = 0;
         if(port_bitwidth <= 512)
            bitsize = resize_to_1_8_16_32_64_128_256_512(port_bitwidth);
         else
         {
            if(port_bitwidth % 8)
               bitsize = 8 * (port_bitwidth / 8) + 8;
            else
               bitsize = port_bitwidth;
         }
         read_input_value_from_file_RNONE(input_name, first_valid_input, bitsize);
      }
      else if(InterfaceType == port_o::port_interface::PI_WNONE || InterfaceType == port_o::port_interface::PI_DIN || InterfaceType == port_o::port_interface::PI_DOUT)
         read_input_value_from_file("paddr" + input_name, first_valid_input);
      else
         THROW_ERROR("not yet supported port interface for port " + input_name);
   }
   if(not first_valid_input)
      writer->write("_ch_ = $fgetc(file);\n");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written file reading operations");
}

std::string MinimalInterfaceTestbench::memory_aggregate_slices(unsigned int i, long long int bitsize, long long int Mout_addr_ram_bitsize) const
{
   std::string mem_aggregate = "{";
   for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
   {
      if(bitsize_index)
         mem_aggregate += ", ";
      mem_aggregate += "_bambu_testbench_mem_[Mout_addr_ram[" + boost::lexical_cast<std::string>((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + boost::lexical_cast<std::string>(i * Mout_addr_ram_bitsize) + "] + " + STR((bitsize - bitsize_index) / 8 - 1) +
                       " - base_addr]";
   }
   mem_aggregate += "}";

   return mem_aggregate;
}
