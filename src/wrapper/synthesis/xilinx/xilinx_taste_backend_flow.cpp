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
 *              Copyright (C) 2015-2024 Politecnico di Milano
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
 * @file xilinx_taste_backend_flow.cpp
 * @brief Wrapper to implement a synthesis tools by Xilinx targeting Taste architecture
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "xilinx_taste_backend_flow.hpp"

#include "config_GRLIB_DIR.hpp"

#include "DesignParameters.hpp"
#include "Parameter.hpp"
#include "SynthesisTool.hpp"
#include "XilinxWrapper.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"
#include "string_manipulation.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "xst_wrapper.hpp"

XilinxTasteBackendFlow::XilinxTasteBackendFlow(const ParameterConstRef& _parameters, const std::string& _flow_name,
                                               const generic_deviceRef _device)
    : XilinxBackendFlow(_parameters, _flow_name, _device)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

std::string XilinxTasteBackendFlow::GenerateSynthesisScripts(const std::string&, const structural_managerRef,
                                                             const std::list<std::string>& hdl_files,
                                                             const std::list<std::string>& aux_files)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Generating synthesis scripts");
   std::string synthesis_file_list;
   for(const auto& hdl_file : hdl_files)
   {
      synthesis_file_list += hdl_file + ";";
   }
   actual_parameters = DesignParametersRef(new DesignParameters);
   actual_parameters->component_name = "TASTE_hardware_architecture";
   if(!flow_name.empty())
   {
      actual_parameters->chain_name = flow_name;
   }

   for(const auto& aux_file : aux_files)
   {
      synthesis_file_list += aux_file + ";";
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---List of synthesis files: " + synthesis_file_list);
   actual_parameters->parameter_values[PARAM_HDL_files] = synthesis_file_list;
   const technology_managerRef TM = device->get_technology_manager();
   actual_parameters->parameter_values[PARAM_is_combinational] = STR(false);
   actual_parameters->parameter_values[PARAM_time_constrained] = STR(true);
   if(Param->isOption(OPT_clock_name))
   {
      actual_parameters->parameter_values[PARAM_clk_name] = Param->getOption<std::string>(OPT_clock_name);
   }
   else
   {
      actual_parameters->parameter_values[PARAM_clk_name] = CLOCK_PORT_NAME;
   }
   bool connect_iob = false;
   if(Param->isOption(OPT_connect_iob) && Param->getOption<bool>(OPT_connect_iob))
   {
      connect_iob = true;
   }
   actual_parameters->parameter_values[PARAM_connect_iob] = STR(connect_iob);
   if(Param->isOption(OPT_top_design_name))
   {
      actual_parameters->parameter_values[PARAM_top_id] = Param->getOption<std::string>(OPT_top_design_name);
   }
   else
   {
      actual_parameters->parameter_values[PARAM_top_id] = actual_parameters->component_name;
   }
   if(Param->isOption(OPT_backend_script_extensions))
   {
      actual_parameters->parameter_values[PARAM_has_script_extensions] = STR(true);
      actual_parameters->parameter_values[PARAM_backend_script_extensions] =
          Param->getOption<std::string>(OPT_backend_script_extensions);
   }
   else
   {
      actual_parameters->parameter_values[PARAM_has_script_extensions] = STR(false);
   }
   if(Param->isOption(OPT_VHDL_library))
   {
      actual_parameters->parameter_values[PARAM_has_VHDL_library] = STR(true);
      actual_parameters->parameter_values[PARAM_VHDL_library] = Param->getOption<std::string>(OPT_VHDL_library);
   }
   else
   {
      actual_parameters->parameter_values[PARAM_has_VHDL_library] = STR(false);
   }

   InitDesignParameters();

   const auto ret = CreateScripts(actual_parameters);

   /// Copying GRLIB
   std::error_code ec;
   std::filesystem::copy(
       relocate_install_path(GRLIB_DIR), std::filesystem::current_path() / std::filesystem::path(GRLIB_DIR).filename(),
       std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
   if(ec)
   {
      THROW_ERROR("Error copying GRLIB: " + ec.message());
   }

   /// Modifying xst project adding grlib files
   if(actual_parameters->parameter_values.find(PARAM_xst_prj_file) != actual_parameters->parameter_values.end())
   {
      const auto output_temporary_directory = Param->getOption<std::string>(OPT_output_temporary_directory);
      std::stringstream new_prj_stream;
      new_prj_stream << "vhdl grlib GRLIB/grlib/stdlib/version.vhd\n"
                        "vhdl grlib GRLIB/grlib/stdlib/stdlib.vhd\n"
                        "vhdl grlib GRLIB/grlib/amba/amba.vhd\n"
                        "vhdl techmap GRLIB/techmap/gencomp/gencomp.vhd\n"
                        "vhdl grlib GRLIB/grlib/amba/devices.vhd\n"
                        "vhdl techmap GRLIB/techmap/unisim/pads_unisim.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/allpads.vhd\n"
                        "vhdl gaisler GRLIB/gaisler/misc/misc.vhd\n"
                        "vhdl techmap GRLIB/techmap/unisim/clkgen_unisim.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/toutpad.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/outpad.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/odpad.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/iopad.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/iodpad.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/inpad.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/allclkgen.vhd\n"
                        "vhdl gaisler GRLIB/gaisler/pci/pci.vhd\n"
                        "vhdl gaisler GRLIB/gaisler/misc/ahbmst.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/clkpad.vhd\n"
                        "vhdl techmap GRLIB/techmap/maps/clkgen.vhd\n"
                        "vhdl grlib GRLIB/grlib/amba/apbctrl.vhd\n"
                        "vhdl grlib GRLIB/grlib/amba/ahbctrl.vhd\n"
                        "vhdl gaisler GRLIB/gaisler/pci/pci_target.vhd\n"
                        "vhdl gaisler GRLIB/gaisler/pci/pcipads.vhd\n"
                        "vhdl gaisler GRLIB/gaisler/misc/rstgen.vhd\n";
      const auto xst_prj_file = actual_parameters->parameter_values.at(PARAM_xst_prj_file);
      new_prj_stream << std::ifstream(xst_prj_file).rdbuf();
      std::ofstream(xst_prj_file, std::ios_base::trunc) << new_prj_stream.rdbuf();
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generated synthesis scripts");
   return ret;
}

void XilinxTasteBackendFlow::create_cf(const DesignParametersRef dp, bool xst)
{
   std::string ucf_filename = UCF_SUBDIR + dp->component_name + (xst ? ".xcf" : ".ucf");
   std::ofstream UCF_file(ucf_filename);
   UCF_file << R"(CONFIG STEPPING="0";
   
NET resetn TIG ;

NET "clk" PERIOD = 20.000 ;

NET "pci_clk" PERIOD = 30.000 ;
OFFSET = OUT : 11.000 : AFTER pci_clk ;
OFFSET = IN : 7.000 : BEFORE pci_clk ;

NET "clk"        LOC = "P20"  | IOSTANDARD=LVTTL;
NET "pci_clk"    LOC = "AK19" | IOSTANDARD=LVTTL;

NET "pllref"     LOC = "J19"  | IOSTANDARD=LVTTL;

NET "resetn"     LOC = "G38"  | IOSTANDARD=LVTTL;

NET "pci_ad<0>"  LOC = "AW16" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<1>"  LOC = "AV17" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<2>"  LOC = "AW15" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<3>"  LOC = "AV15" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<4>"  LOC = "AU18" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<5>"  LOC = "AW17" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<6>"  LOC = "AT18" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<7>"  LOC = "AP16" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<8>"  LOC = "AU17" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<9>"  LOC = "AT16" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<10>" LOC = "AU16" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<11>" LOC = "AT15" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<12>" LOC = "AU15" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<13>" LOC = "AR14" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<14>" LOC = "AT14" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<15>" LOC = "AU13" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<16>" LOC = "AT8"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<17>" LOC = "AU8"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<18>" LOC = "AT9"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<19>" LOC = "AU6"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<20>" LOC = "AR8"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<21>" LOC = "AU7"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<22>" LOC = "AU5"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<23>" LOC = "AR7"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<24>" LOC = "AW7"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<25>" LOC = "AV7"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<26>" LOC = "AW6"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<27>" LOC = "AW5"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<28>" LOC = "AV5"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<29>" LOC = "AW4"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<30>" LOC = "AV4"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_ad<31>" LOC = "AV3"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_cbe<0>" LOC = "AT13" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_cbe<1>" LOC = "AU12" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_cbe<2>" LOC = "AR13" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_cbe<3>" LOC = "AR12" | IOSTANDARD=PCI33_3 | BYPASS;

NET "pci_66"     LOC = "AW14" | IOSTANDARD=LVTTL;
NET "pci_host"   LOC = "AV14" | IOSTANDARD=LVTTL;
NET "pci_devsel" LOC = "AV10" | IOSTANDARD=PCI33_3 | BYPASS; # the PCI spec calls this devseln
NET "pci_frame"  LOC = "AR9"  | IOSTANDARD=PCI33_3 | BYPASS; # the PCI spec calls this framen
NET "pci_gnt"    LOC = "AV13" | IOSTANDARD=LVTTL; # the PCI spec calls this gntn
NET "pci_req"    LOC = "AW12" | IOSTANDARD=LVTTL; # the PCI spec calls this reqn
   
NET "pci_idsel"  LOC = "AV9"  | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_irdy"   LOC = "AW9"  | IOSTANDARD=PCI33_3 | BYPASS; # the PCI spec calls this" irdyn
NET "pci_lock"   LOC = "AU11" | IOSTANDARD=PCI33_3 | BYPASS; # the PCI spec calls this" lockn
NET "pci_par"    LOC = "AW11" | IOSTANDARD=PCI33_3 | BYPASS;
NET "pci_perr"   LOC = "AW10" | IOSTANDARD=PCI33_3 | BYPASS; # the PCI spec calls this perrn
NET "pci_rst"    LOC = "AV8"  | IOSTANDARD=LVTTL; # the PCI spec calls this rstn
NET "pci_serr"   LOC = "AT11" | IOSTANDARD=PCI33_3; # the PCI spec calls this serrn
NET "pci_stop"   LOC = "AV12" | IOSTANDARD=PCI33_3 | BYPASS; # the PCI spec calls this stopn
NET "pci_trdy"   LOC = "AU10" | IOSTANDARD=PCI33_3 | BYPASS; # the PCI spec calls this trdyn)";
   UCF_file.close();
   if(xst)
   {
      dp->parameter_values[PARAM_xcf_file] = ucf_filename;
   }
   else
   {
      dp->parameter_values[PARAM_ucf_file] = ucf_filename;
   }
}
