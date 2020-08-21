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
 * @file AlteraBackendFlow.cpp
 * @brief Implementation of the wrapper to Altera tools
 *
 * Implementation of the methods to wrap synthesis tools by Altera
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Silvia Lovergine <lovergine@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "AlteraBackendFlow.hpp"

/// Autoheader includes
#include "config_PANDA_DATA_INSTALLDIR.hpp"
#include "config_QUARTUS_13_SETTINGS.hpp"
#include "config_QUARTUS_SETTINGS.hpp"

/// constants include
#include "synthesis_constants.hpp"

#include "AlteraWrapper.hpp"

#include "LUT_model.hpp"
#include "area_model.hpp"
#include "clb_model.hpp"
#include "target_device.hpp"
#include "target_manager.hpp"
#include "time_model.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "xml_dom_parser.hpp"
#include "xml_script_command.hpp"

/// circuit include
#include "string_manipulation.hpp" // for GET_CLASS
#include "structural_objects.hpp"

#define ALTERA_LE "ALTERA_LE"
#define ALTERA_LAB "ALTERA_LAB"
#define ALTERA_FMAX "ALTERA_FMAX"
#define ALTERA_FMAX_RESTRICTED "ALTERA_FMAX_RESTRICTED"
#define ALTERA_REGISTERS "ALTERA_REGISTERS"
#define ALTERA_IOPIN "ALTERA_IOPIN"
#define ALTERA_ALUT "ALTERA_ALUT"
#define ALTERA_ALM "ALTERA_ALM"
#define ALTERA_DSP "ALTERA_DSP"
#define ALTERA_MEM "ALTERA_MEM"

AlteraBackendFlow::AlteraBackendFlow(const ParameterConstRef _Param, const std::string& _flow_name, const target_managerRef _target) : BackendFlow(_Param, _flow_name, _target)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, " .:: Creating Altera Backend Flow ::.");

   default_data["CycloneII"] = "CycloneII.data";

   default_data["CycloneII-R"] = "CycloneII-R.data";

   default_data["CycloneV"] = "CycloneV.data";
   default_data["StratixV"] = "StratixV.data";
   default_data["StratixIV"] = "StratixIV.data";
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
      std::string device_string;
      target_deviceRef device = target->get_target_device();
      if(device->has_parameter("family"))
         device_string = device->get_parameter<std::string>("family");
      else
         device_string = "CycloneII";
      if(default_data.find(device_string) == default_data.end())
         THROW_ERROR("Device family \"" + device_string + "\" not supported!");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Importing default scripts for target device family: " + device_string);
      parser = XMLDomParserRef(new XMLDomParser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/wrapper/synthesis/altera/") + default_data[device_string]));
   }
   parse_flow(parser);
}

AlteraBackendFlow::~AlteraBackendFlow() = default;

void AlteraBackendFlow::xparse_utilization(const std::string& fn)
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
                     if(stringID == "QUARTUS_SYNTHESIS_SUMMARY")
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
   THROW_ERROR("Error during QUARTUS_SH report parsing: " + fn);
}

void AlteraBackendFlow::CheckSynthesisResults()
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Analyzing Altera synthesis results");
   std::string report_filename = actual_parameters->parameter_values[PARAM_quartus_report];
   xparse_utilization(report_filename);

   THROW_ASSERT(design_values.find(ALTERA_LE) != design_values.end(), "Missing logic elements");
   area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
   if(design_values[ALTERA_LE] != 0.0)
      area_m->set_area_value(design_values[ALTERA_LE]);
   else
      area_m->set_area_value(design_values[ALTERA_ALM]);
   auto* area_clb_model = GetPointer<clb_model>(area_m);
   if(design_values[ALTERA_LE] != 0.0)
      area_clb_model->set_resource_value(clb_model::LOGIC_ELEMENTS, design_values[ALTERA_LE]);
   else
      area_clb_model->set_resource_value(clb_model::ALMS, design_values[ALTERA_ALM]);

   area_clb_model->set_resource_value(clb_model::REGISTERS, design_values[ALTERA_REGISTERS]);
   area_clb_model->set_resource_value(clb_model::DSP, design_values[ALTERA_DSP]);
   area_clb_model->set_resource_value(clb_model::BRAM, design_values[ALTERA_MEM]);

   time_m = time_model::create_model(TargetDevice_Type::FPGA, Param);
   auto* lut_m = GetPointer<LUT_model>(time_m);
   const auto combinational_delay = [&]() -> double {
      if(design_values[ALTERA_FMAX] != 0.0)
         return 1000 / design_values[ALTERA_FMAX];
      else if(design_values.find("SLACK") != design_values.end())
         return Param->getOption<double>(OPT_clock_period) - design_values.find("SLACK")->second;
      else
         return 0.0;
   }();
   lut_m->set_timing_value(LUT_model::COMBINATIONAL_DELAY, combinational_delay);
   if(combinational_delay > Param->getOption<double>(OPT_clock_period))
   {
      CopyFile(actual_parameters->parameter_values[PARAM_top_id] + ".sta.rpt", Param->getOption<std::string>(OPT_output_directory) + "/" + flow_name + "/" + STR_CST_synthesis_timing_violation_report);
   }
}

