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
 * @file bambu-cc-Parameter.cpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "bambu-cc-Parameter.hpp"

#include "CompilerWrapper.hpp"
#include "cpu_time.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "module_interface.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"
#include <climits>
#include <cstring>
#include <filesystem>
#include <getopt.h>
#include <iosfwd>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "config_HAVE_I386_CLANG16_COMPILER.hpp"
#include "config_HAVE_I386_CLANG19_COMPILER.hpp"
#include "config_PANDA_INCLUDE_INSTALLDIR.hpp"
#include "config_RELEASE.hpp"

#define OPT_PRINT_FILE_NAME 256
#define OPT_INCLUDE (1 + OPT_PRINT_FILE_NAME)
#define OPT_ISYSTEM (1 + OPT_INCLUDE)
#define OPT_MF (1 + OPT_ISYSTEM)
#define OPT_MT (1 + OPT_MF)
#define OPT_MQ (1 + OPT_MT)
#define OPT_IPLUGINDIR (1 + OPT_MQ)
#define OPT_MINUS_INCLUDE (1 + OPT_IPLUGINDIR)
#define OPT_START_GROUP (1 + OPT_MINUS_INCLUDE)
#define OPT_END_GROUP (1 + OPT_START_GROUP)
#define OPT_MINUS_MAP (1 + OPT_END_GROUP)
#define OPT_GC_SECTIONS (1 + OPT_MINUS_MAP)

bambu_cc_parameter::bambu_cc_parameter(const std::string& _program_name, int _argc, char** const _argv)
    : Parameter(_program_name, _argc, _argv)
{
   SetDefaults();
}

int bambu_cc_parameter::Exec()
{
   int opt;
   exit_code = PARAMETER_NOTPARSED;

   const char* const short_options = COMMON_SHORT_OPTIONS_STRING "o:Ss::tn:cM::i:C:ru:e:T:" CC_SHORT_OPTIONS_STRING;

   const struct option long_options[] = {COMMON_LONG_OPTIONS,
                                         {"iplugindir", required_argument, nullptr, OPT_IPLUGINDIR},
                                         {"print-file-name", required_argument, nullptr, OPT_PRINT_FILE_NAME},
                                         {"MF", required_argument, nullptr, OPT_MF},
                                         {"MT", required_argument, nullptr, OPT_MT},
                                         {"MQ", required_argument, nullptr, OPT_MQ},
                                         {"include", required_argument, nullptr, OPT_MINUS_INCLUDE},
                                         {"Map", required_argument, nullptr, OPT_MINUS_MAP},
                                         {"start-group", no_argument, nullptr, OPT_START_GROUP},
                                         {"end-group", no_argument, nullptr, OPT_END_GROUP},
                                         {"gc-sections", no_argument, nullptr, OPT_GC_SECTIONS},
                                         CC_LONG_OPTIONS,
                                         {nullptr, 0, nullptr, 0}};

   if(argc == 1)
   {
      PrintUsage(std::cerr);
      return EXIT_SUCCESS;
   }

   while((opt = getopt_long_only(argc, argv, short_options, long_options, nullptr)) != -1)
   {
      switch(opt)
      {
         case 'o':
            setOption(OPT_output_file, std::string(optarg));
            break;
         case 'S':
         {
            setOption(OPT_cc_S, true);
            break;
         }
         case 'C':
         {
            setOption(OPT_compress_archive, optarg);
            break;
         }
         case 's':
         {
            if(optarg == nullptr)
            {
               // -s has been passed
               /// the default of bambu-cc is the silent mode so nothing has to be done
            }
            else
            {
               ///
               std::string parameter(optarg);
               if(starts_with(parameter, "td="))
               {
                  setOption(OPT_cc_standard, parameter.substr(parameter.find('=') + 1));
               }
               else
               {
                  THROW_ERROR("unexpected parameter: " + parameter);
               }
            }
            break;
         }
         case 'M':
         {
            std::string cc_extra_options;
            if(optarg != nullptr)
            {
               cc_extra_options = "-M" + std::string(optarg);
            }
            else
            {
               cc_extra_options = "-M";
            }
            appendOption(OPT_cc_extra_options, cc_extra_options, " ");
            break;
         }
         case 'i':
         {
            if(std::string(optarg).find("plugindir=") == 0)
            {
               setOption(OPT_cc_plugindir, std::string(optarg).substr(10));
            }
            else
            {
               appendOption(OPT_cc_extra_options, "-i" + std::string(optarg), " ");
            }
            break;
         }
         case OPT_MINUS_INCLUDE:
         {
            appendOption(OPT_cc_extra_options, "-include " + std::string(optarg), " ");
            break;
         }
         case 'n':
         {
            appendOption(OPT_cc_extra_options, "-n" + std::string(optarg), " ");
            break;
         }
         case OPT_IPLUGINDIR:
         {
            setOption(OPT_cc_plugindir, optarg);
            break;
         }
         case OPT_MF:
         {
            appendOption(OPT_cc_extra_options, "-MF " + std::string(optarg), " ");
            break;
         }
         case OPT_MT:
         {
            appendOption(OPT_cc_extra_options, "-MT " + std::string(optarg), " ");
            break;
         }
         case OPT_MQ:
         {
            appendOption(OPT_cc_extra_options, "-MQ " + std::string(optarg), " ");
            break;
         }
         case 'x':
         {
            appendOption(OPT_cc_extra_options, "-x " + std::string(optarg), " ");
            break;
         }
         case 't':
         {
            appendOption(OPT_cc_extra_options, "-t", " ");
            break;
         }
         case OPT_PRINT_FILE_NAME:
         {
            refcount<bambu_cc_parameter> param(this, null_deleter());
            CompilerWrapper(param, CompilerWrapper_CompilerTarget::CT_NO_COMPILER)
                .QueryCompilerConfig("--print-file-name=" + std::string(optarg));
            return EXIT_SUCCESS;
         }
         case OPT_START_GROUP:
         case OPT_END_GROUP:
         case OPT_MINUS_MAP:
         case OPT_GC_SECTIONS:
         case 'c':
         case 'e':
         case 'u':
         case 'T':
         case 'r':
         {
            /// do nothing
            /// passed to bambu-cc when it work as ld
            break;
         }
         default:
         {
            bool exit_success = false;
            bool res = ManageCCOptions(opt, optarg);
            if(res)
            {
               res = ManageDefaultOptions(opt, optarg, exit_success);
            }
            if(exit_success)
            {
               return EXIT_SUCCESS;
            }
            if(res)
            {
               return PARAMETER_NOTPARSED;
            }
         }
      }
   }

   while(optind < argc)
   {
      const std::string filename(argv[optind]);
      if(std::filesystem::path(filename).extension() == ".o")
      {
         appendOption(OPT_obj_files, filename);
      }
      else if(std::filesystem::path(filename).extension() == ".a")
      {
         appendOption(OPT_archive_files, filename);
      }
      else
      {
         appendOption(OPT_input_file, filename);
         setOption(OPT_input_format, GetFileFormat(filename, true));
      }
      optind++;
   }

   CheckParameters();

   return PARAMETER_PARSED;
}

