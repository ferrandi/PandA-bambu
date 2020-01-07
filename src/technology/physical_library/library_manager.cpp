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
 * @file library_manager.cpp
 * @brief Class implementation of the manager of the specific technology library.
 *
 * This file implements some of the library_manager member functions.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_CIRCUIT_BUILT.hpp"
#include "config_HAVE_FROM_LIBERTY.hpp"
#include "config_HAVE_KOALA_BUILT.hpp"
#include "config_HAVE_LIBRARY_COMPILER.hpp"

#include "library_manager.hpp"

#include "area_model.hpp"
#include "technology_node.hpp"
#if HAVE_LIBRARY_COMPILER
#include "LibraryCompilerWrapper.hpp"
#endif
#include "parse_technology.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "exceptions.hpp"
#include "polixml.hpp"

#include "target_device.hpp"

#include "clb_model.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include <iosfwd>
#include <utility>

attribute::attribute(const std::vector<attributeRef>& _content)
{
   content_list = _content;
}

attribute::attribute(const std::string& _value_type, std::string _content) : content(std::move(_content))
{
   xml_node::convert_escaped(content);
   if(_value_type == "float64")
   {
      value_type = FLOAT64;
   }
   else if(_value_type == "boolean")
   {
      value_type = BOOLEAN;
   }
   else if(_value_type == "int32")
   {
      value_type = INT32;
   }
   else if(_value_type == "string")
   {
      value_type = STRING;
   }
   else
   {
      THROW_ERROR("Not supported attribute type: " + _value_type);
   }
}

attribute::attribute(const value_t _value_type, std::string _content) : content(std::move(_content)), value_type(_value_type)
{
   xml_node::convert_escaped(content);
   value_type = static_cast<value_t>(_value_type);
}

std::string attribute::get_value_type_str() const
{
   if(value_type == FLOAT64)
   {
      return "float64";
   }
   if(value_type == BOOLEAN)
   {
      return "boolean";
   }
   if(value_type == INT32)
   {
      return "int32";
   }
   else if(value_type == STRING)
   {
      return "string";
   }
   else
   {
      THROW_ERROR("Not supported attribute type: " + boost::lexical_cast<std::string>(value_type));
   }
   return "<unknown>";
}

std::string attribute::get_content_str() const
{
   /// fix for content_list
   return content;
}

bool attribute::has_list() const
{
   return !content_list.empty();
}

unsigned int attribute::get_value_type() const
{
   return value_type;
}

void attribute::xload(const xml_element* EnodeC, std::vector<std::string>& ordered_attributes, std::map<std::string, attributeRef>& attributes)
{
   const xml_attribute* att_name = EnodeC->get_attribute("name");
   const xml_node::node_list& list_att = EnodeC->get_children();
   std::vector<attributeRef> _content;
   for(const auto& iter_int : list_att)
   {
      const auto* EnodeC1 = GetPointer<const xml_element>(iter_int);
      if(!EnodeC1)
      {
         continue;
      }
      const xml_text_node* txt_node = EnodeC1->get_child_text();
      _content.push_back(attributeRef(new attribute(EnodeC1->get_name(), txt_node->get_content())));
   }
   if(std::find(ordered_attributes.begin(), ordered_attributes.end(), att_name->get_value()) == ordered_attributes.end())
   {
      ordered_attributes.push_back(att_name->get_value());
   }
   if(!_content.empty())
   {
      attributes[att_name->get_value()] = attributeRef(new attribute(_content));
   }
   else
   {
      const xml_attribute* value_type_node = EnodeC->get_attribute("value_type");
      const xml_text_node* txt_node = EnodeC->get_child_text();
      attributes[att_name->get_value()] = attributeRef(new attribute(value_type_node->get_value(), txt_node->get_content()));
   }
}

void attribute::xwrite(xml_element* xml_node, const std::string& name)
{
   xml_element* attr_name = xml_node->add_child_element("attribute");
   attr_name->set_attribute("name", name);
   if(!has_list())
   {
      attr_name->set_attribute("value_type", get_value_type_str());
      attr_name->add_child_text(get_content_str());
   }
   else
   {
      for(auto& v : content_list)
      {
         xml_element* el = attr_name->add_child_element(v->get_value_type_str());
         el->add_child_text(v->get_content_str());
      }
   }
}

