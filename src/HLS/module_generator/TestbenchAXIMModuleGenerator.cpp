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
 *              Copyright (C) 2023-2023 Politecnico di Milano
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
 * @file TestbenchAXIMModuleGenerator.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "TestbenchAXIMModuleGenerator.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "structural_manager.hpp"
#include "tree_helper.hpp"

TestbenchAXIMModuleGenerator::TestbenchAXIMModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void TestbenchAXIMModuleGenerator::InternalExec(std::ostream& out, structural_objectRef mod_cir,
                                                unsigned int function_id, vertex /* op_v */,
                                                const HDLWriter_Language language,
                                                const std::vector<ModuleGenerator::parameter>& /* _p */,
                                                const std::vector<ModuleGenerator::parameter>& /* _ports_in */,
                                                const std::vector<ModuleGenerator::parameter>& /* _ports_out */,
                                                const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   if(language != HDLWriter_Language::VERILOG)
   {
      THROW_UNREACHABLE("Unsupported output language");
      return;
   }

   const auto port_prefix = mod_cir->get_id().substr(sizeof("if_") - 1U, std::string::npos);
   std::string np_library = mod_cir->get_id() + " index WRITE_DELAY READ_DELAY MAX_QUEUE_SIZE";
   std::string internal_port_assign;
   const auto add_port_parametric = [&](const std::string& name, port_o::port_direction dir, unsigned port_size) {
      const auto port_name = port_prefix + "_" + name;
      structural_manager::add_port(port_name, dir, mod_cir,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
      np_library += " " + port_name;
      out << "reg ";
      if(port_size)
      {
         out << "[BITSIZE_" << port_name << "-1:0] ";
      }
      out << name << ";\n";
      if(dir == port_o::IN)
      {
         internal_port_assign += "assign " + name + " = " + port_name + ";\n";
      }
      else
      {
         internal_port_assign += "assign " + port_name + " = " + name + ";\n";
      }
   };

   add_port_parametric("AWREADY", port_o::OUT, 0U);
   add_port_parametric("WREADY", port_o::OUT, 0U);
   add_port_parametric("BID", port_o::OUT, 1U);
   add_port_parametric("BRESP", port_o::OUT, 2U);
   add_port_parametric("BUSER", port_o::OUT, 1U);
   add_port_parametric("BVALID", port_o::OUT, 0U);
   add_port_parametric("ARREADY", port_o::OUT, 0U);
   add_port_parametric("RID", port_o::OUT, 1U);
   add_port_parametric("RDATA", port_o::OUT, 1U);
   add_port_parametric("RRESP", port_o::OUT, 2U);
   add_port_parametric("RLAST", port_o::OUT, 0U);
   add_port_parametric("RUSER", port_o::OUT, 1U);
   add_port_parametric("RVALID", port_o::OUT, 0U);

   add_port_parametric("AWID", port_o::IN, 1U);
   add_port_parametric("AWADDR", port_o::IN, 1U);
   add_port_parametric("AWLEN", port_o::IN, 1U);
   add_port_parametric("AWSIZE", port_o::IN, 1U);
   add_port_parametric("AWBURST", port_o::IN, 1U);
   add_port_parametric("AWLOCK", port_o::IN, 1U);
   add_port_parametric("AWCACHE", port_o::IN, 1U);
   add_port_parametric("AWPROT", port_o::IN, 1U);
   add_port_parametric("AWQOS", port_o::IN, 1U);
   add_port_parametric("AWREGION", port_o::IN, 1U);
   add_port_parametric("AWUSER", port_o::IN, 1U);
   add_port_parametric("AWVALID", port_o::IN, 0U);

   add_port_parametric("WID", port_o::IN, 1U);
   add_port_parametric("WDATA", port_o::IN, 1U);
   add_port_parametric("WSTRB", port_o::IN, 1U);
   add_port_parametric("WLAST", port_o::IN, 0U);
   add_port_parametric("WUSER", port_o::IN, 1U);
   add_port_parametric("WVALID", port_o::IN, 0U);

   add_port_parametric("BREADY", port_o::IN, 0U);

   add_port_parametric("ARID", port_o::IN, 1U);
   add_port_parametric("ARADDR", port_o::IN, 1U);
   add_port_parametric("ARLEN", port_o::IN, 1U);
   add_port_parametric("ARSIZE", port_o::IN, 1U);
   add_port_parametric("ARBURST", port_o::IN, 1U);
   add_port_parametric("ARLOCK", port_o::IN, 1U);
   add_port_parametric("ARCACHE", port_o::IN, 1U);
   add_port_parametric("ARPROT", port_o::IN, 1U);
   add_port_parametric("ARQOS", port_o::IN, 1U);
   add_port_parametric("ARREGION", port_o::IN, 1U);
   add_port_parametric("ARUSER", port_o::IN, 1U);
   add_port_parametric("ARVALID", port_o::IN, 0U);

   add_port_parametric("RREADY", port_o::IN, 0U);
   structural_manager::add_NP_functionality(mod_cir, NP_functionality::LIBRARY, np_library);
   out << "parameter BITSIZE_awqueue=BITSIZE_" << port_prefix << ";\n";
}
