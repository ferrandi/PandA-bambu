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
 * @file HDL_manager.cpp
 * @brief Implementation of the base methods for writing HDL descriptions
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "HDL_manager.hpp"

/// Autoheader include
#include "config_HAVE_ASSERTS.hpp" // for HAVE_ASSERTS
#include "config_HAVE_FLOPOCO.hpp"
#include "config_PACKAGE_BUGREPORT.hpp"
#include "config_PACKAGE_NAME.hpp"
#include "config_PACKAGE_VERSION.hpp"

#include "NP_functionality.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "FPGA_device.hpp"
#include "target_device.hpp"

#if HAVE_FLOPOCO
#include "flopoco_wrapper.hpp"
#endif

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iosfwd>

/// boost include
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>

/// design_flows include
#include "design_flow_manager.hpp"

/// design_flows/backend/ToHDL includes
#include "VHDL_writer.hpp"
#include "verilog_writer.hpp"

/// HLS include
#include "hls_manager.hpp"

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <utility>

/// technology includes
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "technology_manager.hpp"
#include "technology_node.hpp"

HDL_manager::HDL_manager(const HLS_managerRef _HLSMgr, const target_deviceRef _device, const structural_managerRef _SM, const ParameterConstRef _parameters)
    : HLSMgr(_HLSMgr),
      device(_device),
      TM(_device->get_technology_manager()),
#if HAVE_FLOPOCO
      flopo_wrap(new flopoco_wrapper(_parameters->getOption<int>(OPT_debug_level), _device->get_parameter<std::string>("family"))),
#endif
      SM(_SM),
      parameters(_parameters),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

HDL_manager::HDL_manager(const HLS_managerRef _HLSMgr, const target_deviceRef _device, const ParameterConstRef _parameters)
    : HLSMgr(_HLSMgr),
      device(_device),
      TM(_device->get_technology_manager()),
#if HAVE_FLOPOCO
      flopo_wrap(new flopoco_wrapper(_parameters->getOption<int>(OPT_debug_level), _device->get_parameter<std::string>("family"))),
#endif
      parameters(_parameters),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

HDL_manager::~HDL_manager() = default;

std::string HDL_manager::write_components(const std::string& filename, HDLWriter_Language language, const std::list<structural_objectRef>& components, bool equation, std::list<std::string>& aux_files) const
{
   language_writerRef writer = language_writer::create_writer(language, TM, parameters);

   writer->write_comment(std::string("\n"));
   writer->write_comment(std::string("Politecnico di Milano\n"));
   writer->write_comment(std::string("Code created using ") + PACKAGE_NAME + " - " + parameters->PrintVersion() + std::string(" - Date " + TimeStamp::GetCurrentTimeStamp()) + "\n");
   if(parameters->isOption(OPT_cat_args))
      writer->write_comment(parameters->getOption<std::string>(OPT_program_name) + " executed with: " + parameters->getOption<std::string>(OPT_cat_args) + "\n");

   writer->write_comment("\n");
   writer->write_comment(std::string("Send any bug to: ") + PACKAGE_BUGREPORT + "\n");
   writer->WriteLicense();

   /// write the header of the file
   writer->write_header();

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing components");
   /// write all modules
   for(const auto& c : components)
   {
      NP_functionalityRef npf = GetPointer<module>(c)->get_NP_functionality();
      if(npf and npf->get_NP_functionality(NP_functionality::FLOPOCO_PROVIDED) != "")
      {
         write_module(writer, c, equation, aux_files);
         continue;
      }
      else if(npf and (npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED) != "" || npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED) != ""))
      {
         if(npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED) != "" && language == HDLWriter_Language::VERILOG)
         {
            std::string filename_HDL = GetPath(npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED));
            if(std::find(aux_files.begin(), aux_files.end(), filename_HDL) == aux_files.end())
               aux_files.push_back(filename_HDL);
         }
         else if(npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED) != "" && language == HDLWriter_Language::VHDL)
         {
            std::string filename_HDL = GetPath(npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED));
            if(std::find(aux_files.begin(), aux_files.end(), filename_HDL) == aux_files.end())
               aux_files.push_back(filename_HDL);
         }
         else if(npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED) != "")
         {
            std::string filename_HDL = GetPath(npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED));
            if(std::find(aux_files.begin(), aux_files.end(), filename_HDL) == aux_files.end())
               aux_files.push_back(filename_HDL);
         }
         else if(npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED) != "")
         {
            std::string filename_HDL = GetPath(npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED));
            if(std::find(aux_files.begin(), aux_files.end(), filename_HDL) == aux_files.end())
               aux_files.push_back(filename_HDL);
         }
         else
            THROW_UNREACHABLE("unexpected condition");
         continue;
      }
      std::string library = TM->get_library(c->get_typeRef()->id_type);
      structural_objectRef obj = c;
      /// we write the definition of the object stored in library
      if(library.size())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Component " + c->get_typeRef()->id_type + " is in library " + library);
         technology_nodeRef tn = TM->get_fu(c->get_typeRef()->id_type, library);
         if(GetPointer<functional_unit>(tn))
         {
            THROW_ASSERT(GetPointer<functional_unit>(tn)->CM, tn->get_name());
            obj = GetPointer<functional_unit>(tn)->CM->get_circ();
         }
         else if(GetPointer<functional_unit_template>(tn))
         {
            technology_nodeRef FU = GetPointer<functional_unit_template>(tn)->FU;
            obj = GetPointer<functional_unit>(FU)->CM->get_circ();
         }
         else
            THROW_ERROR("unexpected condition");
      }
      write_module(writer, obj, equation, aux_files);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written components");
   /// write the tail of the file
   writer->write_tail(structural_objectRef());
   auto filename_ext = GetPath(filename + writer->get_extension());
   writer->WriteFile(filename_ext);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written " + filename_ext);
   return filename_ext;
}

