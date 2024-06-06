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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file NanoXploreBackendFlow.cpp
 * @brief Wrapper to NanoXplore synthesis tools
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "NanoXploreBackendFlow.hpp"

#include "config_PANDA_DATA_INSTALLDIR.hpp"

#include "DesignParameters.hpp"
#include "NanoXploreWrapper.hpp"
#include "Parameter.hpp"
#include "area_info.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"
#include "string_manipulation.hpp"
#include "synthesis_constants.hpp"
#include "time_info.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include "xml_script_command.hpp"

#define NANOXPLORE_FE "NANOXPLORE_FE"
#define NANOXPLORE_LUTS "NANOXPLORE_LUTS"
#define NANOXPLORE_SLACK "NANOXPLORE_SLACK"
#define NANOXPLORE_REGISTERS "NANOXPLORE_REGISTERS"
#define NANOXPLORE_IOPIN "NANOXPLORE_IOPIN"
#define NANOXPLORE_DSP "NANOXPLORE_DSPS"
#define NANOXPLORE_MEM "NANOXPLORE_MEM"
#define NANOXPLORE_POWER "NANOXPLORE_POWER"

NanoXploreBackendFlow::NanoXploreBackendFlow(const ParameterConstRef _Param, const std::string& _flow_name,
                                             const generic_deviceRef _device)
    : BackendFlow(_Param, _flow_name, _device)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Creating NanoXplore Backend Flow ::.");
   if(!Param->isOption(OPT_nanoxplore_root))
   {
      THROW_WARNING("NanoXplore install directory was not specified, fallback on path. Specifying NanoXplore root "
                    "through --nanoxplore-root option is preferred.");
   }
   const auto lic_path = std::getenv("LM_LICENSE_FILE");
   const auto nx_lic_path = std::getenv("NXLMD_LICENSE_FILE");
   if((!lic_path || std::string(lic_path) == "") && (!nx_lic_path || std::string(nx_lic_path) == ""))
   {
      THROW_WARNING("NanoXplore license file has not been specified. User must set LM_LICENSE_FILE or "
                    "NXLMD_LICENSE_FILE variable to point to the license file location.");
   }
   const auto bypass_name = std::getenv("NANOXPLORE_BYPASS");
   if((!bypass_name || std::string(bypass_name) == "") && !Param->isOption(OPT_nanoxplore_bypass))
   {
      THROW_WARNING("NanoXplore bypass was not specified. User may set NANOXPLORE_BYPASS variable or use "
                    "--nanoxplore-bypass option.");
   }

   default_data["NG-MEDIUM"] = "NG.data";
   default_data["NG-LARGE"] = "NG.data";
   default_data["NG-ULTRA"] = "NG.data";

   XMLDomParserRef parser;
   if(Param->isOption(OPT_target_device_script))
   {
      auto xml_file_path = Param->getOption<std::string>(OPT_target_device_script);
      if(!std::filesystem::exists(xml_file_path))
      {
         THROW_ERROR("File \"" + xml_file_path + "\" does not exist!");
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Importing scripts from file: " + xml_file_path);
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
         device_string = "NG-MEDIUM";
      }
      if(default_data.find(device_string) == default_data.end())
      {
         THROW_ERROR("Device family \"" + device_string + "\" not supported!");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "---Importing default scripts for target device family: " + device_string);
      parser = XMLDomParserRef(new XMLDomParser(
          relocate_install_path(PANDA_DATA_INSTALLDIR "/wrapper/synthesis/nanoxplore") / default_data[device_string]));
   }
   parse_flow(parser);
}

NanoXploreBackendFlow::~NanoXploreBackendFlow() = default;

void NanoXploreBackendFlow::xparse_utilization(const std::string& fn)
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
                     if(stringID == "NANOXPLORE_SYNTHESIS_SUMMARY")
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
                              design_values[stringID] = std::stod(value);
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
   THROW_ERROR("Error during NanoXplore report parsing: " + fn);
}

