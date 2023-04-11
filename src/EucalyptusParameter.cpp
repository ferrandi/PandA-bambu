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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file EucalyptusParameter.cpp
 * @brief This file contains the implementation of some methods for parameter parsing in Eucalyptus tool
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "EucalyptusParameter.hpp"

#include "language_writer.hpp"
#include "target_device.hpp"

#define TOOL_OPT_BASE 256
#define INPUT_OPT_CHARACTERIZE (1 + TOOL_OPT_BASE)
#define INPUT_OPT_TARGET_DATAFILE (1 + INPUT_OPT_CHARACTERIZE)
#define INPUT_OPT_TARGET_SCRIPTFILE (1 + INPUT_OPT_TARGET_DATAFILE)
#define OPT_LATTICE_ROOT (1 + INPUT_OPT_TARGET_SCRIPTFILE)
#define OPT_XILINX_ROOT (1 + OPT_LATTICE_ROOT)
#define OPT_MENTOR_ROOT (1 + OPT_XILINX_ROOT)
#define OPT_MENTOR_OPTIMIZER (1 + OPT_MENTOR_ROOT)
#define OPT_ALTERA_ROOT (1 + OPT_MENTOR_OPTIMIZER)
#define OPT_NANOXPLORE_ROOT (1 + OPT_ALTERA_ROOT)
#define OPT_NANOXPLORE_BYPASS (1 + OPT_NANOXPLORE_ROOT)
#define OPT_PARALLEL_BACKEND (1 + OPT_NANOXPLORE_BYPASS)

#include "utility.hpp"
#include "utility/fileIO.hpp"
#include <boost/filesystem.hpp>
#include <getopt.h>

void EucalyptusParameter::PrintProgramName(std::ostream& os) const
{
   os << "" << std::endl;
   os << "********************************************************************************" << std::endl;
   os << "                _____                _             _" << std::endl;
   os << "               | ____|   _  ___ __ _| |_   _ _ __ | |_ _   _ ___" << std::endl;
   os << "               |  _|| | | |/ __/ _` | | | | | '_ \\| __| | | / __|" << std::endl;
   os << "               | |__| |_| | (_| (_| | | |_| | |_) | |_| |_| \\__ \\" << std::endl;
   os << R"(               |_____\__,_|\___\__,_|_|\__, | .__/ \__|\__,_|___/)" << std::endl;
   os << "                                       |___/|_|" << std::endl;
   os << "********************************************************************************" << std::endl;
}

void EucalyptusParameter::PrintHelp(std::ostream& os) const
{
   os << "Usage: " << std::endl;
   os << "       " << getOption<std::string>(OPT_program_name) << " [options]" << std::endl;
   os << "\n"
      << "Options: \n";
   PrintGeneralOptionsUsage(os);
   os << "\n"
      << "  Library Estimation:\n"
      << "    --target-device=<file>          Specify the type of the device, separated by commas (e.g.,: "
         "\"xc7z020,-1,clg484\")\n"
      << "    --target-datafile=file          Specify a data XML file describing some defaults value for the target "
         "device.\n"
      << "    --target-scriptfile=file        Specify a script XML file including the scripts for the synthesis w.r.t. "
         "the target device.\n"
      << "    --clock-period=value            Specify the period of the clock signal (default 10 nanoseconds)\n"
      << "    --characterize=<component_name> Characterize the given component\n"
      << std::endl;
   // options defining where backend tools could be found
   os << "  Backend configuration:\n\n"
      << "    --mentor-visualizer\n"
      << "        Simulate the RTL implementation and then open Mentor Visualizer.\n"
      << "        (Mentor root has to be correctly set, see --mentor-root)\n\n"
      << "    --mentor-optimizer=<0|1>\n"
      << "        Enable or disable mentor optimizer. (default=enabled)\n\n"
      << "    --nanoxplore-bypass=<name>\n"
      << "        Define NanoXplore bypass when using NanoXplore. User may set NANOXPLORE_BYPASS\n"
      << "        variable otherwise.\n\n"
      << "    --altera-root=<path>\n"
      << "        Define Altera tools path. Given path is searched for Quartus.\n"
      << "        (default=/opt/altera:/opt/intelFPGA)\n\n"
      << "    --lattice-root=<path>\n"
      << "        Define Lattice tools path. Given path is searched for Diamond.\n"
      << "        (default=/opt/diamond:/usr/local/diamond)\n\n"
      << "    --mentor-root=<path>\n"
      << "        Define Mentor tools path. Given directory is searched for Modelsim and Visualizer\n"
      << "        (default=/opt/mentor)\n\n"
      << "    --nanoxplore-root=<path>\n"
      << "        Define NanoXplore tools path. Given directory is searched for NXMap.\n"
      << "        (default=/opt/NanoXplore/NXMap3)\n\n"
      << "    --xilinx-root=<path>\n"
      << "        Define Xilinx tools path. Given directory is searched for both ISE and Vivado\n"
      << "        (default=/opt/Xilinx)\n\n"
      << std::endl;
}

