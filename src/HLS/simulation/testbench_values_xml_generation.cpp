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
 *              Copyright (c) 2018-2023 Politecnico di Milano
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
 * @file testbench_values_xml_generation.cpp
 * @brief Class to compute testbench values exploiting only XML values
 *
 * @author Marco Lattuada<marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "testbench_values_xml_generation.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// boost include
#include <boost/filesystem.hpp>

/// constants include
#include "testbench_generation_constants.hpp"

/// HLS include
#include "hls_manager.hpp"

/// HLS/memory includes
#include "memory.hpp"
#include "memory_symbol.hpp"

/// HLS/simulation includes
#include "SimulationInformation.hpp"
#include "c_initialization_parser.hpp"
#include "memory_initialization_writer.hpp"
#include "testbench_generation.hpp"
#include "testbench_generation_base_step.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_map.hpp"
#include <iostream>
#include <list>
#include <tuple>
#include <unordered_set>
#include <vector>

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility includes
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

void TestbenchValuesXMLGeneration::Initialize()
{
}

TestbenchValuesXMLGeneration::TestbenchValuesXMLGeneration(const ParameterConstRef _parameters,
                                                           const HLS_managerRef _hls_manager,
                                                           const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _hls_manager, _design_flow_manager, HLSFlowStep_Type::TESTBENCH_VALUES_XML_GENERATION),
      TM(_hls_manager->get_tree_manager()),
      output_directory(parameters->getOption<std::string>(OPT_output_directory) + "/simulation/")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   if(!boost::filesystem::exists(output_directory))
   {
      boost::filesystem::create_directories(output_directory);
   }
}

TestbenchValuesXMLGeneration::~TestbenchValuesXMLGeneration() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
TestbenchValuesXMLGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::TEST_VECTOR_PARSER, HLSFlowStepSpecializationConstRef(),
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

