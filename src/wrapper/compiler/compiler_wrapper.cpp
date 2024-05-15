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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file compiler_wrapper.hpp
 * @brief Implementation of the wrapper to Gcc for C sources.
 *
 * Implementation of the methods for the object for invoke the GCC compiler from sources and create the dump.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#include "compiler_wrapper.hpp"

#include "Parameter.hpp"
#include "compiler_constants.hpp"
#include "compiler_xml.hpp"
#include "cpu_stats.hpp"
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "file_IO_constants.hpp"
#include "hls_step.hpp"
#include "parse_tree.hpp"
#include "polixml.hpp"
#include "string_manipulation.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "config_CLANG_PLUGIN_DIR.hpp"
#include "config_EXTRA_CLANGPP_COMPILER_OPTION.hpp"
#include "config_GCC_PLUGIN_DIR.hpp"
#include "config_HAVE_I386_CLANG10_COMPILER.hpp"
#include "config_HAVE_I386_CLANG10_M32.hpp"
#include "config_HAVE_I386_CLANG10_M64.hpp"
#include "config_HAVE_I386_CLANG10_MX32.hpp"
#include "config_HAVE_I386_CLANG11_COMPILER.hpp"
#include "config_HAVE_I386_CLANG11_M32.hpp"
#include "config_HAVE_I386_CLANG11_M64.hpp"
#include "config_HAVE_I386_CLANG11_MX32.hpp"
#include "config_HAVE_I386_CLANG12_COMPILER.hpp"
#include "config_HAVE_I386_CLANG12_M32.hpp"
#include "config_HAVE_I386_CLANG12_M64.hpp"
#include "config_HAVE_I386_CLANG12_MX32.hpp"
#include "config_HAVE_I386_CLANG13_COMPILER.hpp"
#include "config_HAVE_I386_CLANG13_M32.hpp"
#include "config_HAVE_I386_CLANG13_M64.hpp"
#include "config_HAVE_I386_CLANG13_MX32.hpp"
#include "config_HAVE_I386_CLANG16_COMPILER.hpp"
#include "config_HAVE_I386_CLANG16_M32.hpp"
#include "config_HAVE_I386_CLANG16_M64.hpp"
#include "config_HAVE_I386_CLANG16_MX32.hpp"
#include "config_HAVE_I386_CLANG4_COMPILER.hpp"
#include "config_HAVE_I386_CLANG4_M32.hpp"
#include "config_HAVE_I386_CLANG4_M64.hpp"
#include "config_HAVE_I386_CLANG4_MX32.hpp"
#include "config_HAVE_I386_CLANG5_COMPILER.hpp"
#include "config_HAVE_I386_CLANG5_M32.hpp"
#include "config_HAVE_I386_CLANG5_M64.hpp"
#include "config_HAVE_I386_CLANG5_MX32.hpp"
#include "config_HAVE_I386_CLANG6_COMPILER.hpp"
#include "config_HAVE_I386_CLANG6_M32.hpp"
#include "config_HAVE_I386_CLANG6_M64.hpp"
#include "config_HAVE_I386_CLANG6_MX32.hpp"
#include "config_HAVE_I386_CLANG7_COMPILER.hpp"
#include "config_HAVE_I386_CLANG7_M32.hpp"
#include "config_HAVE_I386_CLANG7_M64.hpp"
#include "config_HAVE_I386_CLANG7_MX32.hpp"
#include "config_HAVE_I386_CLANG8_COMPILER.hpp"
#include "config_HAVE_I386_CLANG8_M32.hpp"
#include "config_HAVE_I386_CLANG8_M64.hpp"
#include "config_HAVE_I386_CLANG8_MX32.hpp"
#include "config_HAVE_I386_CLANG9_COMPILER.hpp"
#include "config_HAVE_I386_CLANG9_M32.hpp"
#include "config_HAVE_I386_CLANG9_M64.hpp"
#include "config_HAVE_I386_CLANG9_MX32.hpp"
#include "config_HAVE_I386_CLANGVVD_COMPILER.hpp"
#include "config_HAVE_I386_CLANGVVD_M32.hpp"
#include "config_HAVE_I386_CLANGVVD_M64.hpp"
#include "config_HAVE_I386_CLANGVVD_MX32.hpp"
#include "config_HAVE_I386_GCC49_COMPILER.hpp"
#include "config_HAVE_I386_GCC49_M32.hpp"
#include "config_HAVE_I386_GCC49_M64.hpp"
#include "config_HAVE_I386_GCC49_MX32.hpp"
#include "config_HAVE_I386_GCC5_COMPILER.hpp"
#include "config_HAVE_I386_GCC5_M32.hpp"
#include "config_HAVE_I386_GCC5_M64.hpp"
#include "config_HAVE_I386_GCC5_MX32.hpp"
#include "config_HAVE_I386_GCC6_COMPILER.hpp"
#include "config_HAVE_I386_GCC6_M32.hpp"
#include "config_HAVE_I386_GCC6_M64.hpp"
#include "config_HAVE_I386_GCC6_MX32.hpp"
#include "config_HAVE_I386_GCC7_COMPILER.hpp"
#include "config_HAVE_I386_GCC7_M32.hpp"
#include "config_HAVE_I386_GCC7_M64.hpp"
#include "config_HAVE_I386_GCC7_MX32.hpp"
#include "config_HAVE_I386_GCC8_COMPILER.hpp"
#include "config_HAVE_I386_GCC8_M32.hpp"
#include "config_HAVE_I386_GCC8_M64.hpp"
#include "config_HAVE_I386_GCC8_MX32.hpp"
#include "config_I386_CLANG10_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG10_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG10_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG10_EXE.hpp"
#include "config_I386_CLANG10_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG10_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG10_SSA_PLUGIN.hpp"
#include "config_I386_CLANG10_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG10_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG10_VERSION.hpp"
#include "config_I386_CLANG11_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG11_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG11_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG11_EXE.hpp"
#include "config_I386_CLANG11_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG11_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG11_SSA_PLUGIN.hpp"
#include "config_I386_CLANG11_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG11_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG11_VERSION.hpp"
#include "config_I386_CLANG12_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG12_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG12_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG12_EXE.hpp"
#include "config_I386_CLANG12_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG12_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG12_SSA_PLUGIN.hpp"
#include "config_I386_CLANG12_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG12_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG12_VERSION.hpp"
#include "config_I386_CLANG13_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG13_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG13_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG13_EXE.hpp"
#include "config_I386_CLANG13_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG13_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG13_SSA_PLUGIN.hpp"
#include "config_I386_CLANG13_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG13_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG13_VERSION.hpp"
#include "config_I386_CLANG16_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG16_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG16_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG16_EXE.hpp"
#include "config_I386_CLANG16_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG16_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG16_SSA_PLUGIN.hpp"
#include "config_I386_CLANG16_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG16_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG16_VERSION.hpp"
#include "config_I386_CLANG4_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG4_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG4_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG4_EXE.hpp"
#include "config_I386_CLANG4_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG4_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG4_SSA_PLUGIN.hpp"
#include "config_I386_CLANG4_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG4_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG4_VERSION.hpp"
#include "config_I386_CLANG5_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG5_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG5_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG5_EXE.hpp"
#include "config_I386_CLANG5_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG5_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG5_SSA_PLUGIN.hpp"
#include "config_I386_CLANG5_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG5_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG5_VERSION.hpp"
#include "config_I386_CLANG6_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG6_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG6_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG6_EXE.hpp"
#include "config_I386_CLANG6_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG6_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG6_SSA_PLUGIN.hpp"
#include "config_I386_CLANG6_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG6_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG6_VERSION.hpp"
#include "config_I386_CLANG7_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG7_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG7_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG7_EXE.hpp"
#include "config_I386_CLANG7_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG7_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG7_SSA_PLUGIN.hpp"
#include "config_I386_CLANG7_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG7_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG7_VERSION.hpp"
#include "config_I386_CLANG8_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG8_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG8_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG8_EXE.hpp"
#include "config_I386_CLANG8_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG8_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG8_SSA_PLUGIN.hpp"
#include "config_I386_CLANG8_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG8_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG8_VERSION.hpp"
#include "config_I386_CLANG9_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANG9_CSROA_PLUGIN.hpp"
#include "config_I386_CLANG9_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANG9_EXE.hpp"
#include "config_I386_CLANG9_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANG9_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANG9_SSA_PLUGIN.hpp"
#include "config_I386_CLANG9_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANG9_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANG9_VERSION.hpp"
#include "config_I386_CLANGPP10_EXE.hpp"
#include "config_I386_CLANGPP11_EXE.hpp"
#include "config_I386_CLANGPP12_EXE.hpp"
#include "config_I386_CLANGPP13_EXE.hpp"
#include "config_I386_CLANGPP16_EXE.hpp"
#include "config_I386_CLANGPP4_EXE.hpp"
#include "config_I386_CLANGPP5_EXE.hpp"
#include "config_I386_CLANGPP6_EXE.hpp"
#include "config_I386_CLANGPP7_EXE.hpp"
#include "config_I386_CLANGPP8_EXE.hpp"
#include "config_I386_CLANGPP9_EXE.hpp"
#include "config_I386_CLANGPPVVD_EXE.hpp"
#include "config_I386_CLANGVVD_ASTANALYZER_PLUGIN.hpp"
#include "config_I386_CLANGVVD_CSROA_PLUGIN.hpp"
#include "config_I386_CLANGVVD_EMPTY_PLUGIN.hpp"
#include "config_I386_CLANGVVD_EXE.hpp"
#include "config_I386_CLANGVVD_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_I386_CLANGVVD_GEPICANON_PLUGIN.hpp"
#include "config_I386_CLANGVVD_SSA_PLUGIN.hpp"
#include "config_I386_CLANGVVD_SSA_PLUGINCPP.hpp"
#include "config_I386_CLANGVVD_TOPFNAME_PLUGIN.hpp"
#include "config_I386_CLANGVVD_VERSION.hpp"
#include "config_I386_CLANG_CPP10_EXE.hpp"
#include "config_I386_CLANG_CPP11_EXE.hpp"
#include "config_I386_CLANG_CPP12_EXE.hpp"
#include "config_I386_CLANG_CPP13_EXE.hpp"
#include "config_I386_CLANG_CPP16_EXE.hpp"
#include "config_I386_CLANG_CPP4_EXE.hpp"
#include "config_I386_CLANG_CPP5_EXE.hpp"
#include "config_I386_CLANG_CPP6_EXE.hpp"
#include "config_I386_CLANG_CPP7_EXE.hpp"
#include "config_I386_CLANG_CPP8_EXE.hpp"
#include "config_I386_CLANG_CPP9_EXE.hpp"
#include "config_I386_CLANG_CPPVVD_EXE.hpp"
#include "config_I386_CPP49_EXE.hpp"
#include "config_I386_CPP5_EXE.hpp"
#include "config_I386_CPP6_EXE.hpp"
#include "config_I386_CPP7_EXE.hpp"
#include "config_I386_CPP8_EXE.hpp"
#include "config_I386_GCC49_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC49_EXE.hpp"
#include "config_I386_GCC49_SSA_PLUGIN.hpp"
#include "config_I386_GCC49_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC49_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC49_VERSION.hpp"
#include "config_I386_GCC5_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC5_EXE.hpp"
#include "config_I386_GCC5_SSA_PLUGIN.hpp"
#include "config_I386_GCC5_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC5_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC5_VERSION.hpp"
#include "config_I386_GCC6_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC6_EXE.hpp"
#include "config_I386_GCC6_SSA_PLUGIN.hpp"
#include "config_I386_GCC6_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC6_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC6_VERSION.hpp"
#include "config_I386_GCC7_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC7_EXE.hpp"
#include "config_I386_GCC7_SSA_PLUGIN.hpp"
#include "config_I386_GCC7_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC7_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC7_VERSION.hpp"
#include "config_I386_GCC8_EMPTY_PLUGIN.hpp"
#include "config_I386_GCC8_EXE.hpp"
#include "config_I386_GCC8_SSA_PLUGIN.hpp"
#include "config_I386_GCC8_SSA_PLUGINCPP.hpp"
#include "config_I386_GCC8_TOPFNAME_PLUGIN.hpp"
#include "config_I386_GCC8_VERSION.hpp"
#include "config_I386_GPP49_EXE.hpp"
#include "config_I386_GPP5_EXE.hpp"
#include "config_I386_GPP6_EXE.hpp"
#include "config_I386_GPP7_EXE.hpp"
#include "config_I386_GPP8_EXE.hpp"
#include "config_I386_LLVM10_LINK_EXE.hpp"
#include "config_I386_LLVM10_OPT_EXE.hpp"
#include "config_I386_LLVM11_LINK_EXE.hpp"
#include "config_I386_LLVM11_OPT_EXE.hpp"
#include "config_I386_LLVM12_LINK_EXE.hpp"
#include "config_I386_LLVM12_OPT_EXE.hpp"
#include "config_I386_LLVM13_LINK_EXE.hpp"
#include "config_I386_LLVM13_OPT_EXE.hpp"
#include "config_I386_LLVM16_LINK_EXE.hpp"
#include "config_I386_LLVM16_OPT_EXE.hpp"
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
#include "config_I386_LLVMVVD_LINK_EXE.hpp"
#include "config_I386_LLVMVVD_OPT_EXE.hpp"
#include "config_NPROFILE.hpp"

