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
 * @file verilog_writer.cpp
 * @brief This class implements the methods to write Verilog descriptions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_FROM_C_BUILT.hpp"

#include "verilog_writer.hpp"

#include "HDL_manager.hpp"

#include "technology_manager.hpp"
#include "time_model.hpp"

#include "NP_functionality.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "structural_objects.hpp"
#include "tree_helper.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <functional>
#include <iosfwd>

#include "state_transition_graph_manager.hpp"

///. include
#include "Parameter.hpp"

/// STD include
#include <limits>

/// STL include
#include <utility>

/// technology include
#include "technology_node.hpp"

/// utility include
#include "indented_output_stream.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

#define VERILOG_2001_SUPPORTED

const std::map<std::string, std::string> verilog_writer::builtin_to_verilog_keyword = {
    {AND_GATE_STD, "and"}, {NAND_GATE_STD, "nand"}, {OR_GATE_STD, "or"}, {NOR_GATE_STD, "nor"}, {XOR_GATE_STD, "xor"}, {XNOR_GATE_STD, "xnor"}, {NOT_GATE_STD, "not"}, {DFF_GATE_STD, "dff"}, {BUFF_GATE_STD, "buf"},
};

const char* verilog_writer::tokenNames[] = {
    "abs", "abstol", "access", "acos", "acosh", "always", "analog", "and", "asin", "asinh", "assign", "atan", "atan2", "atanh", "automatic", "begin", "bool", "buf", "bufif0", "bufif1", "case", "casex", "casez", "ceil", "cell", "cmos", "config",
    "continuous", "cos", "cosh", "ddt_nature", "deassign", "default", "defparam", "design", "disable", "discipline", "discrete", "domain", "edge", "else", "end", "endcase", "endconfig", "enddiscipline", "endfunction", "endgenerate", "endmodule",
    "endnature", "endprimitive", "endspecify", "endtable", "endtask", "event", "exclude", "exp", "floor", "flow", "for", "force", "forever", "fork", "from", "function", "generate", "genvar", "ground", "highz0", "highz1", "hypot", "idt_nature", "if",
    "ifnone", "incdir", "include", "inf", "initial", "inout", "input", "instance", "integer", "join", "large", "liblist", "library", "ln", "localparam", "log", "macromodule", "max", "medium", "min", "module", "nand", "nature", "negedge", "nmos", "nor",
    "noshowcancelled", "not", "notif0", "notif1", "or", "output", "parameter", "pmos", "posedge", "potential", "pow", "primitive", "pull0", "pull1", "pulldown", "pullup", "pulsestyle_onevent", "pulsestyle_ondetect", "rcmos", "real", "realtime", "reg",
    "release", "repeat", "rnmos", "rpmos", "rtran", "rtranif0", "rtranif1", "scalared", "showcancelled", "signed", "sin", "sinh", "small", "specify", "specparam", "sqrt", "strong0", "strong1", "supply0", "supply1", "table", "tan", "tanh", "task", "time",
    "tran", "tranif0", "tranif1", "tri", "tri0", "tri1", "triand", "trior", "trireg", "units", "unsigned", "use", "uwire", "vectored", "wait", "wand", "weak0", "weak1", "while", "wire", "wone", "wor", "xnor", "xor",
    /// some System Verilog 2005 keywords
    "alias", "always_comb", "always_ff", "always_latch", "assert", "assume", "before", "bind", "bins", "binsof", "bit", "break", "byte", "chandle", "class", "clocking", "const", "constraint", "context", "continue", "cover", "covergroup", "coverpoint",
    "cross", "dist", "do", "endclass", "endgroup",
    "endsequence"
    "endclocking",
    "endpackage", "endinterface", "endprogram", "endproperty", "enum", "expect", "export", "extends", "extern", "final", "first_match", "foreach", "forkjoin", "iff", "ignore_bins", "illegal_bins", "import", "intersect", "inside", "interface", "int",
    "join_any", "join_none", "local", "logic", "longint", "matches", "modport", "new", "null", "package", "packed", "priority", "program", "property", "protected", "pure", "rand", "randc", "randcase", "randomize", "randsequence", "ref", "return",
    "sequence", "shortint", "shortreal", "solve", "static", "string", "struct", "super", "tagged", "this", "throughout", "timeprecision", "timeunit", "type", "typedef", "unique", "var", "virtual", "void", "wait_order", "wildcard", "with", "within",
    /// some System Verilog 2009 keywords
    "accept_on", "checker", "endchecker", "eventually", "global", "implies", "let", "nexttime", "reject_on", "restrict", "s_always", "s_eventually", "s_nexttime", "s_until", "s_until_with", "strong", "sync_accept_on", "sync_reject_on", "unique0", "until",
    "until_with", "untyped", "weak",
    /// some System Verilog 2012 keywords
    "implements", "interconnect", "nettype", "soft"};

void verilog_writer::write_comment(const std::string& comment_string)
{
   indented_output_stream->Append("// " + comment_string);
}

std::string verilog_writer::type_converter(structural_type_descriptorRef Type)
{
   switch(Type->type)
   {
      case structural_type_descriptor::BOOL:
      {
         return "";
      }
      case structural_type_descriptor::INT:
      {
         return "signed ";
      }
      case structural_type_descriptor::UINT:
      {
         return "";
      }
      case structural_type_descriptor::USER:
      {
         THROW_ERROR("USER type not yet supported");
         break;
      }
      case structural_type_descriptor::REAL:
      {
         return "";
      }
      case structural_type_descriptor::VECTOR_BOOL:
      {
         return "";
      }
      case structural_type_descriptor::VECTOR_INT:
      {
         return "signed ";
      }
      case structural_type_descriptor::VECTOR_UINT:
      {
         return "";
      }
      case structural_type_descriptor::VECTOR_REAL:
      {
         break;
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
         THROW_ERROR("Not initialized type");
   }
   return "";
}

std::string verilog_writer::type_converter_size(const structural_objectRef& cir)
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
         for(auto const& library_parameter : library_parameters)
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
               return "[" + (PORTSIZE_PREFIX + port_name) + "-1:0] ";
            else
               return "[" + boost::lexical_cast<std::string>(GetPointer<port_o>(cir)->get_ports_size() - 1) + ":0] ";
         }
         else if(cir->get_owner() and cir->get_owner()->get_kind() == port_vector_o_K)
         {
            const auto owner_vector = GetPointer<const port_o>(cir->get_owner());
            for(unsigned int vector_index = 0; vector_index < owner_vector->get_ports_size(); vector_index++)
            {
               if(owner_vector->get_port(vector_index) == cir)
               {
                  return "[" + STR(vector_index) + ":" + STR(vector_index) + "]";
               }
            }
            THROW_UNREACHABLE("");
         }
         else
         {
            return "";
         }
         break;
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
            if(cir->get_kind() == port_vector_o_K)
            {
               unsigned int lsb = GetPointer<port_o>(cir)->get_lsb();
               return "[(" + (PORTSIZE_PREFIX + port_name) + "*" + (BITSIZE_PREFIX + port_name) + ")+(" + boost::lexical_cast<std::string>(static_cast<int>(lsb) - 1) + "):" + boost::lexical_cast<std::string>(lsb) + "] ";
            }
            else if(cir->get_owner() and cir->get_owner()->get_kind() == port_vector_o_K)
            {
               THROW_UNREACHABLE("");
            }
            else
               return "[" + (BITSIZE_PREFIX + port_name) + "-1:0] ";
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

               return "[" + boost::lexical_cast<std::string>(static_cast<int>(msb) - 1) + ":" + boost::lexical_cast<std::string>(lsb) + "] ";
            }
            else if(cir->get_kind() == port_vector_o_K)
            {
               unsigned int lsb = GetPointer<port_o>(cir)->get_lsb();
               unsigned int n_ports = GetPointer<port_o>(cir)->get_ports_size();
               const structural_objectRef first_port = GetPointer<port_o>(cir)->get_port(0);
               const auto Type_fp = first_port->get_typeRef();
               unsigned int size_fp = Type_fp->vector_size > 0 ? Type_fp->size * Type_fp->vector_size : Type_fp->size;
               unsigned int msb = size_fp * n_ports + lsb;
               return "[" + boost::lexical_cast<std::string>(static_cast<int>(msb) - 1) + ":" + boost::lexical_cast<std::string>(lsb) + "] ";
            }
            else if(cir->get_owner() and cir->get_owner()->get_kind() == port_vector_o_K)
            {
               const auto owner_vector = GetPointer<const port_o>(cir->get_owner());
               unsigned int lsb = owner_vector->get_lsb();
               for(unsigned int vector_index = 0; vector_index < owner_vector->get_ports_size(); vector_index++)
               {
                  if(owner_vector->get_port(vector_index) == cir)
                  {
                     const structural_objectRef first_port = owner_vector->get_port(0);
                     unsigned int single_size_port = GET_TYPE_SIZE(first_port);
                     return "[" + STR(((vector_index + 1) * single_size_port) - 1 + lsb) + ":" + STR(vector_index * single_size_port + lsb) + "]";
                  }
               }
               THROW_UNREACHABLE("");
            }
            if(Type->vector_size > 1 && Type->size == 1)
               return "[" + boost::lexical_cast<std::string>(static_cast<int>(Type->vector_size) - 1) + ":0] ";
            else if(Type->vector_size == 1 && Type->size == 1)
               return "";
            else if(Type->vector_size == 0 && Type->size != 0)
               return "[" + boost::lexical_cast<std::string>(static_cast<int>(Type->size) - 1) + ":0] ";
            else
               THROW_ERROR("Not completely specified: " + port_name);
         }
         break;
      }
      case structural_type_descriptor::VECTOR_UINT:
      case structural_type_descriptor::VECTOR_INT:
      case structural_type_descriptor::VECTOR_REAL:
      {
         if(specialization_string)
            return "[" + (BITSIZE_PREFIX + port_name) + "*" + (NUM_ELEM_PREFIX + port_name) + "-1:0] ";
         else if(Type->vector_size * Type->size > 1)
            return "[" + boost::lexical_cast<std::string>(Type->vector_size * Type->size - 1) + ":0] ";
         else
            return "";
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
         THROW_ERROR("Not initialized type");
   }
   return "";
}