void library_manager::set_default_attributes()
{
#if HAVE_KOALA_BUILT
   std::vector<attributeRef> content;
   /// creating default schema
   ordered_attributes.push_back("time_unit");
   ordered_attributes.push_back("voltage_unit");
   ordered_attributes.push_back("current_unit");
   ordered_attributes.push_back("pulling_resistance_unit");
   ordered_attributes.push_back("leakage_power_unit");
   ordered_attributes.push_back("capacitive_load_unit");
   ordered_attributes.push_back("define");

   attributes["time_unit"] = attributeRef(new attribute(attribute::STRING, "1ps"));
   attributes["voltage_unit"] = attributeRef(new attribute(attribute::STRING, "1V"));
   attributes["current_unit"] = attributeRef(new attribute(attribute::STRING, "1uA"));
   attributes["pulling_resistance_unit"] = attributeRef(new attribute(attribute::STRING, "1kohm"));
   attributes["leakage_power_unit"] = attributeRef(new attribute(attribute::STRING, "1pW"));

   content.clear();
   content.push_back(attributeRef(new attribute(attribute::INT32, "1")));
   content.push_back(attributeRef(new attribute(attribute::STRING, "ff")));
   attributes["capacitive_load_unit"] = attributeRef(new attribute(content));

   content.clear();
   content.push_back(attributeRef(new attribute(attribute::STRING, "drive_strength")));
   content.push_back(attributeRef(new attribute(attribute::STRING, "cell")));
   content.push_back(attributeRef(new attribute(attribute::STRING, "float")));
   attributes["define"] = attributeRef(new attribute(content));
#endif
}

library_manager::library_manager(ParameterConstRef _Param, bool std) : Param(std::move(_Param)), is_std(std)
{
   set_default_attributes();
}

library_manager::library_manager(std::string library_name, ParameterConstRef _Param, bool std) : Param(std::move(_Param)), name(std::move(library_name)), is_std(std)
{
   set_default_attributes();
}

library_manager::~library_manager() = default;

void library_manager::xload(const xml_element* node, const library_managerRef& LM, const ParameterConstRef& Param, const target_deviceRef& device)
{
#ifndef NDEBUG
   int debug_level = Param->get_class_debug_level("library_manager");
#endif
   auto output_level = Param->getOption<int>(OPT_output_level);
   const xml_node::node_list& list_int = node->get_children();
   for(const auto& iter_int : list_int)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter_int);
      if(!EnodeC)
      {
         continue;
      }
      if(EnodeC->get_name() == "information")
      {
#if HAVE_FROM_LIBERTY
         const attribute_sequence::attribute_list& attr_list = EnodeC->get_attributes();
         for(attribute_sequence::attribute_list::const_iterator a = attr_list.begin(); a != attr_list.end(); ++a)
         {
            std::string key = (*a)->get_name();
            std::string value = (*a)->get_value();
            if(key == "liberty_file")
               LM->info[LIBERTY] = value;
         }
#endif
      }
      if(EnodeC->get_name() == "name")
      {
         const xml_text_node* text = EnodeC->get_child_text();
         LM->name = text->get_content();
      }
      else if(EnodeC->get_name() == "attribute")
      {
         attribute::xload(EnodeC, LM->ordered_attributes, LM->attributes);
      }
      else if(EnodeC->get_name() == "operating_conditions")
      {
      }
      else if(EnodeC->get_name() == "wire_load")
      {
      }
      else if(EnodeC->get_name() == "power_lut_template")
      {
      }
      else if(EnodeC->get_name() == "lu_table_template")
      {
      }
      else if(EnodeC->get_name() == "output_current_template")
      {
      }
      else if(EnodeC->get_name() == "cell")
      {
         technology_nodeRef fu_curr = technology_nodeRef(new functional_unit(iter_int));
         fu_curr->xload(EnodeC, fu_curr, Param, device);

         const auto cell_name = fu_curr->get_name();

         /// Check if a more recently characterized version of the same cell already exists in the library
         THROW_ASSERT(not LM->is_fu(cell_name), cell_name + " already present");
         LM->add(fu_curr);
      }
      else if(EnodeC->get_name() == "template")
      {
         technology_nodeRef fut_curr = technology_nodeRef(new functional_unit_template(iter_int));
         fut_curr->xload(EnodeC, fut_curr, Param, device);
         LM->add(fut_curr);
      }
#ifndef NDEBUG
      else if(debug_level >= DEBUG_LEVEL_VERBOSE)
      {
         THROW_WARNING("library_manager - not yet supported: " + EnodeC->get_name());
      }
#endif
   }

   if(output_level >= OUTPUT_LEVEL_MINIMUM)
   {
      unsigned int combinational = 0;
      unsigned int others = 0;
      unsigned int total = 0;
      for(auto& l : LM->fu_map)
      {
         /*
          * If the functional unit is a template skip the counting.
          */
         if(GetPointer<functional_unit>(l.second) == nullptr && GetPointer<functional_unit_template>(l.second))
         {
            continue;
         }
         total++;
         if(GetPointer<functional_unit>(l.second)->logical_type == functional_unit::COMBINATIONAL)
         {
            combinational++;
         }
         else
         {
            others++;
         }
      }
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Library Name     : " << LM->name);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "  Total cells    : " << total);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "  - combinational: " << combinational);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "  - others: " << others);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "");
   }
}

