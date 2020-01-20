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
 * @file Parameter.hpp
 * @brief this class is used to manage the command-line or XML options. It has to be specialized with respect to the tool
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef PARAMETER_HPP
#define PARAMETER_HPP

/// Autoheader include
#include "config_HAVE_BAMBU_RESULTS_XML.hpp"
#include "config_HAVE_CODE_ESTIMATION_BUILT.hpp"
#include "config_HAVE_DESIGN_ANALYSIS_BUILT.hpp"
#include "config_HAVE_DIOPSIS.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_AADL_ASN_BUILT.hpp"
#include "config_HAVE_FROM_ARCH_BUILT.hpp"
#include "config_HAVE_FROM_C_BUILT.hpp"
#include "config_HAVE_FROM_LIBERTY.hpp"
#include "config_HAVE_FROM_PSPLIB_BUILT.hpp"
#include "config_HAVE_FROM_SDF3_BUILT.hpp"
#include "config_HAVE_HLS_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_REGRESSORS_BUILT.hpp"
#include "config_HAVE_SOURCE_CODE_STATISTICS_XML.hpp"
#include "config_HAVE_TARGET_PROFILING.hpp"
#include "config_HAVE_TECHNOLOGY_BUILT.hpp"
#include "config_HAVE_TO_C_BUILT.hpp"
#include "config_RELEASE.hpp"

/// Utility include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "refcount.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <climits>
#include <list>
#include <string>
#include <vector>

/// forward decl of xml Element
class xml_element;
#if HAVE_HLS_BUILT
enum class CliqueCovering_Algorithm;
enum class Evaluation_Mode;
enum class HLSFlowStep_Type;
enum class MemoryAllocation_ChannelsType;
enum class MemoryAllocation_Policy;
enum class ParametricListBased_Metric;
enum class SDCScheduling_Algorithm;
#endif
#if HAVE_HOST_PROFILING_BUILT
enum class HostProfiling_Method;
#endif
#if HAVE_FROM_C_BUILT
enum class ActorGraphBackend_Type;
enum class GccWrapper_CompilerTarget;
#endif
#if HAVE_DESIGN_ANALYSIS_BUILT
enum class DesignAnalysis_Step;
#endif
#if HAVE_CODE_ESTIMATION_BUILT
enum class ActorGraphEstimator_Algorithm;
#endif
#if HAVE_TARGET_PROFILING
enum class InstrumentWriter_Level;
enum class TargetArchitecture_Kind;
#endif
#if HAVE_DIOPSIS
enum class DiopsisInstrumentWriter_Type;
#endif

/// An integer value to return if parameters have been right parsed
#define PARAMETER_PARSED INT_MIN
#define PARAMETER_NOTPARSED INT_MAX

#define BAMBU_OPTIONS                                                                                                                                                                                                                                          \
   (chaining)(chaining_algorithm)(constraints_file)(context_switch)(controller_architecture)(datapath_architecture)(distram_threshold)(DSP_allocation_coefficient)(DSP_margin_combinational)(DSP_margin_pipelined)(estimate_logic_and_connections)(            \
       evaluation)(evaluation_mode)(evaluation_objectives)(experimental_setup)(export_core)(export_core_mode)(fsm_encoding)(fu_binding_algorithm)(generate_testbench)(generate_vcd)(hls_flow)(hls_div)(hls_fpdiv)(interface)(interface_type)(additional_top)(  \
       data_bus_bitsize)(addr_bus_bitsize)(libm_std_rounding)(liveness_algorithm)(scheduling_mux_margins)(scheduling_priority)(scheduling_algorithm)(simulate)(simulator)(simulation_output)(speculative)(pipelining)(storage_value_insertion_algorithm)(stg)( \
       stg_algorithm)(register_allocation_algorithm)(register_grouping)(registered_inputs)(resp_model)(datapath_interconnection_algorithm)(insert_memory_profile)(timing_simulation)(top_file)(assert_debug)(memory_allocation_algorithm)(                     \
       memory_allocation_policy)(xml_memory_allocation)(rom_duplication)(base_address)(sync_reset)(level_reset)(reg_init_value)(clock_period_resource_fraction)(channels_type)(channels_number)(memory_controller_type)(soft_float)(soft_fp)(                  \
       softfloat_subnormal)(max_sim_cycles)(sparse_memory)(max_ulp)(skip_pipe_parameter)(gcc_serialize_memory_accesses)(unaligned_access)(aligned_access)(backend_script_extensions)(backend_sdc_extensions)(VHDL_library)(bitvalue_ipa)(                      \
       use_asynchronous_memories)(do_not_chain_memories)(bram_high_latency)(cdfc_module_binding_algorithm)(function_allocation_algorithm)(testbench_input_string)(testbench_input_xml)(weighted_clique_register_algorithm)(disable_function_proxy)(            \
       memory_mapped_top)(do_not_expose_globals)(connect_iob)(profiling_output)(disable_bounded_function)(discrepancy)(discrepancy_force)(discrepancy_hw)(discrepancy_no_load_pointers)(discrepancy_only)(discrepancy_permissive_ptrs)(dry_run_evaluation)(    \
       find_max_cfg_transformations)(generate_taste_architecture)(initial_internal_address)(mem_delay_read)(mem_delay_write)(memory_banks_number)(mixed_design)(no_parse_c_python)(num_accelerators)(post_rescheduling)(technology_file)(                      \
       testbench_extra_gcc_flags)(timing_violation_abort)(top_design_name)(visualizer)(serialize_output)(use_ALUs)

