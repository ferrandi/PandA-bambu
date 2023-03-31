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
 * @file Parameter.cpp
 * @brief
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "Parameter.hpp"

/// Autoheader include
#include "config_HAVE_ARM_COMPILER.hpp"
#include "config_HAVE_FROM_ARCH_BUILT.hpp"
#include "config_HAVE_FROM_CSV_BUILT.hpp"
#include "config_HAVE_FROM_C_BUILT.hpp"
#include "config_HAVE_FROM_IPXACT_BUILT.hpp"
#include "config_HAVE_FROM_PSPLIB_BUILT.hpp"
#include "config_HAVE_FROM_SDF3_BUILT.hpp"
#include "config_HAVE_I386_CLANG10_COMPILER.hpp"
#include "config_HAVE_I386_CLANG11_COMPILER.hpp"
#include "config_HAVE_I386_CLANG12_COMPILER.hpp"
#include "config_HAVE_I386_CLANG13_COMPILER.hpp"
#include "config_HAVE_I386_CLANG16_COMPILER.hpp"
#include "config_HAVE_I386_CLANG4_COMPILER.hpp"
#include "config_HAVE_I386_CLANG5_COMPILER.hpp"
#include "config_HAVE_I386_CLANG6_COMPILER.hpp"
#include "config_HAVE_I386_CLANG7_COMPILER.hpp"
#include "config_HAVE_I386_CLANG8_COMPILER.hpp"
#include "config_HAVE_I386_CLANG9_COMPILER.hpp"
#include "config_HAVE_I386_CLANGVVD_COMPILER.hpp"
#include "config_HAVE_I386_GCC45_COMPILER.hpp"
#include "config_HAVE_I386_GCC46_COMPILER.hpp"
#include "config_HAVE_I386_GCC47_COMPILER.hpp"
#include "config_HAVE_I386_GCC48_COMPILER.hpp"
#include "config_HAVE_I386_GCC49_COMPILER.hpp"
#include "config_HAVE_I386_GCC5_COMPILER.hpp"
#include "config_HAVE_I386_GCC6_COMPILER.hpp"
#include "config_HAVE_I386_GCC7_COMPILER.hpp"
#include "config_HAVE_I386_GCC8_COMPILER.hpp"
#include "config_HAVE_IPXACT_BUILT.hpp"
#include "config_HAVE_PERFORMANCE_METRICS_XML.hpp"
#include "config_HAVE_REGRESSORS_BUILT.hpp"
#include "config_HAVE_SOURCE_CODE_STATISTICS_XML.hpp"
#include "config_HAVE_SPARC_COMPILER.hpp"
#include "config_HAVE_TO_DATAFILE_BUILT.hpp"
#include "config_HAVE_WEIGHT_MODELS_XML.hpp"
#include "config_PACKAGE_BUGREPORT.hpp"
#include "config_PACKAGE_STRING.hpp"

#if HAVE_FROM_C_BUILT
/// wrapper/compiler
#include "compiler_wrapper.hpp"
#endif

/// Boost include
#include <boost/lexical_cast.hpp>

/// Constants include
#if HAVE_REGRESSORS_BUILT
#include "aggregated_features_xml.hpp"
#include "skip_rows_xml.hpp"
#endif
#if HAVE_FROM_ARCH_BUILT
#include "architecture_xml.hpp"
#endif
#include "constant_strings.hpp"
#include "constants.hpp"
#if HAVE_HLS_BUILT
#include "constraints_xml.hpp"
#endif
#include "experimental_setup_xml.hpp"
#if HAVE_TO_DATAFILE_BUILT
#include "latex_table_xml.hpp"
#endif
#if HAVE_PERFORMANCE_METRICS_XML
#include "metrics_xml.hpp"
#endif
#if HAVE_SOURCE_CODE_STATISTICS_XML
#include "source_code_statistics_xml.hpp"
#endif
#if HAVE_PERFORMANCE_METRICS_XML
#include "probability_distribution_xml.hpp"
#endif
#if HAVE_TECHNOLOGY_BUILT
#include "technology_xml.hpp"
#endif
#if HAVE_FROM_SDF3_BUILT
#include "sdf3_xml.hpp"
#endif

#if HAVE_CODE_ESTIMATION_BUILT
/// design_flows/codesign/estimation include
#include "actor_graph_estimator.hpp"
#endif

#if HAVE_FROM_C_BUILT
#include "token_interface.hpp"
#endif

/// STD include
#include <cstdlib>
#include <iosfwd>

/// Utility include
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include "xml_helper.hpp"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

/// XML include
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#if HAVE_FROM_IPXACT_BUILT
#include "ip_xact_xml.hpp"
/// Constants include
#include "design_analysis_xml.hpp"
#endif

#include "fileIO.hpp"

const std::string branch_name = {
#include "branch_name.hpp"
};

const std::string revision_hash = {
#include "revision_hash.hpp"
};

#define OPTION_NAME(r, data, elem) option_name[BOOST_PP_CAT(OPT_, elem)] = #elem;

