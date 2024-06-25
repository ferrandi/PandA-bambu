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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
#include "technology_manager.hpp"

#include "config_HAVE_CIRCUIT_BUILT.hpp"

#include "Parameter.hpp"
#include "area_info.hpp"
#include "constant_strings.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "graph.hpp"
#include "library_manager.hpp"
#include "polixml.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "utility.hpp"
#include "xml_helper.hpp"

#include <filesystem>

const unsigned int technology_manager::XML = 1 << 0;

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

bool technology_manager::can_implement(const std::string& fu_name, const std::string& op_name,
                                       const std::string& Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   if(!node)
   {
      return false;
   }
   return GetPointer<functional_unit>(node)->get_operation(op_name) ? true : false;
}

technology_nodeRef technology_manager::get_fu(const std::string& fu_name, const std::string& Library) const
{
   THROW_ASSERT(Library.size(), "Library not specified for component " + fu_name);
   if(library_map.count(Library) && library_map.at(Library)->is_fu(fu_name))
   {
      return library_map.at(Library)->get_fu(fu_name);
   }
   return nullptr;
}

technology_nodeRef technology_manager::get_fu(const std::string& fu_name, std::string* Library) const
{
   for(const auto& lib : libraries)
   {
      THROW_ASSERT(library_map.count(lib), "Library " + lib + " not found");
      if(library_map.at(lib)->is_fu(fu_name))
      {
         if(Library)
         {
            *Library = lib;
         }
         return library_map.at(lib)->get_fu(fu_name);
      }
   }
   return nullptr;
}

ControlStep technology_manager::get_initiation_time(const std::string& fu_name, const std::string& op_name,
                                                    const std::string& Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   THROW_ASSERT(GetPointer<functional_unit>(node), "Unit " + fu_name + " not stored into library (" + Library + ")");
   technology_nodeRef node_op = GetPointer<functional_unit>(node)->get_operation(op_name);
   THROW_ASSERT(GetPointer<operation>(node_op),
                "Operation " + op_name + " not stored into " + fu_name + " library (" + Library + ")");
   THROW_ASSERT(GetPointer<operation>(node_op)->time_m, "Missing timing information");
   return GetPointer<operation>(node_op)->time_m->get_initiation_time();
}

double technology_manager::get_execution_time(const std::string& fu_name, const std::string& op_name,
                                              const std::string& Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   THROW_ASSERT(GetPointer<functional_unit>(node), "Unit " + fu_name + " not stored into library (" + Library + ")");
   technology_nodeRef node_op = GetPointer<functional_unit>(node)->get_operation(op_name);
   THROW_ASSERT(GetPointer<operation>(node_op),
                "Operation " + op_name + " not stored into " + fu_name + " library (" + Library + ")");
   THROW_ASSERT(GetPointer<operation>(node_op)->time_m, "Missing timing information");
   return GetPointer<operation>(node_op)->time_m->get_execution_time();
}

double technology_manager::get_area(const std::string& fu_name, const std::string& Library) const
{
   technology_nodeRef node = get_fu(fu_name, Library);
   THROW_ASSERT(GetPointer<functional_unit>(node), "Unit " + fu_name + " not stored into library (" + Library + ")");
   THROW_ASSERT(GetPointer<functional_unit>(node)->area_m,
                "Unit " + fu_name + "(" + Library + ") does not store area model");
   return GetPointer<functional_unit>(node)->area_m->get_area_value();
}

#if HAVE_CIRCUIT_BUILT
void technology_manager::add_resource(const std::string& Library, const std::string& fu_name,
                                      const structural_managerRef CM, const bool is_builtin)
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

technology_nodeRef technology_manager::add_operation(const std::string& Library, const std::string& fu_name,
                                                     const std::string& operation_name)
{
   THROW_ASSERT(library_map.count(Library), "Library \"" + Library + "\" not found");
   THROW_ASSERT(library_map.at(Library)->is_fu(fu_name), "Unit \"" + fu_name + "\" not found in library \"" + Library);
   const auto curr = get_fu(fu_name, Library);
   const technology_nodeRef curr_op(new operation);
   GetPointerS<operation>(curr_op)->operation_name = operation_name;
   auto fu = GetPointer<functional_unit>(curr);
   THROW_ASSERT(fu, "");
   fu->add(curr_op);
   function_fu[operation_name] = curr;
   return curr_op;
}

void technology_manager::add(const technology_nodeRef curr, const std::string& Library)
{
   auto it = std::find(libraries.begin(), libraries.end(), Library);
   if(it == libraries.end())
   {
      library_managerRef lib(new library_manager(Library, Param, true));
      library_map[Library] = lib;
      libraries.push_back(Library);
   }
   library_map[Library]->add(curr);
}

