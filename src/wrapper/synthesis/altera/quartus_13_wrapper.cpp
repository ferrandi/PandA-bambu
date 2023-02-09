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
 *              Copyright (C) 2016-2023 Politecnico di Milano
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
 * @file quartus_13_wrapper.cpp
 * @brief Wrapper to quartus 13.x
 *
 * A object used to invoke synthesis tools by Altera
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "quartus_13_wrapper.hpp"

/// wrapper/synthesis include
#include "xml_script_command.hpp"

#include "Parameter.hpp"

Quartus13Wrapper::Quartus13Wrapper(const ParameterConstRef& _Param, const std::string& _output_dir,
                                   const target_deviceRef& _device)
    : AlteraWrapper(_Param, QUARTUS_13_FLOW_TOOL_EXEC, _device, _output_dir, QUARTUS_13_FLOW_TOOL_ID)
{
}

Quartus13Wrapper::~Quartus13Wrapper() = default;
std::string Quartus13Wrapper::get_command_line(const DesignParametersRef& dp) const
{
   std::ostringstream s;
   s << get_tool_exec() << " -t ";
   if(Param->isOption(OPT_quartus_13_64bit) && Param->getOption<bool>(OPT_quartus_13_64bit))
   {
      s << " --64bit ";
   }
   s << script_name;
   for(const auto& option : xml_tool_options)
   {
      if(option->checkCondition(dp))
      {
         std::string value = toString(option, dp);
         replace_parameters(dp, value);
         s << " " << value;
      }
   }
   s << std::endl;
   return s.str();
}