Parameter::Parameter(const std::string& _program_name, int _argc, char** const _argv, int _debug_level)
    : argc(_argc), argv(_argv), debug_level(_debug_level)
{
   setOption(OPT_program_name, _program_name);
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, BAMBU_OPTIONS)
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, EUCALIPTUS_OPTIONS)
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, FRAMEWORK_OPTIONS)
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, COMPILER_OPTIONS)
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, GECCO_OPTIONS)
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, KOALA_OPTIONS)
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, SPIDER_OPTIONS)
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, SYNTHESIS_OPTIONS)
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, TREE_PANDA_COMPILER_OPTIONS)
   BOOST_PP_SEQ_FOR_EACH(OPTION_NAME, BOOST_PP_EMPTY, ZEBU_OPTIONS)
   // This part has been added since boost macro does not expand correctly
   std::map<enum enum_option, std::string>::iterator it, it_end = option_name.end();
   for(it = option_name.begin(); it != it_end; it++)
   {
      it->second = "OPT_" + it->second.substr(19);
      it->second = it->second.substr(0, it->second.find(')'));
   }
   SetCommonDefaults();
}

Parameter::Parameter(const Parameter& other)
    : argc(other.argc),
      argv(other.argv),
      Options(other.Options),
      enum_options(other.enum_options),
      option_name(other.option_name),
      debug_classes(other.debug_classes),
      debug_level(other.debug_level)
{
}

void Parameter::CheckParameters()
{
   const auto temporary_directory = getOption<std::string>(OPT_output_temporary_directory);
   if(boost::filesystem::exists(temporary_directory))
   {
      boost::filesystem::remove_all(temporary_directory);
   }
   boost::filesystem::create_directory(temporary_directory);
   /// Output directory is not removed since it can be the current one
   const auto output_directory = getOption<std::string>(OPT_output_directory);
   if(!boost::filesystem::exists(output_directory))
   {
      boost::filesystem::create_directory(output_directory);
   }
   if(!boost::filesystem::exists(output_directory))
   {
      THROW_ERROR("not able to create directory " + output_directory);
   }
   if(getOption<bool>(OPT_print_dot))
   {
      const auto dot_directory = getOption<std::string>(OPT_dot_directory);
      if(boost::filesystem::exists(dot_directory))
      {
         boost::filesystem::remove_all(dot_directory);
      }
      boost::filesystem::create_directory(dot_directory);
      if(!boost::filesystem::exists(dot_directory))
      {
         THROW_ERROR("not able to create directory " + dot_directory);
      }
   }

#if HAVE_FROM_C_BUILT
   if(isOption(OPT_gcc_m32_mx32))
   {
      const auto mopt = getOption<std::string>(OPT_gcc_m32_mx32);
      if(mopt == "-m32" &&
         CompilerWrapper::hasCompilerGCCM32(getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler)))
      {
         setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2");
      }
      else if((mopt == "-m32" && !CompilerWrapper::hasCompilerCLANGM32(
                                     getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler))) ||
              (mopt == "-mx32" &&
               !CompilerWrapper::hasCompilerMX32(getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler))) ||
              (mopt == "-m64" &&
               !CompilerWrapper::hasCompilerM64(getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler))))
      {
         THROW_ERROR("Option " + mopt + " not supported by " +
                     CompilerWrapper::getCompilerSuffix(OPT_default_compiler) + " compiler.");
      }
   }
#endif
}

Parameter::~Parameter() = default;

void Parameter::load_xml_configuration_file_rec(const xml_element* node)
{
   // Recurse through child nodes:
   const xml_node::node_list list = node->get_children();
   for(const auto& iter : list)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter);
      if(!EnodeC)
      {
         continue;
      }
      /// general options
      if(CE_XVM(value, EnodeC))
      {
         Options[GET_NODE_NAME(EnodeC)] = GET_STRING_VALUE(EnodeC);
      }
      if(EnodeC->get_children().size())
      {
         load_xml_configuration_file_rec(EnodeC);
      }
   }
}

