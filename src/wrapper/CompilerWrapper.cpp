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
 * @file CompilerWrapper.cpp
 * @brief Implementation of the wrapper to CC for C sources.
 *
 * Implementation of the methods for the object for invoke the CC compiler from sources and create the dump.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "CompilerWrapper.hpp"

#include "Parameter.hpp"
#include "compiler_constants.hpp"
#include "cpu_stats.hpp"
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "file_IO_constants.hpp"
#include "hls_step.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "parse_ir.hpp"
#include "polixml.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "config_ANALYZER_COMPILER_PLUGINS_DIR.hpp"
#include "config_BUILD_APPIMAGE.hpp"
#include "config_COMPILER_ASTANALYZER_PLUGIN.hpp"
#include "config_COMPILER_CUSTOMSROA_PLUGIN.hpp"
#include "config_COMPILER_EMPTY_PLUGIN.hpp"
#include "config_COMPILER_EXPANDMEMOPS_PLUGIN.hpp"
#include "config_COMPILER_OPENMP_PLUGIN.hpp"
#include "config_COMPILER_SSA_PLUGIN.hpp"
#include "config_COMPILER_SSA_PLUGINCPP.hpp"
#include "config_COMPILER_TOPFNAME_PLUGIN.hpp"
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
#include "config_I386_CLANG10_EXE.hpp"
#include "config_I386_CLANG10_PLUGIN_DIR.hpp"
#include "config_I386_CLANG10_VERSION.hpp"
#include "config_I386_CLANG11_EXE.hpp"
#include "config_I386_CLANG11_PLUGIN_DIR.hpp"
#include "config_I386_CLANG11_VERSION.hpp"
#include "config_I386_CLANG12_EXE.hpp"
#include "config_I386_CLANG12_PLUGIN_DIR.hpp"
#include "config_I386_CLANG12_VERSION.hpp"
#include "config_I386_CLANG13_EXE.hpp"
#include "config_I386_CLANG13_PLUGIN_DIR.hpp"
#include "config_I386_CLANG13_VERSION.hpp"
#include "config_I386_CLANG16_EXE.hpp"
#include "config_I386_CLANG16_PLUGIN_DIR.hpp"
#include "config_I386_CLANG16_VERSION.hpp"
#include "config_I386_CLANG19_EXE.hpp"
#include "config_I386_CLANG19_PLUGIN_DIR.hpp"
#include "config_I386_CLANG19_VERSION.hpp"
#include "config_I386_CLANG4_EXE.hpp"
#include "config_I386_CLANG4_PLUGIN_DIR.hpp"
#include "config_I386_CLANG4_VERSION.hpp"
#include "config_I386_CLANG5_EXE.hpp"
#include "config_I386_CLANG5_PLUGIN_DIR.hpp"
#include "config_I386_CLANG5_VERSION.hpp"
#include "config_I386_CLANG6_EXE.hpp"
#include "config_I386_CLANG6_PLUGIN_DIR.hpp"
#include "config_I386_CLANG6_VERSION.hpp"
#include "config_I386_CLANG7_EXE.hpp"
#include "config_I386_CLANG7_PLUGIN_DIR.hpp"
#include "config_I386_CLANG7_VERSION.hpp"
#include "config_I386_CLANG8_EXE.hpp"
#include "config_I386_CLANG8_PLUGIN_DIR.hpp"
#include "config_I386_CLANG8_VERSION.hpp"
#include "config_I386_CLANG9_EXE.hpp"
#include "config_I386_CLANG9_PLUGIN_DIR.hpp"
#include "config_I386_CLANG9_VERSION.hpp"
#include "config_I386_CLANGPP10_EXE.hpp"
#include "config_I386_CLANGPP11_EXE.hpp"
#include "config_I386_CLANGPP12_EXE.hpp"
#include "config_I386_CLANGPP13_EXE.hpp"
#include "config_I386_CLANGPP16_EXE.hpp"
#include "config_I386_CLANGPP19_EXE.hpp"
#include "config_I386_CLANGPP4_EXE.hpp"
#include "config_I386_CLANGPP5_EXE.hpp"
#include "config_I386_CLANGPP6_EXE.hpp"
#include "config_I386_CLANGPP7_EXE.hpp"
#include "config_I386_CLANGPP8_EXE.hpp"
#include "config_I386_CLANGPP9_EXE.hpp"
#include "config_I386_CLANG_CPP10_EXE.hpp"
#include "config_I386_CLANG_CPP11_EXE.hpp"
#include "config_I386_CLANG_CPP12_EXE.hpp"
#include "config_I386_CLANG_CPP13_EXE.hpp"
#include "config_I386_CLANG_CPP16_EXE.hpp"
#include "config_I386_CLANG_CPP19_EXE.hpp"
#include "config_I386_CLANG_CPP4_EXE.hpp"
#include "config_I386_CLANG_CPP5_EXE.hpp"
#include "config_I386_CLANG_CPP6_EXE.hpp"
#include "config_I386_CLANG_CPP7_EXE.hpp"
#include "config_I386_CLANG_CPP8_EXE.hpp"
#include "config_I386_CLANG_CPP9_EXE.hpp"
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
#include "config_I386_LLVM19_LINK_EXE.hpp"
#include "config_I386_LLVM19_OPT_EXE.hpp"
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
#include "config_PANDA_LIB_INSTALLDIR.hpp"

#include <algorithm>
#include <cerrno>
#include <fstream>
#include <list>
#include <random>
#include <regex>
#include <string>
#include <unistd.h>

#define SCRIPT_NEWLINE " \\\n  "

enum CompilerMode : int
{
   CM_EMPTY = 1 << 0,              // Empty file compilation
   CM_ANALYZER_INTERFACE = 1 << 1, // Enable frontend code analyzer plugins
   CM_ANALYZER_OPTIMIZE = 1 << 2,  // Enable frontend code optimizer plugins
   CM_ANALYZER_ALL = 3 << 1,       // Enable all frontend plugins
   CM_OPT_CUSTOMSROA = 1 << 7,     // Enable custom scalar replacement of aggregates plugin
   CM_OPT_INTERNALIZE = 1 << 8,    // Enable symbol internalize plugin
   CM_OPT_EXPANDMEMOPS = 1 << 9,   // Enable memory operation optimizer plugin
   CM_OPT_DUMPBAMBUIR = 1 << 10,   // Enable IR dump plugin
   CM_OPT_OPENMP = 1 << 11,        // Enable OpenMP plugin
   CM_OPT_ALL = (15 << 7),         // Enable backend HLS optimization plugins
   CM_LTO_FLAG = 1 << 16,          // Enable LTO optimization flags
   CM_COMPILER_STD = 1 << 24,      // Use default compiler
   CM_COMPILER_OPT = 1 << 25,      // Use compiler optimizer
   CM_COMPILER_LTO = 1 << 26,      // Use compiler linker
};

static std::string __escape_define(const std::string& str)
{
   return std::regex_replace(str, std::regex("([\\(\\) ])"), "\\$1");
}

namespace
{
   std::string trim_whitespace(const std::string& value)
   {
      const auto begin = value.find_first_not_of(" \t\r\n");
      if(begin == std::string::npos)
      {
         return "";
      }
      const auto end = value.find_last_not_of(" \t\r\n");
      return value.substr(begin, end - begin + 1);
   }