void library_manager::xwrite(xml_element* node, TargetDevice_Type dv_type)
{
   xml_element* library = node->add_child_element("library");

#if HAVE_FROM_LIBERTY
   xml_element* info_xml = library->add_child_element("information");
   for(std::map<unsigned int, std::string>::iterator i = info.begin(); i != info.end(); ++i)
   {
      if(i->first == LIBERTY)
         info_xml->set_attribute("liberty_file", i->second);
   }
#endif

   xml_element* xml_name = library->add_child_element("name");
   xml_name->add_child_text(name);

   for(const auto& ordered_attribute : ordered_attributes)
   {
      const attributeRef& attr = attributes[ordered_attribute];
      attr->xwrite(library, ordered_attribute);
   }

   for(auto& f : fu_map)
   {
      xml_element* xml_cell;
      if(GetPointer<functional_unit>(f.second))
      {
         xml_cell = library->add_child_element("cell");
      }
      else
      {
         xml_cell = library->add_child_element("template");
      }
      f.second->xwrite(xml_cell, f.second, Param, dv_type);
   }
}

std::string library_manager::get_library_name() const
{
   return name;
}

void library_manager::add(const technology_nodeRef& node)
{
   /// adding a cells invalidates the library view currently stored
   erase_info();
   std::string _name = node->get_name();
   fu_map[_name] = node;
}

void library_manager::update(const technology_nodeRef& fu_node)
{
   /// adding a cells invalidates the library view currently stored
   erase_info();
   std::string _name = fu_node->get_name();
   technology_nodeRef fu = fu_map[_name];
   technology_nodeRef node = fu_node;
   if(!GetPointer<functional_unit>(node))
   {
      node = GetPointer<functional_unit_template>(node)->FU;
   }
   auto* current_fu = GetPointer<functional_unit>(fu);
   if(!current_fu)
   {
      current_fu = GetPointer<functional_unit>(GetPointer<functional_unit_template>(fu)->FU);
   }
   THROW_ASSERT(current_fu, "unexpected condition");
   current_fu->ordered_attributes = GetPointer<functional_unit>(node)->ordered_attributes;
   current_fu->attributes = GetPointer<functional_unit>(node)->attributes;
   if(GetPointer<functional_unit>(node)->area_m)
   {
      current_fu->area_m = GetPointer<functional_unit>(node)->area_m;
   }
#if HAVE_EXPERIMENTAL
   if(GetPointer<functional_unit>(node)->layout_m)
      current_fu->layout_m = GetPointer<functional_unit>(node)->layout_m;
#endif
   const functional_unit::operation_vec& operations = GetPointer<functional_unit>(node)->get_operations();
   for(const auto& o : operations)
   {
      const operation* op = GetPointer<operation>(o);
      const technology_nodeRef op_fu = current_fu->get_operation(op->operation_name);
      THROW_ASSERT(op_fu, "Missing operation: " + op->operation_name + "-" + _name);
      if(op->time_m)
      {
         GetPointer<operation>(op_fu)->time_m = op->time_m;
      }
      if(GetPointer<functional_unit_template>(fu_node))
      {
         GetPointer<operation>(op_fu)->pipe_parameters = op->pipe_parameters;
      }
      if(GetPointer<functional_unit_template>(fu_node))
      {
         GetPointer<operation>(op_fu)->portsize_parameters = op->portsize_parameters;
      }
      GetPointer<operation>(op_fu)->bounded = op->bounded;
#if HAVE_EXPERIMENTAL
      if(op->power_m)
         GetPointer<operation>(op_fu)->power_m = op->power_m;
#endif
   }

#if HAVE_CIRCUIT_BUILT
   /// update the structural description, if specified
   if(GetPointer<functional_unit>(node) && GetPointer<functional_unit>(node)->CM)
   {
      current_fu->CM = GetPointer<functional_unit>(node)->CM;
   }
#endif
   if(GetPointer<functional_unit_template>(fu_node) && !GetPointer<functional_unit_template>(fu_node)->specialized.empty())
   {
      GetPointer<functional_unit_template>(fu)->specialized = GetPointer<functional_unit_template>(fu_node)->specialized;
   }
   if((GetPointer<functional_unit>(node) && !GetPointer<functional_unit>(node)->fu_template_name.empty()) || GetPointer<functional_unit_template>(fu_node))
   {
      current_fu->fu_template_name = GetPointer<functional_unit>(node)->fu_template_name;
   }
   if((GetPointer<functional_unit>(node) && !GetPointer<functional_unit>(node)->fu_template_parameters.empty()) || GetPointer<functional_unit_template>(fu_node))
   {
      current_fu->fu_template_parameters = GetPointer<functional_unit>(node)->fu_template_parameters;
   }
   if((GetPointer<functional_unit>(node) && !GetPointer<functional_unit>(node)->characterizing_constant_value.empty()) || GetPointer<functional_unit_template>(fu_node))
   {
      current_fu->characterizing_constant_value = GetPointer<functional_unit>(node)->characterizing_constant_value;
   }
   if((GetPointer<functional_unit>(node) && !GetPointer<functional_unit>(node)->memory_type.empty()) || GetPointer<functional_unit_template>(fu_node))
   {
      current_fu->memory_type = GetPointer<functional_unit>(node)->memory_type;
   }
   if((GetPointer<functional_unit>(node) && !GetPointer<functional_unit>(node)->channels_type.empty()) || GetPointer<functional_unit_template>(fu_node))
   {
      current_fu->channels_type = GetPointer<functional_unit>(node)->channels_type;
   }
   if((GetPointer<functional_unit>(node) && !GetPointer<functional_unit>(node)->memory_ctrl_type.empty()) || GetPointer<functional_unit_template>(fu_node))
   {
      current_fu->memory_ctrl_type = GetPointer<functional_unit>(node)->memory_ctrl_type;
   }
   if((GetPointer<functional_unit>(node) && !GetPointer<functional_unit>(node)->bram_load_latency.empty()) || GetPointer<functional_unit_template>(fu_node))
   {
      current_fu->bram_load_latency = GetPointer<functional_unit>(node)->bram_load_latency;
   }

   THROW_ASSERT(current_fu->characterization_timestamp <= GetPointer<functional_unit>(node)->characterization_timestamp, STR(current_fu->characterization_timestamp) + " vs " + STR(GetPointer<functional_unit>(node)->characterization_timestamp));
   current_fu->characterization_timestamp = GetPointer<functional_unit>(node)->characterization_timestamp;
}