void Parameter::load_xml_configuration_file(const std::string& filename)
{
   try
   {
      XMLDomParser parser(filename);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.

         // Recurse through child nodes:
         const xml_node::node_list list = node->get_children();
         for(const auto& iter : list)
         {
            const auto* EnodeC = GetPointer<const xml_element>(iter);
            if(!EnodeC)
            {
               continue;
            }
            /// general options
            if(CE_XVM(value, EnodeC))
            {
               Options[GET_NODE_NAME(EnodeC)] = GET_STRING_VALUE(EnodeC);
            }
            if(EnodeC->get_children().size())
            {
               load_xml_configuration_file_rec(EnodeC);
            }
         }
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
}

void Parameter::write_xml_configuration_file(const std::string& filename)
{
   xml_document document;

   xml_element* parameters = document.create_root_node("parameters");

   for(auto Op = Options.begin(); Op != Options.end(); ++Op)
   {
      xml_element* node = parameters->add_child_element(Op->first);
      WRITE_XNVM2("value", Op->second, node);
   }

   document.write_to_file_formatted(filename);
}

void Parameter::SetCommonDefaults()
{
   setOption(STR_OPT_benchmark_fake_parameters, "<none>");
   std::string current_dir = GetCurrentPath();
   std::string temporary_directory = current_dir + "/" + std::string(STR_CST_temporary_directory);

   setOption(OPT_dot_directory, current_dir + "/dot/");
   setOption(OPT_output_temporary_directory, temporary_directory + "/");
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
   setOption(OPT_seed, 0);

   setOption(OPT_max_transformations, std::numeric_limits<size_t>::max());
   setOption(OPT_find_max_transformations, false);
}

void Parameter::print(std::ostream& os) const
{
   os << "List of parameters: " << std::endl;
   for(const auto& Option : Options)
   {
      os << Option.first << ": " << Option.second << std::endl;
   }
   std::map<enum enum_option, std::string>::const_iterator option, option_end = enum_options.end();
   for(option = enum_options.begin(); option != option_end; ++option)
   {
      os << option_name.find(option->first)->second << ": " << option->second << std::endl;
   }
   os << " === " << std::endl;
}

int Parameter::get_class_debug_level(const std::string& class_name, int _debug_level) const
{
   auto temp = class_name;
   temp.erase(std::remove(temp.begin(), temp.end(), '_'), temp.end());
   if(debug_classes.find(boost::to_upper_copy(temp)) != debug_classes.end() or
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
   os << "                Copyright (C) 2004-2023 Politecnico di Milano" << std::endl;
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
#if !RELEASE
      case OPT_READ_PARAMETERS_XML:
      {
         setOption(OPT_read_parameter_xml, optarg_param);
         load_xml_configuration_file(getOption<std::string>(OPT_read_parameter_xml));
         break;
      }
      case OPT_WRITE_PARAMETERS_XML:
      {
         setOption(OPT_write_parameter_xml, optarg_param);
         break;
      }
#endif
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
            CompilerWrapper_CompilerTarget preferred_compiler;
            preferred_compiler = getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
            PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0,
                          CompilerWrapper::getCompilerVersion(static_cast<int>(preferred_compiler)));
            exit_success = true;
            break;
         }
#endif
         if(std::string(optarg_param) == "N")
         {
            std::string gcc_extra_options = "-dN";
            if(isOption(OPT_gcc_extra_options))
            {
               gcc_extra_options = getOption<std::string>(OPT_gcc_extra_options) + " " + gcc_extra_options;
            }
            setOption(OPT_gcc_extra_options, gcc_extra_options);
            break;
         }
#ifndef NDEBUG
         else
         {
            debug_level = boost::lexical_cast<int>(optarg_param);
            setOption(OPT_debug_level, optarg_param);
            break;
         }
#endif
         return true;
      }
#ifndef NDEBUG
      case OPT_DEBUG_CLASSES:
      {
         std::vector<std::string> Splitted = SplitString(optarg_param, ",");
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
      case OPT_OUTPUT_TEMPORARY_DIRECTORY:
      {
         /// If the path is not absolute, make it into absolute
         std::string path(optarg_param);
         std::string temporary_directory_pattern;
         temporary_directory_pattern = GetPath(path) + "/" + std::string(STR_CST_temporary_directory);
         // The %s are required by the mkdtemp function
         boost::filesystem::path temp_path = temporary_directory_pattern + "-%%%%-%%%%-%%%%-%%%%";

         boost::filesystem::path temp_path_obtained = boost::filesystem::unique_path(temp_path);
         boost::filesystem::create_directories(temp_path_obtained.string());

         path = temp_path_obtained.string();
         path = path + "/";
         setOption(OPT_output_temporary_directory, path);
         break;
      }
      case INPUT_OPT_PANDA_PARAMETER:
      {
         std::string param_pair(optarg_param);
         std::vector<std::string> splitted = SplitString(param_pair, "=");
         if(splitted.size() != 2)
         {
            THROW_ERROR("panda-parameter should be in the form <parameter>=<value>: " + param_pair);
         }
         panda_parameters[splitted[0]] = splitted[1];
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
bool Parameter::ManageGccOptions(int next_option, char* optarg_param)
{
   switch(next_option)
   {
      case 'c':
      {
         setOption(OPT_gcc_c, true);
         break;
      }
      case 'D':
      {
         std::string defines;
         if(isOption(OPT_gcc_defines))
         {
            defines = getOption<std::string>(OPT_gcc_defines) + STR_CST_string_separator;
         }
         if(std::string(optarg_param).find('=') != std::string::npos)
         {
            bool has_parenthesis = std::string(optarg_param).find('(') != std::string::npos &&
                                   std::string(optarg_param).find(')') != std::string::npos;
            std::string temp_var = std::string(optarg_param);
            boost::replace_first(temp_var, "=", "=\'");
            if(has_parenthesis)
            {
               defines += "\'" + temp_var + "\'" + "\'";
            }
            else
            {
               defines += temp_var + "\'";
            }
         }
         else
         {
            defines += std::string(optarg_param);
         }
         setOption(OPT_gcc_defines, defines);
         break;
      }
      case 'f':
      {
         if(std::string(optarg_param).find("openmp-simd") != std::string::npos)
         {
            if(std::string(optarg_param).find('=') != std::string::npos)
            {
               setOption(OPT_gcc_openmp_simd,
                         std::string(optarg_param).substr(std::string(optarg_param).find('=') + 1));
            }
            else
            {
               setOption(OPT_gcc_openmp_simd, 4);
            }
            break;
         }
         else if(std::string(optarg_param).find("openmp") != std::string::npos)
         {
            setOption(OPT_parse_pragma, true);
            break;
         }
         else
         {
            std::string optimizations;
            if(isOption(OPT_gcc_optimizations))
            {
               optimizations = getOption<std::string>(OPT_gcc_optimizations) + STR_CST_string_separator;
            }
            THROW_ASSERT(optarg_param != nullptr && optarg_param[0] != 0, "-f alone not allowed");
            setOption(OPT_gcc_optimizations, optimizations + optarg_param);
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
            const std::string opt_level = std::string(optarg_param);
            if(opt_level == "32")
            {
               setOption(OPT_gcc_m32_mx32, "-m32");
            }
            else if(opt_level == "x32")
            {
               setOption(OPT_gcc_m32_mx32, "-mx32");
            }
            else if(opt_level == "64")
            {
               setOption(OPT_gcc_m32_mx32, "-m64");
            }
         }
         break;
      }
      case 'W':
      {
         std::string gcc_warnings;
         if(isOption(OPT_gcc_warnings))
         {
            gcc_warnings = getOption<std::string>(OPT_gcc_warnings) + STR_CST_string_separator;
         }
         setOption(OPT_gcc_warnings, gcc_warnings + optarg_param);
         break;
      }
      case 'E':
      {
         setOption(OPT_gcc_E, true);
         break;
      }
      case 'I':
      {
         std::string includes = "-I " + GetPath(std::string(optarg));
         if(isOption(OPT_gcc_includes))
         {
            includes = getOption<std::string>(OPT_gcc_includes) + " " + includes;
         }
         setOption(OPT_gcc_includes, includes);
         break;
      }
      case 'l':
      {
         std::string libraries;
         if(isOption(OPT_gcc_libraries))
         {
            libraries = getOption<std::string>(OPT_gcc_libraries) + STR_CST_string_separator;
         }
         setOption(OPT_gcc_libraries, libraries + optarg_param);
         break;
      }
      case 'L':
      {
         std::string library_directories;
         if(isOption(OPT_gcc_library_directories))
         {
            library_directories = getOption<std::string>(OPT_gcc_library_directories) + STR_CST_string_separator;
         }
         setOption(OPT_gcc_library_directories, library_directories + GetPath(optarg_param));
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
            else if(opt_level == "g")
            {
               setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::Og);
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
         if(isOption(OPT_gcc_undefines))
         {
            undefines = getOption<std::string>(OPT_gcc_undefines) + STR_CST_string_separator;
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
            setOption(OPT_gcc_undefines, undefines + optarg_param);
         }
         break;
      }
      case INPUT_OPT_CUSTOM_OPTIONS:
      {
         setOption(OPT_gcc_extra_options, optarg);
         break;
      }
#if !RELEASE
      case INPUT_OPT_COMPUTE_SIZEOF:
      {
         setOption(OPT_compute_size_of, true);
         break;
      }
#endif
      case INPUT_OPT_COMPILER:
      {
#if HAVE_ARM_COMPILER
         if(std::string(optarg_param) == "ARM")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_ARM_GCC));
            break;
         }
#endif
#if HAVE_SPARC_COMPILER
         if(std::string(optarg_param) == "SPARC")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_SPARC_GCC));
            break;
         }
