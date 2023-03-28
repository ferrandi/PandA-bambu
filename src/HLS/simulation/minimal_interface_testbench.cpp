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
#include "minimal_interface_testbench.hpp"

#include "HDL_manager.hpp"
#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "memory_allocation.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "structural_objects.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#if HAVE_FROM_DISCREPANCY_BUILT
#include "Discrepancy.hpp"
#endif

#include <string>

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

MinimalInterfaceTestbench::MinimalInterfaceTestbench(const ParameterConstRef _parameters, const HLS_managerRef _AppM,
                                                     const DesignFlowManagerConstRef _design_flow_manager)
    : TestbenchGenerationBaseStep(_parameters, _AppM, _design_flow_manager,
                                  HLSFlowStep_Type::MINIMAL_TESTBENCH_GENERATION)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

MinimalInterfaceTestbench::~MinimalInterfaceTestbench() = default;

void MinimalInterfaceTestbench::cond_load(unsigned long long Mout_addr_ram_bitsize, const std::string& post_slice,
                                          const std::string& res_string, unsigned int i, const std::string& in_else,
                                          const std::string& mem_aggregate) const
{
   writer->write("assign " + res_string + post_slice + " = ((base_addr <= Mout_addr_ram[" +
                 STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + STR(i * Mout_addr_ram_bitsize) +
                 "] && Mout_addr_ram[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                 STR(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)))" + " ? " + mem_aggregate + " : " +
                 in_else + ";\n");
}

void MinimalInterfaceTestbench::cond_load_from_queue(unsigned long long Mout_addr_ram_bitsize,
                                                     unsigned int Mout_addr_ram_n_ports, std::string queue_type,
                                                     const std::string& post_slice, const std::string& res_string,
                                                     unsigned int i, const std::string& in_else_low,
                                                     const std::string& in_else_high,
                                                     const std::string& mem_aggregate) const
{
   /* In case of address not belonging to the bambu address space return  different values. Low addresses can be
    * associated to BRAMS */
   writer->write(res_string + post_slice + " = base_addr <= Mout_addr_ram_queue_curr[" +
                 STR((i + 1) * Mout_addr_ram_bitsize - 1) + "+(" + queue_type + "-1)*" +
                 STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + ":" + STR(i * Mout_addr_ram_bitsize) + "+(" +
                 queue_type + "-1)*" + STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) +
                 "] ? (Mout_addr_ram_queue_curr[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + "+(" + queue_type +
                 "-1)*" + STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + ":" + STR(i * Mout_addr_ram_bitsize) +
                 "+(" + queue_type + "-1)*" + STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) +
                 "] < (base_addr + MEMSIZE)" + " ? " + mem_aggregate + " : " + in_else_high + ") : " + in_else_low +
                 ";\n");
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

