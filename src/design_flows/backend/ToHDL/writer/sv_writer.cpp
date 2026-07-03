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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file verilog_writer.cpp
 * @brief Write system verilog provided descriptions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "sv_writer.hpp"

#include "HDL_manager.hpp"
#include "NP_functionality.hpp"
#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "indented_output_stream.hpp"
#include "string_manipulation.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"

void system_verilog_writer::write_NP_functionalities(const structural_objectRef& cir)
{
   auto* mod = GetPointer<module_o>(cir);
   THROW_ASSERT(mod, "Expected a component object");
   const NP_functionalityRef& np = mod->get_NP_functionality();
   THROW_ASSERT(np, "NP Behavioral description is missing for module: " +
                        HDL_manager::convert_to_identifier(GET_TYPE_NAME(cir)));
   std::string beh_desc = np->get_NP_functionality(NP_functionality::SYSTEM_VERILOG_PROVIDED);
   THROW_ASSERT(beh_desc != "", "SYSTEM VERILOG behavioral description is missing for module: " +
                                    HDL_manager::convert_to_identifier(GET_TYPE_NAME(cir)));
   /// manage reset by preprocessing the behavioral description
   if(!parameters->getOption<bool>(OPT_reset_level))
   {
      boost::replace_all(beh_desc, "1RESET_EDGE_FORCE", "or negedge " + std::string(RESET_PORT_NAME));
      if(parameters->getOption<std::string>(OPT_reset_type) == "async")
      {
         boost::replace_all(beh_desc, "1RESET_EDGE", "or negedge " + std::string(RESET_PORT_NAME));
      }
      else
      {
         boost::replace_all(beh_desc, "1RESET_EDGE", "");
      }
      boost::replace_all(beh_desc, "1RESET_VALUE", std::string(RESET_PORT_NAME) + " == 1'b0");
   }
   else
   {
      boost::replace_all(beh_desc, "1RESET_EDGE_FORCE", "or posedge " + std::string(RESET_PORT_NAME));
      if(parameters->getOption<std::string>(OPT_reset_type) == "async")
      {
         boost::replace_all(beh_desc, "1RESET_EDGE", "or posedge " + std::string(RESET_PORT_NAME));
      }
      else
      {
         boost::replace_all(beh_desc, "1RESET_EDGE", "");
      }
      boost::replace_all(beh_desc, "1RESET_VALUE", std::string(RESET_PORT_NAME) + " == 1'b1");
   }
   if(parameters->getOption<bool>(OPT_reg_init_value))
   {
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUE", "=0");
   }
   else
   {
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUE", "");
   }
   indented_output_stream->Append(beh_desc);
}

system_verilog_writer::system_verilog_writer(const ParameterConstRef _parameters) : verilog_writer(_parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(this));
}
