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
 * @file technology_manager.cpp
 * @brief Class implementation of the manager of the technology library structures.
 *
 * This file implements some of the technology_manager member functions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_CIRCUIT_BUILT.hpp"
#include "config_HAVE_FROM_LIBERTY.hpp"

#include "library_manager.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include "area_model.hpp"
#include "time_model.hpp"
#if HAVE_EXPERIMENTAL
#include "lef2xml.hpp"
#endif
#include "graph.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#if HAVE_FROM_LIBERTY
#include "lib2xml.hpp"
#endif

#include "exceptions.hpp"
#include "fileIO.hpp"
#include "polixml.hpp"
#include "utility.hpp"
#include "xml_helper.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"

#include "simple_indent.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

/// STL include
#include "custom_map.hpp"

const unsigned int technology_manager::XML = 1 << 0;
#if HAVE_FROM_LIBERTY
#define LIBERTY_VERSION "0.1"
const unsigned int technology_manager::LIB = 1 << 1;
#endif
const unsigned int technology_manager::LEF = 1 << 2;

technology_manager::technology_manager(const ParameterConstRef _Param) : Param(_Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this));
}

technology_manager::~technology_manager() = default;

void technology_manager::print(std::ostream& os) const
{
   for(const auto& librarie : libraries)
   {
      const library_managerRef library = library_map.find(librarie)->second;
      const library_manager::fu_map_type& cells = library->get_library_fu();
      for(const auto& cell : cells)
      {
         cell.second->print(os);
      }
   }
}

bool technology_manager::can_implement(const std::string& fu_name, const std::string& op_name, const std::string& Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   if(!node)
      return false;
   return GetPointer<functional_unit>(node)->get_operation(op_name) ? true : false;
}

technology_nodeRef technology_manager::get_fu(const std::string& fu_name, const std::string& Library) const
{
   THROW_ASSERT(Library.size(), "Library not specified for component " + fu_name);
   if(library_map.find(Library) != library_map.end() and library_map.find(Library)->second->is_fu(fu_name))
      return library_map.find(Library)->second->get_fu(fu_name);
   return technology_nodeRef();
}

ControlStep technology_manager::get_initiation_time(const std::string& fu_name, const std::string& op_name, const std::string& Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   THROW_ASSERT(GetPointer<functional_unit>(node), "Unit " + fu_name + " not stored into library (" + Library + ")");
   technology_nodeRef node_op = GetPointer<functional_unit>(node)->get_operation(op_name);
   THROW_ASSERT(GetPointer<operation>(node_op), "Operation " + op_name + " not stored into " + fu_name + " library (" + Library + ")");
   THROW_ASSERT(GetPointer<operation>(node_op)->time_m, "Missing timing information");
   return GetPointer<operation>(node_op)->time_m->get_initiation_time();
}

double technology_manager::get_execution_time(const std::string& fu_name, const std::string& op_name, const std::string& Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   THROW_ASSERT(GetPointer<functional_unit>(node), "Unit " + fu_name + " not stored into library (" + Library + ")");
   technology_nodeRef node_op = GetPointer<functional_unit>(node)->get_operation(op_name);
   THROW_ASSERT(GetPointer<operation>(node_op), "Operation " + op_name + " not stored into " + fu_name + " library (" + Library + ")");
   THROW_ASSERT(GetPointer<operation>(node_op)->time_m, "Missing timing information");
   return GetPointer<operation>(node_op)->time_m->get_execution_time();
}

double technology_manager::get_area(const std::string& fu_name, const std::string& Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   THROW_ASSERT(GetPointer<functional_unit>(node), "Unit " + fu_name + " not stored into library (" + Library + ")");
   THROW_ASSERT(GetPointer<functional_unit>(node)->area_m, "Unit " + fu_name + "(" + Library + ") does not store area model");
   return GetPointer<functional_unit>(node)->area_m->get_area_value();
}

#if 0
double technology_manager::get_height(const std::string&fu_name, const std::string&Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   THROW_ASSERT(GetPointer<functional_unit>(node), "Unit " + fu_name + " not stored into library (" + Library + ")");
   THROW_ASSERT(GetPointer<cell_model>(GetPointer<functional_unit>(node)->area), "malformed library");
   return GetPointer<cell_model>(GetPointer<functional_unit>(node)->area)->get_height_value();
}

