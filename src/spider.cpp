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
 * @file spider.cpp
 * @brief Parser for deep profiling information
 *
 * @author Daniele Loiacono <loiacono@elet.polimi.it>
 *
 */

/// Autoheader include
#include "config_HAVE_CMOS_BUILT.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_ARCH_BUILT.hpp"
#include "config_HAVE_FROM_CSV_BUILT.hpp"
#include "config_HAVE_FROM_LIBERTY.hpp"
#include "config_HAVE_FROM_PROFILING_ANALYIS_BUILT.hpp"
#include "config_HAVE_R.hpp"

///. includes
#include "SpiderParameter.hpp"

#if HAVE_REGRESSORS_BUILT
/// algorithms/regressors include
#include "cross_validation.hpp"
#if HAVE_R
#include "linear_regression.hpp"
#endif
#include "regressor.hpp"

/// algorithms/regressors/preprocessing includes
#include "cell_selection.hpp"
#if HAVE_FROM_LIBERTY
#include "cell_area_preprocessing.hpp"
#endif
#include "performance_estimation_preprocessing.hpp"
#if HAVE_R
#include "significance_preprocessing.hpp"
#endif
#endif

/// boost include
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>

/// constants include
#if HAVE_R
#include "regressors_constants.hpp"
#endif
#if HAVE_FROM_LIBERTY
#include "physical_library_models_constants.hpp"
#endif

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/backend/ToDataFile includes
#if HAVE_TECHNOLOGY_BUILT
#include "to_data_file_step.hpp"
#include "to_data_file_step_factory.hpp"
#endif
#include "translator.hpp"
#if HAVE_EXPERIMENTAL
#include "xml_generator.hpp"
#endif

#if HAVE_TECHNOLOGY_BUILT
/// design_flows/technology include
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#endif

/// parser/data_xml include
#include "data_xml_parser.hpp"

#if HAVE_FROM_CSV_BUILT
/// parser/csv include
#include "parse_csv.hpp"
#endif

#if HAVE_FROM_PROFILING_ANALYIS_BUILT
/// parser/rapid_miner include
#include "parse_rapid_miner.hpp"
#endif

#if HAVE_RTL_BUILT
/// RTL include
#include "rtl_node.hpp"
#endif

/// STD include
#include <fstream>
#include <iosfwd>
#include <string>

#include "custom_map.hpp"
#include "custom_set.hpp"

#if HAVE_TECHNOLOGY_BUILT
/// technology include
#include "parse_technology.hpp"
#include "target_manager.hpp"
#include "target_technology.hpp"
#include "technology_manager.hpp"
#endif

#if HAVE_FROM_LIBERTY
/// technology/physical_library/models include
#include "features_extractor.hpp"
#endif

/// utility includes
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "exceptions.hpp"
#include "string_manipulation.hpp"

#define MAX_LENGTH 10000

#define INIT(x, y) x.push_back(std::string(y))