void HDL_manager::write_components(const std::string& filename, const std::list<structural_objectRef>& components, bool equation, std::list<std::string>& hdl_files, std::list<std::string>& aux_files)
{
   /// default language
   auto language = static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Everything seems ok. Let's start with the real job.");

   NP_functionality::NP_functionaly_type type = NP_functionality::UNKNOWN;
   if(language == HDLWriter_Language::VERILOG)
   {
      type = NP_functionality::VERILOG_PROVIDED;
   }
   else if(language == HDLWriter_Language::SYSTEM_VERILOG)
   {
      type = NP_functionality::SYSTEM_VERILOG_PROVIDED;
   }
   else if(language == HDLWriter_Language::VHDL)
   {
      type = NP_functionality::VHDL_PROVIDED;
   }
   else
      THROW_ERROR("Language not supported");

   /// determine the proper language for each component
   std::map<HDLWriter_Language, std::list<structural_objectRef>> component_language;
   for(const auto& component : components)
   {
      auto* mod = GetPointer<module>(component);
      THROW_ASSERT(mod, "Expected a component object");

      unsigned int n_elements = mod->get_internal_objects_size();

      const NP_functionalityRef np = mod->get_NP_functionality();

      if(n_elements || (np && (np->exist_NP_functionality(type) || (language == HDLWriter_Language::VERILOG && np->exist_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED)) ||
                               (language == HDLWriter_Language::VHDL && np->exist_NP_functionality(NP_functionality::VHDL_PROVIDED)))))
      {
         component_language[language].push_back(component);
      }
      else
      {
         if(np and (np->exist_NP_functionality(NP_functionality::FSM) or np->exist_NP_functionality(NP_functionality::FSM_CS)))
         {
            component_language[language].push_back(component);
         }
         else if(np && (np->exist_NP_functionality(NP_functionality::VERILOG_PROVIDED) || np->exist_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED)))
         {
#if HAVE_EXPERIMENTAL
            const auto module_type = mod->get_typeRef()->id_type;
            const auto fu = GetPointer<functional_unit>(TM->get_fu(module_type, TM->get_library(module_type)));
            if(not parameters->getOption<bool>(OPT_mixed_design))
            {
               THROW_ERROR("VHDL implementation of " + (*cit)->get_path() + " - type " + module_type + " is not available");
            }
            else
            {
               if(module_type.find("gimple_asm") == std::string::npos and module_type.find("__builtin_trap") == std::string::npos and module_type.find("return_value_mm_register") == std::string::npos and
                  module_type.find("notify_caller_minimal") == std::string::npos and module_type.find("memory_mapped_register") == std::string::npos and module_type.find("status_register") == std::string::npos and
                  module_type.find("__builtin_wait_call") == std::string::npos and module_type.find("PROXY_CTRL") == std::string::npos and module_type.find("PRINTF") == std::string::npos and module_type.find("trunc_") == std::string::npos and
                  module_type.find("__builtin_memstore") == std::string::npos and module_type != "ui_mult_expr_FU" and module_type != "mult_expr_FU" and module_type.find("ADDRESS_DECODING_LOGIC") == std::string::npos and
                  module_type.find("BRAM") == std::string::npos and module_type != "STD_N21_BYTEMUX" and module_type != "STD_LIVE_VALUE_TABLE" and module_type.find("MEMORY_CTRL") == std::string::npos and module_type != "register_SARSE" and
                  module_type.find("DISTRAM") == std::string::npos and (not fu or (fu->memory_type == "" or fu->memory_ctrl_type == "")))
               {
                  // THROW_UNREACHABLE("VHDL implementation of " + module_type + " not found");
               }
            }
#endif
            component_language[HDLWriter_Language::VERILOG].push_back(component);
         }
         else if(np && (np->exist_NP_functionality(NP_functionality::VHDL_PROVIDED) || np->exist_NP_functionality(NP_functionality::FLOPOCO_PROVIDED) || np->exist_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED)))
         {
#if HAVE_EXPERIMENTAL
            if(not parameters->getOption<bool>(OPT_mixed_design))
            {
               THROW_ERROR("Verilog implementation of " + (*cit)->get_path() + " is not available");
            }
            else
#endif
               THROW_WARNING(component->get_path() + " is available only in VHDL");
            component_language[HDLWriter_Language::VHDL].push_back(component);
         }
         else if(np && np->exist_NP_functionality(NP_functionality::SYSTEM_VERILOG_PROVIDED))
         {
            component_language[HDLWriter_Language::SYSTEM_VERILOG].push_back(component);
         }
         else
            THROW_ERROR("Language not supported! Module " + mod->get_path());
      }
   }

   /// generate the auxiliary files
   for(auto l = component_language.begin(); l != component_language.end(); ++l)
   {
      if(language == l->first)
         continue;
      std::string generated_filename = write_components(filename, l->first, component_language[l->first], equation, aux_files);
      aux_files.push_back(generated_filename);
   }

   std::string complete_filename = write_components(filename, language, component_language[language], equation, aux_files);
   /// add the generated file to the global list
   hdl_files.push_back(complete_filename);

