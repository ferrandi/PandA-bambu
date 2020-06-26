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
 * @file SpiderParameter.cpp
 * @brief
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_FROM_CSV_BUILT.hpp"
#include "config_HAVE_R.hpp"
#include "config_HAVE_REGRESSORS_BUILT.hpp"
#include "config_HAVE_TECHNOLOGY_BUILT.hpp"
#include "config_RELEASE.hpp"

/// Constants include
#include "constant_strings.hpp"
#include "constants.hpp"
#if HAVE_REGRESSORS_BUILT
#include "regressors_constants.hpp"
#endif

/// Header include
#include "SpiderParameter.hpp"

/// Backend include
#include "translator.hpp"

/// STD include
#include <getopt.h>
#include <string>

/// STL include
#include <vector>

/// Utility include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "fileIO.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

#include <boost/algorithm/string/replace.hpp>

/// PARAMETERS STUFF ***********************///
#define INPUT_OPT_ACCURACY 256
#define INPUT_OPT_BENCHMARK_BOUNDS (1 + INPUT_OPT_ACCURACY)
#define INPUT_OPT_COMPONENTS (1 + INPUT_OPT_BENCHMARK_BOUNDS)
#define INPUT_OPT_CROSS_VALIDATION (1 + INPUT_OPT_COMPONENTS)
#define INPUT_OPT_MINIMUM_SIGNIFICANCE (1 + INPUT_OPT_CROSS_VALIDATION)
#define INPUT_OPT_NORMALIZATION_SEQUENCES (1 + INPUT_OPT_MINIMUM_SIGNIFICANCE)
#define INPUT_OPT_NORMALIZATION_WEIGHT_FILE (1 + INPUT_OPT_NORMALIZATION_SEQUENCES)
#define INPUT_OPT_PREDICTION_INTERVAL_VALUE (1 + INPUT_OPT_NORMALIZATION_WEIGHT_FILE)
#define INPUT_OPT_SEQUENCE_LENGTH (1 + INPUT_OPT_PREDICTION_INTERVAL_VALUE)
#define INPUT_OPT_SURVIVING_BENCHMARKS (1 + INPUT_OPT_SEQUENCE_LENGTH)
#define TOOL_OPT_BASE (1 + INPUT_OPT_SURVIVING_BENCHMARKS)

void SpiderParameter::PrintHelp(std::ostream& os) const
{
   os << "Usage: " << getOption<std::string>(OPT_program_name) << " [options] input_file [input_files] output_file" << std::endl;
   os << std::endl;
   os << "Options: \n"
      << "\n";
   PrintGeneralOptionsUsage(os);
   os << "\n"
      << "  Input options:\n"
      << "    --sequence_length               Specify the length of the sequences\n"
      << "    --sequence-length               Specify the length of the sequences\n"
      << "    --surviving-benchmarks=<number> Remove all but <number> benchmarks from the input (default=300)\n"
      << "    --simulator, -t <simulator>     Specify the simulator used in model building:"
      << "                                      diopsis (default)\n"
      << "                                      tsim\n"
      << "                                      simit\n"
      << "    --analysis-level=<value>        Set the analysis level for instructions sequences analysis (default=0): \n"
      << "                                      0: no analysis is performed;\n"
      << "                                      1: analysis is performed at application level. Only aggregate information of all benchmarks \n"
      << "                                      will be saved on XML file;\n"
      << "                                      2: analysis is performed at application level. Aggregate information of all benchmarks and\n"
      << "                                      data of each single benchmark will be saved on XML file;\n"
      << "                                      3: analysis is performed at function level. XML file will be completely generated(file contents:\n"
      << "                                      aggregate information of all benchmarks,  data of each single benchmark and data of each \n"
      << "                                      function analyzed); \n"
      << "\n";
   PrintOutputOptionsUsage(os);
   os << "\n"
      << "  Input options:\n"
      << "    --input-format, -I <extension>  Format of the input file\n"
      << "    --normalize, -n <input_file>    Input file storing the normalizing information\n"
      << "    --processing-element, -p <pe>   Specify of which processing element model weights are\n"
      << "    --accuracy=<level>              Accuracy level while evaluating similar benchmarks (default=0)\n"
      << "    --benchmark-bounds=<number,number>\n"
      << "                                    Set the bound of average execution cycles of benchmarks used for training.\n"
      << "\n"
      << "  Output options:\n"
      << "    --output-format, -O <extension> Format of the output file\n"
      << "\n"
      << "  Possible formats are:\n"
      << "     csv        comma separated values\n"
      << "     rtl.csv    comma separated values of rtl sequences\n"
      << "     tex        latex table\n"
      << "     tree.csv   comma separated values of tree sequences\n"
      << "     xml        xml files\n"
      << "\n"
      << "  Other options:\n"
#if HAVE_R
      << "     --cross-validation=<value>     The value of cross validiation fold\n"
      << "     --minimum-significance         The minimum significance required for a regressor\n"
#endif
      << "     --normalization-sequences      Sequences of transformations to be applied before building performance model; sequences can be composed of:\n"
      << "                                    'L' - remove pseudo-linear dependent benchmarks\n"
      << "                                    'M' - remove smallest and largest benchmarks\n"
      << "                                    'R' - remove smallest benchmarks\n"
      << "                                    'S' - normalize benchmarks\n"
#if HAVE_R
      << "     --prediction-interval-value=<value>\n"
      << "                                    The value of the prediction interval to be computed for each benchmark\n"
#endif
       ;
#if HAVE_TECHNOLOGY_BUILT
   os << "     --components=<value>           The components to be inserted in the list of functional units to be characterized (default=all).\n";
#endif
}

