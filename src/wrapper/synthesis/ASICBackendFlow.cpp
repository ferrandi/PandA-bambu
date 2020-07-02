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
 * @file ASICBackendFlow.cpp
 * @brief Implementation of the wrapper to ASIC tools
 *
 * Implementation of the methods to wrap synthesis tools for integrated circuits
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "ASICBackendFlow.hpp"

#include "config_PANDA_DATA_INSTALLDIR.hpp"

#include "DesignCompilerWrapper.hpp"
#include "SynthesisTool.hpp"

#include "area_model.hpp"
#include "target_device.hpp"
#include "time_model.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "structural_objects.hpp"
#include "xml_dom_parser.hpp"
#include "xml_script_command.hpp"

ASICBackendFlow::ASICBackendFlow(const ParameterConstRef _Param, const std::string& _flow_name, const target_managerRef _target) : BackendFlow(_Param, _flow_name, _target)
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, " .:: Creating ASIC Backend Flow ::.");

   default_data["Nangate"] = "Nangate.data";

   XMLDomParserRef parser;
   if(Param->isOption(OPT_target_device_script))
   {
      std::string xml_file_path = Param->getOption<std::string>(OPT_target_device_script);
      if(!boost::filesystem::exists(xml_file_path))
         THROW_ERROR("File \"" + xml_file_path + "\" does not exist!");
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Importing scripts from file: " + xml_file_path);
      parser = XMLDomParserRef(new XMLDomParser(xml_file_path));
   }
   else
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Importing default scripts for target technology: \"Nangate\"");
      parser = XMLDomParserRef(new XMLDomParser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/wrapper/synthesis/") + default_data["Nangate"]));
   }
   parse_flow(parser);
}

ASICBackendFlow::~ASICBackendFlow() = default;

void ASICBackendFlow::create_sdc(const DesignParametersRef dp)
{
   std::string sdc_filename = out_dir + "/" + dp->component_name + ".sdc";
   std::ofstream SDC_file(sdc_filename.c_str());
   if(dp->parameter_values.find(PARAM_clk_name) != dp->parameter_values.end() && !boost::lexical_cast<bool>(dp->parameter_values[PARAM_is_combinational]))
      SDC_file << "create_clock " << dp->parameter_values[PARAM_clk_name] << " -period " << dp->parameter_values[PARAM_clk_period] << std::endl;
   else
      SDC_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_inputs] -to [all_outputs]\n";

   if(Param->isOption(OPT_backend_sdc_extensions))
      SDC_file << "source " + Param->getOption<std::string>(OPT_backend_sdc_extensions) + "\n";
   SDC_file.close();
   dp->parameter_values[PARAM_sdc_file] = sdc_filename;
}

void ASICBackendFlow::InitDesignParameters()
{
   create_sdc(actual_parameters);

   for(auto& step : steps)
   {
      step->tool->EvaluateVariables(actual_parameters);
   }
}

void ASICBackendFlow::CheckSynthesisResults()
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Analyzing ASIC synthesis results");
   for(auto& step : steps)
   {
      if(step->tool->get_tool_exec() == DESIGN_COMPILER_TOOL_ID)
      {
         area_modelRef area = GetPointer<DesignCompilerWrapper>(step->tool)->parse_area_reports();
         if(area)
            area_m = area;

         time_modelRef time = GetPointer<DesignCompilerWrapper>(step->tool)->parse_time_reports();
         if(time)
            time_m = time;
      }
   }
}

void ASICBackendFlow::WriteFlowConfiguration(std::ostream& /*script*/)
{
   /// nothing to do
}