   std::vector<std::string> split_shell_words(const std::string& command_line)
   {
      std::vector<std::string> words;
      std::string current_word;
      char quote = '\0';
      bool escaping = false;

      for(const auto c : command_line)
      {
         if(escaping)
         {
            current_word += c;
            escaping = false;
            continue;
         }

         if(quote == '\'')
         {
            if(c == '\'')
            {
               quote = '\0';
            }
            else
            {
               current_word += c;
            }
            continue;
         }

         if(quote == '"')
         {
            if(c == '"')
            {
               quote = '\0';
            }
            else if(c == '\\')
            {
               escaping = true;
            }
            else
            {
               current_word += c;
            }
            continue;
         }

         if(c == '\\')
         {
            escaping = true;
         }
         else if(c == '\'' || c == '"')
         {
            quote = c;
         }
         else if(c == ' ' || c == '\t' || c == '\r' || c == '\n')
         {
            if(!current_word.empty())
            {
               words.push_back(current_word);
               current_word.clear();
            }
         }
         else
         {
            current_word += c;
         }
      }

      if(escaping)
      {
         current_word += '\\';
      }
      if(!current_word.empty())
      {
         words.push_back(current_word);
      }

      return words;
   }

   std::string join_shell_words(const std::vector<std::string>& words)
   {
      std::string command_line;
      for(const auto& word : words)
      {
         if(!command_line.empty())
         {
            command_line += " ";
         }
         command_line += shell_escape_argument(word);
      }
      return command_line;
   }

   std::vector<std::filesystem::path> parse_include_search_paths(const std::filesystem::path& include_file)
   {
      std::ifstream includefile(include_file);
      if(!includefile.is_open())
      {
         THROW_ERROR_USAGE(
             "Failed to read compiler system include paths. Check compiler installation and permissions.");
      }

      enum class parse_state
      {
         before_system_includes,
         in_system_includes
      };

      parse_state state = parse_state::before_system_includes;
      std::vector<std::filesystem::path> includes;
      std::string line;

      while(getline(includefile, line))
      {
         const auto trimmed = trim_whitespace(line);
         if(state == parse_state::before_system_includes)
         {
            if(trimmed == "#include <...> search starts here:")
            {
               state = parse_state::in_system_includes;
            }
            continue;
         }

         if(trimmed == "End of search list." || trimmed == "End of framework search list." ||
            trimmed.find("search ends here:") != std::string::npos)
         {
            break;
         }
         if(trimmed.empty())
         {
            continue;
         }

         static const std::string framework_suffix = " (framework directory)";
         auto include_dir = trimmed;
         if(ends_with(include_dir, framework_suffix))
         {
            include_dir.resize(include_dir.size() - framework_suffix.size());
         }

         std::error_code ec;
         auto include_path = std::filesystem::path(include_dir);
         if(std::filesystem::exists(include_path, ec))
         {
            const auto canonical_path = std::filesystem::weakly_canonical(include_path, ec);
            if(!ec)
            {
               include_path = canonical_path;
            }
         }
         else
         {
            include_path = include_path.lexically_normal();
         }

         if(std::find(includes.begin(), includes.end(), include_path) == includes.end())
         {
            includes.push_back(include_path);
         }
      }

      return includes;
   }
} // namespace

std::string CompilerWrapper::bambu_ir_info;

class passesType
{
   std::string passes;
   CompilerWrapper_CompilerTarget target;

 public:
   passesType(CompilerWrapper_CompilerTarget _target) : target(_target)
   {
   }
   void add_pass(std::string pass, std::string O_level = "")
   {
      THROW_ASSERT((!pass.empty() && O_level.empty()) || (pass.empty() && !O_level.empty()), "unexpected condition");
      if(O_level.size())
      {
         if(CompilerWrapper::isCurrentOrNewer(target, CompilerWrapper_CompilerTarget::CT_I386_CLANG16))
         {
            pass = "'default<O" + O_level + ">'";
         }
         else
         {
            pass = "-O" + O_level;
         }
      }
      if(passes.empty())
      {
         if(CompilerWrapper::isCurrentOrNewer(target, CompilerWrapper_CompilerTarget::CT_I386_CLANG16))
         {
            passes = "--passes=";
         }
         else
         {
            passes = "-";
         }
      }
      else
      {
         if(CompilerWrapper::isCurrentOrNewer(target, CompilerWrapper_CompilerTarget::CT_I386_CLANG16))
         {
            passes += ",";
         }
         else
         {
            passes += " -";
         }
      }
      passes += pass;
   }
   const std::string& get()
   {
      return passes;
   }
};

CompilerWrapper::CompilerWrapper(const ParameterConstRef _Param, const CompilerWrapper_CompilerTarget _compiler_target,
                                 const CompilerWrapper_OptimizationSet _OS)
    : Param(_Param),
      compiler_target(_compiler_target),
      OS(_OS),
      cpp_input(_Param->isOption(OPT_input_format) &&
                (_Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP ||
                 _Param->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LLVM_CPP)),
      output_level(_Param->getOption<int>(OPT_output_level)),
      debug_level(_Param->get_class_debug_level("CompilerWrapper"))
{
   InitializeCompilerParameters();
}