void MinimalInterfaceTestbench::update_memory_queue(std::string port_name, std::string delay_type,
                                                    unsigned long long bitsize, unsigned long long portsize) const
{
   const auto size = bitsize * portsize;
   writer->write("generate");
   writer->write(STR(STD_OPENING_CHAR) + "\n");
   writer->write("if(" + delay_type + " != 1)\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR) + "\n");
   writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR) + "\n");
   writer->write(port_name + "_queue_next[" + delay_type + "*" + STR(size) + " -1 : " + STR(size) +
                 "] <= " + port_name + "_queue_curr[(" + delay_type + "-1) *" + STR(size) + " -1 : 0];\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write("always @(*)\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR) + "\n");
   writer->write(port_name + "_queue_curr[" + delay_type + "*" + STR(size) + " -1 : " + STR(size) + "] = " + port_name +
                 "_queue_next[" + delay_type + "*" + STR(size) + " -1 : " + STR(size) + "];\n");
   writer->write(port_name + "_queue_curr[" + STR(size - 1) + " :0] = " + port_name + ";\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write("else\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR) + "\n");
   writer->write("always @(*)\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR) + "\n");
   writer->write(port_name + "_queue_curr[" + STR(size - 1) + " :0] = " + port_name + ";\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("endgenerate\n\n");
}

void MinimalInterfaceTestbench::write_memory_handler() const
{
   // TO DO remove when upgrading svelto
   if(parameters->isOption(OPT_parse_pragma) && parameters->getOption<bool>(OPT_parse_pragma))
   {
      const auto memory_allocation_policy =
          parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy);
      const auto Mout_data_ram_size_port = mod->find_member("Mout_data_ram_size", port_o_K, cir);
      THROW_ASSERT(Mout_data_ram_size_port, "Mout_data_ram_size port is missing");
      const auto Mout_Wdata_ram_port = mod->find_member("Mout_Wdata_ram", port_o_K, cir);
      THROW_ASSERT(Mout_Wdata_ram_port, "Mout_Wdata_ram port is missing");
      const auto Mout_Wdata_ram_bitsize =
          Mout_Wdata_ram_port->get_typeRef()->size * Mout_Wdata_ram_port->get_typeRef()->vector_size;
      if(Mout_data_ram_size_port->get_kind() == port_vector_o_K)
      {
         const auto Mout_data_ram_size_n_ports = Mout_data_ram_size_port->get_kind() == port_vector_o_K ?
                                                     GetPointer<port_o>(Mout_data_ram_size_port)->get_ports_size() :
                                                     1;
         const auto Mout_data_ram_size_bitsize =
             Mout_data_ram_size_port->get_typeRef()->size * Mout_data_ram_size_port->get_typeRef()->vector_size;
         for(unsigned int i = 0; i < Mout_data_ram_size_n_ports; ++i)
         {
            const auto mask_string = "(1 << Mout_data_ram_size[" + STR((i + 1) * Mout_data_ram_size_bitsize - 1) + ":" +
                                     STR((i)*Mout_data_ram_size_bitsize) + "]) -1";
            writer->write("assign mask[" + STR((i + 1) * Mout_Wdata_ram_bitsize - 1) + ":" +
                          STR((i)*Mout_Wdata_ram_bitsize) + "] = " + mask_string + ";\n");
         }
      }
      else
      {
         writer->write("assign mask = (1 << Mout_data_ram_size) -1;\n");
      }

      writer->write_comment("OffChip Memory write\n");
      writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR) + "\n");

      const auto Mout_addr_ram_port = mod->find_member("Mout_addr_ram", port_o_K, cir);
      THROW_ASSERT(Mout_addr_ram_port, "Mout_addr_ram port is missing");
      const auto Mout_addr_ram_bitsize =
          Mout_addr_ram_port->get_typeRef()->size * Mout_addr_ram_port->get_typeRef()->vector_size;
      const auto Mout_Wdata_ram_n_ports = Mout_Wdata_ram_port->get_kind() == port_vector_o_K ?
                                              GetPointer<port_o>(Mout_Wdata_ram_port)->get_ports_size() :
                                              1;
      auto bitsize = Mout_Wdata_ram_port->get_typeRef()->size * Mout_Wdata_ram_port->get_typeRef()->vector_size;
      for(unsigned int i = 0; i < Mout_Wdata_ram_n_ports; ++i)
      {
         const auto mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
         std::string post_slice;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
         {
            post_slice = "[" + STR(i) + "]";
         }
         writer->write("if (Mout_we_ram" + post_slice + " === 1'b1 && base_addr <= Mout_addr_ram[" +
                       STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + STR(i * Mout_addr_ram_bitsize) +
                       "] && Mout_addr_ram[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                       STR(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE))\n");
         writer->write("begin");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         if(Mout_Wdata_ram_port->get_kind() == port_vector_o_K)
         {
            post_slice = "[" + STR((i + 1) * bitsize - 1) + ":" + STR(i * bitsize) + "]";
         }
         else
         {
            post_slice = "";
         }
         writer->write(mem_aggregate + " = (Mout_Wdata_ram" + post_slice + " & mask" + post_slice + ") | (" +
                       mem_aggregate + " & ~(mask" + post_slice + "));\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n\n");

      const auto Min_Wdata_ram_port = mod->find_member("Min_Wdata_ram", port_o_K, cir);
      if(Min_Wdata_ram_port)
      {
         writer->write("assign Min_Wdata_ram = 0;\n");
      }

      const auto M_Rdata_ram_port = mod->find_member("M_Rdata_ram", port_o_K, cir);
      THROW_ASSERT(M_Rdata_ram_port, "M_Rdata_ram port is missing");
      bitsize = M_Rdata_ram_port->get_typeRef()->size * M_Rdata_ram_port->get_typeRef()->vector_size;
      unsigned int M_Rdata_ram_port_n_ports =
          M_Rdata_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(M_Rdata_ram_port)->get_ports_size() : 1;

      const auto Sout_Rdata_ram_port = mod->find_member("Sout_Rdata_ram", port_o_K, cir);
      const auto Sin_Rdata_ram_port = mod->find_member("Sin_Rdata_ram", port_o_K, cir);
      if(Sin_Rdata_ram_port)
      {
         for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
         {
            const auto mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
            std::string post_slice2;
            if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            {
               post_slice2 = "[" + STR((i + 1) * bitsize - 1) + ":" + STR(i * bitsize) + "]";
            }
            else
            {
               post_slice2 = "";
            }

            cond_load(Mout_addr_ram_bitsize, post_slice2, "Sin_Rdata_ram", i, STR(bitsize) + "'b0", mem_aggregate);
         }
         THROW_ASSERT(Sout_Rdata_ram_port, "Sout_Rdata_ram port is missing");
         writer->write("assign M_Rdata_ram = Sout_Rdata_ram;\n");
      }
      else if(Sout_Rdata_ram_port)
      {
         for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
         {
            const auto mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
            std::string post_slice;
            if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            {
               post_slice = "[" + STR((i + 1) * bitsize - 1) + ":" + STR(i * bitsize) + "]";
            }
            else
            {
               post_slice = "";
            }
            if(memory_allocation_policy == MemoryAllocation_Policy::ALL_BRAM ||
               memory_allocation_policy == MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
            {
               cond_load(Mout_addr_ram_bitsize, post_slice, "M_Rdata_ram_delayed_temporary", i, STR(bitsize) + "'b0",
                         mem_aggregate);
               writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
               writer->write("begin");
               writer->write(STR(STD_OPENING_CHAR) + "\n");
               writer->write("for (_i_=0; _i_<`MEM_DELAY_READ-1; _i_=_i_+1)");
               writer->write(STR(STD_OPENING_CHAR) + "\n");
               writer->write("if(_i_ == `MEM_DELAY_READ-2)");
               writer->write(STR(STD_OPENING_CHAR) + "\n");
               writer->write("M_Rdata_ram_delayed[_i_]" + post_slice + " <= M_Rdata_ram_delayed_temporary" +
                             post_slice + ";");
               writer->write(STR(STD_CLOSING_CHAR) + "\n");
               writer->write("else");
               writer->write(STR(STD_OPENING_CHAR) + "\n");
               writer->write("M_Rdata_ram_delayed[_i_]" + post_slice + " <= M_Rdata_ram_delayed[_i_+1]" + post_slice +
                             ";");
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write(STR(STD_CLOSING_CHAR) + "\n");
               writer->write(STR(STD_CLOSING_CHAR) + "\n");
               writer->write("end\n");
               writer->write("assign M_Rdata_ram" + post_slice + " = M_Rdata_ram_delayed[0]" + post_slice + "|" +
                             "Sout_Rdata_ram" + post_slice + " ;\n\n");
            }
            else
            {
               cond_load(Mout_addr_ram_bitsize, post_slice, "M_Rdata_ram", i, "Sout_Rdata_ram" + post_slice,
                         mem_aggregate);
            }
         }
      }
      else
      {
         for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
         {
            const auto mem_aggregate = memory_aggregate_slices(i, bitsize, Mout_addr_ram_bitsize);
            std::string post_slice;
            if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            {
               post_slice = "[" + STR((i + 1) * bitsize - 1) + ":" + STR(i * bitsize) + "]";
            }
            else
            {
               post_slice = "";
            }
            if(memory_allocation_policy == MemoryAllocation_Policy::ALL_BRAM ||
               memory_allocation_policy == MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
            {
               cond_load(Mout_addr_ram_bitsize, post_slice, "M_Rdata_ram_delayed_temporary", i, STR(bitsize) + "'b0",
                         mem_aggregate);
               writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
               writer->write("begin");
               writer->write(STR(STD_OPENING_CHAR) + "\n");
               writer->write("for (_i_=0; _i_<`MEM_DELAY_READ-1; _i_=_i_+1)");
               writer->write(STR(STD_OPENING_CHAR) + "\n");
               writer->write("if(_i_ == `MEM_DELAY_READ-2)");
               writer->write(STR(STD_OPENING_CHAR) + "\n");
               writer->write("M_Rdata_ram_delayed[_i_]" + post_slice + " <= M_Rdata_ram_delayed_temporary" +
                             post_slice + ";");
               writer->write(STR(STD_CLOSING_CHAR) + "\n");
               writer->write("else");
               writer->write(STR(STD_OPENING_CHAR) + "\n");
               writer->write("M_Rdata_ram_delayed[_i_]" + post_slice + " <= M_Rdata_ram_delayed[_i_+1]" + post_slice +
                             ";");
               writer->write(STR(STD_CLOSING_CHAR));
               writer->write(STR(STD_CLOSING_CHAR) + "\n");
               writer->write(STR(STD_CLOSING_CHAR) + "\n");
               writer->write("end\n");
               writer->write("assign M_Rdata_ram" + post_slice + " = M_Rdata_ram_delayed[0]" + post_slice + ";\n\n");
            }
            else
            {
               cond_load(Mout_addr_ram_bitsize, post_slice, "M_Rdata_ram", i, STR(bitsize) + "'b0", mem_aggregate);
            }
         }
      }
      const auto Sin_Wdata_ram_port = mod->find_member("S_Wdata_ram", port_o_K, cir);
      if(Sin_Wdata_ram_port)
      {
         writer->write("assign S_Wdata_ram = Mout_Wdata_ram;\n");
      }

      writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
      {
         std::string post_slice1;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
         {
            post_slice1 = "[" + STR(i) + "]";
         }

         writer->write("if ((Mout_oe_ram" + post_slice1 + "===1'b1 && base_addr <= Mout_addr_ram[" +
                       STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + STR(i * Mout_addr_ram_bitsize) +
                       "] && Mout_addr_ram[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                       STR(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)))\n");
         writer->write("begin");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("if (reg_DataReady[" + STR(i) + "] >= 0 && reg_DataReady[" + STR(i) + "] < `MEM_DELAY_READ-1)");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("reg_DataReady[" + STR(i) + "] <= 1 + reg_DataReady[" + STR(i) + "];");
         writer->write(STR(STD_CLOSING_CHAR) + "\n");
         writer->write("else");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("reg_DataReady[" + STR(i) + "] <= 0;\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n\n");

         writer->write("else if ((Mout_we_ram" + post_slice1 + "===1'b1 && base_addr <= Mout_addr_ram[" +
                       STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + STR(i * Mout_addr_ram_bitsize) +
                       "] && Mout_addr_ram[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                       STR(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)))\n");
         writer->write("begin");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("if (reg_DataReady[" + STR(i) + "] >= 0 && reg_DataReady[" + STR(i) + "] < `MEM_DELAY_WRITE-1)");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("reg_DataReady[" + STR(i) + "] <= 1 + reg_DataReady[" + STR(i) + "];\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("else");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("reg_DataReady[" + STR(i) + "] <= 0;\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n\n");

         writer->write("else");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("reg_DataReady[" + STR(i) + "] <= 0;");
         writer->write(STR(STD_CLOSING_CHAR) + "\n");
      }
      writer->write(STR(STD_CLOSING_CHAR) + "\n");
      writer->write("end\n\n");

      const auto Sin_DataRdy_port = mod->find_member("Sin_DataRdy", port_o_K, cir);
      const auto Sout_DataRdy_port = mod->find_member("Sout_DataRdy", port_o_K, cir);
      if(Sin_DataRdy_port)
      {
         for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
         {
            std::string post_slice1;
            if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            {
               post_slice1 = "[" + STR(i) + "]";
            }
            writer->write("assign Sin_DataRdy" + post_slice1 + " = (base_addr <= Mout_addr_ram[" +
                          STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + STR(i * Mout_addr_ram_bitsize) +
                          "] && Mout_addr_ram[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                          STR(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)) && (((reg_DataReady[" + STR(i) +
                          "] == `MEM_DELAY_READ-1)) || (Mout_we_ram" + post_slice1 + "===1'b1 && (reg_DataReady[" +
                          STR(i) + "] == `MEM_DELAY_WRITE-1)));\n");
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
            {
               post_slice1 = "[" + STR(i) + "]";
            }
            writer->write("assign M_DataRdy" + post_slice1 + " = Sout_DataRdy" + post_slice1 +
                          " | ((base_addr <= Mout_addr_ram[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                          STR(i * Mout_addr_ram_bitsize) + "] && Mout_addr_ram[" +
                          STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + STR(i * Mout_addr_ram_bitsize) +
                          "] < (base_addr + MEMSIZE)) && (((reg_DataReady[" + STR(i) +
                          "] == `MEM_DELAY_READ-1)) || (Mout_we_ram" + post_slice1 + "===1'b1 && (reg_DataReady[" +
                          STR(i) + "] == `MEM_DELAY_WRITE-1))));\n");
         }
      }
      else
      {
         for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
         {
            std::string post_slice1;
            if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            {
               post_slice1 = "[" + STR(i) + "]";
            }
            writer->write("assign M_DataRdy" + post_slice1 + " = (base_addr <= Mout_addr_ram[" +
                          STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + STR(i * Mout_addr_ram_bitsize) +
                          "] && Mout_addr_ram[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                          STR(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)) && (((reg_DataReady[" + STR(i) +
                          "] == `MEM_DELAY_READ-1)) || (Mout_we_ram" + post_slice1 + "===1'b1 && (reg_DataReady[" +
                          STR(i) + "] == `MEM_DELAY_WRITE-1)));\n");
         }
      }

      /// Master inputs set up
      structural_objectRef Min_oe_ram_port = mod->find_member("Min_oe_ram", port_o_K, cir);
      if(Min_oe_ram_port)
      {
         writer->write("assign Min_oe_ram = 0;\n");
      }
      structural_objectRef Min_we_ram_port = mod->find_member("Min_we_ram", port_o_K, cir);
      if(Min_we_ram_port)
      {
         writer->write("assign Min_we_ram = 0;\n");
      }
      structural_objectRef Min_addr_ram_port = mod->find_member("Min_addr_ram", port_o_K, cir);
      if(Min_addr_ram_port)
      {
         writer->write("assign Min_addr_ram = 0;\n");
      }
      structural_objectRef Min_data_ram_size_port = mod->find_member("Min_data_ram_size", port_o_K, cir);
      if(Min_data_ram_size_port)
      {
         writer->write("assign Min_data_ram_size = 0;\n");
      }
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
         {
            post_slice1 = "[" + STR(i) + "]";
         }
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
   else
   {
      writer->write_comment("Memory queue update\n");

      structural_objectRef Mout_oe_ram_port = mod->find_member("Mout_oe_ram", port_o_K, cir);
      THROW_ASSERT(Mout_oe_ram_port, "Mout_Wdata_ram port is missing");
      const auto Mout_oe_ram_bitsize =
          Mout_oe_ram_port->get_typeRef()->size *
          (Mout_oe_ram_port->get_typeRef()->vector_size == 0 ? 1 : Mout_oe_ram_port->get_typeRef()->vector_size);
      const auto Mout_oe_ram_n_ports =
          Mout_oe_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(Mout_oe_ram_port)->get_ports_size() : 1;

      structural_objectRef Mout_we_ram_port = mod->find_member("Mout_we_ram", port_o_K, cir);
      THROW_ASSERT(Mout_we_ram_port, "Mout_Wdata_ram port is missing");
      const auto Mout_we_ram_bitsize =
          Mout_we_ram_port->get_typeRef()->size *
          (Mout_we_ram_port->get_typeRef()->vector_size == 0 ? 1 : Mout_we_ram_port->get_typeRef()->vector_size);
      const auto Mout_we_ram_n_ports =
          Mout_we_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(Mout_we_ram_port)->get_ports_size() : 1;

      structural_objectRef Mout_addr_ram_port = mod->find_member("Mout_addr_ram", port_o_K, cir);
      THROW_ASSERT(Mout_addr_ram_port, "Mout_addr_ram port is missing");
      const auto Mout_addr_ram_bitsize =
          Mout_addr_ram_port->get_typeRef()->size * Mout_addr_ram_port->get_typeRef()->vector_size;
      const auto Mout_addr_ram_n_ports = Mout_addr_ram_port->get_kind() == port_vector_o_K ?
                                             GetPointer<port_o>(Mout_addr_ram_port)->get_ports_size() :
                                             1;

      structural_objectRef Mout_Wdata_ram_port = mod->find_member("Mout_Wdata_ram", port_o_K, cir);
      THROW_ASSERT(Mout_Wdata_ram_port, "Mout_Wdata_ram port is missing");
      const auto Mout_Wdata_ram_bitsize =
          Mout_Wdata_ram_port->get_typeRef()->size * Mout_Wdata_ram_port->get_typeRef()->vector_size;
      const auto Mout_Wdata_ram_n_ports = Mout_Wdata_ram_port->get_kind() == port_vector_o_K ?
                                              GetPointer<port_o>(Mout_Wdata_ram_port)->get_ports_size() :
                                              1;

      structural_objectRef Mout_data_ram_size_port = mod->find_member("Mout_data_ram_size", port_o_K, cir);
      THROW_ASSERT(Mout_data_ram_size_port, "Mout_data_ram_size port is missing");

      const auto Mout_data_ram_size_bitsize =
          Mout_data_ram_size_port->get_typeRef()->size * Mout_data_ram_size_port->get_typeRef()->vector_size;
      const auto Mout_data_ram_size_n_ports = Mout_data_ram_size_port->get_kind() == port_vector_o_K ?
                                                  GetPointer<port_o>(Mout_data_ram_size_port)->get_ports_size() :
                                                  1;

      update_memory_queue("Mout_oe_ram", "`MEM_DELAY_READ", Mout_oe_ram_bitsize, Mout_oe_ram_n_ports);
      update_memory_queue("Mout_we_ram", "`MEM_DELAY_WRITE", Mout_we_ram_bitsize, Mout_we_ram_n_ports);
      update_memory_queue("Mout_addr_ram", "`MEM_MAX_DELAY", Mout_addr_ram_bitsize, Mout_addr_ram_n_ports);
      update_memory_queue("Mout_Wdata_ram", "`MEM_DELAY_WRITE", Mout_Wdata_ram_bitsize, Mout_Wdata_ram_n_ports);
      update_memory_queue("Mout_data_ram_size", "`MEM_MAX_DELAY", Mout_data_ram_size_bitsize,
                          Mout_data_ram_size_n_ports);

      // compute mask used for writing on the memory
      for(unsigned int j = 0; j < Mout_oe_ram_n_ports; ++j)
      {
         writer->write("always @(*)\n");
         writer->write("begin");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("if(Mout_we_ram_queue_curr[" + STR(Mout_we_ram_bitsize * Mout_we_ram_n_ports) +
                       "*(`MEM_DELAY_WRITE-1) + " + STR(j * Mout_we_ram_bitsize) + "] === 1'b1)\n");
         writer->write("begin");
         writer->write(STR(STD_OPENING_CHAR) + "\n");

         std::string mask_string = "(1 << Mout_data_ram_size_queue_curr[" +
                                   STR((j + 1) * Mout_data_ram_size_bitsize - 1) + "+(`MEM_DELAY_WRITE-1)*" +
                                   STR(Mout_data_ram_size_bitsize * Mout_data_ram_size_n_ports) + " : " +
                                   STR((j)*Mout_data_ram_size_bitsize) + "+(`MEM_DELAY_WRITE-1)*" +
                                   STR(Mout_data_ram_size_bitsize * Mout_data_ram_size_n_ports) + "]) -1";

         writer->write("mask[" + STR((j + 1) * Mout_Wdata_ram_bitsize - 1) + ":" + STR((j)*Mout_Wdata_ram_bitsize) +
                       "] = " + mask_string + ";\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
         writer->write("else\n");
         writer->write("begin");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("mask[" + STR((j + 1) * Mout_Wdata_ram_bitsize - 1) + ":" + STR(j * Mout_Wdata_ram_bitsize) +
                       "] = 0;\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n\n");
      }

      // write on the testbench memory
      writer->write_comment("OffChip Memory write\n");
      writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR) + "\n");

      for(unsigned int i = 0; i < Mout_we_ram_n_ports; ++i)
      {
         std::string mem_aggregate = memory_aggregate_slices_queue(i, Mout_Wdata_ram_bitsize, Mout_addr_ram_bitsize,
                                                                   Mout_addr_ram_n_ports, "`MEM_DELAY_WRITE");
         std::string post_slice =
             "[" + STR(i) + "+" + STR(Mout_we_ram_bitsize * Mout_we_ram_n_ports) + "*(`MEM_DELAY_WRITE-1)]";
         writer->write("if (Mout_we_ram_queue_curr" + post_slice +
                       " === 1'b1 && base_addr <= Mout_addr_ram_queue_curr[" +
                       STR((i + 1) * Mout_addr_ram_bitsize - 1) + "+(`MEM_DELAY_WRITE-1)*" +
                       STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + ":" + STR(i * Mout_addr_ram_bitsize) +
                       "+(`MEM_DELAY_WRITE-1)*" + STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) +
                       "] && Mout_addr_ram_queue_curr[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) +
                       "+(`MEM_DELAY_WRITE-1)*" + STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + ":" +
                       STR(i * Mout_addr_ram_bitsize) + "+(`MEM_DELAY_WRITE-1)*" +
                       STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + "] < (base_addr + MEMSIZE))\n");
         writer->write("begin");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         post_slice = "[" + STR((i + 1) * Mout_Wdata_ram_bitsize - 1) + "+(`MEM_DELAY_WRITE-1)*" +
                      STR(Mout_Wdata_ram_bitsize * Mout_Wdata_ram_n_ports) + ":" + STR(i * Mout_Wdata_ram_bitsize) +
                      "+(`MEM_DELAY_WRITE-1)*" + STR(Mout_Wdata_ram_bitsize * Mout_Wdata_ram_n_ports) + "]";
         const auto post_slice_mask =
             "[" + STR((i + 1) * Mout_Wdata_ram_bitsize - 1) + ":" + STR(i * Mout_Wdata_ram_bitsize) + "]";
         writer->write(mem_aggregate + " = (Mout_Wdata_ram_queue_curr" + post_slice + " & mask" + post_slice_mask +
                       ") | (" + mem_aggregate + " & ~(mask" + post_slice_mask + "));\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n");
      }
      writer->write(STR(STD_CLOSING_CHAR));
      writer->write("end\n\n");

      const auto Min_Wdata_ram_port = mod->find_member("Min_Wdata_ram", port_o_K, cir);
      if(Min_Wdata_ram_port)
      {
         writer->write("assign Min_Wdata_ram = 0;\n");
      }

      const auto M_Rdata_ram_port = mod->find_member("M_Rdata_ram", port_o_K, cir);
      THROW_ASSERT(M_Rdata_ram_port, "M_Rdata_ram port is missing");
      const auto M_Rdata_ram_bitsize =
          M_Rdata_ram_port->get_typeRef()->size * M_Rdata_ram_port->get_typeRef()->vector_size;
      const auto M_Rdata_ram_port_n_ports =
          M_Rdata_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(M_Rdata_ram_port)->get_ports_size() : 1;

      // write Rdata_ram (TODO remove specialization for BRAM and update Sout and Sin)
      structural_objectRef Sout_Rdata_ram_port = mod->find_member("Sout_Rdata_ram", port_o_K, cir);
      structural_objectRef Sin_Rdata_ram_port = mod->find_member("Sin_Rdata_ram", port_o_K, cir);
      if(Sin_Rdata_ram_port)
      {
         for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
         {
            std::string mem_aggregate = memory_aggregate_slices(i, M_Rdata_ram_bitsize, Mout_addr_ram_bitsize);
            std::string post_slice = "";
            if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            {
               post_slice = "[" + STR((i + 1) * M_Rdata_ram_bitsize - 1) + ":" + STR(i * M_Rdata_ram_bitsize) + "]";
            }
            cond_load(Mout_addr_ram_bitsize, post_slice, "Sin_Rdata_ram", i, STR(M_Rdata_ram_bitsize) + "'b0",
                      mem_aggregate);
         }
         THROW_ASSERT(Sout_Rdata_ram_port, "Sout_Rdata_ram port is missing");
         writer->write("assign M_Rdata_ram = Sout_Rdata_ram;\n");
      }
      else
      {
         for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
         {
            std::string mem_aggregate = memory_aggregate_slices_queue(i, M_Rdata_ram_bitsize, Mout_addr_ram_bitsize,
                                                                      Mout_addr_ram_n_ports, "`MEM_DELAY_READ");
            std::string post_slice = "";
            if(M_Rdata_ram_port->get_kind() == port_vector_o_K)
            {
               post_slice = "[" + STR((i + 1) * M_Rdata_ram_bitsize - 1) + ":" + STR(i * M_Rdata_ram_bitsize) + "]";
            }
            writer->write("always @(*)\n");
            writer->write("begin");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            cond_load_from_queue(Mout_addr_ram_bitsize, Mout_addr_ram_n_ports, "`MEM_DELAY_READ", post_slice,
                                 "M_Rdata_ram_temp", i, STR(M_Rdata_ram_bitsize) + "'b0",
                                 STR(M_Rdata_ram_bitsize) + "'bX", mem_aggregate);
            writer->write(STR(STD_CLOSING_CHAR));
            writer->write("end\n");
            if(Sout_Rdata_ram_port)
            {
               writer->write("assign M_Rdata_ram" + post_slice + " = M_Rdata_ram_temp" + post_slice +
                             " | Sout_Rdata_ram" + post_slice + ";\n\n");
            }
            else
            {
               writer->write("assign M_Rdata_ram" + post_slice + " = M_Rdata_ram_temp" + post_slice + ";\n\n");
            }
         }
      }

      structural_objectRef Sin_Wdata_ram_port = mod->find_member("S_Wdata_ram", port_o_K, cir);
      if(Sin_Wdata_ram_port)
      {
         writer->write("assign S_Wdata_ram = Mout_Wdata_ram;\n");
      }

      // write DataRdy (TODO remove specialization for BRAM and update Sout and Sin)
      structural_objectRef Sin_DataRdy_port = mod->find_member("Sin_DataRdy", port_o_K, cir);
      structural_objectRef Sout_DataRdy_port = mod->find_member("Sout_DataRdy", port_o_K, cir);
      if(Sin_DataRdy_port)
      {
         for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
         {
            std::string post_slice;
            if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            {
               post_slice = "[" + STR(i) + "]";
            }
            writer->write("assign Sin_DataRdy" + post_slice + " = (base_addr <= Mout_addr_ram[" +
                          STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" + STR(i * Mout_addr_ram_bitsize) +
                          "] && Mout_addr_ram[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                          STR(i * Mout_addr_ram_bitsize) + "] < (base_addr + MEMSIZE)) && (((reg_DataReady[" + STR(i) +
                          "] == `MEM_DELAY_READ-1)) || (Mout_we_ram" + post_slice + "===1'b1 && (reg_DataReady[" +
                          STR(i) + "] == `MEM_DELAY_WRITE-1)));\n");
         }
         THROW_ASSERT(Sout_DataRdy_port, "Sout_DataRdy port is missing");
         writer->write("assign M_DataRdy = Sout_DataRdy;\n");
      }
      else
      {
         for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports; ++i)
         {
            std::string post_slice, post_slice2;
            if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
            {
               post_slice = "[" + STR(i) + "]";
            }
            writer->write("always @(*)\n");
            writer->write("begin\n");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("M_DataRdy_temp" + post_slice + " =0;");
            writer->write("if(Mout_we_ram_queue_curr[" + STR(Mout_we_ram_bitsize * Mout_we_ram_n_ports) +
                          "*(`MEM_DELAY_WRITE-1) + " + STR(i * Mout_we_ram_bitsize) + "] === 1'b1)\n");
            writer->write("begin");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("M_DataRdy_temp" + post_slice + " = ");
            writer->write("Mout_addr_ram_queue_curr[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) +
                          "+(`MEM_DELAY_WRITE-1)*" + STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + ":" +
                          STR(i * Mout_addr_ram_bitsize) + "+(`MEM_DELAY_WRITE-1)*" +
                          STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + "] >= base_addr;");
            writer->write(STR(STD_CLOSING_CHAR) + "\n");
            writer->write("end\n");
            writer->write("else if(Mout_oe_ram_queue_curr[" + STR(Mout_oe_ram_bitsize * Mout_oe_ram_n_ports) +
                          "*(`MEM_DELAY_READ-1) + " + STR(i * Mout_oe_ram_bitsize) + "] === 1'b1)\n");
            writer->write("begin");
            writer->write(STR(STD_OPENING_CHAR) + "\n");
            writer->write("M_DataRdy_temp" + post_slice + " = ");
            writer->write("Mout_addr_ram_queue_curr[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) +
                          "+(`MEM_DELAY_WRITE-1)*" + STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + ":" +
                          STR(i * Mout_addr_ram_bitsize) + "+(`MEM_DELAY_WRITE-1)*" +
                          STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + "] >= base_addr;");
            writer->write(STR(STD_CLOSING_CHAR) + "\n");
            writer->write("end");
            writer->write(STR(STD_CLOSING_CHAR) + "\n");
            writer->write("end\n\n");
         }
         if(Sout_DataRdy_port)
         {
            writer->write("assign M_DataRdy = M_DataRdy_temp | Sout_DataRdy;\n\n");
         }
         else
         {
            writer->write("assign M_DataRdy = M_DataRdy_temp;\n\n");
         }
      }

      /// Master inputs set up
      structural_objectRef Min_oe_ram_port = mod->find_member("Min_oe_ram", port_o_K, cir);
      if(Min_oe_ram_port)
      {
         writer->write("assign Min_oe_ram = 0;\n");
      }
      structural_objectRef Min_we_ram_port = mod->find_member("Min_we_ram", port_o_K, cir);
      if(Min_we_ram_port)
      {
         writer->write("assign Min_we_ram = 0;\n");
      }
      structural_objectRef Min_addr_ram_port = mod->find_member("Min_addr_ram", port_o_K, cir);
      if(Min_addr_ram_port)
      {
         writer->write("assign Min_addr_ram = 0;\n");
      }
      structural_objectRef Min_data_ram_size_port = mod->find_member("Min_data_ram_size", port_o_K, cir);
      if(Min_data_ram_size_port)
      {
         writer->write("assign Min_data_ram_size = 0;\n");
      }

      /// Slave inputs connections
      THROW_ASSERT(Mout_oe_ram_port, "Mout_oe_ram port is missing");
      structural_objectRef S_oe_ram_port = mod->find_member("S_oe_ram", port_o_K, cir);
      if(S_oe_ram_port)
      {
         writer->write("assign S_oe_ram = Mout_oe_ram;\n");
      }
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

      // check not both oe and we enabled at the same time.
      writer->write("always @(posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
      writer->write("begin");
      writer->write(STR(STD_OPENING_CHAR) + "\n");
      for(unsigned int i = 0; i < M_Rdata_ram_port_n_ports && i < Mout_Wdata_ram_n_ports; ++i)
      {
         std::string post_slice;
         if(Mout_addr_ram_port->get_kind() == port_vector_o_K)
         {
            post_slice = "[" + STR(i) + "]";
         }
         writer->write("if (Mout_we_ram" + post_slice + "===1'b1 && Mout_oe_ram" + post_slice + "===1'b1)\n");
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
}

void MinimalInterfaceTestbench::write_interface_handler() const
{
   if(mod->get_in_port_size())
   {
      bool firstRValid = true;
      bool firstWAck = true;
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         const auto portInst = mod->get_in_port(i);
         const auto InterfaceType = GetPointer<port_o>(portInst)->get_port_interface();
         if(InterfaceType != port_o::port_interface::PI_DEFAULT)
         {
            /// check if you have both _vld and _ack
            bool have_both = false;
            if(InterfaceType == port_o::port_interface::PI_RVALID || InterfaceType == port_o::port_interface::PI_WACK)
            {
               const auto curr = InterfaceType == port_o::port_interface::PI_RVALID ? "_vld" : "_ack";
               const auto other = InterfaceType == port_o::port_interface::PI_RVALID ? "_ack" : "_vld";
               const auto other_name = boost::replace_last_copy(portInst->get_id(), curr, other);
               const auto port_other = mod->find_member(other_name, port_o_K, cir);
               have_both = port_other != nullptr;
            }

            if(InterfaceType == port_o::port_interface::PI_RVALID || InterfaceType == port_o::port_interface::PI_WACK ||
               InterfaceType == port_o::port_interface::PI_EMPTY_N ||
               InterfaceType == port_o::port_interface::PI_FULL_N ||
               InterfaceType == port_o::port_interface::PI_S_AXIS_TVALID ||
               InterfaceType == port_o::port_interface::PI_M_AXIS_TREADY)
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
               writer->write_comment(port_o::GetString(InterfaceType) + " handler\n");
               writer->write("always @ (posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
               writer->write(STR(STD_OPENING_CHAR));
               writer->write("begin\n");
               if(have_both && InterfaceType == port_o::port_interface::PI_WACK)
               {
                  const auto vld_port = boost::replace_last_copy(portInst->get_id(), "_ack", "_vld");
                  writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) +
                                " <= " + HDL_manager::convert_to_identifier(writer.get(), vld_port) + " & !" +
                                HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) + ";\n");
               }
               else
               {
                  if(!have_both && InterfaceType == port_o::port_interface::PI_RVALID)
                  {
                     writer->write("if(__state == 3 && __vld_port_state == 0)\n");
                  }
                  else
                  {
                     writer->write("if(__state == 3)\n");
                  }
                  writer->write(STR(STD_OPENING_CHAR));
                  writer->write("begin\n");

                  if(InterfaceType == port_o::port_interface::PI_WACK)
                  {
                     writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) +
                                   " <= __ack_port_state < 1 ? 1'b0 : 1'b1;\n");
                     writer->write("__ack_port_state <= __ack_port_state + 1;");
                  }
                  else if(InterfaceType == port_o::port_interface::PI_FULL_N)
                  {
                     const auto parm_name = boost::replace_last_copy(portInst->get_id(), "_full_n", "");
                     THROW_ASSERT(HLSMgr->RSim->test_vectors.size() &&
                                      HLSMgr->RSim->test_vectors.front().count(parm_name),
                                  "Unable to find initialization for FIFO parameter " + parm_name);
                     const auto& test_v = HLSMgr->RSim->test_vectors.front().at(parm_name);
                     const auto fifo_depth = SplitString(test_v, ",").size();
                     const auto write_counter =
                         "fifo_counter_" + HDL_manager::convert_to_identifier(writer.get(), parm_name + "_din");
                     writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) +
                                   " <= " + write_counter + " < " + STR(fifo_depth) + " ? 1'b1 : 1'b0;\n");
                  }
                  else if(InterfaceType == port_o::port_interface::PI_EMPTY_N)
                  {
                     const auto parm_name = boost::replace_last_copy(portInst->get_id(), "_empty_n", "");
                     THROW_ASSERT(HLSMgr->RSim->test_vectors.size() &&
                                      HLSMgr->RSim->test_vectors.front().count(parm_name),
                                  "Unable to find initialization for FIFO parameter " + parm_name);
                     const auto& test_v = HLSMgr->RSim->test_vectors.front().at(parm_name);
                     const auto fifo_depth = SplitString(test_v, ",").size();
                     const auto read_counter =
                         "fifo_counter_" + HDL_manager::convert_to_identifier(writer.get(), parm_name + "_dout");
                     writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) +
                                   " <= " + read_counter + " < " + STR(fifo_depth) + " ? 1'b1 : 1'b0;\n");
                  }
                  else if(InterfaceType == port_o::port_interface::PI_M_AXIS_TREADY)
                  {
                     const auto parm_name = boost::replace_last_copy(
                         boost::replace_first_copy(portInst->get_id(), "m_axis_", ""), "_TREADY", "");
                     THROW_ASSERT(HLSMgr->RSim->test_vectors.size() &&
                                      HLSMgr->RSim->test_vectors.front().count(parm_name),
                                  "Unable to find initialization for FIFO parameter " + parm_name);
                     const auto& test_v = HLSMgr->RSim->test_vectors.front().at(parm_name);
                     const auto fifo_depth = SplitString(test_v, ",").size();
                     const auto read_counter = "fifo_counter_" + HDL_manager::convert_to_identifier(
                                                                     writer.get(), "m_axis_" + parm_name + "_TDATA");
                     writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) +
                                   " <= " + read_counter + " < " + STR(fifo_depth) + " ? 1'b1 : 1'b0;\n");
                  }
                  else if(InterfaceType == port_o::port_interface::PI_S_AXIS_TVALID)
                  {
                     const auto parm_name = boost::replace_last_copy(
                         boost::replace_first_copy(portInst->get_id(), "s_axis_", ""), "_TVALID", "");
                     THROW_ASSERT(HLSMgr->RSim->test_vectors.size() &&
                                      HLSMgr->RSim->test_vectors.front().count(parm_name),
                                  "Unable to find initialization for FIFO parameter " + parm_name);
                     const auto& test_v = HLSMgr->RSim->test_vectors.front().at(parm_name);
                     const auto fifo_depth = SplitString(test_v, ",").size();
                     const auto read_counter = "fifo_counter_" + HDL_manager::convert_to_identifier(
                                                                     writer.get(), "s_axis_" + parm_name + "_TDATA");
                     writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) +
                                   " <= " + read_counter + " < " + STR(fifo_depth) + " ? 1'b1 : 1'b0;\n");
                  }
                  else
                  {
                     writer->write(HDL_manager::convert_to_identifier(writer.get(), portInst->get_id()) +
                                   " <= 1'b1;\n");
                  }
                  if(!have_both && InterfaceType == port_o::port_interface::PI_RVALID)
                  {
                     writer->write("__vld_port_state <= 1;");
                  }
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

            if(InterfaceType == port_o::port_interface::PI_FDOUT ||
               InterfaceType == port_o::port_interface::PI_S_AXIS_TDATA)
            {
               const auto bitsize = local_port_size(portInst);
               const auto port_id = portInst->get_id();
               std::string valid_suffix, par_name;
               if(InterfaceType == port_o::port_interface::PI_FDOUT)
               {
                  valid_suffix = "_read";
                  par_name = port_id.substr(0, port_id.size() - sizeof("_dout") + 1U);
               }
               else if(InterfaceType == port_o::port_interface::PI_S_AXIS_TDATA)
               {
                  valid_suffix = "_TREADY";
                  par_name = port_id.substr(0, port_id.size() - sizeof("_TDATA") + 1U);
               }
               else
               {
                  THROW_UNREACHABLE("unexpected signal name");
               }
               write_read_fifo_manager(par_name, port_id, bitsize, valid_suffix);
            }
         }
      }
   }
}