#include <cerrno>
#include <list>
#include <random>
#include <regex>
#include <string>
#include <unistd.h>

enum CompilerMode : int
{
   CM_EMPTY = 1 << 0,              // Empty file compilation
   CM_ANALYZER_INTERFACE = 1 << 1, // Enable frontend code analyzer plugins
   CM_ANALYZER_OPTIMIZE = 1 << 2,  // Enable frontend code optimizer plugins
   CM_ANALYZER_ALL = 3 << 1,       // Enable all frontend plugins
   CM_OPT_INTERNALIZE = 1 << 8,    // Enable symbol internalize plugin
   CM_OPT_EXPANDMEMOPS = 1 << 9,   // Enable memory operation optimizer plugin
   CM_OPT_DUMPGIMPLE = 1 << 10,    // Enable IR dump plugin
   CM_OPT_ALL = (7 << 8),          // Enable backend HLS optimization plugins
   CM_LTO_FLAG = 1 << 16,          // Enable LTO optimization flags
   CM_COMPILER_STD = 1 << 24,      // Use default compiler
   CM_COMPILER_OPT = 1 << 25,      // Use compiler optimizer
   CM_COMPILER_LTO = 1 << 26,      // Use compiler linker
};

static std::string __escape_define(const std::string& str)
{
   return std::regex_replace(str, std::regex("([\\(\\) ])"), "\\$1");
}

std::string CompilerWrapper::bambu_ir_info;

CompilerWrapper::CompilerWrapper(const ParameterConstRef _Param, const CompilerWrapper_CompilerTarget _compiler_target,
                                 const CompilerWrapper_OptimizationSet _OS)
    : Param(_Param),
      compiler_target(_compiler_target),
      OS(_OS),
      output_level(_Param->getOption<int>(OPT_output_level)),
      debug_level(_Param->get_class_debug_level("CompilerWrapper"))
{
   InitializeCompilerParameters();
   if(Param->isOption(OPT_gcc_write_xml))
   {
      WriteXml(Param->getOption<std::string>(OPT_gcc_write_xml));
   }
}

// destructor
CompilerWrapper::~CompilerWrapper() = default;

void CompilerWrapper::CompileFile(std::string& input_filename, const std::string& output_filename,
                                  const std::string& parameters_line, int cm, const std::string& costTable)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Compiling " + input_filename);
   THROW_ASSERT(cm == CM_EMPTY || (cm & ~CM_EMPTY) == cm,
                "Empty compilation must not require any other compilation mode.");

   const auto compiler = GetCompiler();
   const auto output_temporary_directory = Param->getOption<std::string>(OPT_output_temporary_directory);
   const auto compiler_output_filename = output_temporary_directory + "/" STR_CST_gcc_output;

   const auto isWholeProgram =
       Param->isOption(OPT_gcc_optimizations) &&
       Param->getOption<std::string>(OPT_gcc_optimizations).find("whole-program") != std::string::npos &&
       Param->getOption<std::string>(OPT_gcc_optimizations).find("no-whole-program") == std::string::npos;
   const auto top_fnames = [&]() -> std::string {
      if(Param->isOption(OPT_top_functions_names))
      {
         auto fnames = Param->getOption<std::string>(OPT_top_functions_names);
         boost::replace_all(fnames, STR_CST_string_separator, ",");
         return fnames;
      }
      return "";
   }();
   THROW_ASSERT(!isWholeProgram || top_fnames == "main", "Unexpected -fwhole-program with non-main top function.");

   std::string real_filename = input_filename;
   std::string load_prefix, command;
   if(cm & CM_COMPILER_OPT)
   {
      THROW_ASSERT(!(cm & CM_ANALYZER_ALL), "Analyzer plugin requires compiler frontend to run");
      load_prefix = compiler.llvm_opt;
   }
   else if(cm & CM_COMPILER_LTO)
   {
      THROW_ASSERT(cm == CM_COMPILER_LTO, "Plugins must not be enabled when linker is required");
      load_prefix = compiler.llvm_link;
   }
   else
   {
      cm |= CM_COMPILER_STD;
      if((cm & CM_ANALYZER_ALL) && !compiler.is_clang)
      {
         THROW_ASSERT(!(cm & CM_OPT_ALL), "Optimization must not be performed with analyze compiler");
         load_prefix = GetAnalyzeCompiler();
      }
      else
      {
         load_prefix = compiler.gcc;
      }
      command += " -c";
      command += " -D__NO_INLINE__"; /// needed to avoid problem with glibc inlines
      command += " " + compiler.extra_options;
#ifdef _WIN32
      if(compiler.is_clang || (cm & CM_ANALYZER_ALL))
      {
         command += " -isystem /mingw64/include -isystem /mingw64/x86_64-w64-mingw32/include -isystem "
                    "/mingw64/include/c++/v1/"; /// needed by clang compiler
      }
#endif

      if(!(Param->getOption<bool>(OPT_compute_size_of)))
      {
         command += " -D\"" STR_CST_panda_sizeof "(arg)=" STR_CST_string_sizeof "(#arg)\"";
      }
      if((Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy)) ||
         (Param->isOption(OPT_discrepancy_hw) && Param->getOption<bool>(OPT_discrepancy_hw)))
      {
         command += " -D__BAMBU_DISCREPANCY__";
      }
   }

   if(cm & CM_LTO_FLAG)
   {
      THROW_ASSERT(cm & CM_COMPILER_STD, "Unexpected compiler type");
      if(compiler.is_clang)
      {
         command += " -flto";
      }
   }

   if(cm & CM_EMPTY)
   {
      if(input_filename == "-")
      {
         THROW_ERROR("Reading from standard input which does not contain any function definition");
      }
      static int empty_counter = 0;
      real_filename = output_temporary_directory + "/empty_" + std::to_string(empty_counter++) + ".c";
      CopyFile(input_filename, real_filename);
      {
         std::ofstream empty_file(real_filename, std::ios_base::app);
         empty_file << "\nvoid __bambu_empty_function__(){}\n";
      }
      if(compiler.is_clang)
      {
         load_prefix += " " + load_plugin(compiler.empty_plugin_obj, compiler_target);
         command +=
             " -mllvm -pandaGE-outputdir=" + output_temporary_directory + " -mllvm -pandaGE-infile=" + real_filename;
      }
      else
      {
         load_prefix += " -fplugin=" + compiler.empty_plugin_obj;
         command += " -fplugin-arg-" + compiler.empty_plugin_name + "-outputdir=" + output_temporary_directory;
      }
   }

   if(cm & CM_ANALYZER_ALL)
   {
      /// remove some extra option not compatible with clang
      boost::replace_all(command, "-mlong-double-64", "");
      if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD)
      {
         boost::replace_all(command, "-fhls", "");
         boost::replace_all(command, "-target fpga64-xilinx-linux-gnu", "");
      }
      load_prefix += " -fplugin=" + compiler.ASTAnalyzer_plugin_obj;
      command += " -Xclang -add-plugin -Xclang " + compiler.ASTAnalyzer_plugin_name;

      if(cm & CM_ANALYZER_INTERFACE)
      {
         command += " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang -action";
         command += " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang analyze";
         command += " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang -outputdir";
         command +=
             " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang " + output_temporary_directory;

         if(Param->isOption(OPT_input_format) &&
            (Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP ||
             Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LLVM_CPP))
         {
            command += " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang -cppflag";
         }
         if(top_fnames.size())
         {
            command += " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang -topfname";
            command += " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang " + top_fnames;
         }
      }
      if(cm & CM_ANALYZER_OPTIMIZE)
      {
         command += " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang -action";
         command += " -Xclang -plugin-arg-" + compiler.ASTAnalyzer_plugin_name + " -Xclang optimize";
      }
   }

   const auto load_and_run_plugin = [&](const std::string& plugin_obj, const std::string& plugin_name) {
      if(cm & CM_COMPILER_STD)
      {
         load_prefix += load_plugin(plugin_obj, compiler_target);
      }
      else
      {
         load_prefix += load_plugin_opt(plugin_obj, compiler_target);
         command += add_plugin_prefix(compiler_target) + plugin_name;
      }
   };
   const auto append_arg = [&](const std::string& arg) {
      if(cm & CM_COMPILER_STD)
      {
         command += " -mllvm " + arg;
      }
      else
      {
         command += " " + arg;
      }
   };
   if((cm & CM_OPT_INTERNALIZE) && top_fnames.size())
   {
      THROW_ASSERT(!(cm & CM_LTO_FLAG), "Internalizing symbols in partial object files is not expected");
      if(compiler.is_clang)
      {
         append_arg("-internalize-outputdir=" + output_temporary_directory);
         append_arg("-panda-TFN=" + top_fnames);
         if(Param->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
         {
            append_arg("-add-noalias");
         }
         const auto extern_symbols = readExternalSymbols(input_filename);
         if(!extern_symbols.empty())
         {
            append_arg("-panda-ESL=" + extern_symbols);
         }
         if(isWholeProgram || !Param->getOption<bool>(OPT_expose_globals))
         {
            append_arg("-panda-Internalize");
         }
         load_and_run_plugin(compiler.topfname_plugin_obj, compiler.topfname_plugin_name);
         if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1 &&
            !compiler.CSROA_plugin_obj.empty() && !compiler.expandMemOps_plugin_obj.empty())
         {
            append_arg("-panda-KN=" + top_fnames);
            if(Param->IsParameter("max-CSROA"))
            {
               auto max_CSROA = Param->GetParameter<int>("max-CSROA");
               append_arg("-csroa-max-transformations=" + STR(max_CSROA));
            }
            load_and_run_plugin(compiler.CSROA_plugin_obj, compiler.CSROA_plugin_name);
         }
      }
      else
      {
         if(!isWholeProgram)
         {
            /// LTO not yet supported with GCC
            load_prefix += " -fplugin=" + compiler.topfname_plugin_obj;
            command += " -fplugin-arg-" + compiler.topfname_plugin_name + "-topfname=" + top_fnames;
         }
      }
   }
   if(cm & CM_OPT_EXPANDMEMOPS)
   {
      if(compiler.is_clang)
      {
         load_and_run_plugin(compiler.expandMemOps_plugin_obj, compiler.expandMemOps_plugin_name);
      }
   }

   command += " " + parameters_line;

   if(cm & CM_OPT_DUMPGIMPLE)
   {
      if(compiler.is_clang)
      {
         append_arg("-panda-outputdir=" + output_temporary_directory);
         append_arg("-panda-infile=" + input_filename);
         append_arg("-panda-cost-table=\"" + costTable + "\"");
         if(top_fnames.size())
         {
            append_arg("-panda-topfname=" + top_fnames);
         }
         load_and_run_plugin(compiler.ssa_plugin_obj, compiler.ssa_plugin_name);
         // command += " -emit-llvm";
      }
      else
      {
         load_prefix += " -fplugin=" + compiler.ssa_plugin_obj;
         command += " -fplugin-arg-" + compiler.ssa_plugin_name + "-outputdir=" + output_temporary_directory;
      }
   }

   const auto _output_filename = output_filename.size() ?
                                     output_filename :
                                     (output_temporary_directory + "/" + unique_path(STR_CST_gcc_obj_file).string());
   command += " -o " + _output_filename;

   if(real_filename == "-" || real_filename == "/dev/null")
   {
      command += " " + real_filename;
   }
   else
   {
      const auto srcs = string_to_container<std::vector<std::string>>(real_filename, STR_CST_string_separator);
      for(const auto& src : srcs)
      {
         const auto extension = std::filesystem::path(src).extension().string();
         /// assembler files are not allowed so in some cases we pass a C file renamed with extension .S
         if(extension == ".S")
         {
            command += "-x c ";
         }
         command += " \"" + src + "\"";
      }
   }
   command = load_prefix + command;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Invoke: " + command);
#if !NPROFILE
   long int gcc_compilation_time = 0;
   if(output_level >= OUTPUT_LEVEL_VERBOSE)
   {
      START_TIME(gcc_compilation_time);
   }
#endif
   int ret = PandaSystem(Param, command, false, compiler_output_filename);
#if !NPROFILE
   if(output_level >= OUTPUT_LEVEL_VERBOSE)
   {
      STOP_TIME(gcc_compilation_time);
      dump_exec_time("Compilation time", gcc_compilation_time);
   }
#endif

   if(output_filename.empty())
   {
      std::remove(_output_filename.c_str());
   }
   if(IsError(ret))
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0, "Error in compilation");
      if(std::filesystem::exists(std::filesystem::path(compiler_output_filename)))
      {
         CopyStdout(compiler_output_filename);
         THROW_ERROR_CODE(COMPILING_EC,
                          "Front-end compiler returns an error during compilation " + std::to_string(errno));
         THROW_ERROR("Front-end compiler returns an error during compilation " + std::to_string(errno));
      }
      else
      {
         THROW_ERROR("Error in front-end compiler invocation");
      }
   }
   else
   {
      if(output_level >= OUTPUT_LEVEL_VERBOSE)
      {
         CopyStdout(compiler_output_filename);
      }
   }
   input_filename = real_filename;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Compiled file");
}

