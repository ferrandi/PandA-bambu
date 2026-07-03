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
 * @file CompilerWrapper.hpp
 * @brief Wrapper to the frontend compiler for C/C++ sources.
 *
 * A object used to invoke the frontend compiler from sources and create the dump.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef COMPILER_WRAPPER_HPP
#define COMPILER_WRAPPER_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "refcount.hpp"

#include <filesystem>
#include <iosfwd>
#include <vector>

REF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(ir_manager);
class passesType;

/// Possible optimization sets
enum class CompilerWrapper_OptimizationSet
{
   O0,     /**< -O0 */
   O1,     /**< -O1 */
   Oz,     /**< -Oz */
   Os,     /**< -Os */
   O2,     /**< -O2 */
   O3,     /**< -O3 */
   Ofast,  /**< -Ofast */
   O4,     /**< -O4 */
   O5,     /**< -O5 */
   OBAMBU, /**< Bambu optimizationss + OPT_compiler_opt_level */
};

/**
 * target of the compiler
 */
enum class CompilerWrapper_CompilerTarget
{
   CT_NO_COMPILER = 0,
   CT_I386_CLANG4 = 512,
   CT_I386_CLANG5 = 1024,
   CT_I386_CLANG6 = 2048,
   CT_I386_CLANG7 = 4096,
   CT_I386_CLANG8 = 8192,
   CT_I386_CLANG9 = 16384,
   CT_I386_CLANG10 = 32768,
   CT_I386_CLANG11 = 65536,
   CT_I386_CLANG12 = 131072,
   CT_I386_CLANG13 = 262144,
   CT_I386_CLANG16 = 524288,
   CT_I386_CLANG19 = 1048576
};

/**
 * @class CompilerWrapper
 * Main class for wrapping the frontend compiler. It allows an XML configuration file to be specified where the
 * parameters and the related values are stored.
 */
class CompilerWrapper
{
 private:
   /// Class storing information of a compiler
   class Compiler
   {
    public:
      /// The cpp executable
      std::string cpp;

      /// The compiler frontend executable
      std::string cc;

      /// The extra_options
      std::string extra_options;

      /// The clang llvm-link executable
      std::string llvm_link;

      /// The clang llvm-opt executable
      std::string llvm_opt;

      /// The compiler plugin directory
      std::string plugin_dir;

      /// The analyzer compiler plugin directory
      std::string analyzer_plugin_dir;

      Compiler()
      {
      }

      inline std::string GetPluginObject(const std::string& plugin_name) const
      {
         return plugin_dir + plugin_name + ".so";
      }

      inline std::string GetAnalyzerPluginObject(const std::string& plugin_name) const
      {
         return analyzer_plugin_dir + plugin_name + ".so";
      }
   };

   /// The set of input parameters
   const ParameterConstRef Param;

   /// The target compiler to be used
   const CompilerWrapper_CompilerTarget compiler_target;

   /// The set of optimizations to be applied
   const CompilerWrapper_OptimizationSet OS;

   const bool cpp_input;

   /// The frontend compiler parameters line for compiling a file
   std::string frontend_compiler_parameters;

   /// The compiler parameters for executable creation
   std::string compiler_linking_parameters;

   /// The set of files for which IR manager has already been computed
   CustomSet<std::string> already_processed_files;

   /// debug level
   int output_level;

   /// debug level
   int debug_level;

   /// The values of optimizations parameters
   std::map<std::string, int> parameter_values;

   /// The set of activated optimizations
   std::map<std::string, bool> optimization_flags;

   /// The value of parametric activate optimizations
   std::map<std::string, int> optimization_values;

   /**
    * Invoke the frontend compiler to compile file(s)
    * @param input_filename is the source file name (is modified in case of empty file compilation)
    * @param output_file is the output file name
    * @param parameters_line are the parameters to be passed to the frontend compiler
    * @param passes is the list of the opt passes to be executed
    * @param cm is the mode in which we compile
    * @param costTable is the cost table description forwarded to the frontend flow
    */
   void CompileFile(std::string& input_filename, const std::string& output_file, const std::string& parameters_line,
                    passesType passes, int cm, const std::string& costTable);

