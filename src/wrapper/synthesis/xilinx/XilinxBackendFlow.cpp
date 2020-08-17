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
 * @file XilinxBackendFlow.cpp
 * @brief Implementation of the wrapper to Xilinx tools
 *
 * Implementation of the methods to wrap synthesis tools by Xilinx
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
/// Header include
#include "XilinxBackendFlow.hpp"

#include "config_PANDA_DATA_INSTALLDIR.hpp"
#include "config_XILINX_SETTINGS.hpp"
#include "config_XILINX_VIVADO_SETTINGS.hpp"

/// constants include
#include "synthesis_constants.hpp"

#include "LUT_model.hpp"
#include "area_model.hpp"
#include "clb_model.hpp"
#include "map_wrapper.hpp"
#include "target_device.hpp"
#include "target_manager.hpp"
#include "time_model.hpp"
#include "trce_wrapper.hpp"

#include "XilinxWrapper.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "xml_dom_parser.hpp"
#include "xml_script_command.hpp"

/// circuit include
#include "structural_objects.hpp"

#include "string_manipulation.hpp" // for GET_CLASS

#define XST_NUMBER_OF_SLICE_REGISTERS "XST_NUMBER_OF_SLICE_REGISTERS"
#define XST_NUMBER_OF_SLICE_LUTS "XST_NUMBER_OF_SLICE_LUTS"
#define XST_NUMBER_OF_LUT_FLIP_FLOP_PAIRS_USED "XST_NUMBER_OF_LUT_FLIP_FLOP_PAIRS_USED"
#define XST_NUMBER_OF_BLOCK_RAMFIFO "XST_NUMBER_OF_BLOCK_RAMFIFO"

#define VIVADO_XILINX_LUT_FLIP_FLOP_PAIRS_USED "XILINX_LUT_FLIP_FLOP_PAIRS_USED"
#define VIVADO_XILINX_SLICE "XILINX_SLICE"
#define VIVADO_XILINX_SLICE_REGISTERS "XILINX_SLICE_REGISTERS"
#define VIVADO_XILINX_SLICE_LUTS "XILINX_SLICE_LUTS"
#define VIVADO_XILINX_BLOCK_RAMFIFO "XILINX_BLOCK_RAMFIFO"
#define VIVADO_XILINX_IOPIN "XILINX_IOPIN"
#define VIVADO_XILINX_DSPS "XILINX_DSPS"
#define VIVADO_XILINX_OUTPUT "XILINX_OUTPUT"
#define VIVADO_XILINX_POWER "XILINX_POWER"
#define VIVADO_XILINX_DESIGN_DELAY "XILINX_DESIGN_DELAY"

XilinxBackendFlow::XilinxBackendFlow(const ParameterConstRef _Param, const std::string& _flow_name, const target_managerRef _target) : BackendFlow(_Param, _flow_name, _target)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Creating Xilinx Backend Flow ::.");
   boost::filesystem::create_directories(UCF_SUBDIR);

   default_data["Virtex-4"] = "Virtex-4.data";
#if HAVE_TASTE
   default_data["Virtex-4-Taste"] = "Virtex-4-Taste.data";
#endif
   default_data["Virtex-5"] = "Virtex-5.data";
   default_data["Virtex-6"] = "Virtex-6.data";
   default_data["Virtex-7"] = "Virtex-7.data";
   default_data["Virtex-7-VVD"] = "Virtex-7-VVD.data";
   default_data["Artix-7-VVD"] = "Artix-7-VVD.data";
   default_data["Zynq-VVD"] = "Zynq-VVD.data";
   default_data["Zynq-YOSYS-VVD"] = "Zynq-YOSYS-VVD.data";
   default_data["Zynq"] = "Zynq.data";

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
         device_string = "Zynq-VVD";
#if HAVE_TASTE
      if(Param->isOption(OPT_generate_taste_architecture) and Param->getOption<bool>(OPT_generate_taste_architecture))
      {
         device_string = device_string + "-Taste";
      }
#endif
      if(default_data.find(device_string) == default_data.end())
         THROW_ERROR("Device family \"" + device_string + "\" not supported!");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Importing default scripts for target device family: " + device_string);
      parser = XMLDomParserRef(new XMLDomParser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/wrapper/synthesis/xilinx/") + default_data[device_string]));
   }
   parse_flow(parser);
}

XilinxBackendFlow::~XilinxBackendFlow() = default;