bool library_manager::is_fu(const std::string& _name) const
{
   return fu_map.find(_name) != fu_map.end();
}

technology_nodeRef library_manager::get_fu(const std::string& _name) const
{
   THROW_ASSERT(is_fu(_name), "functional unit " + _name + " not stored");
   return fu_map.find(_name)->second;
}

size_t library_manager::get_gate_count() const
{
   return static_cast<unsigned int>(fu_map.size());
}

void library_manager::set_info(unsigned int type, const std::string& information)
{
   info[type] = information;
}

void library_manager::erase_info()
{
   info.clear();
}

std::string library_manager::get_info(info_t type, const TargetDevice_Type dv_type)
{
   if(!is_info(type))
   {
      switch(static_cast<info_t>(type))
      {
         case XML:
         {
            write_xml_technology_File(get_library_name() + ".xml", this, dv_type);
            break;
         }
#if HAVE_FROM_LIBERTY
         case LIBERTY:
         {
            write_lib_technology_File(get_library_name() + ".lib", this, dv_type);
            break;
         }
#endif
#if HAVE_EXPERIMENTAL
         case LEF:
         {
            write_lef_technology_File(get_library_name() + ".xml", this, dv_type);
            break;
         }
#if HAVE_LIBRARY_COMPILER
         case DB:
         {
            NOT_YET_IMPLEMENTED();
#if 0
            std::string lib_file = get_info(library_manager::LIBERTY);
            unsigned int output_level = Param->getOption<unsigned int>(OPT_output_level);
            PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "(library_manager) Library to be compiled: " << name << " - " << lib_file);
            LibraryCompilerWrapperRef LCW(new LibraryCompilerWrapper(Param, device, ""));
            std::string db_library = LCW->compile_library(name, lib_file);
            set_info(library_manager::DB, db_library);
#endif
            break;
         }
#endif
#endif
         default:
            THROW_ERROR("Not enough information to return the library information");
      }
   }
   THROW_ASSERT(is_info(type), "The information is not available");
   return info.find(type)->second;
}

bool library_manager::is_info(unsigned int type) const
{
   return info.find(type) != info.end();
}

bool library_manager::is_virtual() const
{
   return !is_std;
}

void library_manager::remove_fu(const std::string& _name)
{
   if(fu_map.find(_name) == fu_map.end())
   {
      return;
   }
   fu_map.erase(_name);
   erase_info();
}

void library_manager::set_dont_use(const std::string& _name)
{
   if(fu_map.find(_name) == fu_map.end())
   {
      return;
   }
   dont_use.insert(_name);
}

CustomOrderedSet<std::string> library_manager::get_dont_use_cells() const
{
   return dont_use;
}

size_t library_manager::get_dont_use_num() const
{
   return static_cast<unsigned int>(dont_use.size());
}

void library_manager::remove_dont_use(const std::string& _name)
{
   dont_use.erase(_name);
}