EucalyptusParameter::EucalyptusParameter(const std::string& _program_name, int _argc, char** const _argv)
    : Parameter(_program_name, _argc, _argv)
{
   SetDefaults();
}

int EucalyptusParameter::Exec()
{
   exit_code = PARAMETER_NOTPARSED;

   /// variable used into option parsing
   int option_index;

   // Short option. An option character in this string can be followed by a colon (`:') to indicate that it
   // takes a required argument. If an option character is followed by two colons (`::'), its argument is optional;
   // this is a GNU extension.
   const char* const short_options = COMMON_SHORT_OPTIONS_STRING "w:";

   const struct option long_options[] = {COMMON_LONG_OPTIONS,
                                         {"characterize", required_argument, nullptr, INPUT_OPT_CHARACTERIZE},
                                         {"clock-period", required_argument, nullptr, 0},
                                         {"target-datafile", required_argument, nullptr, INPUT_OPT_TARGET_DATAFILE},
                                         {"target-device", required_argument, nullptr, 0},
                                         {"target-scriptfile", required_argument, nullptr, INPUT_OPT_TARGET_SCRIPTFILE},
                                         {"writer", required_argument, nullptr, 'w'},
                                         {"altera-root", optional_argument, nullptr, OPT_ALTERA_ROOT},
                                         {"lattice-root", optional_argument, nullptr, OPT_LATTICE_ROOT},
                                         {"mentor-root", optional_argument, nullptr, OPT_MENTOR_ROOT},
                                         {"mentor-optimizer", optional_argument, nullptr, OPT_MENTOR_OPTIMIZER},
                                         {"nanoxplore-root", optional_argument, nullptr, OPT_NANOXPLORE_ROOT},
                                         {"nanoxplore-bypass", optional_argument, nullptr, OPT_NANOXPLORE_BYPASS},
                                         {"xilinx-root", optional_argument, nullptr, OPT_XILINX_ROOT},
                                         {"parallel-backend", no_argument, nullptr, OPT_PARALLEL_BACKEND},
                                         {nullptr, 0, nullptr, 0}};

   if(argc == 1) // Bambu called without arguments, it simple prints help message
   {
      PrintUsage(std::cout);
      return EXIT_SUCCESS;
   }

   while(true)
   {
      int next_option = getopt_long(argc, argv, short_options, long_options, &option_index);

      // no more options are available
      if(next_option == -1)
      {
         break;
      }

      switch(next_option)
      {
         case INPUT_OPT_CHARACTERIZE:
         {
            setOption(OPT_component_name, optarg);
            break;
         }
         case INPUT_OPT_TARGET_DATAFILE:
         {
            setOption(OPT_target_device_file, optarg);
            break;
         }
         case INPUT_OPT_TARGET_SCRIPTFILE:
         {
            setOption(OPT_target_device_script, optarg);
            break;
         }
         case OPT_ALTERA_ROOT:
         {
            setOption(OPT_altera_root, GetPath(optarg));
            break;
         }
         case OPT_LATTICE_ROOT:
         {
            setOption(OPT_lattice_root, GetPath(optarg));
            break;
         }
         case OPT_MENTOR_ROOT:
         {
            setOption(OPT_mentor_root, GetPath(optarg));
            break;
         }
         case OPT_MENTOR_OPTIMIZER:
         {
            setOption(OPT_mentor_optimizer, boost::lexical_cast<bool>(optarg));
            break;
         }
         case OPT_NANOXPLORE_ROOT:
         {
            setOption(OPT_nanoxplore_root, GetPath(optarg));
            break;
         }
         case OPT_NANOXPLORE_BYPASS:
         {
            setOption(OPT_nanoxplore_bypass, std::string(optarg));
            break;
         }
         case OPT_XILINX_ROOT:
         {
            setOption(OPT_xilinx_root, GetPath(optarg));
            break;
         }
         case OPT_PARALLEL_BACKEND:
         {
            setOption(OPT_parallel_backend, true);
            break;
         }
         /// output options
         case 'w':
         {
            if(std::string(optarg) == "V")
            {
               setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VERILOG));
            }
            else if(std::string(optarg) == "H")
            {
               setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VHDL));
            }
            else
            {
               throw "BadParameters: backend language not correctly specified";
            }
            break;
         }
         case 0:
         {
            if(long_options[option_index].name == std::string("target-device"))
            {
               std::string tmp_string = optarg;
               std::vector<std::string> values = convert_string_to_vector<std::string>(tmp_string, std::string(","));
               setOption("device_name", "");
               setOption("device_speed", "");
               setOption("device_package", "");
               setOption("device_synthesis_tool", "");
               if(values.size() == 2)
               {
                  setOption(OPT_device_string, values[1]);
               }
               else if(values.size() == 3)
               {
                  setOption("device_name", values[0]);
                  setOption("device_speed", values[1]);
                  setOption("device_package", values[2]);
               }
               else if(values.size() == 4)
               {
                  setOption("device_name", values[0]);
                  setOption("device_speed", values[1]);
                  setOption("device_package", values[2]);
                  setOption("device_synthesis_tool", values[3]);
               }
               else
               {
                  THROW_ERROR("Malformed device: " + tmp_string);
               }
            }
            else if(long_options[option_index].name == std::string("clock-period"))
            {
               setOption(OPT_clock_period, optarg);
            }
            else
            {
               THROW_ERROR("Not supported option: " + std::string(long_options[option_index].name));
            }
            break;
         }
         /// other options
         default:
         {
            bool exit_success = false;
            bool res = ManageDefaultOptions(next_option, optarg, exit_success);
            if(exit_success)
            {
               return EXIT_SUCCESS;
            }
            if(res)
            {
               std::cerr << optarg << std::endl;
               return PARAMETER_NOTPARSED;
            }
         }
      }
   }
   std::string cat_args;

   for(int i = 0; i < argc; i++)
   {
      cat_args += std::string(argv[i]) + " ";
   }

   if(getOption<int>(OPT_output_level) >= OUTPUT_LEVEL_MINIMUM)
   {
      std::cerr << " ==  Eucalyptus executed with: ";
      std::cerr << cat_args;
      std::cerr << std::endl << std::endl;
   }
   CheckParameters();

   return PARAMETER_PARSED;
}