void XilinxBackendFlow::xparse_map_utilization(const std::string& fn)
{
   std::ifstream output_file(fn.c_str());
   if(output_file.is_open())
   {
      while(!output_file.eof())
      {
         std::string line;
         getline(output_file, line);
         if(line.size() and line.find("Number of Slice Registers:") != std::string::npos)
         {
            std::string tk = "Number of Slice Registers:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::REGISTERS, boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of Slice Flip Flops:") != std::string::npos)
         {
            std::string tk = "Number of Slice Flip Flops:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::REGISTERS, boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of 4 input LUTs:") != std::string::npos)
         {
            std::string tk = "Number of 4 input LUTs:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::SLICE_LUTS, boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of Slice LUTs:") != std::string::npos)
         {
            std::string tk = "Number of Slice LUTs:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::SLICE_LUTS, boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of occupied Slices:") != std::string::npos)
         {
            std::string tk = "Number of occupied Slices:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::SLICE, boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of LUT Flip Flop pairs used:") != std::string::npos)
         {
            std::string tk = "Number of LUT Flip Flop pairs used:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::LUT_FF_PAIRS, boost::lexical_cast<unsigned int>(token));
            clb_m->set_area_value(boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of DSP48Es:") != std::string::npos)
         {
            std::string tk = "Number of DSP48Es:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::DSP, boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of DSP48E1s:") != std::string::npos)
         {
            std::string tk = "Number of DSP48E1s:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::DSP, boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of BlockRAM/FIFO:") != std::string::npos)
         {
            std::string tk = "Number of BlockRAM/FIFO:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::BRAM, boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of RAMB36E1/FIFO36E1s:") != std::string::npos)
         {
            std::string tk = "Number of RAMB36E1/FIFO36E1s:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::BRAM, boost::lexical_cast<unsigned int>(token));
         }
         else if(line.size() and line.find("Number of FIFO16/RAMB16s:") != std::string::npos)
         {
            std::string tk = "Number of FIFO16/RAMB16s:";
            std::string token = line.substr(line.find(tk) + tk.size() + 1, line.length());
            boost::algorithm::trim(token);
            token = token.substr(0, token.find(" "));
            boost::algorithm::trim(token);
            boost::replace_all(token, ",", "");
            if(!area_m)
               area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
            auto* clb_m = GetPointer<clb_model>(area_m);
            clb_m->set_resource_value(clb_model::BRAM, boost::lexical_cast<unsigned int>(token));
         }
      }
   }
}

void XilinxBackendFlow::xparse_xst_utilization(const std::string& fn)
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
                     if(stringID == "XST_DEVICE_UTILIZATION_SUMMARY")
                     {
                        const xml_node::node_list list_item = nodeS->get_children();
                        for(const auto& it_item : list_item)
                        {
                           const auto* nodeIt = GetPointer<const xml_element>(it_item);
                           if(!nodeIt or nodeIt->get_name() != "item")
                              continue;

                           if(CE_XVM(stringID, nodeIt))
                              LOAD_XVM(stringID, nodeIt);

                           if(stringID != "XST_SELECTED_DEVICE")
                           {
                              std::string value;
                              if(CE_XVM(value, nodeIt))
                              {
                                 LOAD_XVM(value, nodeIt);
                                 boost::replace_all(value, ",", "");
                                 design_values[stringID] = boost::lexical_cast<unsigned int>(value);
                              }
                           }
                        }
                     }
                  }
               }
            }
         }

         area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
         auto* clb_m = GetPointer<clb_model>(area_m);
         if(design_values.find(XST_NUMBER_OF_SLICE_REGISTERS) != design_values.end())
            clb_m->set_resource_value(clb_model::REGISTERS, design_values[XST_NUMBER_OF_SLICE_REGISTERS]);
         if(design_values.find(XST_NUMBER_OF_SLICE_LUTS) != design_values.end())
            clb_m->set_resource_value(clb_model::SLICE_LUTS, design_values[XST_NUMBER_OF_SLICE_LUTS]);
         if(design_values.find(XST_NUMBER_OF_BLOCK_RAMFIFO) != design_values.end())
            clb_m->set_resource_value(clb_model::BRAM, design_values[XST_NUMBER_OF_BLOCK_RAMFIFO]);

         /// setting the global area occupation as the number of LUT/FF pairs when possible
         if(design_values.find(XST_NUMBER_OF_LUT_FLIP_FLOP_PAIRS_USED) == design_values.end())
            design_values[XST_NUMBER_OF_LUT_FLIP_FLOP_PAIRS_USED] = 0; // it may happen when the component will be completely "destroyed" by the logic synthesisstep

         clb_m->set_resource_value(clb_model::LUT_FF_PAIRS, design_values[XST_NUMBER_OF_LUT_FLIP_FLOP_PAIRS_USED]);
         area_m->set_area_value(design_values[XST_NUMBER_OF_LUT_FLIP_FLOP_PAIRS_USED]);

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
   THROW_ERROR("Error during XST report parsing: " + fn);
}

