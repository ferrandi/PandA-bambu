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
 * @file Parameter.hpp
 * @brief this class is used to manage the command-line or XML options. It has to be specialized with respect to the
 * tool
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

#include "config_HAVE_BAMBU_RESULTS_XML.hpp"
#include "config_HAVE_FROM_AADL_ASN_BUILT.hpp"
#include "config_HAVE_FROM_C_BUILT.hpp"
#include "config_HAVE_HLS_BUILT.hpp"
#include "config_HAVE_TECHNOLOGY_BUILT.hpp"
#include "config_RELEASE.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <climits>
#include <filesystem>
#include <list>
#include <string>
#include <vector>

/// forward decl of xml Element
class xml_element;

/// An integer value to return if parameters have been right parsed
#define PARAMETER_PARSED INT_MIN
#define PARAMETER_NOTPARSED INT_MAX

#define BAMBU_OPTIONS                                                                                                  \
   (chaining)(chaining_algorithm)(constraints_file)(context_switch)(controller_architecture)(datapath_architecture)(   \
       distram_threshold)(DSP_allocation_coefficient)(DSP_margin_combinational)(DSP_margin_pipelined)(DSP_fracturing)( \
       estimate_logic_and_connections)(evaluation)(evaluation_mode)(evaluation_objectives)(experimental_setup)(        \
       export_core)(export_core_mode)(fsm_encoding)(fu_binding_algorithm)(generate_testbench)(generate_vcd)(hls_flow)( \
       hls_div)(hls_fpdiv)(interface)(interface_type)(data_bus_bitsize)(addr_bus_bitsize)(libm_std_rounding)(          \
       liveness_algorithm)(scheduling_mux_margins)(scheduling_priority)(scheduling_algorithm)(simulate)(simulator)(    \
       simulation_output)(speculative)(pipelining)(storage_value_insertion_algorithm)(stg)(stg_algorithm)(             \
       register_allocation_algorithm)(register_grouping)(registered_inputs)(resp_model)(                               \
       datapath_interconnection_algorithm)(insert_memory_profile)(top_file)(assert_debug)(                             \
       memory_allocation_algorithm)(memory_allocation_policy)(xml_memory_allocation)(rom_duplication)(base_address)(   \
       reset_type)(reset_level)(reg_init_value)(clock_period_resource_fraction)(channels_type)(channels_number)(       \
       memory_controller_type)(soft_float)(soft_fp)(fp_subnormal)(max_sim_cycles)(sparse_memory)(max_ulp)(             \
       skip_pipe_parameter)(gcc_serialize_memory_accesses)(unaligned_access)(aligned_access)(                          \
       backend_script_extensions)(backend_sdc_extensions)(VHDL_library)(bitvalue_ipa)(use_asynchronous_memories)(      \
       do_not_chain_memories)(bram_high_latency)(cdfc_module_binding_algorithm)(function_allocation_algorithm)(        \
       testbench_input_string)(testbench_input_file)(testbench_argv)(testbench_param_size)(testbench_map_mode)(        \
       weighted_clique_register_algorithm)(disable_function_proxy)(memory_mapped_top)(expose_globals)(connect_iob)(    \
       profiling_output)(disable_bounded_function)(discrepancy)(discrepancy_force)(discrepancy_hw)(                    \
       discrepancy_no_load_pointers)(discrepancy_only)(discrepancy_permissive_ptrs)(generate_taste_architecture)(      \
       initial_internal_address)(mem_delay_read)(mem_delay_write)(tb_queue_size)(memory_banks_number)(mixed_design)(   \
       num_accelerators)(technology_file)(tb_extra_gcc_options)(timing_violation_abort)(top_design_name)(              \
       serialize_output)(use_ALUs)(range_analysis_mode)(fp_format)(fp_format_propagate)(fp_format_interface)(          \
       fp_rounding_mode)(fp_exception_mode)(parallel_backend)(architecture_xml)(lattice_root)(lattice_settings)(       \
       lattice_pmi_def)(lattice_inc_dirs)(xilinx_root)(xilinx_settings)(xilinx_vivado_settings)(xilinx_glbl)(          \
       mentor_root)(mentor_modelsim_bin)(mentor_optimizer)(verilator)(verilator_timescale_override)(                   \
       verilator_parallel)(altera_root)(quartus_settings)(quartus_13_settings)(quartus_13_64bit)(nanoxplore_root)(     \
       nanoxplore_settings)(nanoxplore_bypass)(shared_input_registers)(inline_functions)(function_constraints)(        \
       resource_constraints)(axi_burst_type)(generate_components_library)

