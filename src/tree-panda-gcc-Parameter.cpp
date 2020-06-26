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
 * @file tree-panda-gcc-Parameter.cpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_I386_CLANG4_COMPILER.hpp"
#include "config_HAVE_I386_CLANG5_COMPILER.hpp"
#include "config_HAVE_I386_CLANG6_COMPILER.hpp"
#include "config_HAVE_I386_CLANG7_COMPILER.hpp"
#include "config_HAVE_I386_CLANG8_COMPILER.hpp"
#include "config_HAVE_I386_CLANG9_COMPILER.hpp"
#include "config_HAVE_I386_GCC45_COMPILER.hpp"
#include "config_HAVE_I386_GCC46_COMPILER.hpp"
#include "config_HAVE_I386_GCC47_COMPILER.hpp"
#include "config_HAVE_I386_GCC48_COMPILER.hpp"
#include "config_HAVE_I386_GCC49_COMPILER.hpp"
#include "config_HAVE_I386_GCC5_COMPILER.hpp"
#include "config_HAVE_I386_GCC6_COMPILER.hpp"
#include "config_HAVE_I386_GCC7_COMPILER.hpp"
#include "config_HAVE_I386_GCC8_COMPILER.hpp"
#include "config_HAVE_MAPPING_BUILT.hpp"
#include "config_RELEASE.hpp"

/// Header include
#include "tree-panda-gcc-Parameter.hpp"

/// Boost include
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/path.hpp>

/// Constants include
#include "constants.hpp"

/// STD include
#include <climits>
#include <cstring>
#include <iosfwd>

/// STL include
#include "custom_set.hpp"
#include <vector>

/// Utility include
#include "boost/lexical_cast.hpp"
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "module_interface.hpp"
#include "utility.hpp"
#include <getopt.h>

/// Wrapper include
#include "gcc_wrapper.hpp"

#define OPT_PRINT_FILE_NAME 256
#define OPT_INCLUDE 257
#define OPT_ISYSTEM 258
#define OPT_MF 259
#define OPT_MT 260
#define OPT_MQ 261
#define OPT_IQUOTE 262
#define OPT_ISYSROOT 263
#define OPT_IMULTILIB 264
#define OPT_IPLUGINDIR 265
#define OPT_MINUS_INCLUDE 266
#define OPT_START_GROUP 267
#define OPT_END_GROUP 268
#define OPT_MINUS_MAP 269
#define OPT_GC_SECTIONS 270

tree_panda_gcc_parameter::tree_panda_gcc_parameter(const std::string& _program_name, int _argc, char** const _argv) : Parameter(_program_name, _argc, _argv)
{
   SetDefaults();
}

tree_panda_gcc_parameter::~tree_panda_gcc_parameter() = default;