void MinimalInterfaceTestbench::write_slave_initializations(bool with_memory) const
{
   if(with_memory)
   {
      return;
   }
   /// write slave signals initialization
   if(mod->get_in_port_size())
   {
      bool print_header_comment = true;
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         const auto portInst = mod->get_in_port(i);
         if(GetPointer<port_o>(portInst)->get_is_memory())
         {
            if(GetPointer<port_o>(portInst)->get_id().find('S') == 0)
            {
               if(print_header_comment)
               {
                  print_header_comment = false;
                  writer->write_comment("Slave signals initialization\n");
               }
               writer->write("assign " +
                             HDL_manager::convert_to_identifier(writer.get(), mod->get_in_port(i)->get_id()) +
                             " = 0;\n");
            }
         }
      }
      if(!print_header_comment)
      {
         writer->write("\n");
      }
   }
}

void MinimalInterfaceTestbench::write_input_signal_declaration(const tree_managerConstRef TreeM,
                                                               bool& with_memory) const
{
   /// write input signals declaration
   if(mod->get_in_port_size())
   {
      writer->write_comment("INPUT SIGNALS\n");
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         const auto portInst = mod->get_in_port(i);
         const auto port_if = GetPointer<port_o>(portInst)->get_port_interface();
         auto port_name = mod->get_in_port(i)->get_id();
         if(GetPointer<port_o>(portInst)->get_is_memory())
         {
            if(GetPointer<port_o>(portInst)->get_id().find('M') == 0)
            {
               with_memory = true;
            }
            writer->write("wire ");
         }
         else if(parameters->isOption(OPT_clock_name) &&
                 GetPointer<port_o>(portInst)->get_id() == parameters->getOption<std::string>(OPT_clock_name))
         {
            writer->write("input ");
         }
         else if(GetPointer<port_o>(portInst)->get_id() == CLOCK_PORT_NAME)
         {
            writer->write("input ");
         }
         else
         {
            writer->write("reg ");
         }
         writer->write(writer->type_converter(portInst->get_typeRef()) + writer->type_converter_size(portInst));

         if(parameters->isOption(OPT_clock_name) &&
            GetPointer<port_o>(portInst)->get_id() == parameters->getOption<std::string>(OPT_clock_name))
         {
            port_name = CLOCK_PORT_NAME;
         }
         else if(parameters->isOption(OPT_reset_name) &&
                 GetPointer<port_o>(portInst)->get_id() == parameters->getOption<std::string>(OPT_reset_name))
         {
            port_name = RESET_PORT_NAME;
         }
         else if(parameters->isOption(OPT_start_name) &&
                 GetPointer<port_o>(portInst)->get_id() == parameters->getOption<std::string>(OPT_start_name))
         {
            port_name = START_PORT_NAME;
         }
         /* Add next_* to any input AXI signals */
         if(port_if >= port_o::port_interface::M_AXI_AWVALID && port_if <= port_o::port_interface::M_AXI_BUSER)
         {
            const auto ptName = HDL_manager::convert_to_identifier(writer.get(), port_name);
            writer->write(ptName + ", next_" + ptName + ";\n");
         }
         else
         {
            writer->write(HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
         }
         if(portInst->get_typeRef()->treenode > 0 &&
            tree_helper::IsPointerType(TreeM->CGetTreeReindex(portInst->get_typeRef()->treenode)))
         {
            auto pt_node = tree_helper::CGetPointedType(
                tree_helper::CGetType(TreeM->CGetTreeReindex(portInst->get_typeRef()->treenode)));
            while(GetPointer<const array_type>(GET_CONST_NODE(pt_node)))
            {
               pt_node = GetPointer<const array_type>(GET_CONST_NODE(pt_node))->elts;
            }

            if(tree_helper::IsRealType(pt_node))
            {
               auto bitsize = tree_helper::Size(pt_node);
               writer->write("reg [" + STR(bitsize - 1) + ":0] ex_" + portInst->get_id() + ";\n");
            }
            else
            {
               writer->write("reg [7:0] ex_" + portInst->get_id() + ";\n");
            }
         }
         if(port_if == port_o::port_interface::PI_FDOUT || port_if == port_o::port_interface::PI_S_AXIS_TDATA)
         {
            writer->write("integer fifo_counter_" + HDL_manager::convert_to_identifier(writer.get(), port_name) +
                          ";\n");
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
         const auto portInst = mod->get_out_port(i);
         const auto port_if = GetPointer<port_o>(portInst)->get_port_interface();
         auto port_name = portInst->get_id();
         writer->write("wire " + writer->type_converter(portInst->get_typeRef()) +
                       writer->type_converter_size(portInst));
         if(parameters->isOption(OPT_done_name) && port_name == parameters->getOption<std::string>(OPT_done_name))
         {
            port_name = DONE_PORT_NAME;
         }
         writer->write(HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
         if(port_name == RETURN_PORT_NAME)
         {
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) +
                          writer->type_converter_size(portInst));
            writer->write("ex_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) +
                          writer->type_converter_size(portInst));
            writer->write("registered_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
         }
         if(port_if == port_o::port_interface::PI_WNONE)
         {
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) +
                          writer->type_converter_size(portInst));
            writer->write("ex_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) +
                          writer->type_converter_size(portInst));
            writer->write("registered_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
         }
         else if(port_if == port_o::port_interface::PI_DOUT)
         {
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) +
                          writer->type_converter_size(portInst));
            writer->write("ex_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
         }
         else if(port_if == port_o::port_interface::PI_FDIN || port_if == port_o::port_interface::PI_M_AXIS_TDATA)
         {
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) +
                          writer->type_converter_size(portInst));
            writer->write("ex_" + HDL_manager::convert_to_identifier(writer.get(), port_name) + ";\n");
            writer->write("reg " + writer->type_converter(portInst->get_typeRef()) +
                          writer->type_converter_size(portInst));
            writer->write("registered_" + HDL_manager::convert_to_identifier(writer.get(), port_name) +
                          " [0:MEMSIZE-1];\n");
            writer->write("integer fifo_counter_" + HDL_manager::convert_to_identifier(writer.get(), port_name) +
                          ";\n");
         }
      }
      writer->write("\n");
   }
}