void XilinxBackendFlow::parse_timing(const std::string& log_file)
{
   std::ifstream output_file(log_file.c_str());
   if(output_file.is_open())
   {
      while(!output_file.eof())
      {
         std::string line;
         getline(output_file, line);
         if(line.size() and line.find("Minimum period:") != std::string::npos)
         {
            if(line.find("No path found") != std::string::npos)
            {
               time_m = time_model::create_model(TargetDevice_Type::FPGA, Param);
               auto* lut_m = GetPointer<LUT_model>(time_m);
               lut_m->set_timing_value(LUT_model::COMBINATIONAL_DELAY, 0.0);
            }
            else
            {
               std::string token("Minimum period:");
               std::string tk = line.substr(line.find(token) + token.size() + 1, line.size());
               boost::trim(tk);
               tk = tk.substr(0, tk.find_first_of(' '));
               boost::replace_all(tk, "ns", "");
               time_m = time_model::create_model(TargetDevice_Type::FPGA, Param);
               auto* lut_m = GetPointer<LUT_model>(time_m);
               lut_m->set_timing_value(LUT_model::COMBINATIONAL_DELAY, boost::lexical_cast<double>(tk));
               if(boost::lexical_cast<double>(tk) > Param->getOption<double>(OPT_clock_period))
               {
                  CopyFile(Param->getOption<std::string>(OPT_output_directory) + "/Synthesis/xst/" + actual_parameters->component_name + ".log",
                           Param->getOption<std::string>(OPT_output_directory) + "/" + flow_name + "/" + STR_CST_synthesis_timing_violation_report);
               }
            }
         }
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, line);
      }
      if(!time_m)
      {
         THROW_WARNING("something of wrong happen with synthesis. It may happen in some corner cases.");
         time_m = time_model::create_model(TargetDevice_Type::FPGA, Param);
      }
   }
}

void XilinxBackendFlow::parse_DSPs(const std::string& log_file)
{
   std::ifstream output_file(log_file.c_str());
   if(output_file.is_open())
   {
      while(!output_file.eof())
      {
         std::string line;
         getline(output_file, line);
         if(line.size() and line.find("Number of DSP48Es:") != std::string::npos)
         {
            std::string token("Number of DSP48Es:");
            std::string tk = line.substr(line.find(token) + token.size() + 1, line.size());
            boost::trim(tk);
            tk = tk.substr(0, tk.find_first_of(' '));
            auto* clb_m = GetPointer<clb_model>(area_m);
            THROW_ASSERT(clb_m, "missing area model");
            clb_m->set_resource_value(clb_model::DSP, boost::lexical_cast<double>(tk));
         }
      }
   }
}

