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
 *              Copyright (C) 2020-2023 Politecnico di Milano
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
 * @file BashBackendFlow.cpp
 * @brief Backend based on a simple bash script
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@elet.polimi.it>
 *
 */
#include "BashBackendFlow.hpp"

#include "DesignParameters.hpp"
#include "Parameter.hpp"
#include "SynthesisTool.hpp"
#include "area_info.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"
#include "string_manipulation.hpp"
#include "structural_objects.hpp"
#include "synthesis_constants.hpp"
#include "time_info.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include "xml_script_command.hpp"

#include "config_PANDA_DATA_INSTALLDIR.hpp"

#define BASHBACKEND_AREA "BASHBACKEND_AREA"
#define BASHBACKEND_POWER "BASHBACKEND_POWER"
#define BASHBACKEND_DESIGN_DELAY "BASHBACKEND_DESIGN_DELAY"

BashBackendFlow::BashBackendFlow(const ParameterConstRef _Param, const std::string& _flow_name,
                                 const generic_deviceRef _device)
    : BackendFlow(_Param, _flow_name, _device)
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, " .:: Creating Generic Bash Backend Flow ::.");

   default_data["Generic-yosysOpenROAD"] = "Generic-yosysOpenROAD.data";

   XMLDomParserRef parser;
   if(Param->isOption(OPT_target_device_script))
   {
      auto xml_file_path = Param->getOption<std::string>(OPT_target_device_script);
      if(!std::filesystem::exists(xml_file_path))
      {
         THROW_ERROR("File \"" + xml_file_path + "\" does not exist!");
      }
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Importing scripts from file: " + xml_file_path);
      parser = XMLDomParserRef(new XMLDomParser(xml_file_path));
   }
   else
   {
      std::string device_string;
      if(device->has_parameter("family"))
      {
         device_string = device->get_parameter<std::string>("family");
      }
      else
      {
         device_string = "yosysOpenROAD";
      }
      if(default_data.find(device_string) == default_data.end())
      {
         THROW_ERROR("Device family \"" + device_string + "\" not supported!");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "---Importing default scripts for target device family: " + device_string);
      parser = XMLDomParserRef(
          new XMLDomParser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/wrapper/synthesis/", true) +
                           default_data[device_string]));
   }
   parse_flow(parser);
}

BashBackendFlow::~BashBackendFlow() = default;

void BashBackendFlow::xparse_utilization(const std::string& fn)
{
   try
   {
      XMLDomParser parser(fn);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         THROW_ASSERT(node->get_name() == "document", "Wrong root name: " + node->get_name());

         const xml_node::node_list list_int = node->get_children();
         for(const auto& iter_int : list_int)
         {
            const auto* EnodeC = GetPointer<const xml_element>(iter_int);
            if(!EnodeC)
            {
               continue;
            }

            if(EnodeC->get_name() == "application")
            {
               const xml_node::node_list list_sec = EnodeC->get_children();
               for(const auto& iter_sec : list_sec)
               {
                  const auto* nodeS = GetPointer<const xml_element>(iter_sec);
                  if(!nodeS)
                  {
                     continue;
                  }

                  if(nodeS->get_name() == "section")
                  {
                     std::string stringID;
                     if(CE_XVM(stringID, nodeS))
                     {
                        LOAD_XVM(stringID, nodeS);
                     }
                     if(stringID == "BASH_SYNTHESIS_SUMMARY")
                     {
                        const xml_node::node_list list_item = nodeS->get_children();
                        for(const auto& it_item : list_item)
                        {
                           const auto* nodeIt = GetPointer<const xml_element>(it_item);
                           if(!nodeIt or nodeIt->get_name() != "item")
                           {
                              continue;
                           }

                           if(CE_XVM(stringID, nodeIt))
                           {
                              LOAD_XVM(stringID, nodeIt);
                           }

                           std::string value;
                           if(CE_XVM(value, nodeIt))
                           {
                              LOAD_XVM(value, nodeIt);
                              boost::replace_all(value, ",", "");
                              design_values[stringID] = boost::lexical_cast<double>(value);
                           }
                        }
                     }
                  }
               }
            }
         }
         return;
      }
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
   THROW_ERROR("Error during report parsing: " + fn);
}