int main(int argc, char* argv[])
{
   ParameterRef parameters;
   try
   {
      // ---------- Parameter parsing ------------ //
      parameters = ParameterRef(new SpiderParameter(argv[0], argc, argv));
      switch(parameters->Exec())
      {
         case PARAMETER_NOTPARSED:
         {
            exit_code = PARAMETER_NOTPARSED;
            throw "Bad Parameters format";
         }
         case EXIT_SUCCESS:
         {
            if(not(parameters->getOption<bool>(OPT_no_clean)))
            {
               boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
            }
            return EXIT_SUCCESS;
         }
         case PARAMETER_PARSED:
         {
            exit_code = EXIT_FAILURE;
            break;
         }
         default:
         {
            if(not(parameters->getOption<bool>(OPT_no_clean)))
            {
               boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
            }
            THROW_ERROR("Bad Parameters parsing");
         }
      }
#if !defined(NDEBUG) || HAVE_EXPERIMENTAL
      auto debug_level = parameters->getOption<int>(OPT_debug_level);
#endif
      Parameters_FileFormat input_format = parameters->getOption<Parameters_FileFormat>(OPT_input_format);
      Parameters_FileFormat output_format = parameters->getOption<Parameters_FileFormat>(OPT_output_format);
      switch(input_format)
      {
#if HAVE_FROM_CSV_BUILT
         case(Parameters_FileFormat::FF_CSV):
         {
            TranslatorConstRef tr(new Translator(parameters));
            std::string csv_file;
            const auto input_files = parameters->getOption<const CustomSet<std::string>>(OPT_input_file);
            for(const auto input_file : input_files)
            {
               if(parameters->GetFileFormat(input_file, false) == Parameters_FileFormat::FF_CSV)
               {
                  csv_file = input_file;
                  break;
               }
            }
            std::map<std::string, CustomMap<std::string, std::string>> results;
            ParseCsvFile(results, csv_file, parameters);
            switch(output_format)
            {
               case(Parameters_FileFormat::FF_TEX):
               {
                  /// Read data
                  tr->write_to_latex(results, Parameters_FileFormat::FF_CSV, parameters->getOption<std::string>(OPT_output_file));
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
               case(Parameters_FileFormat::FF_CSV_RTL):
               case(Parameters_FileFormat::FF_CSV_TRE):
#if HAVE_FROM_LIBERTY
               case(Parameters_FileFormat::FF_LIB):
#endif
               case(Parameters_FileFormat::FF_LOG):
               case(Parameters_FileFormat::FF_PA):
#if HAVE_FROM_C_BUILT
               case(Parameters_FileFormat::FF_RAW):
#endif
               case(Parameters_FileFormat::FF_TGFF):
               case(Parameters_FileFormat::FF_VERILOG):
               case(Parameters_FileFormat::FF_VHDL):
               case(Parameters_FileFormat::FF_XML):
               case(Parameters_FileFormat::FF_XML_AGG):
#if HAVE_FROM_ARCH_BUILT
               case(Parameters_FileFormat::FF_XML_ARCHITECTURE):
#endif
               case(Parameters_FileFormat::FF_XML_BAMBU_RESULTS):
#if HAVE_FROM_LIBERTY
               case(Parameters_FileFormat::FF_XML_CELLS):
#endif
#if HAVE_HLS_BUILT
               case(Parameters_FileFormat::FF_XML_CON):
#endif
               case Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP:
               case(Parameters_FileFormat::FF_XML_IP_XACT_COMPONENT):
               case(Parameters_FileFormat::FF_XML_IP_XACT_CONFIG):
               case(Parameters_FileFormat::FF_XML_IP_XACT_DESIGN):
               case(Parameters_FileFormat::FF_XML_IP_XACT_GENERATOR):
               case(Parameters_FileFormat::FF_XML_SKIP_ROW):
               case(Parameters_FileFormat::FF_XML_STAT):
#if HAVE_TECHNOLOGY_BUILT
               case(Parameters_FileFormat::FF_XML_TARGET):
               case(Parameters_FileFormat::FF_XML_TEC):
#endif
               case(Parameters_FileFormat::FF_XML_SYM_SIM):
               case(Parameters_FileFormat::FF_XML_TEX_TABLE):
               case(Parameters_FileFormat::FF_XML_WGT_GM):
               case(Parameters_FileFormat::FF_XML_WGT_SYM):
               case(Parameters_FileFormat::FF_UNKNOWN):
               default:
                  THROW_ERROR("Not support combination input file - output file types");
            }
            break;
         }
#endif
         case(Parameters_FileFormat::FF_XML):
         {
            switch(output_format)
            {
               case(Parameters_FileFormat::FF_TEX):
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input: XML - Output: TEX");
                  const auto input_files = parameters->getOption<const CustomSet<std::string>>(OPT_input_file);
                  std::map<std::string, CustomMap<std::string, std::string>> results;
                  const DataXmlParserConstRef data_xml_parser(new DataXmlParser(parameters));
                  data_xml_parser->Parse(input_files, results);
                  TranslatorConstRef tr(new Translator(parameters));
                  tr->write_to_latex(results, Parameters_FileFormat::FF_XML, parameters->getOption<std::string>(OPT_output_file));
                  break;
               }
#if HAVE_EXPERIMENTAL
               case(Parameters_FileFormat::FF_XML):
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input: XML - Output: XML");
                  CustomUnorderedMap<std::string, long double> results;
                  std::map<enum rtl_kind, std::map<enum mode_kind, long double>> output;
                  const auto input_files = parameters->getOption<const CustomSet<std::string>>(OPT_input_file);
                  if(input_files.size() != 1)
                     THROW_ERROR("Only one rapid miner output can be analyzed at a time");
                  parse_rapid_miner(*(input_files.begin()), results, debug_level);
                  TranslatorConstRef tr(new Translator(parameters));
                  tr->Translate(results, output);
                  tr->write_to_xml(output, parameters->getOption<std::string>(OPT_output_file));
                  break;
               }
#else
               case(Parameters_FileFormat::FF_XML):
#endif
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
#if HAVE_FROM_LIBERTY
               case(Parameters_FileFormat::FF_LIB):
#endif
#if HAVE_EXPERIMENTAL
               case(Parameters_FileFormat::FF_LOG):
               case(Parameters_FileFormat::FF_PA):
#endif
#if HAVE_FROM_C_BUILT
               case(Parameters_FileFormat::FF_RAW):
#endif
               case(Parameters_FileFormat::FF_TGFF):
               case(Parameters_FileFormat::FF_VERILOG):
               case(Parameters_FileFormat::FF_VHDL):
#if HAVE_REGRESSORS_BUILT
               case(Parameters_FileFormat::FF_XML_AGG):
#endif
#if HAVE_FROM_ARCH_BUILT
               case(Parameters_FileFormat::FF_XML_ARCHITECTURE):
#endif
               case(Parameters_FileFormat::FF_XML_BAMBU_RESULTS):
#if HAVE_FROM_LIBERTY
               case(Parameters_FileFormat::FF_XML_CELLS):
#endif
#if HAVE_HLS_BUILT
               case(Parameters_FileFormat::FF_XML_CON):
#endif
               case Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP:
               case(Parameters_FileFormat::FF_XML_IP_XACT_COMPONENT):
               case(Parameters_FileFormat::FF_XML_IP_XACT_CONFIG):
               case(Parameters_FileFormat::FF_XML_IP_XACT_DESIGN):
               case(Parameters_FileFormat::FF_XML_IP_XACT_GENERATOR):
               case(Parameters_FileFormat::FF_XML_SKIP_ROW):
#if HAVE_SOURCE_CODE_STATISTICS_XML
               case(Parameters_FileFormat::FF_XML_STAT):
#endif
               case(Parameters_FileFormat::FF_XML_SYM_SIM):
#if HAVE_TECHNOLOGY_BUILT
               case(Parameters_FileFormat::FF_XML_TARGET):
               case(Parameters_FileFormat::FF_XML_TEC):
#endif
               case(Parameters_FileFormat::FF_XML_TEX_TABLE):
               case(Parameters_FileFormat::FF_XML_WGT_GM):
               case(Parameters_FileFormat::FF_XML_WGT_SYM):
               case(Parameters_FileFormat::FF_UNKNOWN):
               default:
                  THROW_ERROR("Not support combination input file - output file types");
            }
            break;
         }
#if HAVE_FROM_LIBERTY && HAVE_R && HAVE_CMOS_BUILT
         case(Parameters_FileFormat::FF_LIB):
         case(Parameters_FileFormat::FF_XML_CELLS):
         {
            if(output_format != Parameters_FileFormat::FF_XML)
               THROW_ERROR("Not supported combination input file - output file types");
            int output_level = parameters->getOption<int>(OPT_output_level);
            if(output_level >= OUTPUT_LEVEL_MINIMUM)
            {
               if(output_level >= OUTPUT_LEVEL_VERBOSE)
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*                        Computing cell area and time models                  *");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
               }
               else
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, " ======================= Computing cell area and time models ================= ");
               }
            }
            const technology_managerRef TM = technology_managerRef(new technology_manager(parameters));

            /// read the technology library
            read_technology_library(TM, parameters, target_device::create_device(target_device::IC, parameters, TM));

            /// Generate features
            std::map<std::string, std::map<std::string, long double>> data, preprocessed_data;
            FeaturesExtractorRef features_extractor(new FeaturesExtractor(parameters));
            CustomOrderedSet<std::string> column_names;
            features_extractor->ExtractAreaFeatures(TM, data, column_names);
            /*
               const CellAreaPreprocessingConstRef preprocessing(new CellAreaPreprocessing(parameters));
               preprocessing->Exec(data, preprocessed_data, column_names);
               data = preprocessed_data;*/
            const PreprocessingConstRef cell_selection(new CellSelection(parameters));
            cell_selection->Exec(data, preprocessed_data, column_names, STR_CST_physical_library_models_area);
            const RegressorConstRef linear_regression = RegressorConstRef(new LinearRegression(parameters));
            if(parameters->getOption<int>(OPT_cross_validation) > 1)
            {
               const RegressorConstRef cross_validation_regression(new CrossValidation(linear_regression, parameters));
               const RegressionResultsRef results = cross_validation_regression->Exec(column_names, STR_CST_physical_library_models_area, preprocessed_data);
               if(output_level >= OUTPUT_LEVEL_MINIMUM)
               {
                  if(output_level >= OUTPUT_LEVEL_VERBOSE)
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*                             Cross validation results                        *");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
                  }
                  else
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, " ============================ Cross validation results ======================= ");
                  }
                  results->Print(std::cerr);
               }
            }
            else
            {
               const RegressionResultsRef results = linear_regression->Exec(column_names, STR_CST_physical_library_models_area, preprocessed_data);
               if(output_level >= OUTPUT_LEVEL_MINIMUM)
               {
                  if(output_level >= OUTPUT_LEVEL_VERBOSE)
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*                                Regression results                           *");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
                  }
                  else
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
                     INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, " =============================== Regression results ========================== ");
                  }
                  results->Print(std::cerr);
               }
            }
            break;
         }