#define FRAMEWORK_OPTIONS                                                                                            \
   (benchmark_name)(cat_args)(find_max_transformations)(max_transformations)(compatible_compilers)(compute_size_of)( \
       configuration_name)(debug_level)(default_compiler)(dot_directory)(host_compiler)(ilp_max_time)(ilp_solver)(   \
       input_file)(input_format)(no_clean)(no_parse_files)(no_return_zero)(output_file)(output_level)(               \
       output_temporary_directory)(output_directory)(panda_parameter)(parse_pragma)(pretty_print)(print_dot)(        \
       profiling_method)(program_name)(read_parameter_xml)(revision)(seed)(test_multiple_non_deterministic_flows)(   \
       test_single_non_deterministic_flow)(top_functions_names)(xml_input_configuration)(xml_output_configuration)(  \
       write_parameter_xml)(ignore_parallelism)(ignore_mapping)(mapping)(sequence_length)(without_transformation)(   \
       blackbox)(input_libraries)(frontend_statistics)(exec_argv)(path)

#define COMPILER_OPTIONS                                                                                              \
   (gcc_config)(gcc_costs)(gcc_defines)(gcc_extra_options)(gcc_include_sysdir)(gcc_includes)(gcc_libraries)(          \
       gcc_library_directories)(gcc_openmp_simd)(compiler_opt_level)(gcc_m_env)(gcc_optimizations)(                   \
       gcc_optimization_set)(gcc_parameters)(gcc_plugindir)(gcc_read_xml)(gcc_standard)(gcc_undefines)(gcc_warnings)( \
       gcc_E)(gcc_S)(gcc_write_xml)

#define SYNTHESIS_OPTIONS                                                                                            \
   (clock_period)(clock_name)(reset_name)(start_name)(done_name)(device_string)(synthesis_flow)(target_device_file)( \
       target_device_script)(top_component)(writer_language)

#define SPIDER_OPTIONS                                                                                              \
   (accuracy)(aggregated_features)(cross_validation)(experimental_setup_file)(latex_format_file)(max_bound)(        \
       maximum_error)(min_bound)(minimum_significance)(normalization_file)(normalization_sequences)(output_format)( \
       precision)(processing_element_type)(skip_rows)(surviving_benchmarks)

#define EUCALIPTUS_OPTIONS (component_name)

#define TREE_PANDA_COMPILER_OPTIONS (archive_files)(obj_files)(compress_archive)

#define OPTIONS_ENUM(r, data, elem) BOOST_PP_CAT(OPT_, elem),

/// Possible options
// cppcheck-suppress syntaxError
enum enum_option
{
   BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, BAMBU_OPTIONS)
       BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, EUCALIPTUS_OPTIONS)
           BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, FRAMEWORK_OPTIONS)
               BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, COMPILER_OPTIONS)
                   BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, SPIDER_OPTIONS)
                       BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, SYNTHESIS_OPTIONS)
                           BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, TREE_PANDA_COMPILER_OPTIONS)
};

#define DEFAULT_OPT_BASE 512
#define OPT_READ_PARAMETERS_XML DEFAULT_OPT_BASE
#define OPT_WRITE_PARAMETERS_XML (1 + OPT_READ_PARAMETERS_XML)
#define OPT_DEBUG_CLASSES (1 + OPT_WRITE_PARAMETERS_XML)
#define OPT_BENCHMARK_NAME (1 + OPT_DEBUG_CLASSES)
#define OPT_BENCHMARK_FAKE_PARAMETERS (1 + OPT_BENCHMARK_NAME)
#define INPUT_OPT_ERROR_ON_WARNING (1 + OPT_BENCHMARK_FAKE_PARAMETERS)
#define OPT_OUTPUT_TEMPORARY_DIRECTORY (1 + INPUT_OPT_ERROR_ON_WARNING)
#define INPUT_OPT_PRINT_DOT (1 + OPT_OUTPUT_TEMPORARY_DIRECTORY)
#define INPUT_OPT_SEED (1 + INPUT_OPT_PRINT_DOT)
#define INPUT_OPT_NO_CLEAN (1 + INPUT_OPT_SEED)
#define INPUT_OPT_CONFIGURATION_NAME (1 + INPUT_OPT_NO_CLEAN)
#define INPUT_OPT_MAX_TRANSFORMATIONS (1 + INPUT_OPT_CONFIGURATION_NAME)
#define INPUT_OPT_FIND_MAX_TRANSFORMATIONS (1 + INPUT_OPT_MAX_TRANSFORMATIONS)
#define INPUT_OPT_PANDA_PARAMETER (1 + INPUT_OPT_FIND_MAX_TRANSFORMATIONS)

