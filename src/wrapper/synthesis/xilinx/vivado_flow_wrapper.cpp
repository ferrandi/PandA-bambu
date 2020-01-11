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
 * @file vivado_flow_wrapper.cpp
 * @brief Implementation of the wrapper to vivado_flow
 *
 * Implementation of the methods to wrap vivado_flow by Xilinx
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "vivado_flow_wrapper.hpp"
#include "XilinxBackendFlow.hpp"

#include "ToolManager.hpp"
#include "xml_script_command.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_

#define PARAM_vivado_outdir "vivado_outdir"

// constructor
vivado_flow_wrapper::vivado_flow_wrapper(const ParameterConstRef& _Param, const std::string& _output_dir, const target_deviceRef& _device) : XilinxWrapper(_Param, VIVADO_FLOW_TOOL_EXEC, _device, _output_dir, VIVADO_FLOW_TOOL_ID)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating the VIVADO_FLOW wrapper...");
}

// destructor
vivado_flow_wrapper::~vivado_flow_wrapper() = default;

void vivado_flow_wrapper::EvaluateVariables(const DesignParametersRef dp)
{
   std::string top_id = dp->component_name;
   dp->assign(PARAM_vivado_outdir, output_dir, false);
   dp->parameter_values[PARAM_vivado_report] = output_dir + "/" + top_id + "_report.xml";
   dp->parameter_values[PARAM_vivado_timing_report] = output_dir + "/post_route_timing_summary.rpt";
   create_sdc(dp);
}

void vivado_flow_wrapper::create_sdc(const DesignParametersRef& dp)
{
   std::string clock = dp->parameter_values[PARAM_clk_name];

   std::string sdc_filename = output_dir + "/" + dp->component_name + ".sdc";
   std::ofstream sdc_file(sdc_filename.c_str());
   if(!boost::lexical_cast<bool>(dp->parameter_values[PARAM_is_combinational]))
   {
      sdc_file << "create_clock -period " + dp->parameter_values[PARAM_clk_period] + " -name " + clock + " [get_ports " + clock + "]\n";
      if(boost::lexical_cast<bool>(dp->parameter_values[PARAM_connect_iob]) || (Param->IsParameter("profile-top") && Param->GetParameter<int>("profile-top") == 1))
      {
         sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_inputs] -to [all_outputs]\n";
         sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_inputs] -to [all_registers]\n";
         sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_registers] -to [all_outputs]\n";
      }
   }
   else
   {
      sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_inputs] -to [all_outputs]\n";
   }

   sdc_file.close();
   dp->parameter_values[PARAM_sdc_file] = sdc_filename;
}

std::string vivado_flow_wrapper::get_command_line(const DesignParametersRef& dp) const
{
   std::ostringstream s;
   s << get_tool_exec() << " -mode batch -nojournal -nolog -source " << script_name;
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