void CompilerWrapper::FillTreeManager(const tree_managerRef TM, std::vector<std::string>& source_files,
                                      const std::string& costTable)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Invoking front-end compiler");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   if(source_files.size() == 0)
   {
      THROW_ERROR("No files specified for parsing");
   }

   const auto compiler = GetCompiler();
   const auto multi_source = source_files.size() > 1;
   const auto enable_LTO = (compiler.is_clang && multi_source);
   const auto compile_only = Param->isOption(OPT_gcc_S) && Param->getOption<bool>(OPT_gcc_S);
   const auto preprocess_only = Param->isOption(OPT_gcc_E) && Param->getOption<bool>(OPT_gcc_E);
   const auto optimization_set = Param->getOption<CompilerWrapper_OptimizationSet>(OPT_compiler_opt_level);
   const auto output_temporary_directory = Param->getOption<std::string>(OPT_output_temporary_directory);

   /// check for aligned option
   if(optimization_set == CompilerWrapper_OptimizationSet::O3 ||
      optimization_set == CompilerWrapper_OptimizationSet::O4 ||
      optimization_set == CompilerWrapper_OptimizationSet::O5)
   {
      if((optimization_flags.find("tree-vectorize") == optimization_flags.end() ||
          optimization_flags.find("tree-vectorize")->second) ||
         (optimization_flags.find("vectorize") == optimization_flags.end() ||
          optimization_flags.find("vectorize")->second) ||
         (optimization_flags.find("slp-vectorize") == optimization_flags.end() ||
          optimization_flags.find("slp-vectorize")->second))
      {
         if(Param->isOption(OPT_aligned_access) && Param->getOption<bool>(OPT_aligned_access))
         {
            THROW_ERROR("Option --aligned-access cannot be used with -O3 or vectorization");
         }
      }
   }

#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER ||    \
    HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER ||    \
    HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER || HAVE_I386_CLANG12_COMPILER || \
    HAVE_I386_CLANG13_COMPILER || HAVE_I386_CLANG16_COMPILER || HAVE_I386_CLANGVVD_COMPILER
   if(Param->IsParameter("disable-pragma-parsing") && Param->GetParameter<int>("disable-pragma-parsing") == 1)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Pragma analysis disabled");
   }
   else if(!compiler.is_clang)
   {
      std::string analyzing_compiling_parameters;
      if(Param->isOption(OPT_gcc_standard))
      {
         analyzing_compiling_parameters += " --std=" + Param->getOption<std::string>(OPT_gcc_standard);
      }
      if(Param->isOption(OPT_gcc_defines))
      {
         const auto defines = Param->getOption<std::list<std::string>>(OPT_gcc_defines);
         for(const auto& define : defines)
         {
            analyzing_compiling_parameters += " -D" + __escape_define(define);
         }
      }
      if(Param->isOption(OPT_gcc_undefines))
      {
         const auto undefines = Param->getOption<std::list<std::string>>(OPT_gcc_undefines);
         for(const auto& undefine : undefines)
         {
            analyzing_compiling_parameters += " -U" + __escape_define(undefine);
         }
      }
      if(Param->isOption(OPT_gcc_warnings))
      {
         const auto warnings = Param->getOption<std::list<std::string>>(OPT_gcc_warnings);
         for(const auto& warning : warnings)
         {
            if(!warning.empty())
            {
               analyzing_compiling_parameters += " -W" + warning;
            }
         }
      }
      if(Param->isOption(OPT_gcc_includes))
      {
         analyzing_compiling_parameters += " " + Param->getOption<std::string>(OPT_gcc_includes);
      }
      for(auto& source_file : source_files)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyze file " + source_file);
         CompileFile(source_file, "", analyzing_compiling_parameters, CM_ANALYZER_INTERFACE, costTable);
      }
   }