void BashBackendFlow::create_sdc(const DesignParametersRef dp)
{
   std::string sdc_filename = out_dir + "/" + dp->component_name + ".sdc";
   std::ofstream SDC_file(sdc_filename.c_str());
   if(dp->parameter_values.find(PARAM_clk_name) != dp->parameter_values.end() &&
      !boost::lexical_cast<bool>(dp->parameter_values[PARAM_is_combinational]))
   {
      SDC_file << "create_clock " << dp->parameter_values[PARAM_clk_name] << " -period "
               << dp->parameter_values[PARAM_clk_period] << std::endl;
   }
   else
   {
      SDC_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_inputs] -to [all_outputs]\n";
   }

   if(Param->isOption(OPT_backend_sdc_extensions))
   {
      SDC_file << "source " + Param->getOption<std::string>(OPT_backend_sdc_extensions) + "\n";
   }
   SDC_file.close();
   dp->parameter_values[PARAM_sdc_file] = sdc_filename;
}

void BashBackendFlow::InitDesignParameters()
{
   /// determine if power optimization has to be performed
   bool pwr_enabled = false;
   if(Param->isOption("power_optimization") && Param->getOption<bool>("power_optimization"))
   {
      pwr_enabled = true;
   }
   actual_parameters->parameter_values[PARAM_power_optimization] = STR(pwr_enabled);
   auto device_name = device->get_parameter<std::string>("model");
   actual_parameters->parameter_values[PARAM_target_device] = device_name;

   std::string HDL_files = actual_parameters->parameter_values[PARAM_HDL_files];
   std::vector<std::string> file_list = convert_string_to_vector<std::string>(HDL_files, ";");
   std::string sources_macro_list;
   for(unsigned int v = 0; v < file_list.size(); v++)
   {
      if(v)
      {
         sources_macro_list += " ";
      }
      sources_macro_list += file_list[v];
      std::filesystem::path file_path(file_list[v]);
   }

   actual_parameters->parameter_values[PARAM_bash_sources_macro_list] = sources_macro_list;

   create_sdc(actual_parameters);

   for(auto& step : steps)
   {
      step->tool->EvaluateVariables(actual_parameters);
   }
}

void BashBackendFlow::CheckSynthesisResults()
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Analyzing synthesis results");
   std::string report_filename = actual_parameters->parameter_values[PARAM_bash_backend_report];
   xparse_utilization(report_filename);

   THROW_ASSERT(design_values.find(BASHBACKEND_AREA) != design_values.end(), "Missing logic elements");
   area_m = area_info::factory(Param);
   area_m->set_area_value(design_values[BASHBACKEND_AREA]);
   area_m->set_resource_value(area_info::LOGIC_AREA, design_values[BASHBACKEND_AREA]);
   area_m->set_resource_value(area_info::POWER, design_values[BASHBACKEND_POWER]);

   time_m = time_info::factory(Param);
   if(design_values[BASHBACKEND_DESIGN_DELAY] != 0.0)
   {
      auto is_time_unit_PS =
          device->has_parameter("USE_TIME_UNIT_PS") && device->get_parameter<int>("USE_TIME_UNIT_PS") == 1;
      time_m->set_execution_time(design_values[BASHBACKEND_DESIGN_DELAY] / (is_time_unit_PS ? 1000 : 1));
   }
   else
   {
      time_m->set_execution_time(0.0);
   }
   if((output_level >= OUTPUT_LEVEL_VERY_PEDANTIC or
       (Param->IsParameter("DumpingTimingReport") and Param->GetParameter<int>("DumpingTimingReport"))) and
      ((actual_parameters->parameter_values.find(PARAM_bash_backend_timing_report) !=
            actual_parameters->parameter_values.end() and
        std::filesystem::exists(actual_parameters->parameter_values.find(PARAM_bash_backend_timing_report)->second))))
   {
      CopyStdout(actual_parameters->parameter_values.find(PARAM_bash_backend_timing_report)->second);
   }
}

void BashBackendFlow::WriteFlowConfiguration(std::ostream& script)
{
   script << "export PANDA_DATA_INSTALLDIR=" << relocate_compiler_path(std::string(PANDA_DATA_INSTALLDIR "/panda/"))
          << "\n";
   script << "export CURR_WORKDIR=" << GetCurrentPath() << "\n";

   for(const auto& pair : device->get_device_bash_vars())
   {
      script << ": ${" << pair.first << ":=" << pair.second << "}"
             << "\n";
      script << "export " << pair.first << "\n";
   }
}