/// define the default tool short option string
#define COMMON_SHORT_OPTIONS_STRING "hVv:d:"

/// define default TOOL long options
#define COMMON_LONG_OPTIONS                                                                                            \
   {"help", no_argument, nullptr, 'h'}, {"verbosity", required_argument, nullptr, 'v'},                                \
       {"version", no_argument, nullptr, 'V'},                                                                         \
       {"read-parameters-XML", required_argument, nullptr, OPT_READ_PARAMETERS_XML},                                   \
       {"write-parameters-XML", required_argument, nullptr, OPT_WRITE_PARAMETERS_XML},                                 \
       {"debug", required_argument, nullptr, 'd'}, {"debug-classes", required_argument, nullptr, OPT_DEBUG_CLASSES},   \
       {"no-clean", no_argument, nullptr, INPUT_OPT_NO_CLEAN},                                                         \
       {"benchmark-name", required_argument, nullptr, OPT_BENCHMARK_NAME},                                             \
       {"configuration-name", required_argument, nullptr, INPUT_OPT_CONFIGURATION_NAME},                               \
       {"benchmark-fake-parameters", required_argument, nullptr, OPT_BENCHMARK_FAKE_PARAMETERS},                       \
       {"output-temporary-directory", required_argument, nullptr, OPT_OUTPUT_TEMPORARY_DIRECTORY},                     \
       {"error-on-warning", no_argument, nullptr, INPUT_OPT_ERROR_ON_WARNING},                                         \
       {"print-dot", no_argument, nullptr, INPUT_OPT_PRINT_DOT}, {"seed", required_argument, nullptr, INPUT_OPT_SEED}, \
       {"max-transformations", required_argument, nullptr, INPUT_OPT_MAX_TRANSFORMATIONS},                             \
       {"find-max-transformations", no_argument, nullptr, INPUT_OPT_FIND_MAX_TRANSFORMATIONS},                         \
   {                                                                                                                   \
      "panda-parameter", required_argument, nullptr, INPUT_OPT_PANDA_PARAMETER                                         \
   }

#define INPUT_OPT_CUSTOM_OPTIONS 1024
#define INPUT_OPT_COMPUTE_SIZEOF (1 + INPUT_OPT_CUSTOM_OPTIONS)
#define INPUT_OPT_COMPILER (1 + INPUT_OPT_COMPUTE_SIZEOF)
#define INPUT_OPT_GCC_CONFIG (1 + INPUT_OPT_COMPILER)
#define INPUT_OPT_INCLUDE_SYSDIR (1 + INPUT_OPT_GCC_CONFIG)
#define INPUT_OPT_PARAM (1 + INPUT_OPT_INCLUDE_SYSDIR)
#define INPUT_OPT_READ_GCC_XML (1 + INPUT_OPT_PARAM)
#define INPUT_OPT_STD (1 + INPUT_OPT_READ_GCC_XML)
#define INPUT_OPT_USE_RAW (1 + INPUT_OPT_STD)
#define INPUT_OPT_WRITE_GCC_XML (1 + INPUT_OPT_USE_RAW)
#define LAST_GCC_OPT INPUT_OPT_WRITE_GCC_XML

/// define the GCC short option string
#define GCC_SHORT_OPTIONS_STRING "f:I:D:U:O::l:L:W:Em:g::"

#if !RELEASE
#define GCC_LONG_OPTIONS_RAW_XML                                             \
   {"use-raw", no_argument, nullptr, INPUT_OPT_USE_RAW},                     \
       {"read-GCC-XML", required_argument, nullptr, INPUT_OPT_READ_GCC_XML}, \
       {"write-GCC-XML", required_argument, nullptr, INPUT_OPT_WRITE_GCC_XML},