int tree_panda_gcc_parameter::Exec()
{
   exit_code = PARAMETER_NOTPARSED;

   /// variable used into option parsing
   int option_index;

   const char* const short_options = COMMON_SHORT_OPTIONS_STRING "o:Ss::x:tn:cM::i:C:ru:e:T:" GCC_SHORT_OPTIONS_STRING;

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
                                         GCC_LONG_OPTIONS,
                                         {nullptr, 0, nullptr, 0}};

   if(argc == 1)
   {
      PrintUsage(std::cerr);
      return EXIT_SUCCESS;
   }

   while(true)
   {
      int next_option = getopt_long_only(argc, argv, short_options, long_options, &option_index);

      // no more options are available
      if(next_option == -1)
         break;

      switch(next_option)
      {
         case 'o':
            setOption(OPT_output_file, GetPath(optarg));
            break;
         case 'S':
         {
            setOption(OPT_gcc_S, true);
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
               /// the default of tree-panda-gcc is the silent mode so nothing has to be done
            }
            else
            {
               ///
               std::string parameter(optarg);
               if(boost::algorithm::starts_with(parameter, "td="))
                  setOption(OPT_gcc_standard, parameter.substr(parameter.find("=") + 1));
               else
                  THROW_ERROR("unexpected parameter: " + parameter);
            }
            break;
         }
         case 'M':
         {
            std::string gcc_extra_options;
            if(optarg != nullptr)
               gcc_extra_options = "-M" + std::string(optarg);
            else
               gcc_extra_options = "-M";
            if(isOption(OPT_gcc_extra_options))
               gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
            setOption(OPT_gcc_extra_options, gcc_extra_options);
            break;
         }
         case 'i':
         {
            if(std::string(optarg).find("plugindir=") == 0)
            {
               setOption(OPT_gcc_plugindir, std::string(optarg).substr(10));
            }
            else
            {
               std::string gcc_extra_options;
               gcc_extra_options = "-i" + std::string(optarg);
               if(isOption(OPT_gcc_extra_options))
                  gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
               setOption(OPT_gcc_extra_options, gcc_extra_options);
            }
            break;
         }
         case OPT_MINUS_INCLUDE:
         {
            std::string gcc_extra_options = "-include " + std::string(optarg);
            if(isOption(OPT_gcc_extra_options))
               gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
            setOption(OPT_gcc_extra_options, gcc_extra_options);
            break;
         }
         case 'n':
         {
            std::string parameter(optarg);
            std::string gcc_extra_options = "-n" + std::string(parameter);
            if(isOption(OPT_gcc_extra_options))
               gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
            setOption(OPT_gcc_extra_options, gcc_extra_options);
            break;
         }
         case OPT_IPLUGINDIR:
         {
            setOption(OPT_gcc_plugindir, optarg);
            break;
         }
         case OPT_MF:
         {
            std::string parameter(optarg);
            std::string gcc_extra_options = "-MF " + std::string(parameter);
            if(isOption(OPT_gcc_extra_options))
               gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
            setOption(OPT_gcc_extra_options, gcc_extra_options);
            break;
         }
         case OPT_MT:
         {
            std::string parameter(optarg);
            std::string gcc_extra_options = "-MT " + std::string(parameter);
            if(isOption(OPT_gcc_extra_options))
               gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
            setOption(OPT_gcc_extra_options, gcc_extra_options);
            break;
         }
         case OPT_MQ:
         {
            std::string parameter(optarg);
            std::string gcc_extra_options = "-MQ " + std::string(parameter);
            if(isOption(OPT_gcc_extra_options))
               gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
            setOption(OPT_gcc_extra_options, gcc_extra_options);
            break;
         }
         case 'x':
         {
            std::string gcc_extra_options = "-x " + std::string(optarg);
            if(isOption(OPT_gcc_extra_options))
               gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
            setOption(OPT_gcc_extra_options, gcc_extra_options);
            break;
         }
         case 't':
         {
            std::string gcc_extra_options = "-t";
            if(isOption(OPT_gcc_extra_options))
               gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
            setOption(OPT_gcc_extra_options, gcc_extra_options);
            break;
         }
         case OPT_PRINT_FILE_NAME:
         {
            const GccWrapper_OptimizationSet optimization_set = getOption<GccWrapper_OptimizationSet>(OPT_gcc_optimization_set);
            refcount<tree_panda_gcc_parameter> param(this, null_deleter());
            GccWrapperRef Wrap = GccWrapperRef(new GccWrapper(param, GccWrapper_CompilerTarget::CT_NO_GCC, optimization_set));
            Wrap->QueryGccConfig("--print-file-name=" + std::string(optarg));
            return EXIT_SUCCESS;
         }
         case OPT_START_GROUP:
         case OPT_END_GROUP:
         case OPT_MINUS_MAP:
         case OPT_GC_SECTIONS:
         case 'e':
         case 'u':
         case 'T':
         case 'r':
         {
            /// do nothing
            /// passed to tree-panda-gcc when it work as ld
            break;
         }
         default:
         {
            bool exit_success = false;
            bool res = ManageGccOptions(next_option, optarg);
            if(res)
               res = ManageDefaultOptions(next_option, optarg, exit_success);
            if(exit_success)
               return EXIT_SUCCESS;
            if(res)
            {
               return PARAMETER_NOTPARSED;
            }
         }
      }
   }

#if !RELEASE
   if(isOption(OPT_write_parameter_xml))
   {
      const std::string file_name = getOption<std::string>(OPT_write_parameter_xml);
      write_xml_configuration_file(file_name);
      return EXIT_SUCCESS;
   }
#endif
   while(optind < argc)
   {
      if(GetExtension(argv[optind]) == "o")
      {
         std::string object_files;
         if(isOption(OPT_obj_files))
         {
            object_files = getOption<std::string>(OPT_obj_files) + STR_CST_string_separator;
         }
         setOption(OPT_obj_files, object_files + GetPath(argv[optind]));
      }
      else if(GetExtension(argv[optind]) == "a")
      {
         std::string archive_files;
         if(isOption(OPT_archive_files))
         {
            archive_files = getOption<std::string>(OPT_archive_files) + STR_CST_string_separator;
         }
         setOption(OPT_archive_files, archive_files + GetPath(argv[optind]));
      }
      else
      {
         std::string input_file;
         if(isOption(OPT_input_file))
         {
            input_file = getOption<std::string>(OPT_input_file) + STR_CST_string_separator;
         }
         setOption(OPT_input_file, input_file + GetPath(argv[optind]));
      }
      optind++;
   }

   CheckParameters();

   return PARAMETER_PARSED;
}

void tree_panda_gcc_parameter::CheckParameters()
{
   Parameter::CheckParameters();
}

void tree_panda_gcc_parameter::PrintHelp(std::ostream& os) const
{
   os << "Usage: " << getOption<std::string>(OPT_program_name) << " [options] <input_file1> [<input_file2> ... <input_fileN>]" << std::endl;
   os << std::endl;
   os << "Options: \n"
      << "\n";
   PrintGeneralOptionsUsage(os);
   os << "\n";
   PrintOutputOptionsUsage(os);
   os << "    -o=<file>                        Specify the output file name\n"
      << "\n";
   PrintGccOptionsUsage(os);
   os << "\n";
}