std::string verilog_writer::may_slice_string(const structural_objectRef& cir)
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
            return "[" + GetPointer<port_o>(cir)->get_id() + "]";
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
               return "[((" + boost::lexical_cast<std::string>(GetPointer<port_o>(cir)->get_id()) + "+1)*" + (BITSIZE_PREFIX + port_name) + ")+(" + boost::lexical_cast<std::string>(static_cast<int>(lsb) - 1) + "):(" +
                      boost::lexical_cast<std::string>(GetPointer<port_o>(cir)->get_id()) + "*" + (BITSIZE_PREFIX + port_name) + ")+" + boost::lexical_cast<std::string>(lsb) + "]";
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
               return "[" + boost::lexical_cast<std::string>((1 + boost::lexical_cast<int>(GetPointer<port_o>(cir)->get_id())) * static_cast<int>(size_fp) + static_cast<int>(lsb) - 1) + ":" +
                      boost::lexical_cast<std::string>((boost::lexical_cast<int>(GetPointer<port_o>(cir)->get_id())) * static_cast<int>(size_fp) + static_cast<int>(lsb)) + "]";
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
               return "[((" + boost::lexical_cast<std::string>(GetPointer<port_o>(cir)->get_id()) + "+1)*" + (BITSIZE_PREFIX + port_name) + "*" + (NUM_ELEM_PREFIX + port_name) + ")+(" + boost::lexical_cast<std::string>(static_cast<int>(lsb) - 1) + "):(" +
                      boost::lexical_cast<std::string>(GetPointer<port_o>(cir)->get_id()) + "*" + (BITSIZE_PREFIX + port_name) + "*" + (NUM_ELEM_PREFIX + port_name) + ")+" + boost::lexical_cast<std::string>(lsb) + "]";
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
               return "[" + boost::lexical_cast<std::string>((1 + boost::lexical_cast<int>(GetPointer<port_o>(cir)->get_id())) * static_cast<int>(size_fp) + static_cast<int>(lsb) - 1) + ":" +
                      boost::lexical_cast<std::string>((boost::lexical_cast<int>(GetPointer<port_o>(cir)->get_id())) * static_cast<int>(size_fp) + static_cast<int>(lsb)) + "]";
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

void verilog_writer::write_library_declaration(const structural_objectRef&)
{
}

void verilog_writer::write_module_declaration(const structural_objectRef& cir)
{
   auto* mod = GetPointer<module>(cir);
   THROW_ASSERT(mod, "Expected a module got something of different");
   indented_output_stream->Append("`timescale 1ns / 1ps\n");
   if(HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)) == register_AR_NORETIME || GET_TYPE_NAME(cir) == register_AR_NORETIME_INT || GET_TYPE_NAME(cir) == register_AR_NORETIME_UINT || GET_TYPE_NAME(cir) == register_AR_NORETIME_REAL)
   {
      indented_output_stream->Append("(* keep_hierarchy = \"yes\" *) ");
   }
   indented_output_stream->Append("module " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)) + "(");
   bool first_obj = false;
   /// write IO port declarations respecting the position
   for(unsigned int i = 0; i < mod->get_num_ports(); i++)
   {
      if(first_obj)
         indented_output_stream->Append(", ");
      else
         first_obj = true;
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, mod->get_positional_port(i)->get_id()));
   }
   if(HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)) == register_AR_NORETIME || GET_TYPE_NAME(cir) == register_AR_NORETIME_INT || GET_TYPE_NAME(cir) == register_AR_NORETIME_UINT || GET_TYPE_NAME(cir) == register_AR_NORETIME_REAL)
      indented_output_stream->Append(")/* synthesis syn_hier = \"hard\"*/;\n");
   else
      indented_output_stream->Append(");\n");
   indented_output_stream->Indent();
}

void verilog_writer::write_module_internal_declaration(const structural_objectRef&)
{
}

void verilog_writer::write_assign(const std::string& op0, const std::string& op1)
{
   if(op0 != op1)
      indented_output_stream->Append("  assign " + op0 + " = " + op1 + ";\n");
}

void verilog_writer::write_port_declaration(const structural_objectRef& cir, bool)
{
   THROW_ASSERT(cir->get_kind() == port_o_K || cir->get_kind() == port_vector_o_K, "Expected a port got something of different " + cir->get_id());
   port_o::port_direction dir;
   dir = GetPointer<port_o>(cir)->get_port_direction();
   switch(dir)
   {
      case port_o::IN:
      {
         indented_output_stream->Append("input ");
         break;
      }
      case port_o::OUT:
      {
         indented_output_stream->Append("output ");
         break;
      }
      case port_o::IO:
      {
         indented_output_stream->Append("inout ");
         break;
      }
      case port_o::GEN:
      {
         THROW_ERROR("Generic port not yet supported!");
         break;
      }
      case port_o::TLM_IN:
      case port_o::TLM_INOUT:
      case port_o::TLM_OUT:
      case port_o::UNKNOWN:
      default:
         THROW_ERROR("Something went wrong!");
   }

   indented_output_stream->Append(type_converter(cir->get_typeRef()) + type_converter_size(cir));
   indented_output_stream->Append(HDL_manager::convert_to_identifier(this, cir->get_id()) + ";\n");
}

void verilog_writer::write_component_declaration(const structural_objectRef&)
{
}

void verilog_writer::write_signal_declaration(const structural_objectRef& cir)
{
   THROW_ASSERT(cir->get_kind() == signal_o_K || cir->get_kind() == signal_vector_o_K, "Expected a signal or a signal vector got something of different");
   indented_output_stream->Append("wire " + type_converter(cir->get_typeRef()) + type_converter_size(cir));
   if(cir->get_kind() == signal_vector_o_K and cir->get_typeRef()->type == structural_type_descriptor::BOOL)
   {
      const structural_objectRef first_sig = GetPointer<signal_o>(cir)->get_signal(0);
      structural_type_descriptorRef Type_fs = first_sig->get_typeRef();
      unsigned int n_sign = GetPointer<signal_o>(cir)->get_signals_size();
      unsigned int size_fs = Type_fs->vector_size > 0 ? Type_fs->size * Type_fs->vector_size : Type_fs->size;
      unsigned int lsb = GetPointer<signal_o>(cir)->get_lsb();
      unsigned int msb = size_fs * n_sign + lsb;
      indented_output_stream->Append("[" + boost::lexical_cast<std::string>(msb - 1) + ":" + boost::lexical_cast<std::string>(lsb) + "] ");
   }
   indented_output_stream->Append(HDL_manager::convert_to_identifier(this, cir->get_id()) + ";\n");
}