#endif
         case(Parameters_FileFormat::FF_XML_BAMBU_RESULTS):
#if HAVE_SOURCE_CODE_STATISTICS_XML
         case(Parameters_FileFormat::FF_XML_STAT):
#endif
         {
            const auto input_files = parameters->getOption<const CustomSet<std::string>>(OPT_input_file);
            std::map<std::string, CustomMap<std::string, std::string>> results;
            const DataXmlParserConstRef data_xml_parser(new DataXmlParser(parameters));
            data_xml_parser->Parse(input_files, results);
            TranslatorConstRef tr(new Translator(parameters));
            switch(output_format)
            {
               case(Parameters_FileFormat::FF_TEX):
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input: xml - Output: tex");
                  tr->write_to_latex(results, input_format, parameters->getOption<std::string>(OPT_output_file));
                  break;
               }
               case(Parameters_FileFormat::FF_CSV):
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input: XML - Output: CSV");
                  tr->write_to_csv(results, parameters->getOption<std::string>(OPT_output_file));
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
#if HAVE_FROM_CSV_BUILT
               case(Parameters_FileFormat::FF_CSV_RTL):
               case(Parameters_FileFormat::FF_CSV_TRE):
#endif
#if HAVE_FROM_LIBERTY
               case(Parameters_FileFormat::FF_LIB):
#endif
#if HAVE_EXPERIMENTAL
               case(Parameters_FileFormat::FF_LOG):
               case(Parameters_FileFormat::FF_PA):
#endif
#if HAVE_FROM_C_BUILT
               case(Parameters_FileFormat::FF_RAW):
#endif
               case(Parameters_FileFormat::FF_TGFF):
               case(Parameters_FileFormat::FF_VERILOG):
               case(Parameters_FileFormat::FF_VHDL):
               case(Parameters_FileFormat::FF_XML):
#if HAVE_REGRESSORS_BUILT
               case(Parameters_FileFormat::FF_XML_AGG):
#endif
#if HAVE_FROM_ARCH_BUILT
               case(Parameters_FileFormat::FF_XML_ARCHITECTURE):
#endif
               case(Parameters_FileFormat::FF_XML_BAMBU_RESULTS):
#if HAVE_FROM_LIBERTY
               case(Parameters_FileFormat::FF_XML_CELLS):
#endif
#if HAVE_HLS_BUILT
               case(Parameters_FileFormat::FF_XML_CON):
#endif
               case Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP:
               case(Parameters_FileFormat::FF_XML_IP_XACT_COMPONENT):
               case(Parameters_FileFormat::FF_XML_IP_XACT_CONFIG):
               case(Parameters_FileFormat::FF_XML_IP_XACT_DESIGN):
               case(Parameters_FileFormat::FF_XML_IP_XACT_GENERATOR):
               case(Parameters_FileFormat::FF_XML_SKIP_ROW):
#if HAVE_SOURCE_CODE_STATISTICS_XML
               case(Parameters_FileFormat::FF_XML_STAT):
#endif
               case(Parameters_FileFormat::FF_XML_SYM_SIM):
#if HAVE_TECHNOLOGY_BUILT
               case(Parameters_FileFormat::FF_XML_TARGET):
               case(Parameters_FileFormat::FF_XML_TEC):
#endif
               case(Parameters_FileFormat::FF_XML_TEX_TABLE):
               case(Parameters_FileFormat::FF_XML_WGT_GM):
               case(Parameters_FileFormat::FF_XML_WGT_SYM):
               case(Parameters_FileFormat::FF_UNKNOWN):
               default:
               {
                  THROW_ERROR("Not supported combination input file - output file types");
               }
            }
            break;
         }
