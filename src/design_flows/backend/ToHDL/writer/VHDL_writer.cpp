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
 * @file VHDL_writer.cpp
 * @brief This class implements the methods to write VHDL descriptions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Autoheader include
#include "config_HAVE_FROM_C_BUILT.hpp"

#include "VHDL_writer.hpp"

#include "HDL_manager.hpp"

#include "NP_functionality.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "structural_objects.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <functional>
#include <iosfwd>
#include <vector>

///. include
#include "Parameter.hpp"

/// HLS/stg include
#include "state_transition_graph_manager.hpp"

/// STL include
#include <utility>

/// technology/physical_library include
#include "technology_node.hpp"

/// utility include
#include "indented_output_stream.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

const char* VHDL_writer::tokenNames[] = {"abs",           "access",   "after",      "alias",  "all",      "and",      "architecture", "array",   "assert",  "attribute", "begin",     "block",    "body",     "buffer",  "bus",    "case",      "component",
                                         "configuration", "constant", "disconnect", "downto", "else",     "elsif",    "end",          "entity",  "exit",    "file",      "for",       "function", "generate", "generic", "group",  "guarded",   "if",
                                         "impure",        "in",       "inertial",   "inout",  "is",       "label",    "library",      "linkage", "literal", "loop",      "map",       "mod",      "nand",     "new",     "next",   "nor",       "not",
                                         "null",          "of",       "on",         "open",   "or",       "others",   "out",          "package", "port",    "postponed", "procedure", "process",  "pure",     "range",   "record", "register",  "reject",
                                         "rem",           "return",   "rol",        "ror",    "select",   "severity", "signal",       "shared",  "sla",     "sli",       "sra",       "srl",      "subtype",  "then",    "to",     "transport", "type",
                                         "unaffected",    "units",    "until",      "use",    "variable", "wait",     "when",         "while",   "with",    "xnor",      "xor"};

VHDL_writer::VHDL_writer(const technology_managerConstRef _TM, const ParameterConstRef _parameters) : language_writer(STD_OPENING_CHAR, STD_OPENING_CHAR, _parameters), TM(_TM)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   for(auto& tokenName : tokenNames)
      keywords.insert(boost::to_upper_copy<std::string>(tokenName));
}

VHDL_writer::~VHDL_writer() = default;

void VHDL_writer::write_comment(const std::string& comment_string)
{
   indented_output_stream->Append("-- " + comment_string);
}

std::string VHDL_writer::type_converter(structural_type_descriptorRef Type)
{
   switch(Type->type)
   {
      case structural_type_descriptor::BOOL:
      {
         return "std_logic";
      }
      case structural_type_descriptor::INT:
      {
         return "signed";
      }
      case structural_type_descriptor::UINT:
      {
         return "unsigned";
      }
      case structural_type_descriptor::REAL:
      {
         return "std_logic_vector";
      }
      case structural_type_descriptor::USER:
      {
         THROW_ERROR("USER type not yet supported");
         break;
      }
      case structural_type_descriptor::VECTOR_BOOL:
      {
         return "std_logic_vector";
      }
      case structural_type_descriptor::VECTOR_INT:
      case structural_type_descriptor::VECTOR_UINT:
      case structural_type_descriptor::VECTOR_REAL:
      {
         return "std_logic_vector";
      }
      case structural_type_descriptor::VECTOR_USER:
      {
         THROW_ERROR("VECTOR_USER type not yet supported");
         break;
      }
      case structural_type_descriptor::OTHER:
      {
         return Type->id_type;
      }
      case structural_type_descriptor::UNKNOWN:
      default:
         THROW_UNREACHABLE("");
   }
   return "";
}

std::string VHDL_writer::type_converter_size(const structural_objectRef& cir)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---type_converter_size of " + cir->get_path());
   structural_type_descriptorRef Type = cir->get_typeRef();
   const structural_objectRef Owner = cir->get_owner();
   auto* mod = GetPointer<module>(Owner);
   std::string port_name = cir->get_id();
   bool specialization_string = false;
   // std::cerr << "cir: " << cir->get_id() << " " << GetPointer<port_o>(cir)->size_parameter << std::endl;
   if(mod)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---owner is " + cir->get_path());
      const NP_functionalityRef& np = mod->get_NP_functionality();
      if(np)
      {
         std::vector<std::pair<std::string, structural_objectRef>> library_parameters;
         mod->get_NP_library_parameters(Owner, library_parameters);
         for(const auto& library_parameter : library_parameters)
            if(port_name == library_parameter.first)
               specialization_string = true;
      }
   }
   switch(Type->type)
   {
      case structural_type_descriptor::BOOL:
      {
         if(cir->get_kind() == port_vector_o_K)
         {
            if(specialization_string)
            {
               return "((" + (PORTSIZE_PREFIX + port_name) + "*" + (BITSIZE_PREFIX + port_name) + ")-1 downto 0) ";
            }
            else
            {
               const auto& port_vector = GetPointer<port_o>(cir);
               return " (" + STR(port_vector->get_ports_size() - 1) + " downto 0)";
            }
         }
         else
         {
            return "";
         }
      }
      case structural_type_descriptor::INT:
      case structural_type_descriptor::UINT:
      case structural_type_descriptor::REAL:
      {
         if(!GetPointer<port_o>(cir) or !GetPointer<port_o>(cir)->get_reverse())
         {
            if(GetPointer<port_o>(cir) && GetPointer<port_o>(cir)->size_parameter.size())
               return " (" + (GetPointer<port_o>(cir)->size_parameter) + "-1 downto 0)";
            else if(specialization_string)
               return " (" + (HDL_manager::convert_to_identifier(this, BITSIZE_PREFIX + port_name)) + "-1 downto 0)";
            else
               return " (" + boost::lexical_cast<std::string>(Type->size - 1) + " downto 0)";
         }
         else
         {
            if(GetPointer<port_o>(cir) && GetPointer<port_o>(cir)->size_parameter.size())
               return " (0 to " + (GetPointer<port_o>(cir)->size_parameter) + "-1)";
            else if(specialization_string)
               return " (0 to " + (HDL_manager::convert_to_identifier(this, BITSIZE_PREFIX + port_name)) + "-1)";
            else
               return " (0 to " + boost::lexical_cast<std::string>(Type->size - 1) + ")";
         }
      }
      case structural_type_descriptor::USER:
      {
         THROW_ERROR("USER type not yet supported");
         break;
      }
      case structural_type_descriptor::VECTOR_BOOL:
      {
         if(specialization_string)
         {
            if(cir->get_kind() == port_vector_o_K)
            {
               unsigned int lsb = GetPointer<port_o>(cir)->get_lsb();
               return "((" + (PORTSIZE_PREFIX + port_name) + "*" + (BITSIZE_PREFIX + port_name) + ")+(" + boost::lexical_cast<std::string>(static_cast<int>(lsb) - 1) + ") downto " + boost::lexical_cast<std::string>(lsb) + ") ";
            }
            else
               return "(" + (BITSIZE_PREFIX + port_name) + "-1 downto 0) ";
         }
         else
         {
            if(cir->get_kind() == signal_vector_o_K)
            {
               const structural_objectRef first_sig = GetPointer<signal_o>(cir)->get_signal(0);
               structural_type_descriptorRef Type_fs = first_sig->get_typeRef();
               unsigned int n_sign = GetPointer<signal_o>(cir)->get_signals_size();
               unsigned int size_fs = Type_fs->vector_size > 0 ? Type_fs->size * Type_fs->vector_size : Type_fs->size;
               unsigned int lsb = GetPointer<signal_o>(cir)->get_lsb();
               unsigned int msb = size_fs * n_sign + lsb;

               return "(" + boost::lexical_cast<std::string>(static_cast<int>(msb) - 1) + " downto " + boost::lexical_cast<std::string>(lsb) + ") ";
            }
            else if(cir->get_kind() == port_vector_o_K)
            {
               unsigned int lsb = GetPointer<port_o>(cir)->get_lsb();
               unsigned int n_ports = GetPointer<port_o>(cir)->get_ports_size();
               const structural_objectRef first_port = GetPointer<port_o>(cir)->get_port(0);
               const auto Type_fp = first_port->get_typeRef();
               unsigned int size_fp = Type_fp->vector_size > 0 ? Type_fp->size * Type_fp->vector_size : Type_fp->size;
               unsigned int msb = size_fp * n_ports + lsb;
               return "(" + boost::lexical_cast<std::string>(static_cast<int>(msb) - 1) + " downto " + boost::lexical_cast<std::string>(lsb) + ") ";
            }
            if(Type->vector_size > 1 && Type->size == 1)
               return "(" + boost::lexical_cast<std::string>(static_cast<int>(Type->vector_size) - 1) + " downto 0) ";
            else if(Type->vector_size == 1 && Type->size == 1)
               return "(0 downto 0)";
            else if(Type->vector_size == 0 && Type->size != 0)
               return "(" + boost::lexical_cast<std::string>(static_cast<int>(Type->size) - 1) + " downto 0) ";
            else
               THROW_ERROR("Not completely specified: " + port_name);
         }
         break;
      }
      case structural_type_descriptor::VECTOR_INT:
      case structural_type_descriptor::VECTOR_UINT:
      case structural_type_descriptor::VECTOR_REAL:
      {
         if(GetPointer<port_o>(cir) and specialization_string)
         {
            return " (" + HDL_manager::convert_to_identifier(this, BITSIZE_PREFIX + port_name) + "*" + HDL_manager::convert_to_identifier(this, NUM_ELEM_PREFIX + port_name) + "-1 downto 0)";
         }
         else
         {
            return "(" + STR(GET_TYPE_SIZE(cir) - 1) + " downto 0)";
         }
      }
      case structural_type_descriptor::VECTOR_USER:
      {
         THROW_UNREACHABLE("");
         break;
      }
      case structural_type_descriptor::OTHER:
      {
         return Type->id_type;
      }
      case structural_type_descriptor::UNKNOWN:
      default:
         THROW_ERROR("Not initialized type");
   }
   return "";
}