void MinimalInterfaceTestbench::write_signal_queue(std::string port_name, std::string delay_type) const
{
   structural_objectRef port = mod->find_member(port_name, port_o_K, cir);
   const auto bitsize =
       port->get_typeRef()->size * (port->get_typeRef()->vector_size == 0 ? 1 : port->get_typeRef()->vector_size);
   const auto n_ports = port->get_kind() == port_vector_o_K ? GetPointer<port_o>(port)->get_ports_size() : 1;
   writer->write("reg [" + STR(bitsize * n_ports) + "*" + delay_type + "-1:0] " + port_name + "_queue_next;\n");
   writer->write("reg [" + STR(bitsize * n_ports) + "*" + delay_type + "-1:0] " + port_name + "_queue_curr;\n");
}

void MinimalInterfaceTestbench::write_signals(const tree_managerConstRef TreeM, bool& withMemory, bool&) const
{
   const auto memory_allocation_policy = parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy);
   write_input_signal_declaration(TreeM, withMemory);
   write_output_signal_declaration();
   // write parameters
   if(withMemory)
   {
      const auto M_Rdata_ram_port = mod->find_member("M_Rdata_ram", port_o_K, cir);
      const auto M_Rdata_ram_bitsize =
          M_Rdata_ram_port->get_typeRef()->size *
          (M_Rdata_ram_port->get_typeRef()->vector_size == 0 ? 1 : M_Rdata_ram_port->get_typeRef()->vector_size);
      const auto M_Rdata_ram_n_ports =
          M_Rdata_ram_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(M_Rdata_ram_port)->get_ports_size() : 1;

      const auto M_DataRdy_port = mod->find_member("M_DataRdy", port_o_K, cir);
      const auto M_DataRdy_bitsize =
          M_DataRdy_port->get_typeRef()->size *
          (M_DataRdy_port->get_typeRef()->vector_size == 0 ? 1 : M_DataRdy_port->get_typeRef()->vector_size);
      const auto M_DataRdy_n_ports =
          M_DataRdy_port->get_kind() == port_vector_o_K ? GetPointer<port_o>(M_DataRdy_port)->get_ports_size() : 1;

      // TO DO remove when upgrading svelto
      if(parameters->isOption(OPT_parse_pragma) && parameters->getOption<bool>(OPT_parse_pragma))
      {
         writer->write("reg signed [31:0] reg_DataReady[" + STR(M_Rdata_ram_n_ports - 1) + ":0];\n");
         writer->write("reg [" + STR(M_Rdata_ram_bitsize * M_Rdata_ram_n_ports - 1) + ":0] mask;\n");
      }
      else
      {
         writer->write("reg signed [31:0] reg_DataReady[" + STR(M_Rdata_ram_n_ports - 1) + ":0];\n");
         writer->write("reg [" + STR(M_Rdata_ram_bitsize * M_Rdata_ram_n_ports - 1) + ":0] mask;\n");
         writer->write("reg [" + STR(M_Rdata_ram_bitsize * M_Rdata_ram_n_ports - 1) + ":0] M_Rdata_ram_temp;\n");
         writer->write("reg [" + STR(M_DataRdy_bitsize * M_DataRdy_n_ports - 1) + ":0] M_DataRdy_temp;\n\n");

         write_signal_queue("Mout_oe_ram", "`MEM_DELAY_READ");
         write_signal_queue("Mout_we_ram", "`MEM_DELAY_WRITE");
         write_signal_queue("Mout_addr_ram", "`MEM_MAX_DELAY");
         write_signal_queue("Mout_Wdata_ram", "`MEM_DELAY_WRITE");
         write_signal_queue("Mout_data_ram_size", "`MEM_MAX_DELAY");

         writer->write("//initialization of memory queue signals\n");
         writer->write("initial\n");
         writer->write("begin");
         writer->write(STR(STD_OPENING_CHAR) + "\n");
         writer->write("Mout_oe_ram_queue_next = 0;\n");
         writer->write("Mout_oe_ram_queue_curr = 0;\n");
         writer->write("Mout_we_ram_queue_next = 0;\n");
         writer->write("Mout_we_ram_queue_curr = 0;\n");
         writer->write("Mout_addr_ram_queue_next = 0;\n");
         writer->write("Mout_addr_ram_queue_curr = 0;\n");
         writer->write("Mout_Wdata_ram_queue_next = 0;\n");
         writer->write("Mout_Wdata_ram_queue_curr = 0;\n");
         writer->write("Mout_data_ram_size_queue_next = 0;\n");
         writer->write("Mout_data_ram_size_queue_curr = 0;\n");
         writer->write("M_DataRdy_temp = 0;\n");
         writer->write("M_Rdata_ram_temp = 0;\n");
         writer->write("base_addr = 0;\n");
         writer->write(STR(STD_CLOSING_CHAR));
         writer->write("end\n\n");
      }

      if(memory_allocation_policy == MemoryAllocation_Policy::ALL_BRAM or
         memory_allocation_policy == MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         writer->write("wire [" + STR(M_Rdata_ram_bitsize * M_Rdata_ram_n_ports - 1) +
                       ":0] M_Rdata_ram_delayed_temporary;\n");
         writer->write("reg [" + STR(M_Rdata_ram_bitsize * M_Rdata_ram_n_ports - 1) +
                       ":0] M_Rdata_ram_delayed [`MEM_DELAY_READ-2:0];\n\n");
      }
   }
}