void technology_manager::xload(const xml_element* node)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Loading xml technology");
   std::map<unsigned int, std::string> info;
   CustomOrderedSet<library_managerRef> temp_libraries;

   const xml_node::node_list list = node->get_children();
   for(const auto& iter : list)
   {
      const auto* Enode = GetPointer<const xml_element>(iter);
      if(!Enode)
      {
         continue;
      }
      if(Enode->get_name() == "information")
      {
      }
      if(Enode->get_name() == "library")
      {
         library_managerRef LM(new library_manager(Param));
         library_manager::xload(Enode, LM, Param);

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
            for(const auto& fu : fus)
            {
               if(library_map[library_name]->is_fu(fu.first))
               {
                  /// First part of the condition is for skip template
                  if(not GetPointer<const functional_unit>(library_map[library_name]->get_fu(fu.first)) or
                     GetPointer<const functional_unit>(library_map[library_name]->get_fu(fu.first))
                             ->characterization_timestamp <=
                         GetPointer<const functional_unit>(fu.second)->characterization_timestamp)
                  {
                     INDENT_DBG_MEX(
                         DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                         "-->Updating " + fu.first +
                             (GetPointer<const functional_unit>(library_map[library_name]->get_fu(fu.first)) ?
                                  " characterized at " +
                                      STR(GetPointer<const functional_unit>(fu.second)->characterization_timestamp) +
                                      " - Previous characterization is at " +
                                      STR(GetPointer<const functional_unit>(library_map[library_name]->get_fu(fu.first))
                                              ->characterization_timestamp) :
                                  ""));
                     library_map[library_name]->update(fu.second);
                  }
                  else
                  {
                     INDENT_DBG_MEX(
                         DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                         "-->Not updating " + fu.first +
                             (GetPointer<const functional_unit>(library_map[library_name]->get_fu(fu.first)) ?
                                  " characterized at " +
                                      STR(GetPointer<const functional_unit>(fu.second)->characterization_timestamp) +
                                      " - Previous characterization is at " +
                                      STR(GetPointer<const functional_unit>(library_map[library_name]->get_fu(fu.first))
                                              ->characterization_timestamp) :
                                  ""));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               }
               else
               {
                  INDENT_DBG_MEX(
                      DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                      "-->Loading " + fu.first +
                          (GetPointer<const functional_unit>(fu.second) ?
                               " characterized at " +
                                   STR(GetPointer<const functional_unit>(fu.second)->characterization_timestamp) :
                               ""));
                  library_map[library_name]->add(fu.second);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated library " + library_name);
         }
      }
   }
   for(const auto& temp_library : temp_libraries)
   {
      for(const auto& temp_info : info)
      {
         temp_library->set_info(temp_info.first, temp_info.second);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Loaded xml technology");
}

void technology_manager::xwrite(xml_element* rootnode, const CustomOrderedSet<std::string>& _libraries)
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
      {
         continue;
      }
      if(_libraries.empty() || _libraries.count(library))
      {
         if(get_library_manager(library)->get_library_fu().size())
         {
            get_library_manager(library)->xwrite(rootnode);
         }
      }
   }
}

std::string technology_manager::get_library(const std::string& Name) const
{
   std::string Library;
   for(const auto& librarie : libraries)
   {
      Library = librarie;
      THROW_ASSERT(library_map.find(Library) != library_map.end(), "Library " + Library + " not found");
      if(library_map.find(Library)->second->is_fu(Name))
      {
         return Library;
      }
   }
   /// empty string. it means that the cell is not contained into any library
   return "";
}

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
   {
      libraries.erase(std::find(libraries.begin(), libraries.end(), Name));
   }
}

bool technology_manager::IsBuiltin(const std::string& component_name) const
{
   return builtins.find(component_name) != builtins.end();
}

const functional_unit* technology_manager::CGetSetupHoldFU() const
{
   const technology_nodeConstRef f_unit_as = get_fu("ASSIGN_SINGLE_UNSIGNED_FU", LIBRARY_STD_FU);
   if(f_unit_as)
   {
      return GetPointer<const functional_unit>(f_unit_as);
   }
   else
   {
      return nullptr;
   }
}

double technology_manager::CGetSetupHoldTime() const
{
   const auto fu_as = CGetSetupHoldFU();
   if(!fu_as)
   {
      return 0.1;
   }
   const technology_nodeConstRef op_as_node = fu_as->get_operation("ASSIGN_SINGLE");
   const auto op_ASSIGN = GetPointer<const operation>(op_as_node);
   THROW_ASSERT(op_ASSIGN->time_m->get_execution_time() > 0.0, "expected a setup time greater than zero");
   return op_ASSIGN->time_m->get_execution_time();
}

TimeStamp technology_manager::CGetSetupHoldTimeStamp() const
{
   auto fu = CGetSetupHoldFU();
   if(fu)
   {
      return CGetSetupHoldFU()->characterization_timestamp;
   }
   else
   {
      return TimeStamp();
   }
}

technology_nodeRef technology_manager::GetFunctionFU(const std::string& fname) const
{
   const auto fu_it = function_fu.find(fname);
   return fu_it != function_fu.end() ? fu_it->second : nullptr;
}