#else
#define GCC_LONG_OPTIONS_RAW_XML {"use-raw", no_argument, nullptr, INPUT_OPT_USE_RAW},
#endif
#define GCC_LONG_OPTIONS_COMPILER {"compiler", required_argument, nullptr, INPUT_OPT_COMPILER},

#define GCC_LONG_OPTIONS                                                               \
   GCC_LONG_OPTIONS_COMPILER{"std", required_argument, nullptr, INPUT_OPT_STD},        \
       GCC_LONG_OPTIONS_RAW_XML{"param", required_argument, nullptr, INPUT_OPT_PARAM}, \
       {"Include-sysdir", no_argument, nullptr, INPUT_OPT_INCLUDE_SYSDIR},             \
       {"gcc-config", no_argument, nullptr, INPUT_OPT_GCC_CONFIG},                     \
       {"compute-sizeof", no_argument, nullptr, INPUT_OPT_COMPUTE_SIZEOF},             \
   {                                                                                   \
      "extra-gcc-options", required_argument, nullptr, INPUT_OPT_CUSTOM_OPTIONS        \
   }

/**
 * File formats
 */
enum class Parameters_FileFormat
{
   FF_UNKNOWN = 0, /**< UNKNOWN */
#if HAVE_FROM_AADL_ASN_BUILT
   FF_AADL, /**< (Input) Aadl file */
   FF_ASN,  /**< (Input) Asn file */
#endif
#if HAVE_FROM_C_BUILT
   FF_C,            /**< (Input/Output) C source file */
   FF_OBJECTIVEC,   /**< (Input/Output) Objective C source file */
   FF_CPP,          /**< (Input/Output) C++ source file */
   FF_OBJECTIVECPP, /**< (Input/Output) Objective C++ source file */
   FF_FORTRAN,      /**< (Input/Output) Fortran source file */
   FF_LLVM,         /**< (Input/Output) LLVM source bitcode file */
   FF_LLVM_CPP,     /**< (Input/Output) LLVM source bitcode file generated from c++ source code*/
#endif
   FF_CSV, /**< (Input) comma separated value */
#if HAVE_FROM_C_BUILT
   FF_RAW, /**< (Input/Output) raw file */
#endif
   FF_TEX,     /**< (Output) Latex table */
   FF_TGFF,    /**< (Input) task graph for free */
   FF_VERILOG, /**< (Input) verilog */
   FF_VHDL,    /**< (Input) vhdl */
   FF_XML,     /**< (Input/Output) XML */
#if HAVE_BAMBU_RESULTS_XML
   FF_XML_BAMBU_RESULTS, /**< (Input) XML bambu results*/
#endif
#if HAVE_HLS_BUILT
   FF_XML_CON, /**< (Input) XML storing constraints */
#endif
   FF_XML_EXPERIMENTAL_SETUP, /**< (Input) XML storing experimental setup */
   FF_XML_SKIP_ROW,           /**< (Input) XML benchhmarks to be execluded from training set */
   FF_XML_SYM_SIM,            /**< (Input) XML storing symbolic symulation results */
#if HAVE_TECHNOLOGY_BUILT
   FF_XML_TARGET, /**< (Input) XML storing information about a particular target device */
   FF_XML_TEC,    /**< (Input) XML storing technology libraries */
#endif
   FF_XML_TEX_TABLE, /**< (Input) XML storing format of latex table to be produced */
   FF_XML_WGT_GM,    /**< (Output) XML weights of single operations computed */
   FF_XML_WGT_SYM,   /**< (Input/Output) XML symbolic weights */
};

class Parameter
{
 protected:
   /// The number of input paramters
   int argc;

   /// The input parameters;
   char** const argv;

   /// Map between the name of the option and the related string-form value
   CustomMap<std::string, std::string> Options;

   /// Map between the name of a parameter and the related string-form value
   CustomMap<std::string, std::string> panda_parameters;

   /// Map between an enum option and the related string-form value
   CustomMap<enum enum_option, std::string> enum_options;

   /// Name of the enum options
   static const CustomMap<enum enum_option, std::string> option_name;