void MinimalInterfaceTestbench::read_input_value_from_file_RNONE(const std::string& input_name, bool& first_valid_input,
                                                                 unsigned long long bitsize) const
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
         writer->write(R"(_r_ = $fscanf(file,"%b\n", paddr)" + input_name + "); ");
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
         std::string mem_aggregate = "{";
         for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
         {
            if(bitsize_index)
            {
               mem_aggregate += ", ";
            }
            mem_aggregate += "_bambu_testbench_mem_[paddr" + input_name + " + " +
                             STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]";
         }
         mem_aggregate += "}";
         writer->write(input_name + " = " + mem_aggregate + ";\n");
         size_t escaped_pos = input_name.find('\\');
         std::string nonescaped_name = input_name;
         if(escaped_pos != std::string::npos)
         {
            nonescaped_name.erase(std::remove(nonescaped_name.begin(), nonescaped_name.end(), '\\'),
                                  nonescaped_name.end());
         }
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

void MinimalInterfaceTestbench::write_read_fifo_manager(std::string par, const std::string& pi_dout_name,
                                                        unsigned long long bitsize, std::string valid_suffix) const
{
   writer->write("\n");
   writer->write_comment("Manage fifo signals for " + pi_dout_name +
                         " --------------------------------------------------------------\n");
   writer->write("always @ (posedge " + std::string(CLOCK_PORT_NAME) + ")\n");
   writer->write(STR(STD_OPENING_CHAR));
   writer->write("begin\n");
   writer->write("if(" + par + valid_suffix + " == 1'b1)\n");
   writer->write("begin");
   writer->write(STR(STD_OPENING_CHAR) + "\n");
   writer->write("paddr" + pi_dout_name + " <= paddr" + pi_dout_name + " + " + STR(bitsize / 8) + ";\n");
   writer->write(STR(STD_CLOSING_CHAR) + "end\n");
   std::string mem_aggregate = "{";
   for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
   {
      if(bitsize_index)
      {
         mem_aggregate += ", ";
      }
      mem_aggregate += "_bambu_testbench_mem_[paddr" + pi_dout_name + " + " + STR((bitsize - bitsize_index) / 8 - 1) +
                       " - base_addr]";
   }
   mem_aggregate += "}";
   writer->write(STR(STD_CLOSING_CHAR));
   writer->write("end\n");
   writer->write("always @ (*) " + pi_dout_name + " = " + mem_aggregate + ";\n");
}

