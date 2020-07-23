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
 *              Copyright (c) 2019-2020 Politecnico di Milano
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

/// Header include
#include "memory_initialization_c_writer.hpp"

/// . include
#include "Parameter.hpp"

/// HLS/simulation include
#include "testbench_generation.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

/// utility includes
#include "dbgPrintHelper.hpp"
#include "indented_output_stream.hpp"
#include "utility.hpp"

MemoryInitializationCWriter::MemoryInitializationCWriter(const IndentedOutputStreamRef _indented_output_stream, const tree_managerConstRef _TM, const BehavioralHelperConstRef _behavioral_helper, const unsigned long int _reserved_mem_bytes,
                                                         const tree_nodeConstRef _function_parameter, const TestbenchGeneration_MemoryType _testbench_generation_memory_type, const ParameterConstRef _parameters)
    : MemoryInitializationWriterBase(_TM, _behavioral_helper, _reserved_mem_bytes, _function_parameter, _testbench_generation_memory_type, _parameters), indented_output_stream(_indented_output_stream)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

static bool is_all_8zeros(const std::string& str)
{
   size_t size = str.size();
   if(size % 8 != 0 || size == 8)
      return false;
   for(size_t i = 0; i < size; ++i)
      if(str.at(i) != '0')
         return false;
   return true;
}

void MemoryInitializationCWriter::Process(const std::string& content)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing C code to write " + content + " in binary form to initialize memory");
   unsigned int base_type_index = 0;
   /// Second, according to the type let's how many elements have to have been processed
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Currently writing " + status.back().first->get_kind_text());
   switch(status.back().first->get_kind())
   {
      case pointer_type_K:
      case integer_type_K:
      case real_type_K:
      case boolean_type_K:
         base_type_index = status.back().first->index;
         break;
      case array_type_K:
      case CharType_K:
      case enumeral_type_K:
      case complex_type_K:
      case record_type_K:
      case union_type_K:
      case function_type_K:
      case lang_type_K:
      case method_type_K:
      case nullptr_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case reference_type_K:
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
   THROW_ASSERT(base_type_index != 0, "");
   const auto base_type = TM->CGetTreeNode(base_type_index);
   std::string binary_value = "";
   unsigned int size = 0;
   switch(base_type->get_kind())
   {
      case boolean_type_K:
         size = 8;
         binary_value = ConvertInBinary(content, size, false, true);
         break;
      case integer_type_K:
         size = tree_helper::size(TM, base_type_index);
         binary_value = ConvertInBinary(content, size, false, tree_helper::is_unsigned(TM, base_type_index));
         break;
      case real_type_K:
         size = tree_helper::size(TM, base_type_index);
         binary_value = ConvertInBinary(content, size, true, false);
         break;
      case pointer_type_K:
         size = tree_helper::size(TM, base_type_index);
         binary_value = ConvertInBinary(content, size, false, true);
         break;
      case array_type_K:
      case CharType_K:
      case enumeral_type_K:
      case complex_type_K:
      case record_type_K:
      case union_type_K:
      case function_type_K:
      case lang_type_K:
      case method_type_K:
      case nullptr_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case reference_type_K:
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
   THROW_ASSERT(binary_value.size() % 8 == 0, "");
   written_bytes += binary_value.size() / 8;
   switch(testbench_generation_memory_type)
   {
      case TestbenchGeneration_MemoryType::INPUT_PARAMETER:
      {
         THROW_ASSERT(write_in_a_file, "unexpected condition");
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//parameter: " + behavioral_helper->PrintVariable(function_parameter->index) + " value: " + content + "\\n\");\n");
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"p" + binary_value + "\\n\");\n");
         break;
      }
      case TestbenchGeneration_MemoryType::MEMORY_INITIALIZATION:
      {
         if(write_in_a_file)
         {
            for(size_t bit = 0; bit < binary_value.size(); bit += 8)
            {
               memory_init_file << "m" + binary_value.substr(binary_value.size() - 8 - bit, 8) + "\n";
            }
         }
         else
         {
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//memory initialization for variable: " + behavioral_helper->PrintVariable(function_parameter->index) + " value: " + content + "\\n\");\n");
            if(is_all_8zeros(binary_value))
            {
               indented_output_stream->Append("for (__testbench_index = 0; "
                                              "__testbench_index < " +
                                              STR(binary_value.size() / 8) + "; " +
                                              "++__testbench_index)\n"
                                              "   fprintf(__bambu_testbench_fp, \"m00000000\\n\");\n");
            }
            else
            {
               for(size_t bit = 0; bit < binary_value.size(); bit += 8)
               {
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"m" + binary_value.substr(binary_value.size() - 8 - bit, 8) + "\\n\");\n");
               }
            }
         }
         break;
      }
      case TestbenchGeneration_MemoryType::OUTPUT_PARAMETER:
      {
         THROW_ASSERT(write_in_a_file, "unexpected condition");
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output: " + behavioral_helper->PrintVariable(function_parameter->index) + " value: " + content + "\\n\");\n");
         for(size_t bit = 0; bit < binary_value.size(); bit += 8)
         {
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o" + binary_value.substr(binary_value.size() - 8 - bit, 8) + "\\n\");\n");
         }
         break;
      }
      case TestbenchGeneration_MemoryType::RETURN:
      default:
         THROW_UNREACHABLE("");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added code to write " + content + " (" + STR(binary_value.size() / 8) + " bytes) in binary form to initialize memory");
}

void MemoryInitializationCWriter::ActivateFileInit(const std::string& filename)
{
   write_in_a_file = true;
   file_variable = filename;
   memory_init_file.open(file_variable.c_str());
}

void MemoryInitializationCWriter::FinalizeFileInit()
{
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("FILE * __bambu_testbench_fp_local_copy;\n");
   indented_output_stream->Append("char * line = NULL;\n");
   indented_output_stream->Append("size_t len = 0;\n");
   indented_output_stream->Append("ssize_t read;\n");
   indented_output_stream->Append("__bambu_testbench_fp_local_copy = fopen(\"" + file_variable + "\", \"r\");\n");
   indented_output_stream->Append("if (__bambu_testbench_fp_local_copy == NULL)\n");
   indented_output_stream->Append("   exit(1);\n");
   indented_output_stream->Append("while ((read = getline(&line, &len, __bambu_testbench_fp_local_copy)) != -1) {\n");
   indented_output_stream->Append("   fprintf(__bambu_testbench_fp, \"%s\", line);\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("fclose(__bambu_testbench_fp_local_copy);\n");
   indented_output_stream->Append("if (line)\n");
   indented_output_stream->Append("   free(line);\n");
   indented_output_stream->Append("}\n");
}
