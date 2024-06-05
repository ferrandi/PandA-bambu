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
 * @brief Wrapper to the frontend compiler for C/C++ sources.
 *
 * A object used to invoke the frontend compiler from sources and create the dump.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
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
REF_FORWARD_DECL(tree_manager);

/// Possible optimization sets
enum class CompilerWrapper_OptimizationSet
{
   O0,     /**< -O0 */
   O1,     /**< -O1 */
   O2,     /**< -O2 */
   O3,     /**< -O3 */
   O4,     /**< -O4 */
   O5,     /**< -O5 */
   Og,     /**< -Og */
   Os,     /**< -Os */
   Oz,     /**< -Oz */
   Ofast,  /**< -Ofast */
   OBAMBU, /**< Bambu optimizationss + OPT_compiler_opt_level */
   OSF,    /**< Bambu optimizations for soft float: O3 + -finline-limit=10000 */
};

/**
 * target of the compiler
 */
enum class CompilerWrapper_CompilerTarget
{
   CT_NO_COMPILER = 0,
   CT_I386_GCC49 = 16,
   CT_I386_GCC5 = 32,
   CT_I386_GCC6 = 64,
   CT_I386_GCC7 = 128,
   CT_I386_GCC8 = 256,
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
   CT_I386_CLANGVVD = 1048576
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
      std::string gcc;

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

      /// true when compiler is based on clang/llvm
      bool is_clang{false};

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

   /// The set of files for which tree manager has already been computed
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
    * @param output_filename is the output file name
    * @param parameters_line are the parameters to be passed to the frontend compiler
    * @param multiple_files is the true in case multiple files are considered.
    * @param cm is the mode in which we compile
    */
   void CompileFile(std::string& input_filename, const std::string& output_file, const std::string& parameters_line,
                    int cm, const std::string& costTable);

   std::string GetAnalyzeCompiler() const;

   /**
    * Initialize the frontend compiler parameters line
    * @param OS is the optimizations set to be considered
    * @return the string with the parameters
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

   /**
    * Add includes of source file directories
    * @param source_files are the source files to be considered
    * @return the string to be passed to the frontend compiler
    */
   const std::string AddSourceCodeIncludes(const std::list<std::string>& source_files) const;

   std::string readExternalSymbols(const std::filesystem::path& filename) const;

   std::string clang_recipes(const CompilerWrapper_OptimizationSet optimization_level, const std::string& fname) const;

   std::string load_plugin(const std::string& plugin_obj, CompilerWrapper_CompilerTarget target) const;

   std::string load_plugin_opt(std::string plugin_obj, CompilerWrapper_CompilerTarget target) const;

   std::string add_plugin_prefix(CompilerWrapper_CompilerTarget target, std::string O_level = "") const;

 public:
   /// The version of the frontend compiler
   static std::string bambu_ir_info;

   /**
    * Constructor
    * @param Param is the set of parameters
    * @param compiler is the compiler to be used
    * @param OS is the optimization set
    */
   CompilerWrapper(const ParameterConstRef Param, const CompilerWrapper_CompilerTarget _compiler_target,
                   const CompilerWrapper_OptimizationSet OS);

   /**
    * Destructor
    */
   ~CompilerWrapper();

   /**
    * Return the compiler for a given target
    * @return a structure containing information about compiler
    */
   Compiler GetCompiler() const;

   /**
    * This function fills the tree manager with the nodes created from a set of source files
    * @param TM is where tree_manager will be stored
    * @param source_files are the source files to be compiled; key is the original source code file, value is the
    * transformed source code file
    */
   void FillTreeManager(const tree_managerRef TM, std::vector<std::string>& source_files, const std::string& costTable);

   /**
    * Return the list of the frontend compiler system includes
    * @param includes is where result will be stored
    */
   void GetSystemIncludes(std::vector<std::string>& includes) const;

   /**
    * Dump the frontend compiler configuration
    */
   void GetCompilerConfig() const;

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
    */
   void CreateExecutable(const std::list<std::string>& file_names, const std::string& executable_name,
                         const std::string& extra_compiler_options, bool no_frontend_compiler_parameters = false) const;

   /**
    * Create an executable starting from source code
    * @param file_names is the set of filename
    * @param executable_name is the name of the executable
    * @param extra_compiler_options is extra options to be used only for this compilation
    */
   void CreateExecutable(const CustomSet<std::string>& file_names, const std::string& executable_name,
                         const std::string& extra_compiler_options, bool no_frontend_compiler_parameters = false) const;

   /**
    * Read the frontend compiler configuration from file
    * @param file_name is the name of the file
    */
   void ReadXml(const std::string& file_name);

   /**
    * Write the frontend compiler configuration on file
    * @param file_name is the name of the file
    */
   void WriteXml(const std::string& file_name) const;

   /**
    * Writes the optimization level as a string
    * @param optimization_level is the optimization level to be printed
    * @return the optimization level in string format
    */
   static std::string WriteOptimizationLevel(const CompilerWrapper_OptimizationSet optimization_level);

   /**
    * Return the size of the pointer in bit
    * @param parameters is the set of input parameters
    * @return the size of pointers in bit
    */
   static size_t CGetPointerSize(const ParameterConstRef parameters);

   static bool isCurrentOrNewer(CompilerWrapper_CompilerTarget ct, CompilerWrapper_CompilerTarget compare);
   static bool isClangCheck(CompilerWrapper_CompilerTarget ct);
   static bool isGccCheck(CompilerWrapper_CompilerTarget ct);
   static int getCompatibleCompilers();
   static int getDefaultCompiler();
   static std::string getCompilerSuffix(CompilerWrapper_CompilerTarget pc);
   static bool hasCompilerM64(CompilerWrapper_CompilerTarget ct);
   static bool hasCompilerM32(CompilerWrapper_CompilerTarget ct);
   static bool hasCompilerMX32(CompilerWrapper_CompilerTarget ct);
   static std::string getCompilerVersion(int ct);
};

/// Refcount definition for the CompilerWrapper class
using CompilerWrapperRef = refcount<CompilerWrapper>;
using CompilerWrapperConstRef = refcount<const CompilerWrapper>;

#endif