void MinimalInterfaceTestbench::write_file_reading_operations() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Write file reading operations");
   /// file reading operations
   // cppcheck-suppress variableScope
   bool first_valid_input = true;
   /// iterate over all interface ports
   const auto& DesignSignature = HLSMgr->RSim->simulationArgSignature;
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
      if(InterfaceType == port_o::port_interface::PI_DEFAULT)
      {
         read_input_value_from_file(input_name, first_valid_input);
      }
      else if(InterfaceType == port_o::port_interface::PI_RNONE)
      {
         const auto bitsize = local_port_size(portInst);
         read_input_value_from_file_RNONE(input_name, first_valid_input, bitsize);
      }
      else if(InterfaceType == port_o::port_interface::PI_WNONE || InterfaceType == port_o::port_interface::PI_DIN ||
              InterfaceType == port_o::port_interface::PI_DOUT || InterfaceType == port_o::port_interface::PI_FDOUT ||
              InterfaceType == port_o::port_interface::PI_S_AXIS_TDATA ||
              InterfaceType == port_o::port_interface::PI_FDIN ||
              InterfaceType == port_o::port_interface::PI_M_AXIS_TDATA)
      {
         read_input_value_from_file("paddr" + input_name, first_valid_input);
      }
      else
      {
         THROW_ERROR("not yet supported port interface for port " + input_name);
      }
   }
   if(!first_valid_input)
   {
      writer->write("_ch_ = $fgetc(file);\n");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written file reading operations");
}