#define FRAMEWORK_OPTIONS                                                                                                                                                                                                                                     \
   (architecture)(benchmark_name)(cat_args)(cfg_max_transformations)(compatible_compilers)(compute_size_of)(configuration_name)(debug_level)(default_compiler)(dot_directory)(dump_profiling_data)(file_costs)(file_input_data)(host_compiler)(ilp_max_time)( \
       ilp_solver)(input_file)(input_format)(model_costs)(no_clean)(no_parse_files)(no_return_zero)(output_file)(output_level)(output_temporary_directory)(output_directory)(panda_parameter)(parse_pragma)(pretty_print)(print_dot)(profiling_file)(         \
       profiling_method)(program_name)(read_parameter_xml)(revision)(seed)(task_threshold)(test_multiple_non_deterministic_flows)(test_single_non_deterministic_flow)(top_functions_names)(use_rtl)(xml_input_configuration)(xml_output_configuration)(       \
       write_parameter_xml)

#define GCC_OPTIONS                                                                                                                                                                                                                                         \
   (gcc_config)(gcc_costs)(gcc_defines)(gcc_extra_options)(gcc_include_sysdir)(gcc_includes)(gcc_libraries)(gcc_library_directories)(gcc_openmp_simd)(gcc_opt_level)(gcc_m32_mx32)(gcc_optimizations)(gcc_optimization_set)(gcc_parameters)(gcc_plugindir)( \
       gcc_read_xml)(gcc_standard)(gcc_undefines)(gcc_warnings)(gcc_c)(gcc_E)(gcc_S)(gcc_write_xml)

#define GECCO_OPTIONS (algorithms)(analyses)

#define KOALA_OPTIONS                                                                                                                                                                                                                                         \
   (aig_analysis)(aig_analysis_algorithm)(apply_reduction_to_standard_library)(characterization_with_DC)(circuit_debug_level)(complete_library_post_covering)(complete_library_pre_covering)(covering)(csv_file)(design_compiler_effort)(                     \
       drive_strength_values)(equation)(evolutionary_reduction)(explore_cell_variants)(extract_features)(generated_library_name)(group_glue)(has_complete_characterization)(hdl_backend)(icarus_debug_level)(input_libraries)(library)(library_optimization)( \
       library_optimization_algorithm)(lib_output_format)(max_area)(max_delay)(output_libraries)(output_name)(regularity_abstraction_level)(regularity_algorithm)(regularity_coloring_type)(regularity_covering)(regularity_extraction)(regularity_fast)(     \
       regularity_forward)(regularity_hierarchical)(regularity_include_sequential)(regularity_max_inputs)(regularity_min_frequency)(regularity_min_size)(regularity_window_size)(reordering)(perform_resynthesis)(print_templates)(                           \
       reimplement_standard_cells)(separate_templates)(set_constraint)(set_optimization_goal)(skew_values)(split_roots)(store_library_creator_db)(synthesis_tool_xml)(template_file)(xml_library_cells)(xml_library_statistics)