void verilog_writer::write_module_definition_begin(const structural_objectRef&)
{
   indented_output_stream->Append("\n");
}

void verilog_writer::WriteBuiltin(const structural_objectConstRef component)
{
   const auto mod = GetPointer<const module>(component);
   THROW_ASSERT(mod, component->get_path() + " is not a module");
   THROW_ASSERT(GetPointer<const port_o>(mod->get_out_port(0)), "does not have an output port");
   THROW_ASSERT(component->get_owner(), "does not have an owner");
   THROW_ASSERT(GetPointer<const port_o>(mod->get_out_port(0))->find_bounded_object(component->get_owner()), component->get_path() + " does not have a bounded object");
   const auto object_bounded = GetPointer<const port_o>(mod->get_out_port(0))->find_bounded_object(component->get_owner());
   const auto component_name = GET_TYPE_NAME(component);
   THROW_ASSERT(builtin_to_verilog_keyword.find(component_name) != builtin_to_verilog_keyword.end(), "Verilog keyword corresponding to " + component_name + " not found");
   indented_output_stream->Append(builtin_to_verilog_keyword.find(component_name)->second + " " + builtin_to_verilog_keyword.find(component_name)->second + "_" + component->get_id() + "(");
   THROW_ASSERT(mod->get_out_port_size() == 1, component->get_path() + " has " + STR(mod->get_out_port_size()) + " output ports");
   indented_output_stream->Append(" " + HDL_manager::convert_to_identifier(this, object_bounded->get_id()) + ", ");
   for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
   {
      if(mod->get_in_port(i)->get_kind() == port_o_K)
      {
         THROW_ASSERT(GetPointer<port_o>(mod->get_in_port(i))->find_bounded_object(component->get_owner()), "does not have a structural object connected to the input");
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
                  indented_output_stream->Append(", ");
               const auto vec_in_object_bounded = GetPointer<port_o>(port->get_port(port_index))->find_bounded_object();
               THROW_ASSERT(vec_in_object_bounded, port->get_port(port_index)->get_path());
               indented_output_stream->Append(HDL_manager::convert_to_identifier(this, vec_in_object_bounded->get_id()));
            }
         }
      }
      else
      {
         THROW_UNREACHABLE("");
      }
   }
   indented_output_stream->Append(");\n");
}

void verilog_writer::write_module_instance_begin(const structural_objectRef& cir, const std::string& module_name, bool write_parametrization)
{
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
#if 0
   if(module_name.find("widen_mult_expr") != std::string::npos)
   {
      indented_output_stream->Append("(* dont_touch = \"true\" *) ");
   }
#endif
   indented_output_stream->Append(module_name);

   /// check possible module parametrization
   if(write_parametrization)
      write_module_parametrization(cir);
   indented_output_stream->Append(" " + HDL_manager::convert_to_identifier(this, cir->get_id()) + " (");
}

void verilog_writer::write_module_instance_end(const structural_objectRef&)
{
   indented_output_stream->Append(");\n");
}

void verilog_writer::write_module_definition_end(const structural_objectRef&)
{
   indented_output_stream->Deindent();
   indented_output_stream->Append("\nendmodule\n\n");
}

void verilog_writer::write_vector_port_binding(const structural_objectRef& port, bool& first_port_analyzed)
{
   THROW_ASSERT(port, "NULL object_bounded received");
   THROW_ASSERT(port->get_kind() == port_vector_o_K, "Expected a port vector, got something of different");
   const structural_objectRef p_object_bounded = GetPointer<port_o>(port)->find_bounded_object(port->get_owner());
   if(p_object_bounded)
   {
      if(first_port_analyzed)
         indented_output_stream->Append(", ");
      indented_output_stream->Append(".");
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, port->get_id()));
      indented_output_stream->Append("(");
      if(p_object_bounded->get_kind() == port_vector_o_K)
      {
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, p_object_bounded->get_id()));
      }
      else if(p_object_bounded->get_kind() == signal_vector_o_K)
      {
         indented_output_stream->Append(HDL_manager::convert_to_identifier(this, p_object_bounded->get_id()));
      }
      else
         THROW_ERROR("not expected case");
      indented_output_stream->Append(")");
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Port binding for " + port->get_path() + " not found - looking for single port binding");
      std::string port_binding;
      auto* pv = GetPointer<port_o>(port);
      bool local_first_port_analyzed = false;
      unsigned int msb, lsb;
      msb = std::numeric_limits<unsigned int>::max();
      lsb = std::numeric_limits<unsigned int>::max();
      structural_objectRef slice;
      structural_objectRef null_object;
      unsigned int n_ports = pv->get_ports_size();
      for(unsigned int j = 0; j < n_ports; ++j)
      {
         unsigned int index = n_ports - j - 1;
         structural_objectRef object_bounded = GetPointer<port_o>(pv->get_port(index))->find_bounded_object();
         if(!object_bounded)
            THROW_ERROR("not bounded: " + pv->get_port(index)->get_path());

         if(object_bounded->get_owner()->get_kind() == port_vector_o_K || object_bounded->get_owner()->get_kind() == signal_vector_o_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bounded to a port of a port vector");
            unsigned int vector_position = boost::lexical_cast<unsigned int>(object_bounded->get_id());
            if(slice and slice->get_id() != object_bounded->get_owner()->get_id())
            {
               if(local_first_port_analyzed)
                  port_binding += ", ";
               port_binding += HDL_manager::convert_to_identifier(this, slice->get_id());
               unsigned int max = 0;
               auto* pvs = GetPointer<port_o>(slice);
               if(pvs)
                  max = pvs->get_ports_size();
               else
               {
                  auto* sv = GetPointer<signal_o>(slice);
                  if(sv)
                     max = sv->get_signals_size();
               }
               if(lsb != 0 || msb != (max - 1))
               {
                  port_binding += "[" + boost::lexical_cast<std::string>(msb);
                  if(msb != lsb)
                     port_binding += ":" + boost::lexical_cast<std::string>(lsb);
                  port_binding += "]";
               }
               local_first_port_analyzed = true;
               slice = null_object;
               msb = std::numeric_limits<unsigned int>::max();
            }
            if(!slice || (slice->get_id() == object_bounded->get_owner()->get_id() and (((vector_position + 1) * GET_TYPE_SIZE(object_bounded)) - 1) == lsb - 1))
            {
               slice = object_bounded->get_owner();
               if(msb == std::numeric_limits<unsigned int>::max())
                  msb = (vector_position + 1) * GET_TYPE_SIZE(object_bounded) - 1;
               lsb = vector_position * GET_TYPE_SIZE(object_bounded);
               continue;
            }
         }
         else if(object_bounded->get_kind() == constant_o_K)
         {
            if(local_first_port_analyzed)
               port_binding += ", ";
            if(slice)
            {
               port_binding += HDL_manager::convert_to_identifier(this, slice->get_id());
               unsigned int max = 0;
               auto* pvs = GetPointer<port_o>(slice);
               if(pvs)
                  max = pvs->get_ports_size();
               else
               {
                  auto* sv = GetPointer<signal_o>(slice);
                  if(sv)
                     max = sv->get_signals_size();
               }
               if(lsb != 0 || msb != (max - 1))
               {
                  port_binding += "[" + boost::lexical_cast<std::string>(msb);
                  if(msb != lsb)
                     port_binding += ":" + boost::lexical_cast<std::string>(lsb);
                  port_binding += "], ";
               }
               else if(local_first_port_analyzed)
                  port_binding += ", ";
               slice = null_object;
               msb = std::numeric_limits<unsigned int>::max();
            }
            auto* con = GetPointer<constant_o>(object_bounded);
            std::string trimmed_value = "";
            auto long_value = boost::lexical_cast<unsigned long long int>(con->get_value());
            for(unsigned int ind = 0; ind < GET_TYPE_SIZE(con); ind++)
               trimmed_value = trimmed_value + (((1LLU << (GET_TYPE_SIZE(con) - ind - 1)) & long_value) ? '1' : '0');
            port_binding += STR(GET_TYPE_SIZE(con)) + "'b" + trimmed_value;
         }
         else
         {
            if(local_first_port_analyzed)
               port_binding += ", ";
            if(slice)
            {
               port_binding += HDL_manager::convert_to_identifier(this, slice->get_id());
               unsigned int max = 0;
               auto* pvs = GetPointer<port_o>(slice);
               if(pvs)
                  max = pvs->get_ports_size();
               else
               {
                  auto* sv = GetPointer<signal_o>(slice);
                  if(sv)
                     max = sv->get_signals_size();
               }
               if(lsb != 0 || msb != (max - 1))
               {
                  port_binding += "[" + boost::lexical_cast<std::string>(msb);
                  if(msb != lsb)
                     port_binding += ":" + boost::lexical_cast<std::string>(lsb);
                  port_binding += "], ";
               }
               else if(local_first_port_analyzed)
                  port_binding += ", ";
               slice = null_object;
               msb = std::numeric_limits<unsigned int>::max();
            }
            port_binding += HDL_manager::convert_to_identifier(this, object_bounded->get_id());
         }
         local_first_port_analyzed = true;
      }
      if(slice)
      {
         if(local_first_port_analyzed)
            port_binding += ", ";
         port_binding += HDL_manager::convert_to_identifier(this, slice->get_id());
         unsigned int max = 0;
         auto* pvs = GetPointer<port_o>(slice);
         if(pvs)
            max = pvs->get_ports_size();
         else
         {
            auto* sv = GetPointer<signal_o>(slice);
            if(sv)
               max = sv->get_signals_size();
         }
         if(lsb != 0 || msb != (max - 1))
         {
            port_binding += "[" + boost::lexical_cast<std::string>(msb);
            if(msb != lsb)
               port_binding += ":" + boost::lexical_cast<std::string>(lsb);
            port_binding += "]";
         }
      }

      if(port_binding.size() == 0)
         return;

      if(first_port_analyzed)
         indented_output_stream->Append(", ");
      first_port_analyzed = true;
      indented_output_stream->Append(".");
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, port->get_id()));
      indented_output_stream->Append("({");
      indented_output_stream->Append(port_binding);
      indented_output_stream->Append("})");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Port binding for " + port->get_path() + " not found - looking for single port binding");
   }
}

