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
 * @brief Wrapper to Gcc for C sources.
 *
 * A object used to invoke the GCC compiler from sources and create the dump.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef GCC_WRAPPER_HPP
#define GCC_WRAPPER_HPP

/// Autoheader include
#include "config_HAVE_ARM_COMPILER.hpp"
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_FROM_RTL_BUILT.hpp"
#include "config_HAVE_I386_CLANG10_COMPILER.hpp"
#include "config_HAVE_I386_CLANG11_COMPILER.hpp"
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
#include "config_HAVE_SPARC_COMPILER.hpp"
#include "config_HAVE_TUCANO_BUILT.hpp"
#include "config_HAVE_ZEBU_BUILT.hpp"

/// boost include
#include <boost/filesystem/path.hpp>

/// STD include
#include <iosfwd>
#include <vector>

/// utility include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "refcount.hpp"
#include <boost/filesystem/path.hpp>

REF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(tree_manager);

/// Possible optimization sets
enum class GccWrapper_OptimizationSet
{
   O0,    /**< -O0 */
   O1,    /**< -O1 */
   O2,    /**< -O2 */
   O3,    /**< -O3 */
   O4,    /**< -O4 */
   O5,    /**< -O5 */
   Og,    /**< -Og */
   Os,    /**< -Os */
   Oz,    /**< -Oz */
   Ofast, /**< -Ofast */
#if HAVE_BAMBU_BUILT
   OBAMBU, /**< Bambu optimizationss + OPT_gcc_opt_level */
   OSF,    /**< Bambu optimizations for soft float: O3 + -finline-limit=10000 */
#endif
#if HAVE_TUCANO_BUILT
   OTUCANO, /**< Tucano optimizations + OPT_gcc_opt_level */
#endif
#if HAVE_ZEBU_BUILT
   OZEBU, /**< Zebu optimizations + OPT_gcc_opt_level */
#endif
};

/**
 * target of the compiler
 */
enum class GccWrapper_CompilerTarget
{
   CT_NO_GCC = 0,
#if HAVE_I386_GCC45_COMPILER
   CT_I386_GCC45 = 1,
#endif
#if HAVE_I386_GCC46_COMPILER
   CT_I386_GCC46 = 2,
#endif
#if HAVE_I386_GCC47_COMPILER
   CT_I386_GCC47 = 4,
#endif
#if HAVE_I386_GCC48_COMPILER
   CT_I386_GCC48 = 8,
#endif
#if HAVE_I386_GCC49_COMPILER
   CT_I386_GCC49 = 16,
#endif
#if HAVE_I386_GCC5_COMPILER
   CT_I386_GCC5 = 32,
#endif
#if HAVE_I386_GCC6_COMPILER
   CT_I386_GCC6 = 64,
#endif
#if HAVE_I386_GCC7_COMPILER
   CT_I386_GCC7 = 128,
#endif
#if HAVE_I386_GCC8_COMPILER
   CT_I386_GCC8 = 256,
#endif
#if HAVE_I386_CLANG4_COMPILER
   CT_I386_CLANG4 = 512,
#endif
#if HAVE_I386_CLANG5_COMPILER
   CT_I386_CLANG5 = 1024,
#endif
#if HAVE_I386_CLANG6_COMPILER
   CT_I386_CLANG6 = 2048,
#endif
#if HAVE_I386_CLANG7_COMPILER
   CT_I386_CLANG7 = 4096,
#endif
#if HAVE_I386_CLANG8_COMPILER
   CT_I386_CLANG8 = 8192,
#endif
#if HAVE_I386_CLANG9_COMPILER
   CT_I386_CLANG9 = 16384,
#endif
#if HAVE_I386_CLANG10_COMPILER
   CT_I386_CLANG10 = 32768,
#endif
#if HAVE_I386_CLANG11_COMPILER
   CT_I386_CLANG11 = 65536,
#endif
#if HAVE_ARM_COMPILER
   CT_ARM_GCC = 131072,
#endif
#if HAVE_SPARC_COMPILER
   CT_SPARC_GCC = 262144,
   CT_SPARC_ELF_GCC = 524288
#endif
};