#if HAVE_FLOPOCO
   /// as a last thing to do we wrote all the common FloPoCo components
   const std::string flopoco_common = flopo_wrap->writeVHDLcommon();
   if(flopoco_common != "")
      aux_files.push_back(flopoco_common);
#endif
}

void HDL_manager::hdl_gen(const std::string& filename, const std::list<structural_objectRef>& cirs, bool equation, std::list<std::string>& hdl_files, std::list<std::string>& aux_files)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  compute the list of components for which a structural description exists");
   /// compute the list of components for which a structural description exist.
   std::list<structural_objectRef> list_of_com;
   for(const auto& cir : cirs)
      get_post_order_structural_components(cir, list_of_com);
   if(list_of_com.empty())
   {
#if HAVE_ASSERTS
      for(const auto& cir : cirs)
      {
         THROW_ASSERT(GetPointer<module>(cir), "Expected a component or a channel");
         THROW_ASSERT(GetPointer<module>(cir)->get_NP_functionality(), "Structural empty description received");
      }
#endif
      return;
   }

   /// generate the HDL descriptions for all the components
   write_components(filename, list_of_com, equation, hdl_files, aux_files);
   return;
}

/**
 * Used to test if a component has been already inserted into the component list
 */
struct find_eq_module
{
   // predicate function
   bool operator()(const structural_objectRef& el)
   {
      return el == target || HDL_manager::get_mod_typename(lan, el) == HDL_manager::get_mod_typename(lan, target);
   }
   find_eq_module(const language_writer* _lan, const structural_objectRef& _target) : lan(_lan), target(_target)
   {
      THROW_ASSERT(_target, "structural_objectRef must exist");
   }

 private:
   const language_writer* lan;
   const structural_objectRef& target;
};

bool HDL_manager::is_fsm(const structural_objectRef& cir) const
{
   /// check for a fsm description
   auto* mod_inst = GetPointer<module>(cir);
   THROW_ASSERT(mod_inst, "Expected a component or a channel");
   const NP_functionalityRef& np = mod_inst->get_NP_functionality();
   if(np)
   {
      return (np->exist_NP_functionality(NP_functionality::FSM) or np->exist_NP_functionality(NP_functionality::FSM_CS));
   }
   return false;
}

void HDL_manager::get_post_order_structural_components(const structural_objectRef cir, std::list<structural_objectRef>& list_of_com) const
{
   switch(cir->get_kind())
   {
      case component_o_K:
      case channel_o_K:
      {
         auto* mod = GetPointer<module>(cir);
         unsigned int n_elements = mod->get_internal_objects_size();
         if(n_elements)
         {
            for(unsigned int i = 0; i < n_elements; i++)
            {
               switch(mod->get_internal_object(i)->get_kind())
               {
                  case channel_o_K:
                  case component_o_K:
                  {
                     if(!mod->get_internal_object(i)->get_black_box() and not TM->IsBuiltin(GET_TYPE_NAME(mod->get_internal_object(i))))
                        get_post_order_structural_components(mod->get_internal_object(i), list_of_com);
                     break;
                  }
                  case constant_o_K:
                  case signal_vector_o_K:
                  case signal_o_K:
                  case bus_connection_o_K:
                     break; /// no action for signals and bus
                  case action_o_K:
                  case data_o_K:
                  case event_o_K:
                  case port_o_K:
                  case port_vector_o_K:
                  default:
                     THROW_ERROR("Structural object not foreseen: " + std::string(mod->get_internal_object(i)->get_kind_text()));
               }
            }
         }
         NP_functionalityRef NPF = mod->get_NP_functionality();
         if(NPF and NPF->exist_NP_functionality(NP_functionality::IP_COMPONENT))
         {
            std::string ip_cores = NPF->get_NP_functionality(NP_functionality::IP_COMPONENT);
            std::vector<std::string> ip_cores_list = convert_string_to_vector<std::string>(ip_cores, ",");
            for(auto ip_core : ip_cores_list)
            {
               std::vector<std::string> ip_core_vec = convert_string_to_vector<std::string>(ip_core, ":");
               if(ip_core_vec.size() < 1 or ip_core_vec.size() > 2)
                  THROW_ERROR("Malformed IP component definition \"" + ip_core + "\"");
               std::string library, component_name;
               if(ip_core_vec.size() == 2)
               {
                  library = ip_core_vec[0];
                  component_name = ip_core_vec[1];
               }
               else
               {
                  component_name = ip_core_vec[0];
                  library = TM->get_library(component_name);
               }
               technology_nodeRef tn = TM->get_fu(component_name, library);
               structural_objectRef core_cir;
               if(tn->get_kind() == functional_unit_K)
                  core_cir = GetPointer<functional_unit>(tn)->CM->get_circ();
               else if(tn->get_kind() == functional_unit_template_K && GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU))
                  core_cir = GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->CM->get_circ();
               else
                  THROW_ERROR("Unexpected pattern");
               get_post_order_structural_components(core_cir, list_of_com);
               auto fo = std::find_if(list_of_com.begin(), list_of_com.end(), find_eq_module(nullptr, core_cir));
               if(fo == list_of_com.end())
               {
                  list_of_com.push_back(core_cir);
               }
            }
         }
         auto fo = std::find_if(list_of_com.begin(), list_of_com.end(), find_eq_module(nullptr, cir));
         if(fo == list_of_com.end())
            list_of_com.push_back(cir);
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Structural object not foreseen");
   }
}