void CompilerWrapper::CompileFile(std::string& input_filename, const std::string& output_filename,
                                  const std::string& parameters_line, passesType passes, int cm,
                                  const std::string& costTable)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Compiling " + input_filename);
   THROW_ASSERT(cm == CM_EMPTY || (cm & ~CM_EMPTY) == cm,
                "Empty compilation must not require any other compilation mode.");

   const auto compiler = GetCompiler();
   const auto output_temporary_directory = Param->getOption<std::string>(OPT_output_temporary_directory);
   const auto compiler_output_filename = output_temporary_directory + "/" STR_CST_cc_output;

   const auto isWholeProgram =
       Param->isOption(OPT_cc_optimizations) &&
       Param->getOption<std::string>(OPT_cc_optimizations).find("whole-program") != std::string::npos &&
       Param->getOption<std::string>(OPT_cc_optimizations).find("no-whole-program") == std::string::npos;
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
   std::string load_prefix = "";
   std::string command;
   if(cm & CM_COMPILER_OPT)
   {
      if(cm & CM_ANALYZER_ALL)
      {
         THROW_WARNING("AST analyzer plugin not executed on LLVM IR input.");
         cm &= ~CM_ANALYZER_ALL;
      }
      THROW_ASSERT(!(cm & CM_ANALYZER_ALL), "Analyzer plugin requires compiler frontend to run");
      load_prefix += compiler.llvm_opt;
   }
   else if(cm & CM_COMPILER_LTO)
   {
      THROW_ASSERT(cm == CM_COMPILER_LTO, "Plugins must not be enabled when linker is required");
      load_prefix += compiler.llvm_link;
   }
   else
   {
      cm |= CM_COMPILER_STD;
      load_prefix += compiler.cc;

      command += " -c";
      command += " -D__NO_INLINE__"; /// needed to avoid problem with glibc inlines
      command += " " + compiler.extra_options;

      if((Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy)))
      {
         command += " -D__BAMBU_DISCREPANCY__";
      }
   }

   if(cm & CM_LTO_FLAG)
   {
      THROW_ASSERT(cm & CM_COMPILER_STD, "Unexpected compiler type");
      command += " -flto";
   }

   if(cm & CM_EMPTY)
   {
      if(input_filename == "-")
      {
         THROW_ERROR_USAGE("Reading from standard input did not provide any function definition. "
                           "Provide a source file or valid input on stdin.");
      }
      static int empty_counter = 0;
      real_filename = output_temporary_directory + "/empty_" + std::to_string(empty_counter++) + ".c";
      CopyFile(input_filename, real_filename);
      {
         std::ofstream empty_file(real_filename, std::ios_base::app);
         empty_file << "\nvoid __bambu_empty_function__(){}\n";
      }
      load_prefix += SCRIPT_NEWLINE + load_plugin(COMPILER_EMPTY_PLUGIN);
      command += SCRIPT_NEWLINE "-mllvm -pandaGE-outputdir=" + output_temporary_directory +
                 SCRIPT_NEWLINE "-mllvm -pandaGE-infile=" + real_filename;
   }

   if(cm & CM_ANALYZER_ALL)
   {
      /// remove some extra option not compatible with clang
      boost::replace_all(command, "-mlong-double-64", "");
      load_prefix += SCRIPT_NEWLINE "-fplugin=" + compiler.GetAnalyzerPluginObject(COMPILER_ASTANALYZER_PLUGIN);
      command += SCRIPT_NEWLINE "-include csroa_directives.h";
      command += SCRIPT_NEWLINE "-Xclang -add-plugin -Xclang " COMPILER_ASTANALYZER_PLUGIN;

      if(cm & CM_ANALYZER_INTERFACE)
      {
         command += SCRIPT_NEWLINE "-Xclang -plugin-arg-" COMPILER_ASTANALYZER_PLUGIN " -Xclang -action";
         command += SCRIPT_NEWLINE "-Xclang -plugin-arg-" COMPILER_ASTANALYZER_PLUGIN " -Xclang analyze";
         command += SCRIPT_NEWLINE "-Xclang -plugin-arg-" COMPILER_ASTANALYZER_PLUGIN " -Xclang -outputdir";
         command +=
             SCRIPT_NEWLINE "-Xclang -plugin-arg-" COMPILER_ASTANALYZER_PLUGIN " -Xclang " + output_temporary_directory;

         if(cpp_input)
         {
            command += SCRIPT_NEWLINE "-Xclang -plugin-arg-" COMPILER_ASTANALYZER_PLUGIN " -Xclang -cppflag";
         }
      }
      if(cm & CM_ANALYZER_OPTIMIZE)
      {
         command += SCRIPT_NEWLINE "-Xclang -plugin-arg-" COMPILER_ASTANALYZER_PLUGIN " -Xclang -action";
         command += SCRIPT_NEWLINE "-Xclang -plugin-arg-" COMPILER_ASTANALYZER_PLUGIN " -Xclang optimize";
      }
   }

   const auto load_and_run_plugin = [&](const std::string& plugin_name) {
      if(cm & CM_COMPILER_STD)
      {
         load_prefix += SCRIPT_NEWLINE + load_plugin(plugin_name);
      }
      else
      {
         load_prefix += SCRIPT_NEWLINE + load_plugin_opt(plugin_name);
         passes.add_pass(plugin_name);
      }
   };
   const auto append_arg = [&](const std::string& arg) {
      if(cm & CM_COMPILER_STD)
      {
         command += SCRIPT_NEWLINE "-mllvm " + arg;
      }
      else
      {
         command += " " + arg;
      }
   };
   if((cm & CM_OPT_CUSTOMSROA) && compiler_target != CompilerWrapper_CompilerTarget::CT_I386_CLANG4)
   {
      append_arg("-panda-outputdir-csroa=" + output_temporary_directory);
      if(top_fnames.size())
      {
         append_arg("-panda-TFN-csroa=" + top_fnames);
      }
      if(Param->IsParameter("panda-lock-csroa") && Param->GetParameter<int>("panda-lock-csroa"))
      {
         append_arg("-panda-lock-csroa");
      }
      load_and_run_plugin(COMPILER_CUSTOMSROA_PLUGIN);
   }
   if((cm & CM_OPT_INTERNALIZE) && top_fnames.size())
   {
      THROW_ASSERT(!(cm & CM_LTO_FLAG), "Internalizing symbols in partial object files is not expected");
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
      load_and_run_plugin(COMPILER_TOPFNAME_PLUGIN);
   }
   if(cm & CM_OPT_EXPANDMEMOPS)
   {
      load_and_run_plugin(COMPILER_EXPANDMEMOPS_PLUGIN);
   }
   if(cm & CM_OPT_OPENMP)
   {
      load_and_run_plugin(COMPILER_OPENMP_PLUGIN);
      command += " -dflt_nthreads=" + Param->getOption<std::string>(OPT_num_accelerators);
   }

   command += SCRIPT_NEWLINE + parameters_line;

   if(cm & CM_OPT_DUMPBAMBUIR)
   {
      append_arg("-panda-outputdir=" + output_temporary_directory);
      append_arg("-panda-infile=" + input_filename);
      append_arg("-panda-cost-table=\"" + costTable + "\"");
      if(top_fnames.size())
      {
         append_arg("-panda-topfname=" + top_fnames);
      }
      (cpp_input && !(cm & CM_COMPILER_OPT)) ? load_and_run_plugin(COMPILER_SSA_PLUGINCPP) :
                                               load_and_run_plugin(COMPILER_SSA_PLUGIN);
      // if(cm & CM_COMPILER_STD)
      // {
      //    command += " -S -emit-llvm";
      // }
   }
   command += SCRIPT_NEWLINE + passes.get();

   const auto _output_filename = output_filename.size() ?
                                     output_filename :
                                     (output_temporary_directory + "/" + unique_path(STR_CST_cc_obj_file).string());
   command += SCRIPT_NEWLINE "-o " + _output_filename;

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
   command = load_prefix + SCRIPT_NEWLINE + command;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Invoke: " + command);
#if !NPROFILE
   long int cc_compilation_time = 0;
   if(output_level >= OUTPUT_LEVEL_VERBOSE)
   {
      START_TIME(cc_compilation_time);
   }
#endif
   const auto ret = PandaSystem(Param, command, false, compiler_output_filename);
