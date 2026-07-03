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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file HDL_manager.cpp
 * @brief Implementation of the base methods for writing HDL descriptions
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "HDL_manager.hpp"

#include "NP_functionality.hpp"
#include "Parameter.hpp"
#include "VHDL_writer.hpp"
#include "copyrights_strings.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_manager.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"
#include "hls_manager.hpp"
#include "library_manager.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "verilog_writer.hpp"

#include "config_HAVE_ASSERTS.hpp"
#include "config_PACKAGE_BUGREPORT.hpp"
#include "config_PACKAGE_NAME.hpp"
#include "config_PACKAGE_VERSION.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/tokenizer.hpp>

#include <fstream>
#include <iosfwd>
#include <utility>

HDL_manager::HDL_manager(const HLS_managerRef _HLSMgr, const generic_deviceRef _device, const structural_managerRef _SM,
                         const ParameterConstRef _parameters)
    : HLSMgr(_HLSMgr),
      device(_device),
      TM(_device->get_technology_manager()),
      SM(_SM),
      parameters(_parameters),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

HDL_manager::HDL_manager(const HLS_managerRef _HLSMgr, const generic_deviceRef _device,
                         const ParameterConstRef _parameters)
    : HLSMgr(_HLSMgr),
      device(_device),
      TM(_device->get_technology_manager()),
      parameters(_parameters),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

std::string HDL_manager::write_components(const std::string& filename, HDLWriter_Language language,
                                          const std::list<structural_objectRef>& components,
                                          std::list<std::string>& aux_files, bool is_library) const
{
   language_writerRef writer = language_writer::create_writer(language, TM, parameters);

   writer->write_comment("\n");
   writer->write_comment("Politecnico di Milano\n");
   writer->write_comment("Code created using " PACKAGE_NAME " - " + parameters->PrintVersion() + " - Date " +
                         TimeStamp::GetCurrentTimeStamp() + "\n");
   if(parameters->isOption(OPT_cat_args))
   {
      writer->write_comment("Bambu executed with: " + parameters->getOption<std::string>(OPT_cat_args) + "\n");
   }

   writer->write_comment("\n");
   writer->write_comment("Send any bug to: " PACKAGE_BUGREPORT "\n");
   writer->WriteLicense(is_library);

   /// write the header of the file
   writer->write_header(is_library);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing components");
   /// write all modules
   for(const auto& c : components)
   {
      NP_functionalityRef npf = GetPointer<module_o>(c)->get_NP_functionality();
      if(npf)
      {
         if(npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED) != "" ||
            npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED) != "")
         {
            if(npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED) != "" &&
               language == HDLWriter_Language::VERILOG)
            {
               const auto filename_HDL = npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED);
               if(std::find(aux_files.begin(), aux_files.end(), filename_HDL) == aux_files.end())
               {
                  aux_files.push_back(filename_HDL);
               }
            }
            else if(npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED) != "" &&
                    language == HDLWriter_Language::VHDL)
            {
               const auto filename_HDL = npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED);
               if(std::find(aux_files.begin(), aux_files.end(), filename_HDL) == aux_files.end())
               {
                  aux_files.push_back(filename_HDL);
               }
            }
            else if(npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED) != "")
            {
               const auto filename_HDL = npf->get_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED);
               if(std::find(aux_files.begin(), aux_files.end(), filename_HDL) == aux_files.end())
               {
                  aux_files.push_back(filename_HDL);
               }
            }
            else if(npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED) != "")
            {
               const auto filename_HDL = npf->get_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED);
               if(std::find(aux_files.begin(), aux_files.end(), filename_HDL) == aux_files.end())
               {
                  aux_files.push_back(filename_HDL);
               }
            }
            else
            {
               THROW_UNREACHABLE("unexpected condition");
            }
            continue;
         }
      }
      const auto library = TM->get_library(c->get_typeRef()->id_type);
      auto obj = c;
      /// we write the definition of the object stored in library
      if(!library.empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Component " + c->get_typeRef()->id_type + " is in library " + library);
         const auto tn = TM->get_fu(c->get_typeRef()->id_type, library);
         if(GetPointer<functional_unit>(tn))
         {
            THROW_ASSERT(GetPointer<functional_unit>(tn)->CM, tn->get_name());
            obj = GetPointer<functional_unit>(tn)->CM->get_circ();
         }
         else if(GetPointer<functional_unit_template>(tn))
         {
            const auto FU = GetPointer<functional_unit_template>(tn)->FU;
            obj = GetPointer<functional_unit>(FU)->CM->get_circ();
         }
         else
         {
            THROW_ERROR("unexpected condition");
         }
      }
      THROW_ASSERT(!is_library || GetPointerS<module_o>(obj)->get_license() == PANDA_MIT_tag, "inconsistent licensing");
      write_module(writer, obj, is_library);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written components");
   const auto filename_ext = filename + writer->get_extension();
   writer->WriteFile(filename_ext);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written " + filename_ext);
   return std::filesystem::canonical(filename_ext).string();
}