#if HAVE_R
         case(Parameters_FileFormat::FF_XML_SYM_SIM):
         {
            int output_level = parameters->getOption<int>(OPT_output_level);
            if(output_level >= OUTPUT_LEVEL_MINIMUM)
            {
               if(output_level >= OUTPUT_LEVEL_VERBOSE)
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*       Computing probability distribution of operation execution time        *");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
               }
               else
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, " ====== Computing probability distribution of operation execution time =======");
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Input xml is Symbolic Distribution: going to estimate assembly variances");
            if(output_format != Parameters_FileFormat::FF_XML)
               THROW_ERROR("Not supported combination input file - output file types");
            const std::string sequences = parameters->getOption<std::string>(OPT_normalization_sequences);
            std::vector<std::string> sequences_splitted = SplitString(sequences, "-");
            for(size_t sequence_number = 0; sequence_number < sequences_splitted.size(); sequence_number++)
            {
               const auto input_files = parameters->getOption<const CustomSet<std::string>>(OPT_input_file);
               const PreprocessingConstRef performance_estimation_preprocessing(new PerformanceEstimationPreprocessing(sequences_splitted[sequence_number], parameters));
               std::map<std::string, std::map<std::string, long double>> data, filtered_data, preprocessed_data;
               CustomOrderedSet<std::string> column_names;
               performance_estimation_preprocessing->ReadData(input_files, data, column_names);
               const PreprocessingConstRef cell_selection(new CellSelection(parameters));
               cell_selection->Exec(data, filtered_data, column_names, STR_CST_cycles);
               performance_estimation_preprocessing->Exec(filtered_data, preprocessed_data, column_names, STR_CST_cycles);

               long double error = INFINITE_LONG_DOUBLE;
               while(true)
               {
                  RegressorConstRef regression = RegressorConstRef(new LinearRegression(parameters));
                  if(parameters->getOption<int>(OPT_cross_validation) > 1)
                  {
                     regression = RegressorConstRef(new CrossValidation(regression, parameters));
                  }
                  const RegressionResultsRef results = regression->Exec(column_names, STR_CST_cycles, preprocessed_data);
                  error = results->average_training_error;
                  if(error < parameters->getOption<long double>(OPT_maximum_error))
                  {
                     break;
                  }
                  long double max_error = 0.0;
                  std::string benchmark_to_be_removed;
                  const CustomUnorderedMap<std::string, long double>& training_errors = results->training_errors;
                  CustomUnorderedMap<std::string, long double>::const_iterator training_error, training_error_end = training_errors.end();
                  for(training_error = training_errors.begin(); training_error != training_error_end; ++training_error)
                  {
                     long double current_training_error = training_error->second;
                     if(current_training_error < 0.0)
                     {
                        current_training_error = -current_training_error;
                     }
                     if(current_training_error > max_error)
                     {
                        benchmark_to_be_removed = training_error->first;
                        max_error = current_training_error;
                     }
                  }
                  THROW_ASSERT(preprocessed_data.find(benchmark_to_be_removed) != preprocessed_data.end(), benchmark_to_be_removed + " is not in the map");
                  preprocessed_data.erase(preprocessed_data.find(benchmark_to_be_removed));
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, debug_level, "Removed " + benchmark_to_be_removed + " with error " + boost::lexical_cast<std::string>(max_error) + ": current average error is " + boost::lexical_cast<std::string>(error));
               }
               while(true)
               {
                  RegressorConstRef regression = RegressorConstRef(new LinearRegression(parameters));
                  if(parameters->getOption<int>(OPT_cross_validation) > 1)
                  {
                     regression = RegressorConstRef(new CrossValidation(regression, parameters));
                  }
                  const RegressionResultsRef results = regression->Exec(column_names, STR_CST_cycles, preprocessed_data);
                  results->Print(std::cerr);
                  long double current_minimum_significance = GetPointer<LinearRegressionResults>(results)->regressor_minimum_significance;
                  if(current_minimum_significance >= parameters->getOption<long double>(OPT_minimum_significance))
                  {
                     XMLGeneratorConstRef generator(new XMLGenerator(parameters));
                     generator->GenerateRtlSequenceWeightModel(results->model, parameters->getOption<std::string>(OPT_output_file));
                     break;
                  }
                  data = preprocessed_data;
                  const PreprocessingConstRef significance_preprocessing(
                      new SignificancePreprocessing(GetPointer<LinearRegressionResults>(results)->regressor_significances, GetPointer<LinearRegressionResults>(results)->regressor_minimum_significance, parameters));
                  significance_preprocessing->Exec(data, preprocessed_data, column_names, STR_CST_cycles);
               }
            }
            break;
         }