#if !NPROFILE
   if(output_level >= OUTPUT_LEVEL_VERBOSE)
   {
      STOP_TIME(cc_compilation_time);
      dump_exec_time("Compilation time", cc_compilation_time);
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

void CompilerWrapper::FillIRManager(const ir_managerRef TM, std::vector<std::string>& source_files,
                                    const std::string& costTable)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Invoking front-end compiler");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   if(source_files.size() == 0)
   {
      THROW_ERROR_USAGE("No input files specified for parsing. Provide at least one source file.");
   }

   const auto compiler = GetCompiler();
   const auto multi_source = source_files.size() > 1;
   const auto enable_LTO = multi_source;
   const auto compile_only = Param->isOption(OPT_cc_S) && Param->getOption<bool>(OPT_cc_S);
   const auto preprocess_only = Param->isOption(OPT_cc_E) && Param->getOption<bool>(OPT_cc_E);
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
            THROW_ERROR_USAGE("Option --aligned-access cannot be used with -O3 or vectorization. "
                              "Disable vectorization or remove --aligned-access.");
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting compilation of single files");
   const auto compiler_mode = [&]() -> int {
      int flags = 0;
      if(preprocess_only || compile_only)
      {
         return flags;
      }
      flags |= CM_ANALYZER_ALL;
      if(Param->IsParameter("enable_CUSTOM-SROA") && Param->GetParameter<int>("enable_CUSTOM-SROA") == 1)
      {
         flags |= CM_OPT_CUSTOMSROA;
      }
      if(multi_source)
      {
         flags |= CM_LTO_FLAG;
         if(!enable_LTO)
         {
            flags |= CM_OPT_DUMPBAMBUIR;
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
   passesType passes(compiler_target);
   for(auto& source_file : source_files)
   {
      const auto leaf_name = source_file == "-" ? "stdin-" : std::filesystem::path(source_file).filename().string();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Compiling file " + source_file);
      const auto obj_file = ((compile_only || preprocess_only) && Param->isOption(OPT_output_file)) ?
                                Param->getOption<std::string>(OPT_output_file) :
                                unique_path(output_temporary_directory + "/" + leaf_name + ".%%%%%%.o").string();
      const auto is_llvmir = std::filesystem::path(source_file).extension() == ".ll";
      const auto local_compiler_mode = (is_llvmir ? CM_COMPILER_OPT : CM_COMPILER_STD) | compiler_mode;
      auto local_compiler_parameters = is_llvmir ? "" : frontend_compiler_parameters;
      if(Param->isOption(OPT_openmp) && Param->getOption<bool>(OPT_openmp))
      {
         local_compiler_parameters += " -fopenmp ";
      }
      CompileFile(source_file, obj_file, local_compiler_parameters, passes, local_compiler_mode, costTable);
      if(enable_LTO)
      {
         obj_files.push_back(obj_file);
      }
      else if(!(compile_only || preprocess_only))
      {
         auto bambu_ir_file = output_temporary_directory + "/" + leaf_name + STR_CST_bambu_ir_suffix;
         if(!std::filesystem::exists(bambu_ir_file))
         {
            CompileFile(source_file, "", frontend_compiler_parameters, passes, CM_EMPTY, costTable);
            // source_file has been changed by previous call to CompileFile
            bambu_ir_file = output_temporary_directory + "/" + std::filesystem::path(source_file).filename().string() +
                            STR_CST_bambu_ir_suffix;
         }
         obj_files.push_back(bambu_ir_file);
      }
   }

   if(enable_LTO)
   {
      const auto leaf_name = std::filesystem::path(source_files.front()).filename().string();
      const auto ext_symbols_filename = output_temporary_directory + "/external-symbols.txt";
      std::string lto_source = container_to_string(obj_files, STR_CST_string_separator);
      std::string lto_obj = output_temporary_directory + "/" + leaf_name + ".lto.bc";
      CompileFile(lto_source, lto_obj, "", passes, CM_COMPILER_LTO, "");

      lto_source = lto_obj;
      lto_obj = output_temporary_directory + "/" + leaf_name + ".lto-opt.bc";
      std::string opt_command;
      opt_command = clang_recipes(optimization_set, passes);
      CompileFile(lto_source, lto_obj, opt_command, passes, CM_COMPILER_OPT | CM_OPT_INTERNALIZE, costTable);

      lto_source = lto_obj;
      lto_obj = output_temporary_directory + "/" + leaf_name + ".lto-dump.bc";
      THROW_ASSERT(std::filesystem::exists(ext_symbols_filename), "File not found: " + ext_symbols_filename);
      passesType passesSecond(compiler_target);
      passesSecond.add_pass("internalize");
      opt_command = " --internalize-public-api-file=" + ext_symbols_filename + " " +
                    clang_recipes(optimization_set, passesSecond) +
                    " -panda-infile=" + container_to_string(source_files, ",");
      auto compile_config = CM_COMPILER_OPT | CM_OPT_DUMPBAMBUIR;
      if(Param->isOption(OPT_openmp) && Param->getOption<bool>(OPT_openmp))
      {
         compile_config |= CM_OPT_OPENMP;
      }
      CompileFile(lto_source, lto_obj, opt_command, passesSecond, compile_config, costTable);

      const auto bambu_ir_obj = output_temporary_directory + "/" + leaf_name + STR_CST_bambu_ir_suffix;
      if(!std::filesystem::exists(bambu_ir_obj))
      {
         THROW_ERROR_USAGE("Object file not found: " + bambu_ir_obj + ". Check compilation outputs and input paths.");
      }
      ir_managerRef IRM = ParseIRFile(Param, bambu_ir_obj);
#if !NPROFILE
      long int merge_time = 0;
      START_TIME(merge_time);
#endif
      TM->merge_ir_managers(IRM);
#if !NPROFILE
      STOP_TIME(merge_time);
      if(output_level >= OUTPUT_LEVEL_VERBOSE)
      {
         dump_exec_time("IR merging time", merge_time);
      }
#endif
   }
   else if(!Param->isOption(OPT_cc_E) && !Param->isOption(OPT_cc_S))
   {
      for(const auto& obj_file : obj_files)
      {
         if(!std::filesystem::exists(obj_file))
         {
            THROW_ERROR_USAGE("Object file not found: " + obj_file + ". Check compilation outputs and input paths.");
         }
         ir_managerRef IRM = ParseIRFile(Param, obj_file);

#if !NPROFILE
         long int merge_time = 0;
         START_TIME(merge_time);
#endif
         TM->merge_ir_managers(IRM);
#if !NPROFILE
         STOP_TIME(merge_time);
         if(output_level >= OUTPUT_LEVEL_VERBOSE)
         {
            dump_exec_time("IR merging time", merge_time);
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Initializing cc parameters");

   switch(OS)
   {
      case(CompilerWrapper_OptimizationSet::O0):
      case(CompilerWrapper_OptimizationSet::O1):
      case(CompilerWrapper_OptimizationSet::Oz):
      case(CompilerWrapper_OptimizationSet::Os):
      case(CompilerWrapper_OptimizationSet::O2):
      case(CompilerWrapper_OptimizationSet::O3):
      case(CompilerWrapper_OptimizationSet::Ofast):
      case(CompilerWrapper_OptimizationSet::O4):
      case(CompilerWrapper_OptimizationSet::O5):
      {
         frontend_compiler_parameters += (" -O" + WriteOptimizationLevel(OS) + " ");
         break;
      }
      case(CompilerWrapper_OptimizationSet::OBAMBU):
      { /// Filling optimizations map
         SetCompilerDefault();
         SetBambuDefault();
         ReadParameters();

         frontend_compiler_parameters += (" " + WriteOptimizationsString() + " ");
         break;
      }
      default:
      {
         THROW_UNREACHABLE("Unexpected optimization level");
         break;
      }
   }

   /// Adding standard
   if(Param->isOption(OPT_cc_standard))
   {
      auto standard = Param->getOption<std::string>(OPT_cc_standard);
      frontend_compiler_parameters += "--std=" + standard + " ";
   }

   if(Param->isOption(OPT_cc_E) && Param->getOption<bool>(OPT_cc_E))
   {
      frontend_compiler_parameters += "-E ";
   }
   if(Param->isOption(OPT_cc_S) && Param->getOption<bool>(OPT_cc_S))
   {
      frontend_compiler_parameters += "-S ";
   }
   /// Adding defines
   if(Param->isOption(OPT_cc_defines))
   {
      const auto defines = Param->getOption<std::vector<std::string>>(OPT_cc_defines);
      for(const auto& define : defines)
      {
         frontend_compiler_parameters += "-D" + __escape_define(define) + " ";
      }
   }

   /// Adding undefines
   if(Param->isOption(OPT_cc_undefines))
   {
      const auto undefines = Param->getOption<std::vector<std::string>>(OPT_cc_undefines);
      for(const auto& undefine : undefines)
      {
         frontend_compiler_parameters += "-U" + __escape_define(undefine) + " ";
      }
   }

   /// Adding warnings
   if(Param->isOption(OPT_cc_warnings))
   {
      const auto warnings = Param->getOption<std::vector<std::string>>(OPT_cc_warnings);
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
   if(Param->isOption(OPT_cc_includes))
   {
      frontend_compiler_parameters += Param->getOption<std::string>(OPT_cc_includes) + " ";
   }

   /// Adding libraries
   if(Param->isOption(OPT_cc_libraries))
   {
      const auto libraries = Param->getOption<std::vector<std::string>>(OPT_cc_libraries);
      for(const auto& library : libraries)
      {
         if(starts_with(library, "m_"))
         {
            compiler_linking_parameters += "-lm ";
         }
         else
         {
            compiler_linking_parameters += "-l" + library + " ";
         }
      }
   }

   /// Adding library directories
   if(Param->isOption(OPT_cc_library_directories))
   {
      const auto library_directories = Param->getOption<std::vector<std::string>>(OPT_cc_library_directories);
      for(const auto& library_directory : library_directories)
      {
         compiler_linking_parameters += "-L" + library_directory + " ";
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Initialized cc parameters");
}

void CompilerWrapper::SetBambuDefault()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Setting parameters for Bambu tool...");
   const auto opt_level = Param->getOption<CompilerWrapper_OptimizationSet>(OPT_compiler_opt_level);
   const auto compiler = Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);

   /// parameters with enable
   optimization_flags["wrapv"] = true; /// bambu assumes twos complement arithmetic

   // NOTE: compiler's builtins should not be used, but printf support is available just because
   //       of builtins magic, thus it is not possible to disable all of them
   // optimization_flags["builtin"] = false; /// avoid compiler's builtin

   optimization_flags["builtin-memset"] = false;
   optimization_flags["builtin-memcpy"] = false;
   optimization_flags["builtin-memmove"] = false;

   if(opt_level != CompilerWrapper_OptimizationSet::O3 && opt_level != CompilerWrapper_OptimizationSet::Ofast &&
      opt_level != CompilerWrapper_OptimizationSet::O4 && opt_level != CompilerWrapper_OptimizationSet::O5)
   {
      // it is preferable to have unrolling disabled by default
      optimization_flags["unroll-loops"] = false;
   }

   if(isCurrentOrNewer(compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG9))
   {
      optimization_flags["builtin-bcmp"] = false;
   }

   if(isCurrentOrNewer(compiler, CompilerWrapper_CompilerTarget::CT_I386_CLANG16))
   {
      optimization_flags["fp-contract"] = false;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Set parameters for bambu tool");
}

void CompilerWrapper::SetCompilerDefault()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Setting front-end compiler defaults");
   const auto optimization_set = Param->getOption<CompilerWrapper_OptimizationSet>(OPT_compiler_opt_level);

   // In Ubuntu 6.10 and later versions this option is enabled by default for C, C++, ObjC, ObjC++
   optimization_flags["stack-protector"] = false;

   switch(optimization_set)
   {
      case(CompilerWrapper_OptimizationSet::O1):
      case(CompilerWrapper_OptimizationSet::Oz):
      case(CompilerWrapper_OptimizationSet::Os):
      case(CompilerWrapper_OptimizationSet::O2):
      case(CompilerWrapper_OptimizationSet::O3):
      case(CompilerWrapper_OptimizationSet::Ofast):
      case(CompilerWrapper_OptimizationSet::O4):
      case(CompilerWrapper_OptimizationSet::O5):
      {
         frontend_compiler_parameters += (" -O" + WriteOptimizationLevel(optimization_set) + " ");
         break;
      }
      case(CompilerWrapper_OptimizationSet::O0):
      {
         frontend_compiler_parameters += " -O1 ";
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
   if(isCurrentOrNewer(compiler_target, CompilerWrapper_CompilerTarget::CT_I386_CLANG12) &&
      optimization_set != CompilerWrapper_OptimizationSet::O0 // -O0 optimization creates problems to clang
                                                              // when loop flattening is enabled.
      && (!Param->isOption(OPT_openmp) || !Param->getOption<bool>(OPT_openmp)))
   {
      frontend_compiler_parameters += " -mllvm -enable-loop-flatten ";
   }
   optimization_flags["vectorize"] = false;     /// disable vectorization
   optimization_flags["slp-vectorize"] = false; /// disable superword-level parallelism vectorization

   /// in case we are compiling C++ code
   if(cpp_input)
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
#if HAVE_ASSERTS
   CompilerWrapper_CompilerTarget compatible_compilers =
       Param->getOption<CompilerWrapper_CompilerTarget>(OPT_compatible_compilers);
#endif

   std::string cc_extra_options;
   if(Param->isOption(OPT_cc_extra_options))
   {
      cc_extra_options = Param->getOption<std::string>(OPT_cc_extra_options);
   }

   CompilerWrapper_CompilerTarget preferred_compiler;
   if(compiler_target == CompilerWrapper_CompilerTarget::CT_NO_COMPILER)
   {
      preferred_compiler = Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
   }
   else
   {
#if HAVE_ASSERTS
      const bool debug_condition = static_cast<int>(compiler_target) & static_cast<int>(compatible_compilers);
      THROW_ASSERT(debug_condition,
                   "Required compiler is not among the compatible one: " + STR(static_cast<int>(compiler_target)) +
                       " vs " + STR(static_cast<int>(compatible_compilers)));
#endif
      preferred_compiler = compiler_target;
   }
   const std::string plugin_root_dir =
       (Param->isOption(OPT_cc_plugindir) ? Param->getOption<std::string>(OPT_cc_plugindir) :
                                            relocate_install_path(PANDA_LIB_INSTALLDIR, true).string()) +
       "/";

#if HAVE_I386_CLANG4_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG4))
   {
      compiler.cc = cpp_input ? I386_CLANGPP4_EXE : I386_CLANG4_EXE;
      compiler.cpp = I386_CLANG_CPP4_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG4_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM4_LINK_EXE;
      compiler.llvm_opt = I386_LLVM4_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG5_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG5))
   {
      compiler.cc = cpp_input ? I386_CLANGPP5_EXE : I386_CLANG5_EXE;
      compiler.cpp = I386_CLANG_CPP5_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG5_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM5_LINK_EXE;
      compiler.llvm_opt = I386_LLVM5_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG6_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG6))
   {
      compiler.cc = cpp_input ? I386_CLANGPP6_EXE : I386_CLANG6_EXE;
      compiler.cpp = I386_CLANG_CPP6_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG6_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM6_LINK_EXE;
      compiler.llvm_opt = I386_LLVM6_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG7_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG7))
   {
      compiler.cc = cpp_input ? I386_CLANGPP7_EXE : I386_CLANG7_EXE;
      compiler.cpp = I386_CLANG_CPP7_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG7_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM7_LINK_EXE;
      compiler.llvm_opt = I386_LLVM7_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG8_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG8))
   {
      compiler.cc = cpp_input ? I386_CLANGPP8_EXE : I386_CLANG8_EXE;
      compiler.cpp = I386_CLANG_CPP8_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG8_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM8_LINK_EXE;
      compiler.llvm_opt = I386_LLVM8_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG9_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG9))
   {
      compiler.cc = cpp_input ? I386_CLANGPP9_EXE : I386_CLANG9_EXE;
      compiler.cpp = I386_CLANG_CPP9_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG9_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM9_LINK_EXE;
      compiler.llvm_opt = I386_LLVM9_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG10_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG10))
   {
      compiler.cc = cpp_input ? I386_CLANGPP10_EXE : I386_CLANG10_EXE;
      compiler.cpp = I386_CLANG_CPP10_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG10_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM10_LINK_EXE;
      compiler.llvm_opt = I386_LLVM10_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG11_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG11))
   {
      compiler.cc = cpp_input ? I386_CLANGPP11_EXE : I386_CLANG11_EXE;
      compiler.cpp = I386_CLANG_CPP11_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG11_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM11_LINK_EXE;
      compiler.llvm_opt = I386_LLVM11_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG12_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG12))
   {
      compiler.cc = cpp_input ? I386_CLANGPP12_EXE : I386_CLANG12_EXE;
      compiler.cpp = I386_CLANG_CPP12_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG12_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM12_LINK_EXE;
      compiler.llvm_opt = I386_LLVM12_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG13_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG13))
   {
      compiler.cc = cpp_input ? I386_CLANGPP13_EXE : I386_CLANG13_EXE;
      compiler.cpp = I386_CLANG_CPP13_EXE;
      compiler.extra_options = cc_extra_options;
      compiler.extra_options += " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG13_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM13_LINK_EXE;
      compiler.llvm_opt = I386_LLVM13_OPT_EXE;
   }
#endif

#if HAVE_I386_CLANG16_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG16))
   {
      compiler.cc = cpp_input ? I386_CLANGPP16_EXE : I386_CLANG16_EXE;
      compiler.cpp = I386_CLANG_CPP16_EXE;
      compiler.extra_options = cc_extra_options + " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG16_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM16_LINK_EXE;
      compiler.llvm_opt = I386_LLVM16_OPT_EXE;
      if(!Param->IsParameter("opaque-pointers") || !Param->GetParameter<int>("opaque-pointers"))
      {
         compiler.extra_options += " -Xclang -no-opaque-pointers";
         compiler.llvm_link += " --opaque-pointers=0";
         compiler.llvm_opt += " --opaque-pointers=0";
      }
   }
#endif

#if HAVE_I386_CLANG19_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG19))
   {
      compiler.cc = cpp_input ? I386_CLANGPP19_EXE : I386_CLANG19_EXE;
      compiler.cpp = I386_CLANG_CPP19_EXE;
      compiler.extra_options = cc_extra_options + " " + Param->getOption<std::string>(OPT_cc_m_env);
      compiler.plugin_dir = compiler.analyzer_plugin_dir = plugin_root_dir + I386_CLANG19_PLUGIN_DIR "/";
      compiler.llvm_link = I386_LLVM19_LINK_EXE;
      compiler.llvm_opt = I386_LLVM19_OPT_EXE;
   }
#endif

   if(compiler.cc == "")
   {
      THROW_ERROR_USAGE("No compatible frontend compiler was found. "
                        "Install a supported compiler or adjust --compiler.");
   }

   return compiler;
}

