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
 * @file Parameter.cpp
 * @brief
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "Parameter.hpp"

#include "BambuParameterRegistry.hpp"

#include "constant_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "polixml.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

namespace
{
   std::string JoinValues(const std::vector<std::string>& values, const std::string& separator)
   {
      std::string joined;
      for(std::size_t i = 0; i < values.size(); ++i)
      {
         if(i != 0)
         {
            joined += separator;
         }
         joined += values[i];
      }
      return joined;
   }

   void PrintPandaParameterInfo(std::ostream& os, const PandaParameterInfo& info, bool show_details)
   {
      os << info.name;
      if(info.type != PandaParamType::Unknown)
      {
         os << " [" << PandaParamTypeToString(info.type) << "]";
      }
      if(!info.default_value.empty())
      {
         os << " default=" << info.default_value;
      }
      if(!info.category.empty())
      {
         os << " category=" << info.category;
      }
      os << "\n";
      if(!info.description.empty())
      {
         os << "  " << info.description << "\n";
      }
      if(show_details)
      {
         if(!info.allowed_values.empty())
         {
            os << "  allowed-values: " << JoinValues(info.allowed_values, ", ") << "\n";
         }
#ifndef NDEBUG
         if(!info.declared_in.empty())
         {
            os << "  declared-in: " << info.declared_in << "\n";
         }
#endif
      }
   }

   void PrintPandaParameterList(std::ostream& os)
   {
      const auto parameters = ListPandaParameters();
      if(parameters.empty())
      {
         os << "No bambu-parameters registered.\n";
         return;
      }
      for(const auto& info : parameters)
      {
         PrintPandaParameterInfo(os, info, false);
      }
   }
} // namespace

#include "config_HAVE_FROM_C_BUILT.hpp"
#include "config_HAVE_I386_CLANG10_COMPILER.hpp"
#include "config_HAVE_I386_CLANG11_COMPILER.hpp"
#include "config_HAVE_I386_CLANG12_COMPILER.hpp"
#include "config_HAVE_I386_CLANG13_COMPILER.hpp"
#include "config_HAVE_I386_CLANG16_COMPILER.hpp"
#include "config_HAVE_I386_CLANG19_COMPILER.hpp"
#include "config_HAVE_I386_CLANG4_COMPILER.hpp"
#include "config_HAVE_I386_CLANG5_COMPILER.hpp"
#include "config_HAVE_I386_CLANG6_COMPILER.hpp"
#include "config_HAVE_I386_CLANG7_COMPILER.hpp"
#include "config_HAVE_I386_CLANG8_COMPILER.hpp"
#include "config_HAVE_I386_CLANG9_COMPILER.hpp"
#include "config_HAVE_LIBBAMBU_M32.hpp"
#include "config_HAVE_LIBBAMBU_M64.hpp"
#include "config_HAVE_LIBBAMBU_MX32.hpp"
#include "config_PACKAGE_BUGREPORT.hpp"
#include "config_PACKAGE_STRING.hpp"
#include "config_RELEASE.hpp"

#if HAVE_HLS_BUILT
#include "constraints_xml.hpp"
#endif
#if HAVE_TECHNOLOGY_BUILT
#include "technology_xml.hpp"
#endif

#if HAVE_FROM_C_BUILT
#include "CompilerWrapper.hpp"
#include "token_interface.hpp"
#endif

#include <cstdlib>
#include <filesystem>
#include <iosfwd>

const std::string branch_name = {
#include "branch_name.hpp"
};

const std::string revision_hash = {
#include "revision_hash.hpp"
};

#define __TO_STRING_HELPER(r, data, elem) {BOOST_PP_CAT(OPT_, elem), "OPT_" BOOST_PP_STRINGIZE(elem)},

const CustomMap<enum enum_option, std::string> Parameter::option_name = {
    BOOST_PP_SEQ_FOR_EACH(__TO_STRING_HELPER, BOOST_PP_EMPTY, BAMBU_OPTIONS)
        BOOST_PP_SEQ_FOR_EACH(__TO_STRING_HELPER, BOOST_PP_EMPTY, EUCALIPTUS_OPTIONS)
            BOOST_PP_SEQ_FOR_EACH(__TO_STRING_HELPER, BOOST_PP_EMPTY, FRAMEWORK_OPTIONS)
                BOOST_PP_SEQ_FOR_EACH(__TO_STRING_HELPER, BOOST_PP_EMPTY, COMPILER_OPTIONS)
                    BOOST_PP_SEQ_FOR_EACH(__TO_STRING_HELPER, BOOST_PP_EMPTY, SYNTHESIS_OPTIONS)
                        BOOST_PP_SEQ_FOR_EACH(__TO_STRING_HELPER, BOOST_PP_EMPTY, BAMBUCC_OPTIONS)};