void AlteraBackendFlow::WriteFlowConfiguration(std::ostream& script)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing flow configuration");
   std::string device_string = Param->getOption<std::string>(OPT_device_string);
   std::string setupscr;
   if(STR(QUARTUS_SETTINGS) != "0" and device_string.find("EP2C") == std::string::npos)
   {
      setupscr = STR(QUARTUS_SETTINGS);
   }
   else if(STR(QUARTUS_13_SETTINGS) != "0")
   {
      setupscr = STR(QUARTUS_13_SETTINGS);
   }
   if(setupscr.size())
   {
      script << "#configuration" << std::endl;
      if(boost::algorithm::starts_with(setupscr, "export"))
         script << setupscr + " >& /dev/null; ";
      else
         script << ". " << setupscr << " >& /dev/null; ";
      script << std::endl << std::endl;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written flow configuration");
}

void AlteraBackendFlow::create_sdc(const DesignParametersRef dp)
{
   std::string clock = dp->parameter_values[PARAM_clk_name];

   std::string sdc_filename = out_dir + "/" + dp->component_name + ".sdc";
   std::ofstream sdc_file(sdc_filename.c_str());
   if(!boost::lexical_cast<bool>(dp->parameter_values[PARAM_is_combinational]))
   {
      sdc_file << "create_clock -period " + dp->parameter_values[PARAM_clk_period] + " -name " + clock + " [get_ports " + clock + "]\n";
      if(get_flow_name() != "Characterization" && (boost::lexical_cast<bool>(dp->parameter_values[PARAM_connect_iob]) || (Param->IsParameter("profile-top") && Param->GetParameter<int>("profile-top") == 1)) && !Param->isOption(OPT_backend_sdc_extensions))
      {
         sdc_file << "set_min_delay 0 -from [all_inputs] -to [all_registers]\n";
         sdc_file << "set_min_delay 0 -from [all_registers] -to [all_outputs]\n";
         sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_inputs] -to [all_registers]\n";
         sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_registers] -to [all_outputs]\n";
         sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_inputs] -to [all_outputs]\n";
      }
      sdc_file << "derive_pll_clocks\n";
      sdc_file << "derive_clock_uncertainty\n";
   }
   else
      sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_inputs] -to [all_outputs]\n";

   if(Param->isOption(OPT_backend_sdc_extensions))
      sdc_file << "source " + Param->getOption<std::string>(OPT_backend_sdc_extensions) + "\n";
   sdc_file.close();
   dp->parameter_values[PARAM_sdc_file] = sdc_filename;
}

void AlteraBackendFlow::InitDesignParameters()
{
   const target_deviceRef device = target->get_target_device();
   actual_parameters->parameter_values[PARAM_target_device] = device->get_parameter<std::string>("model");
   std::string device_family = device->get_parameter<std::string>("family");
   if(device_family.find('-') != std::string::npos)
      device_family = device_family.substr(0, device_family.find('-'));
   actual_parameters->parameter_values[PARAM_target_family] = device_family;

   std::string HDL_files = actual_parameters->parameter_values[PARAM_HDL_files];
   std::vector<std::string> file_list = convert_string_to_vector<std::string>(HDL_files, ";");
   std::string sources_macro_list;
   bool has_vhdl_library = Param->isOption(OPT_VHDL_library);
   std::string vhdl_library;
   if(has_vhdl_library)
      vhdl_library = Param->getOption<std::string>(OPT_VHDL_library);
   for(const auto& v : file_list)
   {
      if(has_vhdl_library)
         sources_macro_list += "set_global_assignment -name SOURCE_FILE " + v + " -library " + vhdl_library + "\n";
      else
         sources_macro_list += "set_global_assignment -name SOURCE_FILE " + v + "\n";
   }
   actual_parameters->parameter_values[PARAM_sources_macro_list] = sources_macro_list;

   create_sdc(actual_parameters);

   for(auto& step : steps)
   {
      step->tool->EvaluateVariables(actual_parameters);
   }
}

void AlteraBackendFlow::ExecuteSynthesis()
{
   BackendFlow::ExecuteSynthesis();
}
