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
 * @file test_vector_parser.cpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 */
#include "test_vector_parser.hpp"

#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "application_frontend_flow_step.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"
#include "xml_document.hpp"
#include "xml_dom_parser.hpp"

#include <boost/algorithm/string.hpp>
#include <regex>
#include <tuple>
#include <utility>

TestVectorParser::TestVectorParser(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                   const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::TEST_VECTOR_PARSER)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
TestVectorParser::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

bool TestVectorParser::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status TestVectorParser::Exec()
{
   HLSMgr->RSim = std::make_unique<SimulationInformation>();

   if(parameters->isOption(OPT_testbench_input_file))
   {
      const auto tb_files = parameters->getOption<std::vector<std::string>>(OPT_testbench_input_file);
      if(ends_with(tb_files.front(), ".xml"))
      {
         THROW_ASSERT(tb_files.size() == 1, "XML testbench initialization must be in a single file.");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining " + *tb_files.begin());
         HLSMgr->RSim->test_vectors = ParseXMLFile(tb_files.front());
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--User provided co-simulation files will be used for test vectors generation");
         return DesignFlowStep_Status::SUCCESS;
      }
   }
   else if(parameters->isOption(OPT_testbench_input_string))
   {
      const auto input_string = parameters->getOption<std::string>(OPT_testbench_input_string);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining " + input_string);
      HLSMgr->RSim->test_vectors = ParseUserString(input_string);
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---No testbench information provided, generating simulation driver library only.");
      return DesignFlowStep_Status::SUCCESS;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                  "<--Number of input test vectors: " + STR(HLSMgr->RSim->test_vectors.size()));
   return DesignFlowStep_Status::SUCCESS;
}

std::vector<std::map<std::string, std::string>> TestVectorParser::ParseUserString(const std::string& input_string) const
{
   std::vector<std::map<std::string, std::string>> test_vectors;
   auto tb_strings = string_to_container<std::vector<std::string>>(input_string, STR_CST_string_separator);
   for(auto& tb_string : tb_strings)
   {
      /// pre-processing to support arrays
      std::string::iterator last_comma = tb_string.end();
      for(auto it = tb_string.begin(), it_end = tb_string.end(); it != it_end; ++it)
      {
         if(*it == ',')
         {
            last_comma = it;
         }
         else if(*it == '=' && last_comma != it_end)
         {
            *last_comma = '$';
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Preprocessed string " + tb_string);
      test_vectors.push_back(std::map<std::string, std::string>());
      const auto testbench_parameters = string_to_container<std::vector<std::string>>(tb_string, "$");
      for(const auto& parameter : testbench_parameters)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Examining " + parameter);
         const auto temp = string_to_container<std::vector<std::string>>(parameter, "=");
         if(temp.size() != 2)
         {
            THROW_ERROR_USAGE("Invalid --generate-tb argument format. "
                              "Use comma-separated key=value pairs (example: a=1,b=2).");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + temp[0] + "=" + temp[1]);
         test_vectors.back()[temp[0]] = temp[1];
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   return test_vectors;
}

std::vector<std::map<std::string, std::string>>
TestVectorParser::ParseXMLFile(const std::filesystem::path& input_xml_filename) const
{
   THROW_ASSERT(HLSMgr->CGetCallGraphManager().GetCallGraph().num_vertices() != 0,
                "The call graph has not been computed yet");
   const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = HLSMgr->get_ir_manager()->GetFunction(top_symbols.front());
   const auto BH = HLSMgr->CGetFunctionBehavior(top_fnode->index)->CGetBehavioralHelper();

   if(!std::filesystem::exists(input_xml_filename))
   {
      THROW_WARNING("XML file \"" + input_xml_filename.string() +
                    "\" cannot be opened, creating a stub with random values");
      xml_document document;
      const auto nodeRoot = document.create_root_node("function");
      const auto node = nodeRoot->add_child_element("testbench");

      for(const auto& function_parameter : BH->GetParameters())
      {
         if(ir_helper::IsPointerType(function_parameter))
         {
            THROW_UNREACHABLE("Random testbench parameters generation is not available for pointer parameters. Please "
                              "provide a valid testbench XML file.");
            continue;
         }
         const auto param = BH->PrintVariable(function_parameter->index);

         auto value = (rand() % 20);
         if(ir_helper::IsBooleanType(function_parameter))
         {
            value = value % 2;
         }
         node->set_attribute(param, STR(value));
      }

      document.write_to_file_formatted(input_xml_filename);
   }
   try
   {
      XMLDomParser parser(input_xml_filename.string());
      parser.Exec();
      if(parser)
      {
         std::vector<std::map<std::string, std::string>> test_vectors;

         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         const xml_node::node_list list = node->get_children();
         for(const auto& iter : list)
         {
            const auto* Enode = GetPointer<const xml_element>(iter);

            if(!Enode || Enode->get_name() != "testbench")
            {
               continue;
            }

            std::map<std::string, std::string> test_vector;

            for(const auto function_parameter : BH->get_parameters())
            {
               std::string param = BH->PrintVariable(function_parameter);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "Parameter: " + param +
                                  (BH->is_a_pointer(function_parameter) ? " (memory access)" : " (input value)"));
               if((Enode)->get_attribute(param))
               {
                  test_vector[param] = STR((Enode)->get_attribute(param)->get_value());
               }
               else if((Enode)->get_attribute(param + ":init_file"))
               {
                  auto init_file = std::filesystem::path(Enode->get_attribute(param + ":init_file")->get_value());
                  if(init_file.is_relative())
                  {
                     init_file = (input_xml_filename.parent_path() / init_file)
                                     .lexically_relative(std::filesystem::current_path());
                  }
                  if(init_file.extension() == ".dat")
                  {
                     test_vector[param] = init_file.string();
                  }
                  else
                  {
                     const auto input_file = fileIO_istream_open(init_file.string());
                     test_vector[param] =
                         std::string(std::istreambuf_iterator<char>(*input_file), std::istreambuf_iterator<char>());
                  }
               }
               else if(!BH->is_a_pointer(function_parameter))
               {
                  THROW_ERROR_USAGE("Missing input value for parameter '" + param +
                                    "' in testbench XML. Add the attribute in each <testbench> entry.");
               }
            }
            test_vectors.emplace_back(std::move(test_vector));
         }
         return test_vectors;
      }
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
   THROW_ERROR_USAGE("Error parsing test vectors file '" + input_xml_filename.string() +
                     "'. Please verify XML syntax and required testbench attributes.");
   return std::vector<std::map<std::string, std::string>>();
}