void SpiderParameter::PrintProgramName(std::ostream& os) const
{
   os << std::endl;
   os << "********************************************************************************" << std::endl;
   os << "                         ____        _     _" << std::endl;
   os << "                        / ___| _ __ (_) __| | ___ _ __" << std::endl;
   os << "                        \\___ \\| '_ \\| |/ _` |/ _ \\ '__|" << std::endl;
   os << "                         ___) | |_) | | (_| |  __/ |" << std::endl;
   os << "                        |____/| .__/|_|\\__,_|\\___|_|" << std::endl;
   os << "                              |_|" << std::endl;
   os << std::endl;
   os << "********************************************************************************" << std::endl;
}

SpiderParameter::SpiderParameter(const std::string& _program_name, int _argc, char** const _argv) : Parameter(_program_name, _argc, _argv)
{
   SetDefaults();
}

int SpiderParameter::Exec()
{
   exit_code = PARAMETER_NOTPARSED;

   /// variable used into option parsing
   int option_index;

   const char* const short_options = COMMON_SHORT_OPTIONS_STRING "I:O:p:";

   const struct option long_options[] = {
      COMMON_LONG_OPTIONS,
      {"accuracy", required_argument, nullptr, INPUT_OPT_ACCURACY},
      {"benchmark-bounds", required_argument, nullptr, INPUT_OPT_BENCHMARK_BOUNDS},
#if HAVE_TECHNOLOGY_BUILT
      {"components", required_argument, nullptr, INPUT_OPT_COMPONENTS},
#endif
#if HAVE_R
      {"cross-validation", required_argument, nullptr, INPUT_OPT_CROSS_VALIDATION},
#endif
      {"input-format", required_argument, nullptr, 'I'},
#if HAVE_R
      {"minimum-significance", required_argument, nullptr, INPUT_OPT_MINIMUM_SIGNIFICANCE},
#endif
      {"normalization-sequences", required_argument, nullptr, INPUT_OPT_NORMALIZATION_SEQUENCES},
      {"normalize", required_argument, nullptr, INPUT_OPT_NORMALIZATION_WEIGHT_FILE},
      {"output-format", required_argument, nullptr, 'O'},
      {"prediction-interval-value", required_argument, nullptr, INPUT_OPT_PREDICTION_INTERVAL_VALUE},
      {"processing-element", required_argument, nullptr, 'p'},
      {"sequence-length", required_argument, nullptr, INPUT_OPT_SEQUENCE_LENGTH},
      {"surviving-benchmarks", required_argument, nullptr, INPUT_OPT_SURVIVING_BENCHMARKS},
      {nullptr, 0, nullptr, 0}
   };
   if(argc == 1) // Panda called without arguments
   {
      PrintUsage(std::cerr);
      return EXIT_SUCCESS;
   }
   while(true)
   {
      int next_option = getopt_long(argc, argv, short_options, long_options, &option_index);

      // no more options are available
      if(next_option == -1)
      {
         break;
      }

      switch(next_option)
      {
         case 'I':
         {
            setOption(OPT_input_format, optarg);
            break;
         }
         case 'O':
         {
            setOption(OPT_output_format, optarg);
            break;
         }
         case 'p':
         {
            setOption(OPT_processing_element_type, optarg);
            break;
         }
         case INPUT_OPT_ACCURACY:
         {
            setOption(OPT_accuracy, optarg);
            break;
         }
         case INPUT_OPT_BENCHMARK_BOUNDS:
         {
            std::string to_be_splitted(optarg);
            std::vector<std::string> splitted = SplitString(to_be_splitted, ",");
            if(splitted.size() != 2)
            {
               THROW_ERROR("Error in argument of --benchmark-bounds: " + std::string(optarg));
            }
            setOption(OPT_min_bound, splitted[0]);
            setOption(OPT_max_bound, splitted[1]);
            break;
         }
         case INPUT_OPT_COMPONENTS:
         {
            setOption(OPT_component_name, optarg);
            break;
         }
         case INPUT_OPT_CROSS_VALIDATION:
         {
            setOption(OPT_cross_validation, optarg);
            break;
         }
#if HAVE_R
         case INPUT_OPT_MINIMUM_SIGNIFICANCE:
         {
            setOption(OPT_minimum_significance, optarg);
            break;
         }
#endif
         case INPUT_OPT_NORMALIZATION_SEQUENCES:
         {
            setOption(OPT_normalization_sequences, optarg);
            break;
         }
         case INPUT_OPT_NORMALIZATION_WEIGHT_FILE:
         {
            setOption(OPT_normalization_file, optarg);
            break;
         }
#if HAVE_R
         case INPUT_OPT_PREDICTION_INTERVAL_VALUE:
         {
            setOption(OPT_interval_level, optarg);
            break;
         }
#endif
         case INPUT_OPT_SEQUENCE_LENGTH:
         {
            setOption(OPT_sequence_length, optarg);
            break;
         }
         case INPUT_OPT_SURVIVING_BENCHMARKS:
         {
            setOption(OPT_surviving_benchmarks, optarg);
            break;
         }
         default:
         {
            bool exit_success = false;
            bool res = ManageDefaultOptions(next_option, optarg, exit_success);
            if(exit_success)
            {
               return EXIT_SUCCESS;
            }
            else if(res)
            {
               std::cerr << optarg << std::endl;
               return PARAMETER_NOTPARSED;
            }
         }
      }
   }

   THROW_ASSERT(argc >= optind + 2, "Wrong number of files");
   while(optind + 1 < argc)
   {
      std::string input_file;
      if(isOption(OPT_input_file))
      {
         input_file = getOption<std::string>(OPT_input_file) + STR_CST_string_separator;
      }
      setOption(OPT_input_file, input_file + GetPath(argv[optind]));
      optind++;
   }
   setOption(OPT_output_file, GetPath(argv[optind]));

   CheckParameters();

   return PARAMETER_PARSED;
}