std::string CompilerWrapper::GetAnalyzeCompiler() const
{
#if HAVE_I386_CLANG19_COMPILER
   return cpp_input ? I386_CLANGPP19_EXE : I386_CLANG19_EXE;
#elif HAVE_I386_CLANG16_COMPILER
   return cpp_input ? I386_CLANGPP16_EXE : I386_CLANG16_EXE;
#elif HAVE_I386_CLANG13_COMPILER
   return cpp_input ? I386_CLANGPP13_EXE : I386_CLANG13_EXE;
#elif HAVE_I386_CLANG12_COMPILER
   return cpp_input ? I386_CLANGPP12_EXE : I386_CLANG12_EXE;
#elif HAVE_I386_CLANG11_COMPILER
   return cpp_input ? I386_CLANGPP11_EXE : I386_CLANG11_EXE;
#elif HAVE_I386_CLANG10_COMPILER
   return cpp_input ? I386_CLANGPP10_EXE : I386_CLANG10_EXE;
#elif HAVE_I386_CLANG9_COMPILER
   return cpp_input ? I386_CLANGPP9_EXE : I386_CLANG9_EXE;
#elif HAVE_I386_CLANG8_COMPILER
   return cpp_input ? I386_CLANGPP8_EXE : I386_CLANG8_EXE;
#elif HAVE_I386_CLANG7_COMPILER
   return cpp_input ? I386_CLANGPP7_EXE : I386_CLANG7_EXE;
#elif HAVE_I386_CLANG6_COMPILER
   return cpp_input ? I386_CLANGPP6_EXE : I386_CLANG6_EXE;
#elif HAVE_I386_CLANG5_COMPILER
   return cpp_input ? I386_CLANGPP5_EXE : I386_CLANG5_EXE;
#elif HAVE_I386_CLANG4_COMPILER
   return cpp_input ? I386_CLANGPP4_EXE : I386_CLANG4_EXE;
#else
   THROW_ERROR("unexpected condition");
   return "";
#endif
}

