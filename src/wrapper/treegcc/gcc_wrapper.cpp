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
 * @file gcc_wrapper.hpp
 * @brief Implementation of the wrapper to Gcc for C sources.
 *
 * Implementation of the methods for the object for invoke the GCC compiler from sources and create the dump.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_ARM_CPP_EXE.hpp"
#include "config_ARM_EMPTY_PLUGIN.hpp"
#include "config_ARM_GCC_EXE.hpp"
#include "config_ARM_RTL_PLUGIN.hpp"
#include "config_ARM_SSA_PLUGIN.hpp"
#include "config_ARM_SSA_PLUGINCPP.hpp"
#include "config_CLANG_PLUGIN_DIR.hpp"
#include "config_EXTRA_CLANGPP_COMPILER_OPTION.hpp"
#include "config_GCC_PLUGIN_DIR.hpp"
#include "config_HAVE_ARM_COMPILER.hpp"
#include "config_HAVE_FROM_RTL_BUILT.hpp"
#include "config_HAVE_I386_CLANG10_COMPILER.hpp"
#include "config_HAVE_I386_CLANG10_MX32.hpp"
#include "config_HAVE_I386_CLANG11_COMPILER.hpp"
#include "config_HAVE_I386_CLANG11_MX32.hpp"
#include "config_HAVE_I386_CLANG4_COMPILER.hpp"
#include "config_HAVE_I386_CLANG4_MX32.hpp"
#include "config_HAVE_I386_CLANG5_COMPILER.hpp"
#include "config_HAVE_I386_CLANG5_MX32.hpp"
#include "config_HAVE_I386_CLANG6_COMPILER.hpp"
#include "config_HAVE_I386_CLANG6_MX32.hpp"
#include "config_HAVE_I386_CLANG7_COMPILER.hpp"
#include "config_HAVE_I386_CLANG7_MX32.hpp"
#include "config_HAVE_I386_CLANG8_COMPILER.hpp"
#include "config_HAVE_I386_CLANG8_MX32.hpp"
#include "config_HAVE_I386_CLANG9_COMPILER.hpp"
#include "config_HAVE_I386_CLANG9_MX32.hpp"
#include "config_HAVE_I386_GCC45_COMPILER.hpp"
#include "config_HAVE_I386_GCC46_COMPILER.hpp"
#include "config_HAVE_I386_GCC47_COMPILER.hpp"
#include "config_HAVE_I386_GCC47_MX32.hpp"
#include "config_HAVE_I386_GCC48_COMPILER.hpp"
#include "config_HAVE_I386_GCC48_MX32.hpp"
#include "config_HAVE_I386_GCC49_COMPILER.hpp"
#include "config_HAVE_I386_GCC49_MX32.hpp"
#include "config_HAVE_I386_GCC5_COMPILER.hpp"
#include "config_HAVE_I386_GCC5_MX32.hpp"
#include "config_HAVE_I386_GCC6_COMPILER.hpp"
#include "config_HAVE_I386_GCC6_MX32.hpp"
#include "config_HAVE_I386_GCC7_COMPILER.hpp"
#include "config_HAVE_I386_GCC7_MX32.hpp"
#include "config_HAVE_I386_GCC8_COMPILER.hpp"
#include "config_HAVE_I386_GCC8_MX32.hpp"
#include "config_HAVE_SPARC_COMPILER.hpp"
#include "config_HAVE_SPARC_ELF_GCC.hpp"
#include "config_I386_CLANG10_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG10_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG10_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG10_EXE.hpp"
#include "config_I386_CLANG10_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG10_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG10_SSA_PLUGIN.hpp"
#include "config_I386_CLANG10_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG10_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG11_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG11_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG11_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG11_EXE.hpp"
#include "config_I386_CLANG11_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG11_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG11_SSA_PLUGIN.hpp"
#include "config_I386_CLANG11_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG11_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG4_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG4_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG4_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG4_EXE.hpp"
#include "config_I386_CLANG4_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG4_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG4_SSA_PLUGIN.hpp"
#include "config_I386_CLANG4_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG4_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG5_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG5_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG5_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG5_EXE.hpp"
#include "config_I386_CLANG5_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG5_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG5_SSA_PLUGIN.hpp"
#include "config_I386_CLANG5_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG5_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG6_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG6_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG6_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG6_EXE.hpp"
#include "config_I386_CLANG6_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG6_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG6_SSA_PLUGIN.hpp"
#include "config_I386_CLANG6_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG6_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG7_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG7_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG7_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG7_EXE.hpp"
#include "config_I386_CLANG7_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG7_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG7_SSA_PLUGIN.hpp"
#include "config_I386_CLANG7_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG7_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG8_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG8_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG8_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG8_EXE.hpp"
#include "config_I386_CLANG8_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG8_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG8_SSA_PLUGIN.hpp"
#include "config_I386_CLANG8_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG8_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG9_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG9_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG9_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG9_EXE.hpp"
#include "config_I386_CLANG9_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG9_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG9_SSA_PLUGIN.hpp"
#include "config_I386_CLANG9_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG9_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANGPP10_EXE.hpp"
#include "config_I386_CLANGPP11_EXE.hpp"
#include "config_I386_CLANGPP4_EXE.hpp"
#include "config_I386_CLANGPP5_EXE.hpp"
#include "config_I386_CLANGPP6_EXE.hpp"
#include "config_I386_CLANGPP7_EXE.hpp"
#include "config_I386_CLANGPP8_EXE.hpp"
#include "config_I386_CLANGPP9_EXE.hpp"
#include "config_I386_CLANG_CPP10_EXE.hpp"
#include "config_I386_CLANG_CPP11_EXE.hpp"
#include "config_I386_CLANG_CPP4_EXE.hpp"
#include "config_I386_CLANG_CPP5_EXE.hpp"
#include "config_I386_CLANG_CPP6_EXE.hpp"
#include "config_I386_CLANG_CPP7_EXE.hpp"
#include "config_I386_CLANG_CPP8_EXE.hpp"
#include "config_I386_CLANG_CPP9_EXE.hpp"
#include "config_I386_CPP45_EXE.hpp"
#include "config_I386_CPP46_EXE.hpp"
#include "config_I386_CPP47_EXE.hpp"
#include "config_I386_CPP48_EXE.hpp"
#include "config_I386_CPP49_EXE.hpp"
#include "config_I386_CPP5_EXE.hpp"
#include "config_I386_CPP6_EXE.hpp"
#include "config_I386_CPP7_EXE.hpp"
#include "config_I386_CPP8_EXE.hpp"
#include "config_I386_GCC45_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC45_EXE.hpp"
#include "config_I386_GCC45_SSAVRP_PLUGIN.hpp"
#include "config_I386_GCC45_SSA_PLUGIN.hpp"
#include "config_I386_GCC45_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC45_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC46_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC46_EXE.hpp"
#include "config_I386_GCC46_SSAVRP_PLUGIN.hpp"
#include "config_I386_GCC46_SSA_PLUGIN.hpp"
#include "config_I386_GCC46_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC46_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC47_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC47_EXE.hpp"
#include "config_I386_GCC47_SSAVRP_PLUGIN.hpp"
#include "config_I386_GCC47_SSA_PLUGIN.hpp"
#include "config_I386_GCC47_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC47_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC48_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC48_EXE.hpp"
#include "config_I386_GCC48_SSAVRP_PLUGIN.hpp"
#include "config_I386_GCC48_SSA_PLUGIN.hpp"
#include "config_I386_GCC48_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC48_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC49_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC49_EXE.hpp"
#include "config_I386_GCC49_SSA_PLUGIN.hpp"
#include "config_I386_GCC49_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC49_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC5_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC5_EXE.hpp"
#include "config_I386_GCC5_SSA_PLUGIN.hpp"
#include "config_I386_GCC5_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC5_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC6_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC6_EXE.hpp"
#include "config_I386_GCC6_SSA_PLUGIN.hpp"
#include "config_I386_GCC6_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC6_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC7_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC7_EXE.hpp"
#include "config_I386_GCC7_SSA_PLUGIN.hpp"
#include "config_I386_GCC7_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC7_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC8_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC8_EXE.hpp"
#include "config_I386_GCC8_SSA_PLUGIN.hpp"
#include "config_I386_GCC8_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC8_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GPP45_EXE.hpp"
#include "config_I386_GPP46_EXE.hpp"
#include "config_I386_GPP47_EXE.hpp"
#include "config_I386_GPP48_EXE.hpp"
#include "config_I386_GPP49_EXE.hpp"
#include "config_I386_GPP5_EXE.hpp"
#include "config_I386_GPP6_EXE.hpp"
#include "config_I386_GPP7_EXE.hpp"
#include "config_I386_GPP8_EXE.hpp"
#include "config_I386_LLVM10_LINK_EXE.hpp"
#include "config_I386_LLVM10_OPT_EXE.hpp"
#include "config_I386_LLVM11_LINK_EXE.hpp"
#include "config_I386_LLVM11_OPT_EXE.hpp"
#include "config_I386_LLVM4_LINK_EXE.hpp"
#include "config_I386_LLVM4_OPT_EXE.hpp"
#include "config_I386_LLVM5_LINK_EXE.hpp"
#include "config_I386_LLVM5_OPT_EXE.hpp"
#include "config_I386_LLVM6_LINK_EXE.hpp"
#include "config_I386_LLVM6_OPT_EXE.hpp"
#include "config_I386_LLVM7_LINK_EXE.hpp"
#include "config_I386_LLVM7_OPT_EXE.hpp"
#include "config_I386_LLVM8_LINK_EXE.hpp"
#include "config_I386_LLVM8_OPT_EXE.hpp"
#include "config_I386_LLVM9_LINK_EXE.hpp"
#include "config_I386_LLVM9_OPT_EXE.hpp"
#include "config_NPROFILE.hpp"
#include "config_SPARC_CPP_EXE.hpp"
#include "config_SPARC_ELF_CPP.hpp"
#include "config_SPARC_ELF_GCC.hpp"
#include "config_SPARC_EMPTY_PLUGIN.hpp"
#include "config_SPARC_GCC_EXE.hpp"
#include "config_SPARC_RTL_PLUGIN.hpp"
#include "config_SPARC_SSA_PLUGIN.hpp"
#include "config_SPARC_SSA_PLUGINCPP.hpp"
/// Header include
#include "gcc_wrapper.hpp"

/// Behavior include
#include "application_manager.hpp"

/// Constants include
#include "file_IO_constants.hpp"
#include "treegcc_constants.hpp"
#include "treegcc_xml.hpp"

/// Frontend include
#include "Parameter.hpp"

/// HLS include
#include "hls_step.hpp"

/// RTL include
#if HAVE_FROM_RTL_BUILT
#include "parse_rtl.hpp"
#endif
/// STD include
#include <cerrno>
#include <string>
#include <unistd.h>

/// STL include
#include <list>

/// Tree includes
#include "parse_tree.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "cpu_stats.hpp"
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"
#include <boost/regex.hpp>

/// XML includes used for writing and reading the configuration file
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

std::string GccWrapper::current_gcc_version;

std::string GccWrapper::current_plugin_version;

GccWrapper::GccWrapper(const ParameterConstRef _Param, const GccWrapper_CompilerTarget _compiler_target, const GccWrapper_OptimizationSet _OS)
    : Param(_Param), compiler_target(_compiler_target), OS(_OS), output_level(_Param->getOption<int>(OPT_output_level)), debug_level(_Param->get_class_debug_level("GccWrapper"))
{
   InitializeGccParameters();
   if(Param->isOption(OPT_gcc_write_xml))
   {
      WriteXml(Param->getOption<std::string>(OPT_gcc_write_xml));
   }
}

// destructor
GccWrapper::~GccWrapper() = default;