void verilog_writer::write_port_binding(const structural_objectRef& port, const structural_objectRef& object_bounded, bool& first_port_analyzed)
{
   if(!object_bounded)
      return;
   THROW_ASSERT(port, "NULL object_bounded received");
   THROW_ASSERT(port->get_kind() == port_o_K, "Expected a port got something of different");
   THROW_ASSERT(port->get_owner(), "The port has to have an owner");
   THROW_ASSERT(object_bounded, "NULL object_bounded received for port: " + port->get_path());
   THROW_ASSERT(object_bounded->get_kind() != port_o_K || object_bounded->get_owner(), "A port has to have always an owner");
   if(first_port_analyzed)
      indented_output_stream->Append(", ");
   if(port->get_owner()->get_kind() == port_vector_o_K)
   {
      indented_output_stream->Append(".");
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, port->get_owner()->get_id()) + "[" + port->get_id() + "]");
   }
   else
   {
      indented_output_stream->Append(".");
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, port->get_id()));
   }
   indented_output_stream->Append("(");
   if(object_bounded->get_kind() == port_o_K && object_bounded->get_owner()->get_kind() == port_vector_o_K)
   {
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, object_bounded->get_owner()->get_id()) + type_converter_size(object_bounded));
   }
   else if(object_bounded->get_kind() == signal_o_K && object_bounded->get_owner()->get_kind() == signal_vector_o_K)
   {
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, object_bounded->get_owner()->get_id()) + type_converter_size(object_bounded));
   }
   else if(object_bounded->get_kind() == constant_o_K)
   {
      auto* con = GetPointer<constant_o>(object_bounded);
      std::string trimmed_value = "";
      auto long_value = boost::lexical_cast<unsigned long long int>(con->get_value());
      for(unsigned int ind = 0; ind < GET_TYPE_SIZE(con); ind++)
         trimmed_value = trimmed_value + (((1LLU << (GET_TYPE_SIZE(con) - ind - 1)) & long_value) ? '1' : '0');
      indented_output_stream->Append(STR(GET_TYPE_SIZE(con)) + "'b" + trimmed_value);
   }
   else
      indented_output_stream->Append(HDL_manager::convert_to_identifier(this, object_bounded->get_id()));
   indented_output_stream->Append(")");
   first_port_analyzed = true;
}

void verilog_writer::write_io_signal_post_fix(const structural_objectRef& port, const structural_objectRef& sig)
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
      signal_string = STR(GET_TYPE_SIZE(con)) + "'b" + trimmed_value;
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
      indented_output_stream->Append("assign " + port_string + " = " + signal_string + ";\n");
}

void verilog_writer::write_io_signal_post_fix_vector(const structural_objectRef& port, const structural_objectRef& sig)
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
      indented_output_stream->Append("assign " + port_string + " = " + signal_string + ";\n");
}