void HDL_manager::write_components(const std::string& filename, const std::list<structural_objectRef>& components,
                                   std::list<std::string>& hdl_files, std::list<std::string>& aux_files,
                                   HDL_output_mode gen_mode)
{
   /// default language
   auto language = parameters->getOption<HDLWriter_Language>(OPT_writer_language);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Everything seems OK. Let's start with the real job.");

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
   {
      THROW_ERROR("Language not supported");
   }

   /// determine the proper language for each component
   std::map<HDLWriter_Language, std::list<structural_objectRef>> hdl_comp, aux_comp;
   for(const auto& component : components)
   {
      const auto mod = GetPointer<module_o>(component);
      THROW_ASSERT(mod, "Expected a component object");
      const auto n_elements = mod->get_internal_objects_size();
      const auto np = mod->get_NP_functionality();

      HDLWriter_Language comp_language = language;
      if(!n_elements && np)
      {
         const auto has_verilog = np->exist_NP_functionality(NP_functionality::VERILOG_PROVIDED) ||
                                  np->exist_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED);
         const auto has_vhdl = np->exist_NP_functionality(NP_functionality::VHDL_PROVIDED) ||
                               np->exist_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED);
         if(np->exist_NP_functionality(type) || (language == HDLWriter_Language::VERILOG && has_verilog) ||
            (language == HDLWriter_Language::VHDL && has_vhdl) || np->exist_NP_functionality(NP_functionality::FSM) ||
            np->exist_NP_functionality(NP_functionality::FSM_CS))
         {
            comp_language = language;
         }
         else if(has_verilog)
         {
            comp_language = HDLWriter_Language::VERILOG;
         }
         else if(has_vhdl)
         {
            comp_language = HDLWriter_Language::VHDL;
         }
         else if(np->exist_NP_functionality(NP_functionality::SYSTEM_VERILOG_PROVIDED))
         {
            comp_language = HDLWriter_Language::SYSTEM_VERILOG;
         }
         else
         {
            THROW_ERROR("Language not supported! Module " + mod->get_path());
         }
      }
      const auto library = TM->get_library(component->get_typeRef()->id_type);
      if(gen_mode == HDL_OUT_MIX || library.empty() || !starts_with(library, "STD"))
      {
         hdl_comp[comp_language].push_back(component);
      }
      else
      {
         aux_comp[comp_language].push_back(component);
      }
   }

   const auto out_dir = std::filesystem::path(filename).parent_path().string();
   std::filesystem::create_directories(out_dir);

   for(auto& [lang, comps] : aux_comp)
   {
      auto writer = language_writer::create_writer(lang, TM, parameters);
      if(gen_mode == HDL_OUT_WORK_LIBRARY)
      {
         const auto lib_dir = out_dir + "/panda_libtech/" + writer->get_name() + "/";
         std::filesystem::create_directories(lib_dir);
         for(auto& comp : comps)
         {
            const auto mod_name = lib_dir + convert_to_identifier(GET_TYPE_NAME(comp));
            const auto generated_filename = write_components(mod_name, lang, {comp}, aux_files);
            aux_files.push_back(generated_filename);
         }
      }
      else
      {
         const auto lib_name = out_dir + "/panda_libtech";
         const auto generated_filename = write_components(lib_name, lang, comps, aux_files);
         aux_files.push_back(generated_filename);
      }
   }

   for(auto& [lang, comps] : hdl_comp)
   {
      auto writer = language_writer::create_writer(lang, TM, parameters);
      const auto generated_filename = write_components(filename, lang, comps, aux_files, false);
      if(lang == language)
      {
         hdl_files.push_back(generated_filename);
      }
      else
      {
         aux_files.push_back(generated_filename);
      }
   }
}