std::string VHDL_writer::may_slice_string(const structural_objectRef& cir)
{
   structural_type_descriptorRef Type = cir->get_typeRef();
   const structural_objectRef Owner = cir->get_owner();
   auto* mod = GetPointer<module>(Owner);
   std::string port_name = cir->get_id();
   bool specialization_string = false;
   if(mod)
   {
      const NP_functionalityRef& np = mod->get_NP_functionality();
      if(np)
      {
         std::vector<std::pair<std::string, structural_objectRef>> library_parameters;
         mod->get_NP_library_parameters(Owner, library_parameters);
         for(const auto& library_parameter : library_parameters)
            if(port_name == library_parameter.first)
               specialization_string = true;
      }
   }
   switch(Type->type)
   {
      case structural_type_descriptor::BOOL:
      {
         if(Owner->get_kind() == port_vector_o_K)
         {
            return "(" + GetPointer<port_o>(cir)->get_id() + ")";
         }
         else
            return "";
      }
      case structural_type_descriptor::USER:
      {
         Type->print(std::cerr);
         THROW_ERROR("USER type not yet supported");
         break;
      }
      case structural_type_descriptor::INT:
      case structural_type_descriptor::UINT:
      case structural_type_descriptor::REAL:
      case structural_type_descriptor::VECTOR_BOOL:
      {
         if(specialization_string)
         {
            if(Owner->get_kind() == port_vector_o_K)
            {
               unsigned int lsb = GetPointer<port_o>(Owner)->get_lsb();
               return "(((" + boost::lexical_cast<std::string>(GetPointer<port_o>(cir)->get_id()) + "+1)*" + (BITSIZE_PREFIX + port_name) + ")+(" + boost::lexical_cast<std::string>(static_cast<int>(lsb) - 1) + ") downto (" +
                      boost::lexical_cast<std::string>(GetPointer<port_o>(cir)->get_id()) + "*" + (BITSIZE_PREFIX + port_name) + ")+" + boost::lexical_cast<std::string>(lsb) + ")";
            }
            else
               return "";
         }
         else
         {
            if(Owner->get_kind() == port_vector_o_K)
            {
               structural_type_descriptorRef Type_fp = cir->get_typeRef();
               unsigned int size_fp = Type_fp->vector_size > 0 ? Type_fp->size * Type_fp->vector_size : Type_fp->size;
               unsigned int lsb = GetPointer<port_o>(Owner)->get_lsb();
               return "(" + boost::lexical_cast<std::string>((1 + boost::lexical_cast<int>(GetPointer<port_o>(cir)->get_id())) * static_cast<int>(size_fp) + static_cast<int>(lsb) - 1) + " downto " +
                      boost::lexical_cast<std::string>((boost::lexical_cast<int>(GetPointer<port_o>(cir)->get_id())) * static_cast<int>(size_fp) + static_cast<int>(lsb)) + ")";
            }
            else
               return "";
         }
         break;
      }
      case structural_type_descriptor::VECTOR_UINT:
      case structural_type_descriptor::VECTOR_INT:
      case structural_type_descriptor::VECTOR_REAL:
      {
         if(specialization_string)
         {
            if(Owner->get_kind() == port_vector_o_K)
            {
               unsigned int lsb = GetPointer<port_o>(Owner)->get_lsb();
               return "(((" + boost::lexical_cast<std::string>(GetPointer<port_o>(cir)->get_id()) + "+1)*" + (BITSIZE_PREFIX + port_name) + "*" + (NUM_ELEM_PREFIX + port_name) + ")+(" + boost::lexical_cast<std::string>(static_cast<int>(lsb) - 1) +
                      ") downto (" + boost::lexical_cast<std::string>(GetPointer<port_o>(cir)->get_id()) + "*" + (BITSIZE_PREFIX + port_name) + "*" + (NUM_ELEM_PREFIX + port_name) + ")+" + boost::lexical_cast<std::string>(lsb) + ")";
            }
            else
               return "";
         }
         else
         {
            if(Owner->get_kind() == port_vector_o_K)
            {
               structural_type_descriptorRef Type_fp = cir->get_typeRef();
               unsigned int size_fp = Type_fp->vector_size > 0 ? Type_fp->size * Type_fp->vector_size : Type_fp->size;
               unsigned int lsb = GetPointer<port_o>(Owner)->get_lsb();
               return "(" + boost::lexical_cast<std::string>((1 + boost::lexical_cast<int>(GetPointer<port_o>(cir)->get_id())) * static_cast<int>(size_fp) + static_cast<int>(lsb) - 1) + " downto " +
                      boost::lexical_cast<std::string>((boost::lexical_cast<int>(GetPointer<port_o>(cir)->get_id())) * static_cast<int>(size_fp) + static_cast<int>(lsb)) + ")";
            }
            else
               return "";
         }
         break;
      }
      case structural_type_descriptor::OTHER:
      case structural_type_descriptor::VECTOR_USER:
      {
         THROW_ERROR("VECTOR_USER type not yet supported");
         break;
      }
      case structural_type_descriptor::UNKNOWN:
      default:
         THROW_ERROR("Not initialized type");
   }
   return "";
}

void VHDL_writer::write_library_declaration(const structural_objectRef& cir)
{
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   NP_functionalityRef NPF = GetPointer<module>(cir)->get_NP_functionality();
   if(!NPF or !NPF->exist_NP_functionality(NP_functionality::IP_LIBRARY))
   {
      indented_output_stream->Append("library IEEE;\n");
      indented_output_stream->Append("use IEEE.std_logic_1164.all;\n");
      indented_output_stream->Append("use IEEE.numeric_std.all;\n");
      indented_output_stream->Append("use IEEE.math_real.all;\n");
      indented_output_stream->Append("use STD.textio.all;\n");
      indented_output_stream->Append("use IEEE.std_logic_textio.all;\n");
      indented_output_stream->Append("-- synthesis translate_off\n");
      indented_output_stream->Append("use STD.env.all;\n");
      indented_output_stream->Append("-- synthesis translate_on\n");
      indented_output_stream->Append("use work.panda_pkg.all;\n");
      return;
   }
   std::string library = NPF->get_NP_functionality(NP_functionality::IP_LIBRARY);
   std::vector<std::string> library_list = convert_string_to_vector<std::string>(library, ";");
   for(const auto& l : library_list)
   {
      indented_output_stream->Append(l + ";\n");
   }
}

void VHDL_writer::write_module_declaration(const structural_objectRef& cir)
{
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   indented_output_stream->Append("entity " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)) + " is \n");
   list_of_comp_already_def.clear();
}

void VHDL_writer::write_module_internal_declaration(const structural_objectRef& cir)
{
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   indented_output_stream->Append("end " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)) + ";\n");
   indented_output_stream->Append("\narchitecture " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir) + "_arch") + " of ");
   indented_output_stream->Append(HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)) + " is\n");
   indented_output_stream->Indent();
}

void VHDL_writer::write_port_declaration(const structural_objectRef& cir, bool last_port_to_analyze)
{
   THROW_ASSERT(cir->get_kind() == port_o_K || cir->get_kind() == port_vector_o_K, "Expected a port got something of different " + cir->get_id());
   port_o::port_direction dir;
   dir = GetPointer<port_o>(cir)->get_port_direction();
   indented_output_stream->Append(HDL_manager::convert_to_identifier(this, cir->get_id()) + " : ");
   switch(dir)
   {
      case port_o::IN:
      {
         indented_output_stream->Append("in");
         break;
      }
      case port_o::OUT:
      {
         indented_output_stream->Append("out");
         break;
      }
      case port_o::IO:
      {
         indented_output_stream->Append("inout");
         break;
      }
      case port_o::GEN:
      case port_o::TLM_IN:
      case port_o::TLM_INOUT:
      case port_o::TLM_OUT:
      case port_o::UNKNOWN:
      default:
         THROW_ERROR("Something went wrong!");
   }
   indented_output_stream->Append(" " + (cir->get_kind() == port_o_K ? type_converter(cir->get_typeRef()) : "std_logic_vector") + type_converter_size(cir));
   if(not last_port_to_analyze)
      indented_output_stream->Append(";");
   indented_output_stream->Append("\n");
}