void verilog_writer::write_module_parametrization(const structural_objectRef& cir)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing module generics of " + cir->get_path());
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   auto* mod = GetPointer<module>(cir);
   bool first_it = true;
   const NP_functionalityRef& np = mod->get_NP_functionality();

   /// writing memory-related parameters
   if(mod->ExistsParameter(MEMORY_PARAMETER))
   {
      std::string memory_str = mod->GetParameter(MEMORY_PARAMETER);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing memory parameters " + memory_str);
      std::vector<std::string> mem_tag = convert_string_to_vector<std::string>(memory_str, ";");
      for(const auto& i : mem_tag)
      {
         std::vector<std::string> mem_add = convert_string_to_vector<std::string>(i, "=");
         THROW_ASSERT(mem_add.size() == 2, "malformed address");
         if(first_it)
         {
            indented_output_stream->Append(" #(");
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

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Writing ." + name + "(" + name + ")");
         indented_output_stream->Append("." + name + "(" + value + ")");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--written memory parameters");
   }

   if(np)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing np parameters");
      std::vector<std::pair<std::string, structural_objectRef>> library_parameters;
      mod->get_NP_library_parameters(cir, library_parameters);
      for(const auto& library_parameter : library_parameters)
      {
         if(first_it)
         {
            indented_output_stream->Append(" #(");
            first_it = false;
         }
         else
            indented_output_stream->Append(", ");
         const std::string& name = library_parameter.first;
         structural_objectRef obj = library_parameter.second;

         if(!mod->ExistsParameter(std::string(BITSIZE_PREFIX) + name) && obj)
         {
            structural_type_descriptor::s_type type = obj->get_typeRef()->type;
            if((type == structural_type_descriptor::VECTOR_INT || type == structural_type_descriptor::VECTOR_UINT || type == structural_type_descriptor::VECTOR_REAL))
            {
               indented_output_stream->Append("." + std::string(BITSIZE_PREFIX + name) + "(" + boost::lexical_cast<std::string>(obj->get_typeRef()->size) + ")");
               indented_output_stream->Append(", ");
               indented_output_stream->Append("." + std::string(NUM_ELEM_PREFIX + name) + "(" + boost::lexical_cast<std::string>(obj->get_typeRef()->vector_size) + ")");
            }
            else
            {
               indented_output_stream->Append("." + std::string(BITSIZE_PREFIX) + name + "(" + boost::lexical_cast<std::string>(GET_TYPE_SIZE(obj)) + ")");
               if(obj->get_kind() == port_vector_o_K)
               {
                  indented_output_stream->Append(", ");
                  unsigned int ports_size = GetPointer<port_o>(obj)->get_ports_size();
                  indented_output_stream->Append("." + std::string(PORTSIZE_PREFIX) + name + "(" + boost::lexical_cast<std::string>(ports_size) + ")");
               }
            }
         }
         else
         {
            std::string param_value, param_name;
            if(mod->ExistsParameter(name))
            {
               param_value = mod->GetParameter(name);
               param_name = name;
            }
            else if(mod->ExistsParameter(std::string(BITSIZE_PREFIX) + name))
            {
               param_value = mod->GetParameter(std::string(BITSIZE_PREFIX) + name);
               param_name = std::string(BITSIZE_PREFIX) + name;
            }
            else
            {
               cir->print(std::cerr);
               THROW_ERROR("Parameter is missing for port " + name + " in module " + cir->get_path());
            }
            if(param_value.find("\"\"") != std::string::npos)
            {
               boost::replace_all(param_value, "\"\"", "\"");
            }
            else if(param_value.find("\"") != std::string::npos)
            {
               boost::replace_all(param_value, "\"", "");
               param_value = boost::lexical_cast<std::string>(param_value.size()) + "'b" + param_value;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written ." + param_name + "(" + param_value + ")");
            indented_output_stream->Append("." + param_name + "(" + param_value + ")");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written np parameters");
   }

   if(!first_it)
      indented_output_stream->Append(")");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written module generics of " + cir->get_path());
}

void verilog_writer::write_tail(const structural_objectRef&)
{
}

void verilog_writer::write_state_declaration(const structural_objectRef& cir, const std::list<std::string>& list_of_states, const std::string&, const std::string& /*reset_state*/, bool one_hot)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Starting state declaration...");

   auto it_end = list_of_states.end();
   auto n_states = static_cast<unsigned int>(list_of_states.size());
   unsigned int count = 0;
   unsigned int bitsnumber = language_writer::bitnumber(n_states - 1);
   /// adjust in case states are not consecutive
   unsigned max_value = 0;
   for(auto it = list_of_states.begin(); it != it_end; ++it)
   {
      max_value = std::max(max_value, boost::lexical_cast<unsigned int>(it->substr(strlen(STATE_NAME_PREFIX))));
   }
   if(max_value != n_states - 1)
      bitsnumber = language_writer::bitnumber(max_value);
   if(one_hot)
      indented_output_stream->Append("parameter [" + boost::lexical_cast<std::string>(max_value) + ":0] ");
   else
      indented_output_stream->Append("parameter [" + boost::lexical_cast<std::string>(bitsnumber - 1) + ":0] ");
   for(auto it = list_of_states.begin(); it != it_end; ++it)
   {
      if(one_hot)
         indented_output_stream->Append(*it + " = " + boost::lexical_cast<std::string>(max_value + 1) + "'b" + encode_one_hot(1 + max_value, boost::lexical_cast<unsigned int>(it->substr(strlen(STATE_NAME_PREFIX)))));
      else
         indented_output_stream->Append(*it + " = " + boost::lexical_cast<std::string>(bitsnumber) + "'d" + boost::lexical_cast<std::string>(it->substr(strlen(STATE_NAME_PREFIX))));
      count++;
      if(count == n_states)
         ;
      else
      {
         if(count == 1)
            indented_output_stream->Indent();
         indented_output_stream->Append(",\n");
      }
   }
   if(count > 1)
      indented_output_stream->Deindent();
   indented_output_stream->Append(";\n");
   // indented_output_stream->Append("// synthesis attribute init of _present_state is " + reset_state + ";\n");
   // indented_output_stream->Append("// synthesis attribute use_sync_reset of _present_state is no;\n");
   module* mod = GetPointer<module>(cir);
   const NP_functionalityRef& np = mod->get_NP_functionality();
   if(np->exist_NP_functionality(NP_functionality::FSM_CS)) // fsm of context_switch
   {
      if(one_hot)
      {
         indented_output_stream->Append("reg [" + boost::lexical_cast<std::string>(max_value) + ":0] _present_state[" + STR(parameters->getOption<unsigned int>(OPT_context_switch) - 1) + ":0];\n");
         indented_output_stream->Append("reg [" + boost::lexical_cast<std::string>(max_value) + ":0] _next_state;\n");
         // start initializing memory_FSM
         indented_output_stream->Append("integer i;\n");
         indented_output_stream->Append("initial begin\n");
         indented_output_stream->Append("  for (i=0; i<" + STR(parameters->getOption<unsigned int>(OPT_context_switch)) + "; i=i+1) begin\n");
         indented_output_stream->Append("    _present_state[i] = " + boost::lexical_cast<std::string>(max_value + 1) + "'d1;\n");
         indented_output_stream->Append("  end\n");
         indented_output_stream->Append("end\n");
      }
      else
      {
         indented_output_stream->Append("reg [" + boost::lexical_cast<std::string>(bitsnumber - 1) + ":0] _present_state[" + STR(parameters->getOption<unsigned int>(OPT_context_switch) - 1) + ":0];\n");
         indented_output_stream->Append("reg [" + boost::lexical_cast<std::string>(bitsnumber - 1) + ":0] _next_state;\n");
         // start initializing memory_FSM
         indented_output_stream->Append("integer i;\n");
         indented_output_stream->Append("initial begin\n");
         indented_output_stream->Append("  for (i=0; i<" + STR(parameters->getOption<unsigned int>(OPT_context_switch)) + "; i=i+1) begin\n");
         indented_output_stream->Append("    _present_state[i] = " + boost::lexical_cast<std::string>(bitsnumber) + "'d0;\n");
         indented_output_stream->Append("  end\n");
         indented_output_stream->Append("end\n");
      }
   }
   else
   {
      if(one_hot)
         indented_output_stream->Append("reg [" + boost::lexical_cast<std::string>(max_value) + ":0] _present_state, _next_state;\n");
      else
         indented_output_stream->Append("reg [" + boost::lexical_cast<std::string>(bitsnumber - 1) + ":0] _present_state, _next_state;\n");
   }
   THROW_ASSERT(mod, "Expected a component object");
   THROW_ASSERT(mod->get_out_port_size(), "Expected a FSM with at least one output");

   std::string port_name;

   /// enable buffers
   for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
   {
      if(mod->get_out_port(i)->get_id() == PRESENT_STATE_PORT_NAME)
         continue;
      if(mod->get_out_port(i)->get_id() == NEXT_STATE_PORT_NAME)
         continue;
      port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i)->get_id());
      indented_output_stream->Append("reg " + port_name + ";\n");
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Completed state declaration");
}

void verilog_writer::write_present_state_update(const structural_objectRef cir, const std::string& reset_state, const std::string& reset_port, const std::string& clock_port, const std::string& reset_type, bool connect_present_next_state_signals)
{
   if(reset_type == "no" || reset_type == "sync")
      indented_output_stream->Append("always @(posedge " + clock_port + ")\n");
   else if(!parameters->getOption<bool>(OPT_level_reset))
      indented_output_stream->Append("always @(posedge " + clock_port + " or negedge " + reset_port + ")\n");
   else
      indented_output_stream->Append("always @(posedge " + clock_port + " or posedge " + reset_port + ")\n");
   indented_output_stream->Indent();
   /// reset is needed even in case of reset_type == "no"
   module* mod = GetPointer<module>(cir);
   const NP_functionalityRef& np = mod->get_NP_functionality();
   if(np->exist_NP_functionality(NP_functionality::FSM_CS)) // fsm of context_switch
   {
      if(!parameters->getOption<bool>(OPT_level_reset))
         indented_output_stream->Append("if (" + reset_port + " == 1'b0) _present_state[" + STR(SELECTOR_REGISTER_FILE) + "] <= " + reset_state + ";\n");
      else
         indented_output_stream->Append("if (" + reset_port + " == 1'b1) _present_state[" + STR(SELECTOR_REGISTER_FILE) + "] <= " + reset_state + ";\n");
      indented_output_stream->Append("else _present_state[" + STR(SELECTOR_REGISTER_FILE) + "] <= _next_state;\n");
   }
   else
   {
      if(!parameters->getOption<bool>(OPT_level_reset))
         indented_output_stream->Append("if (" + reset_port + " == 1'b0) _present_state <= " + reset_state + ";\n");
      else
         indented_output_stream->Append("if (" + reset_port + " == 1'b1) _present_state <= " + reset_state + ";\n");
      indented_output_stream->Append("else _present_state <= _next_state;\n");
   }
   indented_output_stream->Deindent();
   if(connect_present_next_state_signals)
      indented_output_stream->Append("assign " PRESENT_STATE_PORT_NAME "= _present_state;\nassign " NEXT_STATE_PORT_NAME "= _next_state;\n");
}

void verilog_writer::write_transition_output_functions(bool single_proc, unsigned int output_index, const structural_objectRef& cir, const std::string& reset_state, const std::string& reset_port, const std::string& start_port,
                                                       const std::string& clock_port, std::vector<std::string>::const_iterator& first, std::vector<std::string>::const_iterator& end, bool is_yosys)
{
   typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
   const char soc[3] = {STD_OPENING_CHAR, '\n', '\0'};
   const char scc[3] = {STD_CLOSING_CHAR, '\n', '\0'};
   const char soc1[2] = {STD_OPENING_CHAR, '\0'};
   const char scc1[2] = {STD_CLOSING_CHAR, '\0'};

   boost::char_separator<char> state_sep(":", nullptr);
   boost::char_separator<char> sep(" ", nullptr);

   auto* mod = GetPointer<module>(cir);
   THROW_ASSERT(mod, "Expected a component object");
   THROW_ASSERT(mod->get_out_port_size(), "Expected a FSM with at least one output");
   std::string port_name;

   const NP_functionalityRef& np = mod->get_NP_functionality();
   unsigned int numInputIgnored; // clock,reset,start_port are always present
   if(np->exist_NP_functionality(NP_functionality::FSM_CS))
      numInputIgnored = 4; // added selector
   else
      numInputIgnored = 3;
      /// state transitions description
#ifdef VERILOG_2001_SUPPORTED
   indented_output_stream->Append("\nalways @(*)\nbegin");
#else
   if(np->exist_NP_functionality(NP_functionality::FSM_CS)) // fsm of context_switch
      indented_output_stream->Append("\nalways @(_present_state[" + STR(SELECTOR_REGISTER_FILE) + "]");
   else
      indented_output_stream->Append("\nalways @(_present_state");
   if(mod->get_in_port_size())
   {
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         port_name = HDL_manager::convert_to_identifier(this, mod->get_in_port(i)->get_id());
         if(port_name != reset_port && port_name != clock_port)
            indented_output_stream->Append(" or " + port_name);
      }
   }
   indented_output_stream->Append(")\nbegin");
#endif
   indented_output_stream->Append(soc);

   /// default next_state when multi-process FSMs are considered
   if(!single_proc && output_index == mod->get_out_port_size())
      indented_output_stream->Append("_next_state = " + reset_state + ";\n");

   /// compute the default output
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
      port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i)->get_id());
      indented_output_stream->Append(port_name + " = 1'b0;\n");
   }

   if(np->exist_NP_functionality(NP_functionality::FSM_CS)) // fsm of context_switch
      indented_output_stream->Append("case (_present_state[" + STR(SELECTOR_REGISTER_FILE) + "])");
   else
      indented_output_stream->Append("case (_present_state)");
   indented_output_stream->Append(soc);

   for(auto first_it = first; first_it != end; ++first_it)
   {
      tokenizer state_tokens_first(*first_it, state_sep);

      tokenizer::const_iterator it = state_tokens_first.begin();

      std::string state_description = *it;
      ++it;

      std::vector<std::string> state_transitions;
      for(; it != state_tokens_first.end(); ++it)
         state_transitions.push_back(*it);

      tokenizer tokens_curr(state_description, sep);

      /// get the present state
      it = tokens_curr.begin();
      std::string present_state = HDL_manager::convert_to_identifier(this, *it);
      /// get the current output
      ++it;
      std::string current_output = *it;

      /// check if we can skip this state
      bool skip_state = !single_proc && output_index != mod->get_out_port_size() && default_output[output_index] == current_output[output_index];
      bool skip_state_transition = !single_proc && output_index != mod->get_out_port_size();
      if(!single_proc && output_index != mod->get_out_port_size())
      {
         for(auto current_transition : state_transitions)
         {
            tokenizer transition_tokens(current_transition, sep);
            tokenizer::const_iterator itt = transition_tokens.begin();

            tokenizer::const_iterator current_input_it;
            std::string input_string = *itt;
            if(mod->get_in_port_size() - numInputIgnored) // clock and reset are always present
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

      indented_output_stream->Append(soc1);
      indented_output_stream->Append(present_state + " :\n");

      if(reset_state == present_state)
      {
         indented_output_stream->Append("if(" + start_port + " == 1'b1)\n");
      }

      indented_output_stream->Append("begin");
      indented_output_stream->Append(soc);

      bool unique_transition = (state_transitions.size() == 1);
      if(current_output != default_output && (single_proc || !unique_transition || skip_state_transition))
      {
         for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
         {
            if(mod->get_out_port(i)->get_id() == PRESENT_STATE_PORT_NAME)
               continue;
            if(mod->get_out_port(i)->get_id() == NEXT_STATE_PORT_NAME)
               continue;
            port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i)->get_id());
            if(default_output[i] != current_output[i])
            {
               if(single_proc || output_index == i)
               {
                  switch(current_output[i])
                  {
                     case '1':
                        indented_output_stream->Append(port_name + " = 1'b" + current_output[i] + ";\n");
                        break;

                     case '2':
                        indented_output_stream->Append(port_name + " = 1'bX;\n");
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
         /// check unique-case condition
         bool unique_case_condition = true;
         std::string guard_casez_port;
         unsigned n_bits_guard_casez_port = 0;
         if(!unique_transition)
         {
            for(unsigned int i = 0; i < state_transitions.size(); i++)
            {
               tokenizer transition_tokens(state_transitions[i], sep);
               tokenizer::const_iterator itt = transition_tokens.begin();

               tokenizer::const_iterator current_input_it;
               std::string input_string = *itt;
               if(mod->get_in_port_size() - numInputIgnored)
               {
                  boost::char_separator<char> comma_sep(",", nullptr);
                  tokenizer current_input_tokens(input_string, comma_sep);
                  current_input_it = current_input_tokens.begin();
                  ++itt;
               }
               std::string next_state = *itt;
               ++itt;
               std::string transition_outputs = *itt;
               ++itt;
               THROW_ASSERT(itt == transition_tokens.end(), "Bad transition format");
               if((i + 1) < state_transitions.size())
               {
                  bool first_test = true;
                  for(unsigned int ind = 0; ind < mod->get_in_port_size() && unique_case_condition; ind++)
                  {
                     port_name = HDL_manager::convert_to_identifier(this, mod->get_in_port(ind)->get_id());
                     unsigned int port_size = mod->get_in_port(ind)->get_typeRef()->size;
                     unsigned int vec_size = mod->get_in_port(ind)->get_typeRef()->vector_size;
                     if(port_name != reset_port && port_name != clock_port && port_name != start_port && port_name != STR(SELECTOR_REGISTER_FILE))
                     {
                        std::string in_or_conditions = *current_input_it;
                        boost::char_separator<char> pipe_sep("|", nullptr);
                        tokenizer in_or_conditions_tokens(in_or_conditions, pipe_sep);

                        if((*in_or_conditions_tokens.begin()) != "-")
                        {
                           if(guard_casez_port.empty())
                              guard_casez_port = port_name;
                           else if(guard_casez_port != port_name)
                              unique_case_condition = false;
                           if(!first_test)
                              unique_case_condition = false;
                           else
                              first_test = false;
                           bool first_test_or = true;
                           for(tokenizer::const_iterator in_or_conditions_tokens_it = in_or_conditions_tokens.begin(); in_or_conditions_tokens_it != in_or_conditions_tokens.end() && unique_case_condition; ++in_or_conditions_tokens_it)
                           {
                              THROW_ASSERT((*in_or_conditions_tokens_it) != "-", "wrong conditions structure");
                              if(!first_test_or)
                              {
                                 unique_case_condition = false;
                              }
                              else
                                 first_test_or = false;

                              if((*in_or_conditions_tokens_it)[0] != '&')
                              {
                                 unique_case_condition = false;
                              }
                              else
                              {
                                 if(n_bits_guard_casez_port == 0)
                                    n_bits_guard_casez_port = vec_size == 0 ? port_size : vec_size;
                              }
                           }
                        }
                        ++current_input_it;
                     }
                  }
               }
            }
         }

         if(!unique_transition)
         {
            if(unique_case_condition)
            {
               indented_output_stream->Append("casez (" + guard_casez_port + ")");
               indented_output_stream->Append(soc);
            }
         }
         for(unsigned int i = 0; i < state_transitions.size(); i++)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing transition " + state_transitions[i]);
            tokenizer transition_tokens(state_transitions[i], sep);
            tokenizer::const_iterator itt = transition_tokens.begin();

            tokenizer::const_iterator current_input_it;
            std::string input_string = *itt;
            if(mod->get_in_port_size() - numInputIgnored)
            {
               boost::char_separator<char> comma_sep(",", nullptr);
               tokenizer current_input_tokens(input_string, comma_sep);
               current_input_it = current_input_tokens.begin();
               ++itt;
            }
            std::string next_state = *itt;
            ++itt;
            std::string transition_outputs = *itt;
            ++itt;
            THROW_ASSERT(itt == transition_tokens.end(), "Bad transition format");

            if(!unique_transition)
            {
               if(i == 0)
               {
                  if(unique_case_condition)
                     indented_output_stream->Append(STR(n_bits_guard_casez_port) + "'b");
                  else
                     indented_output_stream->Append("if (");
               }
               else if((i + 1) == state_transitions.size())
               {
                  if(unique_case_condition)
                     indented_output_stream->Append("default");
                  else
                     indented_output_stream->Append("else");
               }
               else
               {
                  if(unique_case_condition)
                     indented_output_stream->Append(STR(n_bits_guard_casez_port) + "'b");
                  else
                     indented_output_stream->Append("else if (");
               }
               if((i + 1) < state_transitions.size())
               {
                  bool first_test = true;
                  for(unsigned int ind = 0; ind < mod->get_in_port_size(); ind++)
                  {
                     port_name = HDL_manager::convert_to_identifier(this, mod->get_in_port(ind)->get_id());
                     unsigned int port_size = mod->get_in_port(ind)->get_typeRef()->size;
                     unsigned int vec_size = mod->get_in_port(ind)->get_typeRef()->vector_size;
                     if(port_name != reset_port && port_name != clock_port && port_name != start_port && port_name != STR(SELECTOR_REGISTER_FILE))
                     {
                        std::string in_or_conditions = *current_input_it;
                        boost::char_separator<char> pipe_sep("|", nullptr);
                        tokenizer in_or_conditions_tokens(in_or_conditions, pipe_sep);

                        if((*in_or_conditions_tokens.begin()) != "-")
                        {
                           if(!first_test)
                              indented_output_stream->Append(" && ");
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
                                 res_or_conditions += " || ";
                                 need_parenthesis = true;
                              }
                              else
                                 first_test_or = false;

                              res_or_conditions += port_name;
                              if((*in_or_conditions_tokens_it)[0] == '&')
                              {
                                 unsigned n_bits = vec_size == 0 ? port_size : vec_size;
                                 auto pos = boost::lexical_cast<unsigned int>((*in_or_conditions_tokens_it).substr(1));
                                 if(unique_case_condition)
                                 {
                                    res_or_conditions = "";
                                    for(unsigned int guard_ind = 0; guard_ind < n_bits; ++guard_ind)
                                       res_or_conditions = (guard_ind == pos ? "1" : (guard_ind < pos ? "0" : (is_yosys ? "0" : "?"))) + res_or_conditions;
                                 }
                                 else
                                    res_or_conditions += (n_bits > 1 ? std::string("[") + STR(pos) + "]" : "") + " == 1'b1";
                              }
                              else
                              {
                                 res_or_conditions += std::string(" == ") + ((*in_or_conditions_tokens_it)[0] == '-' ? "-" : "") + (vec_size == 0 ? boost::lexical_cast<std::string>(port_size) : boost::lexical_cast<std::string>(vec_size));
                                 if(port_size > 1 || (port_size == 1 && vec_size > 0))
                                    res_or_conditions += "'d" + (((*in_or_conditions_tokens_it)[0] == '-') ? ((*in_or_conditions_tokens_it).substr(1)) : *in_or_conditions_tokens_it);
                                 else
                                    res_or_conditions += "'b" + *in_or_conditions_tokens_it;
                              }
                           }
                           if(need_parenthesis)
                              res_or_conditions = "(" + res_or_conditions + ")";
                           indented_output_stream->Append(res_or_conditions);
                        }
                        ++current_input_it;
                     }
                  }
                  if(unique_case_condition)
                     indented_output_stream->Append(" :");
                  else
                     indented_output_stream->Append(")");
               }
               indented_output_stream->Append(soc);
               indented_output_stream->Append("begin");
               indented_output_stream->Append(soc);
            }
            if(single_proc || output_index == mod->get_out_port_size())
               indented_output_stream->Append("_next_state = " + next_state + ";\n");
            for(unsigned int ind = 0; ind < mod->get_out_port_size(); ind++)
            {
               if(mod->get_out_port(ind)->get_id() == PRESENT_STATE_PORT_NAME)
                  continue;
               if(mod->get_out_port(ind)->get_id() == NEXT_STATE_PORT_NAME)
                  continue;
               port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(ind)->get_id());
               if(transition_outputs[ind] != '-')
               {
                  if(single_proc || output_index == ind)
                  {
                     if(transition_outputs[ind] == '2')
                        indented_output_stream->Append(port_name + " = 1'bX;\n");
                     else
                        indented_output_stream->Append(port_name + " = 1'b" + transition_outputs[ind] + ";\n");
                  }
               }
            }
            if(!unique_transition)
            {
               indented_output_stream->Append(scc1);
               indented_output_stream->Append("end");
               indented_output_stream->Append(scc);
            }
         }
         if(!unique_transition)
         {
            if(unique_case_condition)
            {
               indented_output_stream->Append(scc1);
               indented_output_stream->Append("endcase\n");
            }
         }
      }

      indented_output_stream->Append(scc1);
      indented_output_stream->Append("end");

      if(reset_state == present_state)
      {
         indented_output_stream->Append("\nelse");
         indented_output_stream->Append("\nbegin");
         indented_output_stream->Append(soc);

         for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
         {
            if(boost::starts_with(mod->get_out_port(i)->get_id(), "selector_MUX") || boost::starts_with(mod->get_out_port(i)->get_id(), "wrenable_reg"))
            {
               port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i)->get_id());
               if((single_proc || output_index == i) && (parameters->IsParameter("enable-FSMX") && parameters->GetParameter<int>("enable-FSMX") == 1))
                  indented_output_stream->Append(port_name + " = 1'bX;");
               if(single_proc)
                  indented_output_stream->Append("\n");
            }
         }
         if(single_proc || output_index == mod->get_out_port_size())
            indented_output_stream->Append("_next_state = " + present_state + ";");
         indented_output_stream->Append(scc);
         indented_output_stream->Append("end");
      }
      indented_output_stream->Append(scc);
   }

   indented_output_stream->Append(soc1);
   indented_output_stream->Append("default :\n");
   indented_output_stream->Append("begin");
   if(single_proc)
   {
      indented_output_stream->Append(soc);
      indented_output_stream->Append("_next_state = " + reset_state + ";\n");
      for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         if(mod->get_out_port(i)->get_id() == PRESENT_STATE_PORT_NAME)
            continue;
         if(mod->get_out_port(i)->get_id() == NEXT_STATE_PORT_NAME)
            continue;
         if(boost::starts_with(mod->get_out_port(i)->get_id(), "selector_MUX") || boost::starts_with(mod->get_out_port(i)->get_id(), "wrenable_reg"))
         {
            port_name = HDL_manager::convert_to_identifier(this, mod->get_out_port(i)->get_id());
            if((single_proc || output_index == i) && (parameters->IsParameter("enable-FSMX") && parameters->GetParameter<int>("enable-FSMX") == 1))
               indented_output_stream->Append(port_name + " = 1'bX;");
            if(single_proc)
               indented_output_stream->Append("\n");
         }
      }
      indented_output_stream->Append(scc1);
   }
   else
   {
      indented_output_stream->Append("\n");
   }
   indented_output_stream->Append("end");
   indented_output_stream->Append(scc);
   indented_output_stream->Append(scc1);
   indented_output_stream->Append("endcase\n");
   indented_output_stream->Append(scc1);
   indented_output_stream->Append("end");
}

