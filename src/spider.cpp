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
 * @file spider.cpp
 * @brief Parser for deep profiling information
 *
 * @author Daniele Loiacono <loiacono@elet.polimi.it>
 *
 */

#include "SpiderParameter.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include <filesystem>
#if HAVE_TECHNOLOGY_BUILT
#include "to_data_file_step.hpp"
#include "to_data_file_step_factory.hpp"
#endif
#include "translator.hpp"
#if HAVE_TECHNOLOGY_BUILT
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#endif
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "data_xml_parser.hpp"
#include <fstream>
#include <iosfwd>
#include <string>
#if HAVE_TECHNOLOGY_BUILT
#include "generic_device.hpp"
#include "parse_technology.hpp"
#include "technology_manager.hpp"
#endif
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "exceptions.hpp"
#include "fileIO.hpp"
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
               std::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
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
               std::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
            }
            THROW_ERROR("Bad Parameters parsing");
         }
      }
#if !defined(NDEBUG)
      auto debug_level = parameters->getOption<int>(OPT_debug_level);
#endif
      Parameters_FileFormat input_format = parameters->getOption<Parameters_FileFormat>(OPT_input_format);
      Parameters_FileFormat output_format = parameters->getOption<Parameters_FileFormat>(OPT_output_format);
      switch(input_format)
      {
         case(Parameters_FileFormat::FF_XML):
         {
            switch(output_format)
            {
               case(Parameters_FileFormat::FF_TEX):
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Input: XML - Output: TEX");
                  const auto input_files = parameters->getOption<CustomSet<std::string>>(OPT_input_file);
                  std::map<std::string, CustomMap<std::string, std::string>> results;
                  const DataXmlParserConstRef data_xml_parser(new DataXmlParser(parameters));
                  data_xml_parser->Parse(input_files, results);
                  TranslatorConstRef tr(new Translator(parameters));
                  tr->write_to_latex(results, Parameters_FileFormat::FF_XML,
                                     parameters->getOption<std::string>(OPT_output_file));
                  break;
               }
               case(Parameters_FileFormat::FF_XML):
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
               case(Parameters_FileFormat::FF_LLVM_CPP):
#endif
               case(Parameters_FileFormat::FF_CSV):
#if HAVE_FROM_C_BUILT
               case(Parameters_FileFormat::FF_RAW):
#endif
               case(Parameters_FileFormat::FF_TGFF):
               case(Parameters_FileFormat::FF_VERILOG):
               case(Parameters_FileFormat::FF_VHDL):
               case(Parameters_FileFormat::FF_XML_BAMBU_RESULTS):
#if HAVE_HLS_BUILT
               case(Parameters_FileFormat::FF_XML_CON):
#endif
               case Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP:
               case(Parameters_FileFormat::FF_XML_SKIP_ROW):
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
         case(Parameters_FileFormat::FF_XML_BAMBU_RESULTS):
         {
            const auto input_files = parameters->getOption<CustomSet<std::string>>(OPT_input_file);
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
               case(Parameters_FileFormat::FF_LLVM_CPP):
#endif
#if HAVE_FROM_C_BUILT
               case(Parameters_FileFormat::FF_RAW):
#endif
               case(Parameters_FileFormat::FF_TGFF):
               case(Parameters_FileFormat::FF_VERILOG):
               case(Parameters_FileFormat::FF_VHDL):
               case(Parameters_FileFormat::FF_XML):
               case(Parameters_FileFormat::FF_XML_BAMBU_RESULTS):
#if HAVE_HLS_BUILT
               case(Parameters_FileFormat::FF_XML_CON):
#endif
               case Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP:
               case(Parameters_FileFormat::FF_XML_SKIP_ROW):
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
#if HAVE_TECHNOLOGY_BUILT
         case(Parameters_FileFormat::FF_XML_TARGET):
         {
            auto output_level = parameters->getOption<int>(OPT_output_level);
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Merging characterizations");
            // Technology library manager
            technology_managerRef TM = technology_managerRef(new technology_manager(parameters));

            /// creating the data-structure representing the target device
            generic_deviceRef device = generic_device::factory(parameters, TM);

            const DesignFlowManagerRef design_flow_manager(new DesignFlowManager(parameters));
            const auto design_flow_graph = design_flow_manager->CGetDesignFlowGraph();

            const DesignFlowStepFactoryConstRef technology_flow_step_factory(
                new TechnologyFlowStepFactory(TM, device, design_flow_manager, parameters));
            design_flow_manager->RegisterFactory(technology_flow_step_factory);

            const auto load_technology_flow_signature =
                TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
            const auto load_technology_flow_step =
                design_flow_manager->GetDesignFlowStep(load_technology_flow_signature);
            const DesignFlowStepRef load_technology_design_flow_step =
                load_technology_flow_step != DesignFlowGraph::null_vertex() ?
                    design_flow_graph->CGetNodeInfo(load_technology_flow_step)->design_flow_step :
                    GetPointer<const TechnologyFlowStepFactory>(technology_flow_step_factory)
                        ->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
            design_flow_manager->AddStep(load_technology_design_flow_step);

            const auto fix_technology_flow_signature =
                TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::FIX_CHARACTERIZATION);
            const auto fix_technology_flow_step = design_flow_manager->GetDesignFlowStep(fix_technology_flow_signature);
            const DesignFlowStepRef fix_technology_design_flow_step =
                fix_technology_flow_step != DesignFlowGraph::null_vertex() ?
                    design_flow_graph->CGetNodeInfo(fix_technology_flow_step)->design_flow_step :
                    GetPointer<const TechnologyFlowStepFactory>(technology_flow_step_factory)
                        ->CreateTechnologyFlowStep(TechnologyFlowStep_Type::FIX_CHARACTERIZATION);
            design_flow_manager->AddStep(fix_technology_design_flow_step);

            const auto technology_flow_signature =
                TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::WRITE_TECHNOLOGY);
            const auto technology_flow_step = design_flow_manager->GetDesignFlowStep(technology_flow_signature);
            const DesignFlowStepRef technology_design_flow_step =
                technology_flow_step != DesignFlowGraph::null_vertex() ?
                    design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                    GetPointer<const TechnologyFlowStepFactory>(technology_flow_step_factory)
                        ->CreateTechnologyFlowStep(TechnologyFlowStep_Type::WRITE_TECHNOLOGY);
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

            /// creating the data-structure representing the target device
            generic_deviceRef device = generic_device::factory(parameters, TM);

            const DesignFlowManagerRef design_flow_manager(new DesignFlowManager(parameters));
            const auto design_flow_graph = design_flow_manager->CGetDesignFlowGraph();

            const DesignFlowStepFactoryConstRef technology_flow_step_factory(
                new TechnologyFlowStepFactory(TM, device, design_flow_manager, parameters));
            design_flow_manager->RegisterFactory(technology_flow_step_factory);

            const DesignFlowStepFactoryConstRef to_data_file_step_factory(
                new ToDataFileStepFactory(device, design_flow_manager, parameters));
            design_flow_manager->RegisterFactory(to_data_file_step_factory);

            const auto to_data_file_step_signature =
                ToDataFileStep::ComputeSignature(ToDataFileStep_Type::GENERATE_FU_LIST);
            const auto to_data_file_vertex = design_flow_manager->GetDesignFlowStep(to_data_file_step_signature);
            const auto to_data_file_step = to_data_file_vertex != DesignFlowGraph::null_vertex() ?
                                               design_flow_graph->CGetNodeInfo(to_data_file_vertex)->design_flow_step :
                                               GetPointer<const ToDataFileStepFactory>(to_data_file_step_factory)
                                                   ->CreateStep(to_data_file_step_signature);
            design_flow_manager->AddStep(to_data_file_step);
            design_flow_manager->Exec();
#else
            THROW_ERROR("Input technology file not supported.");
#endif

            break;
         }
#endif
         case(Parameters_FileFormat::FF_XML_WGT_SYM):
         case(Parameters_FileFormat::FF_CSV):
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
         case(Parameters_FileFormat::FF_LLVM_CPP):
#endif
#if HAVE_FROM_C_BUILT
         case(Parameters_FileFormat::FF_RAW):
#endif
         case(Parameters_FileFormat::FF_TEX):
         case(Parameters_FileFormat::FF_TGFF):
         case(Parameters_FileFormat::FF_VERILOG):
         case(Parameters_FileFormat::FF_VHDL):
#if HAVE_HLS_BUILT
         case(Parameters_FileFormat::FF_XML_CON):
#endif
         case Parameters_FileFormat::FF_XML_EXPERIMENTAL_SETUP:
         case(Parameters_FileFormat::FF_XML_SKIP_ROW):
         case(Parameters_FileFormat::FF_XML_SYM_SIM):
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
   if(!parameters->getOption<bool>(OPT_no_clean))
   {
      std::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
   }

   return exit_code;
}