#endif
#if HAVE_TECHNOLOGY_BUILT
         case(Parameters_FileFormat::FF_XML_TARGET):
         {
            auto output_level = parameters->getOption<int>(OPT_output_level);
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Merging characterizations");
            // Technology library manager
            technology_managerRef TM = technology_managerRef(new technology_manager(parameters));

            /// creating the datastructure representing the target device
            target_deviceRef device = target_device::create_device(TargetDevice_Type::FPGA, parameters, TM);
            target_managerRef target = target_managerRef(new target_manager(parameters, TM, device));

            const DesignFlowManagerRef design_flow_manager(new DesignFlowManager(parameters));
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager->CGetDesignFlowGraph();

            const DesignFlowStepFactoryConstRef technology_flow_step_factory(new TechnologyFlowStepFactory(TM, device, design_flow_manager, parameters));
            design_flow_manager->RegisterFactory(technology_flow_step_factory);

            const std::string load_technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
            const vertex load_technology_flow_step = design_flow_manager->GetDesignFlowStep(load_technology_flow_signature);
            const DesignFlowStepRef load_technology_design_flow_step = load_technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(load_technology_flow_step)->design_flow_step :
                                                                                                   GetPointer<const TechnologyFlowStepFactory>(technology_flow_step_factory)->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
            design_flow_manager->AddStep(load_technology_design_flow_step);

            const std::string fix_technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::FIX_CHARACTERIZATION);
            const vertex fix_technology_flow_step = design_flow_manager->GetDesignFlowStep(fix_technology_flow_signature);
            const DesignFlowStepRef fix_technology_design_flow_step = fix_technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(fix_technology_flow_step)->design_flow_step :
                                                                                                 GetPointer<const TechnologyFlowStepFactory>(technology_flow_step_factory)->CreateTechnologyFlowStep(TechnologyFlowStep_Type::FIX_CHARACTERIZATION);
            design_flow_manager->AddStep(fix_technology_design_flow_step);

            const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::WRITE_TECHNOLOGY);
            const vertex technology_flow_step = design_flow_manager->GetDesignFlowStep(technology_flow_signature);
            const DesignFlowStepRef technology_design_flow_step = technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step :
                                                                                         GetPointer<const TechnologyFlowStepFactory>(technology_flow_step_factory)->CreateTechnologyFlowStep(TechnologyFlowStep_Type::WRITE_TECHNOLOGY);
            design_flow_manager->AddStep(technology_design_flow_step);
            design_flow_manager->Exec();
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--Merged characterizations");
            break;
         }
         case(Parameters_FileFormat::FF_XML_TEC):
         {
#if HAVE_CIRCUIT_BUILT
            // Technology library manager
            technology_managerRef TM = technology_managerRef(new technology_manager(parameters));

            /// creating the datastructure representing the target device
            target_deviceRef device = target_device::create_device(TargetDevice_Type::FPGA, parameters, TM);
            target_managerRef target = target_managerRef(new target_manager(parameters, TM, device));

            const DesignFlowManagerRef design_flow_manager(new DesignFlowManager(parameters));
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager->CGetDesignFlowGraph();

            const DesignFlowStepFactoryConstRef technology_flow_step_factory(new TechnologyFlowStepFactory(TM, device, design_flow_manager, parameters));
            design_flow_manager->RegisterFactory(technology_flow_step_factory);

            const DesignFlowStepFactoryConstRef to_data_file_step_factory(new ToDataFileStepFactory(target, design_flow_manager, parameters));
            design_flow_manager->RegisterFactory(to_data_file_step_factory);

            const std::string to_data_file_step_signature = ToDataFileStep::ComputeSignature(ToDataFileStep_Type::GENERATE_FU_LIST);
            const vertex to_data_file_vertex = design_flow_manager->GetDesignFlowStep(to_data_file_step_signature);
            const DesignFlowStepRef to_data_file_step =
                to_data_file_vertex ? design_flow_graph->CGetDesignFlowStepInfo(to_data_file_vertex)->design_flow_step : GetPointer<const ToDataFileStepFactory>(to_data_file_step_factory)->CreateStep(to_data_file_step_signature);
            design_flow_manager->AddStep(to_data_file_step);
            design_flow_manager->Exec();
#else
            THROW_ERROR("Input technology file not supported.");
#endif

            break;
         }