void tree_panda_gcc_parameter::PrintProgramName(std::ostream& os) const
{
   os << "********************************************************************************" << std::endl;
   os << "   _                                             _" << std::endl;
   os << "  | |_ _ __ ___  ___       _ __   __ _ _ __   __| | __ _        __ _  ___ ___" << std::endl;
   os << "  | __| '__/ _ \\/ _ \\_____| '_ \\ / _` | '_ \\ / _` |/ _` |_____ / _` |/ __/ __|" << std::endl;
   os << "  | |_| | |  __/  __/_____| |_) | (_| | | | | (_| | (_| |_____| (_| | (_| (__" << std::endl;
   os << "   \\__|_|  \\___|\\___|     | .__/ \\__,_|_| |_|\\__,_|\\__,_|      \\__, |\\___\\___|" << std::endl;
   os << "                          |_|                                  |___/" << std::endl;
   os << "********************************************************************************" << std::endl;
}

void tree_panda_gcc_parameter::SetDefaults()
{
   // ---------- general options ----------- //
   /// Debugging level
   setOption(OPT_debug_level, DEBUG_LEVEL_NONE);
   /// Output level
   setOption(OPT_output_level, OUTPUT_LEVEL_NONE);

   setOption(OPT_gcc_opt_level, GccWrapper_OptimizationSet::O0);

   setOption(OPT_print_dot, false);

   /// ---------- frontend analysis ----------//
   setOption(OPT_parse_pragma, false);
#if HAVE_FROM_RTL_BUILT
   setOption(OPT_use_rtl, false);
#endif
   setOption(OPT_frontend_statistics, false);
   setOption(OPT_pretty_print, false);
#if HAVE_MAPPING_BUILT
   setOption(OPT_driving_component_type, "ARM");
#endif

   // -- GCC options -- //
#if HAVE_I386_GCC47_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC47));
#elif HAVE_I386_GCC46_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC46));
#elif HAVE_I386_GCC45_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC45));
#elif HAVE_I386_GCC48_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC48));
#elif HAVE_I386_GCC49_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC49));
#elif HAVE_I386_GCC5_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC5));
#elif HAVE_I386_GCC6_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC6));
#elif HAVE_I386_GCC7_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC7));
#elif HAVE_I386_GCC8_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC8));
#elif HAVE_I386_CLANG4_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG4));
#elif HAVE_I386_CLANG5_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG5));
#elif HAVE_I386_CLANG6_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG6));
#elif HAVE_I386_CLANG7_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG7));
#elif HAVE_I386_CLANG8_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG8));
#elif HAVE_I386_CLANG9_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG9));
#else
   THROW_ERROR("No GCC compiler available");
#endif
   setOption(OPT_compatible_compilers, 0
#if HAVE_I386_GCC45_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC45)
#endif
#if HAVE_I386_GCC46_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC46)
#endif
#if HAVE_I386_GCC47_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC47)
#endif
#if HAVE_I386_GCC48_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC48)
#endif
#if HAVE_I386_GCC49_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC49)
#endif
#if HAVE_I386_GCC5_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC5)
#endif
#if HAVE_I386_GCC6_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC6)
#endif
#if HAVE_I386_GCC7_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC7)
#endif
#if HAVE_I386_GCC8_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC8)
#endif
#if HAVE_I386_CLANG4_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG4)
#endif
#if HAVE_I386_CLANG5_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG5)
#endif
#if HAVE_I386_CLANG6_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG6)
#endif
#if HAVE_I386_CLANG7_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG7)
#endif
#if HAVE_I386_CLANG8_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG8)
#endif
#if HAVE_I386_CLANG9_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG9)
#endif
#if HAVE_ARM_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_ARM_GCC)
#endif
#if HAVE_SPARC_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_SPARC_GCC)
#endif
   );
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
   setOption(OPT_without_transformation, true);
   setOption(OPT_precision, 3);
   setOption(OPT_compute_size_of, true);
   setOption(OPT_gcc_c, true);
   setOption(OPT_gcc_config, false);
   setOption(OPT_gcc_costs, false);
   setOption(OPT_gcc_openmp_simd, 0);
   setOption(OPT_no_clean, false);
#if HAVE_BAMBU_BUILT
   setOption(OPT_gcc_optimization_set, GccWrapper_OptimizationSet::OBAMBU);
#elif HAVE_ZEBU_BUILT
   setOption(OPT_gcc_optimization_set, GccWrapper_OptimizationSet::OZEBU);
#else
   setOption(OPT_gcc_optimization_set, OS_OZEBU);
#endif
   setOption(OPT_gcc_include_sysdir, false);
   setOption(OPT_model_costs, false);
   setOption(OPT_output_directory, ".");
#if HAVE_HLS_BUILT
   setOption(OPT_interface_type, HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION);
#endif
}