enum class GccWrapper_CompilerMode
{
   CM_STD = 0,
   CM_EMPTY,
   CM_ANALYZER,
   CM_LTO
};

/**
 * @class GccWrapper
 * Main class for wrapping the GCC compiler. It allows an XML configuration file to be specified where the parameters
 * and the related values are stored.
 */
class GccWrapper
{
 private:
   /// Class storing information of a compiler
   class Compiler
   {
    public:
      /// The cpp executable
      boost::filesystem::path cpp;

      /// The gcc executable
      boost::filesystem::path gcc;

      /// The extra_options
      std::string extra_options;

      /// The plugin to dump empty gimple
      std::string empty_plugin_obj;
      std::string empty_plugin_name;

      /// The plugin to dump ssa gimple
      std::string ssa_plugin_obj;
      std::string ssa_plugin_name;

      /// The plugin expanding MemOps calls
      std::string expandMemOps_plugin_obj;
      std::string expandMemOps_plugin_name;

      /// The plugin canocalizing GEPIs
      std::string GepiCanon_plugin_obj;
      std::string GepiCanon_plugin_name;

      /// The plugin performing Custom Scalar Replacement of Aggregates
      std::string CSROA_plugin_obj;
      std::string CSROA_plugin_name;

      /// The plugin making visible only the top function
      std::string topfname_plugin_obj;
      std::string topfname_plugin_name;

      /// The plugin making visible only the top function
      std::string ASTAnalyzer_plugin_obj;
      std::string ASTAnalyzer_plugin_name;

      /// The clang llvm-link executable
      boost::filesystem::path llvm_link;

      /// The clang llvm-opt executable
      boost::filesystem::path llvm_opt;

#if HAVE_FROM_RTL_BUILT
      /// The plugin to dump gimple and rtl
      std::string rtl_plugin;
#endif
      /// true when compiler is based on clang/llvm
      bool is_clang;
      Compiler() : is_clang(false)
      {
      }
   };

   /// The set of input parameters
   const ParameterConstRef Param;

   /// The target compiler to be used
   const GccWrapper_CompilerTarget compiler_target;

   /// The set of optimizations to be applied
   const GccWrapper_OptimizationSet OS;

   /// The gcc parameters line for compiling a file
   std::string gcc_compiling_parameters;

   /// The gcc parameters line for creating an executable
   std::string gcc_linking_parameters;

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
    * Invoke gcc to compile file(s)
    * @param original_file_name is the original file passed through command line; this information is necessary to retrieve include directory
    * @param real_rile_name stores the source code file which is actually compiled; the function can modified it in case of empty file
    * @param parameters_line are the parameters to be passed to gcc
    * @param multiple_files is the true in case multiple files are considered.
    * @param cm is the mode in which we compile
    */
   void CompileFile(const std::string& original_file_name, std::string& real_file_name, const std::string& parameters_line, bool multiple_files, GccWrapper_CompilerMode cm);

   /**
    * Return the compiler for a given target
    * @return a structure containing information about compiler
    */
   Compiler GetCompiler() const;

   /**
    * Initialize the gcc parameters line
    * @param OS is the optimizations set to be considered
    * @return the string with the parameters
    */
   void InitializeGccParameters();

#if HAVE_BAMBU_BUILT || HAVE_TUCANO_BUILT || HAVE_ZEBU_BUILT
   /**
    * Analyze the command line options
    */
   void ReadParameters();

   /**
    * Set the default options for gcc
    */
   void SetGccDefault();
#endif

#if HAVE_BAMBU_BUILT
   /**
    * Set the default options for gcc in bambu
    */
   void SetBambuDefault();
#endif

#if HAVE_ZEBU_BUILT
   /**
    * Set the default options for gcc in zebu
    */
   // cppcheck-suppress unusedPrivateFunction
   void SetZebuDefault();
#endif

   /**
    * Write the string containing gcc optimization options
    * @return the string with optimization options to be passed to the gcc
    */
   std::string WriteOptimizationsString();