#define SYNTHESIS_OPTIONS                                                                                                                                                                                                                           \
   (clock_period)(clock_name)(reset_name)(start_name)(done_name)(design_analysis_steps)(design_compiler_compile_log)(design_compiler_split_log)(design_parameters)(design_hierarchy)(device_string)(dump_genlib)(estimate_library)(export_ip_core)( \
       import_ip_core)(input_liberty_library_file)(ip_xact_architecture_template)(ip_xact_parameters)(is_structural)(lib2xml)(min_metric)(parse_edif)(rtl)(synthesis_flow)(structural_HDL)(target_device)(target_library)(target_library_source)(   \
       target_technology)(target_technology_file)(target_device_file)(target_device_script)(target_device_type)(top_component)(uniquify)(writer_language)

#define SPIDER_OPTIONS                                                                                                                                                                                                                              \
   (accuracy)(aggregated_features)(cross_validation)(experimental_setup_file)(interval_level)(latex_format_file)(max_bound)(maximum_error)(min_bound)(minimum_significance)(normalization_file)(normalization_sequences)(output_format)(precision)( \
       processing_element_type)(skip_rows)(surviving_benchmarks)

#define EUCALIPTUS_OPTIONS (component_name)

#define TREE_PANDA_GCC_OPTIONS (archive_files)(obj_files)(compress_archive)

#define ZEBU_OPTIONS                                                                                                                                                                                                                                           \
   (alternative_metrics)(analysis_level)(blackbox)(cache_analysis)(compare_model_max_iterations)(compare_measure_regions)(compare_models)(cpus_number)(cuda_optimization)(examined_model)(default_fork_cost)(diopsis_instrumentation)(driving_component_type)( \
       driving_metric)(dump_schedule)(evaluate_pointed_size)(exec_argv)(frontend_statistics)(golden_model)(fork_join_backend)(hand_mapping)(ignore_mapping)(ignore_parallelism)(mapping)(measure_profile_overhead)(memory_profiling)(no_sequential)(           \
       normalize_models)(partitioning)(partitioning_algorithm)(partitioning_functions)(path)(performance_estimation)(platform_base_dir)(prof_resolution)(profile_loop_max_iterations)(profile_minutes_timeout)(runs_number)(simit_fork_cost)(                  \
       source_code_statistics)(resolution)(run)(sequence_length)(shorter_sequence)(symbolic_simulation)(trace_buffer_size)(tsim_instrumentation)(tollerance)(without_operating_system)(without_transformation)

#define OPTIONS_ENUM(r, data, elem) BOOST_PP_CAT(OPT_, elem),

/// Possible options
// cppcheck-suppress syntaxError
enum enum_option
{
   BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, BAMBU_OPTIONS) BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, EUCALIPTUS_OPTIONS) BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, FRAMEWORK_OPTIONS)
       BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, GCC_OPTIONS) BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, GECCO_OPTIONS) BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, KOALA_OPTIONS)
           BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, SPIDER_OPTIONS) BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, SYNTHESIS_OPTIONS) BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, TREE_PANDA_GCC_OPTIONS)
               BOOST_PP_SEQ_FOR_EACH(OPTIONS_ENUM, BOOST_PP_EMPTY, ZEBU_OPTIONS)
};

class OptionMap : public std::map<std::string, std::string>
{
 public:
   /**
    * Constructor
    */
   OptionMap() = default;

   /**
    * Destructor
    */
   ~OptionMap() = default;
};

