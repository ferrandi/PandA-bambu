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
 * @file quartus_report_wrapper.cpp
 * @brief Implementation of the wrapper to quartus reporting tool
 *
 * Implementation of the methods to wrap quartus reporting tool
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Silvia Lovergine <lovergine@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
/// Header include
#include "quartus_report_wrapper.hpp"

#include "ToolManager.hpp"
#include "xml_script_command.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_

// constructor
QuartusReportWrapper::QuartusReportWrapper(const ParameterConstRef& _Param, const std::string& _output_dir, const target_deviceRef& _device) : AlteraWrapper(_Param, QUARTUS_REPORT_TOOL_EXEC, _device, _output_dir, QUARTUS_REPORT_TOOL_ID)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating the QUARTUS_REPORT wrapper...");
}

// destructor
QuartusReportWrapper::~QuartusReportWrapper() = default;

void QuartusReportWrapper::EvaluateVariables(const DesignParametersRef dp)
{
   std::string top_id = dp->component_name;
   dp->parameter_values[PARAM_quartus_report] = output_dir + "/" + top_id + "_report.xml";
}

std::string QuartusReportWrapper::get_command_line(const DesignParametersRef& dp) const
{
   std::ostringstream s;
   s << get_tool_exec() << " -t " << script_name;
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