void VHDL_writer::write_component_declaration(const structural_objectRef& cir)
{
   auto* mod = GetPointer<module>(cir);
   THROW_ASSERT(mod, "Expected a module got a " + cir->get_kind_text() + ": " + cir->get_path());

   const std::string comp = HDL_manager::get_mod_typename(this, cir);

   if(list_of_comp_already_def.find(comp) == list_of_comp_already_def.end())
      list_of_comp_already_def.insert(comp);
   else
      return;

   indented_output_stream->Append("\ncomponent " + comp + "\n");
   write_module_parametrization_decl(cir);
   indented_output_stream->Append("port (\n");
   indented_output_stream->Indent();
   if(mod->get_in_port_size())
   {
      write_comment("IN\n");
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         if(i == mod->get_in_port_size() - 1 && !mod->get_out_port_size() && !mod->get_in_out_port_size() && !mod->get_gen_port_size())
            write_port_declaration(mod->get_in_port(i), true);
         else
            write_port_declaration(mod->get_in_port(i), false);
      }
   }
   if(mod->get_out_port_size())
   {
      write_comment("OUT\n");
      for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         if(i == mod->get_out_port_size() - 1 && !mod->get_in_out_port_size() && !mod->get_gen_port_size())
            write_port_declaration(mod->get_out_port(i), true);
         else
            write_port_declaration(mod->get_out_port(i), false);
      }
   }
   if(mod->get_in_out_port_size())
   {
      write_comment("INOUT\n");
      for(unsigned int i = 0; i < mod->get_in_out_port_size(); i++)
      {
         if(i == mod->get_in_out_port_size() - 1 && !mod->get_gen_port_size())
            write_port_declaration(mod->get_in_out_port(i), true);
         else
            write_port_declaration(mod->get_in_out_port(i), false);
      }
   }
   if(mod->get_gen_port_size())
   {
      write_comment("Ports\n");
      for(unsigned int i = 0; i < mod->get_gen_port_size(); i++)
      {
         if(i == mod->get_gen_port_size() - 1)
            write_port_declaration(mod->get_gen_port(i), true);
         else
            write_port_declaration(mod->get_gen_port(i), false);
      }
   }
   indented_output_stream->Deindent();

   indented_output_stream->Append("\n);\nend component;\n");
}

void VHDL_writer::write_signal_declaration(const structural_objectRef& cir)
{
   if(cir->get_kind() == signal_o_K)
   {
      indented_output_stream->Append("signal " + HDL_manager::convert_to_identifier(this, cir->get_id()) + " : " + type_converter(cir->get_typeRef()) + type_converter_size(cir) + ";\n");
   }
   else if(cir->get_kind() == signal_vector_o_K and (cir->get_typeRef()->type == structural_type_descriptor::BOOL or cir->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL))
   {
      const structural_objectRef first_sig = GetPointer<signal_o>(cir)->get_signal(0);
      structural_type_descriptorRef Type_fs = first_sig->get_typeRef();
      unsigned int n_sign = GetPointer<signal_o>(cir)->get_signals_size();
      unsigned int size_fs = Type_fs->vector_size > 0 ? Type_fs->size * Type_fs->vector_size : Type_fs->size;
      unsigned int lsb = GetPointer<signal_o>(cir)->get_lsb();
      unsigned int msb = size_fs * n_sign + lsb;
      indented_output_stream->Append("signal " + HDL_manager::convert_to_identifier(this, cir->get_id()) + " : std_logic_vector (" + STR(msb - 1) + " downto " + STR(lsb) + ");\n");
   }
   else
   {
      THROW_UNREACHABLE("");
   }
}

void VHDL_writer::write_module_definition_begin(const structural_objectRef&)
{
   indented_output_stream->Deindent();
   indented_output_stream->Append("begin\n");
   indented_output_stream->Indent();
}

void VHDL_writer::write_module_instance_begin(const structural_objectRef& cir, const std::string& module_name, bool write_parametrization)
{
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   indented_output_stream->Append(HDL_manager::convert_to_identifier(this, cir->get_id()) + " : " + module_name);
   // check possible module parametrization
   if(write_parametrization)
      write_module_parametrization(cir);
   indented_output_stream->Append(" port map (");
}

void VHDL_writer::write_module_instance_end(const structural_objectRef&)
{
   indented_output_stream->Append(");\n");
}

void VHDL_writer::write_module_definition_end(const structural_objectRef& cir)
{
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   indented_output_stream->Deindent();
   indented_output_stream->Append("\nend " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir) + "_arch") + ";\n\n");
}