   /// Classes to be debugged
   CustomUnorderedSet<std::string> debug_classes;

   /// debug level
   int debug_level;

   /**
    * Loads an XML configuration file (recursive method)
    * @param node is the starting node for the analysis
    */
   void load_xml_configuration_file_rec(const xml_element* node);

   /**
    * Manage default options (common to all tools)
    * @param next_option is the index of the option to be analyzed
    * @param optarg_param is the optional argument of the option
    * @param exit_success is where the exit value is stored
    * @return true if the option has been recognized
    */
   bool ManageDefaultOptions(int next_option, char* optarg_param, bool& exit_success);

#if HAVE_FROM_C_BUILT
   /**
    * Manage Gcc options
    * @param next_option is the index of the option to be analyzed
    * @param optarg_param is the optional argument of the option
    * @return true if the option has been recognized
    */
   bool ManageGccOptions(int next_option, char* optarg_param);
#endif

   /**
    * Print the usage of the general common options
    * @param os is the stream where to print
    */
   void PrintGeneralOptionsUsage(std::ostream& os) const;

   /**
    * Print the usage of the output common options
    * @param os is the stream
    */
   void PrintOutputOptionsUsage(std::ostream& os) const;

#if HAVE_FROM_C_BUILT
   /**
    * Print the gcc options usage
    * @param os is the stream where to print
    */
   void PrintGccOptionsUsage(std::ostream& os) const;
#endif

   /**
    * Sets the default values with respect to the tool
    */
   virtual void SetDefaults() = 0;

   /**
    * Sets the default values common to all tools
    */
   void SetCommonDefaults();

   /**
    * Print the name of the program to be included in the header
    * @param os is the stream on which the program name has to be printed
    */
   virtual void PrintProgramName(std::ostream& os) const = 0;

   /**
    * Print the help
    * @param os is the stream on which the help has to be printed
    */
   virtual void PrintHelp(std::ostream& os) const = 0;

 public:
   /**
    * Constructor
    * @param program_name is the name of the executable
    * @param argc is the number of arguments
    * @param argv is the array of arguments passed to program.
    * @param debug_level is the debug level
    */
   Parameter(const std::string& program_name, int argc, char** const argv, int debug_level = 0);

   /**
    * Copy Constructor
    * @param other is copy element
    */
   Parameter(const Parameter& other);

   /**
    * Destructor
    */
   virtual ~Parameter();

   /**
    * Loads an XML configuration file
    * @param filename is the configuration file name to be loaded
    */
   void load_xml_configuration_file(const std::string& filename);

   /**
    * Write an XML configuration file with the parameters actually stored
    * @param filename is the configuration file name where parameters have to be written
    */
   void write_xml_configuration_file(const std::filesystem::path& filename);

   /**
    * Execute parameter parsing. It has to be specialized
    */
   virtual int Exec() = 0;

   /**
    * Checks the compatibility among the different parameters
    * and determines the implications
    */
   virtual void CheckParameters() = 0;

   /**
    * Friend definition of the << operator.
    */
   friend std::ostream& operator<<(std::ostream& os, const Parameter& s)
   {
      s.print(os);
      return os;
   }

   /**
    * Returns the value of an option
    * @param name is the name of the option
    * @param return the value of the option
    */
   template <typename G, std::enable_if_t<!std::is_enum<G>::value, bool> = true>
   inline G getOption(const std::string& name) const
   {
      THROW_ASSERT(Options.find(name) != Options.end(), "Option \"" + name + "\" not stored");
      return boost::lexical_cast<G>(Options.find(name)->second);
   }

   template <typename G, std::enable_if_t<std::is_enum<G>::value, bool> = true>
   inline G getOption(const std::string& name) const
   {
      THROW_ASSERT(Options.find(name) != Options.end(), "Option \"" + name + "\" not stored");
      return static_cast<G>(std::stoll(Options.find(name)->second));
   }