void HDL_manager::hdl_gen(const std::string& filename, const std::list<structural_objectRef>& cirs,
                          std::list<std::string>& hdl_files, std::list<std::string>& aux_files,
                          HDL_output_mode gen_mode)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "  compute the list of components for which a structural description exists");
   /// compute the list of components for which a structural description exist.
   std::list<structural_objectRef> list_of_com;
   for(const auto& cir : cirs)
   {
      get_post_order_structural_components(cir, list_of_com);
   }
   if(list_of_com.empty())
   {
#if HAVE_ASSERTS
      for(const auto& cir : cirs)
      {
         THROW_ASSERT(GetPointer<module_o>(cir), "Expected a component");
         THROW_ASSERT(GetPointer<module_o>(cir)->get_NP_functionality(), "Structural empty description received");
      }
#endif
      return;
   }

   /// generate the HDL descriptions for all the components
   write_components(filename, list_of_com, hdl_files, aux_files, gen_mode);
}

/**
 * Used to test if a component has been already inserted into the component list
 */
struct find_eq_module
{
   // predicate function
   bool operator()(const structural_objectRef& el)
   {
      return el == target || HDL_manager::get_mod_typename(el) == HDL_manager::get_mod_typename(target);
   }
   find_eq_module(const language_writer* _lan, const structural_objectRef& _target) : lan(_lan), target(_target)
   {
      THROW_ASSERT(_target, "structural_objectRef must exist");
   }

 private:
   const language_writer* lan;
   const structural_objectRef target;
};

bool HDL_manager::is_fsm(const structural_objectRef& cir) const
{
   /// check for a FSM description
   auto* mod_inst = GetPointer<module_o>(cir);
   THROW_ASSERT(mod_inst, "Expected a component");
   const NP_functionalityRef& np = mod_inst->get_NP_functionality();
   if(np)
   {
      return (np->exist_NP_functionality(NP_functionality::FSM) or
              np->exist_NP_functionality(NP_functionality::FSM_CS));
   }
   return false;
}

