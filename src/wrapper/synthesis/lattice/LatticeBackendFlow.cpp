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
 * @file LatticeBackendFlow.cpp
 * @brief Implementation of the wrapper to Lattice tools
 *
 * Implementation of the methods to wrap synthesis tools by Lattice
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "LatticeBackendFlow.hpp"

#include "config_PANDA_DATA_INSTALLDIR.hpp"

#include "DesignParameters.hpp"
#include "LatticeWrapper.hpp"
#include "Parameter.hpp"
#include "area_info.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"
#include "structural_objects.hpp"
#include "time_info.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include "xml_script_command.hpp"

#define LATTICE_SLICE "LATTICE_SLICE"
#define LATTICE_DELAY "LATTICE_DELAY"
#define LATTICE_REGISTERS "LATTICE_REGISTERS"
#define LATTICE_IOPIN "LATTICE_IOPIN"
#define LATTICE_DSP "LATTICE_DSPS"
#define LATTICE_MEM "LATTICE_MEM"

LatticeBackendFlow::LatticeBackendFlow(const ParameterConstRef _Param, const std::string& _flow_name,
                                       const generic_deviceRef _device)
    : BackendFlow(_Param, _flow_name, _device)
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, " .:: Creating Lattice Backend Flow ::.");

   default_data["LatticeECP3"] = "LatticeECP3.data";
   default_data["LatticeECP5"] = "LatticeECP5.data";
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
         device_string = "LatticeECP5";
      }
      if(default_data.find(device_string) == default_data.end())
      {
         THROW_ERROR("Device family \"" + device_string + "\" not supported!");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "Importing default scripts for target device family: " + device_string);
      parser = XMLDomParserRef(
          new XMLDomParser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/wrapper/synthesis/lattice/", true) +
                           default_data[device_string]));
   }
   parse_flow(parser);
}

LatticeBackendFlow::~LatticeBackendFlow() = default;

void LatticeBackendFlow::xparse_utilization(const std::string& fn)
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
                     if(stringID == "LATTICE_SYNTHESIS_SUMMARY")
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
   THROW_ERROR("Error during LATTICE report parsing: " + fn);
}

void LatticeBackendFlow::CheckSynthesisResults()
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Analyzing Lattice synthesis results");
   const auto report_filename = actual_parameters->parameter_values[PARAM_lattice_report];
   xparse_utilization(report_filename);

   THROW_ASSERT(design_values.find(LATTICE_SLICE) != design_values.end(), "Missing logic elements");
   area_m = area_info::factory(Param);
   area_m->set_area_value(design_values[LATTICE_SLICE]);
   area_m->set_resource_value(area_info::SLICE, design_values[LATTICE_SLICE]);
   area_m->set_resource_value(area_info::REGISTERS, design_values[LATTICE_REGISTERS]);
   area_m->set_resource_value(area_info::DSP, design_values[LATTICE_DSP]);
   area_m->set_resource_value(area_info::BRAM, design_values[LATTICE_MEM]);

   time_m = time_info::factory(Param);
   if(design_values[LATTICE_DELAY] != 0.0)
   {
      time_m->set_execution_time(design_values[LATTICE_DELAY]);
   }
   else
   {
      time_m->set_execution_time(0);
   }
}

void LatticeBackendFlow::WriteFlowConfiguration(std::ostream& script)
{
   auto setupscr = Param->isOption(OPT_lattice_settings) ? Param->getOption<std::string>(OPT_lattice_settings) : "";
   if(setupscr.size() && setupscr != "0")
   {
      script << "#configuration" << std::endl;
      if(starts_with(setupscr, "export"))
      {
         script << setupscr + " >& /dev/null; ";
      }
      else
      {
         script << ". " << setupscr << " >& /dev/null; ";
      }
      script << std::endl << std::endl;
   }
}

void LatticeBackendFlow::create_sdc(const DesignParametersRef dp)
{
   std::string clock = dp->parameter_values[PARAM_clk_name];

   std::string sdc_filename = out_dir + "/" + dp->component_name + ".ldc";
   std::ofstream sdc_file(sdc_filename.c_str());
   if(!static_cast<bool>(std::stoi(dp->parameter_values[PARAM_is_combinational])))
   {
      sdc_file << "create_clock -period " + dp->parameter_values[PARAM_clk_period] + " -name " + clock +
                      " [get_ports " + clock + "]\n";
      if((static_cast<bool>(std::stoi(dp->parameter_values[PARAM_connect_iob])) ||
          (Param->IsParameter("profile-top") && Param->GetParameter<int>("profile-top") == 1)) &&
         !Param->isOption(OPT_backend_sdc_extensions))
      {
         sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] +
                         " -from [all_inputs] -to [all_outputs]\n";
      }
   }
   else
   {
      sdc_file << "set_max_delay " + dp->parameter_values[PARAM_clk_period] + " -from [all_inputs] -to [all_outputs]\n";
   }
   if(Param->isOption(OPT_backend_sdc_extensions))
   {
      sdc_file << "source " + Param->getOption<std::string>(OPT_backend_sdc_extensions) + "\n";
   }

   sdc_file.close();
   dp->parameter_values[PARAM_sdc_file] = sdc_filename;
}

void LatticeBackendFlow::InitDesignParameters()
{
   actual_parameters->parameter_values[PARAM_target_device] = device->get_parameter<std::string>("model");
   auto device_family = device->get_parameter<std::string>("family");
   if(device_family.find('-') != std::string::npos)
   {
      device_family = device_family.substr(0, device_family.find('-'));
   }
   actual_parameters->parameter_values[PARAM_target_family] = device_family;

   std::string HDL_files = actual_parameters->parameter_values[PARAM_HDL_files];
   const auto file_list = string_to_container<std::vector<std::filesystem::path>>(HDL_files, ";");
   std::string sources_macro_list;
   bool has_vhdl_library = Param->isOption(OPT_VHDL_library);
   std::string vhdl_library;
   if(has_vhdl_library)
   {
      vhdl_library = Param->getOption<std::string>(OPT_VHDL_library);
   }
   for(const auto& file : file_list)
   {
      const auto extension = file.extension().string();
      if(extension == ".vhd" || extension == ".vhdl" || extension == ".VHD" || extension == ".VHDL")
      {
         if(has_vhdl_library)
         {
            sources_macro_list += "prj_src add -format VHDL -work " + vhdl_library + " " + file.string() + "\n";
         }
         else
         {
            sources_macro_list += "prj_src add -format VHDL " + file.string() + "\n";
         }
      }
      else if(extension == ".v" || extension == ".V" || extension == ".sv" || extension == ".SV")
      {
         sources_macro_list += "prj_src add -format VERILOG " + file.string() + "\n";
      }
      else
      {
         THROW_ERROR("Extension not recognized! " + extension);
      }
   }
   if(Param->isOption(OPT_lattice_pmi_def))
   {
      sources_macro_list += "prj_src add -format VERILOG " + Param->getOption<std::string>(OPT_lattice_pmi_def) + "\n";
   }
   actual_parameters->parameter_values[PARAM_sources_macro_list] = sources_macro_list;

   create_sdc(actual_parameters);

   for(auto& step : steps)
   {
      step->tool->EvaluateVariables(actual_parameters);
   }
}

void LatticeBackendFlow::ExecuteSynthesis()
{
   BackendFlow::ExecuteSynthesis();
}