#endif
#if HAVE_I386_GCC45_COMPILER
         if(std::string(optarg_param) == "I386_GCC45")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC45));
            break;
         }
#endif
#if HAVE_I386_GCC46_COMPILER
         if(std::string(optarg_param) == "I386_GCC46")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC46));
            break;
         }
#endif
#if HAVE_I386_GCC47_COMPILER
         if(std::string(optarg_param) == "I386_GCC47")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC47));
            break;
         }
#endif
#if HAVE_I386_GCC48_COMPILER
         if(std::string(optarg_param) == "I386_GCC48")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC48));
            break;
         }
#endif
#if HAVE_I386_GCC49_COMPILER
         if(std::string(optarg_param) == "I386_GCC49")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC49));
            break;
         }
#endif
#if HAVE_I386_GCC5_COMPILER
         if(std::string(optarg_param) == "I386_GCC5")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC5));
            break;
         }
#endif
#if HAVE_I386_GCC6_COMPILER
         if(std::string(optarg_param) == "I386_GCC6")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC6));
            break;
         }
#endif
#if HAVE_I386_GCC7_COMPILER
         if(std::string(optarg_param) == "I386_GCC7")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC7));
            break;
         }
#endif
#if HAVE_I386_GCC8_COMPILER
         if(std::string(optarg_param) == "I386_GCC8")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC8));
            break;
         }
#endif
#if HAVE_I386_CLANG4_COMPILER
         if(std::string(optarg_param) == "I386_CLANG4")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG4));
            break;
         }
#endif
#if HAVE_I386_CLANG5_COMPILER
         if(std::string(optarg_param) == "I386_CLANG5")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG5));
            break;
         }
#endif
#if HAVE_I386_CLANG6_COMPILER
         if(std::string(optarg_param) == "I386_CLANG6")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG6));
            break;
         }
#endif
#if HAVE_I386_CLANG7_COMPILER
         if(std::string(optarg_param) == "I386_CLANG7")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG7));
            break;
         }
#endif
#if HAVE_I386_CLANG8_COMPILER
         if(std::string(optarg_param) == "I386_CLANG8")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG8));
            break;
         }
#endif
#if HAVE_I386_CLANG9_COMPILER
         if(std::string(optarg_param) == "I386_CLANG9")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG9));
            break;
         }
#endif
#if HAVE_I386_CLANG10_COMPILER
         if(std::string(optarg_param) == "I386_CLANG10")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG10));
            break;
         }
#endif
#if HAVE_I386_CLANG11_COMPILER
         if(std::string(optarg_param) == "I386_CLANG11")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG11));
            break;
         }
#endif
#if HAVE_I386_CLANG12_COMPILER
         if(std::string(optarg_param) == "I386_CLANG12")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG12));
            break;
         }