std::vector<std::filesystem::path> CompilerWrapper::GetSystemIncludes() const
{
   const auto compiler = GetCompiler();
   const auto include_file = Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) / "__include";
   const auto query_language = [&]() {
      if(Param->isOption(OPT_cc_xlang))
      {
         const auto xlang = Param->getOption<std::string>(OPT_cc_xlang);
         if(xlang.find("c++") != std::string::npos)
         {
            return std::string("c++");
         }
         if(xlang == "c" || xlang == "c-header")
         {
            return std::string("c");
         }
      }
      return cpp_input ? std::string("c++") : std::string("c");
   }();

   const auto query_options = [&]() {
      std::vector<std::string> preserved_options;

      if(Param->isOption(OPT_cc_m_env))
      {
         const auto m_env = trim_whitespace(Param->getOption<std::string>(OPT_cc_m_env));
         if(!m_env.empty())
         {
            preserved_options.push_back(m_env);
         }
      }

      if(Param->isOption(OPT_cc_extra_options))
      {
         const auto extra_options = split_shell_words(Param->getOption<std::string>(OPT_cc_extra_options));
         const auto push_option_with_value = [&](const std::string& option, const std::string& value) {
            preserved_options.push_back(option);
            preserved_options.push_back(value);
         };

         for(size_t index = 0; index < extra_options.size(); ++index)
         {
            const auto& option = extra_options[index];
            if(option == "-target" || option == "-isysroot" || option == "-gcc-toolchain" ||
               option == "-resource-dir" || option == "-stdlib")
            {
               if(index + 1 < extra_options.size())
               {
                  push_option_with_value(option, extra_options[index + 1]);
                  ++index;
               }
               continue;
            }

            if(option == "-nostdinc" || option == "-nostdinc++" || option == "-nostdlibinc" ||
               option == "-nobuiltininc" || option == "-m32" || option == "-m64" || option == "-mx32" ||
               starts_with(option, "--sysroot=") || starts_with(option, "--target=") ||
               starts_with(option, "--gcc-toolchain=") || starts_with(option, "-resource-dir=") ||
               starts_with(option, "-stdlib="))
            {
               preserved_options.push_back(option);
            }
         }
      }

      return join_shell_words(preserved_options);
   }();

   int last_wstatus = -1;
   const auto query_includes = [&](const std::string& extra_options) {
      auto command = shell_escape_argument(compiler.cc) + " -E -v -x " + shell_escape_argument(query_language);
      if(!extra_options.empty())
      {
         command += " " + extra_options;
      }
      command += " - < /dev/null";

      last_wstatus = PandaSystem(Param, command, false, include_file);
      if(IsError(last_wstatus))
      {
         std::filesystem::remove(include_file);
         return std::vector<std::filesystem::path>();
      }

      auto includes = parse_include_search_paths(include_file);
      std::filesystem::remove(include_file);
      return includes;
   };

   auto includes = query_includes(query_options);
   if(includes.empty() && !query_options.empty())
   {
      includes = query_includes("");
   }
   if(includes.empty())
   {
      util_print_cpu_stats(std::cerr);
      if(last_wstatus != -1 && WIFEXITED(last_wstatus))
      {
         THROW_ERROR_USAGE("Failed to query compiler system include paths from " + compiler.cc +
                           " (return code: " + std::to_string(WEXITSTATUS(last_wstatus)) +
                           "). Check compiler installation and environment.");
      }
      THROW_ERROR_USAGE("Failed to extract compiler system include paths from " + compiler.cc +
                        ". Check compiler installation and environment.");
   }

   return includes;
}

