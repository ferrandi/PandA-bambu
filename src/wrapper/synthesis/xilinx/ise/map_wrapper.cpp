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
 * @file map_wrapper.cpp
 * @brief Implementation of the wrapper to MAP
 *
 * Implementation of the methods to wrap MAP by Xilinx
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "map_wrapper.hpp"

#include "config_XILINX_SETTINGS.hpp"

#include "ToolManager.hpp"
#include "xml_script_command.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include <iostream>

// constructor
map_wrapper::map_wrapper(const ParameterConstRef& _Param, const std::string& _output_dir, const target_deviceRef& _device) : XilinxWrapper(_Param, MAP_TOOL_ID, _device, _output_dir, "map")
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating the MAP wrapper...");
}

// destructor
map_wrapper::~map_wrapper() = default;

void map_wrapper::init_reserved_vars()
{
   XilinxWrapper::init_reserved_vars();
   ADD_RES_VAR(PARAM_map_tmpdir);
}

void map_wrapper::EvaluateVariables(const DesignParametersRef dp)
{
   dp->assign(PARAM_map_tmpdir, output_dir, false);
   std::string top_id = dp->parameter_values[PARAM_top_id];
   dp->assign(PARAM_map_report, output_dir + "/" + top_id + ".mrp", false);
}

std::string map_wrapper::get_command_line(const DesignParametersRef& dp) const
{
   std::ostringstream s;
   s << get_tool_exec();
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