void HDL_manager::get_post_order_structural_components(const structural_objectRef cir,
                                                       std::list<structural_objectRef>& list_of_com) const
{
   switch(cir->get_kind())
   {
      case module_o_K:
      {
         auto* mod = GetPointer<module_o>(cir);
         unsigned int n_elements = mod->get_internal_objects_size();
         for(unsigned int i = 0; i < n_elements; i++)
         {
            switch(mod->get_internal_object(i)->get_kind())
            {
               case module_o_K:
               {
                  if(!mod->get_internal_object(i)->get_black_box() &&
                     !TM->IsBuiltin(GET_TYPE_NAME(mod->get_internal_object(i))))
                  {
                     get_post_order_structural_components(mod->get_internal_object(i), list_of_com);
                  }
                  break;
               }
               case constant_o_K:
               case signal_vector_o_K:
               case signal_o_K:
                  break; /// no action for signals and bus
               case port_o_K:
               case port_vector_o_K:
               default:
                  THROW_ERROR("Structural object not foreseen: " +
                              std::string(mod->get_internal_object(i)->get_kind_text()));
            }
         }
         const auto NPF = mod->get_NP_functionality();
         if(NPF && NPF->exist_NP_functionality(NP_functionality::IP_COMPONENT))
         {
            const auto ip_cores = NPF->get_NP_functionality(NP_functionality::IP_COMPONENT);
            const auto ip_cores_list = string_to_container<std::vector<std::string>>(ip_cores, ",");
            for(const auto& ip_core : ip_cores_list)
            {
               const auto ip_core_vec = string_to_container<std::vector<std::string>>(ip_core, ":");
               if(ip_core_vec.size() < 1 || ip_core_vec.size() > 2)
               {
                  THROW_ERROR("Malformed IP component definition \"" + ip_core + "\"");
               }
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
               const auto tn = TM->get_fu(component_name, library);
               structural_objectRef core_cir;
               if(tn->get_kind() == functional_unit_K)
               {
                  core_cir = GetPointer<functional_unit>(tn)->CM->get_circ();
               }
               else if(tn->get_kind() == functional_unit_template_K &&
                       GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU))
               {
                  core_cir = GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->CM->get_circ();
               }
               else
               {
                  THROW_ERROR("Unexpected pattern");
               }
               THROW_ASSERT(core_cir, "unexpected condition");
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
         {
            list_of_com.push_back(cir);
         }
         break;
      }
      case constant_o_K:
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
   {
      po_owner = po_owner->get_owner();
   }
   for(unsigned int j = 0; j < p->get_connections_size(); j++)
   {
      if(p->get_connection(j)->get_kind() == signal_o_K and
         (p->get_connection(j)->get_owner() == po_owner or
          (p->get_connection(j)->get_owner()->get_kind() == signal_vector_o_K and
           p->get_connection(j)->get_owner()->get_owner() == po_owner)))
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

void HDL_manager::io_signal_fix_ith_vector(const language_writerRef writer, const structural_objectRef po,
                                           bool& lspf) const
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

void HDL_manager::write_module(const language_writerRef writer, const structural_objectRef cir, bool is_library) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing module " + GET_TYPE_NAME(cir));
   const module_o* mod = GetPointer<module_o>(cir);
   THROW_ASSERT(mod, "Expected a module got something of different");

   const NP_functionalityRef& np = mod->get_NP_functionality();

   /// write module declaration
   writer->write_comment(mod->get_description() + "\n");
   writer->write_comment(mod->get_copyright() + "\n");
   writer->write_comment("Author(s): " + mod->get_authors() + "\n");
   auto license_text = string_to_container<std::vector<std::string>>(mod->get_license(), std::string("\n"));
   for(const auto& line : license_text)
   {
      writer->write_comment("License: " + line + "\n");
   }

   /// write library declaration component
   writer->write_library_declaration(cir);

   writer->write_module_declaration(cir, is_library);

   writer->write_module_parametrization_decl(cir);

   writer->write_port_decl_header();

   /// write IO port declarations
   if(mod->get_in_port_size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing input port declaration");
      writer->write_comment("IN\n");
      for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Writing port declaration " + mod->get_in_port(i)->get_id() + " of type " +
                            mod->get_in_port(i)->get_kind_text());
         if(i == mod->get_in_port_size() - 1 && !mod->get_out_port_size() && !mod->get_in_out_port_size() &&
            !mod->get_gen_port_size())
         {
            writer->write_port_declaration(mod->get_in_port(i), true);
         }
         else
         {
            writer->write_port_declaration(mod->get_in_port(i), false);
         }
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
         {
            writer->write_port_declaration(mod->get_out_port(i), true);
         }
         else
         {
            writer->write_port_declaration(mod->get_out_port(i), false);
         }
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
         {
            writer->write_port_declaration(mod->get_in_out_port(i), true);
         }
         else
         {
            writer->write_port_declaration(mod->get_in_out_port(i), false);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written inout port declaration");
   }

   if(mod->get_gen_port_size())
   {
      writer->write_comment("Ports\n");
      for(unsigned int i = 0; i < mod->get_gen_port_size(); i++)
      {
         if(i == mod->get_gen_port_size() - 1)
         {
            writer->write_port_declaration(mod->get_gen_port(i), true);
         }
         else
         {
            writer->write_port_declaration(mod->get_gen_port(i), false);
         }
      }
   }

   writer->write_port_decl_tail();

   /// close the interface declaration and start the implementation
   writer->write_module_internal_declaration(cir);

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
            case module_o_K:
            {
               break;
            }
            case signal_vector_o_K:
            case signal_o_K:
            {
               cs.push_back(std::make_pair(mod->get_internal_object(i)->get_id(), mod->get_internal_object(i)));
               continue;
            }
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
            case module_o_K:
            {
               cs.push_back(std::make_pair(mod->get_internal_object(i)->get_id(), mod->get_internal_object(i)));
               break;
            }
            case constant_o_K:
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
            writer->write_module_instance_begin(obj, get_mod_typename(obj), true);
            /// write IO ports binding
            auto* mod_inst = GetPointer<module_o>(obj);
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
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "-->Writing port binding of " + mod_inst->get_out_port(i)->get_id());
                     if(mod_inst->get_out_port(i)->get_kind() == port_o_K)
                     {
                        const structural_objectRef object_bounded =
                            GetPointer<port_o>(mod_inst->get_out_port(i))->find_bounded_object(cir);
                        if(!object_bounded)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "<--Skipped " + mod_inst->get_out_port(i)->get_path());
                           continue;
                        }
                        writer->write_port_binding(mod_inst->get_out_port(i), object_bounded, first_port_analyzed);
                     }
                     else
                     {
                        writer->write_vector_port_binding(mod_inst->get_out_port(i), first_port_analyzed);
                     }
                     first_port_analyzed = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "<--Written port binding of " + mod_inst->get_out_port(i)->get_id());
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
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "-->Writing port binding of " + mod_inst->get_in_port(i)->get_id());
                  if(mod_inst->get_in_port(i)->get_kind() == port_o_K)
                  {
                     const structural_objectRef object_bounded =
                         GetPointer<port_o>(mod_inst->get_in_port(i))->find_bounded_object(cir);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Bounded object is " +
                                        (object_bounded ? object_bounded->get_path() : "nothing"));
                     writer->write_port_binding(mod_inst->get_in_port(i), object_bounded, first_port_analyzed);
                  }
                  else
                  {
                     writer->write_vector_port_binding(mod_inst->get_in_port(i), first_port_analyzed);
                  }
                  first_port_analyzed = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Written port binding of " + mod_inst->get_in_port(i)->get_id());
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
                     const structural_objectRef object_bounded =
                         GetPointer<port_o>(mod_inst->get_in_out_port(i))->find_bounded_object();
                     if(!object_bounded)
                     {
                        continue;
                     }
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
                  {
                     writer->write_port_binding(mod_inst->get_gen_port(i),
                                                GetPointer<port_o>(mod_inst->get_gen_port(i))->find_bounded_object(),
                                                first_port_analyzed);
                  }
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
                        const structural_objectRef object_bounded =
                            GetPointer<port_o>(mod_inst->get_out_port(i))->find_bounded_object();
                        if(!object_bounded)
                        {
                           continue;
                        }
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
            {
               io_signal_fix_ith(writer, mod->get_in_port(i), lspf);
            }
            else
            {
               io_signal_fix_ith_vector(writer, mod->get_in_port(i), lspf);
               auto* pv = GetPointer<port_o>(mod->get_in_port(i));
               for(unsigned int k = 0; k < pv->get_ports_size(); k++)
               {
                  io_signal_fix_ith(writer, pv->get_port(k), lspf);
               }
            }
         }
      }
      if(mod->get_out_port_size())
      {
         for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
         {
            if(mod->get_out_port(i)->get_kind() == port_o_K)
            {
               io_signal_fix_ith(writer, mod->get_out_port(i), lspf);
            }
            else
            {
               io_signal_fix_ith_vector(writer, mod->get_out_port(i), lspf);
               auto* pv = GetPointer<port_o>(mod->get_out_port(i));
               for(unsigned int k = 0; k < pv->get_ports_size(); k++)
               {
                  io_signal_fix_ith(writer, pv->get_port(k), lspf);
               }
            }
         }
      }
      if(mod->get_in_out_port_size())
      {
         for(unsigned int i = 0; i < mod->get_in_out_port_size(); i++)
         {
            if(mod->get_in_out_port(i)->get_kind() == port_o_K)
            {
               io_signal_fix_ith(writer, mod->get_in_out_port(i), lspf);
            }
            else
            {
               io_signal_fix_ith_vector(writer, mod->get_in_out_port(i), lspf);
               auto* pv = GetPointer<port_o>(mod->get_in_out_port(i));
               for(unsigned int k = 0; k < pv->get_ports_size(); k++)
               {
                  io_signal_fix_ith(writer, pv->get_port(k), lspf);
               }
            }
         }
      }
      /// for generic ports the post fix is not required. A generic port is never attached to a signal.
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written module instantiation of " + cir->get_id());
   }
   /// check if there is some behavior attached to the module
   else if(is_fsm(cir))
   {
      THROW_ASSERT(np, "Behavior not expected: " + HDL_manager::convert_to_identifier(GET_TYPE_NAME(cir)));
      THROW_ASSERT(
          !(np->exist_NP_functionality(NP_functionality::FSM) and np->exist_NP_functionality(NP_functionality::FSM_CS)),
          "Cannot exist both FSM and fsm_cs for the same function");
      std::string fsm_desc;
      if(np->exist_NP_functionality(NP_functionality::FSM_CS))
      {
         fsm_desc = np->get_NP_functionality(NP_functionality::FSM_CS);
      }
      else
      {
         fsm_desc = np->get_NP_functionality(NP_functionality::FSM);
      }
      THROW_ASSERT(fsm_desc != "", "Behavior not expected: " + HDL_manager::convert_to_identifier(GET_TYPE_NAME(cir)));
      std::string fsm_stages;
      if(np->exist_NP_functionality(NP_functionality::FSM_STAGES))
      {
         fsm_stages = np->get_NP_functionality(NP_functionality::FSM_STAGES);
      }
      write_fsm(writer, cir, fsm_desc, fsm_stages);
   }
   else if(np)
   {
      if(np->exist_NP_functionality(NP_functionality::IP_COMPONENT))
      {
         std::string ip_cores = np->get_NP_functionality(NP_functionality::IP_COMPONENT);
         std::vector<std::string> ip_cores_list = string_to_container<std::vector<std::string>>(ip_cores, ",");
         for(const auto& ip_core : ip_cores_list)
         {
            std::vector<std::string> ip_core_vec = string_to_container<std::vector<std::string>>(ip_core, ":");
            if(ip_core_vec.size() < 1 or ip_core_vec.size() > 2)
            {
               THROW_ERROR("Malformed IP component definition \"" + ip_core + "\"");
            }
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
            {
               core_cir = GetPointer<functional_unit>(tn)->CM->get_circ();
            }
            else if(tn->get_kind() == functional_unit_template_K &&
                    GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU))
            {
               core_cir = GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->CM->get_circ();
            }
            else
            {
               THROW_ERROR("Unexpected pattern");
            }
            writer->write_component_declaration(core_cir);
         }
      }
      writer->write_NP_functionalities(cir);
   }
   else
   {
      THROW_ASSERT(!cir->get_black_box(), "black box component has to be managed in a different way");
   }

   /// write module_instantiation end
   writer->write_module_definition_end(cir, is_library);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written module " + GET_TYPE_NAME(cir));
}