   /**
    * Add includes of source file directories
    * @param source_files are the source files to be considered
    * @return the string to be passed to gcc
    */
   const std::string AddSourceCodeIncludes(const std::list<std::string>& source_files) const;

   /**
    * Convert a string version to a number
    * @param version is the version to be converted
    * @return the corresponding number
    */
   static size_t ConvertVersion(const std::string& version);

   std::string clang_recipes(const GccWrapper_OptimizationSet optimization_level, const GccWrapper_CompilerTarget compiler_target, const std::string& expandMemOps_plugin_obj, const std::string& expandMemOps_plugin_name,
                             const std::string& GepiCanon_plugin_obj, const std::string& GepiCanon_plugin_name, const std::string& CSROA_plugin_obj, const std::string& CSROA_plugin_name, const std::string& fname);

 public:
   /// The version of the gcc
   static std::string current_gcc_version;

   /// The version of the plugin
   static std::string current_plugin_version;

   /**
    * Constructor
    * @param Param is the set of parameters
    * @param compiler is the compiler to be used
    * @param OS is the optimization set
    */
   GccWrapper(const ParameterConstRef Param, const GccWrapper_CompilerTarget compiler, const GccWrapper_OptimizationSet OS);

   /**
    * Destructor
    */
   ~GccWrapper();

   /**
    * This function fills the tree manager with the nodes created from a set of source files
    * @param TM is where tree_manager will be stored
    * @param source_files are the source files to be compiled; key is the original source code file, value is the transformed source code file
    */
   void FillTreeManager(const tree_managerRef TM, std::map<std::string, std::string>& input_files);

   /**
    * Return the list of gcc system include
    * @param includes is where result will be stored
    */
   void GetSystemIncludes(std::vector<std::string>& includes) const;

   /**
    * Dump the gcc configuration
    */
   void GetGccConfig() const;

   /**
    * Function that print of stdout some useful information passing the given option
    */
   void QueryGccConfig(const std::string& gcc_option) const;

   /**
    * Return the total number of lines of the benchmark
    * @param Param is the set of input parameters
    * @return the total number of lines of the benchmark
    */
   static size_t GetSourceCodeLines(const ParameterConstRef Param);

   /**
    * Create an executable starting from source code
    * @param file_names is the list of string; it must be a list since in some cases the order matters
    * @param executable_name is the name of the executable
    * @param extra_gcc_options is extra options to be used only for this compilation
    */
   void CreateExecutable(const std::list<std::string>& file_names, const std::string& executable_name, const std::string& extra_gcc_options, bool no_gcc_compiling_parameters = false) const;

   /**
    * Create an executable starting from source code
    * @param file_names is the set of filename
    * @param executable_name is the name of the executable
    * @param extra_gcc_options is extra options to be used only for this compilation
    */
   void CreateExecutable(const CustomSet<std::string>& file_names, const std::string& executable_name, const std::string& extra_gcc_options, bool no_gcc_compiling_parameters = false) const;

   /**
    * Read gcc configuration from file
    * @param file_name is the name of the file
    */
   void ReadXml(const std::string& file_name);

   /**
    * Write gcc configuration on file
    * @param file_name is the name of the file
    */
   void WriteXml(const std::string& file_name) const;

   /**
    * Writes the optimization level as a string
    * @param optimization_level is the optimization level to be printed
    * @return the optimization level in string format
    */
   static std::string WriteOptimizationLevel(const GccWrapper_OptimizationSet optimiation_level);

   /**
    * Check if the combination gcc-plugin is supported; if not it throws error
    * @param gcc_version is the gcc version in form x.x.x
    * @param plugin_version is the plugin version in form x.x
    */
   static void CheckGccCompatibleVersion(const std::string& gcc_version, const std::string& plugin_version);

   /**
    * Return the size of the pointer in bit
    * @param parameters is the set of input parameters
    * @return the size of pointers in bit
    */
   static size_t CGetPointerSize(const ParameterConstRef parameters);
};

/// Refcount definition for the GccWrapper class
typedef refcount<GccWrapper> GccWrapperRef;
typedef refcount<const GccWrapper> GccWrapperConstRef;

#endif