void verilog_writer::write_NP_functionalities(const structural_objectRef& cir)
{
   auto* mod = GetPointer<module>(cir);
   THROW_ASSERT(mod, "Expected a component object");
   const NP_functionalityRef& np = mod->get_NP_functionality();
   THROW_ASSERT(np, "NP Behavioral description is missing for module: " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)));
   std::string beh_desc = np->get_NP_functionality(NP_functionality::VERILOG_PROVIDED);
   THROW_ASSERT(beh_desc != "", "VERILOG behavioral description is missing for module: " + HDL_manager::convert_to_identifier(this, GET_TYPE_NAME(cir)));
   remove_escaped(beh_desc);
   /// manage reset by preprocessing the behavioral description
   if(!parameters->getOption<bool>(OPT_level_reset))
   {
      if(parameters->getOption<std::string>(OPT_sync_reset) == "async")
         boost::replace_all(beh_desc, "1RESET_EDGE", "or negedge " + std::string(RESET_PORT_NAME));
      else
         boost::replace_all(beh_desc, "1RESET_EDGE", "");
      boost::replace_all(beh_desc, "1RESET_VALUE", std::string(RESET_PORT_NAME) + " == 1'b0");
   }
   else
   {
      if(parameters->getOption<std::string>(OPT_sync_reset) == "async")
         boost::replace_all(beh_desc, "1RESET_EDGE", "or posedge " + std::string(RESET_PORT_NAME));
      else
         boost::replace_all(beh_desc, "1RESET_EDGE", "");
      boost::replace_all(beh_desc, "1RESET_VALUE", std::string(RESET_PORT_NAME) + " == 1'b1");
   }
   if(parameters->getOption<bool>(OPT_reg_init_value))
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUE", "=0");
   else
      boost::replace_all(beh_desc, "1INIT_ZERO_VALUE", "");
   indented_output_stream->Append(beh_desc);
}