void HDL_manager::io_signal_fix_ith(const language_writerRef writer, const structural_objectRef po, bool& lspf) const
{
   THROW_ASSERT(po && po->get_kind() == port_o_K, "Expected a port; got something different");
   auto* p = GetPointer<port_o>(po);
   structural_objectRef po_owner = po->get_owner();
   if(po_owner->get_kind() == port_vector_o_K)
      po_owner = po_owner->get_owner();
   for(unsigned int j = 0; j < p->get_connections_size(); j++)
   {
      if(p->get_connection(j)->get_kind() == signal_o_K and (p->get_connection(j)->get_owner() == po_owner or (p->get_connection(j)->get_owner()->get_kind() == signal_vector_o_K and p->get_connection(j)->get_owner()->get_owner() == po_owner)))
      {
         if(!lspf)
         {
            writer->write_comment("io-signal post fix\n");
            lspf = true;
         }
         writer->write_io_signal_post_fix(po, p->get_connection(j));
      }
      if(p->get_connection(j)->get_kind() == constant_o_K and p->get_connection(j)->get_owner() == po_owner)
      {
         if(!lspf)
         {
            writer->write_comment("io-signal post fix\n");
            lspf = true;
         }
         writer->write_io_signal_post_fix(po, p->get_connection(j));
      }
   }
}

void HDL_manager::io_signal_fix_ith_vector(const language_writerRef writer, const structural_objectRef po, bool& lspf) const
{
   THROW_ASSERT(po && po->get_kind() == port_vector_o_K, "Expected a port; got something different");
   auto* p = GetPointer<port_o>(po);
   THROW_ASSERT(p, "Expected a port; got something different");
   structural_objectRef po_owner = po->get_owner();
   for(unsigned int j = 0; j < p->get_connections_size(); j++)
   {
      if(p->get_connection(j)->get_kind() == signal_vector_o_K and p->get_connection(j)->get_owner() == po_owner)
      {
         if(!lspf)
         {
            writer->write_comment("io-signal post fix\n");
            lspf = true;
         }
         writer->write_io_signal_post_fix_vector(po, p->get_connection(j));
      }
      if(p->get_connection(j)->get_kind() == constant_o_K and p->get_connection(j)->get_owner() == po_owner)
      {
         THROW_ERROR("unexpected condition");
      }
   }
}