void XilinxBackendFlow::xparse_timing(const std::string& fn, bool post)
{
   try
   {
      XMLDomParser parser(fn);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         const xml_text_node* child_text = [&]() -> const xml_text_node* {
            THROW_ASSERT(node->get_name() == "twReport", "Wrong root name: " + node->get_name());

            const xml_node::node_list list_int = node->get_children();
            for(const auto& iter_int : list_int)
            {
               const auto* EnodeC = GetPointer<const xml_element>(iter_int);
               if(!EnodeC)
                  continue;
               if(flow_name == "Characterization" and EnodeC->get_name() == "twBody" && EnodeC->CGetDescendants("twErrRpt/twConst/twPathRpt/twConstPath/twSlack").size())
               {
                  const auto tw_slacks = EnodeC->CGetDescendants("twErrRpt/twConst/twPathRpt/twConstPath/twSlack");
                  if(tw_slacks.size() == 0)
                     THROW_ERROR("Pattern not found in trce report");
                  if(tw_slacks.size() > 1)
                     THROW_ERROR("Found multiple twSlack fields");
                  const auto tw_slack = *(tw_slacks.begin());
                  THROW_ASSERT(GetPointer<const xml_element>(tw_slack), "");
                  return GetPointer<const xml_element>(tw_slack)->get_child_text();
               }
               else if(EnodeC->get_name() == "twSum")
               {
                  const xml_node::node_list list = EnodeC->get_children();
                  for(const auto& iter : list)
                  {
                     const auto* Enode = GetPointer<const xml_element>(iter);
                     if(!Enode)
                        continue;
                     if(Enode->get_name() == "twStats")
                     {
                        const xml_node::node_list listS = Enode->get_children();
                        for(const auto& iterS : listS)
                        {
                           const auto* EnodeS = GetPointer<const xml_element>(iterS);
                           if(!EnodeS)
                              continue;
                           if(EnodeS->get_name() == "twMinPer" or EnodeS->get_name() == "twMaxCombDel")
                           {
                              return EnodeS->get_child_text();
                           }
                        }
                     }
                  }
               }
            }
            THROW_UNREACHABLE("");
            return nullptr;
         }();
         double period = std::abs(boost::lexical_cast<double>(child_text->get_content()));
         time_m = time_model::create_model(TargetDevice_Type::FPGA, Param);
         auto* lut_m = GetPointer<LUT_model>(time_m);
         if(post)
         {
            lut_m->set_timing_value(LUT_model::MINIMUM_PERIOD_POST_PAR, period);
         }
         else
         {
            lut_m->set_timing_value(LUT_model::MINIMUM_PERIOD_POST_MAP, period);
         }
         return;
      }
   }
   catch(const char* msg)
   {
      THROW_ERROR("Error during TRCE report (" + fn + ") :" + *msg);
   }
   catch(const std::string& msg)
   {
      THROW_ERROR("Error during TRCE report (" + fn + ") :" + msg);
   }
   catch(const std::exception& ex)
   {
      THROW_ERROR("Error during TRCE report (" + fn + ") :" + ex.what());
   }
   catch(...)
   {
      THROW_ERROR("Error during TRCE report (" + fn + ") :" + "Unknown exception");
   }
}