void verilog_writer::write_port_decl_header()
{
   /// do nothing
}

void verilog_writer::write_port_decl_tail()
{
   /// do nothing
}

void verilog_writer::write_module_parametrization_decl(const structural_objectRef& cir)
{
   THROW_ASSERT(cir->get_kind() == component_o_K || cir->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   auto* mod = GetPointer<module>(cir);
   const NP_functionalityRef& np = mod->get_NP_functionality();
   bool first_it = true;

   /// writing memory-related parameters
   if(mod->ExistsParameter(MEMORY_PARAMETER))
   {
      /// FIXME: this is workaround due to the fact that the default value of MEMORY_PARAMETER is ""
      std::string memory_str = mod->GetParameter(MEMORY_PARAMETER);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "MEMORY_PARAMETER is " + memory_str);
      std::vector<std::string> mem_tag = convert_string_to_vector<std::string>(memory_str, ";");
      for(const auto& i : mem_tag)
      {
         std::vector<std::string> mem_add = convert_string_to_vector<std::string>(i, "=");
         THROW_ASSERT(mem_add.size() == 2, "malformed address");
         if(first_it)
         {
            indented_output_stream->Append("parameter ");
            first_it = false;
         }
         else
         {
            indented_output_stream->Append(", ");
         }
         std::string name = mem_add[0];
         std::string value = mem_add[1];
         if(value.find("\"\"") != std::string::npos)
         {
            boost::replace_all(value, "\"\"", "\"");
         }
         else if(value.find("\"") != std::string::npos)
         {
            boost::replace_all(value, "\"", "");
            value = boost::lexical_cast<std::string>(value.size()) + "'b" + value;
         }
         indented_output_stream->Append(name + "=" + value);
      }
   }

   if(np)
   {
      /// writing other library parameters
      std::vector<std::pair<std::string, structural_objectRef>> library_parameters;
      mod->get_NP_library_parameters(cir, library_parameters);
      for(const auto& library_parameter : library_parameters)
      {
         if(first_it)
         {
            indented_output_stream->Append("parameter ");
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
               indented_output_stream->Append(BITSIZE_PREFIX + name + "=" + boost::lexical_cast<std::string>(obj->get_typeRef()->size));
               indented_output_stream->Append(", ");
               indented_output_stream->Append(NUM_ELEM_PREFIX + name + "=" + boost::lexical_cast<std::string>(obj->get_typeRef()->vector_size));
            }
            else
            {
               indented_output_stream->Append(BITSIZE_PREFIX + name + "=" + boost::lexical_cast<std::string>(GET_TYPE_SIZE(obj)));
               if(obj->get_kind() == port_vector_o_K)
               {
                  indented_output_stream->Append(", ");
                  unsigned int ports_size = GetPointer<port_o>(obj)->get_ports_size();
                  if(ports_size == 0)
                     ports_size = 2;
                  indented_output_stream->Append(PORTSIZE_PREFIX + name + "=" + boost::lexical_cast<std::string>(ports_size));
               }
            }
         }
         else
         {
            std::string param = mod->GetDefaultParameter(name);
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  parameter = #" << name << "#; value = #" << param << "#");
            if(param.find("\"\"") != std::string::npos)
            {
               boost::replace_all(param, "\"\"", "\"");
            }
            else if(param.find("\"") != std::string::npos)
            {
               boost::replace_all(param, "\"", "");
               param = boost::lexical_cast<std::string>(param.size()) + "'b" + param;
            }
            indented_output_stream->Append(name + "=" + param);
         }
      }
   }

   /// if at least one parameter is used, first_it is true.
   if(!first_it)
      indented_output_stream->Append(";\n");
}