void VHDL_writer::write_vector_port_binding(const structural_objectRef& port, bool& first_port_analyzed)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing vector port binding of " + port->get_id());
   THROW_ASSERT(port, "NULL object_bounded received");
   THROW_ASSERT(port->get_kind() == port_vector_o_K, "Expected a port vector, got something of different");
   if(first_port_analyzed)
      indented_output_stream->Append(", ");
   const structural_objectRef p_object_bounded = GetPointer<port_o>(port)->find_bounded_object(port->get_owner());
   if(p_object_bounded)
   {
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, port->get_id()));
      indented_output_stream->Append(" => ");
      if(p_object_bounded->get_kind() == port_vector_o_K)
      {
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, p_object_bounded->get_id()));
      }
      else if(p_object_bounded->get_kind() == signal_vector_o_K)
      {
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, p_object_bounded->get_id()));
      }
      else
         THROW_UNREACHABLE("not expected case");
   }
   else
   {
      const auto port_vector = GetPointer<port_o>(port);
      const auto vector_port_size = GET_TYPE_SIZE(port) * port_vector->get_ports_size();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Vector port size is " + STR(vector_port_size));
      auto msb = vector_port_size - 1;
      bool open = true;
      for(unsigned int local_port_index = 0; local_port_index < port_vector->get_ports_size(); local_port_index++)
      {
         if(GetPointer<port_o>(port_vector->get_port(local_port_index))->find_bounded_object())
         {
            open = false;
         }
      }
      if(open)
      {
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, port->get_id()));
         indented_output_stream->Append(" => open");
      }
      else
      {
         for(unsigned int local_port_index = 0; local_port_index < port_vector->get_ports_size(); local_port_index++)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering port " + STR(local_port_index));
            if(local_port_index != 0)
               indented_output_stream->Append(", ");
            indented_output_stream->Append(port->get_id());
            unsigned int reverse_port_index = port_vector->get_ports_size() - local_port_index - 1;
            const auto object_bounded = GetPointer<port_o>(port_vector->get_port(reverse_port_index))->find_bounded_object();
            THROW_ASSERT(object_bounded, port_vector->get_port(reverse_port_index)->get_path());
            const auto single_port_size = GET_TYPE_SIZE(object_bounded);
            if(single_port_size == 1 and object_bounded->get_typeRef()->type == structural_type_descriptor::BOOL)
            {
               indented_output_stream->Append("(" + STR(msb) + ")");
            }
            else
            {
               indented_output_stream->Append("(" + STR(msb) + " downto " + STR(msb - single_port_size + 1) + ")");
            }
            msb = msb - single_port_size;
            THROW_ASSERT(msb <= vector_port_size or msb + 1 == 0, STR(msb) + " of " + port->get_id());
            indented_output_stream->Append(" => ");
            if(object_bounded->get_kind() == port_o_K or object_bounded->get_kind() == port_vector_o_K or object_bounded->get_kind() == signal_o_K or object_bounded->get_kind() == signal_vector_o_K)
            {
               if(object_bounded->get_typeRef()->type == structural_type_descriptor::BOOL or object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
               {
                  indented_output_stream->Append(HDL_manager::convert_to_identifier(this, object_bounded->get_id()));
               }
               else
               {
                  indented_output_stream->Append("std_logic_vector(" + HDL_manager::convert_to_identifier(this, object_bounded->get_id()) + ")");
               }
            }
            else if(object_bounded->get_kind() == constant_o_K)
            {
               auto* con = GetPointer<constant_o>(object_bounded);
               std::string trimmed_value = "";
               auto long_value = boost::lexical_cast<unsigned long long int>(con->get_value());
               for(unsigned int ind = 0; ind < GET_TYPE_SIZE(con); ind++)
                  trimmed_value = trimmed_value + (((1LLU << (GET_TYPE_SIZE(con) - ind - 1)) & long_value) ? '1' : '0');
               if(single_port_size == 1 and object_bounded->get_typeRef()->type == structural_type_descriptor::BOOL)
                  indented_output_stream->Append("'" + trimmed_value + "'");
               else
                  indented_output_stream->Append("\"" + trimmed_value + "\"");
            }
            else
            {
               THROW_UNREACHABLE("");
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written vector port binding of " + port->get_id());
}

void VHDL_writer::write_port_binding(const structural_objectRef& port, const structural_objectRef& object_bounded, bool& first_port_analyzed)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Write port binding " + port->get_id() + " (" + port->get_typeRef()->get_name() + ") => " + (object_bounded ? object_bounded->get_id() + " (" + object_bounded->get_typeRef()->get_name() + ")" : ""));
   THROW_ASSERT(port, "NULL object_bounded received");
   THROW_ASSERT(port->get_kind() == port_o_K, "Expected a port got something of different");
   THROW_ASSERT(port->get_owner(), "The port has to have an owner");
   if(first_port_analyzed)
      indented_output_stream->Append(", ");
   if(port->get_owner()->get_kind() == port_vector_o_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Vector port");
      const auto port_name = HDL_manager::convert_to_identifier(this, port->get_owner()->get_id());
      if(object_bounded->get_typeRef()->type == structural_type_descriptor::BOOL)
      {
         indented_output_stream->Append(port_name + "(" + STR(port->get_id()) + ") => ");
      }
      else
      {
         indented_output_stream->Append(port_name + "(" + STR(((boost::lexical_cast<unsigned int>(port->get_id()) + 1) * GET_TYPE_SIZE(port)) - 1) + " downto " + STR(boost::lexical_cast<unsigned int>(port->get_id()) * GET_TYPE_SIZE(port)) + ") => ");
      }
   }
   else if((port->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL or port->get_typeRef()->type == structural_type_descriptor::UINT) and object_bounded->get_typeRef()->type == structural_type_descriptor::BOOL)
   {
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, port->get_id()) + "(0) => ");
   }
   else
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, port->get_id()) + " => ");

   first_port_analyzed = true;
   if(!object_bounded and GetPointer<port_o>(port)->get_port_direction() == port_o::IN)
   {
      long long int size = GET_TYPE_SIZE(port);
      if(port->get_typeRef()->type == structural_type_descriptor::BOOL)
      {
         indented_output_stream->Append("'0'");
      }
      else
      {
         std::string null;
         for(unsigned int s = 0; s < size; s++)
            null += "0";
         indented_output_stream->Append("\"" + null + "\"");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written port binding " + port->get_id() + " => " + (object_bounded ? object_bounded->get_path() : ""));
      return;
   }
   THROW_ASSERT(object_bounded, "NULL object_bounded received");
   THROW_ASSERT(object_bounded->get_kind() != port_o_K || object_bounded->get_owner(), "A port has to have always an owner");
   if(object_bounded->get_kind() == constant_o_K)
   {
      auto* con = GetPointer<constant_o>(object_bounded);
      std::string trimmed_value = "";
      auto long_value = boost::lexical_cast<unsigned long long int>(con->get_value());
      for(unsigned int ind = 0; ind < GET_TYPE_SIZE(con); ind++)
         trimmed_value = trimmed_value + (((1LLU << (GET_TYPE_SIZE(con) - ind - 1)) & long_value) ? '1' : '0');
      if(port->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
      {
         indented_output_stream->Append("\"" + trimmed_value + "\"");
      }
      else if(port->get_typeRef()->type == structural_type_descriptor::BOOL)
      {
         if(port->get_owner()->get_kind() == port_vector_o_K)
         {
            indented_output_stream->Append("\"" + trimmed_value + "\"");
         }
         else
         {
            indented_output_stream->Append("'" + trimmed_value + "'");
         }
      }
      else if(port->get_typeRef()->type == structural_type_descriptor::UINT or port->get_typeRef()->type == structural_type_descriptor::INT or port->get_typeRef()->type == structural_type_descriptor::REAL or
              port->get_typeRef()->type == structural_type_descriptor::VECTOR_INT or port->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT)
      {
         indented_output_stream->Append("\"" + trimmed_value + "\"");
      }
      else
      {
         THROW_UNREACHABLE(STR(port->get_typeRef()->get_name()));
      }
   }
   else if(port->get_typeRef()->type == object_bounded->get_typeRef()->type or object_bounded->get_typeRef()->type == structural_type_descriptor::BOOL)
   {
      if(object_bounded->get_kind() == port_o_K && object_bounded->get_owner()->get_kind() == port_vector_o_K)
      {
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, object_bounded->get_owner()->get_id()) + may_slice_string(object_bounded));
      }
      else
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, object_bounded->get_id()));
   }
   else
   {
      THROW_ASSERT(GetPointer<port_o>(port)->get_port_direction() == port_o::IN or ((port->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT) or
                                                                                    (port->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_INT) or
                                                                                    (port->get_typeRef()->type == structural_type_descriptor::VECTOR_INT and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL) or
                                                                                    (port->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)),
                   "Needed a conversion on output port binding " + port->get_path() + " => " + object_bounded->get_path() + " - Types are " + port->get_typeRef()->get_name() + " vs. " + object_bounded->get_typeRef()->get_name());
      if(port->get_typeRef()->type == structural_type_descriptor::BOOL and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and object_bounded->get_typeRef()->size == 1)
      {
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, object_bounded->get_id()) + "(0)");
      }
      else if((port->get_typeRef()->type == structural_type_descriptor::INT and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL) or
              (port->get_typeRef()->type == structural_type_descriptor::INT and object_bounded->get_typeRef()->type == structural_type_descriptor::UINT) or
              (port->get_typeRef()->type == structural_type_descriptor::UINT and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL) or
              (port->get_typeRef()->type == structural_type_descriptor::REAL and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL) or
              (port->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and object_bounded->get_typeRef()->type == structural_type_descriptor::UINT) or
              (port->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and object_bounded->get_typeRef()->type == structural_type_descriptor::INT) or
              (port->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and object_bounded->get_typeRef()->type == structural_type_descriptor::REAL)
              /// This conversion is needed since vector of bool are mapped on VECTOR_UINT
              or (port->get_typeRef()->type == structural_type_descriptor::VECTOR_INT and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT))
      {
         indented_output_stream->Append(type_converter(port->get_typeRef()) + "(" + (HDL_manager::convert_to_identifier(this, object_bounded->get_id())) + ")");
      }
      else if((port->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT) or
              (port->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_INT) or
              (port->get_typeRef()->type == structural_type_descriptor::VECTOR_INT and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL) or
              (port->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT and object_bounded->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL))
      {
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, object_bounded->get_id()));
      }
      else if(port->get_typeRef()->type == structural_type_descriptor::BOOL and (object_bounded->get_typeRef()->type == structural_type_descriptor::UINT and object_bounded->get_typeRef()->size == 1))
      {
         indented_output_stream->Append(type_converter(port->get_typeRef()) + "(" + (HDL_manager::convert_to_identifier(this, object_bounded->get_id())) + "(0))");
      }
      else
      {
         THROW_UNREACHABLE("Conversion required on port binding: " + port->get_path() + "(" + port->get_typeRef()->get_name() + ") (" + object_bounded->get_id() + "(" + object_bounded->get_typeRef()->get_name() + ")");
      }
   }
   first_port_analyzed = true;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written port binding " + port->get_id() + " => " + (object_bounded ? object_bounded->get_id() : ""));
}