#endif
#if HAVE_EXPERIMENTAL
         case(Parameters_FileFormat::FF_XML_WGT_SYM):
         {
            switch(output_format)
            {
               case(Parameters_FileFormat::FF_XML):
               {
                  int output_level = parameters->getOption<int>(OPT_output_level);
                  if(output_level >= OUTPUT_LEVEL_MINIMUM)
                  {
                     if(output_level >= OUTPUT_LEVEL_VERBOSE)
                     {
                        INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                        INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                        INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
                        INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
                        INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*                   Genereting symbolic sequence weight model                 *");
                        INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "*******************************************************************************");
                     }
                     else
                     {
                        INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
                        INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, " ================== Generating symbolic sequence weight models =============== ");
                     }
                  }
                  XMLGeneratorConstRef generator(new XMLGenerator(parameters));
                  const auto input_files = parameters->getOption<const CustomSet<std::string>>(OPT_input_file);
                  if(input_files.size() > 1)
                     THROW_ERROR("Symbolic sequence weight models can be built only starting from a single file");
                  const std::string output_file = parameters->getOption<std::string>(OPT_output_file);
                  generator->GenerateSymbolicSequenceWeightModel(*(input_files.begin()), output_file);
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
               case(Parameters_FileFormat::FF_CSV_RTL):
               case(Parameters_FileFormat::FF_CSV_TRE):
#if HAVE_FROM_LIBERTY
               case(Parameters_FileFormat::FF_LIB):
#endif
               case(Parameters_FileFormat::FF_LOG):
               case(Parameters_FileFormat::FF_PA):
#if HAVE_FROM_C_BUILT
               case(Parameters_FileFormat::FF_RAW):
#endif
               case(Parameters_FileFormat::FF_TEX):
               case(Parameters_FileFormat::FF_VERILOG):
               case(Parameters_FileFormat::FF_VHDL):
               case(Parameters_FileFormat::FF_XML_AGG):
#if HAVE_FROM_ARCH_BUILT
               case(Parameters_FileFormat::FF_XML_ARCHITECTURE):
#endif
               case(Parameters_FileFormat::FF_XML_BAMBU_RESULTS):
#if HAVE_FROM_LIBERTY
               case(Parameters_FileFormat::FF_XML_CELLS):
#endif
#if HAVE_HLS_BUILT
               case(Parameters_FileFormat::FF_XML_CON):
#endif
               case Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP:
               case(Parameters_FileFormat::FF_XML_IP_XACT_COMPONENT):
               case(Parameters_FileFormat::FF_XML_IP_XACT_CONFIG):
               case(Parameters_FileFormat::FF_XML_IP_XACT_DESIGN):
               case(Parameters_FileFormat::FF_XML_IP_XACT_GENERATOR):
               case(Parameters_FileFormat::FF_XML_SKIP_ROW):
               case(Parameters_FileFormat::FF_XML_STAT):
               case(Parameters_FileFormat::FF_XML_SYM_SIM):
#if HAVE_TECHNOLOGY_BUILT
               case(Parameters_FileFormat::FF_XML_TARGET):
               case(Parameters_FileFormat::FF_XML_TEC):
#endif
               case(Parameters_FileFormat::FF_XML_TEX_TABLE):
               case(Parameters_FileFormat::FF_TGFF):
               case(Parameters_FileFormat::FF_XML_WGT_GM):
               case(Parameters_FileFormat::FF_XML_WGT_SYM):
               case(Parameters_FileFormat::FF_UNKNOWN):
               default:
               {
                  THROW_ERROR("Not supported combination input file - output file types");
               }
            }
            break;
         }