void GccWrapper::CompileFile(const std::string& original_file_name, std::string& real_file_name, const std::string& parameters_line, bool multiple_files, GccWrapper_CompilerMode cm)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Compiling " + original_file_name + "(transformed in " + real_file_name);

   /// The gcc output
   const std::string gcc_output_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_gcc_output;

   const Compiler compiler = GetCompiler();
   std::string command = compiler.gcc.string();
   if(cm == GccWrapper_CompilerMode::CM_ANALYZER && !compiler.is_clang)
   {
#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
      bool flag_cpp;
      if(Param->isOption(OPT_input_format) && Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP)
         flag_cpp = true;
      else
         flag_cpp = false;
#endif
#if HAVE_I386_CLANG11_COMPILER
      command = flag_cpp ? relocate_compiler_path(I386_CLANGPP11_EXE) : relocate_compiler_path(I386_CLANG11_EXE);
#elif HAVE_I386_CLANG10_COMPILER
      command = flag_cpp ? relocate_compiler_path(I386_CLANGPP10_EXE) : relocate_compiler_path(I386_CLANG10_EXE);
#elif HAVE_I386_CLANG9_COMPILER
      command = flag_cpp ? relocate_compiler_path(I386_CLANGPP9_EXE) : relocate_compiler_path(I386_CLANG9_EXE);
#elif HAVE_I386_CLANG8_COMPILER
      command = flag_cpp ? relocate_compiler_path(I386_CLANGPP8_EXE) : relocate_compiler_path(I386_CLANG8_EXE);
#elif HAVE_I386_CLANG7_COMPILER
      command = flag_cpp ? relocate_compiler_path(I386_CLANGPP7_EXE) : relocate_compiler_path(I386_CLANG7_EXE);
#elif HAVE_I386_CLANG6_COMPILER
      command = flag_cpp ? relocate_compiler_path(I386_CLANGPP6_EXE) : relocate_compiler_path(I386_CLANG6_EXE);
#elif HAVE_I386_CLANG5_COMPILER
      command = flag_cpp ? relocate_compiler_path(I386_CLANGPP5_EXE) : relocate_compiler_path(I386_CLANG5_EXE);
#elif HAVE_I386_CLANG4_COMPILER
      command = flag_cpp ? relocate_compiler_path(I386_CLANGPP4_EXE) : relocate_compiler_path(I386_CLANG4_EXE);
#else
      THROW_ERROR("unexpected condition");
#endif
   }
   command += " -D__NO_INLINE__ "; /// needed to avoid problem with glibc inlines
#ifdef _WIN32
   if(compiler.is_clang)
      command += " -isystem /mingw64/include -isystem /mingw64/x86_64-w64-mingw32/include -isystem /mingw64/include/c++/v1/"; /// needed by clang compiler
#endif
   if(((Param->isOption(OPT_discrepancy) and Param->getOption<bool>(OPT_discrepancy)) || (Param->isOption(OPT_discrepancy_hw) and Param->getOption<bool>(OPT_discrepancy_hw))) and
      (cm == GccWrapper_CompilerMode::CM_STD || cm == GccWrapper_CompilerMode::CM_EMPTY))
   {
      command += " -D__BAMBU_DISCREPANCY__ ";
   }

   /// Adding source code includes
   if(original_file_name != "-" and original_file_name != real_file_name)
   {
      std::list<std::string> source_files;
      source_files.push_back(original_file_name);
      command += AddSourceCodeIncludes(source_files) + " ";
   }
   command += " " + compiler.extra_options + " ";

   bool isWholeProgram =
       Param->isOption(OPT_gcc_optimizations) && Param->getOption<std::string>(OPT_gcc_optimizations).find("whole-program") != std::string::npos && Param->getOption<std::string>(OPT_gcc_optimizations).find("no-whole-program") == std::string::npos;

   if(cm == GccWrapper_CompilerMode::CM_EMPTY)
   {
      if(original_file_name == "-")
         THROW_ERROR("Reading from standard input which does not contain any function definition");
      static int empty_counter = 0;
      const std::string temp_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + "/empty_" + boost::lexical_cast<std::string>(empty_counter++) + ".c";
      CopyFile(original_file_name, temp_file_name);
      const std::string append_command = "`echo -e \"\\nvoid __empty_function__(){}\" >> " + temp_file_name + "`";
      int ret = PandaSystem(Param, append_command);
      if(IsError(ret))
      {
         THROW_ERROR("Error in appending empty function");
      }
      real_file_name = temp_file_name;
      if(compiler.is_clang)
         command += " -c -fplugin=" + compiler.empty_plugin_obj + " -mllvm -pandaGE-outputdir=" + Param->getOption<std::string>(OPT_output_temporary_directory) + " -mllvm -pandaGE-infile=" + real_file_name;
      else
         command += " -c -fplugin=" + compiler.empty_plugin_obj + " -fplugin-arg-" + compiler.empty_plugin_name + "-outputdir=" + Param->getOption<std::string>(OPT_output_temporary_directory);
   }
   else if(cm == GccWrapper_CompilerMode::CM_ANALYZER)
   {
      /// remove some extra option not compatible with clang
      boost::replace_all(command, "-mlong-double-64", "");
      std::string fname;
      bool addTopFName = false;
      if(isWholeProgram && compiler.is_clang)
      {
         fname = "main";
         addTopFName = true;
      }
      else if(Param->isOption(OPT_top_functions_names))
      {
         const auto top_functions_names = Param->getOption<const std::list<std::string>>(OPT_top_functions_names);
         addTopFName = top_functions_names.size() == 1;
         fname = top_functions_names.front();
      }
      command += " -c -fplugin=" + compiler.ASTAnalyzer_plugin_obj;
      command += " -Xclang -add-plugin -Xclang " + compiler.ASTAnalyzer_plugin_name + " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang -outputdir -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang " +
                 Param->getOption<std::string>(OPT_output_temporary_directory);

      if(addTopFName)
         command += " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang -topfname -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang " + fname;
   }
   else if((Param->isOption(OPT_gcc_E) and Param->getOption<bool>(OPT_gcc_E)) or (Param->isOption(OPT_gcc_S) and Param->getOption<bool>(OPT_gcc_S)))
      ;
#if HAVE_FROM_RTL_BUILT
   else if(Param->getOption<bool>(OPT_use_rtl))
   {
      command += " -c -fplugin=" + compiler.rtl_plugin;
   }
#endif
   else if(cm == GccWrapper_CompilerMode::CM_STD)
   {
      std::string fname;
      bool addTopFName = false;
      if(isWholeProgram)
      {
         fname = "main";
         addTopFName = compiler.is_clang;
      }
      else if(Param->isOption(OPT_top_functions_names))
      {
         const auto top_functions_names = Param->getOption<const std::list<std::string>>(OPT_top_functions_names);
         addTopFName = top_functions_names.size() == 1 && !Param->isOption(OPT_top_design_name);
         fname = top_functions_names.front();
      }
      if(addTopFName)
      {
         if(compiler.is_clang)
         {
            command += " -fplugin=" + compiler.topfname_plugin_obj + " -mllvm -internalize-outputdir=" + Param->getOption<std::string>(OPT_output_temporary_directory) + " -mllvm -panda-TFN=" + fname;
            std::string extern_symbols;
            std::vector<std::string> xml_files;
            if(Param->isOption(OPT_xml_memory_allocation))
            {
               xml_files.push_back(Param->getOption<std::string>(OPT_xml_memory_allocation));
            }
            else
            {
               /// load xml memory allocation file
               auto source_file = real_file_name;
               const std::string output_temporary_directory = Param->getOption<std::string>(OPT_output_temporary_directory);
               std::string leaf_name = GetLeafFileName(source_file);
               auto XMLfilename = output_temporary_directory + "/" + leaf_name + ".memory_allocation.xml";
               if((boost::filesystem::exists(boost::filesystem::path(XMLfilename))))
               {
                  xml_files.push_back(XMLfilename);
               }
            }
            for(auto XMLfilename : xml_files)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->parsing " + XMLfilename);
               XMLDomParser parser(XMLfilename);
               parser.Exec();
               if(parser)
               {
                  const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
                  const xml_node::node_list list = node->get_children();
                  for(const auto& l : list)
                  {
                     const xml_element* child = GetPointer<xml_element>(l);
                     if(!child)
                        continue;
                     if(child->get_name() == "memory_allocation")
                     {
                        for(const auto& it : child->get_children())
                        {
                           const xml_element* mem_node = GetPointer<xml_element>(it);
                           if(!mem_node)
                              continue;
                           if(mem_node->get_name() == "object")
                           {
                              std::string is_internal;
                              if(!CE_XVM(is_internal, mem_node))
                                 THROW_ERROR("expected the is_internal attribute");
                              LOAD_XVM(is_internal, mem_node);
                              if(is_internal == "T")
                              {
                              }
                              else if(is_internal == "F")
                              {
                                 if(!CE_XVM(name, mem_node))
                                    THROW_ERROR("expected the name attribute");
                                 std::string name;
                                 LOAD_XVM(name, mem_node);
                                 extern_symbols = extern_symbols + name + ",";
                              }
                              else
                                 THROW_ERROR("unexpected value for is_internal attribute");
                           }
                        }
                     }
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
            }
            if(!extern_symbols.empty())
            {
               command += " -mllvm -panda-ESL=" + extern_symbols;
            }
            if(isWholeProgram || Param->getOption<bool>(OPT_do_not_expose_globals))
               command += " -mllvm -panda-Internalize";
            if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1 && !compiler.CSROA_plugin_obj.empty() && !compiler.expandMemOps_plugin_obj.empty())
            {
               command += " -fplugin=" + compiler.CSROA_plugin_obj + " -mllvm -panda-KN=" + fname;
               if(Param->IsParameter("max-CSROA"))
               {
                  auto max_CSROA = Param->GetParameter<int>("max-CSROA");
                  command += " -mllvm -csroa-max-transformations=" + STR(max_CSROA);
               }
            }
         }
         else if(!multiple_files) /// LTO not yet supported with GCC
            command += " -fplugin=" + compiler.topfname_plugin_obj + " -fplugin-arg-" + compiler.topfname_plugin_name + "-topfname=" + fname;
      }
      if(compiler.is_clang)
      {
         command += " -c -fplugin=" + compiler.ssa_plugin_obj + " -mllvm -panda-outputdir=" + Param->getOption<std::string>(OPT_output_temporary_directory) + " -mllvm -panda-infile=" + real_file_name;
         if(addTopFName)
         {
            command += " -mllvm -panda-topfname=" + fname;
         }
      }
      else
         command += " -c -fplugin=" + compiler.ssa_plugin_obj + " -fplugin-arg-" + compiler.ssa_plugin_name + "-outputdir=" + Param->getOption<std::string>(OPT_output_temporary_directory);
   }
   else if(cm == GccWrapper_CompilerMode::CM_LTO)
      command += " -c -flto -o " + Param->getOption<std::string>(OPT_output_temporary_directory) + "/" + GetBaseName(real_file_name) + ".o ";
   else
      THROW_ERROR("compilation mode not yet implemented");

   std::string temporary_file_run_o;
   if(cm != GccWrapper_CompilerMode::CM_LTO)
   {
      if((Param->isOption(OPT_gcc_E) and Param->getOption<bool>(OPT_gcc_E)) or (Param->isOption(OPT_gcc_S) and Param->getOption<bool>(OPT_gcc_S)))
      {
         if(Param->isOption(OPT_output_file))
         {
            temporary_file_run_o = Param->getOption<std::string>(OPT_output_file);
            command += " -o " + temporary_file_run_o;
         }
      }
      else
      {
         temporary_file_run_o = boost::filesystem::unique_path(GetCurrentPath() + std::string(STR_CST_gcc_obj_file)).string();
         command += " -o " + temporary_file_run_o;
      }
   }

   /// manage optimization level
   auto local_parameters_line = parameters_line;
   if(cm == GccWrapper_CompilerMode::CM_LTO)
   {
      boost::replace_all(local_parameters_line, "-O4", "");
      boost::replace_all(local_parameters_line, "-O3", "");
      boost::replace_all(local_parameters_line, "-O2", "");
      boost::replace_all(local_parameters_line, "-O1", "");
      local_parameters_line += " -O1 ";
   }

   if(!(Param->getOption<bool>(OPT_compute_size_of)))
      command += " -D\"" + std::string(STR_CST_panda_sizeof) + "(arg)=" + STR_CST_string_sizeof + "(#arg)\"";
   command += " " + local_parameters_line;
   if(original_file_name == "-" or original_file_name == "/dev/null")
   {
      command += real_file_name;
   }
   else
   {
      boost::filesystem::path file_path(original_file_name);
      std::string extension = GetExtension(file_path);
      /// assembler files are not allowed so in some cases we pass a C file renamed with extension .S
      if(extension == "S")
         command += "-x c ";
      command += "\"" + real_file_name + "\"";
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Invoke: " + command);
#if !NPROFILE
   long int gcc_compilation_time = 0;
   if(output_level >= OUTPUT_LEVEL_VERBOSE)
   {
      START_TIME(gcc_compilation_time);
   }
#endif
   int ret = PandaSystem(Param, command, gcc_output_file_name);
#if !NPROFILE
   if(output_level >= OUTPUT_LEVEL_VERBOSE)
   {
      STOP_TIME(gcc_compilation_time);
      dump_exec_time("Compilation time", gcc_compilation_time);
   }
#endif

   if(!((Param->isOption(OPT_gcc_E) and Param->getOption<bool>(OPT_gcc_E)) or (Param->isOption(OPT_gcc_S) and Param->getOption<bool>(OPT_gcc_S)) or cm == GccWrapper_CompilerMode::CM_LTO))
      std::remove(temporary_file_run_o.c_str());
   if(IsError(ret))
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0, "Error in compilation");
      if(boost::filesystem::exists(boost::filesystem::path(gcc_output_file_name)))
      {
         CopyStdout(gcc_output_file_name);
         THROW_ERROR_CODE(COMPILING_EC, "Front-end compiler returns an error during compilation " + boost::lexical_cast<std::string>(errno));
         THROW_ERROR("Front-end compiler returns an error during compilation " + boost::lexical_cast<std::string>(errno));
      }
      else
      {
         THROW_ERROR("Error in front-end compiler invocation");
      }
   }
   else
   {
      if(output_level >= OUTPUT_LEVEL_VERBOSE)
         CopyStdout(gcc_output_file_name);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Compiled file");
}