void HDL_manager::write_module(const language_writerRef writer, const structural_objectRef cir, bool equation, std::list<std::string>& aux_files) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing module " + GET_TYPE_NAME(cir));
   const module* mod = GetPointer<module>(cir);
   THROW_ASSERT(mod, "Expected a module got something of different");

   const NP_functionalityRef& np = mod->get_NP_functionality();
   if(np && np->get_NP_functionality(NP_functionality::FLOPOCO_PROVIDED) != "")
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, this->debug_level, "FloPoCo compliant module: " + mod->get_id());
      this->write_flopoco_module(cir, aux_files);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written module " + cir->get_path());
      return;
   }

   /// write module declaration
   writer->write_comment(mod->get_description() + "\n");
   writer->write_comment(mod->get_copyright() + "\n");
   writer->write_comment("Author(s): " + mod->get_authors() + "\n");
   writer->write_comment("License: " + mod->get_license() + "\n");

   /// write library declaration component
   writer->write_library_declaration(cir);

   writer->write_module_declaration(cir);

   writer->write_module_parametrization_decl(cir);

   writer->write_port_decl_header();

   /// write IO port declarations
   if(mod->get_in_port_size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing input port declaration");
      writer->write_comment("IN\n");
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing port declaration " + mod->get_in_port(i)->get_id() + " of type " + mod->get_in_port(i)->get_kind_text());
         if(i == mod->get_in_port_size() - 1 && !mod->get_out_port_size() && !mod->get_in_out_port_size() && !mod->get_gen_port_size())
            writer->write_port_declaration(mod->get_in_port(i), true);
         else
            writer->write_port_declaration(mod->get_in_port(i), false);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written input port declaration");
   }
   if(mod->get_out_port_size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing output port declaration");
      writer->write_comment("OUT\n");
      for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         if(i == mod->get_out_port_size() - 1 && !mod->get_in_out_port_size() && !mod->get_gen_port_size())
            writer->write_port_declaration(mod->get_out_port(i), true);
         else
            writer->write_port_declaration(mod->get_out_port(i), false);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written output port declaration");
   }
   if(mod->get_in_out_port_size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing inout port declaration");
      writer->write_comment("INOUT\n");
      for(unsigned int i = 0; i < mod->get_in_out_port_size(); i++)
      {
         if(i == mod->get_in_out_port_size() - 1 && !mod->get_gen_port_size())
            writer->write_port_declaration(mod->get_in_out_port(i), true);
         else
            writer->write_port_declaration(mod->get_in_out_port(i), false);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written inout port declaration");
   }

   if(mod->get_gen_port_size())
   {
      writer->write_comment("Ports\n");
      for(unsigned int i = 0; i < mod->get_gen_port_size(); i++)
      {
         if(i == mod->get_gen_port_size() - 1)
            writer->write_port_declaration(mod->get_gen_port(i), true);
         else
            writer->write_port_declaration(mod->get_gen_port(i), false);
      }
   }

   writer->write_port_decl_tail();

   /// close the interface declaration and start the implementation
   writer->write_module_internal_declaration(cir);

   /// specify the timing annotations to the components
   if(parameters->getOption<bool>(OPT_timing_simulation))
   {
      /// specify timing characterization
      writer->write_timing_specification(TM, cir);
   }

   if(equation)
   {
      std::string behav = np->get_NP_functionality(NP_functionality::EQUATION);
      write_behavioral(writer, cir, behav);
   }
   else
   {
      /// write components declarations
      /// write signal declarations
      unsigned int n_elements = mod->get_internal_objects_size();
      if(n_elements)
      {
         writer->write_comment("Component and signal declarations\n");

         std::list<std::pair<std::string, structural_objectRef>> cs;
         for(unsigned int i = 0; i < n_elements; i++)
         {
            switch(mod->get_internal_object(i)->get_kind())
            {
               case constant_o_K:
               {
                  continue;
               }
               case channel_o_K:
               case component_o_K:
               {
                  break;
               }
               case signal_vector_o_K:
               case signal_o_K:
               {
                  cs.push_back(std::make_pair(mod->get_internal_object(i)->get_id(), mod->get_internal_object(i)));
                  continue;
               }
               case bus_connection_o_K:
                  THROW_ERROR("Bus connection not yes supported.");
               case action_o_K:
               case data_o_K:
               case event_o_K:
               case port_o_K:
               case port_vector_o_K:
               default:; // do nothing
            }
            writer->write_component_declaration(mod->get_internal_object(i));
         }
         cs.sort();

         for(auto& c : cs)
         {
            writer->write_signal_declaration(c.second);
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing module instantiation of " + cir->get_id());

         /// write module_instantiation begin
         writer->write_module_definition_begin(cir);

         cs.clear();
         /// write module instantiation & connection binding
         for(unsigned int i = 0; i < n_elements; i++)
         {
            switch(mod->get_internal_object(i)->get_kind())
            {
               case channel_o_K:
               case component_o_K:
               {
                  cs.push_back(std::make_pair(mod->get_internal_object(i)->get_id(), mod->get_internal_object(i)));
                  break;
               }
               case action_o_K:
               case bus_connection_o_K:
               case constant_o_K:
               case data_o_K:
               case event_o_K:
               case port_o_K:
               case port_vector_o_K:
               case signal_o_K:
               case signal_vector_o_K:
               default:; // do nothing
            }
         }
         cs.sort();

         for(auto& c : cs)
         {
            structural_objectRef obj = c.second;
            if(TM->IsBuiltin(GET_TYPE_NAME(obj)))
            {
               writer->WriteBuiltin(obj);
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing module instance " + obj->get_id());
               writer->write_module_instance_begin(obj, get_mod_typename(writer.get(), obj), true);
               /// write IO ports binding
               auto* mod_inst = GetPointer<module>(obj);
               bool first_port_analyzed = false;
               /// First output and then input. Some backend could have benefits from this ordering.
               /// Some customization are possible, like direct translation of gates into built-in statements.
               if(writer->has_output_prefix())
               {
                  if(mod_inst->get_out_port_size())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing output ports binding");
                     // writer->write_comment("OUT binding\n");
                     for(unsigned int i = 0; i < mod_inst->get_out_port_size(); i++)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing port binding of " + mod_inst->get_out_port(i)->get_id());
                        if(mod_inst->get_out_port(i)->get_kind() == port_o_K)
                        {
                           const structural_objectRef object_bounded = GetPointer<port_o>(mod_inst->get_out_port(i))->find_bounded_object(cir);
                           if(!object_bounded)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped " + mod_inst->get_out_port(i)->get_path());
                              continue;
                           }
                           writer->write_port_binding(mod_inst->get_out_port(i), object_bounded, first_port_analyzed);
                        }
                        else
                        {
                           writer->write_vector_port_binding(mod_inst->get_out_port(i), first_port_analyzed);
                        }
                        first_port_analyzed = true;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written port binding of " + mod_inst->get_out_port(i)->get_id());
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written output ports binding");
                  }
               }
               if(mod_inst->get_in_port_size())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing input ports binding");
                  // writer->write_comment("IN binding\n");
                  for(unsigned int i = 0; i < mod_inst->get_in_port_size(); i++)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing port binding of " + mod_inst->get_in_port(i)->get_id());
                     if(mod_inst->get_in_port(i)->get_kind() == port_o_K)
                     {
                        const structural_objectRef object_bounded = GetPointer<port_o>(mod_inst->get_in_port(i))->find_bounded_object(cir);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bounded object is " + (object_bounded ? object_bounded->get_path() : " nothing"));
                        writer->write_port_binding(mod_inst->get_in_port(i), object_bounded, first_port_analyzed);
                     }
                     else
                     {
                        writer->write_vector_port_binding(mod_inst->get_in_port(i), first_port_analyzed);
                     }
                     first_port_analyzed = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written port binding of " + mod_inst->get_in_port(i)->get_id());
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written input ports binding");
               }
               if(mod_inst->get_in_out_port_size())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing inout ports binding");
                  // writer->write_comment("INOUT binding\n");
                  for(unsigned int i = 0; i < mod_inst->get_in_out_port_size(); i++)
                  {
                     if(mod_inst->get_in_out_port(i)->get_kind() == port_o_K)
                     {
                        const structural_objectRef object_bounded = GetPointer<port_o>(mod_inst->get_in_out_port(i))->find_bounded_object();
                        if(!object_bounded)
                           continue;
                        writer->write_port_binding(mod_inst->get_in_out_port(i), object_bounded, first_port_analyzed);
                     }
                     else
                     {
                        writer->write_vector_port_binding(mod_inst->get_in_out_port(i), first_port_analyzed);
                     }
                     first_port_analyzed = true;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written inout ports binding");
               }
               if(mod_inst->get_gen_port_size())
               {
                  // writer->write_comment("Ports binding\n");
                  for(unsigned int i = 0; i < mod_inst->get_gen_port_size(); i++)
                  {
                     if(mod_inst->get_gen_port(i)->get_kind() == port_o_K)
                        writer->write_port_binding(mod_inst->get_gen_port(i), GetPointer<port_o>(mod_inst->get_gen_port(i))->find_bounded_object(), first_port_analyzed);
                     else
                     {
                        writer->write_vector_port_binding(mod_inst->get_gen_port(i), first_port_analyzed);
                     }
                     first_port_analyzed = true;
                  }
               }
               if(!writer->has_output_prefix())
               {
                  if(mod_inst->get_out_port_size())
                  {
                     // writer->write_comment("OUT binding\n");
                     for(unsigned int i = 0; i < mod_inst->get_out_port_size(); i++)
                     {
                        if(mod_inst->get_out_port(i)->get_kind() == port_o_K)
                        {
                           const structural_objectRef object_bounded = GetPointer<port_o>(mod_inst->get_out_port(i))->find_bounded_object();
                           if(!object_bounded)
                              continue;
                           writer->write_port_binding(mod_inst->get_out_port(i), object_bounded, first_port_analyzed);
                        }
                        else
                        {
                           writer->write_vector_port_binding(mod_inst->get_out_port(i), first_port_analyzed);
                        }
                        first_port_analyzed = true;
                     }
                  }
               }

               writer->write_module_instance_end(obj);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written module instance " + obj->get_id());
            }
         }

         /// write loop signal post fix
         bool lspf = false;
         if(mod->get_in_port_size())
         {
            for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
            {
               if(mod->get_in_port(i)->get_kind() == port_o_K)
                  io_signal_fix_ith(writer, mod->get_in_port(i), lspf);
               else
               {
                  io_signal_fix_ith_vector(writer, mod->get_in_port(i), lspf);
                  auto* pv = GetPointer<port_o>(mod->get_in_port(i));
                  for(unsigned int k = 0; k < pv->get_ports_size(); k++)
                     io_signal_fix_ith(writer, pv->get_port(k), lspf);
               }
            }
         }
         if(mod->get_out_port_size())
         {
            for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
            {
               if(mod->get_out_port(i)->get_kind() == port_o_K)
                  io_signal_fix_ith(writer, mod->get_out_port(i), lspf);
               else
               {
                  io_signal_fix_ith_vector(writer, mod->get_out_port(i), lspf);
                  auto* pv = GetPointer<port_o>(mod->get_out_port(i));
                  for(unsigned int k = 0; k < pv->get_ports_size(); k++)
                     io_signal_fix_ith(writer, pv->get_port(k), lspf);
               }
            }
         }
         if(mod->get_in_out_port_size())
         {
            for(unsigned int i = 0; i < mod->get_in_out_port_size(); i++)
            {
               if(mod->get_in_out_port(i)->get_kind() == port_o_K)
                  io_signal_fix_ith(writer, mod->get_in_out_port(i), lspf);
               else
               {
                  io_signal_fix_ith_vector(writer, mod->get_in_out_port(i), lspf);
                  auto* pv = GetPointer<port_o>(mod->get_in_out_port(i));
                  for(unsigned int k = 0; k < pv->get_ports_size(); k++)
                     io_signal_fix_ith(writer, pv->get_port(k), lspf);
               }
            }
         }
         /// for generic ports the post fix is not required. A generic port is never attached to a signal.
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written module instantiation of " + cir->get_id());
      }
      /// check if there is some behavior attached to the module
      else if(is_fsm(cir))
      {
         THROW_ASSERT(np, "Behavior not expected: " + HDL_manager::convert_to_identifier(writer.get(), GET_TYPE_NAME(cir)));
         THROW_ASSERT(!(np->exist_NP_functionality(NP_functionality::FSM) and np->exist_NP_functionality(NP_functionality::FSM_CS)), "Cannot exist both fsm and fsm_cs for the same function");
         std::string fsm_desc;
         if(np->exist_NP_functionality(NP_functionality::FSM_CS))
            fsm_desc = np->get_NP_functionality(NP_functionality::FSM_CS);
         else
            fsm_desc = np->get_NP_functionality(NP_functionality::FSM);
         THROW_ASSERT(fsm_desc != "", "Behavior not expected: " + HDL_manager::convert_to_identifier(writer.get(), GET_TYPE_NAME(cir)));
         write_fsm(writer, cir, fsm_desc);
      }
      else if(np)
      {
         if(np->exist_NP_functionality(NP_functionality::IP_COMPONENT))
         {
            std::string ip_cores = np->get_NP_functionality(NP_functionality::IP_COMPONENT);
            std::vector<std::string> ip_cores_list = convert_string_to_vector<std::string>(ip_cores, ",");
            for(auto ip_core : ip_cores_list)
            {
               std::vector<std::string> ip_core_vec = convert_string_to_vector<std::string>(ip_core, ":");
               if(ip_core_vec.size() < 1 or ip_core_vec.size() > 2)
                  THROW_ERROR("Malformed IP component definition \"" + ip_core + "\"");
               std::string library, component_name;
               if(ip_core_vec.size() == 2)
               {
                  library = ip_core_vec[0];
                  component_name = ip_core_vec[1];
               }
               else
               {
                  component_name = ip_core_vec[0];
                  library = TM->get_library(component_name);
               }
               technology_nodeRef tn = TM->get_fu(component_name, library);
               structural_objectRef core_cir;
               if(tn->get_kind() == functional_unit_K)
                  core_cir = GetPointer<functional_unit>(tn)->CM->get_circ();
               else if(tn->get_kind() == functional_unit_template_K && GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU))
                  core_cir = GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->CM->get_circ();
               else
                  THROW_ERROR("Unexpected pattern");
               writer->write_component_declaration(core_cir);
            }
         }
         writer->write_NP_functionalities(cir);
      }
      else
      {
         THROW_ASSERT(!cir->get_black_box(), "black box component has to be managed in a different way");
      }
   }

   /// write module_instantiation end
   writer->write_module_definition_end(cir);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written module " + GET_TYPE_NAME(cir));
}