#endif
#if HAVE_I386_CLANG13_COMPILER
         if(std::string(optarg_param) == "I386_CLANG13")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG13));
            break;
         }
#endif
#if HAVE_I386_CLANG16_COMPILER
         if(std::string(optarg_param) == "I386_CLANG16")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG16));
            break;
         }
#endif
#if HAVE_I386_CLANGVVD_COMPILER
         if(std::string(optarg_param) == "I386_CLANGVVD")
         {
            setOption(OPT_default_compiler, static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD));
            break;
         }
#endif
         THROW_ERROR("Unknown compiler " + std::string(optarg_param));
         break;
      }
      case INPUT_OPT_GCC_CONFIG:
      {
         setOption(OPT_gcc_config, true);
         break;
      }
      case INPUT_OPT_INCLUDE_SYSDIR:
      {
         setOption(OPT_gcc_include_sysdir, true);
         break;
      }
      case INPUT_OPT_PARAM:
      {
         std::string parameters;
         if(isOption(OPT_gcc_parameters))
         {
            parameters = getOption<std::string>(OPT_gcc_parameters) + STR_CST_string_separator;
         }
         setOption(OPT_gcc_parameters, parameters + optarg_param);
         break;
      }
      case INPUT_OPT_READ_GCC_XML:
      {
         setOption(OPT_gcc_read_xml, GetPath(optarg));
         break;
      }
      case INPUT_OPT_STD:
      {
         setOption(OPT_gcc_standard, optarg_param);
         break;
      }
      case INPUT_OPT_USE_RAW:
      {
         setOption(OPT_input_format, static_cast<int>(Parameters_FileFormat::FF_RAW));
         break;
      }
      case INPUT_OPT_WRITE_GCC_XML:
      {
         setOption(OPT_gcc_write_xml, GetPath(optarg));
         break;
      }
      default:
      {
         /// next_option is not a GCC/CLANG parameter
         return true;
      }
   }
   return false;
}
#endif

Parameters_FileFormat Parameter::GetFileFormat(const std::string& file_name, const bool check_xml_root_node) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Getting file format of file " + file_name);
   std::string extension = GetExtension(file_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Extension is " + extension);
#if HAVE_FROM_AADL_ASN_BUILT
   if(extension == "aadl" or extension == "AADL")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--aadl file");
      return Parameters_FileFormat::FF_AADL;
   }
   if(extension == "asn" or extension == "ASN")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--asn file");
      return Parameters_FileFormat::FF_ASN;
   }
#endif
#if HAVE_FROM_C_BUILT
   if(extension == "c" or extension == "i")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--C source file");
      return Parameters_FileFormat::FF_C;
   }
   if(extension == "m" or extension == "mi")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Objective C source file");
      return Parameters_FileFormat::FF_OBJECTIVEC;
   }
   if(extension == "mm" or extension == "M" or extension == "mii")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Objective C++ source file");
      return Parameters_FileFormat::FF_OBJECTIVECPP;
   }
   if(extension == "ii" or extension == "cc" or extension == "cp" or extension == "cxx" or extension == "cpp" or
      extension == "CPP" or extension == "c++" or extension == "C")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--C++ source file");
      return Parameters_FileFormat::FF_CPP;
   }
   if(extension == "f" or extension == "for" or extension == "ftn" or extension == "F" or extension == "FOR" or
      extension == "fpp" or extension == "FPP" or extension == "FTN" or extension == "f90" or extension == "f95" or
      extension == "f03" or extension == "f08" or extension == "F90" or extension == "F95" or extension == "F03" or
      extension == "F08")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fortran source file");
      return Parameters_FileFormat::FF_FORTRAN;
   }
   if(extension == "ll")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--LLVM bitcode source file");
      auto removed_ll = file_name.substr(0, file_name.size() - 3);
      extension = GetExtension(removed_ll);
      if(extension == "cpp")
      {
         return Parameters_FileFormat::FF_LLVM_CPP;
      }
      else
      {
         return Parameters_FileFormat::FF_LLVM;
      }
   }
   if(extension == "LL")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--LLVM bitcode source file");

      return Parameters_FileFormat::FF_LLVM_CPP;
   }
#endif
   if(extension == "csv")
   {
      std::string base_name = GetLeafFileName(file_name);
#if HAVE_FROM_CSV_BUILT
      if(base_name.find('.') != std::string::npos)
      {
         std::string local_extension = GetExtension(base_name);
         if(local_extension == "rtl")
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--CSV of RTL operations");
            return Parameters_FileFormat::FF_CSV_RTL;
         }
         else if(local_extension == "tree")
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--CSV of TREE operations");
            return Parameters_FileFormat::FF_CSV_TRE;
         }
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--generic CSV");
      return Parameters_FileFormat::FF_CSV;
   }
#if HAVE_FROM_LIBERTY
   if(extension == "lib")
   {
      return Parameters_FileFormat::FF_LIB;
   }
#endif
#if HAVE_FROM_PSPLIB_BUILT
   if(extension == "mm")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Multi mode project scheduling problem");
      return Parameters_FileFormat::FF_PSPLIB_MM;
   }
   if(extension == "sm")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Single-mode project scheduling problem");
      return Parameters_FileFormat::FF_PSPLIB_SM;
   }
