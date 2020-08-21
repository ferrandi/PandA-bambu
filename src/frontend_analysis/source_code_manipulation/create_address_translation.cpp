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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file  create_address_translation.cpp
 * @brief Writes source code of hdl module to translate addresses from pci address space to bambu address space
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "create_address_translation.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"

/// constants includes
#include "constants.hpp"
#include "taste_constants.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// HLS include
#include "hls_manager.hpp"

/// intermediate_representation/aadl include
#include "aadl_information.hpp"
#include "asn_type.hpp"

/// tree includes
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "indented_output_stream.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

CreateAddressTranslation::CreateAddressTranslation(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, FrontendFlowStepType::CREATE_ADDRESS_TRANSLATION, _design_flow_manager, _parameters), aadl_information(GetPointer<const HLS_manager>(_AppM)->aadl_information)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CreateAddressTranslation::~CreateAddressTranslation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> CreateAddressTranslation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CREATE_TREE_MANAGER, WHOLE_APPLICATION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(design_flow_manager.lock()->GetStatus(GetSignature()) == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CREATE_TREE_MANAGER, WHOLE_APPLICATION));
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

void CreateAddressTranslation::ComputeAddress(const AsnTypeRef asn_type, const unsigned int tree_parameter_type, unsigned long long int& bambu_address, unsigned long long int& taste_address, unsigned int& registers, const bool first_level,
                                              const bool little_endianess)
{
   switch(asn_type->GetKind())
   {
      case AsnType_Kind::BOOLEAN:
      {
         if(first_level)
         {
            address_translation->Append(",0");
            memory_enabling->Append(",0");
            data_size->Append(",0");
            endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
            taste_address = taste_address + 4;
            registers++;
            break;
         }
         else
         {
            address_translation->Append("," + STR(bambu_address));
            memory_enabling->Append(",1");
            data_size->Append(",1");
            endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
            bambu_address = bambu_address + 1;
            taste_address = taste_address + 4;
            break;
         }
         break;
      }
      case AsnType_Kind::CHOICE:
      {
         THROW_ERROR("Choice ASN type not supported");
         break;
      }
      case AsnType_Kind::ENUMERATED:
      {
         if(first_level)
         {
            address_translation->Append(",0");
            memory_enabling->Append(",0");
            data_size->Append(",0");
            endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
            taste_address = taste_address + 4;
            registers++;
            break;
         }
         else
         {
            address_translation->Append("," + STR(bambu_address));
            memory_enabling->Append(",1");
            data_size->Append(",4");
            endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
            bambu_address = bambu_address + 4;
            taste_address = taste_address + 4;
            break;
         }
      }
      case AsnType_Kind::INTEGER:
      {
         if(first_level)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->INTEGER (first level)");
            address_translation->Append(",0,0");
            memory_enabling->Append(",0,0");
            data_size->Append(",0,0");
            endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
            endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
            taste_address = taste_address + 8;
            const auto byte_size = tree_helper::Size(TreeM->get_tree_node_const(tree_parameter_type)) / 8;
            if(byte_size == 8)
            {
               registers += 2;
            }
            else
            {
               registers++;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            break;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->INTEGER (not first level)");
            const auto byte_size = tree_helper::Size(TreeM->get_tree_node_const(tree_parameter_type)) / 8;
            if(byte_size <= 4)
            {
               little_endianess ? address_translation->Append(",0," + STR(bambu_address)) : address_translation->Append("," + STR(bambu_address) + ",0");
               little_endianess ? memory_enabling->Append(",0,1") : memory_enabling->Append(",1,0");
               little_endianess ? data_size->Append(",0," + STR(byte_size)) : data_size->Append("," + STR(byte_size) + ",0");
               endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
               endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
               bambu_address = bambu_address + byte_size;
               taste_address = taste_address + 8;
            }
            else if(byte_size == 8)
            {
               address_translation->Append("," + STR(bambu_address) + "," + STR(bambu_address + 4));
               memory_enabling->Append(",1,1");
               data_size->Append("," + STR(byte_size) + "," + STR(byte_size));
               endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
               endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
               bambu_address = bambu_address + byte_size;
               taste_address = taste_address + 8;
            }
            else
            {
               THROW_ERROR("Integer larger than 128 bits not supported");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         break;
      }
      case AsnType_Kind::OCTET_STRING:
      {
         const auto octet_string = GetPointer<const OctetStringAsnType>(asn_type);
         const auto byte_size = octet_string->size;
         const auto word_size = (byte_size % 4) ? ((byte_size / 4) + 1) * 4 : byte_size / 4;
         for(unsigned int word = 0; word < word_size; word++)
         {
            address_translation->Append("," + STR(bambu_address));
            memory_enabling->Append(",1");
            data_size->Append("," + STR(byte_size));
            endianess_check->Append(",0");
            bambu_address = bambu_address + 1;
            taste_address = taste_address + 1;
         }
         break;
      }
      case AsnType_Kind::REAL:
      {
         if(first_level)
         {
            const auto byte_size = tree_helper::Size(TreeM->get_tree_node_const(tree_parameter_type)) / 8;
            if(byte_size == 8)
            {
               address_translation->Append(",0,0,");
               memory_enabling->Append(",0,0");
               data_size->Append(",0,0,");
               endianess_check->Append("," + std::string(little_endianess ? "0,0" : "1,1"));
               taste_address = taste_address + 8;
               registers += 2;
            }
            else
            {
               address_translation->Append(",0");
               memory_enabling->Append(",0");
               data_size->Append(",0");
               endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
               taste_address = taste_address + 4;
               registers++;
            }
            break;
         }
         else
         {
            const auto byte_size = tree_helper::Size(TreeM->get_tree_node_const(tree_parameter_type)) / 8;
            if(byte_size == 4)
            {
               little_endianess ? address_translation->Append(",0," + STR(bambu_address)) : address_translation->Append("," + STR(bambu_address) + ",0");
               little_endianess ? memory_enabling->Append(",0,1") : memory_enabling->Append(",1,0");
               little_endianess ? data_size->Append(",0," + STR(byte_size)) : data_size->Append("," + STR(byte_size) + ",0");
               endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
               bambu_address = bambu_address + byte_size;
               taste_address = taste_address + 4;
            }
            else if(byte_size == 8)
            {
               address_translation->Append("," + STR(bambu_address) + "," + STR(bambu_address + 4));
               memory_enabling->Append(",1,1");
               data_size->Append("," + STR(byte_size) + "," + STR(byte_size));
               endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
               endianess_check->Append("," + std::string(little_endianess ? "0" : "1"));
               bambu_address = bambu_address + byte_size;
               taste_address = taste_address + 8;
            }
            else
            {
               THROW_ERROR("Unsupported real type size " + STR(byte_size));
            }
            break;
         }
      }
      case AsnType_Kind::REDEFINE:
      {
         const auto redefine = GetPointer<const RedefineAsnType>(asn_type);
         const auto redefine_asn_type = aadl_information->CGetAsnType(redefine->name);
         ComputeAddress(redefine_asn_type, tree_parameter_type, bambu_address, taste_address, registers, first_level, little_endianess);
         break;
      }
      case AsnType_Kind::SEQUENCE:
      {
         const auto sequence = GetPointer<const SequenceAsnType>(asn_type);
         const auto tree_record_type = GetPointer<const record_type>(TreeM->get_tree_node_const(tree_parameter_type));
         const auto tree_fields = tree_record_type->list_of_flds;
         size_t tree_field_index = 0;
         for(const auto& field : sequence->fields)
         {
            ComputeAddress(field.second, tree_helper::CGetType(GET_NODE(tree_fields[tree_field_index]))->index, bambu_address, taste_address, registers, false, little_endianess);
            const auto field_bpos = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(GetPointer<const field_decl>(GET_NODE(tree_fields[tree_field_index]))->bpos)));
            THROW_ASSERT(field_bpos % 8 == 0, "Bitfield not supported");
            const auto current_field_beginning = field_bpos / 8;
            const auto next_field_beginning = [&]() -> long long int {
               if(tree_field_index + 1 > tree_fields.size())
               {
                  return tree_helper::Size(TreeM->get_tree_node_const(tree_parameter_type)) / 8;
               }
               else
               {
                  return tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_NODE(GetPointer<const field_decl>(GET_NODE(tree_fields[tree_field_index + 1]))->bpos))) / 8;
               }
            }();
            const auto field_size = tree_helper::Size(GET_NODE(tree_fields[tree_field_index])) / 8;
            bambu_address = bambu_address + static_cast<unsigned int>(field_size - (next_field_beginning - current_field_beginning));
            tree_field_index++;
         }
         break;
      }
      case AsnType_Kind::SEQUENCEOF:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->SEQUENCEOF");
         const auto sequenceof = GetPointer<const SequenceOfAsnType>(asn_type);
         const auto element_type = aadl_information->CGetAsnType(sequenceof->element);
         for(size_t counter = 0; counter < sequenceof->size; counter++)
         {
            ComputeAddress(element_type, tree_helper::CGetPointedType(TreeM->get_tree_node_const(tree_parameter_type))->index, bambu_address, taste_address, registers, false, little_endianess);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         break;
      }
      case AsnType_Kind::SET:
      {
         const auto set = GetPointer<const SetAsnType>(asn_type);
         const auto tree_record_type = GetPointer<const record_type>(TreeM->get_tree_node_const(tree_parameter_type));
         const auto tree_fields = tree_record_type->list_of_flds;
         size_t tree_field_index = 0;
         for(const auto& field : set->fields)
         {
            ComputeAddress(field.second, tree_helper::CGetType(GET_NODE(tree_fields[tree_field_index]))->index, bambu_address, taste_address, registers, false, little_endianess);
            const auto field_bpos = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(GetPointer<const field_decl>(GET_NODE(tree_fields[tree_field_index]))->bpos)));
            THROW_ASSERT(field_bpos % 8 == 0, "Bitfield not supported");
            const auto current_field_beginning = field_bpos / 8;
            const auto next_field_beginning = [&]() -> long long int {
               if(tree_field_index + 1 > tree_fields.size())
               {
                  return tree_helper::Size(TreeM->get_tree_node_const(tree_parameter_type)) / 8;
               }
               else
               {
                  return tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_NODE(GetPointer<const field_decl>(GET_NODE(tree_fields[tree_field_index + 1]))->bpos))) / 8;
               }
            }();
            const auto field_size = tree_helper::Size(GET_NODE(tree_fields[tree_field_index])) / 8;
            bambu_address = bambu_address + static_cast<unsigned int>(field_size - (next_field_beginning - current_field_beginning));
            tree_field_index++;
         }
         break;
      }
      case AsnType_Kind::SETOF:
      {
         const auto setof = GetPointer<const SetOfAsnType>(asn_type);
         const auto element_type = aadl_information->CGetAsnType(setof->element);
         for(size_t counter = 0; counter < setof->size; counter++)
         {
            ComputeAddress(element_type, tree_helper::CGetPointedType(TreeM->get_tree_node_const(tree_parameter_type))->index, bambu_address, taste_address, registers, false, little_endianess);
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
}

DesignFlowStep_Status CreateAddressTranslation::Exec()
{
   already_executed = true;
   bool changed = false;
   const std::string tmp_directory = parameters->getOption<std::string>(OPT_output_temporary_directory);
   const auto top_functions = parameters->getOption<std::string>(OPT_top_functions_names);
   auto new_top_functions = top_functions;
   THROW_ASSERT(aadl_information->top_functions_names.size(), "");
   for(const auto& top_function_name : aadl_information->top_functions_names)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing function " + top_function_name);
      /// The taste address of return (if any)
      unsigned long long int taste_return_address = 0;

      THROW_ASSERT(aadl_information->function_parameters.find("PI_" + top_function_name) != aadl_information->function_parameters.end(), top_function_name);
      auto& function_parameters = aadl_information->function_parameters.find("PI_" + top_function_name)->second;
      const auto function_id = TreeM->function_index(top_function_name);
      address_translation = IndentedOutputStreamRef(new IndentedOutputStream());
      address_translation->Append("unsigned int " STR_CST_taste_address_translation + top_function_name + "(unsigned int arg)\n");
      address_translation->Append("{\n");
      address_translation->Append("const unsigned int address[] = {0");
      memory_enabling = IndentedOutputStreamRef(new IndentedOutputStream());
      memory_enabling->Append("unsigned int " STR_CST_taste_memory_enabling + top_function_name + "(unsigned int arg)\n");
      memory_enabling->Append("{\n");
      memory_enabling->Append("const unsigned int memory_enabling[] = {0");
      data_size = IndentedOutputStreamRef(new IndentedOutputStream());
      data_size->Append("unsigned int " STR_CST_taste_data_size + top_function_name + "(unsigned int arg)\n");
      data_size->Append("{\n");
      data_size->Append("const unsigned int data_size[] = {0");
      endianess_check = IndentedOutputStreamRef(new IndentedOutputStream());
      endianess_check->Append("unsigned int " STR_CST_taste_endianess_check + top_function_name + "(unsigned int arg)\n");
      endianess_check->Append("{\n");
      endianess_check->Append("const unsigned int endianess_check[] = {0");
      unsigned long long int bambu_address = 0;
      /// Taste address starts from 4, since first 4 bytes are reserved to status register
      unsigned long long int taste_address = 4;
      /// Build the map between parameter name and index type
      CustomMap<std::string, unsigned int> parameter_to_type;
      THROW_ASSERT(function_id, "Function " + top_function_name + " not found in tree");
      const auto fd = GetPointer<const function_decl>(TreeM->CGetTreeNode(function_id));
      for(const auto& arg : fd->list_of_args)
      {
         const auto pd = GetPointer<const parm_decl>(GET_NODE(arg));
         const auto id = GetPointer<const identifier_node>(GET_NODE(pd->name));
         const auto param_name = id->strg;
         parameter_to_type[param_name] = tree_helper::CGetType(GET_NODE(arg))->index;
      }
      const auto ft = GetPointer<const function_type>(GET_NODE(fd->type));
      if(ft->retn)
      {
         parameter_to_type["return"] = ft->retn->index;
      }
      bool used_return = false;
      for(auto& parameter : function_parameters)
      {
         const auto parameter_name = parameter.name;
         bambu_address = (bambu_address % 8) ? ((bambu_address / 8) + 1) * 8 : bambu_address;
         taste_address = (taste_address % 8) ? ((taste_address / 8) + 1) * 8 : taste_address;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing parameter " + parameter_name);
         const auto tree_parameter_index = [&]() -> unsigned int {
            if(parameter_to_type.find(parameter_name) != parameter_to_type.end())
            {
               return parameter_to_type.find(parameter_name)->second;
            }
            else
            {
               THROW_ASSERT(not used_return, "Parameter " + parameter_name + " not found in C function");
               THROW_ASSERT(parameter_to_type.find("return") != parameter_to_type.end(), "Parameter " + parameter_name + " not found in C function");
               used_return = true;
               taste_return_address = taste_address;
               return parameter_to_type.find("return")->second;
            }
         }();
         const auto asn_type = aadl_information->CGetAsnType(parameter.asn_type);
         /// Starting address of the parameter
         parameter.bambu_address = bambu_address;
         parameter.pointer = tree_helper::is_a_pointer(TreeM, tree_parameter_index);
         ComputeAddress(asn_type, tree_parameter_index, bambu_address, taste_address, parameter.num_registers, true, parameter.endianess);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed parameter " + parameter_name);
      }
      address_translation->Append("};\n");
      address_translation->Append("return address[arg];\n");
      address_translation->Append("}\n");
      memory_enabling->Append("};\n");
      memory_enabling->Append("return memory_enabling[arg];\n");
      memory_enabling->Append("}\n");
      data_size->Append("};\n");
      data_size->Append("return data_size[arg];\n");
      data_size->Append("}\n");
      endianess_check->Append("};\n");
      endianess_check->Append("return endianess_check[arg];\n");
      endianess_check->Append("}\n");

      const auto endianess_check_file = tmp_directory + "/" + STR_CST_taste_endianess_check + top_function_name + ".c";
      endianess_check->WriteFile(endianess_check_file);
      AppM->input_files[endianess_check_file] = endianess_check_file;
      new_top_functions += STR_CST_string_separator + STR_CST_taste_endianess_check + top_function_name;

      if(bambu_address != 0)
      {
         aadl_information->internal_memory_sizes[top_function_name] = bambu_address;
         aadl_information->exposed_memory_sizes[top_function_name] = taste_address;
         changed = true;
         const auto address_translation_file = tmp_directory + "/" + STR_CST_taste_address_translation + top_function_name + ".c";
         const auto memory_enabling_file = tmp_directory + "/" + STR_CST_taste_memory_enabling + top_function_name + ".c";
         const auto data_size_file = tmp_directory + "/" + STR_CST_taste_data_size + top_function_name + ".c";
         address_translation->WriteFile(address_translation_file);
         memory_enabling->WriteFile(memory_enabling_file);
         data_size->WriteFile(data_size_file);
         AppM->input_files[address_translation_file] = address_translation_file;
         AppM->input_files[memory_enabling_file] = memory_enabling_file;
         AppM->input_files[data_size_file] = data_size_file;
         new_top_functions +=
             STR_CST_string_separator + STR_CST_taste_address_translation + top_function_name + STR_CST_string_separator + STR_CST_taste_memory_enabling + top_function_name + STR_CST_string_separator + STR_CST_taste_data_size + top_function_name;
      }
      IndentedOutputStreamRef output_multiplexer = IndentedOutputStreamRef(new IndentedOutputStream());
      output_multiplexer->Append("unsigned int " STR_CST_taste_output_multiplexer + top_function_name + "(unsigned int address, unsigned int reg_status" + (used_return ? ", unsigned int function_return_port" : "") +
                                 (bambu_address != 0 ? ", unsigned int from_memory" : "") + ")\n");
      output_multiplexer->Append("{\n");
      output_multiplexer->Append("unsigned int ret = 0;\n");
      if(used_return)
      {
         output_multiplexer->Append("if(address == " + STR(taste_return_address) + ")\n");
         output_multiplexer->Append("{\n");
         output_multiplexer->Append("return function_return_port;\n");
         output_multiplexer->Append("}\n");
      }
      if(bambu_address != 0)
      {
         output_multiplexer->Append("if(address == 0)\n");
         output_multiplexer->Append("{\n");
         output_multiplexer->Append("return reg_status;\n");
         output_multiplexer->Append("}\n");
         output_multiplexer->Append("else\n");
         output_multiplexer->Append("{\n");
         output_multiplexer->Append("return from_memory;\n");
         output_multiplexer->Append("}\n");
      }
      else
      {
         output_multiplexer->Append("return reg_status;\n");
      }
      output_multiplexer->Append("}\n");
      const auto output_multiplexer_file = tmp_directory + "/" + STR_CST_taste_output_multiplexer + top_function_name + ".c";
      output_multiplexer->WriteFile(output_multiplexer_file);
      AppM->input_files[output_multiplexer_file] = output_multiplexer_file;
      new_top_functions += STR_CST_string_separator + STR_CST_taste_output_multiplexer + top_function_name;

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed function " + top_function_name);
   }
   IndentedOutputStreamRef reg_status(new IndentedOutputStream());
   reg_status->Append("unsigned int " STR_CST_taste_reg_status "(unsigned int current_value, unsigned int from_outside, unsigned int from_done)\n");
   reg_status->Append("{\n");
   reg_status->Append("unsigned int ret = 0;\n");
   /// Storing starting coming from outside
   reg_status->Append("unsigned int bit0 = from_outside & 1;\n");
   reg_status->Append("ret = ret | bit0 ;\n");
   /// We are running if we receive the start and we were not running or if we were running and we didn't receive done
   reg_status->Append("unsigned int bit1 = (((bit0 != 0) && (current_value & 2 == 0)) || (((current_value & 2) != 0) && (from_done == 0))) ? 2 : 0;\n");
   reg_status->Append("ret = ret | bit1;\n");
   /// We ended if (we ended in the past or we just ended) and we were not starting
   reg_status->Append("unsigned int bit2 = (((current_value & 4) || (from_done)) && !((bit0) && !(current_value & 2))) ? 4 : 0;\n");
   reg_status->Append("ret = ret | bit2;\n");
   /// We are starting if we start and we are not running
   reg_status->Append("unsigned int bit3 = ((bit0) && (current_value & 2)) ? 8 : 0;\n");
   reg_status->Append("ret = ret | bit3;\n");
   reg_status->Append("return ret;\n");
   reg_status->Append("}\n");
   const auto reg_status_file = tmp_directory + "/" + STR_CST_taste_reg_status + ".c";
   reg_status->WriteFile(reg_status_file);
   AppM->input_files[reg_status_file] = reg_status_file;
   new_top_functions += STR_CST_string_separator + STR_CST_taste_reg_status;

#if 0
   IndentedOutputStreamRef endianess_inversion(new IndentedOutputStream());
   endianess_inversion->Append("unsigned int " STR_CST_taste_endianess_inversion " (unsigned int input, unsigned int endianess)\n");
   endianess_inversion->Append("{\n");
   endianess_inversion->Append("const int inverted = bswap32(input);\n");
   endianess_inversion->Append("return endianess == 1 ? inverted : input;\n");
   endianess_inversion->Append("}\n");
   const auto endianess_inversion_file = tmp_directory + "/" + STR_CST_taste_endianess_inversion + ".c";
   endianess_inversion->WriteFile(endianess_inversion_file);
   AppM->input_files[endianess_inversion_file] = endianess_inversion_file;
#endif
   new_top_functions += STR_CST_string_separator + STR_CST_taste_endianess_inversion;

   const_cast<Parameter*>(parameters.get())->setOption(OPT_top_functions_names, new_top_functions);
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void CreateAddressTranslation::Initialize()
{
   ApplicationFrontendFlowStep::Initialize();
   TreeM = AppM->get_tree_manager();
}

bool CreateAddressTranslation::HasToBeExecuted() const
{
   return not already_executed;
}