#else
   THROW_WARNING("Pragma analysis is not available without Clang/LLVM compiler");
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting compilation of single files");
   const auto compiler_mode = [&]() -> int {
      int flags = CM_COMPILER_STD;
      if(preprocess_only || compile_only)
      {
         return flags;
      }
      if(compiler.is_clang)
      {
         flags |= CM_ANALYZER_ALL;
      }
      if(multi_source)
      {
         flags |= CM_LTO_FLAG;
         if(!enable_LTO)
         {
            flags |= CM_OPT_DUMPGIMPLE;
         }
      }
      else
      {
         flags |= CM_OPT_ALL;
      }
      return flags;
   }();
   std::list<std::string> obj_files;
   THROW_ASSERT(!multi_source || !(compile_only || preprocess_only), "");
   for(auto& source_file : source_files)
   {
      const auto leaf_name = source_file == "-" ? "stdin-" : std::filesystem::path(source_file).filename().string();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Compiling file " + source_file);
      const auto obj_file = ((compile_only || preprocess_only) && Param->isOption(OPT_output_file)) ?
                                Param->getOption<std::string>(OPT_output_file) :
                                unique_path(output_temporary_directory + "/" + leaf_name + ".%%%%%%.o").string();
      CompileFile(source_file, obj_file, frontend_compiler_parameters, compiler_mode, costTable);
      if(enable_LTO)
      {
         obj_files.push_back(obj_file);
      }
      else if(!(compile_only || preprocess_only))
      {
         auto gimple_file = output_temporary_directory + "/" + leaf_name + STR_CST_bambu_ir_suffix;
         if(!std::filesystem::exists(gimple_file))
         {
            CompileFile(source_file, "", frontend_compiler_parameters, CM_EMPTY, costTable);
            // source_file has been changed by previous call to CompileFile
            gimple_file = output_temporary_directory + "/" + std::filesystem::path(source_file).filename().string() +
                          STR_CST_bambu_ir_suffix;
         }
         obj_files.push_back(gimple_file);
      }
   }

   if(enable_LTO)
   {
      const auto leaf_name = std::filesystem::path(source_files.front()).filename().string();
      const auto ext_symbols_filename = output_temporary_directory + "/external-symbols.txt";
      std::string lto_source = container_to_string(obj_files, STR_CST_string_separator);
      std::string lto_obj = output_temporary_directory + "/" + leaf_name + ".lto.bc";
      CompileFile(lto_source, lto_obj, "", CM_COMPILER_LTO, "");

      lto_source = lto_obj;
      lto_obj = output_temporary_directory + "/" + leaf_name + ".lto-opt.bc";
      std::string opt_command = add_plugin_prefix(compiler_target, "1");

      CompileFile(lto_source, lto_obj, opt_command, CM_COMPILER_OPT | CM_OPT_INTERNALIZE, costTable);

      lto_source = lto_obj;
      lto_obj = output_temporary_directory + "/" + leaf_name + ".lto-dump.bc";
      THROW_ASSERT(std::filesystem::exists(ext_symbols_filename), "File not found: " + ext_symbols_filename);
      const auto plugin_prefix = add_plugin_prefix(compiler_target);
      opt_command = " --internalize-public-api-file=" + ext_symbols_filename + " " + plugin_prefix + "internalize " +
                    clang_recipes(optimization_set, "") + " -panda-infile=" + container_to_string(source_files, ",");
      CompileFile(lto_source, lto_obj, opt_command, CM_COMPILER_OPT | CM_OPT_DUMPGIMPLE, costTable);

      const auto gimple_obj = output_temporary_directory + "/" + leaf_name + STR_CST_bambu_ir_suffix;
      if(!std::filesystem::exists(gimple_obj))
      {
         THROW_ERROR("Object file not found: " + gimple_obj);
      }
      tree_managerRef TreeM = ParseTreeFile(Param, gimple_obj);
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
   else if(!Param->isOption(OPT_gcc_E) && !Param->isOption(OPT_gcc_S))
   {
      for(const auto& obj_file : obj_files)
      {
         if(!std::filesystem::exists(obj_file))
         {
            THROW_ERROR("Object file not found: " + obj_file);
         }
         tree_managerRef TreeM = ParseTreeFile(Param, obj_file);

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
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Ended compilation of single files");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Front-end compiler finished");
}

void CompilerWrapper::InitializeCompilerParameters()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Initializing gcc parameters");
   CompilerWrapper_OptimizationSet optimization_set = OS;

   if(Param->isOption(OPT_gcc_read_xml))
   {
      ReadXml(Param->getOption<std::string>(OPT_gcc_read_xml));
   }
   else
   {
      switch(optimization_set)
      {
         case(CompilerWrapper_OptimizationSet::O0):
         case(CompilerWrapper_OptimizationSet::O1):
         case(CompilerWrapper_OptimizationSet::O2):
         case(CompilerWrapper_OptimizationSet::O3):
         case(CompilerWrapper_OptimizationSet::O4):
         case(CompilerWrapper_OptimizationSet::O5):
         case(CompilerWrapper_OptimizationSet::Og):
         case(CompilerWrapper_OptimizationSet::Os):
         case(CompilerWrapper_OptimizationSet::Ofast):
         case(CompilerWrapper_OptimizationSet::Oz):
            frontend_compiler_parameters += (" -O" + WriteOptimizationLevel(optimization_set) + " ");
            break;
         case CompilerWrapper_OptimizationSet::OSF:
            frontend_compiler_parameters += (" -O3 -finline-limit=10000 --param inline-unit-growth=100000 ");
            break;
         case(CompilerWrapper_OptimizationSet::OBAMBU):
            /// Filling optimizations map
            SetCompilerDefault();

            switch(OS)
            {
               case(CompilerWrapper_OptimizationSet::OBAMBU):
                  SetBambuDefault();
                  break;
               case(CompilerWrapper_OptimizationSet::O0):
               case(CompilerWrapper_OptimizationSet::O1):
               case(CompilerWrapper_OptimizationSet::O2):
               case(CompilerWrapper_OptimizationSet::O3):
               case(CompilerWrapper_OptimizationSet::O4):
               case(CompilerWrapper_OptimizationSet::O5):
               case(CompilerWrapper_OptimizationSet::Og):
               case(CompilerWrapper_OptimizationSet::Os):
               case(CompilerWrapper_OptimizationSet::Ofast):
               case(CompilerWrapper_OptimizationSet::Oz):
               case(CompilerWrapper_OptimizationSet::OSF):
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

#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER ||    \
    HAVE_I386_CLANG7_COMPILER || HAVE_I386_CLANG8_COMPILER || HAVE_I386_CLANG9_COMPILER ||    \
    HAVE_I386_CLANG10_COMPILER || HAVE_I386_CLANG11_COMPILER || HAVE_I386_CLANG12_COMPILER || \
    HAVE_I386_CLANG13_COMPILER || HAVE_I386_CLANG16_COMPILER || HAVE_I386_CLANGVVD_COMPILER
            {
               CompilerWrapper_CompilerTarget compiler =
                   Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);

               if(isClangCheck(compiler))
               {
                  /// sanitize CLANG/LLVM options by removing unsupported GCC options
                  if(optimization_flags.find("tree-dominator-opts") != optimization_flags.end())
                  {
                     optimization_flags.erase(optimization_flags.find("tree-dominator-opts"));
                  }
                  if(optimization_flags.find("tree-pre") != optimization_flags.end())
                  {
                     optimization_flags.erase(optimization_flags.find("tree-pre"));
                  }
                  if(optimization_flags.find("tree-pre") != optimization_flags.end())
                  {
                     optimization_flags.erase(optimization_flags.find("tree-pre"));
                  }
                  if(optimization_flags.find("ipa-cp-clone") != optimization_flags.end())
                  {
                     optimization_flags.erase(optimization_flags.find("ipa-cp-clone"));
                  }
                  if(optimization_flags.find("ipa-cp") != optimization_flags.end())
                  {
                     optimization_flags.erase(optimization_flags.find("ipa-cp"));
                  }
               }
            }
#endif

            frontend_compiler_parameters += (" " + WriteOptimizationsString() + " ");

            break;
         default:
         {
            THROW_UNREACHABLE("Unexpected optimization level");
         }
      }
   }

   /// Adding standard
   if(Param->isOption(OPT_gcc_standard))
   {
      auto standard = Param->getOption<std::string>(OPT_gcc_standard);
      frontend_compiler_parameters += "--std=" + standard + " ";
   }

   if(Param->isOption(OPT_gcc_E) && Param->getOption<bool>(OPT_gcc_E))
   {
      frontend_compiler_parameters += "-E ";
   }
   if(Param->isOption(OPT_gcc_S) && Param->getOption<bool>(OPT_gcc_S))
   {
      frontend_compiler_parameters += "-S ";
   }
   /// Adding defines
   if(Param->isOption(OPT_gcc_defines))
   {
      const auto defines = Param->getOption<CustomSet<std::string>>(OPT_gcc_defines);
      for(const auto& define : defines)
      {
         frontend_compiler_parameters += "-D" + __escape_define(define) + " ";
      }
   }

   /// Adding undefines
   if(Param->isOption(OPT_gcc_undefines))
   {
      const auto undefines = Param->getOption<CustomSet<std::string>>(OPT_gcc_undefines);
      for(const auto& undefine : undefines)
      {
         frontend_compiler_parameters += "-U" + __escape_define(undefine) + " ";
      }
   }

   /// Adding warnings
   if(Param->isOption(OPT_gcc_warnings))
   {
      const auto warnings = Param->getOption<CustomSet<std::string>>(OPT_gcc_warnings);
      for(const auto& warning : warnings)
      {
         if(!warning.empty())
         {
            frontend_compiler_parameters += "-W" + warning + " ";
         }
      }
   }
   if(OS == CompilerWrapper_OptimizationSet::OBAMBU)
   {
      frontend_compiler_parameters += "-Wuninitialized ";
   }

   /// Adding includes
   if(Param->isOption(OPT_gcc_includes))
   {
      frontend_compiler_parameters += Param->getOption<std::string>(OPT_gcc_includes) + " ";
   }

   /// Adding libraries
   if(Param->isOption(OPT_gcc_libraries))
   {
      const auto libraries = Param->getOption<CustomSet<std::string>>(OPT_gcc_libraries);
      for(const auto& library : libraries)
      {
         compiler_linking_parameters += "-l" + library + " ";
      }
   }

   /// Adding library directories
   if(Param->isOption(OPT_gcc_library_directories))
   {
      const auto library_directories = Param->getOption<CustomSet<std::string>>(OPT_gcc_library_directories);
      for(const auto& library_directory : library_directories)
      {
         compiler_linking_parameters += "-L" + library_directory + " ";
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Initialized gcc parameters");
}

void CompilerWrapper::SetBambuDefault()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Setting parameters for Bambu tool...");
   const CompilerWrapper_OptimizationSet opt_level =
       Param->getOption<CompilerWrapper_OptimizationSet>(OPT_compiler_opt_level);
   CompilerWrapper_CompilerTarget compiler = Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
   bool is_clang = false;

   /// parameters with enable
   optimization_flags["wrapv"] = true; /// bambu assumes twos complement arithmetic

   if(compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG4 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG5 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG6)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      is_clang = true;
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      if(opt_level != CompilerWrapper_OptimizationSet::O3 && opt_level != CompilerWrapper_OptimizationSet::O4 &&
         opt_level != CompilerWrapper_OptimizationSet::Ofast && opt_level != CompilerWrapper_OptimizationSet::OSF)
      {
         optimization_flags["unroll-loops"] =
             false; // it is preferable to have unrolling disabled by default as with GCC
      }
      return;
   }
   if(compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG7 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG8)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      is_clang = true;
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      return;
   }
   if(compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG9 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG10 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG11 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG12 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG13 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG16)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      is_clang = true;
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      optimization_flags["builtin-bcmp"] = false;
      if(compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG16)
      {
         optimization_flags["fp-contract"] = false;
      }
      return;
   }
   if(compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
      is_clang = true;
      optimization_flags["builtin-memset"] = false;
      optimization_flags["builtin-memcpy"] = false;
      optimization_flags["builtin-memmove"] = false;
      optimization_flags["builtin-bcmp"] = false;
      optimization_flags["hls"] = true;
      return;
   }

   if(!is_clang)
   {
      /// builtin function;
      // optimization_flags["builtin"] = false;
      // optimization_flags["ipa-type-escape"] = true; /// no more supported starting from gcc 4.6.1
      optimization_flags["tree-copy-prop"] =
          true; /// FIXME: this has been always active with gcc >= 4.6; produced c code in bambu for example
      /// gcc_regression_simple/20040307-1.c when disabled
      optimization_flags["ipa-pta"] = true;
   }

   if(compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC49 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC6 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC7 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC8)
   {
      optimization_flags["tree-loop-if-convert"] = true;
      optimization_flags["tree-loop-if-convert-stores"] = true;
      optimization_flags["tree-loop-distribute-patterns"] = false;
      optimization_flags["partial-inlining"] = false; /// artificial functions are not analyzed by the plugin
   }
   if(isGccCheck(compiler))
   {
      if(Param->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
      {
         optimization_flags["tree-vectorize"] = false;
      }
   }

   if(compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC6 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC7 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC8)
   {
      optimization_flags["tree-builtin-call-dce"] = false;
      optimization_flags["ivopts"] = false;
      optimization_flags["ipa-icf"] = false;
   }
   if(opt_level == CompilerWrapper_OptimizationSet::O4)
   {
      optimization_flags["ivopts"] = true;
      optimization_flags["tree-loop-ivcanon"] = true;
      optimization_flags["tree-loop-im"] = true;
      optimization_flags["vect-cost-model"] = false;
      optimization_flags["fast-math"] = true;
   }
   if(opt_level == CompilerWrapper_OptimizationSet::O4 || opt_level == CompilerWrapper_OptimizationSet::O5)
   {
      if(compiler != CompilerWrapper_CompilerTarget::CT_I386_GCC49)
      {
         optimization_flags["tree-loop-linear"] = true;
         optimization_flags["graphite-identity"] = true;
      }
   }
   ///-f option with values
   // optimization_values["tree-parallelize-loops"]=1;///requires tree-loop-optimize
   ///-param option with with values
   if(compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC49 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC6 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC7 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC8)
   {
      parameter_values["tree-reassoc-width"] =
          128; // Set the maximum number of instructions executed in parallel in reassociated tree.
   }
   if(compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC49 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC6 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC7 ||
      compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC8)
   {
      parameter_values["max-completely-peeled-insns"] = 250; // Set the maximum number of insns of a peeled loop.
   }

   if(Param->getOption<bool>(OPT_parse_pragma))
   {
      /// Disable duplication of loop headers to preserve openmp for structure
      optimization_flags["tree-ch"] = false;
   }

   if(Param->getOption<int>(OPT_gcc_openmp_simd) && !is_clang)
   {
      /// Disable optimizations which break loops patterns
      optimization_flags["tree-pre"] = false;
      optimization_flags["tree-vrp"] = false;
      optimization_flags["ivopts"] = false;
      optimization_flags["tree-dominator-opts"] = false;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
}

void CompilerWrapper::SetCompilerDefault()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Setting front-end compiler defaults");
   const auto optimization_set = Param->getOption<CompilerWrapper_OptimizationSet>(OPT_compiler_opt_level);
   optimization_flags["stack-protector"] =
       false; // In Ubuntu 6.10 and later versions this option is enabled by default for C, C++, ObjC, ObjC++

   switch(optimization_set)
   {
      case(CompilerWrapper_OptimizationSet::Os):
      case(CompilerWrapper_OptimizationSet::Og):
      case(CompilerWrapper_OptimizationSet::O1):
      case(CompilerWrapper_OptimizationSet::O2):
      case(CompilerWrapper_OptimizationSet::O3):
      case(CompilerWrapper_OptimizationSet::O4):
      case(CompilerWrapper_OptimizationSet::O5):
      case(CompilerWrapper_OptimizationSet::Ofast):
      case(CompilerWrapper_OptimizationSet::Oz):
      {
         frontend_compiler_parameters += (" -O" + WriteOptimizationLevel(optimization_set) + " ");
         break;
      }
      case CompilerWrapper_OptimizationSet::OSF:
         frontend_compiler_parameters += (" -O3 -finline-limit=10000");
         break;
      case(CompilerWrapper_OptimizationSet::O0):
      {
         frontend_compiler_parameters += " -O1 ";

         if(!isClangCheck(compiler_target))
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
               case(CompilerWrapper_CompilerTarget::CT_I386_GCC49):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_GCC5):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_GCC6):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_GCC7):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_GCC8):
               {
                  optimization_flags["compare-elim"] = false;
                  optimization_flags["ipa-profile"] = false;
                  optimization_flags["tree-bit-ccp"] = false;
                  optimization_flags["tree-copy-prop"] = false;
                  optimization_flags["tree-copyrename"] = false;
                  optimization_flags["tree-slsr"] = false;
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG4):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG5):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG6):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG7):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG8):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG9):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG10):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG11):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG12):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG13):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANG16):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD):
               {
                  break;
               }
               case(CompilerWrapper_CompilerTarget::CT_NO_COMPILER):
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
      case(CompilerWrapper_OptimizationSet::OBAMBU):
      {
         THROW_UNREACHABLE("Unepected optimization level: " + WriteOptimizationLevel(optimization_set));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   /// required by PandA
   if(!isClangCheck(compiler_target))
   {
      optimization_flags["ipa-pure-const"] = true; /// needed to correctly manage global variables
      optimization_flags["tree-dce"] = true;       /// needed to remove unnecessary computations
   }
   else
   {
      optimization_flags["vectorize"] = false;     /// disable vectorization
      optimization_flags["slp-vectorize"] = false; /// disable superword-level parallelism vectorization
   }

   const auto flag_cpp =
       Param->isOption(OPT_input_format) &&
       (Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP ||
        Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LLVM_CPP);
   /// in case we are compiling C++ code
   if(flag_cpp)
   {
      optimization_flags["exceptions"] = false;
      optimization_flags["threadsafe-statics"] = false;
      optimization_flags["use-cxa-atexit"] = false;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set front-end compiler defaults");
}

CompilerWrapper::Compiler CompilerWrapper::GetCompiler() const
{
   Compiler compiler;
#ifndef NDEBUG
   CompilerWrapper_CompilerTarget compatible_compilers =
       Param->getOption<CompilerWrapper_CompilerTarget>(OPT_compatible_compilers);
#endif

   const auto flag_cpp =
       Param->isOption(OPT_input_format) &&
       (Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP ||
        Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LLVM_CPP);

   std::string gcc_extra_options;
   if(Param->isOption(OPT_gcc_extra_options))
   {
      gcc_extra_options = Param->getOption<std::string>(OPT_gcc_extra_options);
   }

   CompilerWrapper_CompilerTarget preferred_compiler;
   if(compiler_target == CompilerWrapper_CompilerTarget::CT_NO_COMPILER)
   {
      preferred_compiler = Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
   }
   else
   {
#ifndef NDEBUG
      const bool debug_condition = static_cast<int>(compiler_target) & static_cast<int>(compatible_compilers);
      THROW_ASSERT(debug_condition,
                   "Required compiler is not among the compatible one: " + STR(static_cast<int>(compiler_target)) +
                       " vs " + STR(static_cast<int>(compatible_compilers)));
#endif
      preferred_compiler = compiler_target;
   }
   const std::string gcc_plugin_dir =
       (Param->isOption(OPT_gcc_plugindir) ? Param->getOption<std::string>(OPT_gcc_plugindir) :
                                             relocate_compiler_path(GCC_PLUGIN_DIR)) +
       "/";
   const std::string clang_plugin_dir =
       (Param->isOption(OPT_gcc_plugindir) ? Param->getOption<std::string>(OPT_gcc_plugindir) + "/../clang_plugin" :
                                             relocate_compiler_path(CLANG_PLUGIN_DIR)) +
       "/";
   const std::string plugin_ext = ".so";

#if HAVE_I386_GCC49_COMPILER || HAVE_I386_GCC5_COMPILER || HAVE_I386_GCC6_COMPILER || HAVE_I386_GCC7_COMPILER || \
    HAVE_I386_GCC8_COMPILER
   auto fillASTAnalyzer_plugin = [&] {
#if HAVE_I386_CLANG16_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG16_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG16_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG13_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG13_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG13_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG12_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG12_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG12_ASTANALYZER_PLUGIN;
#elif HAVE_I386_CLANG11_COMPILER
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
#elif HAVE_I386_CLANGVVD_COMPILER
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANGVVD_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG4_ASTANALYZER_PLUGIN;
#endif
   };
#endif
#if HAVE_I386_GCC49_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC49))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP49_EXE) : relocate_compiler_path(I386_GCC49_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP49_EXE);
      compiler.extra_options = " -mlong-double-64 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() &&
         optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC49_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
#endif
         compiler.extra_options += " -msse2";
      }
      else
      {
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      }
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC49_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC49_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          gcc_plugin_dir + (flag_cpp ? I386_GCC49_SSA_PLUGINCPP : I386_GCC49_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC49_SSA_PLUGINCPP : I386_GCC49_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC49_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC49_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
   }
#endif
#if HAVE_I386_GCC5_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC5))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP5_EXE) : relocate_compiler_path(I386_GCC5_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP5_EXE);
      compiler.extra_options = " -mlong-double-64 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() &&
         optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC5_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
#endif
         compiler.extra_options += " -msse2";
      }
      else
      {
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      }
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC5_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC5_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          gcc_plugin_dir + (flag_cpp ? I386_GCC5_SSA_PLUGINCPP : I386_GCC5_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC5_SSA_PLUGINCPP : I386_GCC5_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC5_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC5_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
   }
#endif

#if HAVE_I386_GCC6_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC6))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP6_EXE) : relocate_compiler_path(I386_GCC6_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP6_EXE);
      compiler.extra_options = " -mlong-double-64 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() &&
         optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC6_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
#endif
         compiler.extra_options += " -msse2";
      }
      else
      {
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      }
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC6_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC6_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          gcc_plugin_dir + (flag_cpp ? I386_GCC6_SSA_PLUGINCPP : I386_GCC6_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC6_SSA_PLUGINCPP : I386_GCC6_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC6_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC6_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
   }