void GccWrapper::FillTreeManager(const tree_managerRef TM, std::map<std::string, std::string>& source_files)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Invoking front-end compiler");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   const std::string output_temporary_directory = Param->getOption<std::string>(OPT_output_temporary_directory);
   if(source_files.size() == 0)
      THROW_ERROR("No files specified for parsing");

   /// check for aligned option
   const GccWrapper_OptimizationSet optimization_level = Param->getOption<GccWrapper_OptimizationSet>(OPT_gcc_opt_level);
   if(optimization_level == GccWrapper_OptimizationSet::O3 || optimization_level == GccWrapper_OptimizationSet::O4 || optimization_level == GccWrapper_OptimizationSet::O5)
   {
      if(optimization_flags.find("tree-vectorize") == optimization_flags.end() || optimization_flags.find("tree-vectorize")->second)
      {
         bool assume_aligned_access_p = Param->isOption(OPT_aligned_access) && Param->getOption<bool>(OPT_aligned_access);
         if(assume_aligned_access_p)
            THROW_ERROR("Option --aligned-access cannot be used with -O3 or -ftree-vectorize");
      }
   }
   const Compiler compiler = GetCompiler();

   bool disable_pragma_parsing = false;
   if(Param->IsParameter("disable-pragma-parsing") && Param->GetParameter<int>("disable-pragma-parsing") == 1)
      disable_pragma_parsing = true;
   if(disable_pragma_parsing ||
      !(HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER))
      THROW_WARNING("pragma analysis requires CLANG");
   else
   {
      for(auto& source_file : source_files)
      {
         if(already_processed_files.find(source_file.first) != already_processed_files.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Already processed " + source_file.first);
            continue;
         }
         std::string analyzing_compiling_parameters;
         if(Param->isOption(OPT_gcc_standard))
         {
            std::string standard = Param->getOption<std::string>(OPT_gcc_standard);
            analyzing_compiling_parameters += "--std=" + standard + " ";
         }
         if(Param->isOption(OPT_gcc_defines))
         {
            const auto defines = Param->getOption<const CustomSet<std::string>>(OPT_gcc_defines);
            for(const auto& define : defines)
            {
               std::string escaped_string = define;
               // add_escape(escaped_string, "\"");
               analyzing_compiling_parameters += "-D" + escaped_string + " ";
            }
         }
         if(Param->isOption(OPT_gcc_undefines))
         {
            const auto undefines = Param->getOption<const CustomSet<std::string>>(OPT_gcc_undefines);
            for(const auto& undefine : undefines)
            {
               std::string escaped_string = undefine;
               // add_escape(escaped_string, "\"");
               analyzing_compiling_parameters += "-U" + escaped_string + " ";
            }
         }
         if(Param->isOption(OPT_gcc_includes))
         {
            analyzing_compiling_parameters += Param->getOption<std::string>(OPT_gcc_includes) + " ";
         }
         CompileFile(source_file.first, source_file.second, analyzing_compiling_parameters, source_files.size() > 1, GccWrapper_CompilerMode::CM_ANALYZER);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting compilation of single files");
   bool enable_LTO = (compiler.is_clang && source_files.size() > 1);
   for(auto& source_file : source_files)
   {
      if(already_processed_files.find(source_file.first) != already_processed_files.end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Already processed " + source_file.first);
         continue;
      }
      else
      {
         already_processed_files.insert(source_file.first);
      }
      std::string leaf_name = source_file.second == "-" ? "stdin-" : GetLeafFileName(source_file.second);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Compiling file " + source_file.second);
      /// create obj
      CompileFile(source_file.first, source_file.second, gcc_compiling_parameters, source_files.size() > 1, enable_LTO ? GccWrapper_CompilerMode::CM_LTO : GccWrapper_CompilerMode::CM_STD);
      if(!Param->isOption(OPT_gcc_E) and !Param->isOption(OPT_gcc_S) and !enable_LTO)
      {
         if(!(boost::filesystem::exists(boost::filesystem::path(output_temporary_directory + "/" + leaf_name + STR_CST_gcc_tree_suffix))))
         {
            THROW_WARNING("Raw not created for file " + output_temporary_directory + "/" + leaf_name);
            CompileFile(source_file.first, source_file.second, gcc_compiling_parameters, source_files.size() > 1, GccWrapper_CompilerMode::CM_EMPTY);
            /// Recomputing leaf_name since source_file.second should be modified in the previous call
            leaf_name = source_file.second == "-" ? "stdin-" : GetLeafFileName(source_file.second);
            if(not(boost::filesystem::exists(boost::filesystem::path(output_temporary_directory + "/" + leaf_name + STR_CST_gcc_empty_suffix))))
               THROW_ERROR(output_temporary_directory + "/" + leaf_name + STR_CST_gcc_empty_suffix + " not found: impossible to create raw file for " + source_file.second);
            rename_file(output_temporary_directory + "/" + leaf_name + STR_CST_gcc_empty_suffix, output_temporary_directory + "/" + leaf_name + STR_CST_gcc_tree_suffix);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Renaming " + source_file.second + STR_CST_gcc_empty_suffix + " in " + source_file.second + STR_CST_gcc_tree_suffix);
         }
         boost::filesystem::path obj = boost::filesystem::path(output_temporary_directory + "/" + leaf_name + STR_CST_gcc_tree_suffix);
         tree_managerRef TreeM = ParseTreeFile(Param, obj.string());

#if HAVE_FROM_RTL_BUILT
         if((Param->getOption<bool>(OPT_use_rtl)) and boost::filesystem::exists(boost::filesystem::path(leaf_name + STR_CST_gcc_rtl_suffix)))
         {
            obj = boost::filesystem::path(leaf_name + STR_CST_gcc_rtl_suffix);
            parse_rtl_File(obj.string(), TreeM, debug_level);
            rename_file(obj, boost::filesystem::path(output_temporary_directory + leaf_name + STR_CST_gcc_rtl_suffix));
         }
#endif
#if !NPROFILE
         long int merge_time = 0;
         START_TIME(merge_time);
#endif
         TM->merge_tree_managers(TreeM);
#if !NPROFILE
         STOP_TIME(merge_time);
         if(output_level >= OUTPUT_LEVEL_VERBOSE)
         {
            dump_exec_time("Tree merging time", merge_time);
         }
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
   }
   if(enable_LTO)
   {
      std::string object_files;
      for(auto& source_file : source_files)
      {
         std::string leaf_name = source_file.second == "-" ? "stdin-" : GetBaseName(GetLeafFileName(source_file.second));
         if((boost::filesystem::exists(boost::filesystem::path(output_temporary_directory + "/" + leaf_name + ".o"))))
            object_files += boost::filesystem::path(output_temporary_directory + "/" + leaf_name + ".o").string() + " ";
      }
      auto temporary_file_o_bc = boost::filesystem::path(Param->getOption<std::string>(OPT_output_temporary_directory) + "/" + boost::filesystem::unique_path(std::string(STR_CST_llvm_obj_file)).string()).string();
      std::string command = compiler.llvm_link.string() + " " + object_files + " -o " + temporary_file_o_bc;
      const std::string llvm_link_output_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_gcc_output;
      int ret = PandaSystem(Param, command, llvm_link_output_file_name);
      if(IsError(ret))
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0, "Error in llvm-link");
         if(boost::filesystem::exists(boost::filesystem::path(llvm_link_output_file_name)))
         {
            CopyStdout(llvm_link_output_file_name);
            THROW_ERROR_CODE(COMPILING_EC, "llvm-link returns an error during compilation " + boost::lexical_cast<std::string>(errno));
            THROW_ERROR("llvm-link returns an error during compilation " + boost::lexical_cast<std::string>(errno));
         }
         else
         {
            THROW_ERROR("Error in llvm-link invocation");
         }
      }
      bool isWholeProgram =
          Param->isOption(OPT_gcc_optimizations) && Param->getOption<std::string>(OPT_gcc_optimizations).find("whole-program") != std::string::npos && Param->getOption<std::string>(OPT_gcc_optimizations).find("no-whole-program") == std::string::npos;
      std::string fname;
      bool addTFNPlugin = false;
      if(isWholeProgram && compiler.is_clang)
      {
         fname = "main";
         addTFNPlugin = true;
      }
      else if(Param->isOption(OPT_top_functions_names))
      {
         const auto top_functions_names = Param->getOption<const std::list<std::string>>(OPT_top_functions_names);
         addTFNPlugin = top_functions_names.size() == 1 && !Param->isOption(OPT_top_design_name);
         fname = top_functions_names.front();
         if(fname == "main" && !compiler.is_clang)
            addTFNPlugin = false;
      }

      if(addTFNPlugin)
      {
         if(compiler.is_clang)
         {
            command = compiler.llvm_opt.string();
#ifndef _WIN32
            auto renamed_plugin = compiler.topfname_plugin_obj;
            boost::replace_all(renamed_plugin, ".so", "_opt.so");
            command += " -load=" + renamed_plugin;
#endif
            command += " -internalize-outputdir=" + Param->getOption<std::string>(OPT_output_temporary_directory);
            command += " -panda-TFN=" + fname;
            std::string extern_symbols;
            std::vector<std::string> xml_files;
            if(Param->isOption(OPT_xml_memory_allocation))
            {
               xml_files.push_back(Param->getOption<std::string>(OPT_xml_memory_allocation));
            }
            else
            {
               for(auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(output_temporary_directory), {}))
               {
                  auto source_file = GetLeafFileName(entry.path().string());
                  if(source_file.find(".memory_allocation.xml") != std::string::npos)
                     xml_files.push_back(source_file);
               }
            }
            for(auto XMLfilename : xml_files)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->parsing " + XMLfilename);
               XMLDomParser parser(XMLfilename);
               parser.Exec();
               if(parser)
               {
                  const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
                  const xml_node::node_list list = node->get_children();
                  for(const auto& l : list)
                  {
                     const xml_element* child = GetPointer<xml_element>(l);
                     if(!child)
                        continue;
                     if(child->get_name() == "memory_allocation")
                     {
                        for(const auto& it : child->get_children())
                        {
                           const xml_element* mem_node = GetPointer<xml_element>(it);
                           if(!mem_node)
                              continue;
                           if(mem_node->get_name() == "object")
                           {
                              std::string is_internal;
                              if(!CE_XVM(is_internal, mem_node))
                                 THROW_ERROR("expected the is_internal attribute");
                              LOAD_XVM(is_internal, mem_node);
                              if(is_internal == "T")
                              {
                              }
                              else if(is_internal == "F")
                              {
                                 if(!CE_XVM(name, mem_node))
                                    THROW_ERROR("expected the name attribute");
                                 std::string name;
                                 LOAD_XVM(name, mem_node);
                                 extern_symbols = extern_symbols + name + ",";
                              }
                              else
                                 THROW_ERROR("unexpected value for is_internal attribute");
                           }
                        }
                     }
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
            }
            if(!extern_symbols.empty())
            {
               command += " -panda-ESL=" + extern_symbols;
            }

            if(isWholeProgram || Param->getOption<bool>(OPT_do_not_expose_globals))
               command += " -panda-Internalize";
            command += " -" + compiler.topfname_plugin_name;
            command += " " + temporary_file_o_bc;
            temporary_file_o_bc = boost::filesystem::path(Param->getOption<std::string>(OPT_output_temporary_directory) + "/" + boost::filesystem::unique_path(std::string(STR_CST_llvm_obj_file)).string()).string();
            command += " -o " + temporary_file_o_bc;
            const std::string tfn_output_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_gcc_output;
            ret = PandaSystem(Param, command, tfn_output_file_name);
            if(IsError(ret))
            {
               PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0, "Error in opt");
               if(boost::filesystem::exists(boost::filesystem::path(tfn_output_file_name)))
               {
                  CopyStdout(tfn_output_file_name);
                  THROW_ERROR_CODE(COMPILING_EC, "opt returns an error during compilation " + boost::lexical_cast<std::string>(errno));
                  THROW_ERROR("opt returns an error during compilation " + boost::lexical_cast<std::string>(errno));
               }
               else
               {
                  THROW_ERROR("Error in opt invocation");
               }
            }

            command = compiler.llvm_opt.string();
            command += " " + temporary_file_o_bc;
            command += " --internalize-public-api-file=" + Param->getOption<std::string>(OPT_output_temporary_directory) + "external-symbols.txt -internalize ";
            temporary_file_o_bc = boost::filesystem::path(Param->getOption<std::string>(OPT_output_temporary_directory) + "/" + boost::filesystem::unique_path(std::string(STR_CST_llvm_obj_file)).string()).string();
            command += " -o " + temporary_file_o_bc;
            const std::string int_output_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_gcc_output;
            ret = PandaSystem(Param, command, int_output_file_name);
            if(IsError(ret))
            {
               PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0, "Error in opt");
               if(boost::filesystem::exists(boost::filesystem::path(int_output_file_name)))
               {
                  CopyStdout(int_output_file_name);
                  THROW_ERROR_CODE(COMPILING_EC, "opt returns an error during compilation " + boost::lexical_cast<std::string>(errno));
                  THROW_ERROR("opt returns an error during compilation " + boost::lexical_cast<std::string>(errno));
               }
               else
               {
                  THROW_ERROR("Error in opt invocation");
               }
            }
         }
         else
            THROW_ERROR("LTO compilation not yet implemented for the chosen front-end compiler");
      }
      if(compiler.is_clang)
      {
         const auto recipe = clang_recipes(optimization_level, Param->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler), compiler.expandMemOps_plugin_obj, compiler.expandMemOps_plugin_name, compiler.GepiCanon_plugin_obj,
                                           compiler.GepiCanon_plugin_name, compiler.CSROA_plugin_obj, compiler.CSROA_plugin_name, fname);
         command = compiler.llvm_opt.string() + recipe + temporary_file_o_bc;
         temporary_file_o_bc = boost::filesystem::path(Param->getOption<std::string>(OPT_output_temporary_directory) + "/" + boost::filesystem::unique_path(std::string(STR_CST_llvm_obj_file)).string()).string();
         command += " -o " + temporary_file_o_bc;
         const std::string o2_output_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_gcc_output;
         ret = PandaSystem(Param, command, o2_output_file_name);
         if(IsError(ret))
         {
            PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0, "Error in opt");
            if(boost::filesystem::exists(boost::filesystem::path(o2_output_file_name)))
            {
               CopyStdout(o2_output_file_name);
               THROW_ERROR_CODE(COMPILING_EC, "opt returns an error during compilation " + boost::lexical_cast<std::string>(errno));
               THROW_ERROR("opt returns an error during compilation " + boost::lexical_cast<std::string>(errno));
            }
            else
            {
               THROW_ERROR("Error in opt invocation");
            }
         }
      }
      else
         THROW_ERROR("LTO compilation not yet implemented for the chosen front-end compiler");

      std::string real_file_names;
      bool first_file = true;
      for(const auto& fsname : source_files)
      {
         if(first_file)
         {
            real_file_names = fsname.second;
            first_file = false;
         }
         else
            real_file_names = real_file_names + "," + fsname.second;
      }
      if(compiler.is_clang)
      {
         command = compiler.llvm_opt.string();
#ifndef _WIN32
         auto renamed_plugin = compiler.ssa_plugin_obj;
         boost::replace_all(renamed_plugin, ".so", "_opt.so");
         command += " -load=" + renamed_plugin;
#endif
         command += " -panda-outputdir=" + Param->getOption<std::string>(OPT_output_temporary_directory) + " -panda-infile=" + real_file_names;
         if(addTFNPlugin)
         {
            command += " -panda-topfname=" + fname;
         }
         command += " -domfrontier -domtree -memdep -memoryssa -lazy-value-info -aa -assumption-cache-tracker -targetlibinfo -loops -simplifycfg -mem2reg -globalopt -break-crit-edges -dse -adce -loop-load-elim";
         command += " " + temporary_file_o_bc;
         temporary_file_o_bc = boost::filesystem::path(Param->getOption<std::string>(OPT_output_temporary_directory) + "/" + boost::filesystem::unique_path(std::string(STR_CST_llvm_obj_file)).string()).string();
         command += " -o " + temporary_file_o_bc;
         command += " -" + compiler.ssa_plugin_name;
         const std::string gimpledump_output_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_gcc_output;
         ret = PandaSystem(Param, command, gimpledump_output_file_name);
         if(IsError(ret))
         {
            PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0, "Error in opt");
            if(boost::filesystem::exists(boost::filesystem::path(gimpledump_output_file_name)))
            {
               CopyStdout(gimpledump_output_file_name);
               THROW_ERROR_CODE(COMPILING_EC, "opt returns an error during compilation " + boost::lexical_cast<std::string>(errno));
               THROW_ERROR("opt returns an error during compilation " + boost::lexical_cast<std::string>(errno));
            }
            else
            {
               THROW_ERROR("Error in opt invocation");
            }
         }
      }
      else
         THROW_ERROR("LTO compilation not yet implemented for the chosen front-end compiler");
      std::string leaf_name = GetLeafFileName(source_files.begin()->second);
      if(not(boost::filesystem::exists(boost::filesystem::path(output_temporary_directory + "/" + leaf_name + STR_CST_gcc_tree_suffix))))
         THROW_ERROR(output_temporary_directory + "/" + leaf_name + STR_CST_gcc_tree_suffix + " not found: impossible to create raw file for " + real_file_names);
      boost::filesystem::path obj = boost::filesystem::path(output_temporary_directory + "/" + leaf_name + STR_CST_gcc_tree_suffix);
      tree_managerRef TreeM = ParseTreeFile(Param, obj.string());
#if !NPROFILE
      long int merge_time = 0;
      START_TIME(merge_time);
#endif
      TM->merge_tree_managers(TreeM);
#if !NPROFILE
      STOP_TIME(merge_time);
      if(output_level >= OUTPUT_LEVEL_VERBOSE)
      {
         dump_exec_time("Tree merging time", merge_time);
      }
#endif
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Ended compilation of single files");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Front-end compiler finished");
}

void GccWrapper::InitializeGccParameters()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Initializing gcc parameters");
   GccWrapper_OptimizationSet optimization_set = OS;
   /// If we are compiling with sparc-elf-gcc
#if HAVE_SPARC_COMPILER
   if(compiler_target == GccWrapper_CompilerTarget::CT_SPARC_ELF_GCC)
   {
      optimization_set = Param->getOption<GccWrapper_OptimizationSet>(OPT_gcc_opt_level);
   }