   /**
    * Returns the value of an option
    * @param name is the name of the option
    * @param return the value of the option
    */
   template <typename G, std::enable_if_t<!std::is_enum<G>::value, bool> = true>
   inline G getOption(const enum enum_option name) const
   {
      THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + option_name.at(name) + "\" not stored");
      return boost::lexical_cast<G>(enum_options.find(name)->second);
   }

   template <typename G, std::enable_if_t<std::is_enum<G>::value, bool> = true>
   inline G getOption(const enum enum_option name) const
   {
      THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + option_name.at(name) + "\" not stored");
      return static_cast<G>(std::stoll(enum_options.find(name)->second));
   }

   /**
    * Sets the value of an option
    * @param name is the name of the option
    * @param value is the value of the option to be saved
    */
   template <typename G, std::enable_if_t<!std::is_enum<G>::value, bool> = true>
   void setOption(const std::string& name, const G value)
   {
      Options[name] = STR(value);
   }

   template <typename G, std::enable_if_t<std::is_enum<G>::value, bool> = true>
   void setOption(const std::string& name, const G value)
   {
      Options[name] = std::to_string(static_cast<long long>(value));
   }

   /**
    * Sets the value of an option
    * @param name is the name of the option
    * @param value is the value of the option to be saved
    */
   template <typename G, std::enable_if_t<!std::is_enum<G>::value, bool> = true>
   inline void setOption(const enum enum_option name, const G value)
   {
      enum_options[name] = STR(value);
   }

   template <typename G, std::enable_if_t<std::is_enum<G>::value, bool> = true>
   inline void setOption(const enum enum_option name, const G value)
   {
      enum_options[name] = std::to_string(static_cast<long long>(value));
   }

   /**
    * Tests if an option has been stored
    * @param name is the name of the option
    * @return true if the option is in the map, false otherwise
    */
   inline bool isOption(const std::string& name) const
   {
      return Options.count(name);
   }

   /**
    * Tests if an option has been stored
    * @param name is the name of the option
    * @return true if the option is in the map, false otherwise
    */
   inline bool isOption(const enum enum_option name) const
   {
      return enum_options.count(name);
   }

   /**
    * Remove an option
    * @param name is the name of the option
    * @return true if the option has been eliminated, false otherwise
    */
   inline bool removeOption(const enum enum_option name)
   {
      return enum_options.erase(name);
   }

   /**
    * Remove an option
    * @param name is the name of the option
    * @return true if the option has been eliminated, false otherwise
    */
   inline bool removeOption(const std::string& name)
   {
      return Options.erase(name);
   }

   /**
    * Return the debug level for a specific class
    * @param name is the name the class
    * @return the corresponding level
    */
   int get_class_debug_level(const std::string& class_name, int debug_level = -1) const;

   /**
    * Return the debug_level of a function
    * @param class_name is the name of the class
    * @param function_name is the name of the function
    * @return the debug_level
    */
   int GetFunctionDebugLevel(const std::string& class_name, const std::string& function_name) const;

   void print(std::ostream& os) const;

   /**
    * Add a class to be debugged
    */
   void add_debug_class(const std::string& class_name);

   /**
    * Print the usage of this tool = PrintHeader() + PrintHelp()
    * @param os is the stream where the message has to be printed
    */
   void PrintUsage(std::ostream& os) const;

   /**
    * This function prints the version of the tool
    */
   std::string PrintVersion() const;

   /**
    * This function prints the header of the tool = PrintProgramName() + PrintVersion()
    */
   virtual void PrintFullHeader(std::ostream& os) const;

   /**
    * Print the bug report request
    * @param os is the stream where the message has to be printed
    */
   void PrintBugReport(std::ostream& os) const;

   /**
    * Return the file format given the file name or the extension
    * @param file is the file name or the extension
    * @param check_cml_root_node tells xml file has to be analyzed
    * @return the type of the file format
    */
   Parameters_FileFormat GetFileFormat(const std::filesystem::path& file, bool check_cml_root_node = false) const;

   /**
    * Returns the value of a parameter
    * @param name is the name of the parameter
    * @param return the value of the parameter
    */
   template <typename G>
   inline G GetParameter(const std::string& name) const
   {
      THROW_ASSERT(panda_parameters.find(name) != panda_parameters.end(), "Parameter \"" + name + "\" not stored");
      return boost::lexical_cast<G>(panda_parameters.find(name)->second);
   }

   /**
    * Return if a parameter has been set
    * @param name is the name of the parameter
    * @return true if the parameter has been set
    */
   inline bool IsParameter(const std::string& name) const
   {
      return panda_parameters.count(name);
   }

   /**
    * Return argv
    */
   const std::vector<std::string> CGetArgv() const;
};