double technology_manager::get_width(const std::string&fu_name, const std::string&Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   THROW_ASSERT(GetPointer<functional_unit>(node), "Unit " + fu_name + " not stored into library (" + Library + ")");
   THROW_ASSERT(GetPointer<cell_model>(GetPointer<functional_unit>(node)->area), "malformed library");
   return GetPointer<cell_model>(GetPointer<functional_unit>(node)->area)->get_width_value();
}
#endif

#if HAVE_CIRCUIT_BUILT
void technology_manager::add_resource(const std::string& Library, const std::string& fu_name, const structural_managerRef CM, const bool is_builtin)
{
   technology_nodeRef curr = get_fu(fu_name, Library);
   if(!curr)
   {
      curr = technology_nodeRef(new functional_unit);
      GetPointer<functional_unit>(curr)->functional_unit_name = fu_name;
      GetPointer<functional_unit>(curr)->CM = CM;
      add(curr, Library);
   }
   else
   {
      GetPointer<functional_unit>(curr)->CM = CM;
   }
   if(is_builtin)
   {
      builtins.insert(fu_name);
   }
}
#endif

void technology_manager::add_operation(const std::string& Library, const std::string& fu_name, const std::string& operation_name)
{
   THROW_ASSERT(library_map.find(Library) != library_map.end(), "Library \"" + Library + "\" not found");
   THROW_ASSERT(library_map[Library]->is_fu(fu_name), "Unit \"" + fu_name + "\" not found in library \"" + Library);
   THROW_ASSERT(library_map.find(Library) != library_map.end(), "Library \"" + Library + "\" not found");
   technology_nodeRef curr = get_fu(fu_name, Library);
   technology_nodeRef curr_op = technology_nodeRef(new operation);
   GetPointer<operation>(curr_op)->operation_name = operation_name;
   GetPointer<functional_unit>(curr)->add(curr_op);
}

void technology_manager::add(const technology_nodeRef curr, const std::string& Library)
{
   auto it = std::find(libraries.begin(), libraries.end(), Library);
   if(it == libraries.end())
   {
      bool std = true;
      if(Library == CG_LIBRARY || Library == DESIGN)
         std = false;
      library_managerRef lib(new library_manager(Library, Param, std));
      library_map[Library] = lib;
      libraries.push_back(Library);
   }
   library_map[Library]->add(curr);
}