#endif

   if(Param->isOption(OPT_gcc_read_xml))
   {
      ReadXml(Param->getOption<std::string>(OPT_gcc_read_xml));
   }
   else
   {
      switch(optimization_set)
      {
         case(GccWrapper_OptimizationSet::O0):
         case(GccWrapper_OptimizationSet::O1):
         case(GccWrapper_OptimizationSet::O2):
         case(GccWrapper_OptimizationSet::O3):
         case(GccWrapper_OptimizationSet::O4):
         case(GccWrapper_OptimizationSet::O5):
         case(GccWrapper_OptimizationSet::Og):
         case(GccWrapper_OptimizationSet::Os):
         case(GccWrapper_OptimizationSet::Ofast):
         case(GccWrapper_OptimizationSet::Oz):
            gcc_compiling_parameters += (" -O" + WriteOptimizationLevel(optimization_set) + " ");
            break;
#if HAVE_BAMBU_BUILT
         case GccWrapper_OptimizationSet::OSF:
            gcc_compiling_parameters += (" -O3 -finline-limit=10000 --param inline-unit-growth=100000 ");
            break;
         case(GccWrapper_OptimizationSet::OBAMBU):
#endif
#if HAVE_TUCANO_BUILT
         case(GccWrapper_OptimizationSet::OTUCANO):
#endif
#if HAVE_ZEBU_BUILT
         case(GccWrapper_OptimizationSet::OZEBU):
#endif
            /// Filling optimizations map
#if HAVE_BAMBU_BUILT || HAVE_TUCANO_BUILT || HAVE_ZEBU_BUILT
            SetGccDefault();

            switch(OS)
            {
#if HAVE_BAMBU_BUILT
               case(GccWrapper_OptimizationSet::OBAMBU):
                  SetBambuDefault();
                  break;
#endif
#if HAVE_TUCANO_BUILT
               case(GccWrapper_OptimizationSet::OTUCANO):
                  break;
#endif
#if HAVE_ZEBU_BUILT
               case(GccWrapper_OptimizationSet::OZEBU):
                  SetZebuDefault();
                  break;
#endif
               case(GccWrapper_OptimizationSet::O0):
               case(GccWrapper_OptimizationSet::O1):
               case(GccWrapper_OptimizationSet::O2):
               case(GccWrapper_OptimizationSet::O3):
               case(GccWrapper_OptimizationSet::O4):
               case(GccWrapper_OptimizationSet::O5):
               case(GccWrapper_OptimizationSet::Og):
               case(GccWrapper_OptimizationSet::Os):
               case(GccWrapper_OptimizationSet::Ofast):
               case(GccWrapper_OptimizationSet::Oz):
#if HAVE_BAMBU_BUILT
               case(GccWrapper_OptimizationSet::OSF):
#endif
               {
                  THROW_UNREACHABLE("Unsupported optimization level " + WriteOptimizationLevel(OS));
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
            ReadParameters();

#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
            {
               GccWrapper_CompilerTarget compiler = Param->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler);

               if(false
#if HAVE_I386_CLANG4_COMPILER
                  || compiler == GccWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if HAVE_I386_CLANG5_COMPILER
                  || compiler == GccWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if HAVE_I386_CLANG6_COMPILER
                  || compiler == GccWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if HAVE_I386_CLANG7_COMPILER
                  || compiler == GccWrapper_CompilerTarget::CT_I386_CLANG7
#endif
#if HAVE_I386_CLANG8_COMPILER
                  || compiler == GccWrapper_CompilerTarget::CT_I386_CLANG8
#endif
#if HAVE_I386_CLANG9_COMPILER
                  || compiler == GccWrapper_CompilerTarget::CT_I386_CLANG9
#endif
#if HAVE_I386_CLANG10_COMPILER
                  || compiler == GccWrapper_CompilerTarget::CT_I386_CLANG10
#endif
#if HAVE_I386_CLANG11_COMPILER
                  || compiler == GccWrapper_CompilerTarget::CT_I386_CLANG11
#endif
               )
               {
                  /// sanitize CLANG/LLVM options by removing unsupported GCC options
                  if(optimization_flags.find("tree-pre") != optimization_flags.end())
                     optimization_flags.erase(optimization_flags.find("tree-pre"));
                  if(optimization_flags.find("ipa-cp-clone") != optimization_flags.end())
                     optimization_flags.erase(optimization_flags.find("ipa-cp-clone"));
                  if(optimization_flags.find("ipa-cp") != optimization_flags.end())
                     optimization_flags.erase(optimization_flags.find("ipa-cp"));
               }
            }
#endif

            gcc_compiling_parameters += (" " + WriteOptimizationsString() + " ");

            break;
#endif
         default:
         {
            THROW_UNREACHABLE("Unexpected optimization level");
         }
      }
   }

   /// Adding standard
   if(Param->isOption(OPT_gcc_standard))
   {
      std::string standard = Param->getOption<std::string>(OPT_gcc_standard);
      gcc_compiling_parameters += "--std=" + standard + " ";
   }

   if(Param->isOption(OPT_gcc_E) and Param->getOption<bool>(OPT_gcc_E))
      gcc_compiling_parameters += "-E ";
   if(Param->isOption(OPT_gcc_S) and Param->getOption<bool>(OPT_gcc_S))
      gcc_compiling_parameters += "-S ";
   /// Adding defines
   if(Param->isOption(OPT_gcc_defines))
   {
      const auto defines = Param->getOption<const CustomSet<std::string>>(OPT_gcc_defines);
      for(const auto& define : defines)
      {
         std::string escaped_string = define;
         // add_escape(escaped_string, "\"");
         gcc_compiling_parameters += "-D" + escaped_string + " ";
      }
   }

   /// Adding undefines
   if(Param->isOption(OPT_gcc_undefines))
   {
      const auto undefines = Param->getOption<const CustomSet<std::string>>(OPT_gcc_undefines);
      for(const auto& undefine : undefines)
      {
         std::string escaped_string = undefine;
         // add_escape(escaped_string, "\"");
         gcc_compiling_parameters += "-U" + escaped_string + " ";
      }
   }

   /// Adding warnings
   if(Param->isOption(OPT_gcc_warnings))
   {
      const auto warnings = Param->getOption<const CustomSet<std::string>>(OPT_gcc_warnings);
      for(const auto& warning : warnings)
      {
         gcc_compiling_parameters += "-W" + warning + " ";
      }
   }
#if HAVE_BAMBU_BUILT
   if(OS == GccWrapper_OptimizationSet::OBAMBU)
   {
      gcc_compiling_parameters += "-Wuninitialized ";
   }
#endif

   /// Adding includes
   if(Param->isOption(OPT_gcc_includes))
   {
      gcc_compiling_parameters += Param->getOption<std::string>(OPT_gcc_includes) + " ";
   }

   /// Adding libraries
   if(Param->isOption(OPT_gcc_libraries))
   {
      const auto libraries = Param->getOption<const CustomSet<std::string>>(OPT_gcc_libraries);
      for(const auto& library : libraries)
      {
         gcc_linking_parameters += "-l" + library + " ";
      }
   }

   /// Adding library directories
   if(Param->isOption(OPT_gcc_library_directories))
   {
      const auto library_directories = Param->getOption<const CustomSet<std::string>>(OPT_gcc_library_directories);
      for(const auto& library_directory : library_directories)
      {
         gcc_linking_parameters += "-L" + library_directory + " ";
      }
   }

   /// Adding no parse
   if(Param->isOption(OPT_no_parse_files))
   {
      const auto no_parse_files = Param->getOption<const CustomSet<std::string>>(OPT_no_parse_files);
      for(const auto& no_parse_file : no_parse_files)
      {
         gcc_linking_parameters += no_parse_file + " ";
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Initialized gcc parameters");
}

#if HAVE_ZEBU_BUILT
void GccWrapper::SetZebuDefault()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Setting parameters for Zebu tool...");
   const GccWrapper_OptimizationSet opt_level = Param->getOption<GccWrapper_OptimizationSet>(OPT_gcc_opt_level);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Optimization level: " + WriteOptimizationLevel(opt_level));

   /// parameters with enable
   optimization_flags["ivopts"] = false;            /// introduce target_memory_ref as gimple_assign operands. The default GCC value is true
   optimization_flags["tree-loop-im"] = false;      /// Execution error in 20040307-1.c example. The default GCC value is true
   optimization_flags["tree-loop-ivcanon"] = false; /// this is requested for rebuild while and for
   optimization_flags["tree-sink"] = true;          /// this is requested for rebuild while and for
   optimization_flags["trapping-math"] = true; ///-fno-trapping-math compiles code assuming that floating-point operations cannot generate user-visible traps.  These traps include division by zero, overflow, underflow, inexact result and invalid operation.
                                               /// This option implies -fno-signaling-nans.  Setting this option may allow faster code if one relies on "non-stop" IEEE arithmetic, for example. This option should never be turned on by any -O option since it
                                               /// can result in incorrect output for programs which depend on an exact implementation of IEEE or ISO rules/specifications for math functions.
   optimization_flags["signed-zeros"] = true;  ///-fno-signed-zeros allows optimizations for floating point arithmetic that ignore the signedness of zero. IEEE arithmetic specifies the behavior of distinct +0.0 and -0.0 values, which then prohibits
                                               /// simplification of expressions such as x+0.0 or 0.0*x (even with -ffinite-math-only).
   optimization_flags["rename-registers"] = false; /// cross compilation problems

   /// builtin function;
   optimization_flags["builtin"] = false;

   // optimization_flags["openmp"] = true;

   /// parameters with values
   // FIXME: to be replaced with plugin; deactivated since it does not work with sparc-elf-gcc
   //   optimization_values["tree-parallelize-loops"]=1;///requires tree-loop-optimize

   if(opt_level == GccWrapper_OptimizationSet::O1 or opt_level == GccWrapper_OptimizationSet::O2 or opt_level == GccWrapper_OptimizationSet::O3)
   {
      optimization_flags["guess-branch-probability"] = false; /// error in declaration of structure used probably by profiling.
      optimization_flags["tree-ch"] = false;
      optimization_flags["tree-copy-prop"] = false;      /// va_list undeclared - problem with split of phi nodes
      optimization_flags["tree-dominator-opts"] = false; /// va_list undeclared
      optimization_flags["tree-sra"] = false;            /// Introduces conversion from struct to scalar
      optimization_flags["rename-registers"] = false;    /// cross compilation problems
   }
   if(opt_level == GccWrapper_OptimizationSet::O2 or opt_level == GccWrapper_OptimizationSet::O3)
   {
      optimization_flags["optimize-sibling-calls"] = false; /// Execution error on 20020406-1.c
      optimization_flags["tree-pre"] = false;               /// some loop are incorrecty identified
      optimization_flags["tree-vrp"] = false;               /// create several irriducible loops
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for Zebu tool");
}
#endif

#if HAVE_BAMBU_BUILT
void GccWrapper::SetBambuDefault()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Setting parameters for Bambu tool...");
   const GccWrapper_OptimizationSet opt_level = Param->getOption<GccWrapper_OptimizationSet>(OPT_gcc_opt_level);
#if HAVE_I386_GCC46_COMPILER || HAVE_I386_GCC47_COMPILER || HAVE_I386_GCC48_COMPILER || HAVE_I386_GCC49_COMPILER || HAVE_I386_GCC5_COMPILER || HAVE_I386_GCC6_COMPILER || HAVE_I386_GCC7_COMPILER || HAVE_I386_GCC8_COMPILER || HAVE_I386_CLANG4_COMPILER || \
    HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
   GccWrapper_CompilerTarget compiler = Param->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler);
#endif
#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
   bool flag_cpp;
   if(Param->isOption(OPT_input_format) && Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP)
      flag_cpp = true;
   else
      flag_cpp = false;
#endif
#if HAVE_I386_CLANG4_COMPILER
   if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG4)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      optimization_flags["wrapv"] = true; /// bambu assumes two complement arithmetic
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      if(!flag_cpp)
         optimization_flags["unroll-loops"] = false; // it is preferable to have unrolling disabled by default as with GCC
      return;
   }
#endif
#if HAVE_I386_CLANG5_COMPILER
   if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG5)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      optimization_flags["wrapv"] = true; /// bambu assumes two complement arithmetic
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      if(!flag_cpp)
         optimization_flags["unroll-loops"] = false; // it is preferable to have unrolling disabled by default as with GCC
      return;
   }
#endif
#if HAVE_I386_CLANG6_COMPILER
   if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG6)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      optimization_flags["wrapv"] = true; /// bambu assumes two complement arithmetic
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      if(!flag_cpp)
         optimization_flags["unroll-loops"] = false; // it is preferable to have unrolling disabled by default as with GCC
      return;
   }
#endif
#if HAVE_I386_CLANG7_COMPILER
   if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG7)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      optimization_flags["wrapv"] = true; /// bambu assumes two complement arithmetic
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      return;
   }
#endif
#if HAVE_I386_CLANG8_COMPILER
   if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG8)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      optimization_flags["wrapv"] = true; /// bambu assumes two complement arithmetic
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      return;
   }
#endif
#if HAVE_I386_CLANG9_COMPILER
   if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG9)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      optimization_flags["wrapv"] = true; /// bambu assumes two complement arithmetic
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      optimization_flags["builtin-bcmp"] = false;
      return;
   }
#endif
#if HAVE_I386_CLANG10_COMPILER
   if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG10)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      optimization_flags["wrapv"] = true; /// bambu assumes two complement arithmetic
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      optimization_flags["builtin-bcmp"] = false;
      return;
   }
#endif
#if HAVE_I386_CLANG11_COMPILER
   if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG11)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      optimization_flags["wrapv"] = true; /// bambu assumes two complement arithmetic
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      optimization_flags["builtin-bcmp"] = false;
      return;
   }
#endif

   /// parameters with enable

   /// builtin function;
   // optimization_flags["builtin"] = false;
   // optimization_flags["ipa-type-escape"] = true; /// no more supported starting from gcc 4.6.1
   optimization_flags["wrapv"] = true;          /// bambu assumes twos complement arithmetic
   optimization_flags["tree-copy-prop"] = true; /// FIXME: this has been always active with gcc >= 4.6; produced c code in bambu for example gcc_regression_simple/20040307-1.c when disabled
   optimization_flags["ipa-pta"] = true;

#if HAVE_I386_GCC46_COMPILER || HAVE_I386_GCC47_COMPILER || HAVE_I386_GCC48_COMPILER || HAVE_I386_GCC49_COMPILER || HAVE_I386_GCC5_COMPILER || HAVE_I386_GCC6_COMPILER || HAVE_I386_GCC7_COMPILER || HAVE_I386_GCC8_COMPILER || HAVE_I386_CLANG4_COMPILER || \
    HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
   /// NOTE: the false here is used to be sure that the first operand of the first or always exists
   if(false
#if HAVE_I386_GCC46_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC46
#endif
#if HAVE_I386_GCC47_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC47
#endif
#if HAVE_I386_GCC48_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC48
#endif
#if HAVE_I386_GCC49_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if HAVE_I386_GCC5_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if HAVE_I386_GCC6_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if HAVE_I386_GCC7_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if HAVE_I386_GCC8_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
   )
   {
      optimization_flags["tree-loop-if-convert"] = true;
      optimization_flags["tree-loop-if-convert-stores"] = true;
      optimization_flags["tree-loop-distribute-patterns"] = false;
      optimization_flags["partial-inlining"] = false; /// artificial functions are not analyzed by the plugin
   }
#endif
   if(false
#if HAVE_I386_GCC5_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if HAVE_I386_GCC6_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if HAVE_I386_GCC7_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if HAVE_I386_GCC8_COMPILER
      || compiler == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
   )
   {
      optimization_flags["tree-builtin-call-dce"] = false;
      optimization_flags["ivopts"] = false;
   }
   if(opt_level == GccWrapper_OptimizationSet::O4)
   {
      optimization_flags["ivopts"] = true;
      optimization_flags["tree-loop-ivcanon"] = true;
      optimization_flags["tree-loop-im"] = true;
      optimization_flags["vect-cost-model"] = false;
      optimization_flags["fast-math"] = true;
   }
   if(opt_level == GccWrapper_OptimizationSet::O4 or opt_level == GccWrapper_OptimizationSet::O5)
   {
      if(true
#if HAVE_I386_GCC49_COMPILER
         and compiler != GccWrapper_CompilerTarget::CT_I386_GCC49
#endif
      )
      {
         optimization_flags["tree-loop-linear"] = true;
         optimization_flags["graphite-identity"] = true;
      }
   }
   ///-f option with values
   // optimization_values["tree-parallelize-loops"]=1;///requires tree-loop-optimize
   ///-param option with with values
   if(false
#if HAVE_I386_GCC47_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC47
#endif
#if HAVE_I386_GCC48_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC48
#endif
#if HAVE_I386_GCC49_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if HAVE_I386_GCC5_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if HAVE_I386_GCC6_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if HAVE_I386_GCC7_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if HAVE_I386_GCC8_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
   )
   {
      parameter_values["tree-reassoc-width"] = 128; // Set the maximum number of instructions executed in parallel in reassociated tree.
   }
   if(false
#if HAVE_I386_GCC48_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC48
#endif
#if HAVE_I386_GCC49_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if HAVE_I386_GCC5_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if HAVE_I386_GCC6_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if HAVE_I386_GCC7_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if HAVE_I386_GCC8_COMPILER
      or compiler == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
   )
   {
      parameter_values["max-completely-peeled-insns"] = 250; // Set the maximum number of insns of a peeled loop.
   }

   if(Param->getOption<bool>(OPT_parse_pragma))
   {
      /// Disable duplication of loop headers to preserve openmp for structure
      optimization_flags["tree-ch"] = false;
   }

   if(Param->getOption<int>(OPT_gcc_openmp_simd))
   {
      /// Disable optimizations which break loops patterns
      optimization_flags["tree-pre"] = false;
      optimization_flags["tree-vrp"] = false;
      optimization_flags["ivopts"] = false;
      optimization_flags["tree-dominator-opts"] = false;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
}
#endif

void GccWrapper::SetGccDefault()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Setting front-end compiler defaults");
   const GccWrapper_OptimizationSet optimization_level = Param->getOption<GccWrapper_OptimizationSet>(OPT_gcc_opt_level);
#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
   GccWrapper_CompilerTarget compiler = Param->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler);