using ParameterRef = refcount<Parameter>;
using ParameterConstRef = refcount<const Parameter>;

template <>
inline long long Parameter::getOption(const enum enum_option name) const
{
   THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + option_name.at(name) + "\" not stored");
   return std::stoll(enum_options.find(name)->second);
}

template <>
inline long Parameter::getOption(const enum enum_option name) const
{
   THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + option_name.at(name) + "\" not stored");
   return std::stol(enum_options.find(name)->second);
}

template <>
inline int Parameter::getOption(const enum enum_option name) const
{
   THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + option_name.at(name) + "\" not stored");
   return std::stoi(enum_options.find(name)->second);
}

template <>
inline unsigned long long Parameter::getOption(const enum enum_option name) const
{
   THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + option_name.at(name) + "\" not stored");
   return std::stoull(enum_options.find(name)->second);
}

template <>
inline unsigned long Parameter::getOption(const enum enum_option name) const
{
   THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + option_name.at(name) + "\" not stored");
   return std::stoul(enum_options.find(name)->second);
}

template <>
inline unsigned int Parameter::getOption(const enum enum_option name) const
{
   return static_cast<unsigned int>(getOption<unsigned long>(name));
}

template <>
inline double Parameter::getOption(const enum enum_option name) const
{
   THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + option_name.at(name) + "\" not stored");
   return std::stod(enum_options.find(name)->second);
}

template <>
inline std::filesystem::path Parameter::getOption(const enum enum_option name) const
{
   THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + option_name.at(name) + "\" not stored");
   return std::filesystem::path(enum_options.find(name)->second);
}

template <>
inline CustomSet<std::string> Parameter::getOption(const enum enum_option name) const
{
   return string_to_container<CustomSet<std::string>>(getOption<std::string>(name), STR_CST_string_separator);
}

template <>
inline std::list<std::string> Parameter::getOption(const enum enum_option name) const
{
   return string_to_container<std::list<std::string>>(getOption<std::string>(name), STR_CST_string_separator);
}

template <>
inline std::vector<std::string> Parameter::getOption(const enum enum_option name) const
{
   return string_to_container<std::vector<std::string>>(getOption<std::string>(name), STR_CST_string_separator);
}

template <>
inline long long Parameter::GetParameter(const std::string& name) const
{
   THROW_ASSERT(panda_parameters.find(name) != panda_parameters.end(), "Parameter \"" + name + "\" not stored");
   return std::stoll(panda_parameters.find(name)->second);
}

template <>
inline long Parameter::GetParameter(const std::string& name) const
{
   THROW_ASSERT(panda_parameters.find(name) != panda_parameters.end(), "Parameter \"" + name + "\" not stored");
   return std::stol(panda_parameters.find(name)->second);
}

template <>
inline int Parameter::GetParameter(const std::string& name) const
{
   THROW_ASSERT(panda_parameters.find(name) != panda_parameters.end(), "Parameter \"" + name + "\" not stored");
   return std::stoi(panda_parameters.find(name)->second);
}

template <>
inline unsigned long long Parameter::GetParameter(const std::string& name) const
{
   THROW_ASSERT(panda_parameters.find(name) != panda_parameters.end(), "Parameter \"" + name + "\" not stored");
   return std::stoull(panda_parameters.find(name)->second);
}

template <>
inline unsigned long Parameter::GetParameter(const std::string& name) const
{
   THROW_ASSERT(panda_parameters.find(name) != panda_parameters.end(), "Parameter \"" + name + "\" not stored");
   return std::stoul(panda_parameters.find(name)->second);
}

template <>
inline unsigned int Parameter::GetParameter(const std::string& name) const
{
   THROW_ASSERT(panda_parameters.find(name) != panda_parameters.end(), "Parameter \"" + name + "\" not stored");
   return static_cast<unsigned int>(GetParameter<unsigned long>(name));
}

template <>
inline double Parameter::GetParameter(const std::string& name) const
{
   THROW_ASSERT(panda_parameters.find(name) != panda_parameters.end(), "Parameter \"" + name + "\" not stored");
   return std::stod(panda_parameters.find(name)->second);
}
#endif