void VHDL_writer::write_io_signal_post_fix(const structural_objectRef& port, const structural_objectRef& sig)
{
   THROW_ASSERT(port && port->get_kind() == port_o_K, "Expected a port got something of different");
   THROW_ASSERT(port->get_owner(), "Expected a port with an owner");
   THROW_ASSERT(sig && (sig->get_kind() == signal_o_K || sig->get_kind() == constant_o_K), "Expected a signal or a constant, got something of different");
   std::string port_string;
   std::string signal_string;
   if(sig->get_kind() == constant_o_K)
   {
      auto* con = GetPointer<constant_o>(sig);
      std::string trimmed_value = "";
      auto long_value = boost::lexical_cast<unsigned long long int>(con->get_value());
      for(unsigned int ind = 0; ind < GET_TYPE_SIZE(con); ind++)
         trimmed_value = trimmed_value + (((1LLU << (GET_TYPE_SIZE(con) - ind - 1)) & long_value) ? '1' : '0');
      signal_string = "\"" + trimmed_value + "\"";
   }
   else if(sig->get_kind() == signal_o_K)
   {
      if(sig->get_owner()->get_kind() == signal_vector_o_K)
         signal_string = HDL_manager::convert_to_identifier(this, sig->get_owner()->get_id()) + may_slice_string(port);
      else
         signal_string = HDL_manager::convert_to_identifier(this, sig->get_id());
   }
   if(port->get_owner()->get_kind() == port_vector_o_K)
      port_string = HDL_manager::convert_to_identifier(this, port->get_owner()->get_id()) + may_slice_string(port);
   else
      port_string = HDL_manager::convert_to_identifier(this, port->get_id());

   if(GetPointer<port_o>(port)->get_port_direction() == port_o::IN)
      std::swap(port_string, signal_string);

   if(port_string != signal_string)
   {
      const auto left = GetPointer<port_o>(port)->get_port_direction() == port_o::IN ? sig : port;
      const auto right = GetPointer<port_o>(port)->get_port_direction() == port_o::IN ? port : sig;
      if(left->get_typeRef()->type == structural_type_descriptor::UINT and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
         signal_string = "unsigned(" + signal_string + ")";
      else if(left->get_typeRef()->type == structural_type_descriptor::INT and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
         signal_string = "signed(" + signal_string + ")";
      else if(left->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and right->get_typeRef()->type == structural_type_descriptor::UINT)
         signal_string = "std_logic_vector(" + signal_string + ")";
      else if(left->get_typeRef()->type == structural_type_descriptor::BOOL and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
         signal_string = "" + signal_string + "(0)";
      else if(left->get_typeRef()->type == structural_type_descriptor::BOOL and right->get_typeRef()->type == structural_type_descriptor::BOOL and right->get_kind() == constant_o_K)
         signal_string = std::string("'") + signal_string.at(1) + "'";
      indented_output_stream->Append(port_string + " <= " + signal_string + ";\n");
   }

   //   const auto left = GetPointer<port_o>(port)->get_port_direction() == port_o::IN ? sig : port;
   //   const auto right = GetPointer<port_o>(port)->get_port_direction() == port_o::IN ? port : sig;
   //   const auto left_string = HDL_manager::convert_to_identifier(this, left->get_id());
   //   const auto right_string = HDL_manager::convert_to_identifier(this, right->get_id());
   //   indented_output_stream->Append(left_string + " <= ");
   //   if(left->get_typeRef()->type == right->get_typeRef()->type)
   //   {
   //      indented_output_stream->Append(right_string + ";\n");
   //   }
   //   /// This part fix the assignment between std_logic_vector of size 1 and std_logic
   //   else if(left->get_typeRef()->type == structural_type_descriptor::BOOL and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
   //   {
   //      THROW_ASSERT(sig->get_typeRef()->size == 1, "Unexpected pattern");
   //      indented_output_stream->Append(right_string + "(0);\n");
   //   }
   //   else if(left->get_owner() and left->get_owner()->get_kind() == port_vector_o_K)
   //   {
   //      indented_output_stream->Append(right_string + "(" + left->get_id() + ");\n");
   //   }
   //   else if(left->get_typeRef()->type == structural_type_descriptor::INT and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
   //   {
   //      indented_output_stream->Append("signed(" + right_string + ");\n");
   //   }
   //   else if(left->get_typeRef()->type == structural_type_descriptor::UINT and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
   //   {
   //      indented_output_stream->Append("unsigned(" + right_string + ");\n");
   //   }
   //   else if(left->get_typeRef()->type == structural_type_descriptor::REAL and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
   //   {
   //      indented_output_stream->Append(right_string + ";\n");
   //   }
   //   else if(left->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and right->get_typeRef()->type == structural_type_descriptor::UINT)
   //   {
   //      indented_output_stream->Append("std_logic_vector(" + right_string + ");\n");
   //   }
   //   else
   //   {
   //      THROW_UNREACHABLE(left_string + "(" + left->get_typeRef()->get_name() + ") <= " + right_string + "(" + right->get_typeRef()->get_name() + ")");
   //   }
}

void VHDL_writer::write_io_signal_post_fix_vector(const structural_objectRef& port, const structural_objectRef& sig)
{
   THROW_ASSERT(port && port->get_kind() == port_vector_o_K, "Expected a port got something of different");
   THROW_ASSERT(port->get_owner(), "Expected a port with an owner");
   THROW_ASSERT(sig && sig->get_kind() == signal_vector_o_K, "Expected a signal got something of different");
   std::string port_string;
   std::string signal_string;
   signal_string = HDL_manager::convert_to_identifier(this, sig->get_id());
   port_string = HDL_manager::convert_to_identifier(this, port->get_id());

   if(GetPointer<port_o>(port)->get_port_direction() == port_o::IN)
      std::swap(port_string, signal_string);

   if(port_string != signal_string)
   {
      const auto left = GetPointer<port_o>(port)->get_port_direction() == port_o::IN ? sig : port;
      const auto right = GetPointer<port_o>(port)->get_port_direction() == port_o::IN ? port : sig;
      if(left->get_typeRef()->type == structural_type_descriptor::UINT and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
         signal_string = "unsigned(" + signal_string + ")";
      else if(left->get_typeRef()->type == structural_type_descriptor::INT and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
         signal_string = "signed(" + signal_string + ")";
      else if(left->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL and right->get_typeRef()->type == structural_type_descriptor::UINT)
         signal_string = "std_logic_vector(" + signal_string + ")";
      else if(left->get_typeRef()->type == structural_type_descriptor::BOOL and right->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL)
         signal_string = "" + signal_string + "(0)";
      indented_output_stream->Append(port_string + " <= " + signal_string + ";\n");
   }
}

void VHDL_writer::write_module_parametrization(const structural_objectRef& cir)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing module generics of " + cir->get_path());
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   auto* mod = GetPointer<module>(cir);
   /// writing memory-related parameters

   bool first_it = true;
   if(mod->ExistsParameter(MEMORY_PARAMETER))
   {
      std::string memory_str = mod->GetParameter(MEMORY_PARAMETER);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Memory parameters are " + memory_str);
      std::vector<std::string> mem_tag = convert_string_to_vector<std::string>(memory_str, ";");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found " + STR(mem_tag.size()) + " parameters");
      for(const auto& i : mem_tag)
      {
         std::vector<std::string> mem_add = convert_string_to_vector<std::string>(i, "=");
         THROW_ASSERT(mem_add.size() == 2, "malformed address");
         if(first_it)
         {
            indented_output_stream->Append(" generic map(");
            first_it = false;
         }
         else
         {
            indented_output_stream->Append(", ");
         }
         std::string name = mem_add[0];
         std::string value;
         if(mod->get_owner() && GetPointer<module>(mod->get_owner()) && GetPointer<module>(mod->get_owner())->ExistsParameter(MEMORY_PARAMETER))
            value = name;
         else
            value = mem_add[1];
         indented_output_stream->Append(name + "=>(" + value + ")");
      }
   }

   const NP_functionalityRef& np = mod->get_NP_functionality();
   if(np)
   {
      std::vector<std::pair<std::string, structural_objectRef>> library_parameters;
      mod->get_NP_library_parameters(cir, library_parameters);
      if(library_parameters.size())
         indented_output_stream->Append(" generic map(");

      for(const auto& library_parameter : library_parameters)
      {
         if(first_it)
         {
            first_it = false;
         }
         else
            indented_output_stream->Append(", ");
         const std::string& name = library_parameter.first;
         structural_objectRef obj = library_parameter.second;
         if(obj)
         {
            structural_type_descriptor::s_type type = obj->get_typeRef()->type;
            if((type == structural_type_descriptor::VECTOR_INT || type == structural_type_descriptor::VECTOR_UINT || type == structural_type_descriptor::VECTOR_REAL))
            {
               indented_output_stream->Append(HDL_manager::convert_to_identifier(this, BITSIZE_PREFIX + name) + "=>" + STR(obj->get_typeRef()->size));
               indented_output_stream->Append("," + HDL_manager::convert_to_identifier(this, NUM_ELEM_PREFIX + name) + "=>" + STR(obj->get_typeRef()->vector_size));
            }
            else
            {
               indented_output_stream->Append(HDL_manager::convert_to_identifier(this, BITSIZE_PREFIX + name) + "=>" + STR(GET_TYPE_SIZE(obj)));
               if(obj->get_kind() == port_vector_o_K)
               {
                  indented_output_stream->Append(", " + HDL_manager::convert_to_identifier(this, PORTSIZE_PREFIX + name) + "=>" + STR(GetPointer<port_o>(obj)->get_ports_size()));
               }
            }
         }
         else
         {
            const auto parameter = mod->GetParameter(name);
            const auto parameter_type = mod->get_parameter_type(TM, name);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter " + name + " has value " + parameter);
            switch(parameter_type)
            {
               case(structural_type_descriptor::OTHER):
               {
                  if(parameter.substr(0, 2) == "\"\"")
                  {
                     indented_output_stream->Append(name + "=>" + parameter.substr(1, parameter.size() - 2));
                  }
                  else
                  {
                     THROW_UNREACHABLE("");
                  }
                  break;
               }
               case structural_type_descriptor::VECTOR_BOOL:
               {
                  if(parameter.front() == '\"' and parameter.back() == '\"')
                  {
                     indented_output_stream->Append(name + "=>" + parameter);
                  }
                  /// Generic of owner module
                  else if(mod->get_owner())
                  {
                     if(GetPointer<const module>(mod->get_owner()))
                     {
                        if(mod->get_owner()->ExistsParameter(parameter))
                        {
#if HAVE_ASSERTS
                           const auto actual_parameter_type = GetPointer<const module>(mod->get_owner())->get_parameter_type(TM, parameter);
#endif
                           THROW_ASSERT(actual_parameter_type == parameter_type, "");
                        }
                        else if(mod->get_owner()->ExistsParameter(MEMORY_PARAMETER))
                        {
#if HAVE_ASSERTS
                           bool found = false;
#endif
                           std::string memory_str = mod->get_owner()->GetParameter(MEMORY_PARAMETER);
                           std::vector<std::string> mem_tag = convert_string_to_vector<std::string>(memory_str, ";");
                           for(const auto& i : mem_tag)
                           {
                              std::vector<std::string> mem_add = convert_string_to_vector<std::string>(i, "=");
                              THROW_ASSERT(mem_add.size() == 2, "malformed address");
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking owner memory parameter " + mem_add[0]);
                              if(mem_add[0] == parameter)
                              {
                                 /// If we reach this point actual parameter of current module is formal parameter of owner module of type memory
                                 /// Since memory parameters are integer and parameter of current module is std_logic_vector conversion has to be added
                                 /// We need to know the size of the generic (name), but this is not possible examining only the current generic
                                 /// At the moment his pattern has been encountered only in constant_value, so we use information about another generic that we know exist int that module
                                 THROW_ASSERT(mod->get_typeRef()->id_type == "constant_value", mod->get_typeRef()->id_type);
                                 THROW_ASSERT(mod->get_out_port_size() == 1, STR(mod->get_out_port_size()));
                                 indented_output_stream->Append(name + "=> std_logic_vector(to_unsigned(" + parameter + ", " + STR(STD_GET_SIZE(mod->get_out_port(0)->get_typeRef())) + "))");
#if HAVE_ASSERTS
                                 found = true;
#endif
                                 break;
                              }
                           }
                           THROW_ASSERT(found, parameter + " " + memory_str);
                           break;
                        }
                        else
                        {
                           THROW_UNREACHABLE("");
                        }
                     }
                     else
                     {
                        THROW_UNREACHABLE("");
                     }
                  }
                  else
                  {
                     THROW_UNREACHABLE("");
                  }
                  break;
               }
               case structural_type_descriptor::INT:
               {
                  if(parameter.front() == '\"' and parameter.back() == '\"')
                  {
                     long long int value = 0;
                     long long int mult = 1;
                     for(const auto digit : boost::adaptors::reverse(parameter.substr(1, parameter.size() - 2)))
                     {
                        if(digit == '1')
                        {
                           value += mult;
                        }
                        else if(digit == '0')
                        {
                        }
                        else
                        {
                           THROW_UNREACHABLE(parameter);
                        }
                        mult *= 2;
                     }
                     indented_output_stream->Append(name + "=>" + STR(value));
                  }
                  else
                  {
                     indented_output_stream->Append(name + "=>" + parameter);
                  }
                  break;
               }
               case structural_type_descriptor::BOOL:
               case structural_type_descriptor::UINT:
               case structural_type_descriptor::REAL:
               case structural_type_descriptor::USER:
               case structural_type_descriptor::VECTOR_INT:
               case structural_type_descriptor::VECTOR_UINT:
               case structural_type_descriptor::VECTOR_REAL:
               case structural_type_descriptor::VECTOR_USER:
               case structural_type_descriptor::UNKNOWN:
               {
                  THROW_UNREACHABLE("Type of parameter " + parameter + " (" + STR(parameter_type) + ") not supported");
                  break;
               }
               default:
                  THROW_UNREACHABLE("");
            }
         }
      }
   }
   if(!first_it)
      indented_output_stream->Append(")");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written module generics of " + cir->get_path());
}

void VHDL_writer::write_tail(const structural_objectRef&)
{
}

void VHDL_writer::write_state_declaration(const structural_objectRef&, const std::list<std::string>& list_of_states, const std::string&, const std::string&, bool one_hot)
{
   auto it_end = list_of_states.end();
   size_t n_states = list_of_states.size();
   unsigned int bitsnumber = language_writer::bitnumber(static_cast<unsigned int>(n_states - 1));
   /// adjust in case states are not consecutives
   unsigned max_value = 0;
   for(auto it = list_of_states.begin(); it != it_end; ++it)
   {
      max_value = std::max(max_value, boost::lexical_cast<unsigned int>(it->substr(strlen(STATE_NAME_PREFIX))));
   }
   if(max_value != n_states - 1)
      bitsnumber = language_writer::bitnumber(max_value);

   write_comment("define the states of FSM model\n");
   if(one_hot or ((parameters->isOption(OPT_generate_vcd) and parameters->getOption<bool>(OPT_generate_vcd)) or (parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))))
   {
      for(const auto& state : list_of_states)
      {
         if(one_hot)
            indented_output_stream->Append("constant " + state + ": std_logic_vector(" + STR(max_value) + " downto 0) := \"" + encode_one_hot(1 + max_value, boost::lexical_cast<unsigned int>(state.substr(strlen(STATE_NAME_PREFIX)))) + "\";\n");
         else
            indented_output_stream->Append("constant " + state + ": std_logic_vector(" + STR(bitsnumber - 1) + " downto 0) := \"" + NumberToBinaryString(boost::lexical_cast<size_t>(state.substr(strlen(STATE_NAME_PREFIX))), bitsnumber) + "\";\n");
      }
      if(one_hot)
         indented_output_stream->Append("signal present_state, next_state : std_logic_vector(" + STR(max_value) + " downto 0);\n");
      else
         indented_output_stream->Append("signal present_state, next_state : std_logic_vector(" + STR(bitsnumber - 1) + " downto 0);\n");
   }
   else
   {
      indented_output_stream->Append("type state_type is (");
      unsigned int count = 0;
      for(auto it = list_of_states.begin(); it != it_end; ++it)
      {
         indented_output_stream->Append(*it);
         count++;
         if(count != n_states)
            indented_output_stream->Append(", ");
      }
      indented_output_stream->Append(");\n");
      indented_output_stream->Append("signal present_state, next_state : state_type;\n");
   }
}

void VHDL_writer::write_present_state_update(const structural_objectRef, const std::string& reset_state, const std::string& reset_port, const std::string& clock_port, const std::string& reset_type, bool)
{
   write_comment("concurrent process#1: state registers\n");
   if(reset_type == "no" || reset_type == "sync")
   {
      indented_output_stream->Append("state_reg: process(" + clock_port + ")\n");
      indented_output_stream->Append("begin\n");
      indented_output_stream->Indent();
      indented_output_stream->Append("if (" + clock_port + "'event and " + clock_port + "='1') then\n");
      indented_output_stream->Indent();
      if(!parameters->getOption<bool>(OPT_level_reset))
         indented_output_stream->Append("if (" + reset_port + "='0') then\n");
      else
         indented_output_stream->Append("if (" + reset_port + "='1') then\n");
      indented_output_stream->Indent();
      indented_output_stream->Append("present_state <= " + reset_state + ";\n");
      indented_output_stream->Deindent();
      indented_output_stream->Append("else\n");
      indented_output_stream->Indent();
      indented_output_stream->Append("present_state <= next_state;\n");
      indented_output_stream->Deindent();
      indented_output_stream->Append("end if;\n");
   }
   else
   {
      indented_output_stream->Append("state_reg: process(" + clock_port + ", " + reset_port + ")\n");
      indented_output_stream->Append("begin\n");
      indented_output_stream->Indent();
      if(!parameters->getOption<bool>(OPT_level_reset))
         indented_output_stream->Append("if (" + reset_port + "='0') then\n");
      else
         indented_output_stream->Append("if (" + reset_port + "='1') then\n");
      indented_output_stream->Indent();
      indented_output_stream->Append("present_state <= " + reset_state + ";\n");
      indented_output_stream->Deindent();
      indented_output_stream->Append("elsif (" + clock_port + "'event and " + clock_port + "='1') then\n");
      indented_output_stream->Indent();
      indented_output_stream->Append("present_state <= next_state;\n");
   }
   indented_output_stream->Deindent();
   indented_output_stream->Append("end if;\n");
   indented_output_stream->Deindent();
   indented_output_stream->Append("end process;\n");
}

void VHDL_writer::write_transition_output_functions(bool single_proc, unsigned int output_index, const structural_objectRef& cir, const std::string& reset_state, const std::string& reset_port, const std::string& start_port, const std::string& clock_port,
                                                    std::vector<std::string>::const_iterator& first, std::vector<std::string>::const_iterator& end, bool)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing transition output function");
   auto* mod = GetPointer<module>(cir);
   boost::char_separator<char> state_sep(":", nullptr);
   boost::char_separator<char> sep(" ", nullptr);
   typedef boost::tokenizer<boost::char_separator<char>> tokenizer;

   /// get the default output of the reset state

   write_comment("concurrent process#" + STR(output_index) + ": combinational logic\n");
   indented_output_stream->Append("comb_logic" + STR(output_index) + ": process(present_state");
   for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
   {
      std::string port_name = HDL_manager::convert_to_identifier(this, mod->get_in_port(i)->get_id());
      if(port_name != clock_port and port_name != reset_port)
      {
         indented_output_stream->Append(", " + port_name);
      }
   }
   indented_output_stream->Append(")\n");

   indented_output_stream->Append("begin\n");
   indented_output_stream->Indent();

   /// set the defaults
   std::string default_output;
   for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
   {
      if(mod->get_out_port(i)->get_id() == PRESENT_STATE_PORT_NAME)
         continue;
      if(mod->get_out_port(i)->get_id() == NEXT_STATE_PORT_NAME)
         continue;
      default_output += "0";
      if(!single_proc && output_index != i)
         continue;
      std::string port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i)->get_id());
      indented_output_stream->Append(port_name + " <= '0';\n");
   }
   if(single_proc || output_index == mod->get_out_port_size())
      indented_output_stream->Append("next_state <= " + reset_state + ";\n");

   indented_output_stream->Append("case present_state is\n");
   indented_output_stream->Indent();

   for(auto first_it = first; first_it != end; ++first_it)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing " + *first_it);
      tokenizer state_tokens_first(*first_it, state_sep);

      tokenizer::const_iterator its = state_tokens_first.begin();

      std::string state_description = *its;
      ++its;

      std::vector<std::string> state_transitions;
      for(; its != state_tokens_first.end(); ++its)
      {
         state_transitions.push_back(*its);
      }

      tokenizer tokens_curr(state_description, sep);
      /// get the present state
      its = tokens_curr.begin();
      std::string present_state = HDL_manager::convert_to_identifier(this, *its);
      /// get the current output
      ++its;
      std::string current_output = *its;

      /// check if we can skip this state or transitions
      bool skip_state = !single_proc && output_index != mod->get_out_port_size() && default_output[output_index] == current_output[output_index];
      bool skip_state_transition = !single_proc && output_index != mod->get_out_port_size();
      if(!single_proc && output_index != mod->get_out_port_size())
      {
         for(auto current_transition : state_transitions)
         {
            tokenizer transition_tokens(current_transition, sep);
            tokenizer::const_iterator itt = transition_tokens.begin();

            // std::string current_input;
            tokenizer::const_iterator current_input_it;
            std::string input_string = *itt;
            if(mod->get_in_port_size() - 3) // clock and reset are always present
            {
               boost::char_separator<char> comma_sep(",", nullptr);
               tokenizer current_input_tokens(input_string, comma_sep);
               current_input_it = current_input_tokens.begin();
               ++itt;
            }
            ++itt;
            std::string transition_outputs = *itt;
            ++itt;
            THROW_ASSERT(itt == transition_tokens.end(), "Bad transition format");
            if(transition_outputs[output_index] != '-')
            {
               skip_state = false;
               skip_state_transition = false;
            }
         }
      }
      if(skip_state)
         continue;

      indented_output_stream->Append("when " + present_state + " =>\n");
      indented_output_stream->Indent();

      if(reset_state == present_state)
      {
         indented_output_stream->Append("if(" + start_port + " /= '1') then\n");
         for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
         {
            if(boost::starts_with(mod->get_out_port(i)->get_id(), "selector_MUX") || boost::starts_with(mod->get_out_port(i)->get_id(), "wrenable_reg"))
            {
               auto port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i)->get_id());
               if(single_proc || output_index == i)
                  indented_output_stream->Append("  " + port_name + " <= 'X';\n");
            }
         }
         if(single_proc || output_index == mod->get_out_port_size())
            indented_output_stream->Append("  next_state <= " + present_state + ";\n");
         indented_output_stream->Append("else\n");
         indented_output_stream->Indent();
      }

      bool unique_transition = (state_transitions.size() == 1);
      if(current_output != default_output && (single_proc || !unique_transition || skip_state_transition))
      {
         for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
         {
            if(mod->get_out_port(i)->get_id() == PRESENT_STATE_PORT_NAME)
               continue;
            if(mod->get_out_port(i)->get_id() == NEXT_STATE_PORT_NAME)
               continue;

            std::string port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i)->get_id());
            if(default_output[i] != current_output[i])
            {
               if(single_proc || output_index == i)
               {
                  switch(current_output[i])
                  {
                     case '1':
                        indented_output_stream->Append(port_name + " <= '" + current_output[i] + "';\n");
                        break;

                     case '2':
                        indented_output_stream->Append(port_name + " <= 'X';\n");
                        break;

                     default:
                        THROW_ERROR("Unsupported value in current output");
                        break;
                  }
               }
            }
         }
      }

      if(!skip_state_transition)
      {
         for(unsigned int i = 0; i < state_transitions.size(); i++)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing transition " + state_transitions[i]);
            // std::cerr << " transition '" << state_transitions[i] << "'" << std::endl;
            tokenizer transition_tokens(state_transitions[i], sep);
            tokenizer::const_iterator itt = transition_tokens.begin();
            THROW_ASSERT(itt != transition_tokens.end(), "");

            // std::string current_input;
            tokenizer::const_iterator current_input_it;
            std::string input_string = *itt;
            if(mod->get_in_port_size() - 3) // clock and reset are always present
            {
               boost::char_separator<char> comma_sep(",", nullptr);
               tokenizer current_input_tokens(input_string, comma_sep);
               current_input_it = current_input_tokens.begin();
               ++itt;
            }
            THROW_ASSERT(itt != transition_tokens.end(), "");
            std::string next_state = HDL_manager::convert_to_identifier(this, *itt);
            ++itt;
            THROW_ASSERT(itt != transition_tokens.end(), "");
            std::string transition_outputs = *itt;
            ++itt;
            THROW_ASSERT(itt == transition_tokens.end(), "Bad transition format");

            if(!unique_transition)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not single transition");
               if(i == 0)
               {
                  indented_output_stream->Append("if (");
               }
               else if((i + 1) == state_transitions.size())
               {
                  indented_output_stream->Append("else\n");
               }
               else
               {
                  indented_output_stream->Append("elsif (");
               }
               if((i + 1) < state_transitions.size())
               {
                  bool first_test = true;
                  for(unsigned int ind = 0; ind < mod->get_in_port_size(); ind++)
                  {
                     std::string port_name = HDL_manager::convert_to_identifier(this, mod->get_in_port(ind)->get_id());
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering port " + port_name);
                     unsigned int port_size = mod->get_in_port(ind)->get_typeRef()->size;
                     unsigned int vec_size = mod->get_in_port(ind)->get_typeRef()->vector_size;
                     if(port_name != reset_port && port_name != clock_port && port_name != start_port)
                     {
                        std::string in_or_conditions = *current_input_it;
                        boost::char_separator<char> pipe_sep("|", nullptr);
                        tokenizer in_or_conditions_tokens(in_or_conditions, pipe_sep);
                        THROW_ASSERT(in_or_conditions_tokens.begin() != in_or_conditions_tokens.end(), "");

                        if((*in_or_conditions_tokens.begin()) != "-")
                        {
                           if(!first_test)
                              indented_output_stream->Append(" and ");
                           else
                              first_test = false;
                           bool first_test_or = true;
                           bool need_parenthesis = false;
                           std::string res_or_conditions;
                           for(tokenizer::const_iterator in_or_conditions_tokens_it = in_or_conditions_tokens.begin(); in_or_conditions_tokens_it != in_or_conditions_tokens.end(); ++in_or_conditions_tokens_it)
                           {
                              THROW_ASSERT((*in_or_conditions_tokens_it) != "-", "wrong conditions structure");
                              if(!first_test_or)
                              {
                                 res_or_conditions += " or ";
                                 need_parenthesis = true;
                              }
                              else
                                 first_test_or = false;

                              res_or_conditions += port_name;
                              if((*in_or_conditions_tokens_it)[0] == '&')
                              {
                                 auto pos = boost::lexical_cast<unsigned int>((*in_or_conditions_tokens_it).substr(1));
                                 res_or_conditions += std::string("(") + STR(pos) + ") = '1'";
                              }
                              else
                              {
                                 res_or_conditions += std::string(" = ") + ((*in_or_conditions_tokens_it)[0] == '-' ? "-" : "");
                                 if(port_size > 1 || (port_size == 1 && vec_size > 0))
                                    res_or_conditions += "\"" + NumberToBinaryString(llabs(boost::lexical_cast<long long>(*in_or_conditions_tokens_it)), port_size) + "\"";
                                 else
                                    res_or_conditions += "'" + *in_or_conditions_tokens_it + "'";
                              }
                           }
                           if(need_parenthesis)
                              res_or_conditions = "(" + res_or_conditions + ")";
                           indented_output_stream->Append(res_or_conditions);
                        }
                        ++current_input_it;
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered port " + port_name);
                  }
                  indented_output_stream->Append(") then\n");
               }
               indented_output_stream->Indent();
            }
            if(single_proc || output_index == mod->get_out_port_size())
               indented_output_stream->Append("next_state <= " + next_state + ";\n");
            for(unsigned int i2 = 0; i2 < mod->get_out_port_size(); i2++)
            {
               if(transition_outputs[i2] != '-')
               {
                  std::string port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i2)->get_id());
                  if(single_proc || output_index == i2)
                  {
                     if(transition_outputs[i2] == '2')
                        indented_output_stream->Append(port_name + " <= 'X';\n");
                     else
                        indented_output_stream->Append(port_name + " <= '" + transition_outputs[i2] + "';\n");
                  }
               }
            }
            indented_output_stream->Deindent();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written transition " + state_transitions[i]);
         }
         if(!unique_transition)
         {
            indented_output_stream->Append("end if;\n");
            indented_output_stream->Deindent();
         }
      }
      else
      {
         indented_output_stream->Deindent();
      }
      if(reset_state == present_state)
      {
         indented_output_stream->Append("end if;\n");
         indented_output_stream->Deindent();
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   /// really useful for one-hot encoding
   indented_output_stream->Append("when others =>\n");
   for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
   {
      if(boost::starts_with(mod->get_out_port(i)->get_id(), "selector_MUX") || boost::starts_with(mod->get_out_port(i)->get_id(), "wrenable_reg"))
      {
         auto port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i)->get_id());
         if(single_proc || output_index == i)
            indented_output_stream->Append("  " + port_name + " <= 'X';\n");
      }
   }

   indented_output_stream->Deindent();
   indented_output_stream->Append("end case;\n");
   indented_output_stream->Deindent();
   indented_output_stream->Append("end process;\n");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written transition output function");
}

