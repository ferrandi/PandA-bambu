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
 * @file test_vector_parser.cpp
 * @brief .
 *
 */
#include "test_vector_parser.hpp"

/// behavior include
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// boost include
#include <boost/algorithm/string.hpp>

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_analysis
#include "application_frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"

/// HLS/ include
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"

/// include from HLS/simulation
#include "SimulationInformation.hpp"

/// parser/polixml include
#include "xml_dom_parser.hpp"

/// polixml include
#include "xml_document.hpp"

/// STL include
#include "custom_set.hpp"
#include <tuple>
#include <utility>

/// tree/ include
#include "behavioral_helper.hpp"

/// utility includes
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

/// wrapper/treegcc include
#include "gcc_wrapper.hpp"

TestVectorParser::TestVectorParser(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager) : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::TEST_VECTOR_PARSER)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

TestVectorParser::~TestVectorParser() = default;

void TestVectorParser::ParseUserString(std::vector<std::map<std::string, std::string>>& test_vectors) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining " + user_input_string);
   std::string local_string = user_input_string;

   /// pre-processing to support arrays
   std::string::iterator last_comma = local_string.end();
   for(auto it = local_string.begin(), it_end = local_string.end(); it != it_end; ++it)
   {
      if(*it == ',')
         last_comma = it;
      else if(*it == '=' && last_comma != it_end)
         *last_comma = '$';
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Preprocessed string " + local_string);
   test_vectors.push_back(std::map<std::string, std::string>());
   std::vector<std::string> testbench_parameters = SplitString(local_string, "$");
   unsigned int index = 0;
   for(auto parameter : testbench_parameters)
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined " + user_input_string);
}

void TestVectorParser::ParseXMLFile(std::vector<std::map<std::string, std::string>>& test_vectors) const
{
   const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();

   THROW_ASSERT(boost::num_vertices(*(call_graph_manager->CGetCallGraph())) != 0, "The call graph has not been computed yet");

   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
   const auto function_id = *(top_function_ids.begin());
   const BehavioralHelperConstRef behavioral_helper = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();

   if(!boost::filesystem::exists(input_xml_filename))
   {
      THROW_WARNING("XML file \"" + input_xml_filename + "\" cannot be opened, creating a stub with random values");
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("function");
      xml_element* node = nodeRoot->add_child_element("testbench");

      for(const auto function_parameter : behavioral_helper->get_parameters())
      {
         if(behavioral_helper->is_a_pointer(function_parameter))
            continue;
         std::string param = behavioral_helper->PrintVariable(function_parameter);

         long long int value = (rand() % 20);
         if(behavioral_helper->is_bool(function_parameter))
            value = value % 2;
         node->set_attribute(param, boost::lexical_cast<std::string>(value));
      }

      document.write_to_file_formatted(input_xml_filename);
   }
   try
   {
      XMLDomParser parser(input_xml_filename);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         const xml_node::node_list list = node->get_children();
         for(const auto& iter : list)
         {
            const auto* Enode = GetPointer<const xml_element>(iter);

            if(!Enode || Enode->get_name() != "testbench")
               continue;

            std::map<std::string, std::string> test_vector;

            for(const auto function_parameter : behavioral_helper->get_parameters())
            {
               std::string param = behavioral_helper->PrintVariable(function_parameter);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Parameter: " + param + (behavioral_helper->is_a_pointer(function_parameter) ? " (memory access)" : " (input value)"));
               if((Enode)->get_attribute(param))
               {
                  test_vector[param] = boost::lexical_cast<std::string>((Enode)->get_attribute(param)->get_value());
               }
               else if((Enode)->get_attribute(param + ":init_file"))
               {
                  const auto test_directory = GetDirectory(input_xml_filename);
                  const auto input_file_name = BuildPath(test_directory, Enode->get_attribute(param + ":init_file")->get_value());
                  if(input_file_name.size() > 4 && input_file_name.substr(input_file_name.size() - 4) == ".dat")
                  {
                     test_vector[param] = input_file_name;
                  }
                  else
                  {
                     const auto input_file = fileIO_istream_open(input_file_name);
                     test_vector[param] = std::string(std::istreambuf_iterator<char>(*input_file), std::istreambuf_iterator<char>());
                  }
               }
               else if(!behavioral_helper->is_a_pointer(function_parameter))
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
                  const auto test_directory = GetDirectory(input_xml_filename);
                  const auto input_file_name = BuildPath(test_directory, Enode->get_attribute(param + ":init_output_file")->get_value());
                  const auto input_file = fileIO_istream_open(input_file_name);
                  test_vector[param + ":output"] = std::string(std::istreambuf_iterator<char>(*input_file), std::istreambuf_iterator<char>());
               }
            }
            if(behavioral_helper->GetFunctionReturnType(function_id) and ((Enode)->get_attribute("return")))
            {
               HLSMgr->RSim->results_available = true;
               test_vector["return"] = ((Enode)->get_attribute("return")->get_value());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Expected return value is " + test_vector["return"]);
            }
            test_vectors.emplace_back(std::move(test_vector));
         }
         /// If discrepancy is enabled, then xml output is ignored
         if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy) and HLSMgr->RSim->results_available)
         {
            HLSMgr->RSim->results_available = false;
            THROW_WARNING("Output stored in xml file will be ignored since discrepancy analysis is enabled");
         }
         return;
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
   return;
}

size_t TestVectorParser::ParseTestVectors(std::vector<std::map<std::string, std::string>>& test_vectors) const
{
   if(not input_xml_filename.empty())
   {
      ParseXMLFile(test_vectors);
   }
   else if(not user_input_string.empty())
   {
      ParseUserString(test_vectors);
   }
   else
   {
      THROW_UNREACHABLE("");
   }
   return test_vectors.size();
}

void TestVectorParser::Initialize()
{
   HLS_step::Initialize();

   if(parameters->isOption(OPT_testbench_input_xml))
   {
      input_xml_filename = parameters->getOption<std::string>(OPT_testbench_input_xml);
      user_input_string.clear();
   }
   else if(parameters->isOption(OPT_testbench_input_string))
   {
      user_input_string = parameters->getOption<std::string>(OPT_testbench_input_string);
      input_xml_filename.clear();
   }
   else
   {
      THROW_UNREACHABLE("");
   }

   HLSMgr->RSim = SimulationInformationRef(new SimulationInformation());
}

DesignFlowStep_Status TestVectorParser::Exec()
{
#ifndef NDEBUG
   size_t n_vectors =
#endif
       ParseTestVectors(HLSMgr->RSim->test_vectors);
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Number of input test vectors: " + STR(n_vectors));
   return DesignFlowStep_Status::SUCCESS;
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> TestVectorParser::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
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
