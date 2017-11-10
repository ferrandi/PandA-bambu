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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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

///Header include
#include "minimal_interface_testbench.hpp"

///. include
#include "Parameter.hpp"

///circuit include
#include "structural_objects.hpp"

///design_flows/backend/ToHDL include
#include "HDL_manager.hpp"
#include "language_writer.hpp"

///HLS include
#include "hls_manager.hpp"

///HLS/memory includes
#include "memory_allocation.hpp"

#if HAVE_FROM_DISCREPANCY_BUILT
// include from HLS/vcd
#include "Discrepancy.hpp"
#endif

///tree include
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

MinimalInterfaceTestbench::MinimalInterfaceTestbench(
      const ParameterConstRef _parameters,
      const HLS_managerRef _AppM,
      const DesignFlowManagerConstRef _design_flow_manager)
   : TestbenchGenerationBaseStep(
         _parameters,
         _AppM,
         _design_flow_manager,
         HLSFlowStep_Type::MINIMAL_TESTBENCH_GENERATION)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

MinimalInterfaceTestbench::~MinimalInterfaceTestbench()
{}

void MinimalInterfaceTestbench::cond_load(
      long long int Mout_addr_ram_bitsize,
      std::string,
      std::string post_slice2,
      std::string res_string,
      unsigned int i,
      std::string in_else,
      std::string mem_aggregate)
   const
{
   writer->write("assign " + res_string + post_slice2 + " = ((base_addr <= Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] && Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] < (base_addr + MEMSIZE)))" + " ? " + mem_aggregate + " : " + in_else + ";\n");
}