void bambu_cc_parameter::CheckParameters()
{
   setOption(OPT_output_hls_directory, isOption(OPT_output_directory) ? getOption<std::string>(OPT_output_directory) :
                                                                        std::filesystem::current_path().string());
   Parameter::CheckParameters();

   appendOption(OPT_cc_includes, "-isystem " + relocate_install_path(PANDA_INCLUDE_INSTALLDIR, true).string(), " ");
   if(!isOption(OPT_cc_standard))
   {
      const auto flag_cpp = isOption(OPT_input_format) &&
                            (getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP ||
                             getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LLVM_CPP);
      if(flag_cpp)
      {
         setOption(OPT_cc_standard, "gnu++14");
      }
      else

      {
         setOption(OPT_cc_standard, "gnu11");
      }
   }
}

void bambu_cc_parameter::PrintHelp(std::ostream& os) const
{
   os << "Usage: " << getOption<std::string>(OPT_program_name)
      << " [options] <input_file1> [<input_file2> ... <input_fileN>]" << std::endl;
   os << std::endl;
   os << "Options: \n"
      << "\n";
   PrintGeneralOptionsUsage(os);
   os << "\n";
   PrintOutputOptionsUsage(os);
   os << "    -o=<file>                        Specify the output file name\n"
      << "\n";
   PrintCCOptionsUsage(os);
   os << "\n";
}

void bambu_cc_parameter::PrintProgramName(std::ostream& os) const
{
   os << "********************************************************************************" << std::endl;
   os << "   _                                             _" << std::endl;
   os << "  | |_ _ __ ___  ___       _ __   __ _ _ __   __| | __ _       ___ ___" << std::endl;
   os << R"(  | __| '__/ _ \/ _ \_____| '_ \ / _` | '_ \ / _` |/ _` |_____/ __/ __|)" << std::endl;
   os << "  | |_| | |  __/  __/_____| |_) | (_| | | | | (_| | (_| |_____( (_( (_" << std::endl;
   os << R"(   \__|_|  \___|\___|     | .__/ \__,_|_| |_|\__,_|\__,_|     \___\___|)" << std::endl;
   os << "                          |_|" << std::endl;
   os << "********************************************************************************" << std::endl;
}

void bambu_cc_parameter::SetDefaults()
{
   // ---------- general options ----------- //
   /// Debugging level
   setOption(OPT_debug_level, DEBUG_LEVEL_NONE);
   /// Output level
   setOption(OPT_output_level, OUTPUT_LEVEL_NONE);

   setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O0);

   /// ---------- frontend analysis ----------//
   setOption(OPT_pretty_print, false);

   // -- CC options -- //
   setOption(OPT_default_compiler, CompilerWrapper::getDefaultCompiler());
   setOption(OPT_compatible_compilers, CompilerWrapper::getCompatibleCompilers());
   setOption(OPT_without_transformation, true);
   setOption(OPT_no_clean, false);
#if HAVE_HLS_BUILT
   setOption(OPT_interface_type, HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION);
#endif
}