std::string MinimalInterfaceTestbench::memory_aggregate_slices(unsigned int i, unsigned long long int bitsize,
                                                               unsigned long long int Mout_addr_ram_bitsize) const
{
   std::string mem_aggregate = "{";
   for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
   {
      if(bitsize_index)
      {
         mem_aggregate += ", ";
      }
      mem_aggregate += "_bambu_testbench_mem_[Mout_addr_ram[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) + ":" +
                       STR(i * Mout_addr_ram_bitsize) + "] + " + STR((bitsize - bitsize_index) / 8 - 1) +
                       " - base_addr]";
   }
   mem_aggregate += "}";

   return mem_aggregate;
}

std::string MinimalInterfaceTestbench::memory_aggregate_slices_queue(unsigned int i, unsigned long long bitsize,
                                                                     unsigned long long Mout_addr_ram_bitsize,
                                                                     unsigned int Mout_addr_ram_n_ports,
                                                                     const std::string& queue_type) const
{
   std::string mem_aggregate = "{";
   for(unsigned int bitsize_index = 0; bitsize_index < bitsize; bitsize_index = bitsize_index + 8)
   {
      if(bitsize_index)
      {
         mem_aggregate += ", ";
      }
      mem_aggregate += "_bambu_testbench_mem_[Mout_addr_ram_queue_curr[" + STR((i + 1) * Mout_addr_ram_bitsize - 1) +
                       "+(" + queue_type + "-1)*" + STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + ":" +
                       STR(i * Mout_addr_ram_bitsize) + "+(" + queue_type + "-1)*" +
                       STR(Mout_addr_ram_bitsize * Mout_addr_ram_n_ports) + "] + " +
                       STR((bitsize - bitsize_index) / 8 - 1) + " - base_addr]";
   }
   mem_aggregate += "}";

   return mem_aggregate;
}