Parameter::Parameter(const std::string& _program_name, int _argc, char** const _argv, int _debug_level)
    : argc(_argc), argv(_argv), debug_level(_debug_level)
{
   setOption(OPT_program_name, _program_name);
   SetCommonDefaults();
}

Parameter::Parameter(const Parameter& other)
    : argc(other.argc),
      argv(other.argv),
      Options(other.Options),
      bambu_parameters(other.bambu_parameters),
      bambu_parameters_cli(other.bambu_parameters_cli),
      enum_options(other.enum_options),
      debug_classes(other.debug_classes),
      debug_level(other.debug_level)
{
}

void Parameter::CheckParameters()
{
   const auto setup_dir = [](const std::filesystem::path& dir) {
      std::filesystem::create_directories(dir);
      if(!std::filesystem::exists(dir))
      {
         THROW_ERROR("Unable to create directory " + dir.string());
      }
   };

   if(!isOption(OPT_output_directory))
   {
      setOption(OPT_output_directory, std::filesystem::current_path().string());
   }
   if(!isOption(OPT_output_temporary_directory))
   {
      setOption(OPT_output_temporary_directory,
                getOption<std::string>(OPT_output_directory) + "/" STR_CST_temporary_directory);
   }
   if(!isOption(OPT_output_hls_directory))
   {
      setOption(OPT_output_hls_directory, getOption<std::string>(OPT_output_directory) + "/HLS_output");
   }

   setup_dir(getOption<std::filesystem::path>(OPT_output_directory));
   // Remove stale temporary directory from a previous run to avoid leftover artifacts
   const auto temp_dir = getOption<std::filesystem::path>(OPT_output_temporary_directory);
   std::filesystem::remove_all(temp_dir);
   setup_dir(temp_dir);
   setup_dir(getOption<std::filesystem::path>(OPT_output_hls_directory));
   if((!debug_classes.empty() && (!IsParameter("print-dot-FF") || GetParameter<unsigned int>("print-dot-FF"))))
   {
      setOption(OPT_print_dot, true);
   }
   setOption(OPT_dot_directory, getOption<std::string>(OPT_output_hls_directory) + "/dot");
   if(getOption<bool>(OPT_print_dot))
   {
      setup_dir(getOption<std::filesystem::path>(OPT_dot_directory));
   }

#if HAVE_FROM_C_BUILT
   if(isOption(OPT_default_compiler))
   {
      const auto default_compiler = getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
      if(isOption(OPT_cc_m_env))
      {
         const auto mopt = getOption<std::string>(OPT_cc_m_env);
         if(false
#if !HAVE_LIBBAMBU_M64
            || mopt == "-m64"
#endif
#if !HAVE_LIBBAMBU_M32
            || mopt == "-m32"
#endif
#if !HAVE_LIBBAMBU_MX32
            || mopt == "-mx32"
#endif
         )
         {
            THROW_ERROR("Option " + mopt + " not supported by " + CompilerWrapper::getCompilerSuffix(default_compiler) +
                        " compiler.");
         }
      }
      else
      {
#if HAVE_LIBBAMBU_M32
         setOption(OPT_cc_m_env, "-m32");
#elif HAVE_LIBBAMBU_MX32
         setOption(OPT_cc_m_env, "-mx32");
#elif HAVE_LIBBAMBU_M64
         setOption(OPT_cc_m_env, "-m64");
#endif
      }
   }
#endif
}

void Parameter::SetCommonDefaults()
{
   setOption(STR_OPT_benchmark_fake_parameters, "<none>");

   setOption(OPT_print_dot, false);

   setOption(OPT_no_clean, false);
   if(revision_hash == "")
   {
      setOption(OPT_revision, "unknown-trunk");
   }
   else
   {
      setOption(OPT_revision, revision_hash + (branch_name != "" ? "-" + branch_name : ""));
   }
   setOption(OPT_seed, 3869983262);

   setOption(OPT_max_transformations, std::numeric_limits<size_t>::max());
   setOption(OPT_find_max_transformations, false);
}