void XilinxBackendFlow::CheckSynthesisResults()
{
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Analyzing Xilinx synthesis results");
   bool is_vivado = false;
   const target_deviceRef device = target->get_target_device();
   std::string device_string;
   device_string = device->get_parameter<std::string>("family");
   if(device_string.find("-VVD") != std::string::npos)
      is_vivado = true;

   if(!is_vivado)
   {
      if(boost::filesystem::exists(actual_parameters->parameter_values[PARAM_map_report]))
      {
         xparse_map_utilization(actual_parameters->parameter_values[PARAM_map_report]);
      }
      else if(boost::filesystem::exists(actual_parameters->parameter_values[PARAM_xst_report]))
      {
         xparse_xst_utilization(actual_parameters->parameter_values[PARAM_xst_report]);
         if(actual_parameters->parameter_values.find(PARAM_xst_log_file) != actual_parameters->parameter_values.end() && boost::filesystem::exists(actual_parameters->parameter_values[PARAM_xst_log_file]))
            parse_DSPs(actual_parameters->parameter_values[PARAM_xst_log_file]);
      }
      else
         THROW_ERROR("the script does not have a synthesis step");

      if(actual_parameters->parameter_values.find(PARAM_trce_report_post) != actual_parameters->parameter_values.end() && boost::filesystem::exists(actual_parameters->parameter_values[PARAM_trce_report_post]))
      {
         xparse_timing(actual_parameters->parameter_values[PARAM_trce_report_post], true);
      }
      else if(actual_parameters->parameter_values.find(PARAM_trce_report_pre) != actual_parameters->parameter_values.end() && boost::filesystem::exists(actual_parameters->parameter_values[PARAM_trce_report_pre]))
      {
         xparse_timing(actual_parameters->parameter_values[PARAM_trce_report_pre], false);
      }
      else if(actual_parameters->parameter_values.find(PARAM_xst_log_file) != actual_parameters->parameter_values.end() && boost::filesystem::exists(actual_parameters->parameter_values[PARAM_xst_log_file]))
      {
         parse_timing(actual_parameters->parameter_values[PARAM_xst_log_file]);
      }
      else
         THROW_ERROR("the script does not have a timing evaluation step");
   }
   else
   {
      std::string report_filename = actual_parameters->parameter_values[PARAM_vivado_report];
      vivado_xparse_utilization(report_filename);
      area_m = area_model::create_model(TargetDevice_Type::FPGA, Param);
      area_m->set_area_value(design_values[VIVADO_XILINX_SLICE_LUTS]);
      auto* area_clb_model = GetPointer<clb_model>(area_m);
      if(design_values[VIVADO_XILINX_LUT_FLIP_FLOP_PAIRS_USED] != 0.0)
         area_clb_model->set_resource_value(clb_model::LUT_FF_PAIRS, design_values[VIVADO_XILINX_LUT_FLIP_FLOP_PAIRS_USED]);

      if(design_values[VIVADO_XILINX_SLICE_LUTS] != 0.0)
         area_clb_model->set_resource_value(clb_model::SLICE_LUTS, design_values[VIVADO_XILINX_SLICE_LUTS]);

      area_clb_model->set_resource_value(clb_model::SLICE, design_values[VIVADO_XILINX_SLICE]);
      area_clb_model->set_resource_value(clb_model::REGISTERS, design_values[VIVADO_XILINX_SLICE_REGISTERS]);
      area_clb_model->set_resource_value(clb_model::DSP, design_values[VIVADO_XILINX_DSPS]);
      area_clb_model->set_resource_value(clb_model::BRAM, design_values[VIVADO_XILINX_BLOCK_RAMFIFO]);

      time_m = time_model::create_model(TargetDevice_Type::FPGA, Param);
      auto* lut_m = GetPointer<LUT_model>(time_m);
      if(design_values[VIVADO_XILINX_DESIGN_DELAY] != 0.0)
      {
         lut_m->set_timing_value(LUT_model::COMBINATIONAL_DELAY, design_values[VIVADO_XILINX_DESIGN_DELAY]);
         if(design_values[VIVADO_XILINX_DESIGN_DELAY] > Param->getOption<double>(OPT_clock_period) and actual_parameters->parameter_values.find(PARAM_vivado_timing_report) != actual_parameters->parameter_values.end() and
            ExistFile(actual_parameters->parameter_values.find(PARAM_vivado_timing_report)->second))
         {
            CopyFile(actual_parameters->parameter_values[PARAM_vivado_timing_report], Param->getOption<std::string>(OPT_output_directory) + "/" + flow_name + "/" + STR_CST_synthesis_timing_violation_report);
         }
      }
      else
         lut_m->set_timing_value(LUT_model::COMBINATIONAL_DELAY, 0);
   }
   if((output_level >= OUTPUT_LEVEL_VERY_PEDANTIC or (Param->IsParameter("DumpingTimingReport") and Param->GetParameter<int>("DumpingTimingReport"))) and
      ((actual_parameters->parameter_values.find(PARAM_vivado_timing_report) != actual_parameters->parameter_values.end() and ExistFile(actual_parameters->parameter_values.find(PARAM_vivado_timing_report)->second))))
   {
      CopyStdout(actual_parameters->parameter_values.find(PARAM_vivado_timing_report)->second);
   }
}

void XilinxBackendFlow::vivado_xparse_utilization(const std::string& fn)
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
                     if(stringID == "XILINX_SYNTHESIS_SUMMARY")
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
   THROW_ERROR("Error during VIVADO report parsing: " + fn);
}

void XilinxBackendFlow::WriteFlowConfiguration(std::ostream& script)
{
   std::string setupscr;
   const target_deviceRef device = target->get_target_device();
   std::string device_string;
   device_string = device->get_parameter<std::string>("family");
   if(device_string.find("-VVD") != std::string::npos)
      setupscr = STR(XILINX_VIVADO_SETTINGS);
   else
      setupscr = STR(XILINX_SETTINGS);
   if(setupscr.size() and setupscr != "0")
   {
      script << "#configuration" << std::endl;
      if(boost::algorithm::starts_with(setupscr, "export"))
         script << setupscr + " >& /dev/null; ";
      else
         script << ". " << setupscr << " >& /dev/null; ";
      script << std::endl << std::endl;
   }
}

void XilinxBackendFlow::ExecuteSynthesis()
{
   BackendFlow::ExecuteSynthesis();
}