void EucalyptusParameter::CheckParameters()
{
   Parameter::CheckParameters();
   const auto sorted_dirs = [](const std::string& parent_dir) {
      std::vector<boost::filesystem::path> sorted_paths;
      std::copy(boost::filesystem::directory_iterator(parent_dir), boost::filesystem::directory_iterator(),
                std::back_inserter(sorted_paths));
      std::sort(sorted_paths.begin(), sorted_paths.end(), NaturalVersionOrder);
      return sorted_paths;
   };

   const auto altera_dirs = SplitString(getOption<std::string>(OPT_altera_root), ":");
   removeOption(OPT_altera_root);
   const auto search_quartus = [&](const std::string& dir) {
      if(boost::filesystem::exists(dir + "/quartus/bin/quartus_sh"))
      {
         if(system(STR("bash -c \"if [[ \\\"$(" + dir +
                       "/quartus/bin/quartus_sh --version | grep Version | awk '{print $2}' | awk -F'.' '{print "
                       "$1}')\\\" -lt \\\"14\\\" ]]; then exit 1; else exit 0; fi\" > /dev/null 2>&1")
                       .c_str()))
         {
            setOption(OPT_quartus_13_settings, "export PATH=$PATH:" + dir + "/quartus/bin/");
            if(system(STR("bash -c \"" + dir +
                          "/quartus/bin/quartus_sh --help | grep \\\"\\-\\-64bit\\\"\" > /dev/null 2>&1")
                          .c_str()) == 0)
            {
               setOption(OPT_quartus_13_64bit, true);
            }
            else
            {
               setOption(OPT_quartus_13_64bit, false);
            }
         }
         else
         {
            setOption(OPT_quartus_settings, "export PATH=$PATH:" + dir + "/quartus/bin/");
         }
      }
   };
   for(const auto& altera_dir : altera_dirs)
   {
      if(boost::filesystem::is_directory(altera_dir))
      {
         for(const auto& ver_dir : sorted_dirs(altera_dir))
         {
            if(boost::filesystem::is_directory(ver_dir))
            {
               search_quartus(ver_dir.string());
            }
         }
         search_quartus(altera_dir);
      }
   }

   /// Search for lattice tool
   const auto lattice_dirs = SplitString(getOption<std::string>(OPT_lattice_root), ":");
   removeOption(OPT_lattice_root);
   auto has_lattice = 0; // 0 = not found, 1 = 32-bit version, 2 = 64-bit version
   const auto search_lattice = [&](const std::string& dir) {
      if(boost::filesystem::exists(dir + "/bin/lin/diamondc"))
      {
         has_lattice = 1;
         setOption(OPT_lattice_root, dir);
      }
      else if(boost::filesystem::exists(dir + "/bin/lin64/diamondc"))
      {
         has_lattice = 2;
         setOption(OPT_lattice_root, dir);
      }
      if(boost::filesystem::exists(dir + "/cae_library/synthesis/verilog/pmi_def.v"))
      {
         setOption(OPT_lattice_pmi_def, dir + "/cae_library/synthesis/verilog/pmi_def.v");
      }
      if(boost::filesystem::exists(dir + "/cae_library/simulation/verilog/pmi/pmi_ram_dp_true_be.v"))
      {
         setOption(OPT_lattice_pmi_tdpbe, dir + "/cae_library/simulation/verilog/pmi/pmi_ram_dp_true_be.v");
      }
      if(boost::filesystem::exists(dir + "/cae_library/simulation/verilog/pmi/pmi_dsp_mult.v"))
      {
         setOption(OPT_lattice_pmi_mul, dir + "/cae_library/simulation/verilog/pmi/pmi_dsp_mult.v");
      }
   };
   for(const auto& lattice_dir : lattice_dirs)
   {
      if(boost::filesystem::is_directory(lattice_dir))
      {
         for(const auto& ver_dir : sorted_dirs(lattice_dir))
         {
            if(boost::filesystem::is_directory(ver_dir))
            {
               search_lattice(ver_dir.string());
            }
         }
         search_lattice(lattice_dir);
      }
   }
   if(has_lattice == 1)
   {
      const auto lattice_dir = getOption<std::string>(OPT_lattice_root);
      setOption(OPT_lattice_settings, "export TEMP=/tmp;export LSC_INI_PATH=\"\";"
                                      "export LSC_DIAMOND=true;"
                                      "export TCL_LIBRARY=" +
                                          lattice_dir +
                                          "/tcltk/lib/tcl8.5;"
                                          "export FOUNDRY=" +
                                          lattice_dir +
                                          "/ispfpga;"
                                          "export PATH=$FOUNDRY/bin/lin:" +
                                          lattice_dir + "/bin/lin:$PATH");
   }
   else if(has_lattice == 2)
   {
      const auto lattice_dir = getOption<std::string>(OPT_lattice_root);
      setOption(OPT_lattice_settings, "export TEMP=/tmp;export LSC_INI_PATH=\"\";"
                                      "export LSC_DIAMOND=true;"
                                      "export TCL_LIBRARY=" +
                                          lattice_dir +
                                          "/tcltk/lib/tcl8.5;"
                                          "export FOUNDRY=" +
                                          lattice_dir +
                                          "/ispfpga;"
                                          "export PATH=$FOUNDRY/bin/lin64:" +
                                          lattice_dir + "/bin/lin64:$PATH");
   }

   /// Search for Mentor tools
   const auto mentor_dirs = SplitString(getOption<std::string>(OPT_mentor_root), ":");
   removeOption(OPT_mentor_root);
   const auto search_mentor = [&](const std::string& dir) {
      if(boost::filesystem::exists(dir + "/bin/vsim"))
      {
         setOption(OPT_mentor_modelsim_bin, dir + "/bin");
      }
      if(boost::filesystem::exists(dir + "/bin/visualizer"))
      {
         setOption(OPT_mentor_visualizer, dir + "/bin/visualizer");
      }
   };
   for(const auto& mentor_dir : mentor_dirs)
   {
      if(boost::filesystem::is_directory(mentor_dir))
      {
         for(const auto& ver_dir : sorted_dirs(mentor_dir))
         {
            if(boost::filesystem::is_directory(ver_dir))
            {
               search_mentor(ver_dir.string());
            }
         }
         search_mentor(mentor_dir);
      }
   }
   if(isOption(OPT_visualizer) && getOption<bool>(OPT_visualizer) && !isOption(OPT_mentor_visualizer))
   {
      THROW_ERROR("Mentor Visualizer was not detected by Bambu. Please check --mentor-root option is correct.");
   }

   /// Search for NanoXPlore tools
   const auto nanox_dirs = SplitString(getOption<std::string>(OPT_nanoxplore_root), ":");
   removeOption(OPT_nanoxplore_root);
   const auto search_xmap = [&](const std::string& dir) {
      if(boost::filesystem::exists(dir + "/bin/nxpython"))
      {
         setOption(OPT_nanoxplore_root, dir);
      }
   };
   for(const auto& nanox_dir : nanox_dirs)
   {
      if(boost::filesystem::is_directory(nanox_dir))
      {
         for(const auto& ver_dir : sorted_dirs(nanox_dir))
         {
            if(boost::filesystem::is_directory(ver_dir))
            {
               search_xmap(ver_dir.string());
            }
         }
         search_xmap(nanox_dir);
      }
   }

   /// Search for Xilinx tools
   const auto target_64 = true;
   const auto xilinx_dirs = SplitString(getOption<std::string>(OPT_xilinx_root), ":");
   removeOption(OPT_xilinx_root);
   const auto search_xilinx = [&](const std::string& dir) {
      if(boost::filesystem::exists(dir + "/ISE"))
      {
         if(target_64 && boost::filesystem::exists(dir + "/settings64.sh"))
         {
            setOption(OPT_xilinx_settings, dir + "/settings64.sh");
         }
         else if(boost::filesystem::exists(dir + "/settings32.sh"))
         {
            setOption(OPT_xilinx_settings, dir + "/settings32.sh");
         }
         if(boost::filesystem::exists(dir + "/ISE/verilog/src/glbl.v"))
         {
            setOption(OPT_xilinx_glbl, dir + "/ISE/verilog/src/glbl.v");
         }
      }
   };
   const auto search_xilinx_vivado = [&](const std::string& dir) {
      if(boost::filesystem::exists(dir + "/ids_lite"))
      {
         if(target_64 && boost::filesystem::exists(dir + "/settings64.sh"))
         {
            setOption(OPT_xilinx_vivado_settings, dir + "/settings64.sh");
         }
         else if(boost::filesystem::exists(dir + "/settings32.sh"))
         {
            setOption(OPT_xilinx_vivado_settings, dir + "/settings32.sh");
         }
         if(boost::filesystem::exists(dir + "/data/verilog/src/glbl.v"))
         {
            setOption(OPT_xilinx_glbl, dir + "/data/verilog/src/glbl.v");
         }
      }
   };
   for(const auto& xilinx_dir : xilinx_dirs)
   {
      if(boost::filesystem::is_directory(xilinx_dir))
      {
         for(const auto& ver_dir : sorted_dirs(xilinx_dir))
         {
            if(boost::filesystem::is_directory(ver_dir))
            {
               for(const auto& ise_dir : boost::filesystem::directory_iterator(ver_dir))
               {
                  const auto ise_path = ise_dir.path().string();
                  if(boost::filesystem::is_directory(ise_dir) && ise_path.find("ISE") > ise_path.find_last_of('/'))
                  {
                     search_xilinx(ise_path);
                  }
               }
            }
         }
         search_xilinx(xilinx_dir);
      }
   }
   for(const auto& xilinx_dir : xilinx_dirs)
   {
      if(boost::filesystem::is_directory(xilinx_dir))
      {
         for(const auto& vivado_dir : boost::filesystem::directory_iterator(xilinx_dir))
         {
            const auto vivado_path = vivado_dir.path().string();
            if(boost::filesystem::is_directory(vivado_dir) &&
               vivado_path.find("Vivado") > vivado_path.find_last_of('/'))
            {
               for(const auto& ver_dir : sorted_dirs(vivado_path))
               {
                  if(boost::filesystem::is_directory(ver_dir))
                  {
                     search_xilinx_vivado(ver_dir.string());
                  }
               }
            }
         }
         search_xilinx_vivado(xilinx_dir);
      }
   }

   /// Search for verilator
   setOption(OPT_verilator, system("which verilator > /dev/null 2>&1") == 0);
   if(getOption<bool>(OPT_verilator))
   {
      setOption(OPT_verilator_l2_name,
                system("bash -c \"if [[ \\\"x$(verilator --l2-name v 2>&1 | head -n1 | grep -i 'Invalid Option')\\\" = "
                       "\\\"x\\\" ]]; then exit 0; else exit 1; fi\" > /dev/null 2>&1") == 0);
      const auto has_timescale_override =
          system("bash -c \"if [[ \\\"x$(verilator --timescale-override v 2>&1 | head -n1 | grep -i 'Invalid "
                 "Option')\\\" = \\\"x\\\" ]]; then exit 0; else exit 1; fi\" > /dev/null 2>&1") == 0;
      if(has_timescale_override)
      {
         setOption(OPT_verilator_timescale_override, "1ps/1ps");
      }
      const auto thread_support =
          system("bash -c \"if [[ \\\"$(verilator --version | head -n1 | awk -F' ' '{print $2}'| awk -F'.' '{print "
                 "$1}')\\\" = \\\"4\\\" ]]; then exit 0; else exit 1; fi\" > /dev/null 2>&1") == 0;
      if(getOption<bool>(OPT_verilator_parallel) && !thread_support)
      {
         THROW_WARNING("Installed version of Verilator does not support multi-threading.");
         setOption(OPT_verilator_parallel, false);
      }
   }

   /// Search for icarus
   setOption(OPT_icarus, system("which iverilog > /dev/null 2>&1") == 0);

   if(isOption(OPT_simulator))
   {
      if(getOption<std::string>(OPT_simulator) == "MODELSIM" && !isOption(OPT_mentor_modelsim_bin))
      {
         THROW_ERROR("Mentor Modelsim was not detected by Bambu. Please check --mentor-root option is correct.");
      }
      else if(getOption<std::string>(OPT_simulator) == "XSIM" && !isOption(OPT_xilinx_vivado_settings))
      {
         THROW_ERROR("Xilinx XSim was not detected by Bambu. Please check --xilinx-root option is correct.");
      }
      else if(getOption<std::string>(OPT_simulator) == "ISIM" && !isOption(OPT_xilinx_settings))
      {
         THROW_ERROR("Xilinx ISim was not detected by Bambu. Please check --xilinx-root option is correct.");
      }
      else if(getOption<std::string>(OPT_simulator) == "ICARUS" && !isOption(OPT_icarus))
      {
         THROW_ERROR("Icarus was not detected by Bambu. Please make sure it is installed in the system.");
      }
      else if(getOption<std::string>(OPT_simulator) == "VERILATOR" && !isOption(OPT_verilator))
      {
         THROW_ERROR("Verilator was not detected by Bambu. Please make sure it is installed in the system.");
      }
   }
   else
   {
      if(isOption(OPT_mentor_modelsim_bin))
      {
         setOption(OPT_simulator, "MODELSIM"); /// Mixed language simulator
      }
      else if(isOption(OPT_xilinx_vivado_settings))
      {
         setOption(OPT_simulator, "XSIM"); /// Mixed language simulator
      }
      else if(isOption(OPT_xilinx_settings))
      {
         setOption(OPT_simulator, "ISIM"); /// Mixed language simulator
      }
      else if(isOption(OPT_icarus))
      {
         setOption(OPT_simulator, "ICARUS");
      }
      else if(isOption(OPT_verilator))
      {
         setOption(OPT_simulator, "VERILATOR");
      }
      else
      {
         THROW_ERROR("No valid simulator was found in the system.");
      }
   }
   if(not isOption(OPT_device_string))
   {
      std::string device_string = getOption<std::string>("device_name") + getOption<std::string>("device_speed") +
                                  getOption<std::string>("device_package");
      if(isOption("device_synthesis_tool") && !getOption<std::string>("device_synthesis_tool").empty())
      {
         device_string += "-" + getOption<std::string>("device_synthesis_tool");
      }
      setOption(OPT_device_string, device_string);
   }
}