void Parameter::print(std::ostream& os) const
{
   os << "List of parameters: " << std::endl;
   for(const auto& [key, val] : Options)
   {
      os << key << ": " << val << std::endl;
   }
   for(const auto& [ekey, val] : enum_options)
   {
      os << option_name.at(ekey) << ": " << val << std::endl;
   }
   os << " === " << std::endl;
}

int Parameter::get_class_debug_level(const std::string& class_name, int _debug_level) const
{
   auto temp = class_name;
   temp.erase(std::remove(temp.begin(), temp.end(), '_'), temp.end());
   if(debug_classes.find(boost::to_upper_copy(temp)) != debug_classes.end() ||
      debug_classes.find(STR_CST_debug_all) != debug_classes.end())
   {
      return DEBUG_LEVEL_INFINITE;
   }
   else if(_debug_level < 0)
   {
      return getOption<int>(OPT_debug_level);
   }
   else
   {
      return _debug_level;
   }
}

int Parameter::GetFunctionDebugLevel(const std::string& class_name, const std::string& function_name) const
{
   auto canonic_class_name = class_name;
   canonic_class_name.erase(std::remove(canonic_class_name.begin(), canonic_class_name.end(), '_'),
                            canonic_class_name.end());
   auto canonic_function_name = function_name;
   canonic_function_name.erase(std::remove(canonic_function_name.begin(), canonic_function_name.end(), '_'),
                               canonic_function_name.end());
   const auto canonic_full_function_name = canonic_class_name + std::string("::") + canonic_function_name;
   if(debug_classes.find(boost::to_upper_copy(canonic_full_function_name)) != debug_classes.end())
   {
      return DEBUG_LEVEL_INFINITE;
   }
   else
   {
      return get_class_debug_level(class_name);
   }
}

void Parameter::add_debug_class(const std::string& class_name)
{
   auto temp = class_name;
   temp.erase(std::remove(temp.begin(), temp.end(), '_'), temp.end());
   debug_classes.insert(boost::to_upper_copy(temp));
}

void Parameter::PrintFullHeader(std::ostream& os) const
{
   PrintProgramName(os);
   os << "                         Politecnico di Milano - DEIB" << std::endl;
   os << "                          System Architectures Group" << std::endl;
   os << "********************************************************************************" << std::endl;
   os << "                Copyright (C) 2004-2026 Politecnico di Milano" << std::endl;
   std::string version = PrintVersion();
   if(version.size() < 80)
   {
      os << std::string(40 - (version.size() / 2), ' ') << version << std::endl;
   }
   else
   {
      os << version << std::endl;
   }
   os << std::endl;
}

std::string Parameter::PrintVersion() const
{
   return std::string("Version: ") + PACKAGE_STRING + " - Revision " + getOption<std::string>(OPT_revision);
}

void Parameter::PrintUsage(std::ostream& os) const
{
   PrintFullHeader(os);
   PrintHelp(os);
}