#define DEFAULT_OPT_BASE 512
#define OPT_READ_PARAMETERS_XML DEFAULT_OPT_BASE
#define OPT_WRITE_PARAMETERS_XML DEFAULT_OPT_BASE + 1
#define OPT_DEBUG_CLASSES DEFAULT_OPT_BASE + 2
#define OPT_BENCHMARK_NAME DEFAULT_OPT_BASE + 3
#define OPT_BENCHMARK_FAKE_PARAMETERS DEFAULT_OPT_BASE + 4
#define INPUT_OPT_ERROR_ON_WARNING DEFAULT_OPT_BASE + 5
#define OPT_OUTPUT_TEMPORARY_DIRECTORY DEFAULT_OPT_BASE + 6
#define INPUT_OPT_PRINT_DOT DEFAULT_OPT_BASE + 7
#define INPUT_OPT_SEED DEFAULT_OPT_BASE + 8
#define INPUT_OPT_NO_CLEAN DEFAULT_OPT_BASE + 9
#define INPUT_OPT_CONFIGURATION_NAME DEFAULT_OPT_BASE + 10
#define INPUT_OPT_CFG_MAX_TRANSFORMATIONS DEFAULT_OPT_BASE + 11
#define INPUT_OPT_PANDA_PARAMETER DEFAULT_OPT_BASE + 12

/// define the default tool short option string
#define COMMON_SHORT_OPTIONS_STRING "hVv:d:"

/// define default TOOL long options
#define COMMON_LONG_OPTIONS                                                                                                                                                                                                                                  \
   {"help", no_argument, nullptr, 'h'}, {"verbosity", required_argument, nullptr, 'v'}, {"version", no_argument, nullptr, 'V'}, {"read-parameters-XML", required_argument, nullptr, OPT_READ_PARAMETERS_XML},                                                \
       {"write-parameters-XML", required_argument, nullptr, OPT_WRITE_PARAMETERS_XML}, {"debug", required_argument, nullptr, 'd'}, {"debug-classes", required_argument, nullptr, OPT_DEBUG_CLASSES}, {"no-clean", no_argument, nullptr, INPUT_OPT_NO_CLEAN}, \
       {"benchmark-name", required_argument, nullptr, OPT_BENCHMARK_NAME}, {"configuration-name", required_argument, nullptr, INPUT_OPT_CONFIGURATION_NAME}, {"benchmark-fake-parameters", required_argument, nullptr, OPT_BENCHMARK_FAKE_PARAMETERS},       \
       {"output-temporary-directory", required_argument, nullptr, OPT_OUTPUT_TEMPORARY_DIRECTORY}, {"error-on-warning", no_argument, nullptr, INPUT_OPT_ERROR_ON_WARNING}, {"print-dot", no_argument, nullptr, INPUT_OPT_PRINT_DOT},                         \
       {"seed", required_argument, nullptr, INPUT_OPT_SEED}, {"cfg-max-transformations", required_argument, nullptr, INPUT_OPT_CFG_MAX_TRANSFORMATIONS},                                                                                                     \
   {                                                                                                                                                                                                                                                         \
      "panda-parameter", required_argument, nullptr, INPUT_OPT_PANDA_PARAMETER                                                                                                                                                                               \
   }

#define INPUT_OPT_CUSTOM_OPTIONS 1024
#define INPUT_OPT_COMPUTE_SIZEOF 1 + INPUT_OPT_CUSTOM_OPTIONS
#define INPUT_OPT_COMPILER 1 + INPUT_OPT_COMPUTE_SIZEOF
#define INPUT_OPT_GCC_CONFIG 1 + INPUT_OPT_COMPILER
#define INPUT_OPT_INCLUDE_SYSDIR 1 + INPUT_OPT_GCC_CONFIG
#define INPUT_OPT_PARAM 1 + INPUT_OPT_INCLUDE_SYSDIR
#define INPUT_OPT_READ_GCC_XML 1 + INPUT_OPT_PARAM
#define INPUT_OPT_STD 1 + INPUT_OPT_READ_GCC_XML
#define INPUT_OPT_USE_RAW 1 + INPUT_OPT_STD
#define INPUT_OPT_WRITE_GCC_XML 1 + INPUT_OPT_USE_RAW
#define LAST_GCC_OPT INPUT_OPT_WRITE_GCC_XML

/// define the GCC short option string
#define GCC_SHORT_OPTIONS_STRING "cf:I:D:U:O::l:L:W:Em:g::"