void VHDL_writer::write_assign(const std::string&, const std::string&)
{
   THROW_ERROR("Not yet implemented");
}

void VHDL_writer::write_NP_functionalities(const structural_objectRef& cir)
{
   auto* mod = GetPointer<module>(cir);
   THROW_ASSERT(mod, "Expected a component object");
   const NP_functionalityRef& np = mod->get_NP_functionality();
   THROW_ASSERT(np, "NP Behavioral description is missing for module: " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)));
   std::string beh_desc = np->get_NP_functionality(NP_functionality::VHDL_PROVIDED);
   THROW_ASSERT(beh_desc != "", "VHDL behavioral description is missing for module: " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)));
   remove_escaped(beh_desc);
   /// manage reset by preprocessing the behavioral description
   if(!parameters->getOption<bool>(OPT_level_reset))
      boost::replace_all(beh_desc, "1RESET_VALUE", std::string(RESET_PORT_NAME) + " = '0'");
   else
      boost::replace_all(beh_desc, "1RESET_VALUE", std::string(RESET_PORT_NAME) + " = '1'");
   if(parameters->getOption<bool>(OPT_reg_init_value))
   {
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUEb", ":= '0'");
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUE", ":= (others => '0')");
   }
   else
   {
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUEb", "");
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUE", "");
   }
   indented_output_stream->Append(beh_desc);
}

