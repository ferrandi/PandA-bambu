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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file memory_initialization_writer.cpp
 * @brief Functor used to write initialization of the memory
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "memory_initialization_writer_base.hpp"

///. include
#include "Parameter.hpp"

/// HLS/simulation include
#include "testbench_generation.hpp"

/// STD include
#include <string>

/// STL include
#include <utility>

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "utility.hpp"

MemoryInitializationWriterBase::MemoryInitializationWriterBase(const tree_managerConstRef _TM, const BehavioralHelperConstRef _behavioral_helper, const unsigned long int _reserved_mem_bytes, const tree_nodeConstRef _function_parameter,
                                                               const TestbenchGeneration_MemoryType _testbench_generation_memory_type, const ParameterConstRef)
    : TM(_TM), behavioral_helper(_behavioral_helper), reserved_mem_bytes(_reserved_mem_bytes), written_bytes(0), function_parameter(_function_parameter), testbench_generation_memory_type(_testbench_generation_memory_type)
{
   const auto parameter_type = GetPointer<const type_node>(function_parameter) ? function_parameter : TM->CGetTreeNode(tree_helper::get_type_index(TM, function_parameter->index));
   status.push_back(std::pair<const tree_nodeConstRef, size_t>(parameter_type, 0));
}
void MemoryInitializationWriterBase::CheckEnd()
{
   if(written_bytes != reserved_mem_bytes)
   {
      THROW_ERROR("Not enough bytes written: " + STR(written_bytes) + " vs. " + STR(reserved_mem_bytes));
   }
   /// First of all we have to check that there is just one element in the stack
   if(status.size() != 1)
      THROW_ERROR("Missing data in C initialization string. Status is " + PrintStatus());
}

void MemoryInitializationWriterBase::GoUp()
{
   THROW_ASSERT(status.size() >= 2, "");
   status.pop_back();
   status.back().second++;
   size_t expected_size = 0;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--GoUp - New status is " + PrintStatus());

   /// Second, according to the type let's how many elements have to have been processed
   std::vector<unsigned int> array_dimensions;
   switch(status.back().first->get_kind())
   {
      case array_type_K:
         /// parameters cannot have this type, but global variables can
         tree_helper::get_array_dimensions(TM, status.back().first->index, array_dimensions);
         expected_size = array_dimensions.front();
         break;
      case complex_type_K:
         expected_size = 2;
         break;
      case pointer_type_K:
         expected_size = 0; /// Actually the expected size is unknown
         break;
      case reference_type_K:
         expected_size = 0; /// Actual size depends on the pointed type
         break;
      case record_type_K:
      case union_type_K:
         expected_size = tree_helper::CGetFieldTypes(status.back().first).size();
         break;
      case boolean_type_K:
      case CharType_K:
      case enumeral_type_K:
      case integer_type_K:
      case real_type_K:
      case function_type_K:
      case lang_type_K:
      case method_type_K:
      case nullptr_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case set_type_K:
      case template_type_parm_K:
      case typename_type_K:
      case type_argument_pack_K:
      case type_pack_expansion_K:
      case vector_type_K:
      case void_type_K:
         THROW_ERROR("Unexpected type in initializing parameter/variable: " + status.back().first->get_kind_text());
         break;
      case aggr_init_expr_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case lut_expr_K:
      case CASE_CPP_NODES:
      case CASE_BINARY_EXPRESSION:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + std::string(status.back().first->get_kind_text()));
   }
   if(expected_size != 0 and expected_size != status.back().second)
      THROW_ERROR("Missing data in C initialization for node of type " + status.back().first->get_kind_text() + " " + STR(expected_size) + " vs. " + STR(status.back().second));
}

void MemoryInitializationWriterBase::GoDown()
{
   THROW_ASSERT(not status.empty(), "");
   const auto type_node = status.back().first;
   const auto new_type = [&]() -> tree_nodeConstRef {
      if(type_node->get_kind() == record_type_K or type_node->get_kind() == union_type_K)
      {
         const auto fields = tree_helper::CGetFieldTypes(type_node);
         THROW_ASSERT(fields.size() > status.back().second, STR(fields.size()) + " vs. " + STR(status.back().second));
         return tree_helper::CGetFieldTypes(type_node)[status.back().second];
      }
      if(type_node->get_kind() == array_type_K)
      {
         return tree_helper::CGetElements(type_node);
      }
      if(type_node->get_kind() == pointer_type_K or type_node->get_kind() == reference_type_K)
      {
         return tree_helper::CGetPointedType(type_node);
      }
      THROW_ERROR("Unexpected nested initialization " + type_node->get_kind_text() + " - Current status is " + PrintStatus());
      return tree_nodeRef();
   }();
   status.push_back(std::pair<const tree_nodeConstRef, size_t>(new_type, 0));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Going down. New level " + PrintStatus());
}

void MemoryInitializationWriterBase::GoNext()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updating the status from " + PrintStatus());
   THROW_ASSERT(status.size() >= 2, "");
   const tree_nodeConstRef upper_type = status[status.size() - 2].first;
   if(upper_type->get_kind() == record_type_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Read field of a record");
      /// We have read a field of record, move to the next field, if any
      const auto record_fields = tree_helper::CGetFieldTypes(upper_type);

      /// Check if there is at least another field
#ifndef NDEBUG
      const auto read_fields = status[status.size() - 2].second;
      THROW_ASSERT(read_fields < record_fields.size(), "");
#endif
      status.pop_back();
      status.back().second++;
      const auto new_type = tree_helper::CGetFieldTypes(upper_type)[status[status.size() - 1].second];
      status.push_back(std::pair<const tree_nodeConstRef, unsigned int>(new_type, 0));
   }
   else
   {
      THROW_ASSERT(upper_type->get_kind() == array_type_K or upper_type->get_kind() == pointer_type_K, upper_type->get_kind_text());
      status[status.size() - 2].second++;
      status[status.size() - 1].second = 0;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated the status to " + PrintStatus());
}

const std::string MemoryInitializationWriterBase::PrintStatus() const
{
   std::string ret;
   for(const auto& level : status)
   {
      if(ret != "")
         ret += ":";
      ret += level.first->get_kind_text() + "[" + STR(level.second) + "]";
   }
   return ret;
}