bool Parameter::ManageDefaultOptions(int next_option, char* optarg_param, bool& exit_success)
{
   exit_success = false;
   switch(next_option)
   {
      case INPUT_OPT_NO_CLEAN:
         setOption(OPT_no_clean, true);
         break;
      case 'h': // print help message and exit
         PrintUsage(std::cout);
         exit_success = true;
         break;
      case 'V':
         PrintFullHeader(std::cout);
         exit_success = true;
         break;
      case 'v':
      {
         setOption(OPT_output_level, optarg_param);
         break;
      }
      case OPT_BENCHMARK_NAME:
      {
         setOption(OPT_benchmark_name, optarg_param);
         break;
      }
      case OPT_BENCHMARK_FAKE_PARAMETERS:
      {
         setOption(STR_OPT_benchmark_fake_parameters, optarg_param);
         break;
      }
#ifndef NDEBUG
      case INPUT_OPT_MAX_TRANSFORMATIONS:
      {
         setOption(OPT_max_transformations, optarg_param);
         break;
      }
      case INPUT_OPT_FIND_MAX_TRANSFORMATIONS:
      {
         setOption(OPT_find_max_transformations, true);
         break;
      }
#endif
      case INPUT_OPT_CONFIGURATION_NAME:
      {
         setOption(OPT_configuration_name, optarg_param);
         break;
      }
      case 'd':
      {
#if HAVE_FROM_C_BUILT
         if(std::string(optarg_param) == "umpversion")
         {
            const auto preferred_compiler = getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
            PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0,
                          CompilerWrapper::getCompilerVersion(static_cast<int>(preferred_compiler)));
            exit_success = true;
            break;
         }
#endif
         if(std::string(optarg_param) == "N")
         {
            appendOption(OPT_cc_extra_options, "-dN", " ");
            break;
         }
#ifndef NDEBUG
         else
         {
            debug_level = std::stoi(optarg_param);
            setOption(OPT_debug_level, optarg_param);
            break;
         }
#endif
         return true;
      }
#ifndef NDEBUG
      case OPT_DEBUG_CLASSES:
      {
         const auto Splitted = string_to_container<std::vector<std::string>>(optarg_param, ",");
         for(const auto& i : Splitted)
         {
            add_debug_class(i);
         }
         setOption(OPT_no_clean, true);
         break;
      }
#endif
      case INPUT_OPT_ERROR_ON_WARNING:
      {
         error_on_warning = true;
         break;
      }
      case INPUT_OPT_PRINT_DOT:
      {
         setOption(OPT_print_dot, true);
         break;
      }
      case INPUT_OPT_SEED:
      {
         setOption(OPT_seed, optarg_param);
         break;
      }
      case OPT_OUTPUT_DIRECTORY:
      {
         setOption(OPT_output_directory, optarg_param);
         break;
      }
      case OPT_OUTPUT_TEMPORARY_DIRECTORY:
      {
         const auto path =
             std::filesystem::path(optarg_param) / unique_path(STR_CST_temporary_directory "-%%%%-%%%%-%%%%-%%%%");
         setOption(OPT_output_temporary_directory, path.string());
         break;
      }
      case INPUT_OPT_PANDA_PARAMETER:
      {
         const auto splitted = string_to_container<std::vector<std::string>>(optarg_param, "=");
         if(splitted.size() != 2)
         {
            THROW_ERROR("bambu-parameter should be in the form <parameter>=<value>: " + std::string(optarg_param));
         }
         SetPandaParameterFromCli(splitted[0], splitted[1]);
         break;
      }
      case INPUT_OPT_LIST_PANDA_PARAMETERS:
      {
         PrintPandaParameterList(std::cout);
         exit_success = true;
         break;
      }
      default:
      {
         /// next_option is not a Tool parameter
         return true;
      }
   }
   return false;
}