verilog_writer::verilog_writer(const ParameterConstRef _parameters) : language_writer(STD_OPENING_CHAR, STD_CLOSING_CHAR, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   for(auto& tokenName : tokenNames)
      keywords.insert(tokenName);
}

verilog_writer::~verilog_writer() = default;

bool verilog_writer::check_keyword(std::string id) const
{
   return keywords.find(id) != keywords.end();
}

void verilog_writer::write_timing_specification(const technology_managerConstRef TM, const structural_objectRef& circ)
{
   module* mod_inst = GetPointer<module>(circ);
   if(mod_inst->get_internal_objects_size() > 0)
      return;
   const NP_functionalityRef& np = mod_inst->get_NP_functionality();
   if(np && (np->exist_NP_functionality(NP_functionality::FSM) or np->exist_NP_functionality(NP_functionality::FSM_CS)))
      return;

   std::string library = TM->get_library(circ->get_typeRef()->id_type);
   const technology_nodeRef& fu = TM->get_fu(circ->get_typeRef()->id_type, library);

   if(!GetPointer<functional_unit>(fu))
      return;

   indented_output_stream->Append("\n");
   write_comment("Timing annotations\n");
   indented_output_stream->Append("specify\n");
   indented_output_stream->Indent();
   const functional_unit::operation_vec& ops = GetPointer<functional_unit>(fu)->get_operations();
   for(unsigned int i = 0; i < ops.size(); i++)
   {
      std::map<std::string, std::map<std::string, double>> delays = GetPointer<operation>(ops[i])->get_pin_to_pin_delay();
      if(delays.size() == 0)
      {
         for(unsigned int out = 0; out < mod_inst->get_out_port_size(); out++)
         {
            for(unsigned int in = 0; in < mod_inst->get_in_port_size(); in++)
            {
               if(ops.size() > 1)
                  indented_output_stream->Append("if (sel_" + GetPointer<operation>(ops[i])->operation_name + " == 1'b1) ");
               THROW_ASSERT(GetPointer<operation>(ops[i])->time_m, "the operation has not any timing information associated with");
               indented_output_stream->Append("(" + mod_inst->get_in_port(in)->get_id() + " *> " + mod_inst->get_out_port(out)->get_id() + ") = " + STR(GetPointer<operation>(ops[i])->time_m->get_execution_time()) + ";\n");
            }
         }
      }
      else
      {
         for(auto& delay : delays)
         {
            for(auto o = delay.second.begin(); o != delay.second.end(); ++o)
            {
               if(ops.size() > 1)
                  indented_output_stream->Append("if (sel_" + GetPointer<operation>(ops[i])->operation_name + " == 1'b1) ");
               indented_output_stream->Append("(" + delay.first + " *> " + o->first + ") = " + STR(o->second) + ";\n");
            }
         }
      }
   }
   indented_output_stream->Deindent();
   indented_output_stream->Append("endspecify\n\n");
}

void verilog_writer::write_header()
{
   indented_output_stream->Append("`ifdef __ICARUS__\n");
   indented_output_stream->Append("  `define _SIM_HAVE_CLOG2\n");
   indented_output_stream->Append("`endif\n");
   indented_output_stream->Append("`ifdef VERILATOR\n");
   indented_output_stream->Append("  `define _SIM_HAVE_CLOG2\n");
   indented_output_stream->Append("`endif\n");
   indented_output_stream->Append("`ifdef MODEL_TECH\n");
   indented_output_stream->Append("  `define _SIM_HAVE_CLOG2\n");
   indented_output_stream->Append("`endif\n");
   indented_output_stream->Append("`ifdef VCS\n");
   indented_output_stream->Append("  `define _SIM_HAVE_CLOG2\n");
   indented_output_stream->Append("`endif\n");
   indented_output_stream->Append("`ifdef NCVERILOG\n");
   indented_output_stream->Append("  `define _SIM_HAVE_CLOG2\n");
   indented_output_stream->Append("`endif\n");
   indented_output_stream->Append("`ifdef XILINX_SIMULATOR\n");
   indented_output_stream->Append("  `define _SIM_HAVE_CLOG2\n");
   indented_output_stream->Append("`endif\n");
   indented_output_stream->Append("`ifdef XILINX_ISIM\n");
   indented_output_stream->Append("  `define _SIM_HAVE_CLOG2\n");
   indented_output_stream->Append("`endif\n\n");
}