#endif
   if(extension == "tex")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Latex table");
      return Parameters_FileFormat::FF_TEX;
   }
   if(extension == "v")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--verilog");
      return Parameters_FileFormat::FF_VERILOG;
   }
   if(extension == "vhd" or extension == "vhdl")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--vhdl");
      return Parameters_FileFormat::FF_VHDL;
   }
   if(extension == "xml")
   {
      if(check_xml_root_node)
      {
         XMLDomParser parser(file_name);
         parser.Exec();
         THROW_ASSERT(parser, "Impossible to parse xml file " + file_name);

#if HAVE_DESIGN_ANALYSIS_BUILT || HAVE_SOURCE_CODE_STATISTICS_XML || HAVE_FROM_IPXACT_BUILT || HAVE_FROM_SDF3_BUILT || \
    HAVE_TO_DATAFILE_BUILT || HAVE_PERFORMANCE_METRICS_XML || HAVE_WEIGHT_MODELS_XML
         const xml_element* root = parser.get_document()->get_root_node();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Root node is " + root->get_name());
#if HAVE_REGRESSORS_BUILT
         if(root->get_name() == STR_XML_aggregated_features_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Aggregated features data");
            return Parameters_FileFormat::FF_XML_AGG;
         }
#endif
#if HAVE_FROM_ARCH_BUILT
         if(root->get_name() == STR_XML_architecture_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Architecture description");
            return Parameters_FileFormat::FF_XML_ARCHITECTURE;
         }
#endif
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
#if HAVE_DESIGN_ANALYSIS_BUILT
         if(root->get_name() == STR_XML_design_analysis_hierarchy)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Design hierarchy");
            return Parameters_FileFormat::FF_XML_DESIGN_HIERARCHY;
         }
#endif
         if(root->get_name() == STR_XML_experimental_setup_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Experimental setup");
            return Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP;
         }
#if HAVE_SOURCE_CODE_STATISTICS_XML
         if(root->get_name() == STR_XML_source_code_statistics_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Source code statistics");
            return Parameters_FileFormat::FF_XML_STAT;
         }
#endif
#if HAVE_FROM_IPXACT_BUILT
         if(root->get_name() == STR_XML_ip_xact_component)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--IP-XACT component");
            return Parameters_FileFormat::FF_XML_IP_XACT_COMPONENT;
         }
         if(root->get_name() == STR_XML_ip_xact_design)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--IP-XACT design");
            return Parameters_FileFormat::FF_XML_IP_XACT_DESIGN;
         }
         if(root->get_name() == STR_XML_ip_xact_generator_chain)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--IP-XACT generator chain");
            return Parameters_FileFormat::FF_XML_IP_XACT_GENERATOR;
         }
         if(root->get_name() == STR_XML_ip_xact_design_configuration)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--IP-XACT design configuration");
            return Parameters_FileFormat::FF_XML_IP_XACT_CONFIG;
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
#if HAVE_TO_DATAFILE_BUILT
         if(root->get_name() == STR_XML_latex_table_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Latex table format");
            return Parameters_FileFormat::FF_XML_TEX_TABLE;
         }
#endif
#if HAVE_FROM_SDF3_BUILT
         if(root->get_name() == STR_XML_sdf3_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Sdf3 format");
            return Parameters_FileFormat::FF_XML_SDF3;
         }
#endif
#if HAVE_REGRESSORS_BUILT
         if(root->get_name() == STR_XML_skip_rows_root)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skip rows data");
            return Parameters_FileFormat::FF_XML_SKIP_ROW;
         }
#endif
#endif
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generic XML");
      return Parameters_FileFormat::FF_XML;
   }