#else
         case(Parameters_FileFormat::FF_XML_WGT_SYM):
#endif
#if !HAVE_FROM_CSV_BUILT
         case(Parameters_FileFormat::FF_CSV):
#endif
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
#if HAVE_FROM_CSV_BUILT
         case(Parameters_FileFormat::FF_CSV_RTL):
         case(Parameters_FileFormat::FF_CSV_TRE):
#endif
#if HAVE_FROM_LIBERTY
         case(Parameters_FileFormat::FF_LIB):
#endif
#if HAVE_EXPERIMENTAL
         case(Parameters_FileFormat::FF_LOG):
         case(Parameters_FileFormat::FF_PA):
#endif
#if HAVE_FROM_C_BUILT
         case(Parameters_FileFormat::FF_RAW):
#endif
         case(Parameters_FileFormat::FF_TEX):
         case(Parameters_FileFormat::FF_TGFF):
         case(Parameters_FileFormat::FF_VERILOG):
         case(Parameters_FileFormat::FF_VHDL):
#if HAVE_REGRESSORS_BUILT
         case(Parameters_FileFormat::FF_XML_AGG):
#endif
#if HAVE_FROM_ARCH_BUILT
         case(Parameters_FileFormat::FF_XML_ARCHITECTURE):
#endif
#if HAVE_FROM_LIBERTY
         case(Parameters_FileFormat::FF_XML_CELLS):