void CompilerWrapper::QueryCompilerConfig(const std::string& compiler_option) const
{
   const auto command = GetCompiler().cc + " " + compiler_option;
   const auto output_file_name =
       Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) / STR_CST_file_IO_shell_output_file;
   if(IsError(PandaSystem(Param, command, false, output_file_name)))
   {
      THROW_ERROR_USAGE(
          "Failed to query compiler configuration. Check compiler installation and command-line options.");
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

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Extra options are " + local_compiler_extra_options);
   command += local_compiler_extra_options + " " + extra_compiler_options + " ";
   boost::replace_all(command, "-target fpga64-xilinx-linux-gnu", "");

   return command;
}

void CompilerWrapper::CreateExecutable(const std::vector<std::filesystem::path>& file_names,
                                       const std::filesystem::path& executable_name,
                                       const std::string& extra_compiler_options,
                                       bool no_frontend_compiler_parameters) const
{
   const auto compiler = GetCompiler();
   std::string command = compiler.cc + " ";
   command += GetCompilerParameters(extra_compiler_options, no_frontend_compiler_parameters);
   command += "-o " + proximate_if_subpath(executable_name, std::filesystem::current_path()).string();

   bool has_cpp_file = false;
   std::string srcs;
   for(const auto& file_name : file_names)
   {
      auto file_format = Param->GetFileFormat(file_name, false);
      if(file_format == Parameters_FileFormat::FF_CPP || file_format == Parameters_FileFormat::FF_LLVM_CPP)
      {
         has_cpp_file = true;
      }
      auto rel_path = proximate_if_subpath(file_name, std::filesystem::current_path());
      srcs += " " + rel_path.string();

      std::string iquote = " -iquote " + rel_path.parent_path().string();
      if(command.find(iquote) == std::string::npos)
      {
         command += iquote;
      }
   }
   if(!has_cpp_file)
   {
      command = std::regex_replace(command, std::regex("[-]{1,2}std=c\\+\\+\\w+"), "");
   }
   command += srcs;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Creating executable " + executable_name.string() + " from" + srcs);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Compilation command is " + command);
   const auto compiler_output_filename =
       Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) / STR_CST_cc_output;

   if(IsError(PandaSystem(Param, command, false, compiler_output_filename)))
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
   if(Param->isOption(OPT_cc_optimizations))
   {
      const auto flags = Param->getOption<std::vector<std::string>>(OPT_cc_optimizations);
      for(const auto& flag : flags)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining parameter " + flag);
         // if the token starts with "no-", the optimization has to be disabled
         const auto enable = flag.find("no-") != 0;
         if(enable)
         {
            optimization_flags[flag] = true;
         }
         else
         {
            optimization_flags[flag.substr(3)] = false;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
   }
   if(Param->isOption(OPT_cc_parameters))
   {
      const auto parameters = Param->getOption<std::vector<std::string>>(OPT_cc_parameters);
      for(const auto& parameter : parameters)
      {
         const size_t equal_size = parameter.find('=');
         if(equal_size == std::string::npos)
         {
            THROW_ERROR_USAGE("Invalid --param argument without value: " + parameter + ". Use --param <name>=<value>.");
         }
         const auto key = parameter.substr(0, equal_size);
         const auto value = parameter.substr(equal_size + 1);
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
      case(CompilerWrapper_OptimizationSet::Oz):
         return "z";
      case(CompilerWrapper_OptimizationSet::Os):
         return "s";
      case(CompilerWrapper_OptimizationSet::O2):
         return "2";
      case(CompilerWrapper_OptimizationSet::O3):
         return "3";
      case(CompilerWrapper_OptimizationSet::O4):
         return "4";
      case(CompilerWrapper_OptimizationSet::O5):
         return "5";
      case(CompilerWrapper_OptimizationSet::Ofast):
         return "fast";
      case(CompilerWrapper_OptimizationSet::OBAMBU):
         return "bambu";
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
   for(const auto& [flag, enable] : optimization_flags)
   {
      /*argument aliasing should be treated in a different way*/
      if(flag == "argument-alias" || flag == "argument-noalias" || flag == "argument-noalias-global")
      {
         continue;
      }
      if(flag == "strict-aliasing")
      {
         optimizations += "-Wstrict-aliasing ";
      }
      if(flag == "fp-contract")
      {
         optimizations += enable ? "-ffp-contract=on " : "-ffp-contract=off ";
      }
      else
      {
         optimizations += std::string(enable ? "-f" : "-fno-") + flag + " ";
      }
   }
   for(const auto& [flag, value] : optimization_values)
   {
      optimizations += "-f" + flag + "=" + std::to_string(value) + " ";
   }
   for(const auto& [param, value] : parameter_values)
   {
      optimizations += "--param " + param + "=" + std::to_string(value) + " ";
   }
   return optimizations;
}

std::string CompilerWrapper::clang_recipes(const CompilerWrapper_OptimizationSet optimization_set,
                                           passesType& passes) const
{
   const auto is_openmp = Param->isOption(OPT_openmp) && Param->getOption<bool>(OPT_openmp);
   const auto opt_level = WriteOptimizationLevel(is_openmp ? CompilerWrapper_OptimizationSet::O2 :
                                                 optimization_set == CompilerWrapper_OptimizationSet::O0 ?
                                                             CompilerWrapper_OptimizationSet::O1 :
                                                             optimization_set);

   std::string recipe;
   recipe += load_plugin_opt(COMPILER_EXPANDMEMOPS_PLUGIN);

   if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG4)
   {
      if((optimization_set == CompilerWrapper_OptimizationSet::O2 ||
          optimization_set == CompilerWrapper_OptimizationSet::O3) &&
         !is_openmp)
      {
         std::string complex_recipe;
         complex_recipe +=
             " -tti -targetlibinfo -tbaa -scoped-noalias -assumption-cache-tracker -profile-summary-info "
             "-forceattrs -inferattrs -" COMPILER_EXPANDMEMOPS_PLUGIN " -domtree -mem2reg "
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
             "-opt-remark-emitter -disable-slp-vectorization -disable-loop-vectorization -scalarizer -loop-simplify "
             "-scalar-evolution -aa -loop-accesses -loop-load-elim -basicaa -aa  -dse -loop-unroll -instcombine "
             "-simplifycfg -domtree -basicaa -aa  -dse -loop-unroll -instcombine -loops -loop-simplify "
             "-lcssa-verification -lcssa -scalar-evolution -loop-unroll "
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
         recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer ";
         recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
      }
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG5)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer ";
      recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG6)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer ";
      recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG7)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer";
      recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG8)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -disable-loop-vectorization -scalarizer";
      recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG9)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -vectorize-loops=false -scalarizer";
      recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG10)
   {
      recipe += " -O" + opt_level + " -disable-slp-vectorization -vectorize-loops=false -scalarizer";
      recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG11)
   {
      recipe += " -O" + opt_level + " --disable-vector-combine -vectorize-loops=false -vectorize-slp=false -scalarizer";
      recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG12)
   {
      recipe += " -O" + opt_level + " --disable-vector-combine -vectorize-loops=false -vectorize-slp=false -scalarizer";
      if(!Param->isOption(OPT_openmp) || !Param->getOption<bool>(OPT_openmp))
      {
         recipe += " -enable-loop-flatten";
      }
      recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG13)
   {
      recipe += " -O" + opt_level + " --disable-vector-combine -vectorize-loops=false -vectorize-slp=false -scalarizer";
      if(!Param->isOption(OPT_openmp) || !Param->getOption<bool>(OPT_openmp))
      {
         recipe += " -enable-loop-flatten";
      }

      recipe += " -" COMPILER_EXPANDMEMOPS_PLUGIN " -simplifycfg ";
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG16)
   {
      passes.add_pass("", opt_level);
      recipe += " --disable-vector-combine -vectorize-loops=false -vectorize-slp=false ";
      if(!Param->isOption(OPT_openmp) || !Param->getOption<bool>(OPT_openmp))
      {
         recipe += " -enable-loop-flatten ";
      }
      passes.add_pass("scalarizer");
      passes.add_pass(COMPILER_EXPANDMEMOPS_PLUGIN);
      passes.add_pass("simplifycfg");
   }
   else if(compiler_target == CompilerWrapper_CompilerTarget::CT_I386_CLANG19)
   {
      passes.add_pass("", opt_level);
      recipe += " --disable-vector-combine -vectorize-loops=false -vectorize-slp=false ";
      if(!Param->isOption(OPT_openmp) || !Param->getOption<bool>(OPT_openmp))
      {
         recipe += " -enable-loop-flatten ";
      }
      passes.add_pass("scalarizer");
      passes.add_pass(COMPILER_EXPANDMEMOPS_PLUGIN);
      passes.add_pass("simplifycfg");
   }
   else
   {
      THROW_ERROR_USAGE("Selected clang compiler target is not supported by this build. "
                        "Choose a supported compiler target.");
   }

   return " " + recipe;
}