#if HAVE_FROM_C_BUILT
   if(boost::filesystem::exists(file_name))
   {
      const auto opened_file = fileIO_istream_open(file_name);
      std::string line;
      if(!opened_file->eof())
      {
         getline(*opened_file, line);
         if(line.find(STOK(TOK_GCC_VERSION)) != std::string::npos)
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
#if !RELEASE
      << "    --read-parameters-XML=<xml_file_name>\n"
      << "        Read command line options from a XML file.\n\n"
      << "    --write-parameters-XML=<xml_file_name>\n"
      << "        Dump the parsed command line options into a XML file.\n\n"
#endif
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
      << "          --panda-parameter=print-tree-manager=1\n"
      << "          --panda-parameter=print-dot-FF=1\n\n"
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
void Parameter::PrintGccOptionsUsage(std::ostream& os) const
{
   os << "  GCC/CLANG front-end compiler options:\n\n"
      << "    --compiler=<compiler_version>\n"
      << "        Specify which compiler is used.\n"
      << "        Possible values for <compiler_version> are:\n"
#if HAVE_ARM_COMPILER
      << "            ARM\n"
#endif
#if HAVE_SPARC_COMPILER
      << "            SPARC\n"
#endif
#if HAVE_I386_GCC45_COMPILER
      << "            I386_GCC45\n"
#endif
#if HAVE_I386_GCC46_COMPILER
      << "            I386_GCC46\n"
#endif
#if HAVE_I386_GCC47_COMPILER
      << "            I386_GCC47\n"
#endif
#if HAVE_I386_GCC48_COMPILER
      << "            I386_GCC48\n"
#endif
#if HAVE_I386_GCC49_COMPILER
      << "            I386_GCC49\n"
#endif
#if HAVE_I386_GCC5_COMPILER
      << "            I386_GCC5\n"
#endif
#if HAVE_I386_GCC6_COMPILER
      << "            I386_GCC6\n"
#endif
#if HAVE_I386_GCC7_COMPILER
      << "            I386_GCC7\n"
#endif
#if HAVE_I386_GCC8_COMPILER
      << "            I386_GCC8\n"
#endif
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
#if HAVE_I386_CLANGVVD_COMPILER
      << "            I386_CLANGVVD\n"
#endif
      << "\n"
      << "    -O<level>\n"
      << "        Enable a specific optimization level. Possible values are the usual\n"
      << "        optimization flags accepted by compilers, plus some others:\n"
      << "        -O0,-O1,-O2,-O3,-Os,-O4,-O5.\n\n"
      << "    -f<option>\n"
      << "        Enable or disable a GCC/CLANG optimization option. All the -f or -fno options\n"
      << "        are supported. In particular, -ftree-vectorize option triggers the\n"
      << "        high-level synthesis of vectorized operations.\n\n"
      << "    -I<path>\n"
      << "        Specify a path where headers are searched for.\n\n"
      << "    -W<warning>\n"
      << "        Specify a warning option passed to GCC/CLANG. All the -W options available in\n"
      << "        GCC/CLANG are supported.\n\n"
      << "    -E\n"
      << "        Enable preprocessing mode of GCC/CLANG.\n\n"
      << "    --std=<standard>\n"
      << "        Assume that the input sources are for <standard>. All\n"
      << "        the --std options available in GCC/CLANG are supported.\n\n"
      << "    -D<name>\n"
      << "        Predefine name as a macro, with definition 1.\n\n"
      << "    -D<name=definition>\n"
      << "        Tokenize <definition> and process as if it appeared as a #define directive.\n\n"
      << "    -U<name>\n"
      << "        Remove existing definition for macro <name>.\n\n"
      << "    --param <name>=<value>\n"
      << "        Set the amount <value> for the GCC/CLANG parameter <name> that could be used for\n"
      << "        some optimizations.\n\n"
      << "    -l<library>\n"
      << "        Search the library named <library> when linking.\n\n"
      << "    -L<dir>\n"
      << "        Add directory <dir> to the list of directories to be searched for -l.\n\n"
      << "    --use-raw\n"
      << "        Specify that input file is already a raw file and not a source file.\n\n"
      << "    -m<machine-option>\n"
      << "        Specify machine dependend options (currently not used).\n\n"
#if !RELEASE
      << "    --read-GCC-XML=<xml_file_name>\n"
      << "        Read GCC options from a XML file.\n\n"
      << "    --write-GCC-XML=<xml_file_name>\n"
      << "        Dump the parsed GCC/CLANG compiler options into a XML file.\n\n"
#endif
      << "    --Include-sysdir\n"
      << "        Return the system include directory used by the wrapped GCC/CLANG compiler.\n\n"
      << "    --gcc-config\n"
      << "        Return the GCC/CLANG configuration.\n\n"
#if !RELEASE
      << "    --compute-sizeof\n"
      << "        Replace sizeof with the computed valued for the considered target\n"
      << "        architecture.\n\n"
#endif
      << "    --extra-gcc-options\n"
      << "        Specify custom extra options to the compiler.\n\n"
      << std::endl;
}
#endif

template <>
const CustomSet<std::string> Parameter::getOption(const enum enum_option name) const
{
   CustomSet<std::string> ret;
   const auto to_be_splitted = getOption<std::string>(name);
   std::vector<std::string> splitted = SplitString(to_be_splitted, STR_CST_string_separator);
   size_t i_end = splitted.size();
   for(size_t i = 0; i < i_end; i++)
   {
      ret.insert(splitted[i]);
   }
   return ret;
}

template <>
const std::list<std::string> Parameter::getOption(const enum enum_option name) const
{
   std::list<std::string> ret;
   const auto to_be_splitted = getOption<std::string>(name);
   std::vector<std::string> splitted = SplitString(to_be_splitted, STR_CST_string_separator);
   size_t i_end = splitted.size();
   for(size_t i = 0; i < i_end; i++)
   {
      ret.push_back(splitted[i]);
   }
   return ret;
}

const std::vector<std::string> Parameter::CGetArgv() const
{
   std::vector<std::string> ret;
   for(int arg = 0; arg < argc; arg++)
   {
      ret.push_back(std::string(argv[arg]));
   }
   return ret;
}

#if HAVE_HOST_PROFILING_BUILT
template <>
HostProfiling_Method Parameter::getOption(const enum enum_option name) const
{
   return static_cast<HostProfiling_Method>(getOption<int>(name));
}
#endif

#if HAVE_TARGET_PROFILING
template <>
InstrumentWriter_Level Parameter::getOption(const enum enum_option name) const
{
   return static_cast<InstrumentWriter_Level>(getOption<int>(name));
}

template <>
TargetArchitecture_Kind Parameter::getOption(const enum enum_option name) const
{
   return static_cast<TargetArchitecture_Kind>(getOption<int>(name));
}
#endif

#if HAVE_FROM_C_BUILT
template <>
CompilerWrapper_CompilerTarget Parameter::getOption(const enum enum_option name) const
{
   return static_cast<CompilerWrapper_CompilerTarget>(getOption<int>(name));
}
#endif

template <>
Parameters_FileFormat Parameter::getOption(const enum enum_option name) const
{
   return static_cast<Parameters_FileFormat>(getOption<int>(name));
}

#if HAVE_CODE_ESTIMATION_BUILT
template <>
CustomUnorderedSet<ActorGraphEstimator_Algorithm> Parameter::getOption(const enum enum_option name) const
{
   CustomUnorderedSet<ActorGraphEstimator_Algorithm> return_value;
   const std::string temp = getOption<std::string>(name);

   std::vector<std::string> splitted = SplitString(temp, ",");
   size_t i_end = splitted.size();
   for(size_t i = 0; i < i_end; i++)
   {
      if(splitted[i] == "")
         continue;
      if(splitted[i] == STR_CST_path_based)
         return_value.insert(ActorGraphEstimator_Algorithm::PE_PATH_BASED);
      else if(splitted[i] == STR_CST_worst_case)
         return_value.insert(ActorGraphEstimator_Algorithm::PE_WORST_CASE);
      else if(splitted[i] == STR_CST_average_case)
         return_value.insert(ActorGraphEstimator_Algorithm::PE_AVERAGE_CASE);
      else
         THROW_ERROR("Unrecognized performance estimation algorithm: " + splitted[i]);
   }
   return return_value;
}

template <>
ActorGraphEstimator_Algorithm Parameter::getOption(const enum enum_option name) const
{
   return static_cast<ActorGraphEstimator_Algorithm>(getOption<int>(name));
}
#endif

#if HAVE_DIOPSIS
template <>
DiopsisInstrumentWriter_Type Parameter::getOption(const enum enum_option name) const
{
   return static_cast<DiopsisInstrumentWriter_Type>(getOption<int>(name));
}
#endif

#if HAVE_DESIGN_ANALYSIS_BUILT
template <>
DesignAnalysis_Step Parameter::getOption(const enum enum_option name) const
{
   return static_cast<DesignAnalysis_Step>(getOption<int>(name));
}
#endif

#if HAVE_FROM_C_BUILT
template <>
CompilerWrapper_OptimizationSet Parameter::getOption(const enum enum_option name) const
{
   return static_cast<CompilerWrapper_OptimizationSet>(getOption<int>(name));
}
template <>
void Parameter::setOption(const enum enum_option name, const CompilerWrapper_OptimizationSet value)
{
   enum_options[name] = boost::lexical_cast<std::string>(static_cast<int>(value));
}
#endif

#if HAVE_TO_C_BUILT
template <>
ActorGraphBackend_Type Parameter::getOption(const enum enum_option name) const
{
   return static_cast<ActorGraphBackend_Type>(getOption<int>(name));
}
#endif
#if HAVE_HLS_BUILT
template <>
HLSFlowStep_Type Parameter::getOption(const enum enum_option name) const
{
   return static_cast<HLSFlowStep_Type>(getOption<int>(name));
}

template <>
void Parameter::setOption(const enum enum_option name, const HLSFlowStep_Type hls_flow_step_type)
{
   enum_options[name] = boost::lexical_cast<std::string>(static_cast<int>(hls_flow_step_type));
}

template <>
MemoryAllocation_Policy Parameter::getOption(const enum enum_option name) const
{
   return static_cast<MemoryAllocation_Policy>(getOption<int>(name));
}

template <>
void Parameter::setOption(const enum enum_option name, const MemoryAllocation_Policy memory_allocation_policy)
{
   enum_options[name] = boost::lexical_cast<std::string>(static_cast<int>(memory_allocation_policy));
}

template <>
MemoryAllocation_ChannelsType Parameter::getOption(const enum enum_option name) const
{
   return static_cast<MemoryAllocation_ChannelsType>(getOption<int>(name));
}

template <>
void Parameter::setOption(const enum enum_option name,
                          const MemoryAllocation_ChannelsType memory_allocation_channels_type)
{
   enum_options[name] = boost::lexical_cast<std::string>(static_cast<int>(memory_allocation_channels_type));
}

template <>
CliqueCovering_Algorithm Parameter::getOption(const enum enum_option name) const
{
   return static_cast<CliqueCovering_Algorithm>(getOption<int>(name));
}

template <>
void Parameter::setOption(const enum enum_option name, const CliqueCovering_Algorithm clique_covering_algorithm)
{
   enum_options[name] = boost::lexical_cast<std::string>(static_cast<int>(clique_covering_algorithm));
}

template <>
Evaluation_Mode Parameter::getOption(const enum enum_option name) const
{
   return static_cast<Evaluation_Mode>(getOption<int>(name));
}

template <>
void Parameter::setOption(const enum enum_option name, const Evaluation_Mode evaluation_mode)
{
   enum_options[name] = boost::lexical_cast<std::string>(static_cast<int>(evaluation_mode));
}

template <>
ParametricListBased_Metric Parameter::getOption(const enum enum_option name) const
{
   return static_cast<ParametricListBased_Metric>(getOption<int>(name));
}

template <>
void Parameter::setOption(const enum enum_option name, const ParametricListBased_Metric parametric_list_based_metric)
{
   enum_options[name] = boost::lexical_cast<std::string>(static_cast<int>(parametric_list_based_metric));
}

template <>
SDCScheduling_Algorithm Parameter::getOption(const enum enum_option name) const
{
   return static_cast<SDCScheduling_Algorithm>(getOption<int>(name));
}

template <>
void Parameter::setOption(const enum enum_option name, const SDCScheduling_Algorithm sdc_scheduling_algorithm)
{
   enum_options[name] = boost::lexical_cast<std::string>(static_cast<int>(sdc_scheduling_algorithm));
}

#endif
bool Parameter::IsParameter(const std::string& name) const
{
   return panda_parameters.find(name) != panda_parameters.end();
}