void technology_manager::xload(const xml_element* node, const target_deviceRef device)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Loading xml technology");
   std::map<unsigned int, std::string> info;
   CustomOrderedSet<library_managerRef> temp_libraries;

   const xml_node::node_list list = node->get_children();
   for(const auto& iter : list)
   {
      const auto* Enode = GetPointer<const xml_element>(iter);
      if(!Enode)
         continue;
      if(Enode->get_name() == "information")
      {
         const attribute_sequence::attribute_list& attr_list = Enode->get_attributes();
         for(auto a = attr_list.begin(); a != attr_list.end(); ++a)
         {
#if HAVE_FROM_LIBERTY
            std::string key = (*a)->get_name();
            std::string value = (*a)->get_value();
            if(key == "liberty_file")
               info[library_manager::LIBERTY] = value;
#endif
         }
      }
      if(Enode->get_name() == "library")
      {
         library_managerRef LM(new library_manager(Param));
         library_manager::xload(Enode, LM, Param, device);

         std::string library_name = LM->get_library_name();
         if(library_map.find(library_name) == library_map.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Loading library " + library_name);
            library_map[library_name] = LM;
            libraries.push_back(library_name);
            temp_libraries.insert(LM);
            LM->set_info(library_manager::XML, "");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Loaded library " + library_name);
         }
         else
         {
            const library_manager::fu_map_type& fus = LM->get_library_fu();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updating library " + library_name);
            for(auto f = fus.begin(); f != fus.end(); ++f)
            {
               if(library_map[library_name]->is_fu(f->first))
               {
                  /// First part of the condition is for skip template
                  if(not GetPointer<const functional_unit>(library_map[library_name]->get_fu(f->first)) or
                     GetPointer<const functional_unit>(library_map[library_name]->get_fu(f->first))->characterization_timestamp <= GetPointer<const functional_unit>(f->second)->characterization_timestamp)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "-->Updating " + f->first +
                                        (GetPointer<const functional_unit>(library_map[library_name]->get_fu(f->first)) ?
                                             " characterized at " + STR(GetPointer<const functional_unit>(f->second)->characterization_timestamp) + " - Previous characterization is at " +
                                                 STR(GetPointer<const functional_unit>(library_map[library_name]->get_fu(f->first))->characterization_timestamp) :
                                             ""));
                     library_map[library_name]->update(f->second);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "-->Not updating " + f->first +
                                        (GetPointer<const functional_unit>(library_map[library_name]->get_fu(f->first)) ?
                                             " characterized at " + STR(GetPointer<const functional_unit>(f->second)->characterization_timestamp) + " - Previous characterization is at " +
                                                 STR(GetPointer<const functional_unit>(library_map[library_name]->get_fu(f->first))->characterization_timestamp) :
                                             ""));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Loading " + f->first + (GetPointer<const functional_unit>(f->second) ? " characterized at " + STR(GetPointer<const functional_unit>(f->second)->characterization_timestamp) : ""));
                  library_map[library_name]->add(f->second);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated library " + library_name);
         }
      }
   }
   for(auto temp_library : temp_libraries)
   {
      for(const auto& temp_info : info)
      {
         temp_library->set_info(temp_info.first, temp_info.second);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Loaded xml technology");
}

#if HAVE_BOOLEAN_PARSER_BUILT
void technology_manager::gload(const std::string& file_name, const fileIO_istreamRef file, const technology_managerRef TM, const ParameterConstRef Param)
{
   std::string library_name = file_name.substr(0, file_name.find_last_of("."));

   unsigned int gate = 0;

   while(*file)
   {
      char tmp[255];
      file->getline(tmp, 255); // delim defaults to '\n'
      std::string line = tmp;
      if(!file or line.size() == 0 or boost::algorithm::starts_with(line, "#"))
         continue;

      std::vector<std::string> splitted = SplitString(line, " ; \t");
      if(splitted[0] == "PIN")
         continue;

      std::string fu_name = splitted[1];
      if(fu_name.find("\"") != std::string::npos)
         fu_name = "gate_" + boost::lexical_cast<std::string>(gate++);

      technology_nodeRef fu_curr = TM->get_fu(fu_name, library_name);
      if(fu_curr)
      {
         GetPointer<functional_unit>(fu_curr)->gload(line, fu_name, fu_curr, Param);
      }
      else
      {
         fu_curr = technology_nodeRef(new functional_unit());
         GetPointer<functional_unit>(fu_curr)->gload(line, fu_name, fu_curr, Param);
         TM->add(fu_curr, library_name);
      }
   }
}
#endif

void technology_manager::xwrite(xml_element* rootnode, TargetDevice_Type dv_type, const CustomOrderedSet<std::string>& _libraries)
{
   /// Set of libraries sorted by name
   CustomOrderedSet<std::string> sorted_libraries;
   for(const auto& library : library_map)
   {
      sorted_libraries.insert(library.first);
   }
   for(const auto& library : sorted_libraries)
   {
      if(library == "design")
         continue;
      if(_libraries.empty() || _libraries.count(library))
      {
         if(get_library_manager(library)->get_library_fu().size())
         {
            get_library_manager(library)->xwrite(rootnode, dv_type);
         }
      }
   }
}

#if HAVE_FROM_LIBERTY
void technology_manager::lib_write(const std::string& filename, TargetDevice_Type dv_type, const CustomOrderedSet<std::string>& local_libraries)
{
   unsigned int output_level = Param->getOption<unsigned int>(OPT_output_level);
   try
   {
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("technology");
      xwrite(nodeRoot, dv_type, local_libraries);
      document.write_to_file_formatted("__library__.xml");

      xml2lib("__library__.xml", filename, output_level, debug_level);
      if(debug_level < DEBUG_LEVEL_PEDANTIC)
         boost::filesystem::remove("__library__.xml");
      for(CustomOrderedSet<std::string>::const_iterator l = local_libraries.begin(); l != local_libraries.end(); ++l)
      {
         if(!is_library_manager(*l))
            continue;
         const library_managerRef LM = get_library_manager(*l);
         LM->set_info(library_manager::LIBERTY, filename);
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
}
#endif

#if HAVE_EXPERIMENTAL
void technology_manager::lef_write(const std::string& filename, TargetDevice_Type dv_type, const CustomOrderedSet<std::string>& _libraries)
{
   unsigned int output_level = Param->getOption<unsigned int>(OPT_output_level);
   try
   {
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("technology");
      xwrite(nodeRoot, dv_type, _libraries);
      document.write_to_file_formatted("__library__.xml");

      xml2lef("__library__.xml", filename, output_level, debug_level);
      boost::filesystem::remove("__library__.xml");
      for(CustomOrderedSet<std::string>::const_iterator l = _libraries.begin(); l != _libraries.end(); ++l)
      {
         if(!is_library_manager(*l))
            continue;
         const library_managerRef LM = get_library_manager(*l);
         LM->set_info(library_manager::LEF, filename);
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
}
#endif

std::string technology_manager::get_library(const std::string& Name) const
{
   std::string Library;
   for(const auto& librarie : libraries)
   {
      Library = librarie;
      THROW_ASSERT(library_map.find(Library) != library_map.end(), "Library " + Library + " not found");
      if(library_map.find(Library)->second->is_fu(Name))
         return Library;
   }
   /// empty string. it means that the cell is not contained into any library
   return "";
}

#if HAVE_PHYSICAL_LIBRARY_MODELS_BUILT
size_t technology_manager::get_library_count(const std::string& Name) const
{
   if(std::find(libraries.begin(), libraries.end(), Name) != libraries.end() && library_map.find(Name) != library_map.end())
      return library_map.find(Name)->second->get_gate_count();

   return 0;
}
#endif

library_managerRef technology_manager::get_library_manager(const std::string& Name) const
{
   THROW_ASSERT(library_map.find(Name) != library_map.end(), "library_manager not stored for library: " + Name);
   return library_map.find(Name)->second;
}

bool technology_manager::is_library_manager(const std::string& Name) const
{
   return library_map.find(Name) != library_map.end();
}

void technology_manager::erase_library(const std::string& Name)
{
   library_map.erase(Name);
   if(std::find(libraries.begin(), libraries.end(), Name) != libraries.end())
      libraries.erase(std::find(libraries.begin(), libraries.end(), Name));
}

#if HAVE_CIRCUIT_BUILT
void technology_manager::add_storage(const std::string& s_name, const structural_managerRef CM, const std::string& Library, const unsigned int bits, const unsigned int words, const unsigned int readinputs, const unsigned int writeinputs,
                                     const unsigned int readwriteinputs)
{
   technology_nodeRef curr_storage = technology_nodeRef(new storage_unit);
   GetPointer<storage_unit>(curr_storage)->storage_unit_name = s_name;
   GetPointer<storage_unit>(curr_storage)->CM = CM;
   GetPointer<storage_unit>(curr_storage)->bits = bits;
   GetPointer<storage_unit>(curr_storage)->words = words;
   GetPointer<storage_unit>(curr_storage)->read_ports = readinputs;
   GetPointer<storage_unit>(curr_storage)->write_ports = writeinputs;
   GetPointer<storage_unit>(curr_storage)->readwrite_ports = readwriteinputs;
   add(curr_storage, Library);
}
#endif

bool technology_manager::IsBuiltin(const std::string& component_name) const
{
   return builtins.find(component_name) != builtins.end();
}

const functional_unit* technology_manager::CGetSetupHoldFU() const
{
   const technology_nodeConstRef f_unit_as = get_fu("ASSIGN_SINGLE_UNSIGNED_FU", LIBRARY_STD_FU);
   THROW_ASSERT(f_unit_as, "Library miss component: ASSIGN_SINGLE_UNSIGNED_FU");
   return GetPointer<const functional_unit>(f_unit_as);
}

double technology_manager::CGetSetupHoldTime() const
{
   const auto fu_as = CGetSetupHoldFU();
   const technology_nodeConstRef op_as_node = fu_as->get_operation("ASSIGN_SINGLE");
   const auto op_ASSIGN = GetPointer<const operation>(op_as_node);
   THROW_ASSERT(op_ASSIGN->time_m->get_execution_time() > 0.0, "expected a setup time greater than zero");
   return op_ASSIGN->time_m->get_execution_time();
}

TimeStamp technology_manager::CGetSetupHoldTimeStamp() const
{
   return CGetSetupHoldFU()->characterization_timestamp;
}