   std::string GetAnalyzeCompiler() const;

   /**
    * Initialize the frontend compiler parameters line
    */
   void InitializeCompilerParameters();

   /**
    * Analyze the command line options
    */
   void ReadParameters();

   /**
    * Set the default options for the frontend compiler
    */
   void SetCompilerDefault();

   /**
    * Set the default options for the frontend compiler in bambu
    */
   void SetBambuDefault();

   /**
    * Write the string containing the frontend compiler optimization options
    * @return the string with optimization options to be passed to the frontend compiler
    */
   std::string WriteOptimizationsString();

   std::string readExternalSymbols(const std::filesystem::path& filename) const;

   std::string clang_recipes(const CompilerWrapper_OptimizationSet optimization_level, passesType& passes) const;

   std::string load_plugin(const std::string& plugin_obj) const;

   std::string load_plugin_opt(const std::string& plugin_obj) const;

 public:
   /// The version of the frontend compiler
   static std::string bambu_ir_info;

   /**
    * Constructor
    * @param Param is the set of parameters
    * @param _compiler_target is the compiler target to be used
    * @param OS is the optimization set
    */
   CompilerWrapper(const ParameterConstRef Param, const CompilerWrapper_CompilerTarget _compiler_target,
                   const CompilerWrapper_OptimizationSet OS = CompilerWrapper_OptimizationSet::OBAMBU);

   /**
    * Return the compiler for a given target
    * @return a structure containing information about compiler
    */
   Compiler GetCompiler() const;

   /**
    * This function fills the IR manager with the nodes created from a set of source files
    * @param TM is where ir_manager will be stored
    * @param source_files are the source files to be compiled; key is the original source code file, value is the
    * transformed source code file
    * @param costTable is the cost table description forwarded to the frontend flow
    */
   void FillIRManager(const ir_managerRef TM, std::vector<std::string>& source_files, const std::string& costTable);

   /**
    * Return the list of the frontend compiler system includes
    */
   std::vector<std::filesystem::path> GetSystemIncludes() const;

   /**
    * Function that print of stdout some useful information passing the given option
    */
   void QueryCompilerConfig(const std::string& compiler_option) const;

   std::string GetCompilerParameters(const std::string& extra_compiler_options,
                                     bool no_frontend_compiler_parameters = false) const;

   /**
    * Create an executable starting from source code
    * @param file_names is the list of string; it must be a list since in some cases the order matters
    * @param executable_name is the name of the executable
    * @param extra_compiler_options is extra options to be used only for this compilation
    * @param no_frontend_compiler_parameters skips the default frontend compiler parameter set when true
    */
   void CreateExecutable(const std::vector<std::filesystem::path>& file_names,
                         const std::filesystem::path& executable_name, const std::string& extra_compiler_options,
                         bool no_frontend_compiler_parameters = false) const;

   /**
    * Writes the optimization level as a string
    * @param optimization_level is the optimization level to be printed
    * @return the optimization level in string format
    */
   static std::string WriteOptimizationLevel(const CompilerWrapper_OptimizationSet optimization_level)
       __attribute__((pure));

   /**
    * Return the size of the pointer in bit
    * @param parameters is the set of input parameters
    * @return the size of pointers in bit
    */
   static size_t CGetPointerSize(const ParameterConstRef& parameters) __attribute__((pure));

   static bool isCurrentOrNewer(CompilerWrapper_CompilerTarget ct, CompilerWrapper_CompilerTarget compare)
       __attribute__((pure));
   static int getCompatibleCompilers() __attribute__((pure));
   static int getDefaultCompiler() __attribute__((pure));
   static std::string getCompilerSuffix(CompilerWrapper_CompilerTarget pc) __attribute__((pure));
   static std::string getCompilerVersion(int ct) __attribute__((pure));
};

#endif