#endif

#if HAVE_I386_GCC7_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC7))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP7_EXE) : relocate_compiler_path(I386_GCC7_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP7_EXE);
      compiler.extra_options = " -mlong-double-64 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() &&
         optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC7_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
#endif
         compiler.extra_options += " -msse2";
      }
      else
      {
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      }
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC7_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC7_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          gcc_plugin_dir + (flag_cpp ? I386_GCC7_SSA_PLUGINCPP : I386_GCC7_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC7_SSA_PLUGINCPP : I386_GCC7_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC7_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC7_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
   }
#endif

#if HAVE_I386_GCC8_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC8))
   {
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_GPP8_EXE) : relocate_compiler_path(I386_GCC8_EXE);
      compiler.cpp = relocate_compiler_path(I386_CPP8_EXE);
      compiler.extra_options = " -mlong-double-64 " + gcc_extra_options;
      if(optimization_flags.find("tree-vectorize") != optimization_flags.end() &&
         optimization_flags.find("tree-vectorize")->second)
      {
#if HAVE_I386_GCC8_MX32
         compiler.extra_options += " -mx32";
#else
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
#endif
         compiler.extra_options += " -msse2";
      }
      else
      {
         compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      }
      compiler.empty_plugin_obj = gcc_plugin_dir + I386_GCC8_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_GCC8_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          gcc_plugin_dir + (flag_cpp ? I386_GCC8_SSA_PLUGINCPP : I386_GCC8_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_GCC8_SSA_PLUGINCPP : I386_GCC8_SSA_PLUGIN);
      compiler.topfname_plugin_obj = gcc_plugin_dir + I386_GCC8_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_GCC8_TOPFNAME_PLUGIN;
      fillASTAnalyzer_plugin();
   }
#endif

#if HAVE_I386_CLANG4_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG4))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP4_EXE) : relocate_compiler_path(I386_CLANG4_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP4_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG4_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG4_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG4_SSA_PLUGINCPP : I386_CLANG4_SSA_PLUGIN) + plugin_ext;
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
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG5))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP5_EXE) : relocate_compiler_path(I386_CLANG5_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP5_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG5_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG5_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG5_SSA_PLUGINCPP : I386_CLANG5_SSA_PLUGIN) + plugin_ext;
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
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG6))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP6_EXE) : relocate_compiler_path(I386_CLANG6_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP6_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG6_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG6_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG6_SSA_PLUGINCPP : I386_CLANG6_SSA_PLUGIN) + plugin_ext;
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
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG7))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP7_EXE) : relocate_compiler_path(I386_CLANG7_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP7_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG7_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG7_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG7_SSA_PLUGINCPP : I386_CLANG7_SSA_PLUGIN) + plugin_ext;
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
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG8))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP8_EXE) : relocate_compiler_path(I386_CLANG8_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP8_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG8_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG8_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG8_SSA_PLUGINCPP : I386_CLANG8_SSA_PLUGIN) + plugin_ext;
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
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG9))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP9_EXE) : relocate_compiler_path(I386_CLANG9_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP9_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG9_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG9_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG9_SSA_PLUGINCPP : I386_CLANG9_SSA_PLUGIN) + plugin_ext;
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
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG10))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP10_EXE) : relocate_compiler_path(I386_CLANG10_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP10_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG10_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG10_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG10_SSA_PLUGINCPP : I386_CLANG10_SSA_PLUGIN) + plugin_ext;
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
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG11))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP11_EXE) : relocate_compiler_path(I386_CLANG11_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP11_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG11_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG11_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG11_SSA_PLUGINCPP : I386_CLANG11_SSA_PLUGIN) + plugin_ext;
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

#if HAVE_I386_CLANG12_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG12))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP12_EXE) : relocate_compiler_path(I386_CLANG12_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP12_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG12_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG12_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG12_SSA_PLUGINCPP : I386_CLANG12_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG12_SSA_PLUGINCPP : I386_CLANG12_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG12_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG12_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG12_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG12_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG12_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG12_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG12_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG12_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG12_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG12_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM12_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM12_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANG13_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG13))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP13_EXE) : relocate_compiler_path(I386_CLANG13_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP13_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG13_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG13_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG13_SSA_PLUGINCPP : I386_CLANG13_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG13_SSA_PLUGINCPP : I386_CLANG13_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG13_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG13_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG13_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG13_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG13_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG13_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG13_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG13_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG13_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG13_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM13_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM13_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANG16_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG16))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPP16_EXE) : relocate_compiler_path(I386_CLANG16_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPP16_EXE);
      compiler.extra_options = gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANG16_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANG16_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANG16_SSA_PLUGINCPP : I386_CLANG16_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANG16_SSA_PLUGINCPP : I386_CLANG16_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANG16_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANG16_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANG16_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANG16_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANG16_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANG16_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANG16_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANG16_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANG16_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANG16_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVM16_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVM16_OPT_EXE);
   }
#endif

#if HAVE_I386_CLANGVVD_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD))
   {
      compiler.is_clang = true;
      compiler.gcc = flag_cpp ? relocate_compiler_path(I386_CLANGPPVVD_EXE) : relocate_compiler_path(I386_CLANGVVD_EXE);
      compiler.cpp = relocate_compiler_path(I386_CLANG_CPPVVD_EXE);
      compiler.extra_options = " -D__VIVADO__ " + gcc_extra_options + (flag_cpp ? EXTRA_CLANGPP_COMPILER_OPTION : "");
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_gcc_m_env);
      compiler.extra_options += " -target fpga64-xilinx-linux-gnu";
      compiler.empty_plugin_obj = clang_plugin_dir + I386_CLANGVVD_EMPTY_PLUGIN + plugin_ext;
      compiler.empty_plugin_name = I386_CLANGVVD_EMPTY_PLUGIN;
      compiler.ssa_plugin_obj =
          clang_plugin_dir + (flag_cpp ? I386_CLANGVVD_SSA_PLUGINCPP : I386_CLANGVVD_SSA_PLUGIN) + plugin_ext;
      compiler.ssa_plugin_name = (flag_cpp ? I386_CLANGVVD_SSA_PLUGINCPP : I386_CLANGVVD_SSA_PLUGIN);
      compiler.expandMemOps_plugin_obj = clang_plugin_dir + I386_CLANGVVD_EXPANDMEMOPS_PLUGIN + plugin_ext;
      compiler.expandMemOps_plugin_name = I386_CLANGVVD_EXPANDMEMOPS_PLUGIN;
      compiler.GepiCanon_plugin_obj = clang_plugin_dir + I386_CLANGVVD_GEPICANON_PLUGIN + plugin_ext;
      compiler.GepiCanon_plugin_name = I386_CLANGVVD_GEPICANON_PLUGIN;
      compiler.CSROA_plugin_obj = clang_plugin_dir + I386_CLANGVVD_CSROA_PLUGIN + plugin_ext;
      compiler.CSROA_plugin_name = I386_CLANGVVD_CSROA_PLUGIN;
      compiler.topfname_plugin_obj = clang_plugin_dir + I386_CLANGVVD_TOPFNAME_PLUGIN + plugin_ext;
      compiler.topfname_plugin_name = I386_CLANGVVD_TOPFNAME_PLUGIN;
      compiler.ASTAnalyzer_plugin_obj = clang_plugin_dir + I386_CLANGVVD_ASTANALYZER_PLUGIN + plugin_ext;
      compiler.ASTAnalyzer_plugin_name = I386_CLANGVVD_ASTANALYZER_PLUGIN;
      compiler.llvm_link = relocate_compiler_path(I386_LLVMVVD_LINK_EXE);
      compiler.llvm_opt = relocate_compiler_path(I386_LLVMVVD_OPT_EXE);
   }
#endif

   if(compiler.gcc == "")
   {
      THROW_ERROR("Not found any compatible compiler");
   }

   return compiler;
}

std::string CompilerWrapper::GetAnalyzeCompiler() const
{
   const auto flag_cpp =
       Param->isOption(OPT_input_format) &&
       (Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP ||
        Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LLVM_CPP);

#if HAVE_I386_CLANG16_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP16_EXE) : relocate_compiler_path(I386_CLANG16_EXE);
#elif HAVE_I386_CLANG13_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP13_EXE) : relocate_compiler_path(I386_CLANG13_EXE);
#elif HAVE_I386_CLANG12_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP12_EXE) : relocate_compiler_path(I386_CLANG12_EXE);
#elif HAVE_I386_CLANG11_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP11_EXE) : relocate_compiler_path(I386_CLANG11_EXE);
#elif HAVE_I386_CLANG10_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP10_EXE) : relocate_compiler_path(I386_CLANG10_EXE);
#elif HAVE_I386_CLANG9_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP9_EXE) : relocate_compiler_path(I386_CLANG9_EXE);
#elif HAVE_I386_CLANG8_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP8_EXE) : relocate_compiler_path(I386_CLANG8_EXE);
#elif HAVE_I386_CLANG7_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP7_EXE) : relocate_compiler_path(I386_CLANG7_EXE);
#elif HAVE_I386_CLANG6_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP6_EXE) : relocate_compiler_path(I386_CLANG6_EXE);
#elif HAVE_I386_CLANG5_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP5_EXE) : relocate_compiler_path(I386_CLANG5_EXE);
#elif HAVE_I386_CLANG4_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPP4_EXE) : relocate_compiler_path(I386_CLANG4_EXE);
#elif HAVE_I386_CLANGVVD_COMPILER
   return flag_cpp ? relocate_compiler_path(I386_CLANGPPVVD_EXE) : relocate_compiler_path(I386_CLANGVVD_EXE);
#else
   THROW_ERROR("unexpected condition");
   return "";
#endif
}

void CompilerWrapper::GetSystemIncludes(std::vector<std::string>& includes) const
{
   const auto include_file = Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) / "__include";
   const auto command =
       GetCompiler().cpp +
       " -v  < /dev/null 2>&1 | sed -n '/#include </,/> search ends here:/p' | grep -v -E \"(#|End of search "
       "list.|COMPILER_PATH|LIBRARY_PATH|COLLECT_GCC|OFFLOAD_TARGET_NAMES|OFFLOAD_TARGET_DEFAULT)\" | sed 's/ //'";
   const auto ret = PandaSystem(Param, command, false, include_file);
   PRINT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "");
   if(IsError(ret))
   {
      util_print_cpu_stats(std::cerr);
      THROW_ERROR("Error in retrieving gcc system include. Error is " + std::to_string(ret));
   }

   std::ifstream includefile(include_file);
   if(includefile.is_open())
   {
      std::string line;
      while(getline(includefile, line))
      {
         includes.push_back(line);
      }
      includefile.close();
   }
   else
   {
      THROW_ERROR("Error in retrieving gcc system include");
   }

   std::filesystem::remove(include_file);
}

void CompilerWrapper::GetCompilerConfig() const
{
   QueryCompilerConfig("-v");
}

void CompilerWrapper::QueryCompilerConfig(const std::string& compiler_option) const
{
   const auto command = GetCompiler().gcc + " " + compiler_option;
   const auto output_file_name =
       Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) / STR_CST_file_IO_shell_output_file;
   int ret = PandaSystem(Param, command, false, output_file_name);
   if(IsError(ret))
   {
      THROW_ERROR("Error in retrieving gcc configuration");
   }
   CopyStdout(output_file_name);
}

std::string CompilerWrapper::GetCompilerParameters(const std::string& extra_compiler_options,
                                                   bool no_frontend_compiler_parameters) const
{
   const auto compiler = GetCompiler();
   std::string command =
       (no_frontend_compiler_parameters ? "" : frontend_compiler_parameters) + " " + compiler_linking_parameters + " ";
   command += "-D__NO_INLINE__ "; /// needed to avoid problem with glibc inlines

   std::string local_compiler_extra_options = no_frontend_compiler_parameters ? "" : compiler.extra_options;
   if(extra_compiler_options.find("-m32") != std::string::npos)
   {
      boost::replace_all(local_compiler_extra_options, "-mx32", "");
   }

#ifdef _WIN32
   if(local_compiler_extra_options.find("-m32") != std::string::npos)
   {
      boost::replace_all(local_compiler_extra_options, "-m32", "");
   }
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Extra options are " + local_compiler_extra_options);
   command += local_compiler_extra_options + " " + extra_compiler_options + " ";
   boost::replace_all(command, "-target fpga64-xilinx-linux-gnu", "");

   return command;
}

void CompilerWrapper::CreateExecutable(const CustomSet<std::string>& file_names, const std::string& executable_name,
                                       const std::string& extra_compiler_options,
                                       bool no_frontend_compiler_parameters) const
{
   std::list<std::string> sorted_file_names;
   for(const auto& file_name : file_names)
   {
      sorted_file_names.push_back(file_name);
   }
   CreateExecutable(sorted_file_names, executable_name, extra_compiler_options, no_frontend_compiler_parameters);
}