void VHDL_writer::write_port_decl_header()
{
   indented_output_stream->Append("port (\n");
   indented_output_stream->Indent();
}

void VHDL_writer::write_port_decl_tail()
{
   indented_output_stream->Deindent();
   indented_output_stream->Append("\n);\n");
}

void VHDL_writer::write_module_parametrization_decl(const structural_objectRef& cir)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing generics of entity " + cir->get_id());
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   auto* mod = GetPointer<module>(cir);
   bool first_it = true;
   /// writing memory-related parameters
   if(mod->ExistsParameter(MEMORY_PARAMETER) and mod->GetParameter(MEMORY_PARAMETER) != "")
   {
      indented_output_stream->Append("generic(\n");
      std::string memory_str = mod->GetParameter(MEMORY_PARAMETER);
      std::vector<std::string> mem_tag = convert_string_to_vector<std::string>(memory_str, ";");
      for(const auto& i : mem_tag)
      {
         std::vector<std::string> mem_add = convert_string_to_vector<std::string>(i, "=");
         THROW_ASSERT(mem_add.size() == 2, "malformed address");
         if(first_it)
         {
            first_it = false;
         }
         else
         {
            indented_output_stream->Append(";\n");
         }
         std::string name = mem_add[0];
         std::string value = mem_add[1];
         auto binary_value = NumberToBinaryString(boost::lexical_cast<unsigned long long int>(value));
         indented_output_stream->Append("  " + name + ": integer := " + value);
      }
   }

   const NP_functionalityRef& np = mod->get_NP_functionality();
   if(np)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering parameters");
      std::vector<std::pair<std::string, structural_objectRef>> library_parameters;
      mod->get_NP_library_parameters(cir, library_parameters);
      if(library_parameters.size() and first_it)
         indented_output_stream->Append("generic(");

      for(const auto& library_parameter : library_parameters)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering parameter " + library_parameter.first);
         if(first_it)
         {
            first_it = false;
         }
         else
            indented_output_stream->Append(";");
         indented_output_stream->Append("\n");
         const std::string& name = library_parameter.first;
         structural_objectRef obj = library_parameter.second;
         if(obj)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Object of type " + obj->get_kind_text());
            structural_type_descriptor::s_type type = obj->get_typeRef()->type;
            if((type == structural_type_descriptor::VECTOR_INT || type == structural_type_descriptor::VECTOR_UINT || type == structural_type_descriptor::VECTOR_REAL))
            {
               indented_output_stream->Append(std::string(" ") + HDL_manager::convert_to_identifier(this, BITSIZE_PREFIX + name) + ": integer");
               indented_output_stream->Append(";\n " + HDL_manager::convert_to_identifier(this, NUM_ELEM_PREFIX + name) + ": integer");
            }
            else
            {
               indented_output_stream->Append(std::string(" ") + HDL_manager::convert_to_identifier(this, BITSIZE_PREFIX + name) + ": integer");
               if(obj->get_kind() == port_vector_o_K)
               {
                  indented_output_stream->Append(";\n " + HDL_manager::convert_to_identifier(this, PORTSIZE_PREFIX + name) + ": integer");
               }
            }
         }
         else
         {
            const auto parameter = mod->GetDefaultParameter(name);
            const auto parameter_type = mod->get_parameter_type(TM, name);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter " + name + " has default value " + parameter);
            switch(parameter_type)
            {
               case structural_type_descriptor::INT:
               {
                  indented_output_stream->Append(" " + name + ": integer");
                  break;
               }
               case structural_type_descriptor::OTHER:
               {
                  indented_output_stream->Append(" " + name + ": string");
                  break;
               }
               case structural_type_descriptor::VECTOR_BOOL:
               {
                  indented_output_stream->Append(" " + name + ": std_logic_vector");
                  break;
               }
               case structural_type_descriptor::BOOL:
               case structural_type_descriptor::UINT:
               case structural_type_descriptor::REAL:
               case structural_type_descriptor::USER:
               case structural_type_descriptor::VECTOR_INT:
               case structural_type_descriptor::VECTOR_UINT:
               case structural_type_descriptor::VECTOR_REAL:
               case structural_type_descriptor::VECTOR_USER:
               case structural_type_descriptor::UNKNOWN:
               default:
                  THROW_UNREACHABLE("");
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered parameter " + library_parameter.first);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered parameters");
      /// in case first_it is false at least one parameter has used.
   }
   if(!first_it)
      indented_output_stream->Append(");\n");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written generics of entity " + cir->get_id());
}