#endif
   optimization_flags["stack-protector"] = false; // In Ubuntu 6.10 and later versions this option is enabled by default for C, C++, ObjC, ObjC++

   switch(optimization_level)
   {
      case(GccWrapper_OptimizationSet::Os):
      case(GccWrapper_OptimizationSet::Og):
      case(GccWrapper_OptimizationSet::O1):
      case(GccWrapper_OptimizationSet::O2):
      case(GccWrapper_OptimizationSet::O3):
      case(GccWrapper_OptimizationSet::O4):
      case(GccWrapper_OptimizationSet::O5):
      case(GccWrapper_OptimizationSet::Ofast):
      case(GccWrapper_OptimizationSet::Oz):
      {
         gcc_compiling_parameters += (" -O" + WriteOptimizationLevel(optimization_level) + " ");
         break;
      }
#if HAVE_BAMBU_BUILT
      case GccWrapper_OptimizationSet::OSF:
         gcc_compiling_parameters += (" -O3 -finline-limit=10000");
         break;
#endif
      case(GccWrapper_OptimizationSet::O0):
      {
         gcc_compiling_parameters += " -O1 ";

         if(true
#if HAVE_I386_CLANG4_COMPILER
            && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if HAVE_I386_CLANG5_COMPILER
            && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if HAVE_I386_CLANG6_COMPILER
            && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if HAVE_I386_CLANG7_COMPILER
            && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG7
#endif
#if HAVE_I386_CLANG8_COMPILER
            && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG8
#endif
#if HAVE_I386_CLANG9_COMPILER
            && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG9
#endif
#if HAVE_I386_CLANG10_COMPILER
            && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG10
#endif
#if HAVE_I386_CLANG11_COMPILER
            && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG11
#endif
         )
         {
            optimization_flags["auto-inc-dec"] = false;
            optimization_flags["cprop-registers"] = false;
            optimization_flags["dce"] = false;
            optimization_flags["defer-pop"] = false;
            optimization_flags["delayed-branch"] = false;
            optimization_flags["dse"] = false;
            optimization_flags["guess-branch-probability"] = false;
            optimization_flags["if-conversion"] = false;
            optimization_flags["if-conversion2"] = false;
            optimization_flags["ipa-pure-const"] = false;
            optimization_flags["ipa-reference"] = false;
            optimization_flags["merge-constants"] = false;
            optimization_flags["split-wide-types"] = false;
            optimization_flags["tree-builtin-call-dce"] = false;
            optimization_flags["tree-ccp"] = false;
            optimization_flags["tree-ch"] = false;
            optimization_flags["tree-dce"] = false;
            optimization_flags["tree-dominator-opts"] = false;
            optimization_flags["tree-dse"] = false;
            optimization_flags["tree-forwprop"] = false;
            optimization_flags["tree-fre"] = false;
            optimization_flags["tree-phiprop"] = false;
            optimization_flags["tree-pta"] = false;
            optimization_flags["tree-sra"] = false;
            optimization_flags["tree-ter"] = false;
            /**
             * In the gcc documentation is not clear if unit-at-a-time is activated or not at O0;
             * However it has to be activated to manage empty files
             */
            /// optimization_flags["unit-at-a-time"] = false;
            switch(compiler_target)
            {
#if HAVE_I386_GCC45_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_GCC45):
               {
                  optimization_flags["tree-copy-prop"] = false;
                  break;
               }
#endif
#if HAVE_I386_GCC46_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_GCC46):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  break;
               }
#endif
#if HAVE_I386_GCC47_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_GCC47):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  break;
               }
#endif
#if HAVE_I386_GCC48_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_GCC48):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
#endif
#if HAVE_I386_GCC49_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_GCC49):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
#endif
#if HAVE_I386_GCC5_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_GCC5):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
#endif
#if HAVE_I386_GCC6_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_GCC6):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
#endif
#if HAVE_I386_GCC7_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_GCC7):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
#endif
#if HAVE_I386_GCC8_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_GCC8):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
#endif
#if HAVE_I386_CLANG4_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_CLANG4):
               {
                  break;
               }
#endif
#if HAVE_I386_CLANG5_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_CLANG5):
               {
                  break;
               }
#endif
#if HAVE_I386_CLANG6_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_CLANG6):
               {
                  break;
               }
#endif
#if HAVE_I386_CLANG7_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_CLANG7):
               {
                  break;
               }
#endif
#if HAVE_I386_CLANG8_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_CLANG8):
               {
                  break;
               }
#endif
#if HAVE_I386_CLANG9_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_CLANG9):
               {
                  break;
               }
#endif
#if HAVE_I386_CLANG10_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_CLANG10):
               {
                  break;
               }
#endif
#if HAVE_I386_CLANG11_COMPILER
               case(GccWrapper_CompilerTarget::CT_I386_CLANG11):
               {
                  break;
               }
#endif
#if HAVE_ARM_COMPILER
               case(GccWrapper_CompilerTarget::CT_ARM_GCC):
               {
                  optimization_flags["tree-copy-prop"] = false;
                  break;
               }
#endif
#if HAVE_SPARC_COMPILER
               case(GccWrapper_CompilerTarget::CT_SPARC_GCC):
               case(GccWrapper_CompilerTarget::CT_SPARC_ELF_GCC):
               {
                  optimization_flags["tree-copy-prop"] = false;
                  break;
               }
#endif
               case(GccWrapper_CompilerTarget::CT_NO_GCC):
               {
                  THROW_UNREACHABLE("Unexpected gcc target");
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
         }
         break;
      }
#if HAVE_BAMBU_BUILT
      case(GccWrapper_OptimizationSet::OBAMBU):
#endif
#if HAVE_TUCANO_BUILT
      case(GccWrapper_OptimizationSet::OTUCANO):
#endif
#if HAVE_ZEBU_BUILT
      case(GccWrapper_OptimizationSet::OZEBU):
#endif
      {
         THROW_UNREACHABLE("Unepected optimization level: " + WriteOptimizationLevel(optimization_level));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   /// required by PandA
   if(true
#if HAVE_I386_CLANG4_COMPILER
      && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if HAVE_I386_CLANG5_COMPILER
      && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if HAVE_I386_CLANG6_COMPILER
      && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if HAVE_I386_CLANG7_COMPILER
      && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG7
#endif
#if HAVE_I386_CLANG8_COMPILER
      && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG8
#endif
#if HAVE_I386_CLANG9_COMPILER
      && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG9
#endif
#if HAVE_I386_CLANG10_COMPILER
      && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG10
#endif
#if HAVE_I386_CLANG11_COMPILER
      && compiler != GccWrapper_CompilerTarget::CT_I386_CLANG11
#endif
   )
   {
      optimization_flags["ipa-pure-const"] = true; /// needed to correctly manage global variables
      optimization_flags["tree-dce"] = true;       /// needed to remove unnecessary computations
   }
#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
   else
   {
      optimization_flags["vectorize"] = false;     /// disable vectorization
      optimization_flags["slp-vectorize"] = false; /// disable superword-level parallelism vectorization
   }
#endif
   bool flag_cpp;
   if(Param->isOption(OPT_input_format) && Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP)
      flag_cpp = true;
   else
      flag_cpp = false;
   /// in case we are compiling C++ code
   if(flag_cpp)
   {
      optimization_flags["exceptions"] = false;
      optimization_flags["threadsafe-statics"] = false;
      optimization_flags["use-cxa-atexit"] = false;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set front-end compiler defaults");
}

GccWrapper::Compiler GccWrapper::GetCompiler() const
{
   Compiler compiler;
#if HAVE_I386_GCC45_COMPILER || HAVE_I386_GCC46_COMPILER || HAVE_I386_GCC47_COMPILER || HAVE_I386_GCC48_COMPILER || HAVE_I386_GCC49_COMPILER || HAVE_I386_GCC5_COMPILER || HAVE_I386_GCC6_COMPILER || HAVE_I386_GCC7_COMPILER || HAVE_I386_GCC8_COMPILER || \
    HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER ||               \
    HAVE_SPARC_COMPILER || HAVE_ARM_COMPILER
#ifndef NDEBUG
   GccWrapper_CompilerTarget compatible_compilers = Param->getOption<GccWrapper_CompilerTarget>(OPT_compatible_compilers);
#endif
#endif

#if HAVE_I386_GCC45_COMPILER || HAVE_I386_GCC46_COMPILER || HAVE_I386_GCC47_COMPILER || HAVE_I386_GCC48_COMPILER || HAVE_I386_GCC49_COMPILER || HAVE_I386_GCC5_COMPILER || HAVE_I386_GCC6_COMPILER || HAVE_I386_GCC7_COMPILER || HAVE_I386_GCC8_COMPILER || \
    HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER ||               \
    HAVE_SPARC_COMPILER || HAVE_ARM_COMPILER
   bool flag_cpp;
   if(Param->isOption(OPT_input_format) && Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP && !Param->isOption(OPT_pretty_print))
      flag_cpp = true;
   else
      flag_cpp = false;
#endif

#if HAVE_I386_GCC45_COMPILER || HAVE_I386_GCC46_COMPILER || HAVE_I386_GCC47_COMPILER || HAVE_I386_GCC48_COMPILER || HAVE_I386_GCC49_COMPILER || HAVE_I386_GCC5_COMPILER || HAVE_I386_GCC6_COMPILER || HAVE_I386_GCC7_COMPILER || HAVE_I386_GCC8_COMPILER || \
    HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER ||               \
    HAVE_SPARC_COMPILER || HAVE_ARM_COMPILER || HAVE_SPARC_ELF_GCC
   std::string gcc_extra_options;
   if(Param->isOption(OPT_gcc_extra_options))
      gcc_extra_options = Param->getOption<std::string>(OPT_gcc_extra_options);
#endif

#if HAVE_I386_GCC45_COMPILER || HAVE_I386_GCC46_COMPILER || HAVE_I386_GCC47_COMPILER || HAVE_I386_GCC48_COMPILER || HAVE_I386_GCC49_COMPILER || HAVE_I386_GCC5_COMPILER || HAVE_I386_GCC6_COMPILER || HAVE_I386_GCC7_COMPILER || HAVE_I386_GCC8_COMPILER || \
    HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER ||               \
    HAVE_SPARC_COMPILER || HAVE_ARM_COMPILER
   GccWrapper_CompilerTarget preferred_compiler;
   if(compiler_target == GccWrapper_CompilerTarget::CT_NO_GCC)
   {
      preferred_compiler = Param->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler);
   }
   else
   {
#ifndef NDEBUG
      const bool debug_condition =
#if HAVE_SPARC_COMPILER
          (compiler_target == GccWrapper_CompilerTarget::CT_SPARC_ELF_GCC) or
#endif
          (static_cast<int>(compiler_target) & static_cast<int>(compatible_compilers));
      THROW_ASSERT(debug_condition, "Required compiler is not among the compatible one: " + STR(static_cast<int>(compiler_target)) + " vs " + STR(static_cast<int>(compatible_compilers)));
#endif
      preferred_compiler = compiler_target;
   }
   const std::string gcc_plugin_dir = (Param->isOption(OPT_gcc_plugindir) ? Param->getOption<std::string>(OPT_gcc_plugindir) : relocate_compiler_path(GCC_PLUGIN_DIR)) + "/";
   const std::string clang_plugin_dir = (Param->isOption(OPT_gcc_plugindir) ? Param->getOption<std::string>(OPT_gcc_plugindir) + "/../clang_plugin" : relocate_compiler_path(CLANG_PLUGIN_DIR)) + "/";
   const std::string plugin_ext = ".so";
#endif
   auto fillASTAnalyzer_plugin = [&] {
#if HAVE_I386_CLANG11_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG11_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG11_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG10_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG10_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG10_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG9_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG9_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG9_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG8_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG8_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG8_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG7_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG7_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG7_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG6_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG6_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG6_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG5_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG5_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG5_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG4_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG4_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG4_ASTANALYZER_PLUGIN;
#endif
   };

#if HAVE_I386_GCC45_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC45))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP45_EXE) : relocate_compiler_path(I386_GCC45_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP45_EXE);
      compiler.extra_options = Param->getOption<std::string>(OPT_gcc_m32_mx32) + " -D_FORTIFY_SOURCE=0 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() && optimization_flags.find("tree-vectorize")->second)
         compiler.extra_options += " -msse2 -mfpmath=sse";
      else
         compiler.extra_options += " -mno-sse2";
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC45_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC45_EMPTY_PLUGIN;
      if(optimization_flags.find("tree-vrp") != optimization_flags.end() && optimization_flags.find("tree-vrp")->second)
      {
         compiler.ssa_plugin_obj = gcc_plugin_dir + I386_GCC45_SSAVRP_PLUGIN + plugin_ext;
         compiler.ssa_plugin_name = I386_GCC45_SSAVRP_PLUGIN;
      }
      else
      {
         compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? I386_GCC45_SSA_PLUGINCPP : I386_GCC45_SSA_PLUGIN) + plugin_ext;
         compiler.ssa_plugin_name = (flag_cpp ? I386_GCC45_SSA_PLUGINCPP : I386_GCC45_SSA_PLUGIN);
      }
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC45_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC45_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + "";
#endif
   }
#endif
#if HAVE_I386_GCC46_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC46))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP46_EXE) : relocate_compiler_path(I386_GCC46_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP46_EXE);
      compiler.extra_options = Param->getOption<std::string>(OPT_gcc_m32_mx32) + " -D_FORTIFY_SOURCE=0 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() && optimization_flags.find("tree-vectorize")->second)
         compiler.extra_options += " -msse2 -mfpmath=sse";
      else
         compiler.extra_options += " -mno-sse2";
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC46_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC46_EMPTY_PLUGIN;
      if(optimization_flags.find("tree-vrp") != optimization_flags.end() && optimization_flags.find("tree-vrp")->second)
      {
         compiler.ssa_plugin_obj = gcc_plugin_dir + I386_GCC46_SSAVRP_PLUGIN + plugin_ext;
         compiler.ssa_plugin_name = I386_GCC46_SSAVRP_PLUGIN;
      }
      else
      {
         compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? I386_GCC46_SSA_PLUGINCPP : I386_GCC46_SSA_PLUGIN) + plugin_ext;
         compiler.ssa_plugin_name = (flag_cpp ? I386_GCC46_SSA_PLUGINCPP : I386_GCC46_SSA_PLUGIN);
      }
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC46_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC46_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + "";
#endif
   }
#endif
#if HAVE_I386_GCC47_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC47))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP47_EXE) : relocate_compiler_path(I386_GCC47_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP47_EXE);
      compiler.extra_options = " -D_FORTIFY_SOURCE=0 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() && optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC47_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
#endif
         compiler.extra_options += " -msse2 -mfpmath=sse";
      }
      else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC47_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC47_EMPTY_PLUGIN;
      if(optimization_flags.find("tree-vrp") != optimization_flags.end() && optimization_flags.find("tree-vrp")->second)
      {
         compiler.ssa_plugin_obj = gcc_plugin_dir + I386_GCC47_SSAVRP_PLUGIN + plugin_ext;
         compiler.ssa_plugin_name = I386_GCC47_SSAVRP_PLUGIN;
      }
      else
      {
         compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? I386_GCC47_SSA_PLUGINCPP : I386_GCC47_SSA_PLUGIN) + plugin_ext;
         compiler.ssa_plugin_name = (flag_cpp ? I386_GCC47_SSA_PLUGINCPP : I386_GCC47_SSA_PLUGIN);
      }
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC47_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC47_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + "";
#endif
   }
#endif
#if HAVE_I386_GCC48_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC48))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP48_EXE) : relocate_compiler_path(I386_GCC48_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP48_EXE);
      compiler.extra_options = " -mlong-double-64 -D_FORTIFY_SOURCE=0 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() && optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC48_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