#if HAVE_FROM_C_BUILT
bool Parameter::ManageCCOptions(int next_option, char* optarg_param)
{
   switch(next_option)
   {
      case 'D':
      {
         appendOption(OPT_cc_defines, std::string(optarg_param));
         break;
      }
      case 'f':
      {
         if(std::string(optarg_param).find("openmp") != std::string::npos)
         {
            setOption(OPT_openmp, true);
            break;
         }
         else
         {
            std::string optimizations;
            if(isOption(OPT_cc_optimizations))
            {
               optimizations = getOption<std::string>(OPT_cc_optimizations) + STR_CST_string_separator;
            }
            THROW_ASSERT(optarg_param != nullptr && optarg_param[0] != 0, "-f alone not allowed");
            setOption(OPT_cc_optimizations, optimizations + optarg_param);
            break;
         }
      }
      case 'g':
      {
         ///-g not managed at all
         break;
      }
      case 'm':
      {
         if(optarg_param)
         {
            const std::string opt_level(optarg_param);
            if(opt_level == "32")
            {
               setOption(OPT_cc_m_env, "-m32");
            }
            else if(opt_level == "x32")
            {
               setOption(OPT_cc_m_env, "-mx32");
            }
            else if(opt_level == "64")
            {
               setOption(OPT_cc_m_env, "-m64");
            }
            else
            {
               THROW_ERROR("Unknown environment bitsize: -m" + opt_level);
            }
         }
         break;
      }
      case 'W':
      {
         std::string cc_warnings;
         if(isOption(OPT_cc_warnings))
         {
            cc_warnings = getOption<std::string>(OPT_cc_warnings) + STR_CST_string_separator;
         }
         setOption(OPT_cc_warnings, cc_warnings + optarg_param);
         break;
      }
      case 'E':
      {
         setOption(OPT_cc_E, true);
         break;
      }
      case 'I':
      {
         appendOption(OPT_cc_includes, "-I " + std::string(optarg), " ");
         break;
      }
      case 'l':
      {
         std::string libraries;
         if(isOption(OPT_cc_libraries))
         {
            libraries = getOption<std::string>(OPT_cc_libraries) + STR_CST_string_separator;
         }
         setOption(OPT_cc_libraries, libraries + optarg_param);
         break;
      }
      case 'L':
      {
         std::string library_directories;
         if(isOption(OPT_cc_library_directories))
         {
            library_directories = getOption<std::string>(OPT_cc_library_directories) + STR_CST_string_separator;
         }
         setOption(OPT_cc_library_directories, library_directories + std::string(optarg_param));
         break;
      }
      case 'O':
      {
         if(optarg_param)
         {
            const std::string opt_level = std::string(optarg_param);
            if(opt_level == "0")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O0);
            }
            else if(opt_level == "1")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O1);
            }
            else if(opt_level == "2")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O2);
            }
            else if(opt_level == "3")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O3);
            }
            else if(opt_level == "4")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O4);
            }
            else if(opt_level == "5")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O5);
            }
            else if(opt_level == "s")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::Os);
            }
            else if(opt_level == "fast")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::Ofast);
            }
            else if(opt_level == "z")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::Oz);
            }
            else
            {
               THROW_ERROR("Unknown optimization level: " + opt_level);
            }
         }
         else
         {
            setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O1);
         }
         break;
      }
      case 'U':
      {
         std::string undefines;
         if(isOption(OPT_cc_undefines))
         {
            undefines = getOption<std::string>(OPT_cc_undefines) + STR_CST_string_separator;
         }
         if(std::string(optarg_param).find('=') != std::string::npos)
         {
            bool has_parenthesis = std::string(optarg_param).find('(') != std::string::npos &&
                                   std::string(optarg_param).find(')') != std::string::npos;
            std::string temp_var = std::string(optarg_param);
            boost::replace_first(temp_var, "=", "=\'");
            if(has_parenthesis)
            {
               undefines += "\'" + temp_var + "\'" + "\'";
            }
            else
            {
               undefines += temp_var + "\'";
            }
         }
         else
         {
            setOption(OPT_cc_undefines, undefines + optarg_param);
         }
         break;
      }
      case 'x':
      {
         std::string cc_extra_options = "-x " + std::string(optarg);
         if(isOption(OPT_cc_extra_options))
         {
            cc_extra_options = getOption<std::string>(OPT_cc_extra_options) + " " + cc_extra_options;
         }
         setOption(OPT_cc_extra_options, cc_extra_options);
         setOption(OPT_cc_xlang, std::string(optarg));
         break;
      }
      case INPUT_OPT_CUSTOM_OPTIONS:
      {
         appendOption(OPT_cc_extra_options, std::string(optarg), " ");
         break;
      }
      case INPUT_OPT_SYSROOT:
      {
         appendOption(OPT_cc_extra_options, "--sysroot=" + std::string(optarg), " ");
         break;
      }
      case INPUT_OPT_COMPILER:
      {
#if HAVE_I386_CLANG4_COMPILER
         if(std::string(optarg_param) == "I386_CLANG4")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG4);
            break;
         }
#endif
#if HAVE_I386_CLANG5_COMPILER
         if(std::string(optarg_param) == "I386_CLANG5")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG5);
            break;
         }
#endif
#if HAVE_I386_CLANG6_COMPILER
         if(std::string(optarg_param) == "I386_CLANG6")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG6);
            break;
         }
#endif
#if HAVE_I386_CLANG7_COMPILER
         if(std::string(optarg_param) == "I386_CLANG7")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG7);
            break;
         }
#endif
#if HAVE_I386_CLANG8_COMPILER
         if(std::string(optarg_param) == "I386_CLANG8")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG8);
            break;
         }
#endif
#if HAVE_I386_CLANG9_COMPILER
         if(std::string(optarg_param) == "I386_CLANG9")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG9);
            break;
         }
#endif
#if HAVE_I386_CLANG10_COMPILER
         if(std::string(optarg_param) == "I386_CLANG10")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG10);
            break;
         }
#endif
#if HAVE_I386_CLANG11_COMPILER
         if(std::string(optarg_param) == "I386_CLANG11")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG11);
            break;
         }
#endif
#if HAVE_I386_CLANG12_COMPILER
         if(std::string(optarg_param) == "I386_CLANG12")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG12);
            break;
         }
#endif
#if HAVE_I386_CLANG13_COMPILER
         if(std::string(optarg_param) == "I386_CLANG13")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG13);
            break;
         }
#endif
#if HAVE_I386_CLANG16_COMPILER
         if(std::string(optarg_param) == "I386_CLANG16")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG16);
            break;
         }
#endif
#if HAVE_I386_CLANG19_COMPILER
         if(std::string(optarg_param) == "I386_CLANG19")
         {
            setOption(OPT_default_compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG19);
            break;
         }