DesignFlowStep_Status TestbenchValuesXMLGeneration::Exec()
{
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
   const auto function_id = *(top_function_ids.begin());
   const auto behavioral_helper = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   if(!boost::filesystem::exists(output_directory))
   {
      boost::filesystem::create_directories(output_directory);
   }
   std::string output_file_name = output_directory + STR(STR_CST_testbench_generation_basename) + ".txt";
   std::ofstream output_stream(output_file_name, std::ios::out);
   CInitializationParserRef c_initialization_parser = CInitializationParserRef(new CInitializationParser(parameters));

   /// print base address
   unsigned long long int base_address = HLSMgr->base_address;
   output_stream << "//base address " + STR(base_address) << std::endl;
   std::string trimmed_value;
   for(unsigned int ind = 0; ind < 32; ind++)
   {
      trimmed_value = trimmed_value + (((1LLU << (31 - ind)) & base_address) ? '1' : '0');
   }
   output_stream << "b" + trimmed_value << std::endl;

   const std::map<unsigned int, memory_symbolRef>& mem_vars = HLSMgr->Rmem->get_ext_memory_variables();
   // get the mapping between variables in external memory and their external
   // base address
   std::map<unsigned long long int, unsigned int> address;
   for(const auto& m : mem_vars)
   {
      address[HLSMgr->Rmem->get_external_base_address(m.first)] = m.first;
   }

   /// This is the lis of memory variables and of pointer parameters
   std::list<unsigned int> mem;
   for(const auto& ma : address)
   {
      mem.push_back(ma.second);
   }

   const auto fnode = TM->CGetTreeNode(function_id);
   const auto fname = tree_helper::GetMangledFunctionName(GetPointer<const function_decl>(fnode));

   /* Check if there is at least one typename */
   bool typename_found = false;

   if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end())
   {
      for(auto& par : HLSMgr->design_attributes.at(fname))
      {
         if(par.second.find(attr_typename) != par.second.end())
         {
            typename_found = true;
         }
      }
   }
   HLSMgr->RSim->simulationArgSignature.clear();
   const auto& function_parameters = behavioral_helper->GetParameters();
   for(const auto& function_parameter : function_parameters)
   {
      const auto function_parameter_name = behavioral_helper->PrintVariable(GET_INDEX_CONST_NODE(function_parameter));
      HLSMgr->RSim->simulationArgSignature.push_back(function_parameter_name);
      // if the function has some pointer parameters some memory needs to be
      // reserved for the place where they point to
      if(tree_helper::IsPointerType(function_parameter) &&
         mem_vars.find(GET_INDEX_CONST_NODE(function_parameter)) == mem_vars.end())
      {
         mem.push_back(GET_INDEX_CONST_NODE(function_parameter));
      }
   }
   unsigned int v_idx = 0;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization of memory variables");

   // For each test vector, for each pointer parameter, the space to be reserved in memory
   CustomMap<unsigned int, CustomMap<unsigned int, size_t>> all_reserved_mem_bytes;
   // loop on the test vectors
   for(const auto& curr_test_vector : HLSMgr->RSim->test_vectors)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering new test vector");
      // loop on the variables in memory
      for(const auto& l : mem)
      {
         std::string param = behavioral_helper->PrintVariable(l);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering parameter '" + param + "'");
         const auto is_interface =
             std::find_if(function_parameters.begin(), function_parameters.end(), [&](const tree_nodeRef& tn) {
                return GET_INDEX_CONST_NODE(tn) == l;
             }) != function_parameters.end();
         std::string argTypename = "";
         if(typename_found && is_interface)
         {
            THROW_ASSERT(HLSMgr->design_attributes.at(fname).find(param) != HLSMgr->design_attributes.at(fname).end() &&
                             HLSMgr->design_attributes.at(fname).at(param).count(attr_typename),
                         "Parameter should be present in design interface.");
            argTypename = HLSMgr->design_attributes.at(fname).at(param).at(attr_typename) + " ";
            if(argTypename.find("fixed") == std::string::npos)
            {
               argTypename = "";
            }
         }
         if(param[0] == '"')
         {
            param = "@" + STR(l);
         }

         bool is_memory = false;
         std::string test_v = "0";

         /// Initialization of memory variables which are not pointer parameters
         if(mem_vars.find(l) != mem_vars.end() && !is_interface)
         {
            if(v_idx > 0 && is_memory)
            {
               continue; // memory has been already initialized
            }
            is_memory = true;
            test_v = TestbenchGenerationBaseStep::print_var_init(TM, l, HLSMgr->Rmem);
         }
         /// Parameter: read initialization from parsed xml
         else if(curr_test_vector.find(param) != curr_test_vector.end())
         {
            test_v = curr_test_vector.find(param)->second;
            if(argTypename.find("fixed") != std::string::npos)
            {
               test_v = FixedPointReinterpret(test_v, argTypename);
               std::cout << "   " << test_v << std::endl;
            }
         }

         /// Retrieve the space to be reserved in memory
         const auto reserved_mem_bytes = [&]() -> size_t {
            if(is_memory)
            {
               const auto ret_value = tree_helper::Size(TM->CGetTreeReindex(l)) / 8;
               return ret_value ? ret_value : 0;
            }
            else
            {
               THROW_ASSERT(HLSMgr->RSim->param_mem_size.count(v_idx), "");
               THROW_ASSERT(HLSMgr->RSim->param_mem_size.at(v_idx).count(l), "");
               return HLSMgr->RSim->param_mem_size.at(v_idx).at(l);
            }
         }();

         all_reserved_mem_bytes[v_idx][l] = reserved_mem_bytes;

         if(is_memory)
         {
            std::vector<std::string> splitted = SplitString(test_v, ",");
            for(const auto& element : splitted)
            {
               THROW_ASSERT(element.size() % 8 == 0, element + ": " + STR(element.size()));
               for(size_t bits = 0; bits < element.size(); bits += 8)
               {
                  output_stream << "m" << element.substr(element.size() - 8 - bits, 8);
               }
            }
         }
         else
         {
            /// Call the parser to translate C initialization to Verilog initialization
            const CInitializationParserFunctorRef c_initialization_parser_functor =
                CInitializationParserFunctorRef(new MemoryInitializationWriter(
                    output_stream, TM, behavioral_helper, reserved_mem_bytes, TM->CGetTreeReindex(l),
                    TestbenchGeneration_MemoryType::MEMORY_INITIALIZATION, parameters));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Parsing initialization of " + param + "(" +
                               GET_CONST_NODE(tree_helper::CGetType(TM->CGetTreeReindex(l)))->get_kind_text() +
                               "): " + test_v);
            c_initialization_parser->Parse(c_initialization_parser_functor, test_v);
         }
         size_t next_object_offset = HLSMgr->RSim->param_next_off.at(v_idx).at(l);

         if(next_object_offset > reserved_mem_bytes)
         {
            for(unsigned int padding = 0; padding < next_object_offset - reserved_mem_bytes; padding++)
            {
               output_stream << "m00000000" << std::endl;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered parameter '" + param + "'");
      }
      ++v_idx;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered vector");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written initialization of memory variables");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing values of parameters");
   v_idx = 0;
   for(const auto& curr_test_vector : HLSMgr->RSim->test_vectors)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization of parameters");
      for(const auto& function_parameter : function_parameters)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering parameter " + STR(function_parameter));
         const auto type_node = tree_helper::CGetType(function_parameter);
         const auto type = tree_helper::PrintType(TM, type_node);
         const auto param = behavioral_helper->PrintVariable(GET_INDEX_CONST_NODE(function_parameter));
         if(tree_helper::IsPointerType(function_parameter))
         {
            // std::cerr << v_idx << " " << function_parameter << std::endl;
            const auto memory_addr =
                STR(HLSMgr->RSim->param_address.at(v_idx).at(GET_INDEX_CONST_NODE(function_parameter)));
            output_stream << "//parameter: " +
                                 behavioral_helper->PrintVariable(GET_INDEX_CONST_NODE(function_parameter))
                          << " value: " << memory_addr << std::endl;
            output_stream << "p" << ConvertInBinary(memory_addr, 32, false, false) << std::endl;
         }
         else
         {
            const CInitializationParserFunctorRef c_initialization_parser_functor(new MemoryInitializationWriter(
                output_stream, TM, behavioral_helper, tree_helper::Size(function_parameter) / 8, function_parameter,
                TestbenchGeneration_MemoryType::INPUT_PARAMETER, parameters));
            c_initialization_parser->Parse(c_initialization_parser_functor, curr_test_vector.at(param));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered parameter " + STR(function_parameter));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Writing expected content of pointer parameters at the end of the execution");
      for(const auto& function_parameter : function_parameters)
      {
         if(tree_helper::IsPointerType(function_parameter))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Considering parameter " + STR(function_parameter));
            const auto param = behavioral_helper->PrintVariable(GET_INDEX_CONST_NODE(function_parameter));
            auto expected_values = [&]() -> std::string {
               std::string ctv;
               if(curr_test_vector.find(param + ":output") != curr_test_vector.end())
               {
                  ctv = curr_test_vector.at(param + ":output");
               }
               else
               {
                  ctv = curr_test_vector.at(param);
               }
               if(typename_found)
               {
                  THROW_ASSERT(HLSMgr->design_attributes.at(fname).find(param) !=
                                       HLSMgr->design_attributes.at(fname).end() &&
                                   HLSMgr->design_attributes.at(fname).at(param).count(attr_typename),
                               "Parameter should be present in design interface.");
                  const auto argTypename = HLSMgr->design_attributes.at(fname).at(param).at(attr_typename) + " ";
                  if(argTypename.find("fixed") != std::string::npos)
                  {
                     return FixedPointReinterpret(ctv, argTypename);
                  }
               }
               return ctv;
            }();
            const CInitializationParserFunctorRef c_initialization_parser_functor(new MemoryInitializationWriter(
                output_stream, TM, behavioral_helper,
                all_reserved_mem_bytes.at(v_idx).at(GET_INDEX_CONST_NODE(function_parameter)), function_parameter,
                TestbenchGeneration_MemoryType::OUTPUT_PARAMETER, parameters));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Parsing expected output for " + param + ": " + expected_values);
            c_initialization_parser->Parse(c_initialization_parser_functor, expected_values);
            output_stream << "e" << std::endl;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Considered parameter " + STR(function_parameter));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Written expected content of pointer parameters at the end of the execution");
      const auto return_type = tree_helper::GetFunctionReturnType(fnode);
      if(return_type)
      {
         const CInitializationParserFunctorRef c_initialization_parser_functor(
             new MemoryInitializationWriter(output_stream, TM, behavioral_helper, tree_helper::Size(return_type) / 8,
                                            return_type, TestbenchGeneration_MemoryType::RETURN, parameters));
         c_initialization_parser->Parse(c_initialization_parser_functor, curr_test_vector.at("return"));
      }
      ++v_idx;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered vector");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written values of parameters");
   output_stream << "e" << std::endl;
   output_stream.close();
   return DesignFlowStep_Status::SUCCESS;
}

bool TestbenchValuesXMLGeneration::HasToBeExecuted() const
{
   return true;
}