void XilinxBackendFlow::InitDesignParameters()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->XilinxBackendFlow - Init Design Parameters");

   std::string ise_style;
   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
      ise_style = std::string(INTSTYLE_ISE);
   else
      ise_style = std::string(INTSTYLE_SILENT);
   actual_parameters->parameter_values[PARAM_ise_style] = ise_style;

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

   bool is_vivado = false;
   if(device->get_parameter<std::string>("family").find("-VVD") != std::string::npos)
      is_vivado = true;

   if(is_vivado)
   {
      /// Vivado section
      std::string HDL_files = actual_parameters->parameter_values[PARAM_HDL_files];
      std::vector<std::string> file_list = convert_string_to_vector<std::string>(HDL_files, ";");
      std::string sources_macro_list;
      bool has_vhdl_library = Param->isOption(OPT_VHDL_library);
      std::string vhdl_library;
      if(has_vhdl_library)
         vhdl_library = Param->getOption<std::string>(OPT_VHDL_library);
      for(unsigned int v = 0; v < file_list.size(); v++)
      {
         if(v)
            sources_macro_list += "\n";
         boost::filesystem::path file_path(file_list[v]);
         std::string extension = GetExtension(file_path);
         if(extension == "vhd" || extension == "vhdl" || extension == "VHD" || extension == "VHDL")
         {
            if(has_vhdl_library)
               sources_macro_list += "read_vhdl -library " + vhdl_library + " " + file_list[v];
            else
               sources_macro_list += "read_vhdl " + file_list[v];
         }
         else if(extension == "v" || extension == "V")
            sources_macro_list += "read_verilog " + file_list[v];
         else if(extension == "sv" || extension == "SV")
            sources_macro_list += "read_verilog -sv " + file_list[v];
         else
            THROW_ERROR("Extension not recognized! " + extension);
      }
      /// additional constraints have to be manage at this level
      if(Param->isOption(OPT_backend_sdc_extensions))
         sources_macro_list += "\nread_xdc " + Param->getOption<std::string>(OPT_backend_sdc_extensions);

      actual_parameters->parameter_values[PARAM_vivado_sources_macro_list] = sources_macro_list;

      if(device->get_parameter<std::string>("family").find("-YOSYS-VVD") != std::string::npos)
      {
         sources_macro_list = "";
         for(unsigned int v = 0; v < file_list.size(); v++)
         {
            if(v)
               sources_macro_list += " -p ";
            boost::filesystem::path file_path(file_list[v]);
            std::string extension = GetExtension(file_path);
            if(extension == "v" || extension == "V")
               sources_macro_list += "\\\"read_verilog -defer " + file_list[v] + "\\\"";
            else if(extension == "sv" || extension == "SV")
               sources_macro_list += "\\\"read_verilog -sv -defer " + file_list[v] + "\\\"";
            else
               THROW_ERROR("Extension not recognized! " + extension);
         }
         actual_parameters->parameter_values[PARAM_yosys_vivado_sources_macro_list] = sources_macro_list;
      }
   }
   else
   {
      create_cf(actual_parameters, true);
      create_cf(actual_parameters, false);
   }
   for(auto& step : steps)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Evaluating variables of step " + step->name);
      step->tool->EvaluateVariables(actual_parameters);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Evaluated variables of step " + step->name);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--XilinxBackendFlow - Init Design Parameters");
}

void XilinxBackendFlow::create_cf(const DesignParametersRef dp, bool xst)
{
   std::string ucf_filename = UCF_SUBDIR + dp->component_name + (xst ? ".xcf" : ".ucf");
   std::ofstream UCF_file(ucf_filename.c_str());
   THROW_ASSERT(dp->parameter_values.find(PARAM_clk_name) != dp->parameter_values.end(), "");
   if(!boost::lexical_cast<bool>(dp->parameter_values[PARAM_is_combinational]))
   {
      UCF_file << "NET \"" << dp->parameter_values[PARAM_clk_name] << "\" TNM_NET = " << dp->parameter_values[PARAM_clk_name] << ";" << std::endl;
      if(xst)
      {
         UCF_file << "BEGIN MODEL \"" << dp->component_name << "\"" << std::endl;
         UCF_file << "NET \"" << dp->parameter_values[PARAM_clk_name] << "\" buffer_type=bufgp;" << std::endl;
         UCF_file << "END;" << std::endl;
      }
      UCF_file << "TIMESPEC TS_" << dp->parameter_values[PARAM_clk_name] << " = PERIOD \"" << dp->parameter_values[PARAM_clk_name] << "\" " << dp->parameter_values[PARAM_clk_period] << " ns HIGH 50%;" << std::endl;
   }
   else if(Param->isOption(OPT_connect_iob) && not Param->getOption<bool>(OPT_connect_iob))
   {
      THROW_ERROR("ISE needs IOB to perform timing analysis of combinational circuit");
   }

   UCF_file.close();
   if(xst)
      dp->parameter_values[PARAM_xcf_file] = ucf_filename;
   else
      dp->parameter_values[PARAM_ucf_file] = ucf_filename;
}