#endif
         THROW_ERROR("Unknown compiler " + std::string(optarg_param));
         break;
      }
      case INPUT_OPT_PARAM:
      {
         std::string parameters;
         if(isOption(OPT_cc_parameters))
         {
            parameters = getOption<std::string>(OPT_cc_parameters) + STR_CST_string_separator;
         }
         setOption(OPT_cc_parameters, parameters + optarg_param);
         break;
      }
      case INPUT_OPT_STD:
      {
         setOption(OPT_cc_standard, optarg_param);
         break;
      }
      case INPUT_OPT_USE_RAW:
      {
         setOption(OPT_input_format, Parameters_FileFormat::FF_RAW);
         break;
      }
      default:
      {
         /// next_option is not a CLANG parameter
         return true;
      }
   }
   return false;
}
#endif

Parameters_FileFormat Parameter::GetFileFormat(const std::filesystem::path& file_name,
                                               const bool check_xml_root_node) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Getting file format of file " + file_name.string());
   auto extension = file_name.extension().string();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Extension is " + extension);
   if(isOption(OPT_cc_xlang))
   {
      const auto xlang_string = getOption<std::string>(OPT_cc_xlang);
      if(xlang_string == "c" || xlang_string == "c-header")
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--User-required C frontend");
         return Parameters_FileFormat::FF_C;
      }
      else if(xlang_string.find("c++") != std::string::npos)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--User-required C++ frontend");
         return Parameters_FileFormat::FF_CPP;
      }
      else if(xlang_string.find("f77") != std::string::npos || xlang_string.find("f95") != std::string::npos)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--User-required FORTRAN frontend");
         return Parameters_FileFormat::FF_FORTRAN;
      }
   }
#if HAVE_FROM_C_BUILT
   if(extension == ".c" || extension == ".i")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--C source file");
      return Parameters_FileFormat::FF_C;
   }
   if(extension == ".m" || extension == ".mi")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Objective C source file");
      return Parameters_FileFormat::FF_OBJECTIVEC;
   }
   if(extension == ".mm" || extension == ".M" || extension == ".mii")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Objective C++ source file");
      return Parameters_FileFormat::FF_OBJECTIVECPP;
   }
   if(extension == ".ii" || extension == ".cc" || extension == ".cp" || extension == ".cxx" || extension == ".cpp" ||
      extension == ".CPP" || extension == ".c++" || extension == ".C")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--C++ source file");
      return Parameters_FileFormat::FF_CPP;
   }
   if(extension == ".f" || extension == ".for" || extension == ".ftn" || extension == ".F" || extension == ".FOR" ||
      extension == ".fpp" || extension == ".FPP" || extension == ".FTN" || extension == ".f90" || extension == ".f95" ||
      extension == ".f03" || extension == ".f08" || extension == ".F90" || extension == ".F95" || extension == ".F03" ||
      extension == ".F08")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fortran source file");
      return Parameters_FileFormat::FF_FORTRAN;
   }
   if(extension == ".ll" || extension == ".bc")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--LLVM bitcode source file");
      const auto sub_extension = std::filesystem::path(file_name).replace_extension().extension().string();
      if(sub_extension == ".cpp")
      {
         return Parameters_FileFormat::FF_LLVM_CPP;
      }
      else
      {
         return Parameters_FileFormat::FF_LLVM;
      }
   }
   if(extension == ".LL" || extension == ".BC")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--LLVM bitcode source file");

      return Parameters_FileFormat::FF_LLVM_CPP;
   }
#endif
   if(extension == ".csv")
   {
      const auto sub_extension = std::filesystem::path(file_name).replace_extension().extension().string();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--generic CSV");
      return Parameters_FileFormat::FF_CSV;
   }
   if(extension == ".tex")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Latex table");
      return Parameters_FileFormat::FF_TEX;
   }
   if(extension == ".v")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--verilog");
      return Parameters_FileFormat::FF_VERILOG;
   }
   if(extension == ".vhd" || extension == ".vhdl")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--vhdl");
      return Parameters_FileFormat::FF_VHDL;
   }
   if(extension == ".xml")
   {
      if(check_xml_root_node)
      {
         XMLDomParser parser(file_name.string());
         parser.Exec();
         THROW_ASSERT(parser, "Impossible to parse xml file " + file_name.string());

         const xml_element* root = parser.get_document()->get_root_node();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Root node is " + root->get_name());
#if HAVE_BAMBU_RESULTS_XML
         if(root->get_name() == "bambu_results")
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Bambu results");
            return Parameters_FileFormat::FF_XML_BAMBU_RESULTS;
         }
#endif
#if HAVE_HLS_BUILT
         if(root->get_name() == STR_XML_constraints_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Constraints");
            return Parameters_FileFormat::FF_XML_CON;
         }
#endif
#if HAVE_TECHNOLOGY_BUILT
         if(root->get_name() == STR_XML_technology_target_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Target device information");
            return Parameters_FileFormat::FF_XML_TARGET;
         }
         if(root->get_name() == STR_XML_technology_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Technology libraries");
            return Parameters_FileFormat::FF_XML_TEC;
         }
#endif
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generic XML");
      return Parameters_FileFormat::FF_XML;
   }