size_t CompilerWrapper::CGetPointerSize(const ParameterConstRef& parameters)
{
   const auto cc_m_env = parameters->getOption<std::string>(OPT_cc_m_env);
   if(cc_m_env == "-m32" || cc_m_env == "-mx32")
   {
      return 32;
   }
   if(cc_m_env == "-m64")
   {
      return 64;
   }
   THROW_ERROR_USAGE("Unsupported -m parameter: " + cc_m_env + ". Use one of -m32, -mx32, or -m64.");
   return 0;
}

bool CompilerWrapper::isCurrentOrNewer(CompilerWrapper_CompilerTarget ct, CompilerWrapper_CompilerTarget compare)
{
   return ct >= compare;
}

int CompilerWrapper::getCompatibleCompilers()
{
   return 0
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
#if HAVE_I386_CLANG19_COMPILER
          | static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG19)
#endif
       ;
}

int CompilerWrapper::getDefaultCompiler()
{
   return
#if HAVE_I386_CLANG19_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG19);
#elif HAVE_I386_CLANG16_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG16);
#elif HAVE_I386_CLANG7_COMPILER
       static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG7);
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
#else
       0;
   THROW_ERROR_USAGE("No frontend compiler available in this build. Install a supported compiler toolchain.");
#endif
}

std::string CompilerWrapper::getCompilerSuffix(CompilerWrapper_CompilerTarget pc)
{
   switch(pc)
   {
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG4:
         return "clang-4.0";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG5:
         return "clang-5.0";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG6:
         return "clang-6.0";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG7:
         return "clang-7";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG8:
         return "clang-8";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG9:
         return "clang-9";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG10:
         return "clang-10";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG11:
         return "clang-11";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG12:
         return "clang-12";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG13:
         return "clang-13";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG16:
         return "clang-16";
      case CompilerWrapper_CompilerTarget::CT_I386_CLANG19:
         return "clang-19";
      case CompilerWrapper_CompilerTarget::CT_NO_COMPILER:
      default:
         THROW_ERROR_USAGE("No supported compiler target selected. Check --compiler and installed toolchains.");
         return "";
   }
}

std::string CompilerWrapper::getCompilerVersion(int pc)
{
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
#if HAVE_I386_CLANG19_COMPILER
   if(pc & static_cast<int>(CompilerWrapper_CompilerTarget::CT_I386_CLANG19))
   {
      return I386_CLANG19_VERSION;
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

std::string CompilerWrapper::load_plugin(const std::string& plugin_name) const
{
   const auto plugin_obj = GetCompiler().GetPluginObject(plugin_name);
   if(isCurrentOrNewer(compiler_target, CompilerWrapper_CompilerTarget::CT_I386_CLANG13))
   {
      return "-fpass-plugin=" + plugin_obj + " -Xclang -load -Xclang " + plugin_obj;
   }
   return "-fplugin=" + plugin_obj;
}

std::string CompilerWrapper::load_plugin_opt(const std::string& plugin_name) const
{
   const auto plugin_obj = GetCompiler().GetPluginObject("opt_" + plugin_name);
   auto flags = "-load=" + plugin_obj;
   if(isCurrentOrNewer(compiler_target, CompilerWrapper_CompilerTarget::CT_I386_CLANG13))
   {
      flags += " -load-pass-plugin=" + plugin_obj;
   }
   return flags;
}