#if !RELEASE
#define GCC_LONG_OPTIONS_RAW_XML {"use-raw", no_argument, nullptr, INPUT_OPT_USE_RAW}, {"read-GCC-XML", required_argument, nullptr, INPUT_OPT_READ_GCC_XML}, {"write-GCC-XML", required_argument, nullptr, INPUT_OPT_WRITE_GCC_XML},
#else
#define GCC_LONG_OPTIONS_RAW_XML {"use-raw", no_argument, nullptr, INPUT_OPT_USE_RAW},
#endif
#define GCC_LONG_OPTIONS_COMPILER {"compiler", required_argument, nullptr, INPUT_OPT_COMPILER},

#define GCC_LONG_OPTIONS                                                                                                                                                                                                            \
   GCC_LONG_OPTIONS_COMPILER{"std", required_argument, nullptr, INPUT_OPT_STD}, GCC_LONG_OPTIONS_RAW_XML{"param", required_argument, nullptr, INPUT_OPT_PARAM}, {"Include-sysdir", no_argument, nullptr, INPUT_OPT_INCLUDE_SYSDIR}, \
       {"gcc-config", no_argument, nullptr, INPUT_OPT_GCC_CONFIG}, {"compute-sizeof", no_argument, nullptr, INPUT_OPT_COMPUTE_SIZEOF},                                                                                              \
   {                                                                                                                                                                                                                                \
      "extra-gcc-options", required_argument, nullptr, INPUT_OPT_CUSTOM_OPTIONS                                                                                                                                                     \
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
#endif
   FF_CSV, /**< (Input) comma separated value */
#if HAVE_EXPERIMENTAL
   FF_CSV_RTL, /**< (Output) comma separated value rtl sequences */
   FF_CSV_TRE, /**< (Output) comma seperated value tree sequences */
#endif
#if HAVE_FROM_LIBERTY
   FF_LIB, /**< (Input) Liberty file */
#endif
#if HAVE_EXPERIMENTAL
   FF_LOG, /**< (Input) log file */
   FF_PA,  /**< (Input) Profiling analysis */
#endif
#if HAVE_FROM_PSPLIB_BUILT
   FF_PSPLIB_MM, /**< (Input) Multi-mode Project Scheduling Problem */
   FF_PSPLIB_SM, /**< (Input) Single-mode Project Scheduling Problem */
#endif
#if HAVE_FROM_C_BUILT
   FF_RAW, /**< (Input/Output) raw file */
#endif
   FF_TEX,     /**< (Output) Latex table */
   FF_TGFF,    /**< (Input) task graph for free */
   FF_VERILOG, /**< (Input) verilog */
   FF_VHDL,    /**< (Input) vhdl */
   FF_XML,     /**< (Input/Output) XML */
#if HAVE_REGRESSORS_BUILT
   FF_XML_AGG, /**< (Input) XML aggregated features */
#endif
#if HAVE_FROM_ARCH_BUILT
   FF_XML_ARCHITECTURE, /**< (Input) XML architecture file */
#endif
#if HAVE_BAMBU_RESULTS_XML
   FF_XML_BAMBU_RESULTS, /**< (Input) XML bambu results*/
#endif
#if HAVE_FROM_LIBERTY
   FF_XML_CELLS, /**< (Input) XML describing list of cells */
#endif
#if HAVE_HLS_BUILT
   FF_XML_CON, /**< (Input) XML storing constraints */
#endif
#if HAVE_DESIGN_ANALYSIS_BUILT
   FF_XML_DESIGN_HIERARCHY, /**< (Input) XML storing hierarchy of a design */
#endif
   FF_XML_EXPERIMENTAL_SETUP, /**< (Input) XML storing experimental setup */
   FF_XML_IP_XACT_COMPONENT,  /**< (Input) XML storing IP-XACT component */
   FF_XML_IP_XACT_DESIGN,     /**< (Input) XML storing IP-XACT design */
   FF_XML_IP_XACT_GENERATOR,  /**< (Input) XML storing IP-XACT generator chain */
   FF_XML_IP_XACT_CONFIG,     /**< (Input) XML storing IP-XACT design configuration */
#if HAVE_FROM_SDF3_BUILT
   FF_XML_SDF3, /**< (Input) XML storing synchronous data flow graph */
#endif
   FF_XML_SKIP_ROW, /**< (Input) XML benchhmarks to be execluded from training set */
#if HAVE_SOURCE_CODE_STATISTICS_XML
   FF_XML_STAT, /**< (Input) XML statistics about source code files */
#endif
   FF_XML_SYM_SIM, /**< (Input) XML storing symbolic symulation results */
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
   OptionMap Options;

   /// Map between the name of a parameter and the related string-form value
   CustomMap<std::string, std::string> panda_parameters;

   /// Map between an enum option and the related string-form value
   std::map<enum enum_option, std::string> enum_options;

   /// Name of the enum options
   std::map<enum enum_option, std::string> option_name;

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
    * Manage Gcc options(common to zebu and bambu)
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
   void write_xml_configuration_file(const std::string& filename);

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
    * @param variable is the variable where the value of the option will be saved
    */
   template <typename G>
   void getOption(const std::string& name, G& variable) const
   {
      THROW_ASSERT(Options.find(name) != Options.end(), "Option \"" + name + "\" not stored");
      variable = boost::lexical_cast<G>(Options.find(name)->second);
   }

   /**
    * Returns the value of an option
    * @param name is the name of the option
    * @param variable is the variable where the value of the option will be saved
    */
   template <typename G>
   void getOption(const char* name, G& variable) const
   {
      getOption(std::string(name), variable);
   }

   /**
    * Returns the value of an option
    * @param name is the name of the option
    * @param return the value of the option
    */
   template <typename G>
   G getOption(const std::string& name) const
   {
      THROW_ASSERT(Options.find(name) != Options.end(), "Option \"" + name + "\" not stored");
      return boost::lexical_cast<G>(Options.find(name)->second);
   }

   /**
    * Returns the value of an option
    * @param name is the name of the option
    * @param return the value of the option
    */
   template <typename G>
   G getOption(const char* name) const
   {
      return getOption<G>(std::string(name));
   }

   /**
    * Returns the value of an option
    * @param name is the name of the option
    * @param return the value of the option
    */
   template <typename G>
   G getOption(const enum enum_option name) const
   {
      THROW_ASSERT(enum_options.find(name) != enum_options.end(), "Option \"" + (option_name.find(name))->second + "\" not stored");
      return boost::lexical_cast<G>(enum_options.find(name)->second);
   }

   /**
    * Sets the value of an option
    * @param name is the name of the option
    * @param value is the value of the option to be saved
    */
   template <typename G>
   void setOption(const std::string& name, const G value)
   {
      Options[name] = boost::lexical_cast<std::string>(value);
   }

   /**
    * Sets the value of an option
    * @param name is the name of the option
    * @param value is the value of the option to be saved
    */
   template <typename G>
   void setOption(const char* name, const G value)
   {
      Options[std::string(name)] = boost::lexical_cast<std::string>(value);
   }

   /**
    * Sets the value of an option
    * @param name is the name of the option
    * @param value is the value of the option to be saved
    */
   template <typename G>
   void setOption(const enum enum_option name, const G value)
   {
      enum_options[name] = boost::lexical_cast<std::string>(value);
   }

   /**
    * Tests if an option has been stored
    * @param name is the name of the option
    * @return true if the option is in the map, false otherwise
    */
   bool isOption(const std::string& name) const
   {
      return Options.find(name) != Options.end();
   }

   /**
    * Tests if an option has been stored
    * @param name is the name of the option
    * @return true if the option is in the map, false otherwise
    */
   bool isOption(const char* name) const
   {
      return isOption(std::string(name));
   }

   /**
    * Tests if an option has been stored
    * @param name is the name of the option
    * @return true if the option is in the map, false otherwise
    */
   bool isOption(const enum enum_option name) const
   {
      return enum_options.find(name) != enum_options.end();
   }

   /**
    * Remove an option
    * @param name is the name of the option
    * @return true if the option has been eliminated, false otherwise
    */
   bool removeOption(const enum enum_option name)
   {
      if(!isOption(name))
         return false;
      enum_options.erase(name);
      return true;
   }

   /**
    * Remove an option
    * @param name is the name of the option
    * @return true if the option has been eliminated, false otherwise
    */
   bool removeOption(const char* name)
   {
      return removeOption(std::string(name));
   }

   /**
    * Remove an option
    * @param name is the name of the option
    * @return true if the option has been eliminated, false otherwise
    */
   bool removeOption(const std::string& name)
   {
      if(!isOption(name))
         return false;
      Options.erase(name);
      return true;
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
   Parameters_FileFormat GetFileFormat(const std::string& file, bool check_cml_root_node = false) const;

   /**
    * Returns the value of a parameter
    * @param name is the name of the parameter
    * @param return the value of the parameter
    */
   template <typename G>
   G GetParameter(const std::string& name) const
   {
      THROW_ASSERT(panda_parameters.find(name) != panda_parameters.end(), "Parameter \"" + name + "\" not stored");
      return boost::lexical_cast<G>(panda_parameters.find(name)->second);
   }

   /**
    * Return if a parameter has been set
    * @param name is the name of the parameter
    * @return true if the parameter has been set
    */
   bool IsParameter(const std::string& name) const;

   /**
    * Return argv
    */
   const std::vector<std::string> CGetArgv() const;
};

template <>
const CustomSet<std::string> Parameter::getOption(const enum enum_option name) const;

template <>
const std::list<std::string> Parameter::getOption(const enum enum_option name) const;

#if HAVE_TARGET_PROFILING
template <>
InstrumentWriter_Level Parameter::getOption(const enum enum_option name) const;
#endif

#if HAVE_HOST_PROFILING_BUILT
template <>
HostProfiling_Method Parameter::getOption(const enum enum_option name) const;
#endif

#if HAVE_TARGET_PROFILING
template <>
TargetArchitecture_Kind Parameter::getOption(const enum enum_option name) const;
#endif

template <>
Parameters_FileFormat Parameter::getOption(const enum enum_option name) const;

#if HAVE_FROM_C_BUILT
template <>
GccWrapper_CompilerTarget Parameter::getOption(const enum enum_option name) const;
#endif

#if HAVE_CODE_ESTIMATION_BUILT
template <>
CustomUnorderedSet<ActorGraphEstimator_Algorithm> Parameter::getOption(const enum enum_option name) const;

template <>
ActorGraphEstimator_Algorithm Parameter::getOption(const enum enum_option name) const;
#endif

#if HAVE_DIOPSIS
template <>
DiopsisInstrumentWriter_Type Parameter::getOption(const enum enum_option name) const;
#endif

#if HAVE_DESIGN_ANALYSIS_BUILT
template <>
DesignAnalysis_Step Parameter::getOption(const enum enum_option name) const;
#endif

#if HAVE_FROM_C_BUILT
enum class GccWrapper_OptimizationSet;
template <>
GccWrapper_OptimizationSet Parameter::getOption(const enum enum_option name) const;
template <>
void Parameter::setOption(const enum enum_option name, const GccWrapper_OptimizationSet value);
#endif

#if HAVE_TO_C_BUILT
template <>
ActorGraphBackend_Type Parameter::getOption(const enum enum_option name) const;
#endif

#if HAVE_HLS_BUILT
template <>
HLSFlowStep_Type Parameter::getOption(const enum enum_option name) const;
template <>
void Parameter::setOption(const enum enum_option name, const HLSFlowStep_Type hls_flow_step_type);

template <>
MemoryAllocation_Policy Parameter::getOption(const enum enum_option name) const;
template <>
void Parameter::setOption(const enum enum_option name, const MemoryAllocation_Policy memory_allocation_policy);

template <>
MemoryAllocation_ChannelsType Parameter::getOption(const enum enum_option name) const;
template <>
void Parameter::setOption(const enum enum_option name, const MemoryAllocation_ChannelsType memory_allocation_channels_type);

template <>
CliqueCovering_Algorithm Parameter::getOption(const enum enum_option name) const;
template <>
void Parameter::setOption(const enum enum_option name, const CliqueCovering_Algorithm clique_covering_algorithm);

template <>
Evaluation_Mode Parameter::getOption(const enum enum_option name) const;
template <>
void Parameter::setOption(const enum enum_option name, const Evaluation_Mode evaluation_mode);

template <>
ParametricListBased_Metric Parameter::getOption(const enum enum_option name) const;
template <>
void Parameter::setOption(const enum enum_option name, const ParametricListBased_Metric parametric_list_based_metric);
#endif

typedef refcount<Parameter> ParameterRef;
typedef refcount<const Parameter> ParameterConstRef;

#endif