void HDL_manager::write_flopoco_module(const structural_objectRef& cir, std::list<std::string>&
#if HAVE_FLOPOCO
                                                                            aux_files
#endif
) const
{
   auto* mod_inst = GetPointer<module>(cir);
   long long int mod_size_in = 0, mod_size_out = 0;
   for(unsigned int i = 0; i < mod_inst->get_in_port_size(); i++)
   {
      // Size of module is size of the largest output
      if(mod_size_in < STD_GET_SIZE(mod_inst->get_in_port(i)->get_typeRef()))
         mod_size_in = STD_GET_SIZE(mod_inst->get_in_port(i)->get_typeRef());
   }
   for(unsigned int i = 0; i < mod_inst->get_out_port_size(); i++)
   {
      // Size of module is size of the largest output
      if(mod_size_out < STD_GET_SIZE(mod_inst->get_out_port(i)->get_typeRef()))
         mod_size_out = STD_GET_SIZE(mod_inst->get_out_port(i)->get_typeRef());
   }
#if HAVE_FLOPOCO
   language_writerRef lan;
   std::string mod_type = mod_inst->get_NP_functionality()->get_NP_functionality(NP_functionality::FLOPOCO_PROVIDED);
   std::string mod_name = convert_to_identifier(lan.get(), GET_TYPE_NAME(cir));
   // Create the module
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, this->debug_level, "Creating " + mod_type + ", named " + mod_name + " whose size is " + STR(mod_size_in) + "|" + STR(mod_size_out));
   std::string pipe_parameter;
   if(mod_inst->ExistsParameter(PIPE_PARAMETER))
      pipe_parameter = mod_inst->GetParameter(PIPE_PARAMETER);
   flopo_wrap->add_FU(mod_type, static_cast<unsigned int>(mod_size_in), static_cast<unsigned int>(mod_size_out), mod_name, pipe_parameter);
   std::string created_file;
   flopo_wrap->writeVHDL(mod_name, static_cast<unsigned int>(mod_size_in), static_cast<unsigned int>(mod_size_out), pipe_parameter, created_file);
   aux_files.push_back(created_file);
