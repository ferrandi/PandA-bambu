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
 * @file verilog_writer.cpp
 * @brief Write system verilog provided descriptions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "sv_writer.hpp"

#include "HDL_manager.hpp"

#include "technology_manager.hpp"

#include "NP_functionality.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "structural_objects.hpp"

///. include
#include "Parameter.hpp"

/// Utility include
#include "indented_output_stream.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

void system_verilog_writer::write_NP_functionalities(const structural_objectRef& cir)
{
   auto* mod = GetPointer<module>(cir);
   THROW_ASSERT(mod, "Expected a component object");
   const NP_functionalityRef& np = mod->get_NP_functionality();
   THROW_ASSERT(np, "NP Behavioral description is missing for module: " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)));
   std::string beh_desc = np->get_NP_functionality(NP_functionality::SYSTEM_VERILOG_PROVIDED);
   THROW_ASSERT(beh_desc != "", "SYSTEM VERILOG behavioral description is missing for module: " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)));
   remove_escaped(beh_desc);
   /// manage reset by preprocessing the behavioral description
   if(!parameters->getOption<bool>(OPT_level_reset))
   {
      if(parameters->getOption<std::string>(OPT_sync_reset) == "async")
         boost::replace_all(beh_desc, "1RESET_EDGE", "or negedge " + std::string(RESET_PORT_NAME));
      else
         boost::replace_all(beh_desc, "1RESET_EDGE", "");
      boost::replace_all(beh_desc, "1RESET_VALUE", std::string(RESET_PORT_NAME) + " == 1'b0");
   }
   else
   {
      if(parameters->getOption<std::string>(OPT_sync_reset) == "async")
         boost::replace_all(beh_desc, "1RESET_EDGE", "or posedge " + std::string(RESET_PORT_NAME));
      else
         boost::replace_all(beh_desc, "1RESET_EDGE", "");
      boost::replace_all(beh_desc, "1RESET_VALUE", std::string(RESET_PORT_NAME) + " == 1'b1");
   }
   if(parameters->getOption<bool>(OPT_reg_init_value))
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUE", "=0");
   else
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUE", "");
   indented_output_stream->Append(beh_desc);
}

system_verilog_writer::system_verilog_writer(const ParameterConstRef _parameters) : verilog_writer(_parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(this));
}

system_verilog_writer::~system_verilog_writer() = default;