void MinimalInterfaceTestbench::write_call(bool) const
{
   writer->write("always @(negedge clock)\n");
   writer->write("begin\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("if (done_port == 1)\n");
   writer->write("begin\n");
   writer->write(STR(STD_OPENING_CHAR));

   if (mod->find_member(RETURN_PORT_NAME, port_o_K, cir))
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
         std::string mask_string = "(1 << Mout_data_ram_size[" + boost::lexical_cast<std::string>((i+1)*Mout_data_ram_size_bitsize-1)+":"+ boost::lexical_cast<std::string>((i)*Mout_data_ram_size_bitsize) + "]) -1";
         writer->write("assign mask["+ boost::lexical_cast<std::string>((i+1)*Mout_Wdata_ram_bitsize-1)+":"+ boost::lexical_cast<std::string>((i)*Mout_Wdata_ram_bitsize) +"] = " + mask_string + ";\n");
      }
   }
   else
      writer->write("assign mask = (1 << Mout_data_ram_size) -1;\n");

   writer->write_comment("OffChip Memory write\n");
   writer->write("always @(posedge clock)\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR)+"\n");

   structural_objectRef Mout_addr_ram_port = mod->find_member("Mout_addr_ram", port_o_K, cir);
   THROW_ASSERT(Mout_addr_ram_port, "Mout_addr_ram port is missing");
   long long int Mout_addr_ram_bitsize = Mout_addr_ram_port->get_typeRef()->size * Mout_addr_ram_port->get_typeRef()->vector_size;
   unsigned int Mout_Wdata_ram_n_ports = Mout_Wdata_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(Mout_Wdata_ram_port)->get_ports_size() : 1;
   long long int bitsize = Mout_Wdata_ram_port->get_typeRef()->size * Mout_Wdata_ram_port->get_typeRef()->vector_size;
   for(unsigned int i= 0; i < Mout_Wdata_ram_n_ports; ++i)
   {
      std::string mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
      std::string post_slice;
      if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
         post_slice = "["+boost::lexical_cast<std::string>(i)+"]";
      writer->write("if (Mout_we_ram"+ post_slice + " === 1'b1 && base_addr <= Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] && Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] < (base_addr + MEMSIZE))\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR)+"\n");
      if(Mout_Wdata_ram_port->get_kind() == port_vector_o_K)
         post_slice = "["+boost::lexical_cast<std::string>((i+1)*bitsize-1)+":"+boost::lexical_cast<std::string>(i*bitsize)+"]";
      else
         post_slice = "";
      writer->write(mem_aggregate + " = (Mout_Wdata_ram"+ post_slice + " & mask"+ post_slice + ") | (" + mem_aggregate + " & ~(mask"+ post_slice + "));\n");
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
      for(unsigned int i= 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "["+boost::lexical_cast<std::string>(i)+"]";
         std::string post_slice2;
         if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            post_slice2 = "["+boost::lexical_cast<std::string>((i+1)*bitsize-1)+":"+boost::lexical_cast<std::string>(i*bitsize)+"]";
         else
            post_slice2 = "";

         cond_load(Mout_addr_ram_bitsize, post_slice1, post_slice2, "Sin_Rdata_ram", i, STR(bitsize) + "'b0", mem_aggregate);
      }
      THROW_ASSERT(Sout_Rdata_ram_port, "Sout_Rdata_ram port is missing");
      writer->write("assign M_Rdata_ram = Sout_Rdata_ram;\n");
   }
   else if(Sout_Rdata_ram_port)
   {
      for(unsigned int i= 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "["+boost::lexical_cast<std::string>(i)+"]";
         std::string post_slice2;
         if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            post_slice2 = "["+boost::lexical_cast<std::string>((i+1)*bitsize-1)+":"+boost::lexical_cast<std::string>(i*bitsize)+"]";
         else
            post_slice2 = "";
         if(memory_allocation_policy == MemoryAllocation_Policy::ALL_BRAM or memory_allocation_policy == MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
         {
            cond_load(Mout_addr_ram_bitsize, post_slice1, post_slice2, "M_Rdata_ram_delayed_temporary", i, STR(bitsize) + "'b0", mem_aggregate);
            writer->write("always @(posedge clock)\n");
            writer->write("begin");
            writer->write(STR(STD_OPENING_CHAR)+"\n");
            writer->write("for (_i_=0; _i_<`MEM_DELAY_READ-1; _i_=_i_+1)");
            writer->write(STR(STD_OPENING_CHAR)+"\n");
            writer->write("if(_i_ == `MEM_DELAY_READ-2)");
            writer->write(STR(STD_OPENING_CHAR)+"\n");
            writer->write("M_Rdata_ram_delayed[_i_]"+ post_slice2 + " <= M_Rdata_ram_delayed_temporary"+ post_slice2 + ";");
            writer->write(STR(STD_CLOSING_CHAR)+"\n");
            writer->write("else");
            writer->write(STR(STD_OPENING_CHAR)+"\n");
            writer->write("M_Rdata_ram_delayed[_i_]"+ post_slice2 + " <= M_Rdata_ram_delayed[_i_+1]"+ post_slice2 + ";");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write(STR(STD_CLOSING_CHAR)+"\n");
            writer->write(STR(STD_CLOSING_CHAR)+"\n");
            writer->write("end\n");
            writer->write("assign M_Rdata_ram"+ post_slice2 + " = M_Rdata_ram_delayed[0]"+ post_slice2 + "|" + "Sout_Rdata_ram"+ post_slice2 +" ;\n\n");
         }
         else
         {
            cond_load(Mout_addr_ram_bitsize, post_slice1, post_slice2, "M_Rdata_ram", i, "Sout_Rdata_ram"+ post_slice2, mem_aggregate);
         }
      }
   }
   else
   {
      for(unsigned int i= 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "["+boost::lexical_cast<std::string>(i)+"]";
         std::string post_slice2;
         if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            post_slice2 = "["+boost::lexical_cast<std::string>((i+1)*bitsize-1)+":"+boost::lexical_cast<std::string>(i*bitsize)+"]";
         else
            post_slice2 = "";
         if(memory_allocation_policy == MemoryAllocation_Policy::ALL_BRAM or memory_allocation_policy == MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
         {
            cond_load(Mout_addr_ram_bitsize, post_slice1, post_slice2, "M_Rdata_ram_delayed_temporary", i, STR(bitsize) + "'b0", mem_aggregate);
            writer->write("always @(posedge clock)\n");
            writer->write("begin");
            writer->write(STR(STD_OPENING_CHAR)+"\n");
            writer->write("for (_i_=0; _i_<`MEM_DELAY_READ-1; _i_=_i_+1)");
            writer->write(STR(STD_OPENING_CHAR)+"\n");
            writer->write("if(_i_ == `MEM_DELAY_READ-2)");
            writer->write(STR(STD_OPENING_CHAR)+"\n");
            writer->write("M_Rdata_ram_delayed[_i_]"+ post_slice2 + " <= M_Rdata_ram_delayed_temporary"+ post_slice2 + ";");
            writer->write(STR(STD_CLOSING_CHAR)+"\n");
            writer->write("else");
            writer->write(STR(STD_OPENING_CHAR)+"\n");
            writer->write("M_Rdata_ram_delayed[_i_]"+ post_slice2 + " <= M_Rdata_ram_delayed[_i_+1]"+ post_slice2 + ";");
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write(STR(STD_CLOSING_CHAR)+"\n");
            writer->write(STR(STD_CLOSING_CHAR)+"\n");
            writer->write("end\n");
            writer->write("assign M_Rdata_ram"+ post_slice2 + " = M_Rdata_ram_delayed[0]"+ post_slice2 + ";\n\n");
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


   writer->write("always @(posedge clock)\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR)+"\n");
   for(unsigned int i= 0; i < M_Rdata_ram_port_n_ports; ++i)
   {
      std::string post_slice1;
      if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
         post_slice1 = "["+boost::lexical_cast<std::string>(i)+"]";

      writer->write("if ((Mout_oe_ram"+ post_slice1 + "===1'b1 && base_addr <= Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] && Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] < (base_addr + MEMSIZE)))\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR)+"\n");
      writer->write("if (reg_DataReady["+boost::lexical_cast<std::string>(i)+"] >= 0 && reg_DataReady["+boost::lexical_cast<std::string>(i)+"] < `MEM_DELAY_READ-1)");
      writer->write(STR(STD_OPENING_CHAR)+"\n");
      writer->write("reg_DataReady["+boost::lexical_cast<std::string>(i)+"] <= 1 + reg_DataReady["+boost::lexical_cast<std::string>(i)+"];");
      writer->write(STR(STD_CLOSING_CHAR)+"\n");
      writer->write("else");
      writer->write(STR(STD_OPENING_CHAR)+"\n");
      writer->write("reg_DataReady["+boost::lexical_cast<std::string>(i)+"] <= 0;\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n\n");

      writer->write("else if ((Mout_we_ram"+ post_slice1 + "===1'b1 && base_addr <= Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] && Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] < (base_addr + MEMSIZE)))\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR)+"\n");
      writer->write("if (reg_DataReady["+boost::lexical_cast<std::string>(i)+"] >= 0 && reg_DataReady["+boost::lexical_cast<std::string>(i)+"] < `MEM_DELAY_WRITE-1)");
      writer->write(STR(STD_OPENING_CHAR)+"\n");
      writer->write("reg_DataReady["+boost::lexical_cast<std::string>(i)+"] <= 1 + reg_DataReady["+boost::lexical_cast<std::string>(i)+"];\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("else");
      writer->write(STR(STD_OPENING_CHAR)+"\n");
      writer->write("reg_DataReady["+boost::lexical_cast<std::string>(i)+"] <= 0;\n");
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n\n");

      writer->write("else");
      writer->write(STR(STD_OPENING_CHAR)+"\n");
      writer->write("reg_DataReady["+boost::lexical_cast<std::string>(i)+"] <= 0;");
      writer->write(STR(STD_CLOSING_CHAR)+"\n");
   }
   writer->write(STR(STD_CLOSING_CHAR)+"\n");
   writer->write("end\n\n");

   structural_objectRef Sin_DataRdy_port = mod->find_member("Sin_DataRdy", port_o_K, cir);
   structural_objectRef Sout_DataRdy_port = mod->find_member("Sout_DataRdy", port_o_K, cir);
   if(Sin_DataRdy_port)
   {
      for(unsigned int i= 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "["+boost::lexical_cast<std::string>(i)+"]";
         writer->write("assign Sin_DataRdy"+ post_slice1 + " = (base_addr <= Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] && Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] < (base_addr + MEMSIZE)) && (((reg_DataReady["+boost::lexical_cast<std::string>(i)+"] == `MEM_DELAY_READ-1)) || (Mout_we_ram"+ post_slice1 + "===1'b1 && (reg_DataReady["+boost::lexical_cast<std::string>(i)+"] == `MEM_DELAY_WRITE-1)));\n");
      }
      THROW_ASSERT(Sout_DataRdy_port, "Sout_DataRdy port is missing");
      writer->write("assign M_DataRdy = Sout_DataRdy;\n");
   }
   else if(Sout_DataRdy_port)
   {
      for(unsigned int i= 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "["+boost::lexical_cast<std::string>(i)+"]";
         writer->write("assign M_DataRdy"+ post_slice1 + " = Sout_DataRdy"+ post_slice1 + " | ((base_addr <= Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] && Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] < (base_addr + MEMSIZE)) && (((reg_DataReady["+boost::lexical_cast<std::string>(i)+"] == `MEM_DELAY_READ-1)) || (Mout_we_ram"+ post_slice1 + "===1'b1 && (reg_DataReady["+boost::lexical_cast<std::string>(i)+"] == `MEM_DELAY_WRITE-1))));\n");
      }
   }
   else
   {
      for(unsigned int i= 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            post_slice1 = "["+boost::lexical_cast<std::string>(i)+"]";
         writer->write("assign M_DataRdy"+ post_slice1 + " = (base_addr <= Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] && Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] < (base_addr + MEMSIZE)) && (((reg_DataReady["+boost::lexical_cast<std::string>(i)+"] == `MEM_DELAY_READ-1)) || (Mout_we_ram"+ post_slice1 + "===1'b1 && (reg_DataReady["+boost::lexical_cast<std::string>(i)+"] == `MEM_DELAY_WRITE-1)));\n");
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

   writer->write("always @(posedge clock)\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR)+"\n");
   for(unsigned int i= 0; i < M_Rdata_ram_port_n_ports && i < Mout_Wdata_ram_n_ports; ++i)
   {
      std::string post_slice1;
      if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
         post_slice1 = "["+boost::lexical_cast<std::string>(i)+"]";
      writer->write("if (Mout_we_ram"+ post_slice1 + "===1'b1 && Mout_oe_ram"+ post_slice1 + "===1'b1)\n");
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
   writer->write(STR(STD_CLOSING_CHAR)+"\n");
   writer->write("end\n\n");

   return;
}

void MinimalInterfaceTestbench::write_slave_initializations(
      bool with_memory)
   const
{
   if(with_memory) return;
   /// write slave signals initialization
   if (mod->get_in_port_size())
   {
      bool print_header_comment = true;
      for (unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         const structural_objectRef& port_obj = mod->get_in_port(i);
         if(GetPointer<port_o>(port_obj)->get_is_memory())
         {
            if (GetPointer<port_o>(port_obj)->get_id().find("S") == 0)
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

void MinimalInterfaceTestbench::write_input_signal_declaration(
      const tree_managerConstRef TreeM,
      bool & with_memory)
   const
{
   /// write input signals declaration
   if (mod->get_in_port_size())
   {
      writer->write_comment("INPUT SIGNALS\n");
      for (unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         const structural_objectRef& port_obj = mod->get_in_port(i);
         if(GetPointer<port_o>(port_obj)->get_is_memory())
         {
            if (GetPointer<port_o>(port_obj)->get_id().find("M") == 0)
               with_memory=true;
            writer->write("wire ");
         }
         else if (GetPointer<port_o>(port_obj)->get_id() == CLOCK_PORT_NAME)
            writer->write("input ");
         else
            writer->write("reg ");

         writer->write(writer->type_converter(port_obj->get_typeRef()) + writer->type_converter_size(port_obj));
         writer->write(HDL_manager::convert_to_identifier(writer.get(), mod->get_in_port(i)->get_id()) + ";\n");
         if (port_obj->get_typeRef()->treenode > 0 && tree_helper::is_a_pointer(TreeM, port_obj->get_typeRef()->treenode))
         {
            unsigned int pt_type_index = tree_helper::get_pointed_type(TreeM, tree_helper::get_type_index(TreeM,port_obj->get_typeRef()->treenode));
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

            writer->write("reg [" + STR(bitsize -1)+ ":0] ex_" + port_obj->get_id() + ";\n");
         }
      }
      writer->write("\n");
   }
   writer->write("reg start_next_sim;\n");
}


void MinimalInterfaceTestbench::write_output_signal_declaration() const
{
   /// write output signals declaration
   if (mod->get_out_port_size())
   {
      writer->write_comment("OUTPUT SIGNALS\n");
      for (unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         writer->write("wire " + writer->type_converter(mod->get_out_port(i)->get_typeRef()) + writer->type_converter_size(mod->get_out_port(i)));
         writer->write(HDL_manager::convert_to_identifier(writer.get(), mod->get_out_port(i)->get_id()) + ";\n");
         if (mod->get_out_port(i)->get_id() == RETURN_PORT_NAME)
         {
            writer->write("reg " + writer->type_converter(mod->get_out_port(i)->get_typeRef()) + writer->type_converter_size(mod->get_out_port(i)));
            writer->write("ex_" + HDL_manager::convert_to_identifier(writer.get(), mod->get_out_port(i)->get_id()) + ";\n");
            writer->write("reg " + writer->type_converter(mod->get_out_port(i)->get_typeRef()) + writer->type_converter_size(mod->get_out_port(i)));
            writer->write("registered_" + HDL_manager::convert_to_identifier(writer.get(), mod->get_out_port(i)->get_id()) + ";\n");
         }
      }
      writer->write("\n");
   }
}

void MinimalInterfaceTestbench::write_signals(
      const tree_managerConstRef TreeM,
      bool & withMemory,
      bool & )
   const
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
      writer->write("reg signed [31:0] reg_DataReady["+ STR(n_ports-1)+ ":0];\n");
      writer->write("wire ["+ STR(bitsize*n_ports-1)+ ":0] mask;\n\n");
      if(memory_allocation_policy == MemoryAllocation_Policy::ALL_BRAM or memory_allocation_policy == MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         writer->write("wire ["+ STR(bitsize*n_ports-1)+ ":0] M_Rdata_ram_delayed_temporary;\n\n");
         writer->write("reg ["+ STR(bitsize*n_ports-1)+ ":0] M_Rdata_ram_delayed [`MEM_DELAY_READ-2:0];\n\n");
      }
   }
}

void MinimalInterfaceTestbench::write_file_reading_operations() const
{
   /// file reading operations
   bool first_valid_input = true;
   /// iterate over all input ports
   if (mod->get_in_port_size())
   {
      /// write code for input values extraction
      for (unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         if (GetPointer<port_o>(mod->get_in_port(i))->get_is_memory())
            continue;
         if (GetPointer<port_o>(mod->get_in_port(i))->get_is_extern() && GetPointer<port_o>(mod->get_in_port(i))->get_is_global())
            continue;
         std::string input_name =
            HDL_manager::convert_to_identifier(writer.get(), mod->get_in_port(i)->get_id());
         read_input_value_from_file(input_name, first_valid_input);
      }
   }
   if (not first_valid_input)
      writer->write("_ch_ = $fgetc(file);\n");
}

std::string MinimalInterfaceTestbench::memory_aggregate_slices(
      unsigned int i,
      long long int bitsize,
      long long int Mout_addr_ram_bitsize)
   const
{
   std::string mem_aggregate = "{";
   for(unsigned int bitsize_index=0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
   {
      if(bitsize_index) mem_aggregate += ", ";
      mem_aggregate += "_bambu_testbench_mem_[Mout_addr_ram["+boost::lexical_cast<std::string>((i+1)*Mout_addr_ram_bitsize-1)+ ":"+boost::lexical_cast<std::string>(i*Mout_addr_ram_bitsize)+"] + " + STR((bitsize - bitsize_index)/8-1) + " - base_addr]";
   }
   mem_aggregate += "}";

   return mem_aggregate;
}
