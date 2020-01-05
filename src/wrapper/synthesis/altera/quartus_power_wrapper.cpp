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
 *              Copyright (C) 2017-2020 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */
/// Header include
#include "quartus_power_wrapper.hpp"

#include "Parameter.hpp"
#include "ToolManager.hpp"
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp"
#include "xml_script_command.hpp"

#include <boost/filesystem.hpp>

// constructor
QuartusPowerWrapper::QuartusPowerWrapper(const ParameterConstRef _Param, const std::string& _output_dir, const target_deviceRef _device) : AlteraWrapper(_Param, QUARTUS_POWER_TOOL_EXEC, _device, _output_dir, QUARTUS_POWER_TOOL_ID)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating the QUARTUS_POWER wrapper...");
}

// destructor
QuartusPowerWrapper::~QuartusPowerWrapper()
{
}

void QuartusPowerWrapper::EvaluateVariables(const DesignParametersRef dp)
{
   std::string top_id = dp->component_name;
   dp->parameter_values[PARAM_quartus_report] = output_dir + "/" + top_id + "_report.xml";
}

std::string QuartusPowerWrapper::get_command_line(const DesignParametersRef& dp) const
{
   std::ostringstream s;
   s << get_tool_exec() << " -f " << script_name;
   for(std::vector<xml_parameter_tRef>::const_iterator it = xml_tool_options.begin(); it != xml_tool_options.end(); it++)
   {
      const xml_parameter_tRef& option = *it;
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

void QuartusPowerWrapper::generate_synthesis_script(const DesignParametersRef& dp, const std::string& file_name)
{
   // Export reserved (constant) values to design parameters
   for(auto it = xml_reserved_vars.begin(); it != xml_reserved_vars.end(); ++it)
   {
      const xml_set_variable_tRef& var = (*it);
      dp->assign(var->name, getStringValue(var, dp), false);
   }

   // Bare script generation
   std::ostringstream script;
   script << generate_bare_script(xml_script_nodes, dp);

   // Replace all reserved variables with their value
   std::string script_string = script.str();
   replace_parameters(dp, script_string);
   /// replace some of the escape sequences
   remove_escaped(script_string);

   // Save the generated script
   if(boost::filesystem::exists(file_name))
   {
      boost::filesystem::remove_all(file_name);
   }
   script_name = file_name;
   std::ofstream file_stream(file_name.c_str());
   file_stream << script_string << std::endl;
   file_stream.close();
}