bool VHDL_writer::check_keyword(std::string id) const
{
   return keywords.find(boost::to_upper_copy<std::string>(id)) != keywords.end();
}

void VHDL_writer::WriteBuiltin(const structural_objectConstRef component)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing builtin component " + component->get_path() + " of type " + GET_TYPE_NAME(component));
   std::string operand;
   if(GET_TYPE_NAME(component) == OR_GATE_STD)
   {
      operand = "or";
   }
   else if(GET_TYPE_NAME(component) == AND_GATE_STD)
   {
      operand = "and";
   }
   else
   {
      THROW_UNREACHABLE(GET_TYPE_NAME(component));
   }
   const auto mod = GetPointer<const module>(component);
   THROW_ASSERT(mod, component->get_path() + " is not a module");
   THROW_ASSERT(GetPointer<const port_o>(mod->get_out_port(0)), "Not a port");
   const auto object_bounded = GetPointer<const port_o>(mod->get_out_port(0))->find_bounded_object(component->get_owner());
   THROW_ASSERT(object_bounded, "Object bounded to output not found");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Output bounded object is " + object_bounded->get_path());
   THROW_ASSERT(mod->get_out_port_size() == 1, component->get_path() + " has " + STR(mod->get_out_port_size()) + " output ports");
   indented_output_stream->Append(HDL_manager::convert_to_identifier(this, object_bounded->get_id()) + " <= ");
   for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
   {
      if(i != 0)
      {
         indented_output_stream->Append(" " + operand + " ");
      }
      if(mod->get_in_port(i)->get_kind() == port_o_K)
      {
         const auto in_object_bounded = GetPointer<port_o>(mod->get_in_port(i))->find_bounded_object(component->get_owner());
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, in_object_bounded->get_id()));
      }
      else if(mod->get_in_port(i)->get_kind() == port_vector_o_K)
      {
         const auto port = GetPointer<port_o>(mod->get_in_port(i));
         const auto in_object_bounded = port->find_bounded_object(component);
         if(in_object_bounded)
         {
            indented_output_stream->Append(HDL_manager::convert_to_identifier(this, in_object_bounded->get_id()));
         }
         else
         {
            auto n_ports = port->get_ports_size();
            for(decltype(n_ports) port_index = 0; port_index < n_ports; port_index++)
            {
               if(i != 0 or port_index != 0)
                  indented_output_stream->Append(" " + operand + " ");
               const auto vec_in_object_bounded = GetPointer<port_o>(port->get_port(port_index))->find_bounded_object();
               THROW_ASSERT(vec_in_object_bounded, "");
               indented_output_stream->Append(HDL_manager::convert_to_identifier(this, vec_in_object_bounded->get_id()));
            }
         }
      }
      else
      {
         THROW_UNREACHABLE("");
      }
   }
   indented_output_stream->Append(";\n");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written builtin component " + component->get_path() + " of type " + GET_TYPE_NAME(component));
}

void VHDL_writer::write_header()
{
   indented_output_stream->Append("\n");
   indented_output_stream->Append("library IEEE;\n");
   indented_output_stream->Append("use IEEE.numeric_std.all;\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("package panda_pkg is\n");
   indented_output_stream->Append("   function resize_signed(input : signed; size : integer) return signed;\n");
   indented_output_stream->Append("end;\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("package body panda_pkg is\n");
   indented_output_stream->Append("   function resize_signed(input : signed; size : integer) return signed is\n");
   indented_output_stream->Append("   begin\n");
   indented_output_stream->Append("     if (size > input'length) then\n");
   indented_output_stream->Append("       return resize(input, size);\n");
   indented_output_stream->Append("     else\n");
   indented_output_stream->Append("       return input(size-1+input'right downto input'right);\n");
   indented_output_stream->Append("     end if;\n");
   indented_output_stream->Append("   end function;\n");
   indented_output_stream->Append("end package body;\n");
   indented_output_stream->Append("\n");
}