#else
   THROW_ERROR("Floating point based HLS requires --enable-flopoco at configuration time");
#endif
}

void HDL_manager::write_behavioral(const language_writerRef writer, const structural_objectRef&, const std::string& behav) const
{
   std::vector<std::string> SplitVec = SplitString(behav, ";");
   THROW_ASSERT(SplitVec.size(), "Expected at least one behavioral description");

   for(auto& i : SplitVec)
   {
      std::vector<std::string> SplitVec2 = SplitString(i, "=");
      THROW_ASSERT(SplitVec2.size() == 2, "Expected two operands");
      writer->write_assign(SplitVec2[0], SplitVec2[1]);
   }
}

void HDL_manager::write_fsm(const language_writerRef writer, const structural_objectRef& cir, const std::string& fsm_desc_i) const
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Start writing the FSM...");

   std::string fsm_desc = fsm_desc_i;
   boost::algorithm::erase_all(fsm_desc, "\n");

   std::vector<std::string> SplitVec = SplitString(fsm_desc, ";");
   THROW_ASSERT(SplitVec.size() > 1, "Expected more than one ';' in the fsm specification (the first is the reset)");

   typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
   boost::char_separator<char> sep(" ", nullptr);
   // compute the list of states
   std::list<std::string> list_of_states;
   std::vector<std::string>::const_iterator it_end = SplitVec.end();
   std::vector<std::string>::const_iterator it = SplitVec.begin();
   tokenizer first_line_tokens(*it, sep);
   tokenizer::iterator tok_iter = first_line_tokens.begin();
   std::string reset_state = convert_to_identifier(writer.get(), *tok_iter);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Reset state: '" << reset_state << "'");
   ++tok_iter;
   THROW_ASSERT(tok_iter != first_line_tokens.end(), "Wrong fsm description: expected the reset port name");
   std::string reset_port = convert_to_identifier(writer.get(), *tok_iter);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Reset port: '" << reset_port << "'");
   ++tok_iter;
   THROW_ASSERT(tok_iter != first_line_tokens.end(), "Wrong fsm description: expected the start port name");
   std::string start_port = convert_to_identifier(writer.get(), *tok_iter);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Start port: '" << start_port << "'");
   ++tok_iter;
   THROW_ASSERT(tok_iter != first_line_tokens.end(), "Wrong fsm description: expected the clock port name");
   std::string clock_port = convert_to_identifier(writer.get(), *tok_iter);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Clock port: '" << clock_port << "'");
   ++tok_iter;
   THROW_ASSERT(tok_iter == first_line_tokens.end(), "Wrong fsm description: unexpetcted tokens" + *tok_iter);

   ++it;
   auto first = it;
   for(; it + 1 != it_end; ++it)
   {
      tokenizer tokens(*it, sep);
      list_of_states.push_back(convert_to_identifier(writer.get(), *tokens.begin()));
   }
   auto end = it;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Number of states: " << list_of_states.size());
   // std::cout << list_of_states.size() << " " << bitnumber(list_of_states.size()-1) << std::endl;
   THROW_ASSERT(reset_state == *(list_of_states.begin()), "reset state and first state has to be the same " + reset_state + " : " + fsm_desc);

   /// write state declaration.
   std::string vendor;
   if(device->has_parameter("vendor"))
   {
      vendor = device->get_parameter<std::string>("vendor");
      boost::algorithm::to_lower(vendor);
   }
   bool one_hot_encoding = false;
   if(parameters->getOption<std::string>(OPT_fsm_encoding) == "one-hot")
      one_hot_encoding = true;
   else if(parameters->getOption<std::string>(OPT_fsm_encoding) == "auto" && vendor == "xilinx" && list_of_states.size() < 256)
      one_hot_encoding = true;
   std::string family;
   if(device->has_parameter("vendor"))
   {
      family = device->get_parameter<std::string>("family");
      boost::algorithm::to_lower(family);
   }

   bool is_yosys = family.find("yosys") != std::string::npos;

   writer->write_state_declaration(cir, list_of_states, reset_port, reset_state, one_hot_encoding);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "write module_instantiation begin");
   // write module_instantiation begin
   writer->write_module_definition_begin(cir);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "write the present_state update");
   /// write the present_state update
   writer->write_present_state_update(cir, reset_state, reset_port, clock_port, parameters->getOption<std::string>(OPT_sync_reset), cir->find_member(PRESENT_STATE_PORT_NAME, port_o_K, cir).get() != nullptr);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "write transition and output functions");
   /// write transition and output functions
   if(parameters->IsParameter("multi-proc-fsm") and parameters->GetParameter<int>("multi-proc-fsm") == 1)
   {
      auto* mod = GetPointer<module>(cir);
      const auto n_outs = mod->get_out_port_size();
      for(unsigned int output_index = 0; output_index <= n_outs; output_index++)
      {
         if(output_index != n_outs && mod->get_out_port(output_index)->get_id() == PRESENT_STATE_PORT_NAME)
            continue;
         if(output_index != n_outs && mod->get_out_port(output_index)->get_id() == NEXT_STATE_PORT_NAME)
            continue;
         writer->write_transition_output_functions(false, output_index, cir, reset_state, reset_port, start_port, clock_port, first, end, is_yosys);
      }
   }
   else
      writer->write_transition_output_functions(true, 0, cir, reset_state, reset_port, start_port, clock_port, first, end, is_yosys);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "FSM writing completed!");
}