void CompilerWrapper::CreateExecutable(const std::list<std::string>& file_names, const std::string& executable_name,
                                       const std::string& extra_compiler_options,
                                       bool no_frontend_compiler_parameters) const
{
   std::string file_names_string;
   bool has_cpp_file = false;
   for(const auto& file_name : file_names)
   {
      auto file_format = Param->GetFileFormat(file_name, false);
      if(file_format == Parameters_FileFormat::FF_CPP || file_format == Parameters_FileFormat::FF_LLVM_CPP)
      {
         has_cpp_file = true;
      }
      file_names_string += file_name + " ";
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Creating executable " + executable_name + " from " + file_names_string);

   const auto compiler = GetCompiler();
   std::string command = compiler.gcc + " ";
   command += GetCompilerParameters(extra_compiler_options, no_frontend_compiler_parameters);
   command += "-o " + executable_name + " ";
   command += file_names_string + " ";
   command += AddSourceCodeIncludes(file_names) + " ";

   if(!has_cpp_file)
   {
      command = std::regex_replace(command, std::regex("[-]{1,2}std=c\\+\\+\\w+"), "");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Compilation command is " + command);
   const auto compiler_output_filename =
       Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) / STR_CST_gcc_output;

   int ret = PandaSystem(Param, command, false, compiler_output_filename);
   if(IsError(ret))
   {
      CopyStdout(compiler_output_filename);
      THROW_ERROR_CODE(COMPILING_EC, "Front-end compiler returns an error during compilation " + std::to_string(errno) +
                                         " - Command is " + command);
   }
   else
   {
      if(output_level >= OUTPUT_LEVEL_VERBOSE)
      {
         CopyStdout(compiler_output_filename);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
}

void CompilerWrapper::ReadParameters()
{
   if(Param->isOption(OPT_gcc_optimizations))
   {
      const auto parameters = Param->getOption<CustomSet<std::string>>(OPT_gcc_optimizations);
      for(const auto& parameter : parameters)
      {
         THROW_ASSERT(parameter != "", "unexpected condition:" + Param->getOption<std::string>(OPT_gcc_optimizations));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining parameter " + parameter);
         const size_t found = parameter.find("no-");
         // if the token starts with "no-", the optimization has to be disabled
         if(found == 0)
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
      const auto parameters = Param->getOption<CustomSet<std::string>>(OPT_gcc_parameters);
      for(const auto& parameter : parameters)
      {
         const size_t equal_size = parameter.find('=');
         if(equal_size == std::string::npos)
         {
            THROW_ERROR("--param argument without value " + parameter);
         }
         const std::string key = parameter.substr(0, equal_size);
         const std::string value = parameter.substr(equal_size + 1, parameter.size() - equal_size + 1);
         parameter_values[key] = std::stoi(value);
      }
   }
}

std::string CompilerWrapper::WriteOptimizationLevel(const CompilerWrapper_OptimizationSet optimization_set)
{
   switch(optimization_set)
   {
      case(CompilerWrapper_OptimizationSet::O0):
         return "0";
      case(CompilerWrapper_OptimizationSet::O1):
         return "1";
      case(CompilerWrapper_OptimizationSet::O2):
         return "2";
      case(CompilerWrapper_OptimizationSet::O3):
         return "3";
      case(CompilerWrapper_OptimizationSet::O4):
         return "4";
      case(CompilerWrapper_OptimizationSet::O5):
         return "5";
      case(CompilerWrapper_OptimizationSet::Og):
         return "g";
      case(CompilerWrapper_OptimizationSet::Os):
         return "s";
      case(CompilerWrapper_OptimizationSet::Ofast):
         return "fast";
      case(CompilerWrapper_OptimizationSet::Oz):
         return "z";
      case(CompilerWrapper_OptimizationSet::OBAMBU):
         return "bambu";
      case(CompilerWrapper_OptimizationSet::OSF):
         return "softfloat";
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return "";
}

std::string CompilerWrapper::WriteOptimizationsString()
{
   std::string optimizations;
   /// Preparing optimizations string
   bool argument_noalias = optimization_flags.find("argument-noalias") != optimization_flags.end() &&
                           optimization_flags.find("argument-noalias")->second;
   bool argument_noalias_global = optimization_flags.find("argument-noalias-global") != optimization_flags.end() &&
                                  optimization_flags.find("argument-noalias-global")->second;
   bool argument_noalias_anything = optimization_flags.find("argument-noalias-anything") != optimization_flags.end() &&
                                    optimization_flags.find("argument-noalias-anything")->second;
   bool strict_aliasing = optimization_flags.find("strict-aliasing") != optimization_flags.end() &&
                          optimization_flags.find("strict-aliasing")->second;
   std::map<std::string, bool>::const_iterator it, it_end = optimization_flags.end();
   if(strict_aliasing)
   {
      optimizations += "-Wstrict-aliasing ";
   }
   for(it = optimization_flags.begin(); it != it_end; ++it)
   {
      /*argument aliasing should be treated in a different way*/
      if(it->first == "argument-alias" && (argument_noalias || argument_noalias_global || argument_noalias_anything))
      {
         continue;
      }
      else if(it->first == "argument-noalias" && (argument_noalias_global || argument_noalias_anything))
      {
         continue;
      }
      else if(it->first == "argument-noalias-global" && (argument_noalias_anything))
      {
         continue;
      }
      THROW_ASSERT(it->first != "", "unexpected condition");
      if(it->first == "fp-contract")
      {
         optimizations += it->second ? "-ffp-contract=on " : "-ffp-contract=off ";
      }
      else
      {
         optimizations += std::string(it->second ? "-f" : "-fno-") + it->first + " ";
      }
   }
   std::map<std::string, int>::const_iterator it2, it2_end = optimization_values.end();
   for(it2 = optimization_values.begin(); it2 != it2_end; ++it2)
   {
      optimizations += "-f" + it2->first + "=" + std::to_string(it2->second) + " ";
   }
   std::map<std::string, int>::const_iterator it3, it3_end = parameter_values.end();
   for(it3 = parameter_values.begin(); it3 != it3_end; ++it3)
   {
      optimizations += "--param " + it3->first + "=" + std::to_string(it3->second) + " ";
   }
   return optimizations;
}

void CompilerWrapper::ReadXml(const std::string& file_name)
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
            if(!root_child_element)
            {
               continue;
            }
            if(root_child_element->get_name() == STR_XML_gcc_optimizations)
            {
               const xml_node::node_list optimizations_children = root_child_element->get_children();
               xml_node::node_list::const_iterator optimizations_child,
                   optimizations_child_end = optimizations_children.end();
               for(optimizations_child = optimizations_children.begin(); optimizations_child != optimizations_child_end;
                   ++optimizations_child)
               {
                  const auto* optimizations_child_element = GetPointer<const xml_element>(*optimizations_child);
                  if(!optimizations_child_element)
                  {
                     continue;
                  }
                  if(optimizations_child_element->get_name() == STR_XML_gcc_parameter_values)
                  {
                     const xml_node::node_list parameter_values_children = optimizations_child_element->get_children();
                     xml_node::node_list::const_iterator parameter_value,
                         parameter_value_end = parameter_values_children.end();
                     for(parameter_value = parameter_values_children.begin(); parameter_value != parameter_value_end;
                         ++parameter_value)
                     {
                        const auto* parameter_value_element = GetPointer<const xml_element>(*parameter_value);
                        if(!parameter_value_element)
                        {
                           continue;
                        }
                        if(not(parameter_value_element->get_attribute(STR_XML_gcc_name) &&
                               parameter_value_element->get_attribute(STR_XML_gcc_value)))
                        {
                           THROW_ERROR("Parameter value node without name or value");
                        }
                        parameter_values[parameter_value_element->get_attribute(STR_XML_gcc_name)->get_name()] =
                            std::stoi(parameter_value_element->get_attribute(STR_XML_gcc_value)->get_value());
                     }
                  }
                  else if(optimizations_child_element->get_name() == STR_XML_gcc_optimization_flags)
                  {
                     const xml_node::node_list optimization_flags_children =
                         optimizations_child_element->get_children();
                     xml_node::node_list::const_iterator optimization_flag,
                         optimization_flag_end = optimization_flags_children.end();
                     for(optimization_flag = optimization_flags_children.begin();
                         optimization_flag != optimization_flag_end; ++optimization_flag)
                     {
                        const auto* optimization_flag_element = GetPointer<const xml_element>(*optimization_flag);
                        if(!optimization_flag_element)
                        {
                           continue;
                        }
                        if(!(optimization_flag_element->get_attribute(STR_XML_gcc_name) &&
                             optimization_flag_element->get_attribute(STR_XML_gcc_value)))
                        {
                           THROW_ERROR("Optimization flag node without name or value");
                        }
                        optimization_flags[optimization_flag_element->get_attribute(STR_XML_gcc_name)->get_value()] =
                            static_cast<bool>(
                                std::stoi(optimization_flag_element->get_attribute(STR_XML_gcc_value)->get_value()));
                     }
                  }
                  else if(optimizations_child_element->get_name() == STR_XML_gcc_optimization_values)
                  {
                     const xml_node::node_list optimization_value_children =
                         optimizations_child_element->get_children();
                     xml_node::node_list::const_iterator optimization_value,
                         optimization_value_end = optimization_value_children.end();
                     for(optimization_value = optimization_value_children.begin();
                         optimization_value != optimization_value_end; ++optimization_value)
                     {
                        const auto* optimization_value_element = GetPointer<const xml_element>(*optimization_value);
                        if(!optimization_value_element)
                        {
                           continue;
                        }
                        if(!(optimization_value_element->get_attribute(STR_XML_gcc_name) &&
                             optimization_value_element->get_attribute(STR_XML_gcc_value)))
                        {
                           THROW_ERROR("Optimization value node without name or value");
                        }
                        optimization_values[optimization_value_element->get_attribute(STR_XML_gcc_name)->get_name()] =
                            std::stoi(optimization_value_element->get_attribute(STR_XML_gcc_value)->get_value());
                     }
                  }
               }
               frontend_compiler_parameters += (" " + WriteOptimizationsString() + " ");
            }
            else if(root_child_element->get_name() == STR_XML_gcc_standard)
            {
               const xml_element* standard = root_child_element;
               if(!standard->get_attribute(STR_XML_gcc_value))
               {
                  THROW_ERROR("Standard node without value");
               }
               frontend_compiler_parameters += "--std=" + standard->get_attribute(STR_XML_gcc_value)->get_value() + " ";
            }
            else if(root_child_element->get_name() == STR_XML_gcc_defines)
            {
               const xml_node::node_list defines = root_child_element->get_children();
               xml_node::node_list::const_iterator define, define_end = defines.end();
               for(define = defines.begin(); define != define_end; ++define)
               {
                  const auto* define_element = GetPointer<const xml_element>(*define);
                  if(!define_element)
                  {
                     continue;
                  }
                  if(!define_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  frontend_compiler_parameters +=
                      "-D" + define_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_undefines)
            {
               const xml_node::node_list undefines = root_child_element->get_children();
               xml_node::node_list::const_iterator undefine, undefine_end = undefines.end();
               for(undefine = undefines.begin(); undefine != undefine_end; ++undefine)
               {
                  const auto* undefine_element = GetPointer<const xml_element>(*undefine);
                  if(!undefine_element)
                  {
                     continue;
                  }
                  if(!undefine_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  frontend_compiler_parameters +=
                      "-U" + undefine_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_warnings)
            {
               const xml_node::node_list warnings = root_child_element->get_children();
               xml_node::node_list::const_iterator warning, warning_end = warnings.end();
               for(warning = warnings.begin(); warning != warning_end; ++warning)
               {
                  const auto* warning_element = GetPointer<const xml_element>(*warning);
                  if(!warning_element)
                  {
                     continue;
                  }
                  if(!warning_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  frontend_compiler_parameters +=
                      "-W" + warning_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_includes)
            {
               const xml_node::node_list includes = root_child_element->get_children();
               xml_node::node_list::const_iterator include, include_end = includes.end();
               for(include = includes.begin(); include != include_end; ++include)
               {
                  const auto* include_element = GetPointer<const xml_element>(*include);
                  if(!include_element)
                  {
                     continue;
                  }
                  if(!include_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  frontend_compiler_parameters +=
                      "-I" + include_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_libraries)
            {
               const xml_node::node_list libraries = root_child_element->get_children();
               xml_node::node_list::const_iterator library, library_end = libraries.end();
               for(library = libraries.begin(); library != library_end; ++library)
               {
                  const auto* library_element = GetPointer<const xml_element>(*library);
                  if(!library_element)
                  {
                     continue;
                  }
                  if(!library_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  compiler_linking_parameters +=
                      "-l" + library_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
               }
            }
            else if(root_child_element->get_name() == STR_XML_gcc_library_directories)
            {
               const xml_node::node_list library_directories = root_child_element->get_children();
               xml_node::node_list::const_iterator library_directory, library_directory_end = library_directories.end();
               for(library_directory = library_directories.begin(); library_directory != library_directory_end;
                   ++library_directory)
               {
                  const auto* library_directory_element = GetPointer<const xml_element>(*library_directory);
                  if(!library_directory_element)
                  {
                     continue;
                  }
                  if(!library_directory_element->get_attribute(STR_XML_gcc_value))
                  {
                     THROW_ERROR("Optimization value node without name or value");
                  }
                  frontend_compiler_parameters +=
                      "-L" + library_directory_element->get_attribute(STR_XML_gcc_value)->get_value() + " ";
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

void CompilerWrapper::WriteXml(const std::string& file_name) const
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
      parameter_value_xml->set_attribute(STR_XML_gcc_value, std::to_string(parameter_value->second));
   }
   xml_element* optimization_flags_xml = optimizations->add_child_element(STR_XML_gcc_parameter_values);
   std::map<std::string, bool>::const_iterator optimization_flag,
       optimization_flag_end = this->optimization_flags.end();
   for(optimization_flag = this->optimization_flags.begin(); optimization_flag != optimization_flag_end;
       ++optimization_flag)
   {
      xml_element* optimization_flag_xml = optimization_flags_xml->add_child_element(STR_XML_gcc_optimization_flag);
      optimization_flag_xml->set_attribute(STR_XML_gcc_name, optimization_flag->first);
      optimization_flag_xml->set_attribute(STR_XML_gcc_value, STR(optimization_flag->second));
   }
   xml_element* optimization_values_xml = optimizations->add_child_element(STR_XML_gcc_parameter_values);
   std::map<std::string, int>::const_iterator optimization_value,
       optimization_value_end = this->optimization_values.end();
   for(optimization_value = this->optimization_values.begin(); optimization_value != optimization_value_end;
       ++optimization_value)
   {
      xml_element* optimization_value_xml = optimization_values_xml->add_child_element(STR_XML_gcc_optimization_value);
      optimization_value_xml->set_attribute(STR_XML_gcc_name, optimization_value->first);
      optimization_value_xml->set_attribute(STR_XML_gcc_value, STR(optimization_value->second));
   }
   document.write_to_file_formatted(file_name);
}

const std::string CompilerWrapper::AddSourceCodeIncludes(const std::list<std::string>& source_files) const
{
   std::string includes;
   /// Adding includes of original source code files
   for(const auto& source_file : source_files)
   {
      std::filesystem::path absolute_path = std::filesystem::absolute(source_file);
      std::string new_path = "-iquote " + absolute_path.parent_path().string() + " ";
#ifdef _WIN32
      boost::replace_all(new_path, "\\", "/");
#endif
      if(frontend_compiler_parameters.find(new_path) == std::string::npos &&
         includes.find(new_path) == std::string::npos)
      {
         includes += new_path;
      }
   }
   return includes;
}

std::string CompilerWrapper::clang_recipes(const CompilerWrapper_OptimizationSet optimization_set,
                                           const std::string& fname) const
{
   const auto& compiler = GetCompiler();
   auto plugin_prefix = add_plugin_prefix(compiler_target);
   const auto& expandMemOps_plugin_name = compiler.expandMemOps_plugin_name;
   const auto& GepiCanon_plugin_name = compiler.GepiCanon_plugin_name;
   const auto& CSROA_plugin_name = compiler.CSROA_plugin_name;
#ifndef _WIN32
   const auto& expandMemOps_plugin_obj = compiler.expandMemOps_plugin_obj;
   const auto& GepiCanon_plugin_obj = compiler.GepiCanon_plugin_obj;
   const auto& CSROA_plugin_obj = compiler.CSROA_plugin_obj;
#endif

   const auto opt_level = WriteOptimizationLevel(optimization_set == CompilerWrapper_OptimizationSet::O0 ?
                                                     CompilerWrapper_OptimizationSet::O1 :
                                                     optimization_set);

   std::string recipe;
#ifndef _WIN32
   recipe += load_plugin_opt(expandMemOps_plugin_obj, compiler_target);

   if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1 &&
      !GepiCanon_plugin_obj.empty())
   {
      recipe += load_plugin_opt(GepiCanon_plugin_obj, compiler_target);
   }
#endif

   if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1
#ifndef _WIN32
      && !CSROA_plugin_obj.empty()
#endif
   )
   {
#ifndef _WIN32
      recipe += load_plugin_opt(CSROA_plugin_obj, compiler_target);
#endif
      recipe += " -panda-KN=" + fname;
      if(Param->IsParameter("max-CSROA"))
      {
         auto max_CSROA = Param->GetParameter<int>("max-CSROA");
         recipe += " -csroa-max-transformations=" + STR(max_CSROA);
      }
   }

   if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG4)
   {
      if(optimization_set == CompilerWrapper_OptimizationSet::O2 ||
         optimization_set == CompilerWrapper_OptimizationSet::O3)
      {
         std::string complex_recipe;
         complex_recipe += " -tti -targetlibinfo -tbaa -scoped-noalias -assumption-cache-tracker -profile-summary-info "
                           "-forceattrs -inferattrs " +
                           ("-" + expandMemOps_plugin_name) + " -domtree -mem2reg ";
         if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1
#ifndef _WIN32
            && !GepiCanon_plugin_obj.empty() && !CSROA_plugin_obj.empty()
#endif
         )
         {
            complex_recipe +=
                ("-" + GepiCanon_plugin_name + "PS ") + ("-" + GepiCanon_plugin_name + "COL ") +
                ("-" + GepiCanon_plugin_name + "BVR ") +
                "-loops -loop-simplify -lcssa-verification -lcssa -basicaa -aa -scalar-evolution -loop-unroll " +
                ("-" + CSROA_plugin_name + "FV ") +
                "-ipsccp -globaldce -domtree -mem2reg -deadargelim -basiccg -argpromotion -domtree -loops "
                "-loop-simplify -lcssa-verification -lcssa -basicaa -aa -scalar-evolution -loop-unroll "
                "-simplifycfg ";
         }
         if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1
#ifndef _WIN32
            && !GepiCanon_plugin_obj.empty() && !CSROA_plugin_obj.empty()
#endif
         )
         {
            complex_recipe += "-" + expandMemOps_plugin_name + (" -" + GepiCanon_plugin_name + "PS ") +
                              ("-" + GepiCanon_plugin_name + "COL ") + ("-" + GepiCanon_plugin_name + "BVR ") +
                              ("-" + CSROA_plugin_name + "D ");
         }
         complex_recipe +=
             "-ipsccp -globalopt -dse -loop-unroll -instcombine -libcalls-shrinkwrap -tailcallelim -simplifycfg "
             "-reassociate -domtree -loops -loop-simplify -lcssa-verification -lcssa -basicaa -aa -scalar-evolution "
             "-loop-rotate -licm -loop-unswitch -simplifycfg -domtree -basicaa -aa  -dse -loop-unroll -instcombine "
             "-loops -loop-simplify -lcssa-verification -lcssa -scalar-evolution -indvars -loop-idiom "
             "-loop-deletion "
             "-loop-unroll -mldst-motion -aa -memdep -lazy-branch-prob -lazy-block-freq -opt-remark-emitter -gvn "
             "-basicaa -aa -memdep -memcpyopt -sccp -domtree -demanded-bits -bdce -basicaa -aa  -dse -loop-unroll "
             "-instcombine -lazy-value-info -jump-threading -correlated-propagation -domtree -basicaa -aa -memdep "
             "-dse "
             "-loops -loop-simplify -lcssa-verification -lcssa -aa -scalar-evolution -licm -postdomtree -adce "
             "-simplifycfg -domtree -basicaa -aa  -loop-unroll -instcombine -barrier -elim-avail-extern -basiccg "
             "-rpo-functionattrs -globals-aa -float2int -domtree -loops -loop-simplify -lcssa-verification -lcssa "
             "-basicaa -aa -scalar-evolution -loop-rotate -loop-accesses -lazy-branch-prob -lazy-block-freq "
             "-opt-remark-emitter -loop-distribute -loop-simplify -lcssa-verification -lcssa -branch-prob "
             "-block-freq "
             "-scalar-evolution -basicaa -aa -loop-accesses -demanded-bits -lazy-branch-prob -lazy-block-freq "
             "-opt-remark-emitter -disable-slp-vectorization -disable-loop-vectorization -scalarizer-loop-simplify "
             "-scalar-evolution -aa -loop-accesses -loop-load-elim -basicaa -aa  -dse -loop-unroll -instcombine "
             "-simplifycfg -domtree -basicaa -aa  -dse -loop-unroll -instcombine -loops -loop-simplify "
             "-lcssa-verification -lcssa -scalar-evolution -loop-unroll ";
         if(Param->IsParameter("enable-CSROA") && Param->GetParameter<int>("enable-CSROA") == 1
#ifndef _WIN32
            && !GepiCanon_plugin_obj.empty() && !CSROA_plugin_obj.empty()
#endif
         )
         {
            complex_recipe += " -" + expandMemOps_plugin_name + " -" + CSROA_plugin_name + "WI ";
         }
         complex_recipe +=
             "-domtree -basicaa -aa -memdep -dse -aa -memoryssa -early-cse-memssa -constprop -ipsccp -globaldce "
             "-domtree -mem2reg -deadargelim -basiccg -argpromotion -domtree -loops -loop-simplify "
             "-lcssa-verification "
             "-lcssa -basicaa -aa -scalar-evolution -loop-unroll  -dse -loop-unroll -instcombine -loop-simplify "
             "-lcssa-verification -lcssa -scalar-evolution -licm -alignment-from-assumptions -strip-dead-prototypes "
             "-globaldce -constmerge -domtree -loops -branch-prob -block-freq -loop-simplify -lcssa-verification "
             "-lcssa -basicaa -aa -scalar-evolution -branch-prob -block-freq -loop-sink -instsimplify ";
         recipe += complex_recipe;
      }
      else
      {
         recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer";
         recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
      }
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG5)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG6)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG7)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG8)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG9)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG10)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG11)
   {
      recipe += " -O" + opt_level + " --disable-vector-combine -vectorize-loops=false -vectorize-slp=false -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG12)
   {
      recipe += " -O" + opt_level + " --disable-vector-combine -vectorize-loops=false -vectorize-slp=false -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG13)
   {
      recipe += " -O" + opt_level + " --disable-vector-combine -vectorize-loops=false -vectorize-slp=false -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG16)
   {
      recipe += add_plugin_prefix(compiler_target, opt_level) +
                " --disable-vector-combine -vectorize-loops=false -vectorize-slp=false " + plugin_prefix + "scalarizer";
      recipe += plugin_prefix + expandMemOps_plugin_name + plugin_prefix + "simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD)
   {
      recipe += " -O" + opt_level + " --disable-vector-combine -scalarizer";
      recipe += " -" + expandMemOps_plugin_name + " -simplifycfg ";
   }
   else
   {
      THROW_ERROR("Clang compiler not yet supported");
   }
   return " " + recipe;
}

size_t CompilerWrapper::CGetPointerSize(const ParameterConstRef parameters)
{
   const auto gcc_m_env = parameters->getOption<std::string>(OPT_gcc_m_env);
   if(gcc_m_env == "-m32" || gcc_m_env == "-mx32")
   {
      return 32;
   }
   else if(gcc_m_env == "-m64")
   {
      return 64;
   }
   else
   {
      THROW_ERROR("-m parameter not supported: " + gcc_m_env);
   }
   return 0;
}

bool CompilerWrapper::isCurrentOrNewer(CompilerWrapper_CompilerTarget ct, CompilerWrapper_CompilerTarget compare)
{
   if(isGccCheck(ct) ^ isGccCheck(compare))
   {
      THROW_ERROR("Comparisong between different compiler families not possible");
   }
   return ct >= compare;
}

bool CompilerWrapper::isClangCheck(CompilerWrapper_CompilerTarget ct)
{
   return ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG4 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG5 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG6 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG7 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG8 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG9 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG10 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG11 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG12 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG13 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG16 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD;
}

bool CompilerWrapper::isGccCheck(CompilerWrapper_CompilerTarget ct)
{
   return ct == CompilerWrapper_CompilerTarget::CT_I386_GCC49 || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_GCC6 || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC7 ||
          ct == CompilerWrapper_CompilerTarget::CT_I386_GCC8;
}

int CompilerWrapper::getCompatibleCompilers()
{
   return 0
#if HAVE_I386_GCC49_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC49)
#endif
#if HAVE_I386_GCC5_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC5)
#endif
#if HAVE_I386_GCC6_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC6)
#endif
#if HAVE_I386_GCC7_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC7)
#endif
#if HAVE_I386_GCC8_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC8)
#endif
#if HAVE_I386_CLANG4_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG4)
#endif
#if HAVE_I386_CLANG5_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG5)
#endif
#if HAVE_I386_CLANG6_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG6)
#endif
#if HAVE_I386_CLANG7_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG7)
#endif
#if HAVE_I386_CLANG8_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG8)
#endif
#if HAVE_I386_CLANG9_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG9)
#endif
#if HAVE_I386_CLANG10_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG10)
#endif
#if HAVE_I386_CLANG11_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG11)
#endif
#if HAVE_I386_CLANG12_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG12)
#endif
#if HAVE_I386_CLANG13_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG13)
#endif
#if HAVE_I386_CLANG16_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG16)
#endif
#if HAVE_I386_CLANGVVD_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD)
#endif
       ;
}

int CompilerWrapper::getDefaultCompiler()
{
   return
#if HAVE_I386_CLANG16_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG16);
#elif HAVE_I386_CLANG7_COMPILER && defined(_WIN32)
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG7);
#elif HAVE_I386_GCC49_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC49);
#elif HAVE_I386_GCC8_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC8);
#elif HAVE_I386_GCC7_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC7);
#elif HAVE_I386_GCC6_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC6);
#elif HAVE_I386_GCC5_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC5);
#elif HAVE_I386_CLANG4_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG4);
#elif HAVE_I386_CLANG5_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG5);
#elif HAVE_I386_CLANG6_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG6);
#elif HAVE_I386_CLANG7_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG7);
#elif HAVE_I386_CLANG8_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG8);
#elif HAVE_I386_CLANG9_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG9);
#elif HAVE_I386_CLANG10_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG10);
#elif HAVE_I386_CLANG11_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG11);
#elif HAVE_I386_CLANG12_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG12);
#elif HAVE_I386_CLANG13_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG13);
#elif HAVE_I386_CLANGVVD_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD);
#else
       0;
   THROW_ERROR("No frontend compiler available");
#endif
}

std::string CompilerWrapper::getCompilerSuffix(CompilerWrapper_CompilerTarget pc)
{
   switch(pc)
   {
      case CompilerWrapper_CompilerTarget::CT_I386_GCC49:
         return "gcc49";
      case CompilerWrapper_CompilerTarget::CT_I386_GCC5:
         return "gcc5";
      case CompilerWrapper_CompilerTarget::CT_I386_GCC6:
         return "gcc6";
      case CompilerWrapper_CompilerTarget::CT_I386_GCC7:
         return "gcc7";
      case CompilerWrapper_CompilerTarget::CT_I386_GCC8:
         return "gcc8";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG4:
         return "clang4";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG5:
         return "clang5";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG6:
         return "clang6";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG7:
         return "clang7";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG8:
         return "clang8";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG9:
         return "clang9";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG10:
         return "clang10";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG11:
         return "clang11";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG12:
         return "clang12";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG13:
         return "clang13";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG16:
         return "clang16";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD:
         return "clangvvd";
      case CompilerWrapper_CompilerTarget::CT_NO_COMPILER:
      default:
         THROW_ERROR("no compiler supported");
         return "";
   }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

bool CompilerWrapper::hasCompilerM64(CompilerWrapper_CompilerTarget ct)
{
   return false
#if(HAVE_I386_GCC49_COMPILER && HAVE_I386_GCC49_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if(HAVE_I386_GCC5_COMPILER && HAVE_I386_GCC5_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if(HAVE_I386_GCC6_COMPILER && HAVE_I386_GCC6_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if(HAVE_I386_GCC7_COMPILER && HAVE_I386_GCC7_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if(HAVE_I386_GCC8_COMPILER && HAVE_I386_GCC8_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC8
#endif
#if(HAVE_I386_CLANG4_COMPILER && HAVE_I386_CLANG4_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if(HAVE_I386_CLANG5_COMPILER && HAVE_I386_CLANG5_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if(HAVE_I386_CLANG6_COMPILER && HAVE_I386_CLANG6_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if(HAVE_I386_CLANG7_COMPILER && HAVE_I386_CLANG7_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG7
#endif
#if(HAVE_I386_CLANG8_COMPILER && HAVE_I386_CLANG8_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG8
#endif
#if(HAVE_I386_CLANG9_COMPILER && HAVE_I386_CLANG9_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG9
#endif
#if(HAVE_I386_CLANG10_COMPILER && HAVE_I386_CLANG10_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG10
#endif
#if(HAVE_I386_CLANG11_COMPILER && HAVE_I386_CLANG11_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG11
#endif
#if(HAVE_I386_CLANG12_COMPILER && HAVE_I386_CLANG12_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG12
#endif
#if(HAVE_I386_CLANG13_COMPILER && HAVE_I386_CLANG13_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG13
#endif
#if(HAVE_I386_CLANG16_COMPILER && HAVE_I386_CLANG16_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG16
#endif
#if(HAVE_I386_CLANGVVD_COMPILER && HAVE_I386_CLANGVVD_M64)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD
#endif
       ;
}

bool CompilerWrapper::hasCompilerMX32(CompilerWrapper_CompilerTarget ct)
{
   return false
#if(HAVE_I386_GCC49_COMPILER && HAVE_I386_GCC49_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if(HAVE_I386_GCC5_COMPILER && HAVE_I386_GCC5_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if(HAVE_I386_GCC6_COMPILER && HAVE_I386_GCC6_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if(HAVE_I386_GCC7_COMPILER && HAVE_I386_GCC7_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if(HAVE_I386_GCC8_COMPILER && HAVE_I386_GCC8_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC8
#endif
#if(HAVE_I386_CLANG4_COMPILER && HAVE_I386_CLANG4_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if(HAVE_I386_CLANG5_COMPILER && HAVE_I386_CLANG5_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if(HAVE_I386_CLANG6_COMPILER && HAVE_I386_CLANG6_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if(HAVE_I386_CLANG7_COMPILER && HAVE_I386_CLANG7_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG7
#endif
#if(HAVE_I386_CLANG8_COMPILER && HAVE_I386_CLANG8_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG8
#endif
#if(HAVE_I386_CLANG9_COMPILER && HAVE_I386_CLANG9_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG9
#endif
#if(HAVE_I386_CLANG10_COMPILER && HAVE_I386_CLANG10_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG10
#endif
#if(HAVE_I386_CLANG11_COMPILER && HAVE_I386_CLANG11_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG11
#endif
#if(HAVE_I386_CLANG12_COMPILER && HAVE_I386_CLANG12_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG12
#endif
#if(HAVE_I386_CLANG13_COMPILER && HAVE_I386_CLANG13_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG13
#endif
#if(HAVE_I386_CLANG16_COMPILER && HAVE_I386_CLANG16_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG16
#endif
#if(HAVE_I386_CLANGVVD_COMPILER && HAVE_I386_CLANGVVD_MX32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD
#endif
       ;
}

bool CompilerWrapper::hasCompilerM32(CompilerWrapper_CompilerTarget ct)
{
   return false
#if(HAVE_I386_GCC49_COMPILER && HAVE_I386_GCC49_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if(HAVE_I386_GCC5_COMPILER && HAVE_I386_GCC5_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if(HAVE_I386_GCC6_COMPILER && HAVE_I386_GCC6_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if(HAVE_I386_GCC7_COMPILER && HAVE_I386_GCC7_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if(HAVE_I386_GCC8_COMPILER && HAVE_I386_GCC8_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_GCC8
#endif
#if(HAVE_I386_CLANG4_COMPILER && HAVE_I386_CLANG4_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if(HAVE_I386_CLANG5_COMPILER && HAVE_I386_CLANG5_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if(HAVE_I386_CLANG6_COMPILER && HAVE_I386_CLANG6_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if(HAVE_I386_CLANG7_COMPILER && HAVE_I386_CLANG7_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG7
#endif
#if(HAVE_I386_CLANG8_COMPILER && HAVE_I386_CLANG8_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG8
#endif
#if(HAVE_I386_CLANG9_COMPILER && HAVE_I386_CLANG9_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG9
#endif
#if(HAVE_I386_CLANG10_COMPILER && HAVE_I386_CLANG10_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG10
#endif
#if(HAVE_I386_CLANG11_COMPILER && HAVE_I386_CLANG11_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG11
#endif
#if(HAVE_I386_CLANG12_COMPILER && HAVE_I386_CLANG12_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG12
#endif
#if(HAVE_I386_CLANG13_COMPILER && HAVE_I386_CLANG13_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG13
#endif
#if(HAVE_I386_CLANG16_COMPILER && HAVE_I386_CLANG16_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANG16
#endif
#if(HAVE_I386_CLANGVVD_COMPILER && HAVE_I386_CLANGVVD_M32)
          || ct == CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD
#endif
       ;
}

#pragma GCC diagnostic pop

std::string CompilerWrapper::getCompilerVersion(int pc)
{
#if HAVE_I386_GCC49_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC49))
   {
      return I386_GCC49_VERSION;
   }
#endif
#if HAVE_I386_GCC5_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC5))
   {
      return I386_GCC5_VERSION;
   }
#endif
#if HAVE_I386_GCC6_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC6))
   {
      return I386_GCC6_VERSION;
   }
#endif
#if HAVE_I386_GCC7_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC7))
   {
      return I386_GCC7_VERSION;
   }
#endif
#if HAVE_I386_GCC8_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_GCC8))
   {
      return I386_GCC8_VERSION;
   }
#endif
#if HAVE_I386_CLANG4_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG4))
   {
      return I386_CLANG4_VERSION;
   }
#endif
#if HAVE_I386_CLANG5_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG5))
   {
      return I386_CLANG5_VERSION;
   }
#endif
#if HAVE_I386_CLANG6_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG6))
   {
      return I386_CLANG6_VERSION;
   }
#endif
#if HAVE_I386_CLANG7_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG7))
   {
      return I386_CLANG7_VERSION;
   }
#endif
#if HAVE_I386_CLANG8_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG8))
   {
      return I386_CLANG8_VERSION;
   }
#endif
#if HAVE_I386_CLANG9_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG9))
   {
      return I386_CLANG9_VERSION;
   }
#endif
#if HAVE_I386_CLANG10_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG10))
   {
      return I386_CLANG10_VERSION;
   }
#endif
#if HAVE_I386_CLANG11_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG11))
   {
      return I386_CLANG11_VERSION;
   }
#endif
#if HAVE_I386_CLANG12_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG12))
   {
      return I386_CLANG12_VERSION;
   }
#endif
#if HAVE_I386_CLANG13_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG13))
   {
      return I386_CLANG13_VERSION;
   }
#endif
#if HAVE_I386_CLANG16_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG16))
   {
      return I386_CLANG16_VERSION;
   }
#endif
#if HAVE_I386_CLANGVVD_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANGVVD))
   {
      return I386_CLANGVVD_VERSION;
   }
#endif
   THROW_ERROR("");
   return "";
}

std::string CompilerWrapper::readExternalSymbols(const std::filesystem::path& filename) const
{
   std::string extern_symbols;
   const auto XMLfilename = [&]() -> std::string {
      if(Param->isOption(OPT_xml_memory_allocation))
      {
         return Param->getOption<std::filesystem::path>(OPT_xml_memory_allocation);
      }
      /// load xml memory allocation file
      const auto generate_xml = Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) /
                                filename.filename().concat(".memory_allocation.xml");
      if((std::filesystem::exists(generate_xml)))
      {
         return generate_xml;
      }
      return std::filesystem::path("");
   }();
   if(XMLfilename.size())
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
            if(const auto* child = GetPointer<xml_element>(l))
            {
               if(child->get_name() == "memory_allocation")
               {
                  for(const auto& it : child->get_children())
                  {
                     if(const auto* mem_node = GetPointer<xml_element>(it))
                     {
                        if(mem_node->get_name() == "object")
                        {
                           std::string is_internal;
                           if(!CE_XVM(is_internal, mem_node))
                           {
                              THROW_ERROR("expected the is_internal attribute");
                           }
                           LOAD_XVM(is_internal, mem_node);
                           if(is_internal == "T")
                           {
                           }
                           else if(is_internal == "F")
                           {
                              if(!CE_XVM(name, mem_node))
                              {
                                 THROW_ERROR("expected the name attribute");
                              }
                              std::string name;
                              LOAD_XVM(name, mem_node);
                              extern_symbols += name + ",";
                           }
                           else
                           {
                              THROW_ERROR("unexpected value for is_internal attribute");
                           }
                        }
                     }
                  }
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
   }
   return extern_symbols;
}

std::string CompilerWrapper::load_plugin(const std::string& plugin_obj, CompilerWrapper_CompilerTarget target) const
{
   if(target == CompilerWrapper_CompilerTarget::CT_I386_CLANG13 ||
      target == CompilerWrapper_CompilerTarget::CT_I386_CLANG16)
   {
      return " -fpass-plugin=" + plugin_obj + " -Xclang -load -Xclang " + plugin_obj;
   }
   return " -fplugin=" + plugin_obj;
}

std::string CompilerWrapper::load_plugin_opt(std::string plugin_obj, CompilerWrapper_CompilerTarget target) const
{
   boost::replace_all(plugin_obj, ".so", "_opt.so");
   auto flags = " -load=" + plugin_obj;
   if(target == CompilerWrapper_CompilerTarget::CT_I386_CLANG13 ||
      target == CompilerWrapper_CompilerTarget::CT_I386_CLANG16)
   {
      flags += " -load-pass-plugin=" + plugin_obj;
   }
   return flags;
}

std::string CompilerWrapper::add_plugin_prefix(CompilerWrapper_CompilerTarget target, std::string O_level) const
{
   std::string flags;
   if(O_level.size())
   {
      if(target == CompilerWrapper_CompilerTarget::CT_I386_CLANG16)
      {
         flags += " -p 'default<O" + O_level + ">'";
      }
      else
      {
         flags += " -O" + O_level;
      }
   }
   else
   {
      if(target == CompilerWrapper_CompilerTarget::CT_I386_CLANG16)
      {
         flags += " -p ";
      }
      else
      {
         flags += " -";
      }
   }
   return flags;
}
