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
 * @file NanoXploreBackendFlow.cpp
 * @brief Wrapper to NanoXplore synthesis tools
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
/// Header include
#include "NanoXploreBackendFlow.hpp"

#include "config_NANOXPLORE_BYPASS.hpp"
#include "config_NANOXPLORE_LICENSE.hpp"
#include "config_NANOXPLORE_SETTINGS.hpp"
#include "config_PANDA_DATA_INSTALLDIR.hpp"

/// constants include
#include "synthesis_constants.hpp"

#include "LUT_model.hpp"
#include "area_model.hpp"
#include "clb_model.hpp"
#include "target_device.hpp"
#include "target_manager.hpp"
#include "time_model.hpp"

#include "NanoXploreWrapper.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
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

#define NANOXPLORE_LICENSE_SET std::string("export LM_LICENSE_FILE=") + STR(NANOXPLORE_LICENSE) + std::string(";")
#define NANOXPLORE_BYPASS_SET std::string("export NANOXPLORE_BYPASS=") + STR(NANOXPLORE_BYPASS) + std::string(";")

NanoXploreBackendFlow::NanoXploreBackendFlow(const ParameterConstRef _Param, const std::string& _flow_name, const target_managerRef _target) : BackendFlow(_Param, _flow_name, _target)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Creating NanoXplore Backend Flow ::.");

   default_data["NG-medium"] = "NG-medium.data";
   default_data["NG-large"] = "NG-large.data";

   XMLDomParserRef parser;
   if(Param->isOption(OPT_target_device_script))
   {
      std::string xml_file_path = Param->getOption<std::string>(OPT_target_device_script);
      if(!boost::filesystem::exists(xml_file_path))
         THROW_ERROR("File \"" + xml_file_path + "\" does not exist!");
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Importing scripts from file: " + xml_file_path);
      parser = XMLDomParserRef(new XMLDomParser(xml_file_path));
   }
   else
   {
      const target_deviceRef device = target->get_target_device();
      std::string device_string;
      if(device->has_parameter("family"))
         device_string = device->get_parameter<std::string>("family");
      else
         device_string = "NG-medium";
      if(default_data.find(device_string) == default_data.end())
         THROW_ERROR("Device family \"" + device_string + "\" not supported!");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Importing default scripts for target device family: " + device_string);
      parser = XMLDomParserRef(new XMLDomParser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/wrapper/synthesis/nanoxplore/") + default_data[device_string]));
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
               continue;

            if(EnodeC->get_name() == "application")
            {
               const xml_node::node_list list_sec = EnodeC->get_children();
               for(const auto& iter_sec : list_sec)
               {
                  const auto* nodeS = GetPointer<const xml_element>(iter_sec);
                  if(!nodeS)
                     continue;

                  if(nodeS->get_name() == "section")
                  {
                     std::string stringID;
                     if(CE_XVM(stringID, nodeS))
                        LOAD_XVM(stringID, nodeS);
                     if(stringID == "NANOXPLORE_SYNTHESIS_SUMMARY")
                     {
                        const xml_node::node_list list_item = nodeS->get_children();
                        for(const auto& it_item : list_item)
                        {
                           const auto* nodeIt = GetPointer<const xml_element>(it_item);
                           if(!nodeIt or nodeIt->get_name() != "item")
                              continue;

                           if(CE_XVM(stringID, nodeIt))
                              LOAD_XVM(stringID, nodeIt);

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
   THROW_ERROR("Error during NanoXplore report parsing: " + fn);
}

void NanoXploreBackendFlow::CheckSynthesisResults()
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Analyzing NanoXplore synthesis results");
   std::string report_filename = actual_parameters->parameter_values[PARAM_nxpython_report];
   xparse_utilization(report_filename);

   THROW_ASSERT(design_values.find(NANOXPLORE_FE) != design_values.end(), "Missing logic elements");
   area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
   area_m->set_area_value(design_values[NANOXPLORE_FE]);
   auto* area_clb_model = GetPointer<clb_model>(area_m);
   area_clb_model->set_resource_value(clb_model::FUNCTIONAL_ELEMENTS, design_values[NANOXPLORE_FE]);
   area_clb_model->set_resource_value(clb_model::SLICE_LUTS, design_values[NANOXPLORE_LUTS]);
   area_clb_model->set_resource_value(clb_model::REGISTERS, design_values[NANOXPLORE_REGISTERS]);
   area_clb_model->set_resource_value(clb_model::DSP, design_values[NANOXPLORE_DSP]);
   area_clb_model->set_resource_value(clb_model::BRAM, design_values[NANOXPLORE_MEM]);

   time_m = time_model::create_model(TargetDevice_Type::FPGA, Param);
   auto* lut_m = GetPointer<LUT_model>(time_m);
   if(design_values[NANOXPLORE_SLACK] != 0.0)
   {
      auto clk_val = boost::lexical_cast<double>(actual_parameters->parameter_values[PARAM_clk_period]);
      auto del_val = design_values[NANOXPLORE_SLACK];
      double exec_time = clk_val - del_val;
      if(clk_val < del_val)
         THROW_ERROR("the timing analysis is not consistent with the specified clock period");
      lut_m->set_timing_value(LUT_model::COMBINATIONAL_DELAY, exec_time);
   }
   else
      lut_m->set_timing_value(LUT_model::COMBINATIONAL_DELAY, 0);
   if((output_level >= OUTPUT_LEVEL_VERY_PEDANTIC or (Param->IsParameter("DumpingTimingReport") and Param->GetParameter<int>("DumpingTimingReport"))) and
      ((actual_parameters->parameter_values.find(PARAM_nxpython_timing_report) != actual_parameters->parameter_values.end() and ExistFile(actual_parameters->parameter_values.find(PARAM_nxpython_timing_report)->second))))
   {
      CopyStdout(actual_parameters->parameter_values.find(PARAM_nxpython_timing_report)->second);
   }
}

void NanoXploreBackendFlow::WriteFlowConfiguration(std::ostream& script)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing flow configuration");
   std::string setupscr;
   if(STR(NANOXPLORE_SETTINGS) != "0")
   {
      setupscr = STR(NANOXPLORE_SETTINGS);
   }
   if(setupscr.size() and setupscr != "0")
   {
      script << "#configuration" << std::endl;
      if(boost::algorithm::starts_with(setupscr, "export"))
         script << setupscr + " >& /dev/null; ";
      else
         script << ". " << setupscr << " >& /dev/null; ";
      script << std::endl << std::endl;
   }
   auto nanoxplore_license = STR(NANOXPLORE_LICENSE);
   if(!nanoxplore_license.empty() && nanoxplore_license != "0")
   {
      script << NANOXPLORE_LICENSE_SET << std::endl;
   }
   auto nanoxplore_bypass = STR(NANOXPLORE_BYPASS);
   if(!nanoxplore_bypass.empty() && nanoxplore_bypass != "0")
   {
      script << NANOXPLORE_BYPASS_SET << std::endl;
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
      xpwr_enabled = true;
   actual_parameters->parameter_values[PARAM_power_optimization] = STR(xpwr_enabled);
   const target_deviceRef device = target->get_target_device();
   std::string device_name = device->get_parameter<std::string>("model");
   std::string package = device->get_parameter<std::string>("package");
   std::string speed_grade = device->get_parameter<std::string>("speed_grade");
   std::string device_string = device_name + package + speed_grade;
   actual_parameters->parameter_values[PARAM_target_device] = device_string;

   std::string HDL_files = actual_parameters->parameter_values[PARAM_HDL_files];
   std::vector<std::string> file_list = convert_string_to_vector<std::string>(HDL_files, ";");
   std::string sources_macro_list;
   for(unsigned int v = 0; v < file_list.size(); v++)
   {
      if(v)
         sources_macro_list += ", ";
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