#endif
         compiler.extra_options += " -msse2 -mfpmath=sse";
      }
      else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC48_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC48_EMPTY_PLUGIN;
      if(optimization_flags.find("tree-vrp") != optimization_flags.end() && optimization_flags.find("tree-vrp")->second)
      {
         compiler.ssa_plugin_obj = gcc_plugin_dir + I386_GCC48_SSAVRP_PLUGIN + plugin_ext;
         compiler.ssa_plugin_name = I386_GCC48_SSAVRP_PLUGIN;
      }
      else
      {
         compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? I386_GCC48_SSA_PLUGINCPP : I386_GCC48_SSA_PLUGIN) + plugin_ext;
         compiler.ssa_plugin_name = (flag_cpp ? I386_GCC48_SSA_PLUGINCPP : I386_GCC48_SSA_PLUGIN);
      }
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC48_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC48_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + "";
#endif
   }
#endif
#if HAVE_I386_GCC49_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC49))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP49_EXE) : relocate_compiler_path(I386_GCC49_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP49_EXE);
      compiler.extra_options = " -mlong-double-64 -D_FORTIFY_SOURCE=0 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() && optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC49_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
#endif
         compiler.extra_options += " -msse2 -mfpmath=sse";
      }
      else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC49_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC49_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? I386_GCC49_SSA_PLUGINCPP : I386_GCC49_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC49_SSA_PLUGINCPP : I386_GCC49_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC49_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC49_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + "";
#endif
   }
#endif
#if HAVE_I386_GCC5_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC5))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP5_EXE) : relocate_compiler_path(I386_GCC5_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP5_EXE);
      compiler.extra_options = " -mlong-double-64 -D_FORTIFY_SOURCE=0 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() && optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC5_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
#endif
         compiler.extra_options += " -msse2 -mfpmath=sse";
      }
      else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC5_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC5_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? I386_GCC5_SSA_PLUGINCPP : I386_GCC5_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC5_SSA_PLUGINCPP : I386_GCC5_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC5_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC5_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + "";
#endif
   }
#endif

#if HAVE_I386_GCC6_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC6))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP6_EXE) : relocate_compiler_path(I386_GCC6_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP6_EXE);
      compiler.extra_options = " -mlong-double-64 -D_FORTIFY_SOURCE=0 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() && optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC6_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
#endif
         compiler.extra_options += " -msse2 -mfpmath=sse";
      }
      else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC6_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC6_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? I386_GCC6_SSA_PLUGINCPP : I386_GCC6_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC6_SSA_PLUGINCPP : I386_GCC6_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC6_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC6_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + "";
#endif
   }
#endif

#if HAVE_I386_GCC7_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC7))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP7_EXE) : relocate_compiler_path(I386_GCC7_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP7_EXE);
      compiler.extra_options = " -mlong-double-64 -D_FORTIFY_SOURCE=0 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() && optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC7_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
#endif
         compiler.extra_options += " -msse2 -mfpmath=sse";
      }
      else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC7_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC7_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? I386_GCC7_SSA_PLUGINCPP : I386_GCC7_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC7_SSA_PLUGINCPP : I386_GCC7_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC7_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC7_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + "";
#endif
   }
#endif

#if HAVE_I386_GCC8_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC8))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP8_EXE) : relocate_compiler_path(I386_GCC8_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP8_EXE);
      compiler.extra_options = " -mlong-double-64 -D_FORTIFY_SOURCE=0 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() && optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC8_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
#endif
         compiler.extra_options += " -msse2 -mfpmath=sse";
      }
      else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC8_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC8_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? I386_GCC8_SSA_PLUGINCPP : I386_GCC8_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC8_SSA_PLUGINCPP : I386_GCC8_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC8_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC8_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + "";
#endif
   }
#endif

#if HAVE_I386_CLANG4_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG4))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP4_EXE) : relocate_compiler_path(I386_CLANG4_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP4_EXE);
      compiler.extra_options = " -D_FORTIFY_SOURCE=0 " + gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG4_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG4_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = clang_plugin_dir + (flag_cpp ? I386_CLANG4_SSA_PLUGINCPP : I386_CLANG4_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG4_SSA_PLUGINCPP : I386_CLANG4_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG4_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG4_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG4_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG4_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG4_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG4_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG4_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG4_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG4_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG4_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM4_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM4_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANG5_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG5))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP5_EXE) : relocate_compiler_path(I386_CLANG5_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP5_EXE);
      compiler.extra_options = " -D_FORTIFY_SOURCE=0 " + gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG5_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG5_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = clang_plugin_dir + (flag_cpp ? I386_CLANG5_SSA_PLUGINCPP : I386_CLANG5_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG5_SSA_PLUGINCPP : I386_CLANG5_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG5_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG5_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG5_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG5_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG5_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG5_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG5_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG5_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG5_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG5_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM5_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM5_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANG6_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG6))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP6_EXE) : relocate_compiler_path(I386_CLANG6_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP6_EXE);
      compiler.extra_options = " -D_FORTIFY_SOURCE=0 " + gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG6_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG6_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = clang_plugin_dir + (flag_cpp ? I386_CLANG6_SSA_PLUGINCPP : I386_CLANG6_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG6_SSA_PLUGINCPP : I386_CLANG6_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG6_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG6_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG6_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG6_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG6_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG6_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG6_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG6_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG6_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG6_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM6_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM6_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANG7_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG7))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP7_EXE) : relocate_compiler_path(I386_CLANG7_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP7_EXE);
      compiler.extra_options = " -D_FORTIFY_SOURCE=0 " + gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG7_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG7_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = clang_plugin_dir + (flag_cpp ? I386_CLANG7_SSA_PLUGINCPP : I386_CLANG7_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG7_SSA_PLUGINCPP : I386_CLANG7_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG7_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG7_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG7_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG7_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG7_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG7_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG7_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG7_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG7_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG7_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM7_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM7_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANG8_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG8))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP8_EXE) : relocate_compiler_path(I386_CLANG8_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP8_EXE);
      compiler.extra_options = " -D_FORTIFY_SOURCE=0 " + gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG8_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG8_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = clang_plugin_dir + (flag_cpp ? I386_CLANG8_SSA_PLUGINCPP : I386_CLANG8_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG8_SSA_PLUGINCPP : I386_CLANG8_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG8_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG8_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG8_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG8_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG8_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG8_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG8_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG8_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG8_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG8_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM8_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM8_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANG9_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG9))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP9_EXE) : relocate_compiler_path(I386_CLANG9_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP9_EXE);
      compiler.extra_options = " -D_FORTIFY_SOURCE=0 " + gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG9_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG9_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = clang_plugin_dir + (flag_cpp ? I386_CLANG9_SSA_PLUGINCPP : I386_CLANG9_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG9_SSA_PLUGINCPP : I386_CLANG9_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG9_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG9_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG9_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG9_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG9_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG9_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG9_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG9_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG9_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG9_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM9_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM9_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANG10_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG10))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP10_EXE) : relocate_compiler_path(I386_CLANG10_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP10_EXE);
      compiler.extra_options = " -D_FORTIFY_SOURCE=0 " + gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG10_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG10_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = clang_plugin_dir + (flag_cpp ? I386_CLANG10_SSA_PLUGINCPP : I386_CLANG10_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG10_SSA_PLUGINCPP : I386_CLANG10_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG10_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG10_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG10_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG10_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG10_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG10_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG10_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG10_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG10_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG10_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM10_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM10_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANG11_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG11))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP11_EXE) : relocate_compiler_path(I386_CLANG11_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP11_EXE);
      compiler.extra_options = " -D_FORTIFY_SOURCE=0 " + gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m32_mx32);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG11_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG11_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = clang_plugin_dir + (flag_cpp ? I386_CLANG11_SSA_PLUGINCPP : I386_CLANG11_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG11_SSA_PLUGINCPP : I386_CLANG11_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG11_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG11_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG11_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG11_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG11_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG11_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG11_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG11_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG11_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG11_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM11_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM11_OPT_EXE);
   }
#endif

#if HAVE_SPARC_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_SPARC_GCC))
   {
      compiler.gcc = relocate_compiler_path(SPARC_GCC_EXE);
      compiler.cpp = relocate_compiler_path(SPARC_CPP_EXE);
      compiler.extra_options = gcc_extra_options;
      compiler.empty_plugin_obj = gcc_plugin_dir + SPARC_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = SPARC_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? SPARC_SSA_PLUGINCPP : SPARC_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? SPARC_SSA_PLUGINCPP : SPARC_SSA_PLUGIN);
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + SPARC_RTL_PLUGIN + plugin_ext;
#endif
   }
#endif
#if HAVE_SPARC_ELF_GCC
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_SPARC_ELF_GCC))
   {
      compiler.gcc = SPARC_ELF_GCC;
      compiler.cpp = SPARC_ELF_CPP;
      compiler.extra_options = gcc_extra_options;
   }
#endif
#if HAVE_ARM_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_ARM_GCC))
   {
      compiler.gcc = relocate_compiler_path(ARM_GCC_EXE);
      compiler.cpp = relocate_compiler_path(ARM_CPP_EXE);
      compiler.extra_options = gcc_extra_options + " -mlittle-endian -fsigned-char";
      compiler.empty_plugin_obj = gcc_plugin_dir + ARM_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = ARM_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj = gcc_plugin_dir + (flag_cpp ? ARM_SSA_PLUGINCPP : ARM_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? ARM_SSA_PLUGINCPP : ARM_SSA_PLUGIN);
#if HAVE_FROM_RTL_BUILT
      compiler.rtl_plugin = gcc_plugin_dir + ARM_RTL_PLUGIN + plugin_ext;
#endif
   }
#endif
   if(compiler.gcc == "")
   {
      THROW_ERROR("Not found any compatible compiler");
   }
   return compiler;
}

void GccWrapper::GetSystemIncludes(std::vector<std::string>& includes) const
{
   /// This string contains the path and name of the compiler to be invoked
   const std::string cpp = GetCompiler().cpp.string();

   std::string command =
       cpp + " -v  < /dev/null 2>&1 | grep -v -E \"(#|Configured with|Using built-in|Target|Thread model|gcc version|End of search list|ignoring nonexistent directory|cc1 -E -quiet|cc1.exe -E -quiet|COMPILER_PATH|LIBRARY_PATH|COLLECT_GCC|ignoring "
             "duplicate directory|ignoring nonexistent directory|InstalledDir|clang version|Found candidate|Selected GCC installation|Candidate multilib|Selected multilib|-cc1)\" | tr '\\n' ' ' | tr '\\r' ' '  | sed 's/\\\\/\\//g'";
   int ret = PandaSystem(Param, command, STR_CST_gcc_include);
   PRINT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "");
   if(IsError(ret))
   {
      util_print_cpu_stats(std::cerr);
      THROW_ERROR("Error in retrieving gcc system include. Error is " + boost::lexical_cast<std::string>(ret));
   }

   std::string list_of_dirs;

   std::ifstream includefile(GetPath(STR_CST_gcc_include));
   if(includefile.is_open())
   {
      std::string line;
      while(!includefile.eof())
      {
         getline(includefile, line);
         if(line.size())
         {
            list_of_dirs += line;
         }
      }
   }
   else
      THROW_ERROR("Error in retrieving gcc system include");

   std::remove(GetPath(STR_CST_gcc_include).c_str());

   // Ok, now here there are the list of the system paths in which
   // the system includes are found
   includes = SplitString(list_of_dirs, " ");
}

void GccWrapper::GetGccConfig() const
{
   QueryGccConfig("-v");
}

void GccWrapper::QueryGccConfig(const std::string& gcc_option) const
{
   const std::string gcc = GetCompiler().gcc.string();
   const std::string command = gcc + " " + gcc_option;
   const std::string output_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_file_IO_shell_output_file;
   int ret = PandaSystem(Param, command, output_file_name);
   if(IsError(ret))
   {
      THROW_ERROR("Error in retrieving gcc configuration");
   }
   CopyStdout(output_file_name);
}

size_t GccWrapper::GetSourceCodeLines(const ParameterConstRef Param)
{
#ifndef NDEBUG
   const int debug_level = Param->get_class_debug_level("GccWrapper");
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Getting source code lines number");
   /// The output file
   const std::string output_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_file_IO_shell_output_file;

   std::string command = "cat ";
   const auto source_files = Param->getOption<const CustomSet<std::string>>(OPT_input_file);
   for(const auto& source_file : source_files)
   {
      boost::filesystem::path absolute_path = boost::filesystem::system_complete(source_file);
      command += absolute_path.branch_path().string() + "/*\\.h ";
      command += source_file + " ";
   }
   command += std::string(" 2> /dev/null | wc -l");
   int ret = PandaSystem(Param, command, output_file_name);
   if(IsError(ret))
   {
      THROW_ERROR("Error during execution of computing word lines");
   }

   std::ifstream output_file(output_file_name.c_str());
   if(output_file.is_open() and !output_file.eof())
   {
      std::string line;
      getline(output_file, line);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Got " + line);
      return boost::lexical_cast<size_t>(line);
   }
   else
   {
      THROW_ERROR("Error in opening " + output_file_name);
   }
   return 0;
}

