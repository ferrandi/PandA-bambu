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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file test_vector_parser.cpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 */
#include "test_vector_parser.hpp"

#include "SimulationInformation.hpp"
#include "application_frontend_flow_step.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "compiler_wrapper.hpp"
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
#include "string_manipulation.hpp"
#include "tree_helper.hpp"
#include "xml_document.hpp"
#include "xml_dom_parser.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <tuple>
#include <utility>

TestVectorParser::TestVectorParser(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                   const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::TEST_VECTOR_PARSER)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
TestVectorParser::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
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
   if(!parameters->isOption(OPT_testbench_input_string))
   {
      THROW_UNREACHABLE("");
   }
   HLSMgr->RSim = SimulationInformationRef(new SimulationInformation());

   const auto input_string = parameters->getOption<std::string>(OPT_testbench_input_string);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining " + input_string);
   if(boost::regex_match(input_string, boost::regex("^[\\w\\d\\-\\./]+\\.\\w+$")))
   {
      if(boost::ends_with(input_string, ".xml"))
      {
         HLSMgr->RSim->test_vectors = ParseXMLFile(input_string);
      }
      else
      {
         THROW_UNREACHABLE("Unsupported testbench file format");
      }
   }
   else
   {
      HLSMgr->RSim->test_vectors = ParseUserString(input_string);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level,
                  "<--Number of input test vectors: " + STR(HLSMgr->RSim->test_vectors.size()));
   return DesignFlowStep_Status::SUCCESS;
}

std::vector<std::map<std::string, std::string>> TestVectorParser::ParseUserString(const std::string& input_string) const
{
   std::vector<std::map<std::string, std::string>> test_vectors;
   std::string local_string = input_string;

   /// pre-processing to support arrays
   std::string::iterator last_comma = local_string.end();
   for(auto it = local_string.begin(), it_end = local_string.end(); it != it_end; ++it)
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Preprocessed string " + local_string);
   test_vectors.push_back(std::map<std::string, std::string>());
   std::vector<std::string> testbench_parameters = SplitString(local_string, "$");
   unsigned int index = 0;
   for(const auto& parameter : testbench_parameters)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Examining " + parameter);
      std::vector<std::string> temp = SplitString(parameter, "=");
      if(temp.size() != 2)
      {
         THROW_ERROR("Error in processing --simulate arg");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + temp[0] + "=" + temp[1]);
      test_vectors.back()[temp[0]] = temp[1];
      ++index;
   }
   return test_vectors;
}

std::vector<std::map<std::string, std::string>>
TestVectorParser::ParseXMLFile(const std::string& input_xml_filename) const
{
   const auto CGM = HLSMgr->CGetCallGraphManager();
   THROW_ASSERT(boost::num_vertices(*(CGM->CGetCallGraph())) != 0, "The call graph has not been computed yet");
   const auto top_function_ids = CGM->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
   const auto top_id = *(top_function_ids.begin());
   const auto BH = HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper();

   if(!boost::filesystem::exists(input_xml_filename))
   {
      THROW_WARNING("XML file \"" + input_xml_filename + "\" cannot be opened, creating a stub with random values");
      xml_document document;
      const auto nodeRoot = document.create_root_node("function");
      const auto node = nodeRoot->add_child_element("testbench");

      for(const auto& function_parameter : BH->GetParameters())
      {
         if(tree_helper::IsPointerType(function_parameter))
         {
            THROW_UNREACHABLE("Random testbench parameters generation is not available for pointer parameters. Please "
                              "provide a valid testbench XML file.");
            continue;
         }
         const auto param = BH->PrintVariable(function_parameter->index);

         auto value = (rand() % 20);
         if(tree_helper::IsBooleanType(function_parameter))
         {
            value = value % 2;
         }
         node->set_attribute(param, STR(value));
      }

      document.write_to_file_formatted(input_xml_filename);
   }
   try
   {
      XMLDomParser parser(input_xml_filename);
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
                  const auto test_directory = GetPath(GetDirectory(input_xml_filename));
                  const auto input_file_name =
                      BuildPath(test_directory, Enode->get_attribute(param + ":init_file")->get_value());
                  if(boost::ends_with(input_file_name, ".dat"))
                  {
                     test_vector[param] = input_file_name;
                  }
                  else
                  {
                     const auto input_file = fileIO_istream_open(input_file_name);
                     test_vector[param] =
                         std::string(std::istreambuf_iterator<char>(*input_file), std::istreambuf_iterator<char>());
                  }
               }
               else if(!BH->is_a_pointer(function_parameter))
               {
                  THROW_ERROR("Missing input value for parameter: " + param);
               }
               if((Enode)->get_attribute(param + ":output"))
               {
                  HLSMgr->RSim->results_available = true;
                  test_vector[param + ":output"] = STR((Enode)->get_attribute(param + ":output")->get_value());
               }
               else if((Enode)->get_attribute(param + ":init_output_file"))
               {
                  HLSMgr->RSim->results_available = true;
                  const auto test_directory = GetPath(GetDirectory(input_xml_filename));
                  const auto input_file_name =
                      BuildPath(test_directory, Enode->get_attribute(param + ":init_output_file")->get_value());
                  const auto input_file = fileIO_istream_open(input_file_name);
                  test_vector[param + ":output"] =
                      std::string(std::istreambuf_iterator<char>(*input_file), std::istreambuf_iterator<char>());
               }
            }
            if(BH->GetFunctionReturnType(top_id) && ((Enode)->get_attribute("return")))
            {
               HLSMgr->RSim->results_available = true;
               test_vector["return"] = ((Enode)->get_attribute("return")->get_value());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Expected return value is " + test_vector["return"]);
            }
            test_vectors.emplace_back(std::move(test_vector));
         }
         /// If discrepancy is enabled, then xml output is ignored
         if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy) &&
            HLSMgr->RSim->results_available)
         {
            HLSMgr->RSim->results_available = false;
            THROW_WARNING("Output stored in xml file will be ignored since discrepancy analysis is enabled");
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
   THROW_ERROR("Error parsing the test vectors file " + input_xml_filename);
   return std::vector<std::map<std::string, std::string>>();
}