void NanoXploreBackendFlow::CheckSynthesisResults()
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Analyzing NanoXplore synthesis results");
   const auto report_filename = actual_parameters->parameter_values[PARAM_nxpython_report];
   xparse_utilization(report_filename);

   THROW_ASSERT(design_values.find(NANOXPLORE_FE) != design_values.end(), "Missing logic elements");
   area_m = area_info::factory(Param);
   area_m->set_area_value(design_values[NANOXPLORE_FE]);
   area_m->set_resource_value(area_info::FUNCTIONAL_ELEMENTS, design_values[NANOXPLORE_FE]);
   area_m->set_resource_value(area_info::SLICE_LUTS, design_values[NANOXPLORE_LUTS]);
   area_m->set_resource_value(area_info::REGISTERS, design_values[NANOXPLORE_REGISTERS]);
   area_m->set_resource_value(area_info::DSP, design_values[NANOXPLORE_DSP]);
   area_m->set_resource_value(area_info::BRAM, design_values[NANOXPLORE_MEM]);
   area_m->set_resource_value(area_info::POWER, design_values[NANOXPLORE_POWER]);

   time_m = time_info::factory(Param);
   if(design_values[NANOXPLORE_SLACK] != 0.0)
   {
      auto clk_val = std::stod(actual_parameters->parameter_values[PARAM_clk_period]);
      auto del_val = design_values[NANOXPLORE_SLACK];
      double exec_time = clk_val - del_val;
      if(clk_val < del_val)
      {
         THROW_ERROR("the timing analysis is not consistent with the specified clock period");
      }
      time_m->set_execution_time(exec_time);
   }
   else
   {
      time_m->set_execution_time(0);
   }
   if((output_level >= OUTPUT_LEVEL_VERY_PEDANTIC ||
       (Param->IsParameter("DumpingTimingReport") && Param->GetParameter<int>("DumpingTimingReport"))) &&
      ((actual_parameters->parameter_values.find(PARAM_nxpython_timing_report) !=
            actual_parameters->parameter_values.end() &&
        std::filesystem::exists(actual_parameters->parameter_values.at(PARAM_nxpython_timing_report)))))
   {
      CopyStdout(actual_parameters->parameter_values.at(PARAM_nxpython_timing_report));
   }
}

void NanoXploreBackendFlow::WriteFlowConfiguration(std::ostream& script)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing flow configuration");
   script << "#configuration" << std::endl;
   if(Param->isOption(OPT_nanoxplore_root))
   {
      const auto nxroot = Param->getOption<std::string>(OPT_nanoxplore_root);
      script << "export PATH=$PATH:" + nxroot + "/bin" << std::endl << std::endl;
   }
   script << "if [ ! -z \"$NXLMD_LICENSE_FILE\" ]; then" << std::endl;
   script << "  if [[ \"$NXLMD_LICENSE_FILE\" != \"$LM_LICENSE_FILE\" ]]; then" << std::endl;
   script << "    export LM_LICENSE_FILE=\"$NXLMD_LICENSE_FILE\"" << std::endl;
   script << "  fi" << std::endl;
   script << "fi" << std::endl << std::endl;
   if(Param->isOption(OPT_nanoxplore_bypass))
   {
      script << "export NANOXPLORE_BYPASS=\"" << Param->getOption<std::string>(OPT_nanoxplore_bypass) << "\""
             << std::endl;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written flow configuration");
}

void NanoXploreBackendFlow::ExecuteSynthesis()
{
   BackendFlow::ExecuteSynthesis();
}

void NanoXploreBackendFlow::InitDesignParameters()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->NanoXploreBackendFlow - Init Design Parameters");

   /// determine if power optimization has to be performed
   bool xpwr_enabled = false;
   if(Param->isOption("power_optimization") && Param->getOption<bool>("power_optimization"))
   {
      xpwr_enabled = true;
   }
   actual_parameters->parameter_values[PARAM_power_optimization] = STR(xpwr_enabled);
   const auto family = device->get_parameter<std::string>("family");
   const auto device_name = device->get_parameter<std::string>("model");
   const auto package = device->get_parameter<std::string>("package");
   const auto speed_grade = device->get_parameter<std::string>("speed_grade");
   std::string device_string = device_name + package + speed_grade;
   actual_parameters->parameter_values[PARAM_target_device] = device_string;
   actual_parameters->parameter_values[PARAM_target_family] = family;

   const auto file_list =
       string_to_container<std::vector<std::string>>(actual_parameters->parameter_values[PARAM_HDL_files], ";");
   std::string sources_macro_list;
   for(unsigned int v = 0; v < file_list.size(); v++)
   {
      if(v)
      {
         sources_macro_list += ", ";
      }
      sources_macro_list += "'" + file_list[v] + "'";
   }
   actual_parameters->parameter_values[PARAM_nxpython_sources_macro_list] = sources_macro_list;

   for(auto& step : steps)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Evaluating variables of step " + step->name);
      step->tool->EvaluateVariables(actual_parameters);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Evaluated variables of step " + step->name);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--NanoXploreBackendFlow - Init Design Parameters");
}