void GccWrapper::CreateExecutable(const CustomSet<std::string>& file_names, const std::string& executable_name, const std::string& extra_gcc_options, bool no_gcc_compiling_parameters) const
{
   std::list<std::string> sorted_file_names;
   for(const auto& file_name : file_names)
      sorted_file_names.push_back(file_name);
   CreateExecutable(sorted_file_names, executable_name, extra_gcc_options, no_gcc_compiling_parameters);
}
void GccWrapper::CreateExecutable(const std::list<std::string>& file_names, const std::string& executable_name, const std::string& extra_gcc_options, bool no_gcc_compiling_parameters) const
{
   std::string file_names_string;
   bool has_cpp_file = false;
   for(const auto& file_name : file_names)
   {
      if(Param->GetFileFormat(file_name, false) == Parameters_FileFormat::FF_CPP)
         has_cpp_file = true;
      file_names_string += file_name + " ";
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating executable " + executable_name + " from " + file_names_string);
   std::string command;

   Compiler compiler = GetCompiler();
   command = compiler.gcc.string() + " ";

   command += file_names_string + " ";

   command += (no_gcc_compiling_parameters ? "" : gcc_compiling_parameters) + " " + AddSourceCodeIncludes(file_names) + " " + gcc_linking_parameters + " ";
   if(!has_cpp_file && command.find("--std=c++14") != std::string::npos)
      boost::replace_all(command, "--std=c++14", "");

   command += "-D__NO_INLINE__ "; /// needed to avoid problem with glibc inlines

   std::string local_compiler_extra_options = no_gcc_compiling_parameters ? "" : compiler.extra_options;
   if(extra_gcc_options.find("-m32") != std::string::npos)
      boost::replace_all(local_compiler_extra_options, "-mx32", "");

#ifdef _WIN32
   if(local_compiler_extra_options.find("-m32") != std::string::npos)
      boost::replace_all(local_compiler_extra_options, "-m32", "");
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Extra options are " + local_compiler_extra_options);
   command += local_compiler_extra_options + " " + extra_gcc_options + " ";

   command += "-o " + executable_name + " ";

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Compilation command is " + command);
   const std::string gcc_output_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_gcc_output;

   int ret = PandaSystem(Param, command, gcc_output_file_name);
   if(IsError(ret))
   {
      CopyStdout(gcc_output_file_name);
      THROW_ERROR_CODE(COMPILING_EC, "Front-end compiler returns an error during compilation " + boost::lexical_cast<std::string>(errno) + " - Command is " + command);
   }
   else
   {
      if(output_level >= OUTPUT_LEVEL_VERBOSE)
         CopyStdout(gcc_output_file_name);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
}

void GccWrapper::ReadParameters()
{
   if(Param->isOption(OPT_gcc_optimizations))
   {
      const auto parameters = Param->getOption<const CustomSet<std::string>>(OPT_gcc_optimizations);
      for(const auto& parameter : parameters)
      {
         THROW_ASSERT(parameter != "", "unexpected condition:" + Param->getOption<std::string>(OPT_gcc_optimizations));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining parameter " + parameter);
         const size_t found = parameter.find("no-");
         // if the token starts with "no-", the optimization has to be disabled
         if(found != std::string::npos and found == 0)
         {
            std::string str = parameter.substr(found + std::string("no-").size());
            optimization_flags[str] = false;
         }
         else
         {
            optimization_flags[parameter] = true;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
   }
   if(Param->isOption(OPT_gcc_parameters))
   {
      const auto parameters = Param->getOption<const CustomSet<std::string>>(OPT_gcc_parameters);
      for(const auto& parameter : parameters)
      {
         const size_t equal_size = parameter.find("=");
         if(equal_size == std::string::npos)
         {
            THROW_ERROR("--param argument without value " + parameter);
         }
         const std::string key = parameter.substr(0, equal_size);
         const std::string value = parameter.substr(equal_size + 1, parameter.size() - equal_size + 1);
         parameter_values[key] = boost::lexical_cast<int>(value);
      }
   }
}

std::string GccWrapper::WriteOptimizationLevel(const GccWrapper_OptimizationSet optimization_level)
{
   switch(optimization_level)
   {
      case(GccWrapper_OptimizationSet::O0):
         return "0";
      case(GccWrapper_OptimizationSet::O1):
         return "1";
      case(GccWrapper_OptimizationSet::O2):
         return "2";
      case(GccWrapper_OptimizationSet::O3):
         return "3";
      case(GccWrapper_OptimizationSet::O4):
         return "4";
      case(GccWrapper_OptimizationSet::O5):
         return "5";
      case(GccWrapper_OptimizationSet::Og):
         return "g";
      case(GccWrapper_OptimizationSet::Os):
         return "s";
      case(GccWrapper_OptimizationSet::Ofast):
         return "fast";
      case(GccWrapper_OptimizationSet::Oz):
         return "z";
#if HAVE_BAMBU_BUILT
      case(GccWrapper_OptimizationSet::OBAMBU):
         return "bambu";
      case(GccWrapper_OptimizationSet::OSF):
         return "softfloat";
#endif
#if HAVE_TUCANO_BUILT
      case(GccWrapper_OptimizationSet::OTUCANO):
         return "tucano";
#endif
#if HAVE_ZEBU_BUILT
      case(GccWrapper_OptimizationSet::OZEBU):
         return "zebu";
#endif
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return "";
}

std::string GccWrapper::WriteOptimizationsString()
{
   std::string optimizations;
   /// Preparing optimizations string
   bool argument_noalias = optimization_flags.find("argument-noalias") != optimization_flags.end() and optimization_flags.find("argument-noalias")->second;
   bool argument_noalias_global = optimization_flags.find("argument-noalias-global") != optimization_flags.end() and optimization_flags.find("argument-noalias-global")->second;
   bool argument_noalias_anything = optimization_flags.find("argument-noalias-anything") != optimization_flags.end() and optimization_flags.find("argument-noalias-anything")->second;
   bool strict_aliasing = optimization_flags.find("strict-aliasing") != optimization_flags.end() and optimization_flags.find("strict-aliasing")->second;
   std::map<std::string, bool>::const_iterator it, it_end = optimization_flags.end();
   if(strict_aliasing)
      optimizations += std::string("-Wstrict-aliasing ");
   for(it = optimization_flags.begin(); it != it_end; ++it)
   {
      /*argument aliasing should be treated in a different way*/
      if(it->first == "argument-alias" and (argument_noalias or argument_noalias_global or argument_noalias_anything))
         continue;
      else if(it->first == "argument-noalias" and (argument_noalias_global or argument_noalias_anything))
         continue;
      else if(it->first == "argument-noalias-global" and (argument_noalias_anything))
         continue;
      THROW_ASSERT(it->first != "", "unexpected condition");
      if(it->second)
         optimizations += std::string("-f") + it->first + " "; // enable optimizations set to true
      else
         optimizations += std::string("-fno-") + it->first + " "; // disable optimizations set to false
   }
   std::map<std::string, int>::const_iterator it2, it2_end = optimization_values.end();
   for(it2 = optimization_values.begin(); it2 != it2_end; ++it2)
   {
      optimizations += std::string("-f") + it2->first + "=" + boost::lexical_cast<std::string>(it2->second) + " ";
   }
   std::map<std::string, int>::const_iterator it3, it3_end = parameter_values.end();
   for(it3 = parameter_values.begin(); it3 != it3_end; ++it3)
   {
      optimizations += "--param " + it3->first + "=" + boost::lexical_cast<std::string>(it3->second) + " ";
   }
   return optimizations;
}

void GccWrapper::ReadXml(const std::string& file_name)
{
   try
   {
      XMLDomParser parser(file_name);
      parser.Exec();
      if(parser)
      {
         const xml_element* root = parser.get_document()->get_root_node();

         const xml_node::node_list root_children = root->get_children();
         xml_node::node_list::const_iterator root_child, root_child_end = root_children.end();
         for(root_child = root_children.begin(); root_child != root_child_end; ++root_child)
         {
            const auto* root_child_element = GetPointer<const xml_element>(*root_child);
            if(not root_child_element)
               continue;
            if(root_child_element->get_name() == STR_XML_gcc_optimizations)
            {
               const xml_node::node_list optimizations_children = root_child_element->get_children();
               xml_node::node_list::const_iterator optimizations_child, optimizations_child_end = optimizations_children.end();
               for(optimizations_child = optimizations_children.begin(); optimizations_child != optimizations_child_end; ++optimizations_child)
               {
                  const auto* optimizations_child_element = GetPointer<const xml_element>(*optimizations_child);
                  if(not optimizations_child_element)
                     continue;
                  if(optimizations_child_element->get_name() == STR_XML_gcc_parameter_values)
                  {
                     const xml_node::node_list parameter_values_children = optimizations_child_element->get_children();
                     xml_node::node_list::const_iterator parameter_value, parameter_value_end = parameter_values_children.end();
                     for(parameter_value = parameter_values_children.begin(); parameter_value != parameter_value_end; ++parameter_value)
                     {
                        const auto* parameter_value_element = GetPointer<const xml_element>(*parameter_value);
                        if(not parameter_value_element)
                           continue;
                        if(not(parameter_value_element->get_attribute(STR_XML_gcc_name) and parameter_value_element->get_attribute(STR_XML_gcc_value)))
                        {
                           THROW_ERROR("Parameter value node without name or value");
                        }
                        parameter_values[parameter_value_element->get_attribute(STR_XML_gcc_name)->get_value()] = boost::lexical_cast<int>(parameter_value_element->get_attribute(STR_XML_gcc_value));
                     }
                  }
                  else if(optimizations_child_element->get_name() == STR_XML_gcc_optimization_flags)
                  {
                     const xml_node::node_list optimization_flags_children = optimizations_child_element->get_children();
                     xml_node::node_list::const_iterator optimization_flag, optimization_flag_end = optimization_flags_children.end();
                     for(optimization_flag = optimization_flags_children.begin(); optimization_flag != optimization_flag_end; ++optimization_flag)
                     {
                        const auto* optimization_flag_element = GetPointer<const xml_element>(*optimization_flag);
                        if(not optimization_flag_element)
                           continue;
                        if(not(optimization_flag_element->get_attribute(STR_XML_gcc_name) and optimization_flag_element->get_attribute(STR_XML_gcc_value)))
                        {
                           THROW_ERROR("Optimization flag node without name or value");
                        }
                        optimization_flags[optimization_flag_element->get_attribute(STR_XML_gcc_name)->get_value()] = boost::lexical_cast<bool>(optimization_flag_element->get_attribute(STR_XML_gcc_value));
                     }
                  }
                  else if(optimizations_child_element->get_name() == STR_XML_gcc_optimization_values)
                  {
                     const xml_node::node_list optimization_value_children = optimizations_child_element->get_children();
                     xml_node::node_list::const_iterator optimization_value, optimization_value_end = optimization_value_children.end();
                     for(optimization_value = optimization_value_children.begin(); optimization_value != optimization_value_end; ++optimization_value)
                     {
                        const auto* optimization_value_element = GetPointer<const xml_element>(*optimization_value);
                        if(not optimization_value_element)
                           continue;
                        if(not(optimization_value_element->get_attribute(STR_XML_gcc_name) and optimization_value_element->get_attribute(STR_XML_gcc_value)))
                        {
                           THROW_ERROR("Optimization value node without name or value");
                        }
                        optimization_values[optimization_value_element->get_attribute(STR_XML_gcc_name)->get_value()] = boost::lexical_cast<int>(optimization_value_element->get_attribute(STR_XML_gcc_value));
                     }
                  }
               }
               gcc_compiling_parameters += (" " + WriteOptimizationsString() + " ");
            }
            else if(root_child_element->get_name() == STR_XML_gcc_standard)
            {
               const xml_element* standard = root_child_element;
               if(not standard->get_attribute(STR_XML_gcc_value))
               {
                  THROW_ERROR("Standard node without value");
               }
               gcc_compiling_parameters += "--std=" + standard->get_attribute(STR_XML_gcc_value)->get_value() + " ";
            }
            else if(root_child_element->get_name() == STR_XML_gcc_defines)
            {
               const xml_node::node_list defines = root_child_element->get_children();
               xml_node::node_list::const_iterator define, define_end = defines.end();
               for(define = defines.begin(); define != define_end; ++define)
               {
                  const auto* define_element = GetPointer<const xml_element>(*define);
                  if(not define_element)
                     continue;
                  if(not define_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  gcc_compiling_parameters += "-D" + define_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_undefines)
            {
               const xml_node::node_list undefines = root_child_element->get_children();
               xml_node::node_list::const_iterator undefine, undefine_end = undefines.end();
               for(undefine = undefines.begin(); undefine != undefine_end; ++undefine)
               {
                  const auto* undefine_element = GetPointer<const xml_element>(*undefine);
                  if(not undefine_element)
                     continue;
                  if(not undefine_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  gcc_compiling_parameters += "-U" + undefine_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_warnings)
            {
               const xml_node::node_list warnings = root_child_element->get_children();
               xml_node::node_list::const_iterator warning, warning_end = warnings.end();
               for(warning = warnings.begin(); warning != warning_end; ++warning)
               {
                  const auto* warning_element = GetPointer<const xml_element>(*warning);
                  if(not warning_element)
                     continue;
                  if(not warning_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  gcc_compiling_parameters += "-W" + warning_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_includes)
            {
               const xml_node::node_list includes = root_child_element->get_children();
               xml_node::node_list::const_iterator include, include_end = includes.end();
               for(include = includes.begin(); include != include_end; ++include)
               {
                  const auto* include_element = GetPointer<const xml_element>(*include);
                  if(not include_element)
                     continue;
                  if(not include_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  gcc_compiling_parameters += "-I" + include_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_libraries)
            {
               const xml_node::node_list libraries = root_child_element->get_children();
               xml_node::node_list::const_iterator library, library_end = libraries.end();
               for(library = libraries.begin(); library != library_end; ++library)
               {
                  const auto* library_element = GetPointer<const xml_element>(*library);
                  if(not library_element)
                     continue;
                  if(not library_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  gcc_linking_parameters += "-l" + library_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_library_directories)
            {
               const xml_node::node_list library_directories = root_child_element->get_children();
               xml_node::node_list::const_iterator library_directory, library_directory_end = library_directories.end();
               for(library_directory = library_directories.begin(); library_directory != library_directory_end; ++library_directory)
               {
                  const auto* library_directory_element = GetPointer<const xml_element>(*library_directory);
                  if(not library_directory_element)
                     continue;
                  if(not library_directory_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  gcc_compiling_parameters += "-L" + library_directory_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
         }
      }
   }
   catch(const char* msg)
   {
      THROW_ERROR("Error " + std::string(msg) + " during reading of gcc configuration from " + file_name);
   }
   catch(const std::string& msg)
   {
      THROW_ERROR("Error " + msg + " during reading of gcc configuration from " + file_name);
   }
   catch(const std::exception& ex)
   {
      THROW_ERROR("Error " + std::string(ex.what()) + " during reading of gcc configuration from " + file_name);
   }
   catch(...)
   {
      THROW_ERROR("Unknown exception during reading of gcc configuration from " + file_name);
   }
}

void GccWrapper::WriteXml(const std::string& file_name) const
{
   xml_document document;
   xml_element* root = document.create_root_node(STR_XML_gcc_root);
   xml_element* optimizations = root->add_child_element(STR_XML_gcc_optimizations);
   xml_element* parameter_values_xml = optimizations->add_child_element(STR_XML_gcc_parameter_values);
   std::map<std::string, int>::const_iterator parameter_value, parameter_value_end = this->parameter_values.end();
   for(parameter_value = this->parameter_values.begin(); parameter_value != parameter_value_end; ++parameter_value)
   {
      xml_element* parameter_value_xml = parameter_values_xml->add_child_element(STR_XML_gcc_parameter_value);
      parameter_value_xml->set_attribute(STR_XML_gcc_name, parameter_value->first);
      parameter_value_xml->set_attribute(STR_XML_gcc_value, boost::lexical_cast<std::string>(parameter_value->second));
   }
   xml_element* optimization_flags_xml = optimizations->add_child_element(STR_XML_gcc_parameter_values);
   std::map<std::string, bool>::const_iterator optimization_flag, optimization_flag_end = this->optimization_flags.end();
   for(optimization_flag = this->optimization_flags.begin(); optimization_flag != optimization_flag_end; ++optimization_flag)
   {
      xml_element* optimization_flag_xml = optimization_flags_xml->add_child_element(STR_XML_gcc_optimization_flag);
      optimization_flag_xml->set_attribute(STR_XML_gcc_name, optimization_flag->first);
      optimization_flag_xml->set_attribute(STR_XML_gcc_value, boost::lexical_cast<std::string>(optimization_flag->second));
   }
   xml_element* optimization_values_xml = optimizations->add_child_element(STR_XML_gcc_parameter_values);
   std::map<std::string, int>::const_iterator optimization_value, optimization_value_end = this->optimization_values.end();
   for(optimization_value = this->optimization_values.begin(); optimization_value != optimization_value_end; ++optimization_value)
   {
      xml_element* optimization_value_xml = optimization_values_xml->add_child_element(STR_XML_gcc_optimization_value);
      optimization_value_xml->set_attribute(STR_XML_gcc_name, optimization_value->first);
      optimization_value_xml->set_attribute(STR_XML_gcc_value, boost::lexical_cast<std::string>(optimization_value->second));
   }
   document.write_to_file_formatted(file_name);
}

const std::string GccWrapper::AddSourceCodeIncludes(const std::list<std::string>& source_files) const
{
   std::string includes;
   /// Adding includes of original source code files
   for(const auto& source_file : source_files)
   {
      boost::filesystem::path absolute_path = boost::filesystem::system_complete(source_file);
      std::string new_path = "-iquote " + absolute_path.branch_path().string() + " ";
#ifdef _WIN32
      boost::replace_all(new_path, "\\", "/");
#endif
      if(gcc_compiling_parameters.find(new_path) == std::string::npos and includes.find(new_path) == std::string::npos)
         includes += new_path;
   }
   return includes;
}

size_t GccWrapper::ConvertVersion(const std::string& version)
{
   size_t ret_value = 0;
   std::vector<std::string> version_tokens = SplitString(version, ".");
   for(size_t index = version_tokens.size(); index > 0; index--)
   {
      const auto shifter = static_cast<size_t>(pow(100, static_cast<double>(version_tokens.size() - index)));
      const auto value = boost::lexical_cast<size_t>(version_tokens[index - 1]);
      ret_value += (value * shifter);
   }
   return ret_value;
}

std::string GccWrapper::clang_recipes(const GccWrapper_OptimizationSet
#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
                                          optimization_level
#endif
                                      ,
                                      const GccWrapper_CompilerTarget
#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
                                          compiler
#endif
                                      ,
                                      const std::string&
#ifndef _WIN32
                                          expandMemOps_plugin_obj
#endif
                                      ,
                                      const std::string&
#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
                                          expandMemOps_plugin_name
#endif
                                      ,
                                      const std::string&
#ifndef _WIN32
                                          GepiCanon_plugin_obj
#endif
                                      ,
                                      const std::string&
#if HAVE_I386_CLANG4_COMPILER
                                          //|| HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
                                          GepiCanon_plugin_name
#endif
                                      ,
                                      const std::string&
#ifndef _WIN32
                                          CSROA_plugin_obj
#endif
                                      ,
                                      const std::string&
#if HAVE_I386_CLANG4_COMPILER
                                          //|| HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER || HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER
                                          CSROA_plugin_name
#endif
                                      ,
                                      const std::string& fname)
{
   std::string recipe = "";
#ifndef _WIN32
   auto renamed_pluginEMO = expandMemOps_plugin_obj;
   boost::replace_all(renamed_pluginEMO, ".so", "_opt.so");
   recipe += " -load=" + renamed_pluginEMO;
#endif
#ifndef _WIN32
   if(!GepiCanon_plugin_obj.empty())
   {
      auto renamed_pluginGC = GepiCanon_plugin_obj;
      boost::replace_all(renamed_pluginGC, ".so", "_opt.so");
      recipe += " -load=" + renamed_pluginGC;
   }
#endif

   if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1
#ifndef _WIN32
      && !CSROA_plugin_obj.empty()
#endif
   )
   {
#ifndef _WIN32
      auto renamed_pluginCSROA = CSROA_plugin_obj;
      boost::replace_all(renamed_pluginCSROA, ".so", "_opt.so");
      recipe += " -load=" + renamed_pluginCSROA;
#endif
      recipe += " -panda-KN=" + fname;
      if(Param->IsParameter("max-CSROA"))
      {
         auto max_CSROA = Param->GetParameter<int>("max-CSROA");
         recipe += " -csroa-max-transformations=" + STR(max_CSROA);
      }
   }
#if HAVE_I386_CLANG4_COMPILER
   if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG4)
   {
      if(optimization_level == GccWrapper_OptimizationSet::O2 || optimization_level == GccWrapper_OptimizationSet::O3)
      {
         std::string complex_recipe;
         complex_recipe += " -tti "
                           "-targetlibinfo "
                           "-tbaa -scoped-noalias -assumption-cache-tracker -profile-summary-info -forceattrs -inferattrs "
                           "-" +
                           expandMemOps_plugin_name +
                           " "
                           "-domtree "
                           "-mem2reg ";
         if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1
#ifndef _WIN32
            && !GepiCanon_plugin_obj.empty() && !CSROA_plugin_obj.empty()
#endif
         )
            complex_recipe += "-" + GepiCanon_plugin_name +
                              "PS "
                              "-" +
                              GepiCanon_plugin_name +
                              "COL "
                              "-" +
                              GepiCanon_plugin_name +
                              "BVR "
                              "-loops -loop-simplify -lcssa-verification -lcssa -basicaa -aa -scalar-evolution -loop-unroll "
                              "-" +
                              CSROA_plugin_name +
                              "FV "
                              "-ipsccp -globaldce -domtree -mem2reg -deadargelim -basiccg -argpromotion -domtree -loops -loop-simplify -lcssa-verification -lcssa -basicaa -aa -scalar-evolution -loop-unroll -simplifycfg ";
         if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1
#ifndef _WIN32
            && !GepiCanon_plugin_obj.empty() && !CSROA_plugin_obj.empty()
#endif
         )
            complex_recipe += "-" + expandMemOps_plugin_name +
                              " "
                              "-" +
                              GepiCanon_plugin_name +
                              "PS "
                              "-" +
                              GepiCanon_plugin_name +
                              "COL "
                              "-" +
                              GepiCanon_plugin_name +
                              "BVR "
                              "-" +
                              CSROA_plugin_name + "D ";
         complex_recipe += "-ipsccp -globalopt -dse -loop-unroll "
                           "-instcombine "
                           "-libcalls-shrinkwrap "
                           "-tailcallelim "
                           "-simplifycfg "
                           "-reassociate "
                           "-domtree "
                           "-loops "
                           "-loop-simplify "
                           "-lcssa-verification "
                           "-lcssa "
                           "-basicaa "
                           "-aa "
                           "-scalar-evolution "
                           "-loop-rotate "
                           "-licm "
                           "-loop-unswitch "
                           "-simplifycfg "
                           "-domtree "
                           "-basicaa "
                           "-aa "
                           " -dse -loop-unroll "
                           "-instcombine "
                           "-loops "
                           "-loop-simplify "
                           "-lcssa-verification "
                           "-lcssa "
                           "-scalar-evolution "
                           "-indvars "
                           "-loop-idiom "
                           "-loop-deletion "
                           "-loop-unroll "
                           "-mldst-motion "
                           "-aa "
                           "-memdep "
                           "-lazy-branch-prob "
                           "-lazy-block-freq "
                           "-opt-remark-emitter "
                           "-gvn "
                           "-basicaa "
                           "-aa "
                           "-memdep "
                           "-memcpyopt "
                           "-sccp "
                           "-domtree "
                           "-demanded-bits "
                           "-bdce "
                           "-basicaa "
                           "-aa "
                           " -dse -loop-unroll "
                           "-instcombine "
                           "-lazy-value-info "
                           "-jump-threading "
                           "-correlated-propagation "
                           "-domtree "
                           "-basicaa "
                           "-aa "
                           "-memdep "
                           "-dse "
                           "-loops "
                           "-loop-simplify "
                           "-lcssa-verification "
                           "-lcssa "
                           "-aa "
                           "-scalar-evolution "
                           "-licm "
                           "-postdomtree "
                           "-adce "
                           "-simplifycfg "
                           "-domtree "
                           "-basicaa "
                           "-aa "
                           " -loop-unroll "
                           "-instcombine "
                           "-barrier "
                           "-elim-avail-extern "
                           "-basiccg "
                           "-rpo-functionattrs "
                           "-globals-aa "
                           "-float2int "
                           "-domtree "
                           "-loops "
                           "-loop-simplify "
                           "-lcssa-verification "
                           "-lcssa "
                           "-basicaa "
                           "-aa "
                           "-scalar-evolution "
                           "-loop-rotate "
                           "-loop-accesses "
                           "-lazy-branch-prob "
                           "-lazy-block-freq "
                           "-opt-remark-emitter "
                           "-loop-distribute "
                           "-loop-simplify "
                           "-lcssa-verification "
                           "-lcssa "
                           "-branch-prob "
                           "-block-freq "
                           "-scalar-evolution "
                           "-basicaa "
                           "-aa "
                           "-loop-accesses "
                           "-demanded-bits "
                           "-lazy-branch-prob "
                           "-lazy-block-freq "
                           "-opt-remark-emitter "
                           "-loop-vectorize "
                           "-loop-simplify "
                           "-scalar-evolution "
                           "-aa "
                           "-loop-accesses "
                           "-loop-load-elim "
                           "-basicaa "
                           "-aa "
                           " -dse -loop-unroll "
                           "-instcombine "
                           "-simplifycfg "
                           "-domtree "
                           "-basicaa "
                           "-aa "
                           " -dse -loop-unroll "
                           "-instcombine "
                           "-loops "
                           "-loop-simplify "
                           "-lcssa-verification "
                           "-lcssa "
                           "-scalar-evolution "
                           "-loop-unroll ";
         if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1
#ifndef _WIN32
            && !GepiCanon_plugin_obj.empty() && !CSROA_plugin_obj.empty()
#endif
         )
            complex_recipe += " -" + expandMemOps_plugin_name + " -" + CSROA_plugin_name + "WI ";
         complex_recipe += "-domtree -basicaa -aa -memdep -dse -aa -memoryssa -early-cse-memssa -constprop -ipsccp -globaldce -domtree -mem2reg -deadargelim -basiccg -argpromotion -domtree -loops -loop-simplify -lcssa-verification -lcssa -basicaa -aa "
                           "-scalar-evolution -loop-unroll "
                           " -dse -loop-unroll "
                           "-instcombine "
                           "-loop-simplify "
                           "-lcssa-verification "
                           "-lcssa "
                           "-scalar-evolution "
                           "-licm "
                           "-alignment-from-assumptions "
                           "-strip-dead-prototypes "
                           "-globaldce "
                           "-constmerge "
                           "-domtree "
                           "-loops "
                           "-branch-prob "
                           "-block-freq "
                           "-loop-simplify "
                           "-lcssa-verification "
                           "-lcssa "
                           "-basicaa "
                           "-aa "
                           "-scalar-evolution "
                           "-branch-prob "
                           "-block-freq "
                           "-loop-sink "
                           "-instsimplify ";
         // complex_recipe += complex_recipe;
         recipe += complex_recipe;
      }
      else
      {
         const auto opt_level = optimization_level == GccWrapper_OptimizationSet::O0 ? "1" : WriteOptimizationLevel(optimization_level);
         recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer ";
         recipe += " -" + expandMemOps_plugin_name + " -loop-unroll -simplifycfg ";
      }
   }
   else
#endif
#if HAVE_I386_CLANG5_COMPILER
       if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG5)
   {
      const auto opt_level = optimization_level == GccWrapper_OptimizationSet::O0 ? "1" : WriteOptimizationLevel(optimization_level);
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer ";
      recipe += " -" + expandMemOps_plugin_name + " -loop-unroll -simplifycfg ";
   }
   else
#endif
#if HAVE_I386_CLANG6_COMPILER
       if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG6)
   {
      const auto opt_level = optimization_level == GccWrapper_OptimizationSet::O0 ? "1" : WriteOptimizationLevel(optimization_level);
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer ";
      recipe += " -" + expandMemOps_plugin_name + " -loop-unroll -simplifycfg ";
   }
   else
#endif
#if HAVE_I386_CLANG7_COMPILER
       if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG7)
   {
      const auto opt_level = optimization_level == GccWrapper_OptimizationSet::O0 ? "1" : WriteOptimizationLevel(optimization_level);
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer ";
      recipe += " -" + expandMemOps_plugin_name;
      /*
            recipe += " -" + GepiCanon_plugin_name +
                      "PS "
                      "-" +
                      GepiCanon_plugin_name +
                      "COL "
                      "-" +
                      GepiCanon_plugin_name +
                      "BVR ";
      */
      recipe += " -loop-unroll -simplifycfg ";
   }
   else
#endif
#if HAVE_I386_CLANG8_COMPILER
       if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG8)
   {
      const auto opt_level = optimization_level == GccWrapper_OptimizationSet::O0 ? "1" : WriteOptimizationLevel(optimization_level);
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer ";
      recipe += " -" + expandMemOps_plugin_name;
      /*
            recipe += " -" + GepiCanon_plugin_name +
                      "PS "
                      "-" +
                      GepiCanon_plugin_name +
                      "COL "
                      "-" +
                      GepiCanon_plugin_name +
                      "BVR ";
      */
      recipe += " -loop-unroll -simplifycfg ";
   }
   else
#endif
#if HAVE_I386_CLANG9_COMPILER
       if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG9)
   {
      const auto opt_level = optimization_level == GccWrapper_OptimizationSet::O0 ? "1" : WriteOptimizationLevel(optimization_level);
      recipe += " -O" + opt_level + " -disable-slp-vectorization -scalarizer ";
      recipe += " -" + expandMemOps_plugin_name;
      /*
            recipe += " -" + GepiCanon_plugin_name +
                      "PS "
                      "-" +
                      GepiCanon_plugin_name +
                      "COL "
                      "-" +
                      GepiCanon_plugin_name +
                      "BVR ";
      */
      recipe += " -loop-unroll -simplifycfg ";
   }
   else
#endif
#if HAVE_I386_CLANG10_COMPILER
       if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG10)
   {
      const auto opt_level = optimization_level == GccWrapper_OptimizationSet::O0 ? "1" : WriteOptimizationLevel(optimization_level);
      recipe += " -O" + opt_level + " -disable-slp-vectorization -scalarizer ";
      recipe += " -" + expandMemOps_plugin_name;
      /*
            recipe += " -" + GepiCanon_plugin_name +
                      "PS "
                      "-" +
                      GepiCanon_plugin_name +
                      "COL "
                      "-" +
                      GepiCanon_plugin_name +
                      "BVR ";
      */
      recipe += " -loop-unroll -simplifycfg ";
   }
   else
#endif
#if HAVE_I386_CLANG11_COMPILER
       if(compiler == GccWrapper_CompilerTarget::CT_I386_CLANG11)
   {
      const auto opt_level = optimization_level == GccWrapper_OptimizationSet::O0 ? "1" : WriteOptimizationLevel(optimization_level);
      recipe += " -O" + opt_level + " --disable-vector-combine -scalarizer ";
      recipe += " -" + expandMemOps_plugin_name;
      /*
            recipe += " -" + GepiCanon_plugin_name +
                      "PS "
                      "-" +
                      GepiCanon_plugin_name +
                      "COL "
                      "-" +
                      GepiCanon_plugin_name +
                      "BVR ";
      */
      recipe += " -loop-unroll -simplifycfg ";
   }
   else
#endif
      THROW_ERROR("Clang compiler not yet supported");
   return " " + recipe + " ";
}

void GccWrapper::CheckGccCompatibleVersion(const std::string& gcc_version, const std::string& plugin_version)
{
   current_gcc_version = gcc_version;
   current_plugin_version = plugin_version;
   const size_t gcc_version_number = ConvertVersion(gcc_version);
   const size_t plugin_version_number = ConvertVersion(plugin_version);
   const size_t minimum_plugin_version_number = ConvertVersion(STR_CST_gcc_min_plugin_version);
   const size_t maximum_plugin_version_number = ConvertVersion(STR_CST_gcc_max_plugin_version);
   if(plugin_version_number < minimum_plugin_version_number or plugin_version_number > maximum_plugin_version_number)
   {
      THROW_ERROR("Plugin version not correct. Plugin version supported has to be in this range: [" + std::string(STR_CST_gcc_min_plugin_version) + "-" + std::string(STR_CST_gcc_max_plugin_version) + "]");
   }
   if(std::string(STR_CST_gcc_supported_versions).find(gcc_version) != std::string::npos)
      return;
   if(gcc_version_number < ConvertVersion(STR_CST_gcc_first_not_supported))
   {
      THROW_WARNING("GCC/CLANG " + gcc_version + " has not been tested with the PandA framework");
      return;
   }
   THROW_ERROR("GCC/CLANG " + gcc_version + " is not supported by this version of the PandA framework");
}

size_t GccWrapper::CGetPointerSize(const ParameterConstRef parameters)
{
   const std::string gcc_m32_mx32 = parameters->getOption<std::string>(OPT_gcc_m32_mx32);
   if(gcc_m32_mx32 == "-m32" or gcc_m32_mx32 == "-m32 -mno-sse2")
   {
      return 32;
   }
   else if(gcc_m32_mx32 == "-mx32")
   {
      return 32;
   }
   else if(gcc_m32_mx32 == "-m64")
   {
      return 64;
   }
   else
   {
      THROW_ERROR("-m parameter not supported: " + gcc_m32_mx32);
   }
   return 0;
}