#endif
#if HAVE_HLS_BUILT
         case(Parameters_FileFormat::FF_XML_CON):
#endif
         case Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP:
         case(Parameters_FileFormat::FF_XML_IP_XACT_COMPONENT):
         case(Parameters_FileFormat::FF_XML_IP_XACT_CONFIG):
         case(Parameters_FileFormat::FF_XML_IP_XACT_DESIGN):
         case(Parameters_FileFormat::FF_XML_IP_XACT_GENERATOR):
         case(Parameters_FileFormat::FF_XML_SKIP_ROW):
#if !HAVE_R
         case(Parameters_FileFormat::FF_XML_SYM_SIM):
#endif
         case(Parameters_FileFormat::FF_XML_TEX_TABLE):
         case(Parameters_FileFormat::FF_XML_WGT_GM):
         case(Parameters_FileFormat::FF_UNKNOWN):
         default:
            THROW_ERROR("Not supported input file type " + STR(static_cast<int>(input_format)));
      }
   }
   catch(const char* str)
   {
      std::cerr << str << std::endl;
   }
   catch(const std::string& str)
   {
      std::cerr << str << std::endl;
   }
   catch(std::exception& inException)
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, 0, inException.what());
   }
   catch(...)
   {
      std::cerr << "Unknown error type" << std::endl;
   }
   if(not(parameters->getOption<bool>(OPT_no_clean)))
   {
      boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
   }

   return exit_code;
}