#if HAVE_FROM_C_BUILT
   if(std::filesystem::exists(file_name))
   {
      const auto opened_file = fileIO_istream_open(file_name.string());
      std::string line;
      if(!opened_file->eof())
      {
         getline(*opened_file, line);
         if(line.find(STOK(TOK_COMPILER_VERSION)) != std::string::npos)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Raw file");
            return Parameters_FileFormat::FF_RAW;
         }
      }
   }
#endif
   return Parameters_FileFormat::FF_UNKNOWN;
}

void Parameter::PrintBugReport(std::ostream& os) const
{
   os << "Please report bugs to <" << PACKAGE_BUGREPORT << ">\n" << std::endl;
}

void Parameter::PrintGeneralOptionsUsage(std::ostream& os) const
{
   os << "  General options:\n\n"
      << "    --help, -h\n"
      << "        Display this usage information.\n\n"
      << "    --version, -V\n"
      << "        Display the version of the program.\n\n"
      << "    --seed=<number>\n"
      << "        Set the seed of the random number generator (default=0).\n\n"
      << "    --bambu-parameter=<name>=<value>\n"
      << "        Override a bambu-parameter value.\n\n"
      << "    --list-bambu-parameters\n"
      << "        List registered bambu-parameters and exit.\n\n"
      << std::endl;
}

void Parameter::PrintOutputOptionsUsage(std::ostream& os) const
{
   os << "  Output options:\n\n"
      << "    --verbosity, -v <level>\n"
      << "        Set the output verbosity level\n"
      << "        Possible values for <level>:\n"
      << "            0 - NONE\n"
      << "            1 - MINIMUM\n"
      << "            2 - VERBOSE\n"
      << "            3 - PEDANTIC\n"
      << "            4 - VERY PEDANTIC\n"
      << "        (default = 1)\n"
      << "\n"
#if !RELEASE
      << "    --debug, -d <level>\n"
      << "        Set the verbosity level of debugging information\n"
      << "        Possible values for <level>:\n"
      << "            0 - NONE\n"
      << "            1 - MINIMUM\n"
      << "            2 - VERBOSE\n"
      << "            3 - PEDANTIC\n"
      << "            4 - VERY PEDANTIC\n"
      << "        (default = 1).\n\n"
      << "    --debug-classes=<classes_list>\n"
      << "        Set maximum debug level for classes in <classes_list>\n\n"
      << "    --max-transformations=<number>\n"
      << "        Set a maximum number of transformations.\n\n"
      << "        To reduce the disk usage two PandA parameter could be used:\n"
      << "          --bambu-parameter=print-ir-manager=0\n"
      << "          --bambu-parameter=print-dot-FF=0\n\n"
      << "    --find-max-transformations\n"
      << "        Find the maximum number of transformations raising an exception.\n\n"
#endif
      << "    --no-clean\n"
      << "        Do not remove temporary files.\n\n"
      << "    --benchmark-name=<name>\n"
      << "        Set the name of the current benchmark for data collection.\n"
      << "        Mainly useful for data collection from extensive regression tests.\n\n"
      << "    --configuration-name=<name>\n"
      << "        Set the name of the current tool configuration for data collection.\n"
      << "        Mainly useful for data collection from extensive regression tests.\n\n"
      << "    --benchmark-fake-parameters\n"
      << "        Set the parameters string for data collection. The parameters in the\n"
      << "        string are not actually used, but they are used for data collection in\n"
      << "        extensive regression tests.\n\n"
      << "    --output-directory=<path>\n"
      << "        Set the output directory.\n"
      << "        Default is current working directory\n\n"
      << "    --output-temporary-directory=<path>\n"
      << "        Set the directory where temporary files are saved.\n"
      << "        Default is '" << STR_CST_temporary_directory << "'\n\n"
      << "    --print-dot\n"
      << "        Dump to file several different graphs used in the IR of the tool.\n"
      << "        The graphs are saved in .dot files, in graphviz format\n\n"
#if !RELEASE
      << "    --error-on-warning\n"
      << "        Convert all runtime warnings to errors.\n\n"
#endif
       ;
}

