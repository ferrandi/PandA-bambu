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
 *              Copyright (C) 2004-2022 Politecnico di Milano
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

#include "testbench_memory_allocation.hpp"

#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "behavioral_helper.hpp"
#include "c_initialization_parser.hpp"
#include "c_initialization_parser_functor.hpp"
#include "call_graph_manager.hpp"
#include "compute_reserved_memory.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "string_manipulation.hpp" // for STR
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <list>
#include <string>
#include <tuple>
#include <vector>

TestbenchMemoryAllocation::TestbenchMemoryAllocation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                                     const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::TESTBENCH_MEMORY_ALLOCATION)
{
   flag_cpp = _HLSMgr.get()->get_tree_manager()->is_CPP() && !_parameters->isOption(OPT_pretty_print) &&
              (!_parameters->isOption(OPT_discrepancy) || !_parameters->getOption<bool>(OPT_discrepancy) ||
               !_parameters->isOption(OPT_discrepancy_hw) || !_parameters->getOption<bool>(OPT_discrepancy_hw));
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

TestbenchMemoryAllocation::~TestbenchMemoryAllocation() = default;

DesignFlowStep_Status TestbenchMemoryAllocation::Exec()
{
   AllocTestbenchMemory();
   return DesignFlowStep_Status::SUCCESS;
}

void TestbenchMemoryAllocation::AllocTestbenchMemory(void) const
{
   const auto TM = HLSMgr->get_tree_manager();
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
   const auto function_id = *(top_function_ids.begin());
   const auto BH = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();

   const auto mem_vars = HLSMgr->Rmem->get_ext_memory_variables();
   CInitializationParserRef c_initialization_parser(new CInitializationParser(parameters));
   // get the mapping between variables in external memory and their external
   // base address
   std::map<unsigned long long int, unsigned int> address;
   for(const auto& m : mem_vars)
   {
      address[HLSMgr->Rmem->get_external_base_address(m.first)] = m.first;
   }

   std::list<unsigned int> mem;
   for(const auto& ma : address)
   {
      mem.push_back(ma.second);
   }

   const auto func_parameters = BH->get_parameters();
   for(const auto& p : func_parameters)
   {
      // if the function has some pointer func_parameters some memory needs to be
      // reserved for the place where they point to
      if(tree_helper::is_a_pointer(TM, p) && mem_vars.find(p) == mem_vars.end())
      {
         mem.push_back(p);
      }
   }

   // loop on the test vectors
   unsigned int v_idx = 0;
   for(const auto& curr_test_vector : HLSMgr->RSim->test_vectors)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering test vector " + STR(v_idx));
      HLSMgr->RSim->param_address[v_idx].clear();
      // loop on the variables in memory
      for(auto l = mem.begin(); l != mem.end(); ++l)
      {
         std::string param = BH->PrintVariable(*l);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering " + param);
         if(param[0] == '"')
         {
            param = "@" + STR(*l);
         }
         bool is_memory = false;
         std::string test_v = "0";
         if(mem_vars.find(*l) != mem_vars.end() &&
            std::find(func_parameters.begin(), func_parameters.end(), *l) == func_parameters.end())
         {
            is_memory = true;
            test_v = TestbenchGenerationBaseStep::print_var_init(TM, *l, HLSMgr->Rmem);
         }
         else if(curr_test_vector.find(param) != curr_test_vector.end())
         {
            test_v = curr_test_vector.find(param)->second;
         }

         if(v_idx > 0 && is_memory)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped " + param);
            continue; // memory has been already initialized
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Initialization string is " + test_v);

         const auto lnode = TM->CGetTreeReindex(*l);
         auto reserved_bytes = tree_helper::Size(lnode) / 8;
         if(reserved_bytes == 0)
         {
            reserved_bytes = 1;
         }

         const auto l_type = tree_helper::CGetType(lnode);
         if(tree_helper::IsPointerType(l_type) && !is_memory)
         {
            if(test_v.size() > 4 && test_v.substr(test_v.size() - 4) == ".dat")
            {
               std::ifstream in(test_v, std::ifstream::ate | std::ifstream::binary);
               reserved_bytes = static_cast<unsigned long long>(in.tellg());
            }
            else if(flag_cpp)
            {
               tree_nodeConstRef ptd_base_type;
               if(GET_CONST_NODE(pt_node)->get_kind() == pointer_type_K)
               {
                  ptd_base_type = GetPointer<const pointer_type>(GET_CONST_NODE(pt_node))->ptd;
               }
               else if(GET_CONST_NODE(pt_node)->get_kind() == reference_type_K)
               {
                  ptd_base_type = GetPointer<const reference_type>(GET_CONST_NODE(pt_node))->refd;
               }
               else
               {
                  THROW_ERROR("A pointer type is expected");
               }

               unsigned long long base_type_byte_size;
               if(tree_helper::IsStructType(ptd_base_type) || tree_helper::IsUnionType(ptd_base_type))
               {
                  base_type_byte_size = tree_helper::Size(ptd_base_type) / 8;
               }
               else if(tree_helper::IsArrayEquivType(ptd_base_type))
               {
                  base_type_byte_size = tree_helper::GetArrayElementSize(ptd_base_type) / 8;
               }
               else if(tree_helper::Size(ptd_base_type) == 1)
               {
                  base_type_byte_size = 1;
               }
               else
               {
                  base_type_byte_size = tree_helper::Size(ptd_base_type) / 8;
               }

               if(base_type_byte_size == 0)
               {
                  base_type_byte_size = 1;
               }
               std::vector<std::string> splitted = SplitString(test_v, ",");
               reserved_bytes = (static_cast<unsigned int>(splitted.size())) * base_type_byte_size;
            }
            else
            {
               const CInitializationParserFunctorRef c_initialization_parser_functor(
                   new ComputeReservedMemory(TM, lnode));
               c_initialization_parser->Parse(c_initialization_parser_functor, test_v);
               reserved_bytes = GetPointer<ComputeReservedMemory>(c_initialization_parser_functor)->GetReservedBytes();
            }

            if(HLSMgr->RSim->param_address[v_idx].find(*l) == HLSMgr->RSim->param_address[v_idx].end())
            {
               HLSMgr->RSim->param_address[v_idx][*l] = HLSMgr->Rmem->get_memory_address();
               HLSMgr->RSim->param_mem_size[v_idx][*l] = reserved_bytes;
               HLSMgr->Rmem->reserve_space(reserved_bytes);

               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                              "---Parameter " + param + " (" + STR((*l)) + ") (testvector " + STR(v_idx) +
                                  ") allocated at " + STR(HLSMgr->RSim->param_address.at(v_idx).at(*l)) +
                                  " : reserved_mem_size = " + STR(HLSMgr->RSim->param_mem_size.at(v_idx).at(*l)));
            }
         }
         else if(!is_memory)
         {
            THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "not yet supported case");
         }

         std::list<unsigned int>::const_iterator l_next;
         l_next = l;
         ++l_next;
         unsigned long long int next_object_offset = 0;
         /// check the next free aligned address
         if(l_next != mem.end() && mem_vars.find(*l_next) != mem_vars.end() && mem_vars.find(*l) != mem_vars.end())
         {
            next_object_offset =
                HLSMgr->Rmem->get_base_address(*l_next, function_id) - HLSMgr->Rmem->get_base_address(*l, function_id);
         }
         else if(mem_vars.find(*l) != mem_vars.end() &&
                 (l_next == mem.end() ||
                  HLSMgr->RSim->param_address.at(v_idx).find(*l_next) == HLSMgr->RSim->param_address.at(v_idx).end()))
         {
            next_object_offset = HLSMgr->Rmem->get_memory_address() - HLSMgr->Rmem->get_base_address(*l, function_id);
         }
         else if(l_next != mem.end() && mem_vars.find(*l) != mem_vars.end() &&
                 HLSMgr->RSim->param_address.at(v_idx).find(*l_next) != HLSMgr->RSim->param_address.at(v_idx).end())
         {
            next_object_offset = HLSMgr->RSim->param_address.at(v_idx).find(*l_next)->second -
                                 HLSMgr->Rmem->get_base_address(*l, function_id);
         }
         else if(l_next != mem.end() &&
                 HLSMgr->RSim->param_address.at(v_idx).find(*l) != HLSMgr->RSim->param_address.at(v_idx).end() &&
                 HLSMgr->RSim->param_address.at(v_idx).find(*l_next) != HLSMgr->RSim->param_address.at(v_idx).end())
         {
            next_object_offset = HLSMgr->RSim->param_address.at(v_idx).find(*l_next)->second -
                                 HLSMgr->RSim->param_address.at(v_idx).find(*l)->second;
         }
         else if(HLSMgr->RSim->param_address.at(v_idx).find(*l) != HLSMgr->RSim->param_address.at(v_idx).end())
         {
            next_object_offset =
                HLSMgr->Rmem->get_memory_address() - HLSMgr->RSim->param_address.at(v_idx).find(*l)->second;
         }
         else
         {
            THROW_ERROR("unexpected pattern");
         }

         if(next_object_offset < reserved_bytes)
         {
            THROW_ERROR("more allocated memory than expected  next_object_offset=" + STR(next_object_offset) +
                        " reserved_bytes=" + STR(reserved_bytes));
         }
         HLSMgr->RSim->param_next_off[v_idx][*l] = next_object_offset;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered " + param);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered test vector " + STR(v_idx));
      v_idx++;
   }
   return;
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
TestbenchMemoryAllocation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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

bool TestbenchMemoryAllocation::HasToBeExecuted() const
{
   return true;
}