std::string HDL_manager::convert_to_identifier(const language_writer* writer, const std::string& id)
{
   auto ret = id;
   if(dynamic_cast<const VHDL_writer*>(writer))
   {
      if(ret.find("__") != std::string::npos or ret.front() == '_' or ret.back() == '_' or writer->check_keyword(ret))
      {
         return "\\" + ret + "\\";
      }
      else
      {
         return ret;
      }
   }
   else if(dynamic_cast<const verilog_writer*>(writer))
   {
      if((ret[0] >= '0' && ret[0] <= '9') || (ret.find(".") != std::string::npos) || (ret.find("[") != std::string::npos) || (ret.find("]") != std::string::npos))
         return "\\" + ret + " ";
      else if(writer and writer->check_keyword(ret))
         return "\\" + ret + " ";
      else if(ret == "array")
         return ret + "_S";
      else
         return ret;
   }
   else
   {
      return ret;
   }
   THROW_UNREACHABLE("");
   return ret;
}

std::string HDL_manager::convert_to_identifier(const std::string& id)
{
   const language_writer* lan = nullptr;
   return convert_to_identifier(lan, id);
}

std::string HDL_manager::get_mod_typename(const language_writer* lan, const structural_objectRef& cir)
{
   std::string res = GET_TYPE_NAME(cir);
   auto* mod = GetPointer<module>(cir);
   const NP_functionalityRef& np = mod->get_NP_functionality();
   if(np && np->get_NP_functionality(NP_functionality::FLOPOCO_PROVIDED) != "")
   {
      long long int mod_size_in = 0;
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         // Size of module is size of the largest output
         if(mod_size_in < STD_GET_SIZE(mod->get_in_port(i)->get_typeRef()))
            mod_size_in = STD_GET_SIZE(mod->get_in_port(i)->get_typeRef());
      }
      res = res + "_" + STR(mod_size_in);
      long long int mod_size_out = 0;
      for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
      {
         // Size of module is size of the largest output
         if(mod_size_out < STD_GET_SIZE(mod->get_out_port(i)->get_typeRef()))
            mod_size_out = STD_GET_SIZE(mod->get_out_port(i)->get_typeRef());
      }
      res = res + "_" + STR(mod_size_out);
      if(mod->ExistsParameter(PIPE_PARAMETER) and mod->GetParameter(PIPE_PARAMETER) != "")
         res = res + "_" + mod->GetParameter(PIPE_PARAMETER);
   }
   return convert_to_identifier(lan, res);
}