void SpiderParameter::SetDefaults()
{
   setOption(OPT_accuracy, 0);
#if HAVE_TECHNOLOGY_BUILT
   setOption(OPT_component_name, "all");
#endif
   setOption(OPT_cross_validation, 1);
   /// Debugging level
#if RELEASE
   setOption(OPT_debug_level, DEBUG_LEVEL_NONE);
#else
   setOption(OPT_debug_level, DEBUG_LEVEL_MINIMUM);
#endif
   setOption(OPT_dump_genlib, false);
#if HAVE_REGRESSORS_BUILT
   setOption(OPT_interval_level, NUM_CST_default_prediction_interval_value);
#endif
   setOption(OPT_max_bound, INFINITE_LONG_DOUBLE);
   setOption(OPT_min_bound, 0);
#if HAVE_R
   setOption(OPT_maximum_error, 0.2);
   setOption(OPT_minimum_significance, 0);
#endif
   setOption(OPT_normalization_sequences, "S");
   setOption(OPT_output_directory, ".");
   /// Output level
#if RELEASE
   setOption(OPT_output_level, OUTPUT_LEVEL_NONE);
#else
   setOption(OPT_output_level, OUTPUT_LEVEL_MINIMUM);
#endif
   setOption(OPT_precision, 3);
   setOption(OPT_processing_element_type, "ARM");
   setOption(OPT_sequence_length, 2);
   setOption(OPT_surviving_benchmarks, NUM_CST_surviving_benchmarks);
}