void EucalyptusParameter::SetDefaults()
{
   // ---------- general options ----------- //
   /// debugging levels
   setOption(OPT_output_level, OUTPUT_LEVEL_MINIMUM);
   setOption(OPT_debug_level, DEBUG_LEVEL_MINIMUM);

   setOption(OPT_altera_root, "/opt/altera:/opt/intelFPGA");
   setOption(OPT_lattice_root, "/opt/diamond:/usr/local/diamond");
   setOption(OPT_mentor_root, "/opt/mentor");
   setOption(OPT_mentor_optimizer, true);
   setOption(OPT_nanoxplore_root, "/opt/NanoXplore/NXmap3");
   setOption(OPT_verilator_parallel, false);
   setOption(OPT_xilinx_root, "/opt/Xilinx");

   /// target device
   setOption("device_name", "xc7z020");
   setOption("device_speed", "-1");
   setOption("device_package", "clg484");
   setOption("device_synthesis_tool", "VVD");
   setOption(OPT_connect_iob, false);
   setOption(OPT_target_device_type, static_cast<int>(TargetDevice_Type::FPGA));
   setOption(OPT_clock_period_resource_fraction, 1.0);
   setOption(OPT_parallel_backend, false);

   /// library estimation
   setOption(OPT_estimate_library, false);

   /// backend HDL
   setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VERILOG));
   setOption(OPT_timing_simulation, false);
   setOption(OPT_reset_type, "no");
   setOption(OPT_reg_init_value, false);

   setOption(OPT_output_directory, GetCurrentPath() + "/HLS_output/");
   setOption(OPT_rtl, true);
   setOption(OPT_reset_level, false);
   setOption(OPT_mixed_design, true);
}