void HDL_manager::write_behavioral(const language_writerRef writer, const structural_objectRef&,
                                   const std::string& behav) const
{
   const auto SplitVec = string_to_container<std::vector<std::string>>(behav, ";");
   THROW_ASSERT(SplitVec.size(), "Expected at least one behavioral description");

   for(auto& i : SplitVec)
   {
      const auto SplitVec2 = string_to_container<std::vector<std::string>>(i, "=");
      THROW_ASSERT(SplitVec2.size() == 2, "Expected two operands");
      writer->write_assign(SplitVec2[0], SplitVec2[1]);
   }
}

void HDL_manager::write_fsm(const language_writerRef writer, const structural_objectRef& cir,
                            const std::string& fsm_desc_i, const std::string& fsm_stage_i) const
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Start writing the FSM...");

   const auto fsm_desc = boost::algorithm::erase_all_copy(fsm_desc_i, "\n");

   const auto SplitVec = string_to_container<std::vector<std::string>>(fsm_desc, ";", false);
   THROW_ASSERT(SplitVec.size() > 2, "Expected more than one ';' in the FSM specification (the first is the reset)");

   using tokenizer = boost::tokenizer<boost::char_separator<char>>;
   boost::char_separator<char> sep(" ", nullptr);
   // compute the list of states
   std::list<std::string> list_of_states;
   auto it_end = SplitVec.cend();
   auto it = SplitVec.cbegin();
   tokenizer first_line_tokens(*it, sep);
   tokenizer::iterator tok_iter = first_line_tokens.begin();
   std::string reset_state = convert_to_identifier(*tok_iter);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Reset state: '" << reset_state << "'");
   ++tok_iter;
   THROW_ASSERT(tok_iter != first_line_tokens.end(), "Wrong FSM description: expected the reset port name");
   std::string reset_port = convert_to_identifier(*tok_iter);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Reset port: '" << reset_port << "'");
   ++tok_iter;
   THROW_ASSERT(tok_iter != first_line_tokens.end(), "Wrong FSM description: expected the start port name");
   std::string start_port = convert_to_identifier(*tok_iter);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Start port: '" << start_port << "'");
   ++tok_iter;
   THROW_ASSERT(tok_iter != first_line_tokens.end(), "Wrong FSM description: expected the clock port name");
   std::string clock_port = convert_to_identifier(*tok_iter);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Clock port: '" << clock_port << "'");
   ++tok_iter;
   THROW_ASSERT(tok_iter == first_line_tokens.end(), "Wrong FSM description: unexpetcted tokens" + *tok_iter);

   ++it;
   /// extract bypass signals
   std::map<unsigned int, std::map<std::string, std::set<unsigned int>>> bypass_signals;
   if(!it->empty())
   {
      const auto BypassVec = string_to_container<std::vector<std::string>>(*it, ":");
      for(const auto& assign : BypassVec)
      {
         const auto AssignPair = string_to_container<std::vector<std::string>>(assign, "=");
         THROW_ASSERT(AssignPair.size() == 2, "malformed FSM description " + STR(AssignPair.size()));
         auto out = static_cast<unsigned>(std::stoul(AssignPair.at(0)));
         const auto inStateVec = string_to_container<std::vector<std::string>>(AssignPair.at(1), ",");
         for(const auto& inState : inStateVec)
         {
            const auto StateInsPair = string_to_container<std::vector<std::string>>(inState, ">");
            THROW_ASSERT(StateInsPair.size() == 2, "malformed FSM description " + STR(StateInsPair.size()));
            const auto inVec = string_to_container<std::vector<std::string>>(StateInsPair.at(1), "<");
            for(const auto& in : inVec)
            {
               auto in_val = static_cast<unsigned>(std::stoul(in));
               bypass_signals[out][StateInsPair.at(0)].insert(in_val);
            }
         }
      }
   }

   ++it;
   auto first = it;
   for(; it + 1 != it_end; ++it)
   {
      tokenizer tokens(*it, sep);
      list_of_states.push_back(convert_to_identifier(*tokens.begin()));
   }
   auto end = it;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Number of states: " << list_of_states.size());
   // std::cout << list_of_states.size() << " " << bitnumber(list_of_states.size()-1) << std::endl;
   THROW_ASSERT(reset_state == *(list_of_states.begin()),
                "reset state and first state has to be the same " + reset_state + " : " + fsm_desc);

   /// write state declaration.
   std::string vendor;
   if(device->has_parameter("vendor"))
   {
      vendor = device->get_parameter<std::string>("vendor");
      boost::algorithm::to_lower(vendor);
   }
   bool one_hot_encoding = false;
   if(parameters->getOption<std::string>(OPT_fsm_encoding) == "one-hot")
   {
      one_hot_encoding = true;
   }
   else if(parameters->getOption<std::string>(OPT_fsm_encoding) == "auto" && vendor == "xilinx" &&
           list_of_states.size() < 256)
   {
      one_hot_encoding = true;
   }
   std::string family;
   if(device->has_parameter("vendor"))
   {
      family = device->get_parameter<std::string>("family");
      boost::algorithm::to_lower(family);
   }

   const auto is_yosys = family.find("yosys") != std::string::npos;

   writer->write_state_declaration(cir, list_of_states, reset_port, reset_state, one_hot_encoding);

   if(fsm_stage_i.size())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "write stage declaration");
      const auto StageVec = string_to_container<std::vector<std::string>>(fsm_stage_i, "|", false);
      THROW_ASSERT(StageVec.size() == 4, "unexpected stage format");
      writer->write_stage_declaration(cir, std::stoi(StageVec.at(0)));
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "write module_instantiation begin");
   // write module_instantiation begin
   writer->write_module_definition_begin(cir);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "write the present_state update");
   /// write the present_state update
   writer->write_present_state_update(cir, reset_state, reset_port, clock_port,
                                      parameters->getOption<std::string>(OPT_reset_type),
                                      cir->find_member(PRESENT_STATE_PORT_NAME, port_o_K, cir).get() != nullptr);

   if(fsm_stage_i.size())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "write the present_stages update");
      const auto StageVec = string_to_container<std::vector<std::string>>(fsm_stage_i, "|", false);
      THROW_ASSERT(StageVec.size() == 4, "unexpected stage format");
      writer->write_present_stages_update(cir, reset_port, clock_port,
                                          parameters->getOption<std::string>(OPT_reset_type), start_port,
                                          std::stoi(StageVec.at(0)));
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "write transition and output functions");
   /// write transition and output functions
   if(parameters->IsParameter("multi-proc-fsm") and parameters->GetParameter<int>("multi-proc-fsm") == 1)
   {
      const auto mod = GetPointer<module_o>(cir);
      const auto n_outs = mod->get_out_port_size();
      for(unsigned int output_index = 0; output_index <= n_outs; output_index++)
      {
         if(output_index != n_outs && mod->get_out_port(output_index)->get_id() == PRESENT_STATE_PORT_NAME)
         {
            continue;
         }
         if(output_index != n_outs && mod->get_out_port(output_index)->get_id() == NEXT_STATE_PORT_NAME)
         {
            continue;
         }
         writer->write_transition_output_functions(false, output_index, cir, reset_state, reset_port, start_port,
                                                   clock_port, first, end, is_yosys, bypass_signals, fsm_stage_i);
      }
   }
   else
   {
      writer->write_transition_output_functions(true, 0, cir, reset_state, reset_port, start_port, clock_port, first,
                                                end, is_yosys, bypass_signals, fsm_stage_i);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "FSM writing completed!");
}

std::string HDL_manager::convert_to_identifier(const std::string& id)
{
   if(verilog_writer::check_keyword_verilog(id) || VHDL_writer::check_keyword_vhdl(id))
   {
      return id + "_r";
   }
   std::string mangled = id;
   replace_all_with_restart(id, mangled, "__", "_");
   if(mangled.front() == '_')
   {
      mangled = "p" + mangled;
   }
   if(mangled.back() == '_')
   {
      mangled = mangled + "s";
   }
   return mangled;
}

std::string HDL_manager::get_mod_typename(const structural_objectRef& cir)
{
   std::string res = GET_TYPE_NAME(cir);
   if(language_writer::GetHDLReservedNames().count(res))
   {
      res = res + "_r";
   }
   return convert_to_identifier(res);
}