void SpiderParameter::CheckParameters()
{
   Parameter::CheckParameters();
   if(!isOption(OPT_input_format))
   {
      Parameters_FileFormat input_format = Parameters_FileFormat::FF_UNKNOWN, temp = Parameters_FileFormat::FF_UNKNOWN;
      const auto input_files = getOption<const CustomSet<std::string>>(OPT_input_file);
      for(const auto& input_file : input_files)
      {
         temp = GetFileFormat(input_file, true);
         switch(temp)
         {
#if HAVE_REGRESSORS_BUILT
            case(Parameters_FileFormat::FF_XML_AGG):
               setOption(OPT_aggregated_features, input_file);
               break;
#endif
            case(Parameters_FileFormat::FF_XML_TEX_TABLE):
               setOption(OPT_latex_format_file, input_file);
               break;
            case(Parameters_FileFormat::FF_XML_SKIP_ROW):
               setOption(OPT_skip_rows, input_file);
               break;
#if HAVE_FROM_LIBERTY
            case(Parameters_FileFormat::FF_XML_CELLS):
            case(Parameters_FileFormat::FF_LIB):
               setOption(OPT_input_liberty_library_file, input_file);
               input_format = temp;
               break;
#endif
#if HAVE_TECHNOLOGY_BUILT
            case(Parameters_FileFormat::FF_XML_TARGET):
            {
               if(input_format != Parameters_FileFormat::FF_XML_TEC)
               {
                  input_format = temp;
               }
               break;
            }
#endif
            case Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP:
            {
               setOption(OPT_experimental_setup_file, input_file);
               break;
            }
#if HAVE_FROM_AADL_ASN_BUILT
            case(Parameters_FileFormat::FF_AADL):
            case(Parameters_FileFormat::FF_ASN):
#endif
#if HAVE_FROM_C_BUILT
            case(Parameters_FileFormat::FF_C):
            case(Parameters_FileFormat::FF_OBJECTIVEC):
            case(Parameters_FileFormat::FF_OBJECTIVECPP):
            case(Parameters_FileFormat::FF_CPP):
            case(Parameters_FileFormat::FF_FORTRAN):
            case(Parameters_FileFormat::FF_LLVM):
#endif
            case(Parameters_FileFormat::FF_CSV):
#if HAVE_FROM_CSV_BUILT
            case(Parameters_FileFormat::FF_CSV_RTL):
            case(Parameters_FileFormat::FF_CSV_TRE):
#endif
#if HAVE_EXPERIMENTAL
            case(Parameters_FileFormat::FF_LOG):
            case(Parameters_FileFormat::FF_PA):
#endif
#if HAVE_FROM_C_BUILT
            case(Parameters_FileFormat::FF_RAW):
#endif
            case(Parameters_FileFormat::FF_TEX):
            case(Parameters_FileFormat::FF_VERILOG):
            case(Parameters_FileFormat::FF_VHDL):
            case(Parameters_FileFormat::FF_XML):
#if HAVE_FROM_ARCH_BUILT
            case(Parameters_FileFormat::FF_XML_ARCHITECTURE):
#endif
            case(Parameters_FileFormat::FF_XML_BAMBU_RESULTS):
#if HAVE_HLS_BUILT
            case(Parameters_FileFormat::FF_XML_CON):
#endif
            case(Parameters_FileFormat::FF_XML_IP_XACT_COMPONENT):
            case(Parameters_FileFormat::FF_XML_IP_XACT_CONFIG):
            case(Parameters_FileFormat::FF_XML_IP_XACT_DESIGN):
            case(Parameters_FileFormat::FF_XML_IP_XACT_GENERATOR):
#if HAVE_SOURCE_CODE_STATISTICS_XML
            case(Parameters_FileFormat::FF_XML_STAT):
#endif
            case(Parameters_FileFormat::FF_XML_SYM_SIM):
#if HAVE_TECHNOLOGY_BUILT
            case(Parameters_FileFormat::FF_XML_TEC):
#endif
            case(Parameters_FileFormat::FF_TGFF):
            case(Parameters_FileFormat::FF_XML_WGT_GM):
            case(Parameters_FileFormat::FF_XML_WGT_SYM):
            case(Parameters_FileFormat::FF_UNKNOWN):
            default:
               input_format = temp;
               break;
         }
      }
      setOption(OPT_input_format, static_cast<int>(input_format));
   }
   else
   {
      Parameters_FileFormat input_format = GetFileFormat(getOption<std::string>(OPT_input_format), true);
      setOption(OPT_input_format, static_cast<int>(input_format));
   }
   if(!isOption(OPT_output_format))
   {
      Parameters_FileFormat output_format = GetFileFormat(getOption<std::string>(OPT_output_file), false);
      setOption(OPT_output_format, static_cast<int>(output_format));
   }
   else
   {
      Parameters_FileFormat output_format = GetFileFormat(getOption<std::string>(OPT_output_format), false);
      setOption(OPT_output_format, static_cast<int>(output_format));
   }
#if HAVE_FROM_LIBERTY
   if(getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_XML_CELLS or getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LIB)
   {
      const auto input_files = getOption<const CustomSet<std::string>>(OPT_input_file);
      if(not(input_files.size() == 1 or (input_files.size() == 2 and isOption(OPT_aggregated_features))))
         THROW_ERROR("Only a liberty file required");
   }
#endif
#if HAVE_TECHNOLOGY_BUILT
   for(const auto& input_file : getOption<const CustomSet<std::string>>(OPT_input_file))
   {
      if(GetFileFormat(input_file, true) == Parameters_FileFormat::FF_XML_TEC)
      {
         setOption(OPT_technology_file, (isOption(OPT_technology_file) ? (getOption<std::string>(OPT_technology_file) + STR_CST_string_separator) : "") + input_file);
      }
      else if(GetFileFormat(input_file, true) == Parameters_FileFormat::FF_XML_TARGET)
      {
         if(isOption(OPT_target_device_file) and getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_XML_TEC)
         {
            THROW_ERROR("Multiple target device file: " + input_file + " " + getOption<std::string>(OPT_target_device_file));
         }
         setOption(OPT_target_device_file, (isOption(OPT_target_device_file) ? (getOption<std::string>(OPT_target_device_file) + STR_CST_string_separator) : "") + input_file);
      }
   }
   if(getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_XML_TEC)
   {
      if(not isOption(OPT_target_device_file))
      {
         THROW_ERROR("Target device file not specified");
      }
   }
#endif

#ifndef NDEBUG
   Parameters_FileFormat input_format = getOption<Parameters_FileFormat>(OPT_input_format);
   Parameters_FileFormat output_format = getOption<Parameters_FileFormat>(OPT_output_format);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input format is " + STR(static_cast<int>(input_format)));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Output format is " + STR(static_cast<int>(output_format)));
#endif
}