#if HAVE_FROM_C_BUILT
void Parameter::PrintCCOptionsUsage(std::ostream& os) const
{
   os << "  CLANG front-end compiler options:\n\n"
      << "    --compiler=<compiler_version>\n"
      << "        Specify which compiler is used.\n"
      << "        Possible values for <compiler_version> are:\n"
#if HAVE_I386_CLANG4_COMPILER
      << "            I386_CLANG4\n"
#endif
#if HAVE_I386_CLANG5_COMPILER
      << "            I386_CLANG5\n"
#endif
#if HAVE_I386_CLANG6_COMPILER
      << "            I386_CLANG6\n"
#endif
#if HAVE_I386_CLANG7_COMPILER
      << "            I386_CLANG7\n"
#endif
#if HAVE_I386_CLANG8_COMPILER
      << "            I386_CLANG8\n"
#endif
#if HAVE_I386_CLANG9_COMPILER
      << "            I386_CLANG9\n"
#endif
#if HAVE_I386_CLANG10_COMPILER
      << "            I386_CLANG10\n"
#endif
#if HAVE_I386_CLANG11_COMPILER
      << "            I386_CLANG11\n"
#endif
#if HAVE_I386_CLANG12_COMPILER
      << "            I386_CLANG12\n"
#endif
#if HAVE_I386_CLANG13_COMPILER
      << "            I386_CLANG13\n"
#endif
#if HAVE_I386_CLANG16_COMPILER
      << "            I386_CLANG16\n"
#endif
#if HAVE_I386_CLANG19_COMPILER
      << "            I386_CLANG19\n"
#endif
      << "\n"
      << "    -O<level>\n"
      << "        Enable a specific optimization level. Possible values are the usual\n"
      << "        optimization flags accepted by compilers, plus some others:\n"
      << "        -O0,-O1,-O2,-O3,-Os,-O4,-O5.\n\n"
      << "    -f<option>\n"
      << "        Enable or disable a CLANG optimization option. All the -f or -fno options\n"
      << "        are supported. In particular, -ftree-vectorize option triggers the\n"
      << "        high-level synthesis of vectorized operations.\n\n"
      << "    -I<path>\n"
      << "        Specify a path where headers are searched for.\n\n"
      << "    -W<warning>\n"
      << "        Specify a warning option passed to CLANG. All the -W options available in\n"
      << "        CLANG are supported.\n\n"
      << "    -E\n"
      << "        Enable preprocessing mode of CLANG.\n\n"
      << "    --std=<standard>\n"
      << "        Assume that the input sources are for <standard>. All\n"
      << "        the --std options available in CLANG are supported.\n"
      << "        The default value is gnu90/gnu11/c11 for C and gnu++98/gnu++14/c++14 for C++ \n"
      << "        depending on the selected frontend compiler support.\n\n"
      << "    -D<name>\n"
      << "        Predefine name as a macro, with definition 1.\n\n"
      << "    -D<name=definition>\n"
      << "        Tokenize <definition> and process as if it appeared as a #define directive.\n\n"
      << "    -U<name>\n"
      << "        Remove existing definition for macro <name>.\n\n"
      << "    --param <name>=<value>\n"
      << "        Set the amount <value> for the CLANG parameter <name> that could be used for\n"
      << "        some optimizations.\n\n"
      << "    -l<library>\n"
      << "        Search the library named <library> when linking.\n\n"
      << "    -L<dir>\n"
      << "        Add directory <dir> to the list of directories to be searched for -l.\n\n"
      << "    --use-raw\n"
      << "        Specify that input file is already a raw file and not a source file.\n\n"
      << "    -m<machine-option>\n"
      << "        Specify machine dependend options (currently not used).\n\n"
      << "    --extra-cc-options\n"
      << "        Specify custom extra options to the compiler.\n\n"
      << std::endl;
}
#endif

const std::vector<std::string> Parameter::CGetArgv() const
{
   std::vector<std::string> ret;
   for(int arg = 0; arg < argc; arg++)
   {
      ret.push_back(std::string(argv[arg]));
   }
   return ret;
}
