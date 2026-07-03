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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file EucalyptusParameter.cpp
 * @brief This file contains the implementation of some methods for parameter parsing in Eucalyptus tool
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "EucalyptusParameter.hpp"

#include "generic_device.hpp"
#include "language_writer.hpp"

#define TOOL_OPT_BASE 256
#define INPUT_OPT_CHARACTERIZE (1 + TOOL_OPT_BASE)
#define INPUT_OPT_TARGET_DATAFILE (1 + INPUT_OPT_CHARACTERIZE)
#define INPUT_OPT_TARGET_SCRIPTFILE (1 + INPUT_OPT_TARGET_DATAFILE)
#define OPT_PARALLEL_BACKEND (1 + INPUT_OPT_TARGET_SCRIPTFILE)

#include "utility.hpp"
#include "utility/fileIO.hpp"
#include <filesystem>
#include <getopt.h>
#include <thread>

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
   os << "  Backend configuration:\n\n"
      << "   --parallel-backend[=N]\n"
      << "        Generate parallel backend synthesis script with the given number of threads if possible (default: "
         "1)\n"
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
   int opt, option_index;

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
                                         {"parallel-backend", required_argument, nullptr, OPT_PARALLEL_BACKEND},
                                         {nullptr, 0, nullptr, 0}};

   if(argc == 1) // Bambu called without arguments, it simple prints help message
   {
      PrintUsage(std::cout);
      return EXIT_SUCCESS;
   }

   while((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
   {
      switch(opt)
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
         case OPT_PARALLEL_BACKEND:
         {
            unsigned long nthreads = std::strtoul(optarg, nullptr, 10);
            if(!nthreads || nthreads == std::numeric_limits<unsigned long>::max())
            {
               THROW_ERROR("BadParameters: parallel backend synthesis thread count invalid");
            }
            setOption(OPT_parallel_backend, nthreads);
            break;
         }
         /// output options
         case 'w':
         {
            if(std::string(optarg) == "V")
            {
               setOption(OPT_writer_language, HDLWriter_Language::VERILOG);
            }
            else if(std::string(optarg) == "H")
            {
               setOption(OPT_writer_language, HDLWriter_Language::VHDL);
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
               std::vector<std::string> values =
                   string_to_container<std::vector<std::string>>(tmp_string, std::string(","));
               setOption("device_name", "");
               setOption("device_speed", "");
               setOption("device_package", "");
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
            bool res = ManageDefaultOptions(opt, optarg, exit_success);
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
   setOption(OPT_cat_args, cat_args);

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, getOption<int>(OPT_output_level),
                  " ==  Eucalyptus executed with: " + cat_args + "\n");
   CheckParameters();

   return PARAMETER_PARSED;
}

void EucalyptusParameter::CheckParameters()
{
   Parameter::CheckParameters();

   if(not isOption(OPT_device_string))
   {
      std::string device_string = getOption<std::string>("device_name") + getOption<std::string>("device_speed") +
                                  getOption<std::string>("device_package");
      setOption(OPT_device_string, device_string);
   }
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
   setOption(OPT_connect_iob, false);
   setOption(OPT_clock_period_resource_fraction, 1.0);

   setOption(OPT_parallel_backend, "1");

   /// backend HDL
   setOption(OPT_writer_language, HDLWriter_Language::VERILOG);
   setOption(OPT_reset_type, "no");
   setOption(OPT_reg_init_value, false);

   setOption(OPT_reset_level, false);
   setOption(OPT_mixed_design, true);
}
