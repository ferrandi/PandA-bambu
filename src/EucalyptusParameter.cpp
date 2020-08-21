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

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

/// Header include
#include "EucalyptusParameter.hpp"

#include "target_device.hpp"

#include "language_writer.hpp"

#define TOOL_OPT_BASE 256
#define INPUT_OPT_CHARACTERIZE (1 + TOOL_OPT_BASE)
#define INPUT_OPT_TARGET_DATAFILE (1 + INPUT_OPT_CHARACTERIZE)
#define INPUT_OPT_TARGET_SCRIPTFILE (1 + INPUT_OPT_TARGET_DATAFILE)

#include "utility.hpp"

#include <getopt.h>

void EucalyptusParameter::PrintProgramName(std::ostream& os) const
{
   os << "" << std::endl;
   os << "********************************************************************************" << std::endl;
   os << "                _____                _             _" << std::endl;
   os << "               | ____|   _  ___ __ _| |_   _ _ __ | |_ _   _ ___" << std::endl;
   os << "               |  _|| | | |/ __/ _` | | | | | '_ \\| __| | | / __|" << std::endl;
   os << "               | |__| |_| | (_| (_| | | |_| | |_) | |_| |_| \\__ \\" << std::endl;
   os << "               |_____\\__,_|\\___\\__,_|_|\\__, | .__/ \\__|\\__,_|___/" << std::endl;
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
      << "    --target-device=<file>          Specify the type of the device, separated by commas (e.g.,: \"xc7z020,-1,clg484\")\n"
      << "    --target-datafile=file          Specify a data XML file describing some defaults value for the target device.\n"
      << "    --target-scriptfile=file        Specify a script XML file including the scripts for the synthesis w.r.t. the target device.\n"
      << "    --clock-period=value            Specify the period of the clock signal (default 10 nanoseconds)\n"
      << "    --characterize=<component_name> Characterize the given component"
#if HAVE_EXPERIMENTAL
      << "\n"
      << "  Component Integration:\n"
      << "    --import-ip-core=<file>         Converts the specified file into an XML-based representation.\n"
      << "    --export-ip-core=<name>         Generates the HDL description of the specified component.\n"
      << "    --output=<file>                 Specifies the name of the output file.\n"
#endif
      << "\n"
      << std::endl;
}

EucalyptusParameter::EucalyptusParameter(const std::string& _program_name, int _argc, char** const _argv) : Parameter(_program_name, _argc, _argv)
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

   const struct option long_options[] = {
      COMMON_LONG_OPTIONS,
      {"characterize", required_argument, nullptr, INPUT_OPT_CHARACTERIZE},
      {"clock-period", required_argument, nullptr, 0},
#if HAVE_EXPERIMENTAL
      {"export-ip-core", required_argument, nullptr, 0},
      {"import-ip-core", required_argument, nullptr, 0},
      {"output", required_argument, nullptr, 0},
#endif
      {"target-datafile", required_argument, nullptr, INPUT_OPT_TARGET_DATAFILE},
      {"target-device", required_argument, nullptr, 0},
      {"target-scriptfile", required_argument, nullptr, INPUT_OPT_TARGET_SCRIPTFILE},
      {"writer", required_argument, nullptr, 'w'},
      {nullptr, 0, nullptr, 0}
   };

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
         /// output options
         case 'w':
         {
            if(std::string(optarg) == "V")
               setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VERILOG));
#if HAVE_EXPERIMENTAL
            else if(std::string(optarg) == "S")
               setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::SYSTEMC));
#endif
            else if(std::string(optarg) == "H")
               setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VHDL));
            else
               throw "BadParameters: backend language not correctly specified";
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
#if HAVE_EXPERIMENTAL
            else if(long_options[option_index].name == std::string("import-ip-core"))
            {
               setOption(OPT_import_ip_core, optarg);
            }
            else if(long_options[option_index].name == std::string("export-ip-core"))
            {
               setOption(OPT_export_ip_core, optarg);
            }
            else if(long_options[option_index].name == std::string("output"))
            {
               setOption(OPT_output_file, GetPath(optarg));
            }
#endif
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
   if(not isOption(OPT_device_string))
   {
      std::string device_string = getOption<std::string>("device_name") + getOption<std::string>("device_speed") + getOption<std::string>("device_package");
      if(isOption("device_synthesis_tool") && !getOption<std::string>("device_synthesis_tool").empty())
      {
         device_string += "-" + getOption<std::string>("device_synthesis_tool");
      }
      setOption(OPT_device_string, device_string);
   }
#if HAVE_EXPERIMENTAL
   /// checking of import/export of IP cores
   if(isOption(OPT_import_ip_core) and isOption(OPT_export_ip_core))
   {
      THROW_ERROR("Importing and exporting of IP cores are mutually exclusive");
   }
   if((isOption(OPT_import_ip_core) or isOption(OPT_export_ip_core)) and !isOption(OPT_output_file))
   {
      THROW_ERROR("Importing/Exporting of IP cores requires to specify the name of the output file (--output)");
   }
#endif
}

void EucalyptusParameter::SetDefaults()
{
   // ---------- general options ----------- //
   /// debugging levels
   setOption(OPT_output_level, OUTPUT_LEVEL_MINIMUM);
   setOption(OPT_debug_level, DEBUG_LEVEL_MINIMUM);

   /// target device
   setOption("device_name", "xc7z020");
   setOption("device_speed", "-1");
   setOption("device_package", "clg484");
   setOption("device_synthesis_tool", "VVD");
   setOption(OPT_connect_iob, false);
   setOption(OPT_target_device_type, static_cast<int>(TargetDevice_Type::FPGA));
   setOption(OPT_clock_period_resource_fraction, 1.0);

   /// library estimation
   setOption(OPT_estimate_library, false);

   /// backend HDL
   setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VERILOG));
   setOption(OPT_timing_simulation, false);
   setOption(OPT_sync_reset, "no");
   setOption(OPT_reg_init_value, false);

   setOption(OPT_output_directory, "work");
   setOption(OPT_rtl, true);
   setOption(OPT_level_reset, false);
#if HAVE_EXPERIMENTAL
   setOption(OPT_mixed_design, true);
#endif
}
